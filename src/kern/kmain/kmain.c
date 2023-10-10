/*
 * Copyright (c) 2022
 * Computer Science and Engineering, University of Dhaka
 * Credit: CSE Batch 25 (starter) and Prof. Mosaddek Tushar
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <kmain.h>
#include <schedule.h>

#define STOP 1000000

TCB_TypeDef task[22], _sleep;
int count = 0;

int x = 10;

void task_sleep(void)
{
    int reboot_ = 0;
    printf("\nAll tasks has been completed.\nENTER 1 TO REBOOT DUOS. \nENTER 0 TO SHUTDOWN\n\r");
    uscanf("%d", &reboot_);
    if (reboot_ == 1)
        reboot();

    if (reboot_ == 1)
        ;
}

void Task(void)
{
    uint32_t value;
    uint32_t inc_count = 0;
    while (1)
    {
        uint16_t task_id = getpid() - 1000; /* It is an SVC call*/

        // critical region
        value = count;
        value++;

        // we check if some other tasks increase the count
        if (value != count + 1)
        {
            printf("Task %d running", task_id);
            printf("Error %d != %d\n\r", value, count + 1); /* It is an SVC call*/
        }
        else
        {
            // critical region
            // printf("Task %d running No Error %d == %d\n\r", task_id, value, count + 1); /* It is an SVC call*/
            count = value;
            inc_count++;
        }
        if (count >= STOP)
        {
            /* display how many increments it has successfully done!! */
            printf("Total increment done by task %d is: %d\n\r", task_id, inc_count);
            printf("Total increment done by task is: %d\n\r", inc_count);
            /* above is an SVC call */
            int fd = fopen("S_DISPLAY", O_WRDONLY);

            fprintf(fd, "%d\n", x);
            x = (x + 1) % 8;
            fclose(fd);

            break;
        }
    }
    exit();
}

void unprivileged_mode(void)
{
    // read operation from special register CPSR and SPSR
    __asm volatile("MRS R0, CONTROL");
    __asm volatile("ORRS R0, R0, #1");
    // write operation to special register CPSR and SPSR
    __asm volatile("MSR CONTROL, R0");
}

void __set_interrupt_priorities(void)
{
    __NVIC_SetPriority(SVCall_IRQn, 1);
    __NVIC_SetPriority(SysTick_IRQn, 0x2);
    // lowest priority given to PendSV
    __NVIC_SetPriority(PendSV_IRQn, 0xFF);
}

void kmain(void)
{
    __sys_init();
    __set_interrupt_priorities();

    fopen("STDIN FILENO", O_RDONLY);
    fopen("STDOUT FILENO", O_RDONLY);
    fopen("STDERR FILENO", O_RDONLY);

    int task_count = 20;

    for (int i = 0; i < task_count; i++)
    {
        task_create(task + i, Task, (uint32_t *)(TASK_STACK_START - (i * TASK_STACK_SIZE)));
    }

    task_create(&_sleep, task_sleep, (uint32_t *)(TASK_STACK_START - (task_count * TASK_STACK_SIZE)));

    initialize_queue();

    for (int i = 0; i < task_count; i++)
        add_to_ready_queue(task + i);
    set_sleeping_task(&_sleep);

    // going to user mode
    unprivileged_mode();
    // set pendsv before starting task
    set_task_pending(1);

    task_start();

    while (1)
        ;
}
