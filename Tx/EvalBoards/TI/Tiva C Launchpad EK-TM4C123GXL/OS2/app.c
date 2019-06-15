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
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"

#include "driverlib/can.h"
#include "inc/hw_can.h"

//------------------------------------
#include "inc/tm4c123gh6pm.h"
#include "inc/hw_can.h"
#include "inc/hw_ints.h"
#include "inc/hw_types.h"
#include "driverlib/can.h"
#include "driverlib/interrupt.h"
#include <string.h> 
#include "inc/rgb.h"
//------------------------------------

/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/
//#define receiver_address 0x17FF00F0

#define CAN0_CTL_R              (*((volatile uint32_t *)0x40040000))
#define CAN0_TST_R              (*((volatile uint32_t *)0x40040014))
#define SwitchPressed	0x01

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


/*$PAGE*/
/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

static  OS_STK       AppTaskStartStk[APP_CFG_TASK_START_STK_SIZE];
static  OS_STK       SendMessageStk[APP_CFG_TASK_START_STK_SIZE];
static  OS_STK       KeypadScanStk[APP_CFG_TASK_START_STK_SIZE];

static	INT8U				 err;


tCANMsgObject messageArray[16];		//array of CAN message objects
INT8U messageData[16][8] = {0};		//matrix of CAN message data
unsigned char *messageArrayData[16];	//pointer to message data 


//tCANMsgObject message1;		//CAN message
//INT8U data[8] = {0};			//message data
//unsigned char *message1Data = (unsigned char *)&data;		//pointer to message1Data


OS_FLAG_GRP *ButtonStatus;


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
static  void  AppTaskStart          (void       *p_arg);
static  void  SendMessage          (void       *p_arg);
static  void  KeypadScan          (void       *p_arg);

static void canSetup(void);
static void CANMessageInit(void);
static void KeyPadInit(void);


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

		ButtonStatus = OSFlagCreate(0x00, &err);
		KeyPadInit();
		canSetup();	
		CANMessageInit();								
									
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
OSTaskCreate((void (*)(void *)) SendMessage,           /* Create the first task                                */
                    (void           *) 0,							// argument
                    (OS_STK         *)&SendMessageStk[APP_CFG_TASK_START_STK_SIZE - 1],
                    (INT8U           ) 8 );  						// Task Priority        										
										
OSTaskCreate((void (*)(void *)) KeypadScan,           /* Create the second task                                */
                    (void           *) 0,							// argument
                    (OS_STK         *)&KeypadScanStk[APP_CFG_TASK_START_STK_SIZE - 1],
                    (INT8U           ) 6);  						// Task Priority
}

//---------------------------------------------------------------------------------------------------------------------------
//Task 1 : Sends CAN Message
//---------------------------------------------------------------------------------------------------------------------------
static  void  SendMessage (void *p_arg)
{
		OS_FLAGS flags;
		INT8U err;
	
   (void)p_arg;

    while (1) {              
			
			flags = OSFlagQuery(ButtonStatus, &err);
			if (flags !=0)
			{ 
				CANMessageSet(CAN0_BASE, 1,  &messageArray[flags-1], MSG_OBJ_TYPE_TX);
				UARTprintf("\nsent message: %d",flags);
				OSFlagPost(ButtonStatus, 0xFF, OS_FLAG_CLR, &err); // clear all flags
				BSP_LED_Toggle(1);		//0:green		1:blue		2:red		3:all on	
			}
			else
			{
				UARTprintf("\nNo data to send. Idle wait...");
			}
			OSTimeDlyHMSM(0, 0, 0, 250);
		}
}

