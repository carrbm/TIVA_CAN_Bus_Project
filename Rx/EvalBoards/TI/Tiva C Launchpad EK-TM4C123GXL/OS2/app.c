/*
*********************************************************************************************************
*                                            EXAMPLE CODE
*
*               This file is provided as an example on how to use Micrium products.
*
*               Please feel free to use any application code labeled as 'EXAMPLE CODE' in
*               your application products.  Example code may be used as is, in whole or in
*               part, or may be used as a reference only. This file can be modified as
*               required to meet the end-product requirements.
*
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*
*               You can find our product's user manual, API reference, release notes and
*               more information at https://doc.micrium.com.
*               You can contact us at www.micrium.com.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                           APPLICATION CODE
*
*                                      Texas Instruments TM4C129x
*                                                on the
*                                             DK-TM4C129X
*                                           Development Kit
*       		Modified by Dr. Samir A. Rawashdeh, for the TM4C123GH6PM microcontroller on the 
*						TM4C123G Tiva C Series Launchpad (TM4C123GXL), November 2014.
*
* Filename      : app.c
* Version       : V1.00
* Programmer(s) : FF
*********************************************************************************************************
* Note(s)       : None.
*********************************************************************************************************
*/
/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#include  "app_cfg.h"
#include  <cpu_core.h>
#include  <os.h>

#include  "..\bsp\bsp.h"
#include  "..\bsp\bsp_led.h"
#include  "..\bsp\bsp_sys.h"

// SAR Addition
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "driverlib/interrupt.h"
#include "inc/hw_ints.h"
#include "tm4c123gh6pm.h"

#include "driverlib/can.h"
#include "inc/hw_can.h"
#include "bsp_int.h"

/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                           LOCAL CONSTANTS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*/


#define CAN0_CTL_R              (*((volatile uint32_t *)0x40040000))
#define CAN0_TST_R              (*((volatile uint32_t *)0x40040014))

/*$PAGE*/
/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

static  OS_STK       AppTaskStartStk[APP_CFG_TASK_START_STK_SIZE];
static  OS_STK       ReceiveMessageStk[APP_CFG_TASK_START_STK_SIZE];
//static  OS_STK       Task2Stk[APP_CFG_TASK_START_STK_SIZE];

tCANMsgObject message1;		//CAN message
unsigned char message1Data[8];
OS_FLAG_GRP *can_recv_flag = (OS_FLAG_GRP*)NULL;
 
/*
*********************************************************************************************************
*                                            LOCAL MACRO'S
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  AppTaskCreate         (void);
void CANIntHandler(void);
static void setupCan(void);
static  void  AppTaskStart          (void       *p_arg);
static  void  ReceiveMessage          (void       *p_arg);
//static  void  Task2          (void       *p_arg);

/*$PAGE*/
/*
*********************************************************************************************************
*                                               main()
*
* Description : Entry point for C code.
*
* Arguments   : none.
*
* Returns     : none.
*
* Note(s)     : (1) It is assumed that your code will call main() once you have performed all necessary
*                   initialization.
*********************************************************************************************************
*/

int  main (void)
{
#if (OS_TASK_NAME_EN > 0)
    CPU_INT08U  err;
#endif

#if (CPU_CFG_NAME_EN == DEF_ENABLED)
    CPU_ERR     cpu_err;
#endif

#if (CPU_CFG_NAME_EN == DEF_ENABLED)
    CPU_NameSet((CPU_CHAR *)"TM4C129XNCZAD",
                (CPU_ERR  *)&cpu_err);
#endif

    CPU_IntDis();                                               /* Disable all interrupts.                              */

    OSInit();                                                   /* Initialize "uC/OS-II, The Real-Time Kernel"          */

    OSTaskCreateExt((void (*)(void *)) AppTaskStart,           /* Create the start task                                */
                    (void           *) 0,
                    (OS_STK         *)&AppTaskStartStk[APP_CFG_TASK_START_STK_SIZE - 1],
                    (INT8U           ) APP_CFG_TASK_START_PRIO,
                    (INT16U          ) APP_CFG_TASK_START_PRIO,
                    (OS_STK         *)&AppTaskStartStk[0],
                    (INT32U          ) APP_CFG_TASK_START_STK_SIZE,
                    (void           *) 0,
                    (INT16U          )(OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR));

#if (OS_TASK_NAME_EN > 0)
    OSTaskNameSet(APP_CFG_TASK_START_PRIO, "Start", &err);
#endif
//------------------------------------------------------------------------------------------------------------------------------------
//CAN Setup
//------------------------------------------------------------------------------------------------------------------------------------
//Enable PORTE
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE))
	{}

		setupCan();							
    OSStart();                                                  /* Start multitasking (i.e. give control to uC/OS-II)   */

    while (1) {
        ;
    }
}


