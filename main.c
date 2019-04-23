///////////////////////////////////////////////////////////////////////////////
// Vitor Finotti
//
// <project-url>
///////////////////////////////////////////////////////////////////////////////
//
// unit name:     DMA application using FreeRTOS
//
// description:
//
//    Application to be used on the ARM Cortex-M0 implementation for FPGA
//    available at https://github.com/vfinotti/cortex-m0-soft-microcontroller
//
//
///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Vitor Finotti
///////////////////////////////////////////////////////////////////////////////
// MIT
///////////////////////////////////////////////////////////////////////////////
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
///////////////////////////////////////////////////////////////////////////////

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* Hardware specific includes. */
#include "cm0fpga.h"

#define mainLED_TOGGLE_PATTERN 0xf0f0f0f0
#define mainIS_SIMULATION 1

/*-----------------------------------------------------------*/

/*
 * Perform any application specific hardware configuration.  The clocks,
 * memory, etc. are configured before main() is called.
 */
static void prvSetupHardware( void );

void vMainHandlerDMA( void *pvParameters );

SemaphoreHandle_t xBinarySemaphore;
/*-----------------------------------------------------------*/

int main( void )
{
    /* Prepare the hardware with the initial configuration. */
    /* prvSetupHardware(); */




    /* Start the tasks and timer running. */
    vTaskStartScheduler();


    /* If all is well, the scheduler will now be running, and the following
       line will never be reached.  If the following line does execute, then
       there was insufficient FreeRTOS heap memory available for the idle and/or
       timer tasks to be created.  See the memory management section on the
       FreeRTOS web site for more details. */
    for( ;; );
}
/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/
void vMainHandlerDMA( void *pvParameters )
{
    volatile unsigned long ulTrap;   /* memory access pattern receiver. Volatile to ensure ul is not
                                      * optimized away.
                                      */
    volatile unsigned long ulPeriod;  /* time interval for memory access. Volatile to ensure ul is
                                       * not optimized away.
                                       */

#if (mainIS_SIMULATION == 1)
    {
        ulPeriod = 1U; /*period for simulations in Xilinx Vivado tool*/
    }
#else
    {
        ulPeriod = 1000U; /* period for FPGA implementation; roughly 1 seconds for a 10MHz osc in CM0_DS */
    }
#endif

    const TickType_t xDelayMs = pdMS_TO_TICKS( ulPeriod );

    /* As per most tasks, this task is implemented in an infinite loop. */
    for( ;; )
    {
        // Calculate sine and cosine of pi/3
        CM0FPGA_CORDIC0->CONTROL_START = 0;
        CM0FPGA_CORDIC0->X = 1073741824; // 1 in Q1.30 notation
        CM0FPGA_CORDIC0->Y = 0;
        CM0FPGA_CORDIC0->Z = 715827883; // angle pi/3, since 2pi = 2^32
        CM0FPGA_CORDIC0->CONTROL_START = 1;

        // Calculate sine and cosine of pi/4
        CM0FPGA_CORDIC1->CONTROL_START = 0;
        CM0FPGA_CORDIC1->X = 1073741824; // 1 in Q1.30 notation
        CM0FPGA_CORDIC1->Y = 0;
        CM0FPGA_CORDIC1->Z = 536870912; // angle pi/4, since 2pi = 2^32
        CM0FPGA_CORDIC1->CONTROL_START = 1;

        // Calculate sine and cosine of pi/6
        CM0FPGA_CORDIC2->CONTROL_START = 0;
        CM0FPGA_CORDIC2->X = 1073741824; // 1 in Q1.30 notation
        CM0FPGA_CORDIC2->Y = 0;
        CM0FPGA_CORDIC2->Z = 357913941; // angle pi/6, since 2pi = 2^32
        CM0FPGA_CORDIC2->CONTROL_START = 1;

        // Calculate sine and cosine of pi
        CM0FPGA_CORDIC3->CONTROL_START = 0;
        CM0FPGA_CORDIC3->X = 1073741824; // 1 in Q1.30 notation
        CM0FPGA_CORDIC3->Y = 0;
        CM0FPGA_CORDIC3->Z = 4294967295; // angle pi, since 2pi = 2^32
        CM0FPGA_CORDIC3->CONTROL_START = 1;

        ulTrap = mainLED_TOGGLE_PATTERN; // memory access pattern (toggle LED)
        ulTrap++;                        // force toggle value to change value


        // Enabling and configuring DMA
        CM0FPGA_DMA->CSR = 0x0;                           // Pause is disabled
        CM0FPGA_DMA->INT_MSK_A = 0x1;                     // Enable interrupt for channel 0 on inta_o
        CM0FPGA_DMA->CH0_SZ = (0x0<<16                    // Chunck transfer size (in words - 4 bytes)
                               | 0x100);                  // Total transfer size (in words - 4 bytes)
        CM0FPGA_DMA->CH0_A0  = (0x40001000 & 0xfffffffc); // Source Address
        CM0FPGA_DMA->CH0_A1  = (0x40002000 & 0xfffffffc); // Destination Address
        /* CM0FPGA_DMA->CH0_AM0 = ();                     // Source address mask register */
        /* CM0FPGA_DMA->CH0_AM1 = ();                     // Destination address mask register */
        CM0FPGA_DMA->CH0_DESC = (0x00000000);             // Linked list descriptor pointer
        CM0FPGA_DMA->CH0_SWPTR = (0x00000000);            // Software pointer
        CM0FPGA_DMA->CH0_CSR = (1<<18                     // Enable IRQ when Done
                                | 0<<6                    // don't auto restart when Done
                                | 0<<5                    // Normal Mode
                                | 1<<4                    // Increment source address
                                | 1<<3                    // Increment destination address
                                | 1);                     // Channel enabled

        /* Delay for a period. This time a call to vTaskDelay() is used which places
           the task into the Blocked state until the delay period has expired. The
           parameter takes a time specified in ‘ticks’, and the pdMS_TO_TICKS() macro
           is used (where the xDelayMs constant is declared) to convert a period in
           milliseconds into an equivalent time in ticks. */
        vTaskDelay( xDelayMs );

        /* Use the semaphore to wait for the event. The semaphore was created
           before the scheduler was started, so before this task ran for the first
           time. The task blocks indefinitely, meaning this function call will only
           return once the semaphore has been successfully obtained - so there is
           no need to check the value returned by xSemaphoreTake(). */
        xSemaphoreTake( xBinarySemaphore, portMAX_DELAY );

    }
}
/*-----------------------------------------------------------*/