//---------------------------------------------------------------------------------------------------------------------------
// Task 2: Keypad Scan
//---------------------------------------------------------------------------------------------------------------------------
static  void  KeypadScan (void *p_arg)
{
		INT32U count = 0;
   (void)p_arg;
    while (1) 
			{		
					if (count%4 == 0)
					{
						
						GPIO_PORTB_DATA_R &= 0xFE;
						OSTimeDlyHMSM(0, 0, 0, 20);
						if (GPIO_PORTB_DATA_R == 0xEE)
						{
							OSFlagPost(ButtonStatus,0x01,OS_FLAG_SET, &err);
						}
						else if (GPIO_PORTB_DATA_R == 0xDE)
						{
							OSFlagPost(ButtonStatus,0x02,OS_FLAG_SET, &err);
						}
						else if (GPIO_PORTB_DATA_R == 0xBE)
						{
							OSFlagPost(ButtonStatus,0x03,OS_FLAG_SET, &err);
						}
						else if (GPIO_PORTB_DATA_R == 0x7E)
						{
							OSFlagPost(ButtonStatus,0x04,OS_FLAG_SET, &err);
						}
					}
					else if (count%4 == 1)
					{
						GPIO_PORTB_DATA_R &= 0xFD;
						OSTimeDlyHMSM(0, 0, 0, 20);
						if (GPIO_PORTB_DATA_R == 0xED)
						{
							OSFlagPost(ButtonStatus,0x05,OS_FLAG_SET, &err);
						}
						else if (GPIO_PORTB_DATA_R == 0xDD)
						{
							OSFlagPost(ButtonStatus,0x06,OS_FLAG_SET, &err);
						}
						else if (GPIO_PORTB_DATA_R == 0xBD)
						{
							OSFlagPost(ButtonStatus,0x07,OS_FLAG_SET, &err);
						}
						else if (GPIO_PORTB_DATA_R == 0x7D)
						{
							OSFlagPost(ButtonStatus,0x08,OS_FLAG_SET, &err);
						}
					}
					else if (count%4 == 2)
					{
						GPIO_PORTB_DATA_R &= 0xFB;
						OSTimeDlyHMSM(0, 0, 0, 20);
						if (GPIO_PORTB_DATA_R == 0xEB)
						{
							OSFlagPost(ButtonStatus,0x09,OS_FLAG_SET, &err);
						}
						else if (GPIO_PORTB_DATA_R == 0xDB)
						{
							OSFlagPost(ButtonStatus,0x0A,OS_FLAG_SET, &err);
						}
						else if (GPIO_PORTB_DATA_R == 0xBB)
						{
							OSFlagPost(ButtonStatus,0x0B,OS_FLAG_SET, &err);
						}
						else if (GPIO_PORTB_DATA_R == 0x7B)
						{
							OSFlagPost(ButtonStatus,0x0C,OS_FLAG_SET, &err);
						}
					}
					else if (count%4 == 3)
					{
						GPIO_PORTB_DATA_R &= 0xF7;
						OSTimeDlyHMSM(0, 0, 0, 20);
						if (GPIO_PORTB_DATA_R == 0xE7)
						{
							OSFlagPost(ButtonStatus,0x0D,OS_FLAG_SET, &err);
						}
						else if (GPIO_PORTB_DATA_R == 0xD7)
						{
							OSFlagPost(ButtonStatus,0x0E,OS_FLAG_SET, &err);
						}
						else if (GPIO_PORTB_DATA_R == 0xB7)
						{
							OSFlagPost(ButtonStatus,0x0F,OS_FLAG_SET, &err);
						}
						else if (GPIO_PORTB_DATA_R == 0x77)
						{
							OSFlagPost(ButtonStatus,0x10,OS_FLAG_SET, &err);
						}
					}
					
					GPIO_PORTB_DATA_R	|= 0xFF;
					count++;
					OSTimeDlyHMSM(0, 0, 0, 25);
			}
			
}

/*************************************************************************/
/*  Function Definitions                                                 */
/*************************************************************************/

//CAN Initialization

static void canSetup(void)
{
//Enable PORTE
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE))
	{}
		
// Configure the GPIO pin muxing to select CAN0 functions for these pins.
// This step selects which alternate function is available for these pins.
	GPIOPinConfigure(GPIO_PE4_CAN0RX);
	GPIOPinConfigure(GPIO_PE5_CAN0TX);
		
//Configure the pins for CAN
	GPIOPinTypeCAN(GPIO_PORTE_BASE, GPIO_PIN_4 | GPIO_PIN_5);
		
// Enable the CAN0 module.
	SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN0);
	while(!SysCtlPeripheralReady(SYSCTL_PERIPH_CAN0))
	{}
		
// Reset the state of all the message objects and the state of the CAN
// module to a known state.
	CANInit(CAN0_BASE);
		
// Configure the controller for 250 Kbit operation.
	CANBitRateSet(CAN0_BASE, SysCtlClockGet(), 250000);
		