/*$PAGE*/
/*
*********************************************************************************************************
*                                           App_TaskStart()
*
* Description : Startup task example code.
*
* Arguments   : p_arg       Argument passed by 'OSTaskCreate()'.
*
* Returns     : none.
*
* Created by  : main().
*
* Notes       : (1) The first line of code is used to prevent a compiler warning because 'p_arg' is not
*                   used.  The compiler should not generate any code for this statement.
*********************************************************************************************************
*/

static  void  AppTaskStart (void *p_arg)
{
		CPU_INT32U  cpu_clk_freq;
    CPU_INT32U  cnts;
    (void)p_arg;                                                /* See Note #1                                              */


   (void)&p_arg;

    BSP_Init();                                                 /* Initialize BSP functions                             */

    cpu_clk_freq = BSP_SysClkFreqGet();                         /* Determine SysTick reference freq.                    */
    cnts         = cpu_clk_freq                                 /* Determine nbr SysTick increments                     */
                 / (CPU_INT32U)OS_TICKS_PER_SEC;

    OS_CPU_SysTickInit(cnts);
    CPU_Init();                                                 /* Initialize the uC/CPU services                       */

#if (OS_TASK_STAT_EN > 0)
    OSStatInit();                                               /* Determine CPU capacity                                   */
#endif

    Mem_Init();

#ifdef CPU_CFG_INT_DIS_MEAS_EN
    CPU_IntDisMeasMaxCurReset();
#endif


		BSP_LED_Toggle(0);
		OSTimeDlyHMSM(0, 0, 0, 200);
		BSP_LED_Toggle(0);
		BSP_LED_Toggle(1);
		OSTimeDlyHMSM(0, 0, 0, 200);
		BSP_LED_Toggle(1);
		BSP_LED_Toggle(2);
		OSTimeDlyHMSM(0, 0, 0, 200);    
		BSP_LED_Toggle(2);

		OSTimeDlyHMSM(0, 0, 1, 0);   

		AppTaskCreate();                                            /* Creates all the necessary application tasks.         */

    while (DEF_ON) {

        OSTimeDlyHMSM(0, 0, 0, 200);			

    }
}


/*
*********************************************************************************************************
*                                         AppTaskCreate()
*
* Description :  Create the application tasks.
*
* Argument(s) :  none.
*
* Return(s)   :  none.
*
* Caller(s)   :  AppTaskStart()
*
* Note(s)     :  none.
*********************************************************************************************************
*/

static  void  AppTaskCreate (void)
{
OSTaskCreate((void (*)(void *)) ReceiveMessage,           /* Create the second task                                */
                    (void           *) 0,							// argument
                    (OS_STK         *)&ReceiveMessageStk[APP_CFG_TASK_START_STK_SIZE - 1],
                    (INT8U           ) 5 );  						// Task Priority
                

//OSTaskCreate((void (*)(void *)) Task2,           /* Create the second task                                */
//                    (void           *) 0,							// argument
//                    (OS_STK         *)&Task2Stk[APP_CFG_TASK_START_STK_SIZE - 1],
//                    (INT8U           ) 6 );  						// Task Priority
         										
}