static void prvSetupHardware( void )
{
/* extern unsigned long _vStackTop[], _pvHeapStart[]; */
/* unsigned long ulInterruptStackSize; */

    /* Enable AHB clock for GPIO. */
    /* LPC_SYSCON->SYSAHBCLKCTRL |= ( 1 << 6 ); */

    /* Configure GPIO for LED output. */
    /* xGPIO0->DIR |= mainLED_BIT; */

    /* The size of the stack used by main and interrupts is not defined in
    the linker, but just uses whatever RAM is left.  Calculate the amount of
    RAM available for the main/interrupt/system stack, and check it against
    a reasonable number.  If this assert is hit then it is likely you don't
    have enough stack to start the kernel, or to allow interrupts to nest.
    Note - this is separate to the stacks that are used by tasks.  The stacks
    that are used by tasks are automatically checked if
    configCHECK_FOR_STACK_OVERFLOW is not 0 in FreeRTOSConfig.h - but the stack
    used by interrupts is not.  Reducing the conifgTOTAL_HEAP_SIZE setting will
    increase the stack available to main() and interrupts. */
    /* ulInterruptStackSize = ( ( unsigned long ) _vStackTop ) - ( ( unsigned long ) _pvHeapStart ); */
    /* configASSERT( ulInterruptStackSize > 350UL ); */

    /* Fill the stack used by main() and interrupts to a known value, so its
    use can be manually checked. */
    /* memcpy( ( void * ) _pvHeapStart, ucExpectedInterruptStackValues, sizeof( ucExpectedInterruptStackValues ) ); */
}
/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook( void )
{
	/* vApplicationMallocFailedHook() will only be called if
	configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
	function that will get called if a call to pvPortMalloc() fails.
	pvPortMalloc() is called internally by the kernel whenever a task, queue,
	timer or semaphore is created.  It is also called by various parts of the
	demo application.  If heap_1.c or heap_2.c are used, then the size of the
	heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
	FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
	to query the size of free heap space that remains (although it does not
	provide information on how the remaining heap might be fragmented). */
	taskDISABLE_INTERRUPTS();
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
	/* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
	to 1 in FreeRTOSConfig.h.  It will be called on each iteration of the idle
	task.  It is essential that code added to this hook function never attempts
	to block in any way (for example, call xQueueReceive() with a block time
	specified, or call vTaskDelay()).  If the application makes use of the
	vTaskDelete() API function (as this demo application does) then it is also
	important that vApplicationIdleHook() is permitted to return to its calling
	function, because it is the responsibility of the idle task to clean up
	memory allocated by the kernel to any task that has since been deleted. */
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected. */
	taskDISABLE_INTERRUPTS();
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationTickHook( void )
{
/* #if mainCHECK_INTERRUPT_STACK == 1 */
/* extern unsigned long _pvHeapStart[]; */

	/* This function will be called by each tick interrupt if
	configUSE_TICK_HOOK is set to 1 in FreeRTOSConfig.h.  User code can be
	added here, but the tick hook is called from an interrupt context, so
	code must not attempt to block, and only the interrupt safe FreeRTOS API
	functions can be used (those that end in FromISR()). */

	/* Manually check the last few bytes of the interrupt stack to check they
	have not been overwritten.  Note - the task stacks are automatically
	checked for overflow if configCHECK_FOR_STACK_OVERFLOW is set to 1 or 2
	in FreeRTOSConifg.h, but the interrupt stack is not. */
	/* configASSERT( memcmp( ( void * ) _pvHeapStart, ucExpectedInterruptStackValues, sizeof( ucExpectedInterruptStackValues ) ) == 0U ); */
/* #endif /\* mainCHECK_INTERRUPT_STACK *\/ */
}