//Start CAN0 peripheral
	CANEnable(CAN0_BASE);
										
//Set CAN0 to looback test mode
	CAN0_CTL_R = CAN0_CTL_R | CAN_CTL_TEST;
	CAN0_TST_R = CAN0_TST_R | CAN_TST_LBACK;
}

// Keypad Initialization

static void KeyPadInit(void)
{
  SYSCTL_RCGCGPIO_R |= 0x0002;        // enable Port B
  GPIO_PORTB_DEN_R |= 0xFF;        // enable digital I/O on Port B
  GPIO_PORTB_DIR_R = 0x0F;       // make PB
	GPIO_PORTB_PUR_R = 0xF0;
	
  GPIO_PORTB_PCTL_R = 0;
  GPIO_PORTB_AFSEL_R = 0;     // disable alternate functionality on PA
  GPIO_PORTB_AMSEL_R = 0;     // disable analog functionality on PA
	GPIO_PORTB_ODR_R = 0x0F;
}

// CAN MessageInitialization

static void setupCANData(void)
{
	// Message 0, Device 1: Turn on red LED
	messageData[0][0] = 0x00; // the type of message to send (LED or printf)
	messageData[0][1] = 0xFF; // red RGB value
	messageData[0][2] = 0x00; // green RGB value
	messageData[0][3] = 0x00; // blue RGB value
	messageData[0][4] = 0xFF; // intensity
	
	// Message 1, Device 1: Turn on green LED
	messageData[1][0] = 0x00; // the type of message to send (LED or printf)
	messageData[1][1] = 0x00; // red RGB value
	messageData[1][2] = 0xFF; // green RGB value
	messageData[1][3] = 0x00; // blue RGB value
	messageData[1][4] = 0xFF; // intensity
	
	// Message 2, Device 1: Turn on blue LED
	messageData[2][0] = 0x00; // the type of message to send (LED or printf)
	messageData[2][1] = 0x00; // red RGB value
	messageData[2][2] = 0x00; // green RGB value
	messageData[2][3] = 0xFF; // blue RGB value
	messageData[2][4] = 0xFF; // intensity
	
	// Message 3, Device 1: Turn on yellow LED
	messageData[3][0] = 0x00; // the type of message to send (LED or printf)
	messageData[3][1] = 0xFF; // red RGB value
	messageData[3][2] = 0XFF; // green RGB value
	messageData[3][3] = 0X00; // blue RGB value
	messageData[3][4] = 0xFF; // intensity
	
	// Message 4, Device 2: Turn on red LED
	messageData[4][0] = 0x00; // the type of message to send (LED or printf)
	messageData[4][1] = 0xFF; // red RGB value
	messageData[4][2] = 0x00; // green RGB value
	messageData[4][3] = 0x00; // blue RGB value
	messageData[4][4] = 0xFF; // intensity
	
	// Message 5, Device 2: Turn on green LED
	messageData[5][0] = 0x00; // the type of message to send (LED or printf)
	messageData[5][1] = 0x00; // red RGB value
	messageData[5][2] = 0xFF; // green RGB value
	messageData[5][3] = 0x00; // blue RGB value
	messageData[5][4] = 0xFF; // intensity
	
	// Message 6, Device 2: Turn on blue LED
	messageData[6][0] = 0x00; // the type of message to send (LED or printf)
	messageData[6][1] = 0x00; // red RGB value
	messageData[6][2] = 0x00; // green RGB value
	messageData[6][3] = 0xFF; // blue RGB value
	messageData[6][4] = 0xFF; // intensity
	
	// Message 7, Device 2: Turn on yellow LED
	messageData[7][0] = 0x00; // the type of message to send (LED or printf)
	messageData[7][1] = 0xFF; // red RGB value
	messageData[7][2] = 0xFF; // green RGB value
	messageData[7][3] = 0x00; // blue RGB value
	messageData[7][4] = 0xFF; // intensity
	
	// Message 8, Device 3: Turn on red LED
	messageData[8][0] = 0x00; // the type of message to send (LED or printf)
	messageData[8][1] = 0xFF; // red RGB value
	messageData[8][2] = 0x00; // green RGB value
	messageData[8][3] = 0x00; // blue RGB value
	messageData[8][4] = 0xFF; // intensity
	
	// Message 9, Device 3: Turn on green LED
	messageData[9][0] = 0x00; // the type of message to send (LED or printf)
	messageData[9][1] = 0x00; // red RGB value
	messageData[9][2] = 0xFF; // green RGB value
	messageData[9][3] = 0x00; // blue RGB value
	messageData[9][4] = 0xFF; // intensity
	
	// Message 10, Device 3: Turn on blue LED
	messageData[10][0] = 0x00; // the type of message to send (LED or printf)
	messageData[10][1] = 0x00; // red RGB value
	messageData[10][2] = 0x00; // green RGB value
	messageData[10][3] = 0xFF; // blue RGB value
	messageData[10][4] = 0xFF; // intensity
	
	// Message 11, Device 3: Turn on yellow LED
	messageData[11][0] = 0x00; // the type of message to send (LED or printf)
	messageData[11][1] = 0xFF; // red RGB value
	messageData[11][2] = 0xFF; // green RGB value
	messageData[11][3] = 0x00; // blue RGB value
	messageData[11][4] = 0xFF; // intensity
	
	/***************************************************************************************/
	
	// Message 12, Device 1: Send "HUSKIEOS"
	messageData[12][0] = 0x48; // nonzero value so printf (H)
	messageData[12][1] = 0x55; // U
	messageData[12][2] = 0x53; // S
	messageData[12][3] = 0x4B; // K
	messageData[12][4] = 0x49; // I
	messageData[12][5] = 0x45; // E
	messageData[12][6] = 0x4F; // O
	messageData[12][7] = 0x53; // S
	
	// Message 13, Device 2: Send "MAGICBUS"
	messageData[13][0] = 0x4D; // nonzero value so printf (M)		
	messageData[13][1] = 0x41; // A															
	messageData[13][2] = 0x47; // G															
	messageData[13][3] = 0x49; // I															
	messageData[13][4] = 0x43; // C															
	messageData[13][5] = 0x42; // B															
	messageData[13][6] = 0x55; // U															
	messageData[13][7] = 0x53; // S															
	
	// Message 14, Device 3: Send "DEARBORN"
	messageData[14][0] = 0x44; // nonzero value so printf (D)
	messageData[14][1] = 0x45; // E
	messageData[14][2] = 0x41; // A
	messageData[14][3] = 0x52; // R
	messageData[14][4] = 0x42; // B
	messageData[14][5] = 0x4F; // O
	messageData[14][6] = 0x52; // R
	messageData[14][7] = 0x4E; // N
	
	// Message 15, Device 3: Send "JABCDCRP"
	messageData[15][0] = 0x4A; // nonzero value so printf (J)
	messageData[15][1] = 0x41; // A
	messageData[15][2] = 0x42; // B
	messageData[15][3] = 0x43; // C
	messageData[15][4] = 0x44; // D
	messageData[15][5] = 0x43; // C
	messageData[15][6] = 0x52; // R
	messageData[15][7] = 0x50; // P
}