static  void  ReceiveMessage (void *p_arg)
{
		INT8U err;
		unsigned int colour[3];
		unsigned int type;
		unsigned char string[9];
		int i =0;
		float intensity;
		uint32_t count = 0;
    (void)p_arg;
	
    while (1) 
		{
			UARTprintf("\nWaiting for signal...");
			if(CANStatusGet(CAN0_BASE, CAN_STS_NEWDAT))//message1.ui32MsgID == 0x400)
			{
				//UARTprintf("\nnewdata");
				count++;
				CANMessageGet(CAN0_BASE, 1, &message1, true);   //get received data
				//OSFlagPost(can_recv_flag, 0x01, OS_FLAG_CLR, &err);
				//BSP_LED_Toggle(0);
				type = message1Data[0] * 0xFF;
				//UARTprintf("\n%d", type);
				if(type == 0)
				{
					for(i=0;i<3;i++)
					{
						BSP_LED_Off(i);
					}
					colour[0] = message1Data[1];
					colour[1] = message1Data[2];
					colour[2] = message1Data[3];
					if((int)colour[0] == 255)
					{
						UARTprintf("\nsetting red");
						BSP_LED_Toggle(2);
					}
					if((int)colour[1] == 255)
					{
						UARTprintf("\nsetting green");
						BSP_LED_Toggle(0);
					}
					if((int)colour[2] == 255)
					{
						UARTprintf("\nsetting blue");
						BSP_LED_Toggle(1);
					}
				}
				else
				{
					for(i=0;i<8;i++)
					{
						string[i] = message1Data[i];
					}
					string[8] = '\0';
					UARTprintf("\nreceived from master: %s", string);
				}
			}
			OSTimeDlyHMSM(0, 0, 0, 500);
	 }
}

static void setupCan()
{
	SysCtlPeripheralEnable(GPIO_PORTE_BASE);
	while(!SysCtlPeripheralReady(GPIO_PORTE_BASE))
	{}
		
	GPIOPinConfigure(GPIO_PE4_CAN0RX);
	GPIOPinConfigure(GPIO_PE5_CAN0TX);
	
	GPIOPinTypeCAN(GPIO_PORTE_BASE, GPIO_PIN_4 | GPIO_PIN_5);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN0);
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_CAN0))
	{}
	
	//CANIntRegister(CAN0_BASE, CANIntHandler);//CAN0); // use dynamic vector table allocation
	//CANIntEnable(CAN0_BASE, CAN_INT_MASTER | CAN_INT_ERROR | CAN_INT_STATUS);
	//IntEnable(INT_CAN0);
		
	CANInit(CAN0_BASE);
	CANBitRateSet(CAN0_BASE, SysCtlClockGet(), 250000);
	CANEnable(CAN0_BASE);
										
	message1.ui32MsgID = 0x400;
	message1.ui32MsgIDMask = 0x8FF;
	message1.ui32MsgLen = 0x08;
	message1.pui8MsgData = message1Data;
	CANMessageSet(CAN0_BASE, 1, &message1, MSG_OBJ_TYPE_RX);
	UARTprintf("\nCAN Bus initialized!");
	
	//CAN0_CTL_R |= CAN_CTL_TEST;
	//CAN0_TST_R |= CAN_TST_LBACK;
}

void CANIntHandler(void) 
{
	INT8U err;
	unsigned long status = CANIntStatus(CAN0_BASE, CAN_INT_STS_CAUSE); // read interrupt status
	UARTprintf("\nInt thrown!");
	if(status == CAN_INT_INTID_STATUS) 
	{ // controller status interrupt
		status = CANStatusGet(CAN0_BASE, CAN_STS_CONTROL);
		OSFlagPost(can_recv_flag, 0x10, OS_FLAG_SET, &err);
	} 
	else if(status == 1) 
	{ // msg object 1
		CANIntClear(CAN0_BASE, 1); // clear interrupt
		OSFlagPost(can_recv_flag, 0x01, OS_FLAG_SET, &err);
	} 
	else 
	{ // should never happen
		UARTprintf("Unexpected CAN bus interrupt\n");
	}

}
