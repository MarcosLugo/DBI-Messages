#include <18F26K80.h>
#INCLUDE <STDLIB.H>
#FUSES   NOMCLR
#FUSES   WDT                         //No Watch Dog Timer
#FUSES   WDT2048      
#FUSES   NOXINST                     //Extended set extension and Indexed Addressing mode disabled (Legacy mode)
#FUSES   ECH_IO                      //High speed Osc, medium power 4MHz-16MHz
#FUSES   NOBROWNOUT                  //No brownout reset
#FUSES   WDT_NOSLEEP                 //Watch Dog Timer, disabled during SLEEP
#FUSES   PROTECT
#FUSES   PUT
#use     delay(clock=40000000,RESTART_WDT)

#define interruptTime 5
#define Led PIN_A0

#use rs232(baud=9600,parity=N,xmit=PIN_C6,rcv=PIN_C7,bits=8,stream=Serial ,restart_wdt,errors)
#use rs232(baud=19200,parity=N,xmit=PIN_B6,rcv=PIN_B7,bits=8,stream=IOX,restart_wdt,errors)

struct
{
   unsigned int1 customData; 
   unsigned int1 StatusData; 
   unsigned int1 MIMEin; 
   unsigned int1 MIMEout; 
   unsigned int1 MimeACKResponse;
   unsigned int1 rqstGoInfo;
   unsigned int1 rqstActuator; 
   unsigned int1 showGoInfo;
   unsigned int1 blockMIMEByACK;
   unsigned int1 showInputstatus;
   unsigned int1 sendInputstatus;
   unsigned int1 saveEepromDueInput;
   
}AllowedFuntions;

static int8 firmwareMajorVersion = 1;
static int8 firmwareMinorVersion = 1;
static int8 HardwareMajorVersion = 1;
static int8 HardwareMinorVersion = 0;