static void CANMessageInit(void)
{
	static INT8U i;
	INT16U addr[3] = {0x200, 0x400, 0x600};
	static INT8U count = 0;
	setupCANData();
	for(i=0; i<16;i++)
	{
			messageArrayData[i] = (unsigned char *)&messageData[i];
			messageArray[i].ui32MsgLen = 0x08;
			messageArray[i].pui8MsgData = messageArrayData[i];
			
	}
	messageArray[0].ui32MsgID = addr[0];
	messageArray[1].ui32MsgID = addr[0];
	messageArray[2].ui32MsgID = addr[0];
	messageArray[3].ui32MsgID = addr[0];
	
	messageArray[4].ui32MsgID = addr[1];
	messageArray[5].ui32MsgID = addr[1];
	messageArray[6].ui32MsgID = addr[1];
	messageArray[7].ui32MsgID = addr[1];
	
	messageArray[8].ui32MsgID = addr[2];
	messageArray[9].ui32MsgID = addr[2];
	messageArray[10].ui32MsgID = addr[2];
	messageArray[11].ui32MsgID = addr[2];
	
	messageArray[12].ui32MsgID = addr[0];
	messageArray[13].ui32MsgID = addr[1];
	messageArray[14].ui32MsgID = addr[2];
	messageArray[15].ui32MsgID = addr[2];
}
