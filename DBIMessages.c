#include <DBIMessages.h>
#include <Eeprom.h>
#include <RS232_HOS.c>
#include <ActuadorRemoto.c>
#include <MessageFromPC.c>
#include <MessageFormServer.c>
#include <Eeprom.c>
#include <EntradaDigital.c>

static unsigned int16 deviceType = 0x1000;

//Varialbles para mostrar mensaje en string
char cdmMessage[70];
unsigned int8 cmdSize = 0;

char GoInfo[255];
unsigned int8 GoInfoSize = 0;

unsigned int16 timeToShow = 5000;
unsigned int1  showDevice = true;

HOS_DeviceData GoDeviceInfo;

#int_TIMER2
void  TIMER2_isr(void) 
{
   //interrupcion cada 5 ms
   HOS_Set_Time();
   Actuator_SetTime();
   InputSetTime();
   
   if(GoIOXstatus.synchronized == true && showDevice == true)
   {
      timeToShow -= interruptTime;
   }
}

#int_RDA
void  RDA_isr(void) 
{
   if(kbhit(Serial))
   {
      unsigned int8 dato = getc(serial);
      set_BufferPC(dato);
   }
   
}

#int_RDA2
void  RDA2_isr(void) 
{
   if(kbhit(IOX))
   {
      unsigned int8 dato = getc(IOX);
      HOS_Set_Buffer(dato);
   }
}

Variables upDateVariables();

void setup(void)
{
   setup_timer_2(T2_DIV_BY_16,195,16);      //313 us overflow, 5.0 ms interrupt
   setup_comparator(NC_NC_NC_NC);           //This device COMP currently not supported by the PICWizard
   enable_interrupts(INT_TIMER2);
   enable_interrupts(INT_RDA);
   enable_interrupts(INT_RDA2);
   enable_interrupts(GLOBAL);
   setup_wdt(WDT_ON);
   
   set_tris_c(0);
   set_tris_a(0);
   output_low(led);
   output_bit(Relay,0);
   for(unsigned int16 i = 0; i < 10; i++)
   {
      output_toggle(led);
      delay_ms(25);
      restart_wdt();
      delay_ms(25);
      restart_wdt();
      delay_ms(25);
      restart_wdt();
      delay_ms(25);
      restart_wdt();
   }
   
   if(EepromInit() == false)  //regresa true si encontro valor valido, si no regresa false y los valores inicializados por default
   {
      //fprintf(Serial,"Eeprom Valida%c%c",0x1f,0x03);
   }
   
   AllowedFuntions.customData      = ValoresIniciales.CScustomEnabled;
   AllowedFuntions.StatusData      = ValoresIniciales.CSstatusEnabled;
   AllowedFuntions.MIMEin          = ValoresIniciales.MIMEinEnabled;
   AllowedFuntions.MIMEout         = ValoresIniciales.MIMEoutEnabled;
   AllowedFuntions.MimeACKResponse = ValoresIniciales.MIMEautoResponseACK;
   AllowedFuntions.blockMIMEByACK  = ValoresIniciales.MIMEWaitACKtoSend;
   AllowedFuntions.rqstGoInfo      = ValoresIniciales.HOSrqstEnabled;
   AllowedFuntions.showGoInfo      = ValoresIniciales.HOSautoShowEnabled;
   AllowedFuntions.rqstActuator    = ValoresIniciales.ACTenableClientRequest;
   AllowedFuntions.showInputstatus = ValoresIniciales.InputEnableShow;
   AllowedFuntions.sendInputstatus = ValoresIniciales.InputEnablestatus;
   AllowedFuntions.saveEepromDueInput = ValoresIniciales.InputEnableSaveEeprom;
   GoDeviceInfo.Ignition = false;;
}

void main()
{
   setup();   
   Actuator_Init();
   HOS_Init();
   InputInit();
   
   cmdSize = sprintf(cdmMessage, "The DBI is turned ON");
   sendToPC(DBIInitialized,cmdSize,&cdmMessage);

   while(true)
   {      
      if(pc_result != free)
      {
         switch(pc_result)
         {
            case msjOK:
            {
               switch(msgFromPC.comando)
               {
                  case TextMsg:
                  {
                     if(AllowedFuntions.MIMEout == true)
                     {
                           if(GoIOXstatus.MIMESent < GoIOXstatus.maxMIMEs)
                           {
                              if(GoIOXstatus.synchronized == true)
                              {
                                 if(msgFromPC.Longitud > 200)
                                 {
                                    cmdSize = sprintf(cdmMessage, "Data Length Not Allowed");
                                    sendToPC(LengthInvalid,cmdSize,&cdmMessage);
                                 }
                                 else if(GoIOXstatus.WattingACKmimeFromServer == false || AllowedFuntions.blockMIMEByACK == false)
                                 {
                                    cmdSize = sprintf(cdmMessage, "MIME Message Received");
                                    sendToPC(TextMsgReceived,cmdSize,&cdmMessage); 
                                    SentFormatMIMEPacket(MimeMsgFromPC, msgFromPC.Longitud, &msgFromPC.Dato);
                                 }
                                 else
                                 {
                                    cmdSize = sprintf(cdmMessage, "Waiting MIME ACK");
                                    sendToPC(MIMEWattingACK,cmdSize,&cdmMessage); 
                                 }
                              }
                              else
                              {
                                 cmdSize = sprintf(cdmMessage, "The IOX is Not Synchronized");
                                 sendToPC(ShowIOXstatus,cmdSize,&cdmMessage);
                              }   
                           }
                           else
                           {
                              cmdSize = sprintf(cdmMessage, "Wait %lu S To Send a Text Message Again",(600000-GoIOXstatus.MIMETime)/1000);
                              sendToPC(TextMsgLimitReached,cmdSize,&cdmMessage);
                           }
                     }
                     else
                     {
                        cmdSize = sprintf(cdmMessage, "MIME Message to Server Not Allowed");
                        sendToPC(CommandNotAllowed,cmdSize,&cdmMessage);
                     }
                     break;
                  }
                  case SendCustomData:
                  {
                     if(AllowedFuntions.customData == true)
                     {
                        if(GoIOXstatus.customsSent < GoIOXstatus.maxCustoms)
                        {
                           if(msgFromPC.Longitud > 27)
                           {
                              cmdSize = sprintf(cdmMessage, "Data Length Not Allowed");
                              sendToPC(LengthInvalid,cmdSize,&cdmMessage);
                           }
                           else if(GoIOXstatus.WattingACK == 0)
                           {
                              cmdSize = sprintf(cdmMessage, "Custom Data Received");
                              sendToPC(CustomDataReceived,cmdSize,&cdmMessage); 
                              if(GoIOXstatus.synchronized == true)
                              {
                                 //enviamos  custom
                                 SentFreeFormatThirdPartyData(msgFromPC.Longitud, &msgFromPC.Dato); 
                                 sentByPC = true;
                              }
                              else
                              {
                                 cmdSize = sprintf(cdmMessage, "The IOX is Not Synchronized");
                                 sendToPC(ShowIOXstatus,cmdSize,&cdmMessage);
                              }
                           }
                           else if(GoIOXstatus.WattingACK == 1)
                           {
                              cmdSize = sprintf(cdmMessage, "Watting Custom Data ACK");
                              sendToPC(CSWattingACK,cmdSize,&cdmMessage); 
                           }
                           else if(GoIOXstatus.WattingACK == 2)
                           {
                              cmdSize = sprintf(cdmMessage, "Watting Status Data ACK");
                              sendToPC(CSWattingACK,cmdSize,&cdmMessage); 
                           }
                        }
                        else
                        {
                           cmdSize = sprintf(cdmMessage, "Wait %lu S To Send a Custom Data Again",(600000-GoIOXstatus.CustomsTime)/1000);
                           sendToPC(CustomDataLimitReached,cmdSize,&cdmMessage);
                        }
                     }
                     else
                     {
                        cmdSize = sprintf(cdmMessage, "Custom Data Not Supported");
                        sendToPC(CommandNotAllowed,cmdSize,&cdmMessage);
                     }
                     break;
                  }
                  case SendStatusData:
                  {
                     if(AllowedFuntions.StatusData == true)
                     {
                        if(msgFromPC.Longitud != 6)
                        {
                           cmdSize = sprintf(cdmMessage, "Data Length Not Allowed");
                           sendToPC(LengthInvalid,cmdSize,&cdmMessage);
                        }
                        else if(GoIOXstatus.WattingACK == 0)
                        {
                           cmdSize = sprintf(cdmMessage, "Status Data Received");
                           sendToPC(StatusDataReceived,cmdSize,&cdmMessage); 
                           
                           if(GoIOXstatus.synchronized == true)
                           {
                              unsigned int16 idStatus = make16(msgFromPC.Dato[0],msgFromPC.Dato[1]);
                              unsigned int32 dataStatus = make32(msgFromPC.Dato[2],msgFromPC.Dato[3],msgFromPC.Dato[4],msgFromPC.Dato[5]);
                              SentThirdPartyDataStatusData(0,idStatus, dataStatus); 
                              sentByPC = true;
                           }
                           else
                           {
                              cmdSize = sprintf(cdmMessage, "The IOX is Not Synchronized");
                              sendToPC(ShowIOXstatus,cmdSize,&cdmMessage);
                           }
                        }
                        else if(GoIOXstatus.WattingACK == 1)
                        {
                           cmdSize = sprintf(cdmMessage, "Waiting Custom Data ACK");
                           sendToPC(CSWattingACK,cmdSize,&cdmMessage); 
                        }
                        else if(GoIOXstatus.WattingACK == 2)
                        {
                           cmdSize = sprintf(cdmMessage, "Waiting Status Data ACK");
                           sendToPC(CSWattingACK,cmdSize,&cdmMessage); 
                        }
                     }
                     else
                     {
                        cmdSize = sprintf(cdmMessage, "Status Data Not Supported");
                        sendToPC(CommandNotAllowed,cmdSize,&cdmMessage);
                     }
                     break;
                  }
                  case StopWattingCSACK:
                  {
                     cmdSize = sprintf(cdmMessage, "Waiting Custom and Status Data Cancelled");
                     sendToPC(CSACKWattingStopped,cmdSize,&cdmMessage); 
                     GoIOXstatus.n_timeOutCustom = 0;
                     GoIOXstatus.WattingACK = 0; 
                     break;
                  }
                  case PingRequest:
                  {
                     cmdSize = sprintf(cdmMessage, "Ping Ok");
                     sendToPC(Pingresponse,cmdSize,&cdmMessage); 
                     break;
                  }
                  case GetVehicleInfo:
                  {
                     if(AllowedFuntions.rqstGoInfo == true)
                     {
                       if(GoIOXstatus.synchronized == true)
                        {
                           if(GoInfoDataReady == true)
                           {
                              GoInfoSize = 0;
                              for(int8 i= 0; i< GoInfo_Leng; i++)
                              {
                                 char cdmMessage[2];
                                 sprintf(cdmMessage, "%X",GoInfo_Data[i]);
                                 GoInfo[GoInfoSize++] = cdmMessage[0];
                                 GoInfo[GoInfoSize++] = cdmMessage[1];
                              }
                              sendToPC(ShowVehicleInfo,GoInfoSize,&GoInfo);
                              GoIOXstatus.n_TimetoRqstHOS = 0 ;
                              GoInfoDataReady = false;
                              GoInfo_Leng = 0;
                           }
                           else
                           {
                              cmdSize = sprintf(cdmMessage, "No Go Info Data Available");
                              sendToPC(ShowGoInfoError,cmdSize,&cdmMessage);
                           }
                           SentRequestDeviceDataMessage();
                        }
                        else
                        {
                           cmdSize = sprintf(cdmMessage, "The IOX is Not Synchronized");
                           sendToPC(ShowIOXstatus,cmdSize,&cdmMessage);
                        }
                     }
                     else
                     {
                        cmdSize = sprintf(cdmMessage, "Go Info Request Not Supported");
                        sendToPC(CommandNotAllowed,cmdSize,&cdmMessage);
                     }
                     break;
                  }
                  case StopWattingMIMEACK:
                  {
                     cmdSize = sprintf(cdmMessage, "MIME Waiting ACK From Server Cancelled");
                     sendToPC(MIMEACKWattingStopped,cmdSize,&cdmMessage); 
                     GoIOXstatus.n_timeOutACKFromServer = 0;
                     GoIOXstatus.WattingACKmimeFromServer = false;
                     break;
                  }
                  case GetActuatorMode:
                  {
                     if(Actuator.OutputMode == ActuadorTemporizado)
                     {
                         cmdSize = sprintf(cdmMessage, "The Actuator is Configured as Timed Output");
                         sendToPC(ShowActuatorMode,cmdSize,&cdmMessage); 
                     }
                     else if(Actuator.OutputMode == ActuadorONOFF)
                     {
                         cmdSize = sprintf(cdmMessage, "The Actuator is Configured as Output");
                         sendToPC(ShowActuatorMode,cmdSize,&cdmMessage); 
                     }
                     else
                     {
                         cmdSize = sprintf(cdmMessage, "The Actuator is Not Configured");
                         sendToPC(ShowActuatorMode,cmdSize,&cdmMessage); 
                     }
                     break;
                  }
                  case SetTimedOutput:
                  {
                     if(AllowedFuntions.rqstActuator == true)
                     {
                        if(Actuator.OutputMode == ActuadorTemporizado)
                        {
                           if(msgFromPC.Longitud == 8)
                           {
                              unsigned int32 toReset = make32(msgFromPC.Dato[0],msgFromPC.Dato[1],msgFromPC.Dato[2],msgFromPC.Dato[3]);
                              unsigned int32 inReset = make32(msgFromPC.Dato[4],msgFromPC.Dato[5],msgFromPC.Dato[6],msgFromPC.Dato[7]);
                              if(Actuator.TimeToReset != toReset || Actuator.TimeInReset != inReset)
                              {
                                 Actuator.TimeToReset = toReset;
                                 Actuator.TimeInReset = inReset;
                                 //guardar valor en eeprom
                              }
                              cmdSize = sprintf(cdmMessage, "The Timed Actuator is Going to be Applied in %LU S During %LU S by PC", Actuator.TimeToReset/1000,Actuator.TimeInReset/1000);
                              sendToPC(ShowTimedOutputReason,cmdSize,&cdmMessage); 
                              Actuator.ActivateTimedOutput = true;
                              if(GoIOXstatus.synchronized == true)
                              {
                                 cmdSize = sprintf(cdmMessage, "Act PC TO %Lu %Lu!",Actuator.TimeToReset,Actuator.TimeInReset);
                                 SentFreeFormatThirdPartyData(cmdSize, &cdmMessage); 
                              }
                           }
                           else
                           {
                              cmdSize = sprintf(cdmMessage, "The Timed Actuator is Going to be Applied in %LU S During %LU S by PC", Actuator.TimeToReset/1000,Actuator.TimeInReset/1000);
                              sendToPC(ShowTimedOutputReason,cmdSize,&cdmMessage); 
                              Actuator.ActivateTimedOutput = true;
                              if(GoIOXstatus.synchronized == true)
                              {
                                 cmdSize = sprintf(cdmMessage, "Act PC TO %Lu %Lu!",Actuator.TimeToReset,Actuator.TimeInReset);
                                 SentFreeFormatThirdPartyData(cmdSize, &cdmMessage); 
                              }
                           }
                        }
                        else
                        {
                           cmdSize = sprintf(cdmMessage, "The Actuator is Configured as Output");
                           sendToPC(ShowActuatorMode,cmdSize,&cdmMessage); 
                        }
                     }
                     else
                     {
                        cmdSize = sprintf(cdmMessage, "Local Timed Actuator Control Not Supported");
                        sendToPC(CommandNotAllowed,cmdSize,&cdmMessage);
                     }
                     break;
                  }
                  case SetOuputOnOff:
                  {
                     if(AllowedFuntions.rqstActuator == true)
                     {
                        if(Actuator.OutputMode == ActuadorONOFF)
                        {
                           if(msgFromPC.Longitud == 1 && msgFromPC.Dato[0] < 2)
                           {
                              cmdSize = sprintf(cdmMessage, "Actuator Desired Status = %U by PC", msgFromPC.Dato[0]);
                              sendToPC(ShowOutputReason,cmdSize,&cdmMessage); 
                              Actuator_Set_Value(msgFromPC.Dato[0]);
                              if(GoIOXstatus.synchronized == true)
                              {
                                 cmdSize = sprintf(cdmMessage, "Act PC O %u!",msgFromPC.Dato[0]);
                                 SentFreeFormatThirdPartyData(cmdSize, &cdmMessage); 
                              }
                           }
                        }
                        else
                        {
                           cmdSize = sprintf(cdmMessage, "The Actuator is Configured as Timed Output");
                           sendToPC(ShowActuatorMode,cmdSize,&cdmMessage); 
                        }
                     }
                     else
                     {
                        cmdSize = sprintf(cdmMessage, "Local Actuator Control Not Supported");
                        sendToPC(CommandNotAllowed,cmdSize,&cdmMessage);
                     }
                     break;
                  }
                  case CancelOutput:
                  {
                     if(Actuator.ActivateTimedOutput == true || Actuator.ActivateOutput == true)
                     {
                        cmdSize = sprintf(cdmMessage, "Actuator Request Cancelled");
                        sendToPC(CancelResult,cmdSize,&cdmMessage);
                        Actuator.ActivateTimedOutput = false;
                        Actuator.ActivateOutput = false;
                        if(GoIOXstatus.synchronized == true)
                        {
                           cmdSize = sprintf(cdmMessage, "Act PC Cancelled!");
                           SentFreeFormatThirdPartyData(cmdSize, &cdmMessage); 
                        }
                     }
                     else
                     {
                        cmdSize = sprintf(cdmMessage, "Actuator Request Not Cancelled Due TimeOut");
                        sendToPC(CancelResult,cmdSize,&cdmMessage);
                     }
                     
                     break;
                  }
                  case GetOutputStatus:
                  {
                     cmdSize = sprintf(cdmMessage, "Actuator Value: %u",Actuator.OutputState);
                     sendToPC(ShowOutputStatus,cmdSize,&cdmMessage); 
                     break;
                  }
                  case ResetDBI:
                  {                     
                     cmdSize = sprintf(cdmMessage, "DBI is going to be restarted in 2 seconds By PC");
                     sendToPC(ResetDBIByClient,cmdSize,&cdmMessage); 
                     for(unsigned int16 i = 0; i < 20; i++)
                     {
                        delay_ms(100);
                        restart_wdt();
                     }
                     reset_cpu();
                     //DBIReset.Reset = true;
                     break;
                  }
                  case GetIOXStatus:
                  {                     
                     if(GoIOXstatus.synchronized == true)
                     {
                        cmdSize = sprintf(cdmMessage, "The IOX is Synchronized");
                        sendToPC(ShowIOXstatus,cmdSize,&cdmMessage);
                     }
                     else
                     {
                        cmdSize = sprintf(cdmMessage, "The IOX is Not Synchronized");
                        sendToPC(ShowIOXstatus,cmdSize,&cdmMessage);
                     }
                     break;
                  } 
                  case RqstFW:
                  {
                     cmdSize = sprintf(cdmMessage, "Fimrware Version: %u.%u",firmwareMajorVersion,firmwareMinorVersion);
                     sendToPC(AsnwFW,cmdSize,&cdmMessage); 
                     break;
                  }
                  case RqstHW:
                  {
                     cmdSize = sprintf(cdmMessage, "Hardware Version: %u.%u",HardwareMajorVersion,HardwareMinorVersion);
                     sendToPC(AsnwHW,cmdSize,&cdmMessage); 
                     break;
                  }
                  case GetIgnitionstatus:
                  {
                     if(GoDeviceInfo.Ignition == true)
                     {
                        cmdSize = sprintf(cdmMessage, "Ignition ON");
                     }
                     else
                     {
                        cmdSize = sprintf(cdmMessage, "Ignition OFF");
                     }
                     sendToPC(ShowIgnitionStatus,cmdSize,&cdmMessage); 
                     break;
                  }
                  case InputGetStatus:
                  {
                     if(AllowedFuntions.showInputstatus == true)
                     {
                       cmdSize = sprintf(cdmMessage, "Input Value: %u",!InputStatus);
                       sendToPC(InputShowStatus,cmdSize,&cdmMessage);
                     }
                     else
                     {
                        cmdSize = sprintf(cdmMessage, "Input State Request Not Supported");
                        sendToPC(CommandNotAllowed,cmdSize,&cdmMessage);
                     }
                     break;
                  }
                  case DBIGetSerie:
                  {
                     unsigned int8 serie[8];
                     read_program_memory(0x200000, serie, 8);
                     cmdSize = sprintf(cdmMessage, "Serial ID: %LX%X%X%X%X%X%X%X%X",deviceType,serie[7],serie[6],serie[5],serie[4],serie[3],serie[2],serie[1],serie[0]);
                     sendToPC(InputShowStatus,cmdSize,&cdmMessage);
                     break;
                  }
                  
                  default:
                  {
                     cmdSize = sprintf(cdmMessage, "Unidentified Command");
                     sendToPC(ComandoUnidentified,cmdSize,&cdmMessage); 
                     break;
                  }
               }
               break;
            }
            case chkFail:
            {
               cmdSize = sprintf(cdmMessage, "Checksum Fail");
               sendToPC(MsgFailACK,cmdSize,&cdmMessage); 
               break;
            }
            case formatFail:
            {
               cmdSize = sprintf(cdmMessage, "Invalid Frame Format");
               sendToPC(MsgFailFormat,cmdSize,&cdmMessage); 
               break;
            }
            default:
            {
               break;
            }
         }
         pc_result = free;
      }
      
      if(GoIOXstatus.timeToSync > 1000) // se cumplio un segundo
      {
         if(GoIOXstatus.synchronized == false)
         {
            SentHandShakeRequest();
            output_toggle(Led);
         }
         if(GoIOXstatus.lastSynchronized != GoIOXstatus.synchronized)
         {
            if(GoIOXstatus.synchronized == true)
            {
               cmdSize = sprintf(cdmMessage, "The IOX is Synchronized");
               sendToPC(ShowIOXstatus,cmdSize,&cdmMessage);
               
               /*cmdSize = sprintf(cdmMessage, "DBI Messages Detected");
               SentFreeFormatThirdPartyData(cmdSize, &cdmMessage); */
               output_high(Led);
            }
            else
            {
               cmdSize = sprintf(cdmMessage, "The IOX is Not Synchronized");
               sendToPC(ShowIOXstatus,cmdSize,&cdmMessage);
            }   
            GoIOXstatus.lastSynchronized = GoIOXstatus.synchronized;
         }
         GoIOXstatus.timeToSync = 0;
      }
      
      if(GoIOXstatus.msgReady)
      {
         switch (Msg_Type)
         {            
            case HandshakeRequest:
            {
               SentHandshakeConfirmation(4151, 0, 1);  //
               GoIOXstatus.synchronized = True;
               ////fprintf(Serial,"#HandShake Answered\n\r");
               break;
            }
            case ThirdPartyDataACK:
            {
               if(sentByPC == true)
               {
                  if(GoIOXstatus.WattingACK == 1)
                  {
                     cmdSize = sprintf(cdmMessage, "Custom Data Sent");
                     sendToPC(CustomDataSent,cmdSize,&cdmMessage); 
                  }
                  else if(GoIOXstatus.WattingACK == 2)
                  {
                     cmdSize = sprintf(cdmMessage, "Status Data Sent");
                     sendToPC(StatusDataSent,cmdSize,&cdmMessage); 
                  }
                  sentByPC = false;
                  GoIOXstatus.n_timeOutCustom = 0;
                  GoIOXstatus.WattingACK = 0; 
               }
               else
               {
                  GoIOXstatus.n_timeOutCustom = 0;
                  GoIOXstatus.WattingACK = 0; 
               }
               break;
            }
            case GoDeviceData:
            {
               SentDeviceDataACK(); 
               GetParameters(&GoDeviceInfo);
               GoInfo_Leng  = Msg_Leng;
               for(unsigned int8 i= 0; i< Msg_Leng; i++)
               {
                  GoInfo_Data[i] = Msg_Data[i];
               }
               //GetParameters(&GoDeviceInfo);   
               GoInfoDataReady = true;
               if(AllowedFuntions.showGoInfo == true)
               {
                  GoInfoSize = 0;
                  for(unsigned int8 i= 0; i< Msg_Leng; i++)
                  {
                     char cdmMessage[2];
                     sprintf(cdmMessage, "%X",Msg_Data[i]);
                     GoInfo[GoInfoSize++] = cdmMessage[0];
                     GoInfo[GoInfoSize++] = cdmMessage[1];
                  }
                  sendToPC(ShowVehicleInfo,GoInfoSize,&GoInfo);
               }
               GoIOXstatus.WattingGoInfoResponse = false;
               GoIOXstatus.n_timeOutGoInfoResponse = 0;
               break;
            }
            case BinaryDataResponse:
            {
               GoIOXstatus.WattingACKmime = false;
               GoIOXstatus.n_timeOutMime = 0;
               
               if(Msg_Data[0] == 1)
               {
                  cmdSize = sprintf(cdmMessage, "MIME Message Sent");
                  sendToPC(TextMsgSent,cmdSize,&cdmMessage); 
               }
               else
               {
                  //el mensaje fallo
                  cmdSize = sprintf(cdmMessage, "MIME Message Failed");
                  sendToPC(TextMsgFailed,cmdSize,&cdmMessage); 
               }
               break;
            }
            case BinaryDataPacketIN:
            {
               if(Msg_Data[0] == 0 && Msg_Data[1] == 0x03 && Msg_Data[2] == 0x41 && Msg_Data[3] == 0x43 && Msg_Data[4] == 0x4B && Msg_Data[5] == 0x04) //hacer mejor esta validacion
               {
                  ////fprintf(Serial,"#Mensaje %Lu entregado\n\r",x);
                  cmdSize = sprintf(cdmMessage, "Message Delivered");
                  sendToPC(TextMsgDelivered,cmdSize,&cdmMessage); 
                  GoIOXstatus.WattingACKmimeFromServer = false;
                  GoIOXstatus.n_timeOutACKFromServer = 0;
               }
               else
               {
                  if(isFormatedMessage(&Msg_Data))
                  {
                     switch(Msg_Data[0])
                     {
                        case MimeMsgFromServer:
                        {
                           if(AllowedFuntions.MIMEin == true)
                           {
                              sendToPC(TextMsgFromServer,Msg_Data[1],&Msg_Data[2]);
                              if(AllowedFuntions.MimeACKResponse == true)
                              {
                                 unsigned int8 buf[1];
                                 buf[0] = MimeMsgFromServer;
                                 SentFormatMIMEPacket(ACK, 1, &buf[0]);
                              }
                           }
                           else
                           {
                              cmdSize = sprintf(cdmMessage, "MIME Messages From Server Not Supported");
                              sendToPC(TextMsgFromServerNotSupported,cmdSize,&cdmMessage); 
                           }
                           break;
                        }
                        case MimeSetTimeOutBinary:
                        {
                           if(Msg_Data[1] == 2)
                           {
                              unsigned int16 valor = make16(Msg_Data[2],Msg_Data[3]);
                              GoIOXstatus.p_timeOutMime = valor;
                              ////fprintf(Serial,"TOut Binary= %lu %c%c",GoIOXstatus.p_timeOutMime,0x1f,0x03);
                              unsigned int8 buf[2];
                              buf[0] = GoIOXstatus.p_timeOutMime >> 0 & 0xFF;
                              buf[1] = GoIOXstatus.p_timeOutMime >> 8 & 0xFF;
                              SentFormatMIMEPacket(MimeGetTimeOutBinary, 2, &buf[0]);UpDateVariables();
                           }
                           break;
                        }
                        case MimeGetTimeOutBinary:
                        {
                           unsigned int8 buf[2];
                           buf[0] = GoIOXstatus.p_timeOutMime >> 0 & 0xFF;
                           buf[1] = GoIOXstatus.p_timeOutMime >> 8 & 0xFF;
                           SentFormatMIMEPacket(MimeGetTimeOutBinary, 2, &buf[0]);
                           break;
                        }
                        case MimeSetTimeOutACKServer:
                        {
                           if(Msg_Data[1] == 4)
                           {
                              unsigned int32 valor = make32(Msg_Data[2],Msg_Data[3],Msg_Data[4],Msg_Data[5]);
                              GoIOXstatus.p_timeOutACKFromServer = valor;
                              ////fprintf(Serial,"TOut Server ACK= %lu %c%c",GoIOXstatus.p_timeOutACKFromServer,0x1f,0x03);
                              unsigned int8 buf[4];
                              buf[0] = GoIOXstatus.p_timeOutACKFromServer >> 0 & 0xFF;
                              buf[1] = GoIOXstatus.p_timeOutACKFromServer >> 8 & 0xFF;
                              buf[2] = GoIOXstatus.p_timeOutACKFromServer >> 16 & 0xFF;
                              buf[3] = GoIOXstatus.p_timeOutACKFromServer >> 24 & 0xFF;
                              SentFormatMIMEPacket(MimeGetTimeOutACKServer, 4, &buf[0]);
                           }
                           break;
                        }
                        case MimeGetTimeOutACKServer:
                        {
                           unsigned int8 buf[4];
                           buf[0] = GoIOXstatus.p_timeOutACKFromServer >> 0 & 0xFF;
                           buf[1] = GoIOXstatus.p_timeOutACKFromServer >> 8 & 0xFF;
                           buf[2] = GoIOXstatus.p_timeOutACKFromServer >> 16 & 0xFF;
                           buf[3] = GoIOXstatus.p_timeOutACKFromServer >> 24 & 0xFF;
                           SentFormatMIMEPacket(MimeGetTimeOutACKServer, 4, &buf[0]);
                           break;
                        }
                        case MimeSetMaxMimes:
                        {
                           if(Msg_Data[1] == 2)
                           {
                              unsigned int16 valor = make16(Msg_Data[2],Msg_Data[3]);
                              GoIOXstatus.maxMIMEs = valor;
                              ////fprintf(Serial,"Max Mimes Allowed= %lu %c%c",GoIOXstatus.maxMIMEs,0x1f,0x03);
                              unsigned int8 buf[2];
                              buf[0] = GoIOXstatus.maxMIMEs >> 0 & 0xFF;
                              buf[1] = GoIOXstatus.maxMIMEs >> 8 & 0xFF;
                              SentFormatMIMEPacket(MimeGetMaxMimes, 2, &buf[0]);
                           }
                           break;
                        }
                        case MimeGetMaxMimes:
                        {
                           unsigned int8 buf[2];
                           buf[0] = GoIOXstatus.maxMIMEs >> 0 & 0xFF;
                           buf[1] = GoIOXstatus.maxMIMEs >> 8 & 0xFF;
                           SentFormatMIMEPacket(MimeGetMaxMimes, 2, &buf[0]);
                           break;
                        }
                        case MimeSetEnableOut:
                        {
                           if(Msg_Data[1] == 4 && Msg_Data[2] == 0x25  && Msg_Data[3] == 0x33  && Msg_Data[4] == 0x41  && Msg_Data[5] == 0x49)
                           {
                              AllowedFuntions.MIMEout    = true;
                              ////fprintf(Serial,"Mime Out Allowed%c%c",0x1f,0x03);
                              unsigned int8 buf[1];
                              buf[0] = 1;
                              SentFormatMIMEPacket(MimeGetEnableOut, 1, &buf[0]);
                           }
                           else if(Msg_Data[1] == 4 && Msg_Data[2] == 0x25  && Msg_Data[3] == 0x28  && Msg_Data[4] == 0x31  && Msg_Data[5] == 0x34)
                           {
                              AllowedFuntions.MIMEout    = false;
                              ////fprintf(Serial,"Mime Out Not Allowed%c%c",0x1f,0x03);
                              unsigned int8 buf[1];
                              buf[0] = 0;
                              SentFormatMIMEPacket(MimeGetEnableOut, 1, &buf[0]);
                           }
                           break;
                        }
                        case MimeGetEnableOut:
                        {
                           unsigned int8 buf[1];
                           buf[0] = AllowedFuntions.MIMEout;
                           SentFormatMIMEPacket(MimeGetEnableOut, 1, &buf[0]);
                           break;
                        }
                        case MimeSetEnableIn:
                        {
                           if(Msg_Data[1] == 4 && Msg_Data[2] == 0x25  && Msg_Data[3] == 0x33  && Msg_Data[4] == 0x41  && Msg_Data[5] == 0x49)
                           {
                              AllowedFuntions.MIMEin    = true;
                              ////fprintf(Serial,"Mime In Allowed%c%c",0x1f,0x03);
                              unsigned int8 buf[1];
                              buf[0] = 1;
                              SentFormatMIMEPacket(MimeGetEnableIn, 1, &buf[0]);
                           }
                           else if(Msg_Data[1] == 4 && Msg_Data[2] == 0x25  && Msg_Data[3] == 0x28  && Msg_Data[4] == 0x31  && Msg_Data[5] == 0x34)
                           {
                              AllowedFuntions.MIMEin    = false;
                              ////fprintf(Serial,"Mime In Not Allowed%c%c",0x1f,0x03);
                              unsigned int8 buf[1];
                              buf[0] = 0;
                              SentFormatMIMEPacket(MimeGetEnableIn, 1, &buf[0]);
                           }
                           break;
                        }
                        case MimeGetEnableIn:
                        {
                           unsigned int8 buf[1];
                           buf[0] = AllowedFuntions.MIMEin;
                           SentFormatMIMEPacket(MimeGetEnableIn, 1, &buf[0]);
                           break;
                        }
                        case MimeSetMsgACKFromServer:
                        {
                           if(Msg_Data[1] == 4 && Msg_Data[2] == 0x25  && Msg_Data[3] == 0x33  && Msg_Data[4] == 0x41  && Msg_Data[5] == 0x49)
                           {
                              AllowedFuntions.MimeACKResponse   = true;
                              ////fprintf(Serial,"ACK Response Required%c%c",0x1f,0x03);
                              unsigned int8 buf[1];
                              buf[0] = 1;
                              SentFormatMIMEPacket(MimeGetMsgACKFromServer, 1, &buf[0]);
                           }
                           else if(Msg_Data[1] == 4 && Msg_Data[2] == 0x25  && Msg_Data[3] == 0x28  && Msg_Data[4] == 0x31  && Msg_Data[5] == 0x34)
                           {
                              AllowedFuntions.MimeACKResponse    = false;
                              ////fprintf(Serial,"ACK Response Not Required%c%c",0x1f,0x03);
                              unsigned int8 buf[1];
                              buf[0] = 0;
                              SentFormatMIMEPacket(MimeGetMsgACKFromServer, 1, &buf[0]);
                           }
                           break;
                        }
                        case MimeGetMsgACKFromServer:
                        {
                           unsigned int8 buf[1];
                           buf[0] = AllowedFuntions.MimeACKResponse;
                           SentFormatMIMEPacket(MimeGetMsgACKFromServer, 1, &buf[0]);
                           break;
                        }
                        case MimeSetWaitACKFromServer:
                        {
                           if(Msg_Data[1] == 4 && Msg_Data[2] == 0x25  && Msg_Data[3] == 0x33  && Msg_Data[4] == 0x41  && Msg_Data[5] == 0x49)
                           {
                              AllowedFuntions.blockMIMEByACK   = true;
                              ////fprintf(Serial,"ACK Response Required%c%c",0x1f,0x03);
                              unsigned int8 buf[1];
                              buf[0] = 1;
                              SentFormatMIMEPacket(MimeGetWaitACKFromServer, 1, &buf[0]);
                           }
                           else if(Msg_Data[1] == 4 && Msg_Data[2] == 0x25  && Msg_Data[3] == 0x28  && Msg_Data[4] == 0x31  && Msg_Data[5] == 0x34)
                           {
                              AllowedFuntions.blockMIMEByACK    = false;
                              ////fprintf(Serial,"ACK Response Not Required%c%c",0x1f,0x03);
                              unsigned int8 buf[1];
                              buf[0] = 0;
                              SentFormatMIMEPacket(MimeGetWaitACKFromServer, 1, &buf[0]);
                           }
                           break;
                        }
                        case MimeGetWaitACKFromServer:
                        {
                           unsigned int8 buf[1];
                           buf[0] = AllowedFuntions.blockMIMEByACK;
                           SentFormatMIMEPacket(MimeGetWaitACKFromServer, 1, &buf[0]);
                           break;
                        }
                        case CSSetTimeOut:
                        {
                           if(Msg_Data[1] == 2)
                           {
                              unsigned int16 valor = make16(Msg_Data[2],Msg_Data[3]);
                              GoIOXstatus.p_timeOutCustom = valor;
                              ////fprintf(Serial,"TOut Custom ACK= %lu %c%c",GoIOXstatus.p_timeOutCustom,0x1f,0x03);
                              unsigned int8 buf[2];
                              buf[0] = GoIOXstatus.p_timeOutCustom >> 0 & 0xFF;
                              buf[1] = GoIOXstatus.p_timeOutCustom >> 8 & 0xFF;
                              SentFormatMIMEPacket(CSGetTimeOut, 2, &buf[0]);
                           }
                           break;
                        }
                        case CSGetTimeOut:
                        {
                           unsigned int8 buf[4];
                           buf[0] = GoIOXstatus.p_timeOutCustom >> 0 & 0xFF;
                           buf[1] = GoIOXstatus.p_timeOutCustom >> 8 & 0xFF;
                           SentFormatMIMEPacket(CSGetTimeOut, 4, &buf[0]);
                           break;
                        }
                        case CSSetMaxCustoms:
                        {
                           if(Msg_Data[1] == 2)
                           {
                              unsigned int16 valor = make16(Msg_Data[2],Msg_Data[3]);
                              GoIOXstatus.maxCustoms = valor;
                              ////fprintf(Serial,"Max Customs Allowed= %lu %c%c",GoIOXstatus.maxCustoms,0x1f,0x03);
                              unsigned int8 buf[2];
                              buf[0] = GoIOXstatus.maxCustoms >> 0 & 0xFF;
                              buf[1] = GoIOXstatus.maxCustoms >> 8 & 0xFF;
                              SentFormatMIMEPacket(CSGatMaxCustoms, 2, &buf[0]);
                           }
                           break;
                        }
                        case CSGatMaxCustoms:
                        {
                           unsigned int8 buf[2];
                           buf[0] = GoIOXstatus.maxCustoms >> 0 & 0xFF;
                           buf[1] = GoIOXstatus.maxCustoms >> 8 & 0xFF;
                           SentFormatMIMEPacket(CSGatMaxCustoms, 2, &buf[0]);
                           break;
                        }
                        case CSSetAllowedCustoms:
                        {
                           if(Msg_Data[1] == 4 && Msg_Data[2] == 0x25  && Msg_Data[3] == 0x33  && Msg_Data[4] == 0x41  && Msg_Data[5] == 0x49)
                           {
                              AllowedFuntions.customData = true;
                              //fprintf(Serial,"Customs Data Allowed%c%c",0x1f,0x03);
                              unsigned int8 buf[1];
                              buf[0] = 1;
                              SentFormatMIMEPacket(CSGetAllowedCustoms, 1, &buf[0]);
                           }
                           else if(Msg_Data[1] == 4 && Msg_Data[2] == 0x25  && Msg_Data[3] == 0x28  && Msg_Data[4] == 0x31  && Msg_Data[5] == 0x34)
                           {
                              AllowedFuntions.customData = false;
                              //fprintf(Serial,"Custom Data Not Allowed%c%c",0x1f,0x03);
                              unsigned int8 buf[1];
                              buf[0] = 0;
                              SentFormatMIMEPacket(CSGetAllowedCustoms, 1, &buf[0]);
                           }
                           break;
                        }
                        case CSGetAllowedCustoms:
                        {
                           unsigned int8 buf[1];
                           buf[0] = AllowedFuntions.customData;
                           SentFormatMIMEPacket(CSGetAllowedCustoms, 1, &buf[0]);
                           break;
                        }
                        case CSSetAllowedStatusData:
                        {
                           if(Msg_Data[1] == 4 && Msg_Data[2] == 0x25  && Msg_Data[3] == 0x33  && Msg_Data[4] == 0x41  && Msg_Data[5] == 0x49)
                           {
                              AllowedFuntions.StatusData = true;
                              //fprintf(Serial,"Status Data Allowed%c%c",0x1f,0x03);
                              unsigned int8 buf[1];
                              buf[0] = 1;
                              SentFormatMIMEPacket(CSGetAllowedStatusData, 1, &buf[0]);
                           }
                           else if(Msg_Data[1] == 4 && Msg_Data[2] == 0x25  && Msg_Data[3] == 0x28  && Msg_Data[4] == 0x31  && Msg_Data[5] == 0x34)
                           {
                              AllowedFuntions.StatusData = false;
                              //fprintf(Serial,"Status Data Not Allowed%c%c",0x1f,0x03);
                              unsigned int8 buf[1];
                              buf[0] = 0;
                              SentFormatMIMEPacket(CSGetAllowedStatusData, 1, &buf[0]);
                           }
                           break;
                        }
                        case CSGetAllowedStatusData:
                        {
                           unsigned int8 buf[1];
                           buf[0] = AllowedFuntions.StatusData;
                           SentFormatMIMEPacket(CSGetAllowedStatusData, 1, &buf[0]);
                           break;
                        }
                        case ActSetMode:
                        {
                           if(Msg_Data[1] == 4 && Msg_Data[2] == 0x25  && Msg_Data[3] == 0x33  && Msg_Data[4] == 0x41  && Msg_Data[5] == 0x49)
                           {
                              Actuator.OutputMode = ActuadorONOFF;
                              //fprintf(Serial,"Actuator as Output%c%c",0x1f,0x03);
                              unsigned int8 buf[1];
                              buf[0] = Actuator.OutputMode;
                              SentFormatMIMEPacket(ActGetMode, 1, &buf[0]);
                           }
                           else if(Msg_Data[1] == 4 && Msg_Data[2] == 0x25  && Msg_Data[3] == 0x28  && Msg_Data[4] == 0x31  && Msg_Data[5] == 0x34)
                           {
                              Actuator.OutputMode = ActuadorTemporizado;
                              //fprintf(Serial,"Actuator as Timed Output%c%c",0x1f,0x03);
                              unsigned int8 buf[1];
                              buf[0] = Actuator.OutputMode;
                              SentFormatMIMEPacket(ActGetMode, 1, &buf[0]);
                           }
                           break;
                        }
                        case ActGetMode:
                        {
                           unsigned int8 buf[1];
                           buf[0] = Actuator.OutputMode;
                           SentFormatMIMEPacket(ActGetMode, 1, &buf[0]);
                           break;
                        }
                        case ActSetConfigTimedOutput:
                        {
                           if(Msg_Data[1] == 8)
                           {
                              unsigned int32 toReset = make32(Msg_Data[2],Msg_Data[3],Msg_Data[4],Msg_Data[5]);
                              unsigned int32 inReset = make32(Msg_Data[6],Msg_Data[7],Msg_Data[8],Msg_Data[9]);
                              Actuator.TimeInReset = inReset;
                              Actuator.TimeToReset = toReset;
                              //fprintf(Serial,"Act. toReset= %lu, inReset%lu %c%c",Actuator.TimeToReset,Actuator.TimeInReset,0x1f,0x03);
                              unsigned int8 buf[8];
                              buf[0] = Actuator.TimeToReset >> 0 & 0xFF;
                              buf[1] = Actuator.TimeToReset >> 8 & 0xFF;
                              buf[2] = Actuator.TimeToReset >> 16 & 0xFF;
                              buf[3] = Actuator.TimeToReset >> 24 & 0xFF;
                              buf[4] = Actuator.TimeInReset >> 0 & 0xFF;
                              buf[5] = Actuator.TimeInReset >> 8 & 0xFF;
                              buf[6] = Actuator.TimeInReset >> 16 & 0xFF;
                              buf[7] = Actuator.TimeInReset >> 24 & 0xFF;
                              SentFormatMIMEPacket(ActGetConfigTimedOutput, 8, &buf[0]);
                           }
                           break;
                        }
                        case ActGetConfigTimedOutput:
                        {
                           unsigned int8 buf[8];
                           buf[0] = Actuator.TimeToReset >> 0 & 0xFF;
                           buf[1] = Actuator.TimeToReset >> 8 & 0xFF;
                           buf[2] = Actuator.TimeToReset >> 16 & 0xFF;
                           buf[3] = Actuator.TimeToReset >> 24 & 0xFF;
                           buf[4] = Actuator.TimeInReset >> 0 & 0xFF;
                           buf[5] = Actuator.TimeInReset >> 8 & 0xFF;
                           buf[6] = Actuator.TimeInReset >> 16 & 0xFF;
                           buf[7] = Actuator.TimeInReset >> 24 & 0xFF;
                           SentFormatMIMEPacket(ActGetConfigTimedOutput, 8, &buf[0]);
                           break;
                        }
                        case ActSetTimeOutPass:
                        {
                           if(Msg_Data[1] == 4)
                           {
                              unsigned int32 valor = make32(Msg_Data[2],Msg_Data[3],Msg_Data[4],Msg_Data[5]);
                              Actuator.P_TimeOutPass = valor;
                              //fprintf(Serial,"TOut Act PAss= %lu %c%c",Actuator.P_TimeOutPass,0x1f,0x03);
                              unsigned int8 buf[4];
                              buf[0] = Actuator.P_TimeOutPass >> 0 & 0xFF;
                              buf[1] = Actuator.P_TimeOutPass >> 8 & 0xFF;
                              buf[2] = Actuator.P_TimeOutPass >> 16 & 0xFF;
                              buf[3] = Actuator.P_TimeOutPass >> 24 & 0xFF;
                              SentFormatMIMEPacket(ActGetTimeOutPass, 4, &buf[0]);
                           }
                           break;
                        }
                        case ActGetTimeOutPass:
                        {
                           unsigned int8 buf[4];
                              buf[0] = Actuator.P_TimeOutPass >> 0 & 0xFF;
                              buf[1] = Actuator.P_TimeOutPass >> 8 & 0xFF;
                              buf[2] = Actuator.P_TimeOutPass >> 16 & 0xFF;
                              buf[3] = Actuator.P_TimeOutPass >> 24 & 0xFF;
                              SentFormatMIMEPacket(ActGetTimeOutPass, 4, &buf[0]);
                           break;
                        }
                        case ActSetOutput:
                        {
                           if(Actuator.OutputMode == ActuadorONOFF)
                           {
                              if(Msg_Data[1] == 4 && Msg_Data[2] == 0x25  && Msg_Data[3] == 0x33  && Msg_Data[4] == 0x41  && Msg_Data[5] == 0x49)
                              {
                                 //fprintf(Serial,"Actuator ON requested%c%c",0x1f,0x03);
                                 Actuator.LastPetition = ON;
                              }
                              else if(Msg_Data[1] == 4 && Msg_Data[2] == 0x25  && Msg_Data[3] == 0x28  && Msg_Data[4] == 0x31  && Msg_Data[5] == 0x34)
                              {
                                 //fprintf(Serial,"Actuator OFF requested%c%c",0x1f,0x03);
                                 Actuator.LastPetition = OFF;
                              }
                              srand(get_timer2());
                              unsigned int16 pass = 0;
                              while(pass < 112 || pass > 9999)
                              {
                                 pass = rand();
                              }
                              Actuator.Pass = pass;
                              Actuator.WaittingPass = true;
                              unsigned int8 buf[2];
                              buf[0] = Actuator.Pass >> 0 & 0xFF;
                              buf[1] = Actuator.Pass >> 8 & 0xFF;
                              SentFormatMIMEPacket(ActSetPass, 2, &buf[0]);
                              //fprintf(Serial,"Actuator Pin Sent= %lu%c%c",pass,0x1f,0x03);
                              //envio Pass
                           }
                           else
                           {
                              unsigned int8 buf[1];
                              buf[0] = Actuator.OutputMode;
                              SentFormatMIMEPacket(ActGetMode, 1, &buf[0]);
                           }
                           break;
                        }
                        case ActGetActualValue:
                        {
                           unsigned int8 buf[1];
                           buf[0] = Actuator.OutputState;
                           SentFormatMIMEPacket(ActGetActualValue, 1, &buf[0]);
                           break;
                        }
                        case ActSetPass:
                        {
                           if(Actuator.WaittingPass == true)
                           {
                              if(Msg_Data[1] == 2)
                              {
                                 unsigned int16 pin = make16(Msg_Data[2],Msg_Data[3]);
                                 if(pin == Actuator.Pass)
                                 {
                                    if(Actuator.LastPetition == ON || Actuator.LastPetition == OFF || Actuator.LastPetition == TimedOutput)
                                    {
                                       if(Actuator.OutputMode == ActuadorONOFF)
                                       {
                                          if(Actuator.LastPetition == ON)
                                          {
                                             cmdSize = sprintf(cdmMessage, "Actuator Desired Status = %U by Server", 1);
                                             sendToPC(ShowOutputReason,cmdSize,&cdmMessage); 
                                             Actuator_Set_Value(1);
                                             if(GoIOXstatus.synchronized == true)
                                             {
                                                cmdSize = sprintf(cdmMessage, "Act Server O 1!");
                                                SentFreeFormatThirdPartyData(cmdSize, &cdmMessage); 
                                             }
                                          }
                                          else if(Actuator.LastPetition == OFF)
                                          {
                                             cmdSize = sprintf(cdmMessage, "Actuator Desired Status = %U by Server", 0);
                                             sendToPC(ShowOutputReason,cmdSize,&cdmMessage); 
                                             Actuator_Set_Value(0);
                                             if(GoIOXstatus.synchronized == true)
                                             {
                                                cmdSize = sprintf(cdmMessage, "Act Server O 0!");
                                                SentFreeFormatThirdPartyData(cmdSize, &cdmMessage); 
                                             }
                                          }
                                          else if(Actuator.LastPetition == TimedOutput)
                                          {
                                             //no se puede llevar esta accion en este modo
                                             cmdSize = sprintf(cdmMessage, "Timed Output Actuator no Realizado por Modo de Actuador");
                                             SentFormatMIMEPacket(ActError, cmdSize, &cdmMessage);
                                          }
                                       }
                                       else if(Actuator.OutputMode == ActuadorTemporizado)
                                       {
                                          if(Actuator.LastPetition == TimedOutput)
                                          {
                                             cmdSize = sprintf(cdmMessage, "The Timed Actuator is Going to be Applied in %LU S During %LU S by Server", Actuator.TimeToReset/1000,Actuator.TimeInReset/1000);
                                             sendToPC(ShowTimedOutputReason,cmdSize,&cdmMessage); 
                                             Actuator.ActivateTimedOutput = true;
                                             if(GoIOXstatus.synchronized == true)
                                             {
                                                cmdSize = sprintf(cdmMessage, "Act Server TO %Lu %Lu!",Actuator.TimeToReset,Actuator.TimeInReset);
                                                SentFreeFormatThirdPartyData(cmdSize, &cdmMessage); 
                                             }
                                          }
                                          else if(Actuator.LastPetition == ON ||Actuator.LastPetition == OFF)
                                          {
                                             //no se puede llevar esta accion en este modo
                                             cmdSize = sprintf(cdmMessage, "Output Actuator no Realizada por Modo de Actuador");
                                             SentFormatMIMEPacket(ActError, cmdSize, &cdmMessage);
                                          }
                                       }
                                       Actuator.LastPetition = 0;
                                    }
                                    else
                                    {
                                       //no hay peticion guardada
                                       cmdSize = sprintf(cdmMessage, "Sin Peticion de Actuador");
                                       SentFormatMIMEPacket(ActError, cmdSize, &cdmMessage);
                                    }
                                    Actuator.WaittingPass = false;
                                    Actuator.Pass = 10000;
                                 }
                                 else
                                 {
                                    cmdSize = sprintf(cdmMessage, "PIN %lu Incorrecto",pin);
                                    SentFormatMIMEPacket(ActError, cmdSize, &cdmMessage);
                                    Actuator.LastPetition = 0;
                                 }
                              } 
                              else
                              {
                                 cmdSize = sprintf(cdmMessage, "PIN Sin Formato");
                                 SentFormatMIMEPacket(ActError, cmdSize, &cdmMessage);
                              }
                           }
                           else
                           {
                              if(Actuator.LastPetition != 0)
                              {
                                 cmdSize = sprintf(cdmMessage, "Tiempo de Espera Para PIN Superado");
                                 SentFormatMIMEPacket(ActError, cmdSize, &cdmMessage);
                                 Actuator.LastPetition = 0 ;
                              }
                              else
                              {
                                 cmdSize = sprintf(cdmMessage, "PIN No Esperado");
                                 SentFormatMIMEPacket(ActError, cmdSize, &cdmMessage);
                              }
                           }
                           break;
                        }
                        case ActSettimedOutput:
                        {
                           if(Actuator.OutputMode == ActuadorTemporizado)
                           {
                              if(Msg_Data[1] == 4 && Msg_Data[2] == 0x25  && Msg_Data[3] == 0x33  && Msg_Data[4] == 0x41  && Msg_Data[5] == 0x49)
                              {
                                 //fprintf(Serial,"Actuator timed requested%c%c",0x1f,0x03);
                                 Actuator.LastPetition = TimedOutput;
                              }
                              srand(get_timer2());
                              unsigned int16 pass = 0;
                              while(pass < 112 || pass > 9999)
                              {
                                 pass = rand();
                              }
                              Actuator.Pass = pass;
                              Actuator.WaittingPass = true;
                              unsigned int8 buf[2];
                              buf[0] = Actuator.Pass >> 0 & 0xFF;
                              buf[1] = Actuator.Pass >> 8 & 0xFF;
                              SentFormatMIMEPacket(ActSetPass, 2, &buf[0]);
                              //fprintf(Serial,"Actuator Pin Sent= %lu%c%c",pass,0x1f,0x03);
                              //envio Pass
                           }
                           else
                           {
                              unsigned int8 buf[1];
                              buf[0] = Actuator.OutputMode;
                              SentFormatMIMEPacket(ActGetMode, 1, &buf[0]);
                           }
                           break;
                        }
                        case ActSetEnableClientRqst:
                        {
                           if(Msg_Data[1] == 4 && Msg_Data[2] == 0x25  && Msg_Data[3] == 0x33  && Msg_Data[4] == 0x41  && Msg_Data[5] == 0x49)
                           {
                              AllowedFuntions.rqstActuator = true;
                              //fprintf(Serial,"Actuator Rqsr by Client Allowed%c%c",0x1f,0x03);
                              unsigned int8 buf[1];
                              buf[0] = AllowedFuntions.rqstActuator;
                              SentFormatMIMEPacket(ActGetEnableClientRqst, 1, &buf[0]);
                           }
                           else if(Msg_Data[1] == 4 && Msg_Data[2] == 0x25  && Msg_Data[3] == 0x28  && Msg_Data[4] == 0x31  && Msg_Data[5] == 0x34)
                           {
                              AllowedFuntions.rqstActuator = false;
                              //fprintf(Serial,"Actuator Rqsr by Client Not Allowed%c%c",0x1f,0x03);
                              unsigned int8 buf[1];
                              buf[0] = AllowedFuntions.rqstActuator;
                              SentFormatMIMEPacket(ActGetEnableClientRqst, 1, &buf[0]);
                           }
                           break;
                        }
                        case ActGetEnableClientRqst:
                        {
                           unsigned int8 buf[1];
                           buf[0] = AllowedFuntions.rqstActuator;
                           SentFormatMIMEPacket(ActGetEnableClientRqst, 1, &buf[0]);
                           break;
                        } 
                        case DBISetTimeHeartBeat:
                        {
                           if(Msg_Data[1] == 4)
                           {
                              unsigned int32 valor = make32(Msg_Data[2],Msg_Data[3],Msg_Data[4],Msg_Data[5]);
                              GoIOXstatus.p_TimetoHearBeat = valor;
                              //fprintf(Serial,"Time to Send HearBeat %lu %c%c",GoIOXstatus.p_TimetoHearBeat,0x1f,0x03);
                              unsigned int8 buf[4];
                              buf[0] = GoIOXstatus.p_TimetoHearBeat >> 0 & 0xFF;
                              buf[1] = GoIOXstatus.p_TimetoHearBeat >> 8 & 0xFF;
                              buf[2] = GoIOXstatus.p_TimetoHearBeat >> 16 & 0xFF;
                              buf[3] = GoIOXstatus.p_TimetoHearBeat >> 24 & 0xFF;
                              SentFormatMIMEPacket(DBIGetTimeHeartBeat, 4, &buf[0]);
                           }
                           break;
                        }
                        case DBIGetTimeHeartBeat:
                        {
                           unsigned int8 buf[4];
                           buf[0] = GoIOXstatus.p_TimetoHearBeat >> 0 & 0xFF;
                           buf[1] = GoIOXstatus.p_TimetoHearBeat >> 8 & 0xFF;
                           buf[2] = GoIOXstatus.p_TimetoHearBeat >> 16 & 0xFF;
                           buf[3] = GoIOXstatus.p_TimetoHearBeat >> 24 & 0xFF;
                           SentFormatMIMEPacket(DBIGetTimeHeartBeat, 4, &buf[0]);
                           break;
                        }
                        case DBISetEnableHeartBeat:
                        {
                           if(Msg_Data[1] == 4 && Msg_Data[2] == 0x25  && Msg_Data[3] == 0x33  && Msg_Data[4] == 0x41  && Msg_Data[5] == 0x49)
                           {
                              GoIOXstatus.HeartbeatEnable = true;
                              //fprintf(Serial,"HeartBeat Enabled%c%c",0x1f,0x03);
                              unsigned int8 buf[1];
                              buf[0] = GoIOXstatus.HeartbeatEnable;
                              SentFormatMIMEPacket(DBIGetEnableHeartBeat, 1, &buf[0]);
                           }
                           else if(Msg_Data[1] == 4 && Msg_Data[2] == 0x25  && Msg_Data[3] == 0x28  && Msg_Data[4] == 0x31  && Msg_Data[5] == 0x34)
                           {
                              GoIOXstatus.HeartbeatEnable = false;
                              //fprintf(Serial,"HeartBeat Disabled%c%c",0x1f,0x03);
                              unsigned int8 buf[1];
                              buf[0] = GoIOXstatus.HeartbeatEnable;
                              SentFormatMIMEPacket(DBIGetEnableHeartBeat, 1, &buf[0]);
                           }
                           break;
                        }
                        case DBIGetEnableHeartBeat:
                        {
                           unsigned int8 buf[1];
                           buf[0] = GoIOXstatus.HeartbeatEnable;
                           SentFormatMIMEPacket(DBIGetEnableHeartBeat, 1, &buf[0]);
                           break;
                        }
                        case DBIForceToSentHearbeat:
                        {
                           if(GoIOXstatus.synchronized == true)
                           {
                              cmdSize = sprintf(cdmMessage, "DBI HeartBeat!");
                              SentFreeFormatThirdPartyData(cmdSize, &cdmMessage); 
                           }
                           break;
                        }
                        case DBIGetFwVersion:
                        {
                           unsigned int8 buf[2];
                           buf[0] = firmwareMajorVersion;
                           buf[1] = firmwareMinorVersion;
                           SentFormatMIMEPacket(DBIGetFwVersion, 2, &buf[0]);
                           break;
                        }
                        case DBIGetHwVersion:
                        {
                           unsigned int8 buf[2];
                           buf[0] = HardwareMajorVersion;
                           buf[1] = HardwareMinorVersion;
                           SentFormatMIMEPacket(DBIGetHwVersion, 2, &buf[0]);
                           break;
                        }
                        case DBIResetHardware:
                        {
                           cmdSize = sprintf(cdmMessage, "DBI is going to be restarted in 2 seconds By Server");
                           sendToPC(ResetDBIByServer,cmdSize,&cdmMessage); 
                           for(unsigned int16 i = 0; i < 20; i++)
                           {
                              delay_ms(100);
                              restart_wdt();
                           }
                           //guardar en eeprom
                           reset_cpu();
                           break;
                        }
                        case DBIGetDeviceSerie:
                        {
                           unsigned int8 serie[8];
                           read_program_memory(0x200000, serie, 8);
                           unsigned int8 buf[10];
                           buf[0] = (deviceType >> 8) & 0x00FF;
                           buf[1] = (deviceType >> 0) & 0x00FF;
                           buf[2] = serie[7];
                           buf[3] = serie[6];
                           buf[4] = serie[5];
                           buf[5] = serie[4];
                           buf[6] = serie[3];
                           buf[7] = serie[2];
                           buf[8] = serie[1];
                           buf[9] = serie[0];
                           SentFormatMIMEPacket(DBIGetDeviceSerie, 10, &buf[0]);
                           break;
                        }
                        case GoInfoSetRequestTime:
                        {
                           if(Msg_Data[1] == 4)
                           {
                              unsigned int32 valor = make32(Msg_Data[2],Msg_Data[3],Msg_Data[4],Msg_Data[5]);
                              GoIOXstatus.p_TimetoRqstHOS = valor;
                              //fprintf(Serial,"Time to Request Go Info %lu %c%c",GoIOXstatus.p_TimetoRqstHOS,0x1f,0x03);
                              unsigned int8 buf[4];
                              buf[0] = GoIOXstatus.p_TimetoRqstHOS >> 0 & 0xFF;
                              buf[1] = GoIOXstatus.p_TimetoRqstHOS >> 8 & 0xFF;
                              buf[2] = GoIOXstatus.p_TimetoRqstHOS >> 16 & 0xFF;
                              buf[3] = GoIOXstatus.p_TimetoRqstHOS >> 24 & 0xFF;
                              SentFormatMIMEPacket(GoInfoGetRequestTime, 4, &buf[0]);
                           }
                           break;
                        }
                        case GoInfoGetRequestTime:
                        {
                           unsigned int8 buf[4];
                           buf[0] = GoIOXstatus.p_TimetoRqstHOS >> 0 & 0xFF;
                           buf[1] = GoIOXstatus.p_TimetoRqstHOS >> 8 & 0xFF;
                           buf[2] = GoIOXstatus.p_TimetoRqstHOS >> 16 & 0xFF;
                           buf[3] = GoIOXstatus.p_TimetoRqstHOS >> 24 & 0xFF;
                           SentFormatMIMEPacket(GoInfoGetRequestTime, 4, &buf[0]);
                           break;
                        }
                        case GoInfoSetEnableShow:
                        {
                           if(Msg_Data[1] == 4 && Msg_Data[2] == 0x25  && Msg_Data[3] == 0x33  && Msg_Data[4] == 0x41  && Msg_Data[5] == 0x49)
                           {
                              AllowedFuntions.showGoInfo = true;
                              //fprintf(Serial,"Auto Show Go Info Enabled%c%c",0x1f,0x03);
                              unsigned int8 buf[1];
                              buf[0] = AllowedFuntions.showGoInfo;
                              SentFormatMIMEPacket(GoInfoGetEnableShow, 1, &buf[0]);
                           }
                           else if(Msg_Data[1] == 4 && Msg_Data[2] == 0x25  && Msg_Data[3] == 0x28  && Msg_Data[4] == 0x31  && Msg_Data[5] == 0x34)
                           {
                              AllowedFuntions.showGoInfo = false;
                              //fprintf(Serial,"Auto Show Go Info Disabled%c%c",0x1f,0x03);
                              unsigned int8 buf[1];
                              buf[0] = AllowedFuntions.showGoInfo;
                              SentFormatMIMEPacket(GoInfoGetEnableShow, 1, &buf[0]);
                           }
                           break;
                        }
                        case GoInfoGetEnableShow:
                        {
                           unsigned int8 buf[1];
                           buf[0] = AllowedFuntions.showGoInfo;
                           SentFormatMIMEPacket(GoInfoGetEnableShow, 1, &buf[0]);
                           break;
                        }
                        case GoInfoSetEnableRqst:
                        {
                           if(Msg_Data[1] == 4 && Msg_Data[2] == 0x25  && Msg_Data[3] == 0x33  && Msg_Data[4] == 0x41  && Msg_Data[5] == 0x49)
                           {
                              AllowedFuntions.rqstGoInfo = true;
                              //fprintf(Serial,"Request Go Info Enabled%c%c",0x1f,0x03);
                              unsigned int8 buf[1];
                              buf[0] = AllowedFuntions.rqstGoInfo;
                              SentFormatMIMEPacket(GoInfoGetEnableRqst, 1, &buf[0]);
                           }
                           else if(Msg_Data[1] == 4 && Msg_Data[2] == 0x25  && Msg_Data[3] == 0x28  && Msg_Data[4] == 0x31  && Msg_Data[5] == 0x34)
                           {
                              AllowedFuntions.rqstGoInfo = false;
                              //fprintf(Serial,"Request Go Info Disabled%c%c",0x1f,0x03);
                              unsigned int8 buf[1];
                              buf[0] = AllowedFuntions.rqstGoInfo;
                              SentFormatMIMEPacket(GoInfoGetEnableRqst, 1, &buf[0]);
                           }
                           break;
                        }
                        case GoInfoGetEnableRqst:
                        {
                           unsigned int8 buf[1];
                           buf[0] = AllowedFuntions.rqstGoInfo;
                           SentFormatMIMEPacket(GoInfoGetEnableRqst, 1, &buf[0]);
                           break;
                        } 
                        case DBIGetAllInfo:
                        {
                           unsigned int8 index = 0;
                           unsigned int8 buf[60];
                           Variables result;
                           result = UpDateVariables();
                           
                           buf[index++] = (result.MIMEackByGo >> 8) & 0xFF;
                           buf[index++] = (result.MIMEackByGo >> 0) & 0xFF;
                           
                           buf[index++] = (result.MIMEackByServer >> 24) & 0xFF;
                           buf[index++] = (result.MIMEackByServer >> 16) & 0xFF;
                           buf[index++] = (result.MIMEackByServer >> 8) & 0xFF;
                           buf[index++] = (result.MIMEackByServer >> 0) & 0xFF;
                           
                           buf[index++] = (result.MIMEmax >> 8) & 0xFF;
                           buf[index++] = (result.MIMEmax >> 0) & 0xFF;
                           
                           buf[index++] = result.MIMEoutEnabled;
                           buf[index++] = result.MIMEinEnabled;
                           buf[index++] = result.MIMEWaitACKtoSend;
                           buf[index++] = result.MIMEautoResponseACK;
                           
                           buf[index++] = (result.CSackByGo >> 8) & 0xFF;
                           buf[index++] = (result.CSackByGo >> 0) & 0xFF;
                           
                           buf[index++] = (result.CSmax >> 8) & 0xFF;
                           buf[index++] = (result.CSmax >> 0) & 0xFF;
                           
                           buf[index++] = result.CScustomEnabled;
                           buf[index++] = result.CSstatusEnabled;
                           
                           buf[index++] = (result.HOStimeToResquest >> 24) & 0xFF;
                           buf[index++] = (result.HOStimeToResquest >> 16) & 0xFF;
                           buf[index++] = (result.HOStimeToResquest >> 8) & 0xFF;
                           buf[index++] = (result.HOStimeToResquest >> 0) & 0xFF;
                           
                           buf[index++] = result.HOSautoShowEnabled;
                           buf[index++] = result.HOSrqstEnabled;
                           
                           buf[index++] = result.ACTmode;
                           buf[index++] = result.ACTstatus;
                           
                           buf[index++] = (result.ACTtoReset >> 24) & 0xFF;
                           buf[index++] = (result.ACTtoReset >> 16) & 0xFF;
                           buf[index++] = (result.ACTtoReset >> 8) & 0xFF;
                           buf[index++] = (result.ACTtoReset >> 0) & 0xFF;
                           
                           buf[index++] = (result.ACTinReset >> 24) & 0xFF;
                           buf[index++] = (result.ACTinReset >> 16) & 0xFF;
                           buf[index++] = (result.ACTinReset >> 8) & 0xFF;
                           buf[index++] = (result.ACTinReset >> 0) & 0xFF;
                           
                           buf[index++] = (result.ACTtimeOutPIN >> 24) & 0xFF;
                           buf[index++] = (result.ACTtimeOutPIN >> 16) & 0xFF;
                           buf[index++] = (result.ACTtimeOutPIN >> 8) & 0xFF;
                           buf[index++] = (result.ACTtimeOutPIN >> 0) & 0xFF;
                           
                           buf[index++] = result.ACTenableClientRequest;
                           
                           buf[index++] = (result.DBItimeToHeartBeat >> 24) & 0xFF;
                           buf[index++] = (result.DBItimeToHeartBeat >> 16) & 0xFF;
                           buf[index++] = (result.DBItimeToHeartBeat >> 8) & 0xFF;
                           buf[index++] = (result.DBItimeToHeartBeat >> 0) & 0xFF;
                           
                           buf[index++] = result.DBIheartBeatEnabled;
                           
                           buf[index++] = result.InputEnableShow;
                           buf[index++] = result.InputEnablestatus;
                           buf[index++] = (result.InputTimeToSense >> 8) & 0xFF;
                           buf[index++] = (result.InputTimeToSense >> 0) & 0xFF;
                           buf[index++] = (result.InputStatusDataId >> 8) & 0xFF;
                           buf[index++] = (result.InputStatusDataId >> 0) & 0xFF;
                           buf[index++] = result.InputEnableSaveEeprom;
                           buf[index++] = result.InputValueToSave;
                           
                           buf[index++] = firmwareMajorVersion;
                           buf[index++] = firmwareMinorVersion;
                           buf[index++] = HardwareMajorVersion;
                           buf[index++] = HardwareMinorVersion;
                           
                           SentFormatMIMEPacket(DBIGetAllInfo, index, &buf[0]);
                           break;
                        }
                        case EepromSaveAll:
                        {
                           Variables structNow;
                           structNow = UpDateVariables();
                           if(CompareStructs(ValoresIniciales, structNow))
                           {
                              EepromSave(structNow);
                              unsigned int8 buf[1];
                              buf[0] = BloqueEeprom;
                              SentFormatMIMEPacket(EepromAllSaved, 1, &buf[0]);
                           }
                           break;
                        }
                        case InputSetEnableShow:
                        {
                           if(Msg_Data[1] == 4 && Msg_Data[2] == 0x25  && Msg_Data[3] == 0x33  && Msg_Data[4] == 0x41  && Msg_Data[5] == 0x49)
                           {
                              AllowedFuntions.showInputstatus = true;
                              unsigned int8 buf[1];
                              buf[0] = AllowedFuntions.showInputstatus;
                              SentFormatMIMEPacket(InputGetEnableShow, 1, &buf[0]);
                           }
                           else if(Msg_Data[1] == 4 && Msg_Data[2] == 0x25  && Msg_Data[3] == 0x28  && Msg_Data[4] == 0x31  && Msg_Data[5] == 0x34)
                           {
                              AllowedFuntions.showInputstatus = false;
                              //fprintf(Serial,"Auto Show Go Info Disabled%c%c",0x1f,0x03);
                              unsigned int8 buf[1];
                              buf[0] = AllowedFuntions.showInputstatus;
                              SentFormatMIMEPacket(InputGetEnableShow, 1, &buf[0]);
                           }
                           break;
                        }
                        case InputGetEnableShow:
                        {
                           unsigned int8 buf[1];
                           buf[0] = AllowedFuntions.showInputstatus;
                           SentFormatMIMEPacket(InputGetEnableShow, 1, &buf[0]);
                           break;
                        } 
                        case InputSetEnableStatus:
                        {
                           if(Msg_Data[1] == 4 && Msg_Data[2] == 0x25  && Msg_Data[3] == 0x33  && Msg_Data[4] == 0x41  && Msg_Data[5] == 0x49)
                           {
                              AllowedFuntions.sendInputstatus = true;
                              unsigned int8 buf[1];
                              buf[0] = AllowedFuntions.sendInputstatus;
                              SentFormatMIMEPacket(InputGetEnableStatus, 1, &buf[0]);
                           }
                           else if(Msg_Data[1] == 4 && Msg_Data[2] == 0x25  && Msg_Data[3] == 0x28  && Msg_Data[4] == 0x31  && Msg_Data[5] == 0x34)
                           {
                              AllowedFuntions.sendInputstatus = false;
                              //fprintf(Serial,"Auto Show Go Info Disabled%c%c",0x1f,0x03);
                              unsigned int8 buf[1];
                              buf[0] = AllowedFuntions.sendInputstatus;
                              SentFormatMIMEPacket(InputGetEnableStatus, 1, &buf[0]);
                           }
                           break;
                        }
                        case InputGetEnableStatus:
                        {
                           unsigned int8 buf[1];
                           buf[0] = AllowedFuntions.sendInputstatus;
                           SentFormatMIMEPacket(InputGetEnableStatus, 1, &buf[0]);
                           break;
                        } 
                        case InputSetTimeToSense:
                        {
                           if(Msg_Data[1] == 2)
                           {
                              unsigned int16 valor = make16(Msg_Data[2],Msg_Data[3]);
                              EntradaDigital.TimeToSense = valor;
                              ////fprintf(Serial,"Max Customs Allowed= %lu %c%c",GoIOXstatus.maxCustoms,0x1f,0x03);
                              unsigned int8 buf[2];
                              buf[0] = EntradaDigital.TimeToSense >> 0 & 0xFF;
                              buf[1] = EntradaDigital.TimeToSense >> 8 & 0xFF;
                              SentFormatMIMEPacket(InputGetTimeToSense, 2, &buf[0]);
                           }
                           break;
                        }
                        case InputGetTimeToSense:
                        {
                           unsigned int8 buf[2];
                           buf[0] = EntradaDigital.TimeToSense >> 0 & 0xFF;
                           buf[1] = EntradaDigital.TimeToSense >> 8 & 0xFF;
                           SentFormatMIMEPacket(InputGetTimeToSense, 2, &buf[0]);
                           break;
                        }
                        case InputSetStatusDataId:
                        {
                           if(Msg_Data[1] == 2)
                           {
                              unsigned int16 valor = make16(Msg_Data[2],Msg_Data[3]);
                              EntradaDigital.StatusDataID = valor;
                              ////fprintf(Serial,"Max Customs Allowed= %lu %c%c",GoIOXstatus.maxCustoms,0x1f,0x03);
                              unsigned int8 buf[2];
                              buf[0] = EntradaDigital.StatusDataID >> 0 & 0xFF;
                              buf[1] = EntradaDigital.StatusDataID >> 8 & 0xFF;
                              SentFormatMIMEPacket(InputGetStatusDataId, 2, &buf[0]);
                           }
                           break;
                        }
                        case InputGetStatusDataId:
                        {
                           unsigned int8 buf[2];
                           buf[0] = EntradaDigital.StatusDataID >> 0 & 0xFF;
                           buf[1] = EntradaDigital.StatusDataID >> 8 & 0xFF;
                           SentFormatMIMEPacket(InputGetStatusDataId, 2, &buf[0]);
                           break;
                        }
                        case InputSetEnableSaveEeprom:
                        {
                           if(Msg_Data[1] == 4 && Msg_Data[2] == 0x25  && Msg_Data[3] == 0x33  && Msg_Data[4] == 0x41  && Msg_Data[5] == 0x49)
                           {
                              AllowedFuntions.saveEepromDueInput = true;
                              unsigned int8 buf[1];
                              buf[0] = AllowedFuntions.saveEepromDueInput;
                              SentFormatMIMEPacket(InputGetEnableSaveEeprom, 1, &buf[0]);
                           }
                           else if(Msg_Data[1] == 4 && Msg_Data[2] == 0x25  && Msg_Data[3] == 0x28  && Msg_Data[4] == 0x31  && Msg_Data[5] == 0x34)
                           {
                              AllowedFuntions.saveEepromDueInput = false;
                              //fprintf(Serial,"Auto Show Go Info Disabled%c%c",0x1f,0x03);
                              unsigned int8 buf[1];
                              buf[0] = AllowedFuntions.saveEepromDueInput;
                              SentFormatMIMEPacket(InputGetEnableSaveEeprom, 1, &buf[0]);
                           }
                           break;
                        }
                        case InputGetEnableSaveEeprom:
                        {
                           unsigned int8 buf[1];
                           buf[0] = AllowedFuntions.saveEepromDueInput;
                           SentFormatMIMEPacket(InputGetEnableSaveEeprom, 1, &buf[0]);
                           break;
                        }
                        case InputSetValueToSave:
                        {
                           if(Msg_Data[1] == 4 && Msg_Data[2] == 0x25  && Msg_Data[3] == 0x33  && Msg_Data[4] == 0x41  && Msg_Data[5] == 0x49)
                           {
                              EepromInputValueToSave = true;
                              unsigned int8 buf[1];
                              buf[0] = EepromInputValueToSave;
                              SentFormatMIMEPacket(InputGetValueToSave, 1, &buf[0]);
                           }
                           else if(Msg_Data[1] == 4 && Msg_Data[2] == 0x25  && Msg_Data[3] == 0x28  && Msg_Data[4] == 0x31  && Msg_Data[5] == 0x34)
                           {
                              EepromInputValueToSave = false;
                              //fprintf(Serial,"Auto Show Go Info Disabled%c%c",0x1f,0x03);
                              unsigned int8 buf[1];
                              buf[0] = EepromInputValueToSave;
                              SentFormatMIMEPacket(InputGetValueToSave, 1, &buf[0]);
                           }
                           break;
                        }
                        case InputGetValueToSave:
                        {
                           unsigned int8 buf[1];
                           buf[0] = EepromInputValueToSave;
                           SentFormatMIMEPacket(InputGetValueToSave, 1, &buf[0]);
                           break;
                        }
                        default:
                        {
                           //sendToPC(0xFF,Msg_Data[1],&Msg_Data[2]);
                           for(unsigned int8 i = 0; i < Msg_Leng; i++)
                           {
                              //fprintf(Serial,"%X ",Msg_Data[i]);
                           }
                           //fprintf(Serial,"%c%c",0x1f,0x03);
                           break;
                        }
                     } 
                  }
                  else
                  {
                     /*//fprintf(Serial,"Lllego mensaje invalido\n\r");
                     //fprintf(Serial,"$");
                     for(unsigned int8 i = 0; i < Msg_Leng; i++)
                     {
                        //fprintf(Serial,"%X",Msg_Data[i]);
                     }
                     //fprintf(Serial,"\n\r");*/
                  }
               }
               break;
            }
            default:
            {
                /*//fprintf(Serial,"\n\rData Unknown: ");
               
               for(unsigned int8 i = 0; i < Msg_Leng; i++)
               {
                  //fprintf(Serial,"%X",Msg_Data[i]);
               }
               //fprintf(Serial,"\n\r");*/
               break;
            }
         }
         GoIOXstatus.msgReady = False;
      }     
      
      if(GoIOXstatus.n_TimetoRqstHOS >= GoIOXstatus.p_TimetoRqstHOS)
      {
         if(GoIOXstatus.synchronized == true)
         {
            SentRequestDeviceDataMessage();
         }
         GoIOXstatus.n_TimetoRqstHOS = 0;
      }
      
      if(GoIOXstatus.lastIgnition != GoDeviceInfo.Ignition)
      {
         if(GoDeviceInfo.Ignition == true)
         {
            cmdSize = sprintf(cdmMessage, "Ignition ON");
         }
         else
         {
            cmdSize = sprintf(cdmMessage, "Ignition OFF");
            Variables structNow;
            structNow = UpDateVariables();
            if(CompareStructs(ValoresIniciales, structNow))
            {
               EepromSave(structNow);
               unsigned int8 buf[1];
               buf[0] = BloqueEeprom;
               SentFormatMIMEPacket(EepromAllSaved, 1, &buf[0]);
            }
         }
         sendToPC(ShowIgnitionStatus,cmdSize,&cdmMessage); 
         GoIOXstatus.lastIgnition = GoDeviceInfo.Ignition;
      }
      
      if(timeToShow < 10)
      {
         timeToShow = 5000;
         showDevice = false;
         if(GoIOXstatus.synchronized == true)
         {
            cmdSize = sprintf(cdmMessage, "DBI Msg. FW: v%u.%u HW: v%u.%u",firmwareMajorVersion,firmwareMinorVersion,HardwareMajorVersion,HardwareMinorVersion);
            SentFreeFormatThirdPartyData(cmdSize, &cdmMessage); 
            
            delay_ms(50);
            cmdSize = sprintf(cdmMessage, "DBI EEPROM Block: %u",BloqueEeprom);
            SentFreeFormatThirdPartyData(cmdSize, &cdmMessage); 
         }
      }
      
      if(GoIOXstatus.n_TimetoHeratBeat >= GoIOXstatus.p_TimetoHearBeat)
      {
         GoIOXstatus.n_TimetoHeratBeat = 0;
         if(GoIOXstatus.synchronized == true)
         {
            cmdSize = sprintf(cdmMessage, "DBI HeartBeat!");
            SentFreeFormatThirdPartyData(cmdSize, &cdmMessage); 
         }
      }
      
      if(InputEventDetected == true)
      {  
         if(AllowedFuntions.showInputstatus == true)
         {
            cmdSize = sprintf(cdmMessage, "Input Value: %u",!InputStatus);
            sendToPC(InputShowStatus,cmdSize,&cdmMessage);
         }
         
         if(AllowedFuntions.SendInputstatus == true)
         {
            unsigned int16 idStatus = EntradaDigital.StatusDataID;
            unsigned int32 dataStatus = make32(0,0,0,!InputStatus);
            SentThirdPartyDataStatusData(0,idStatus, dataStatus); 
         }
         
         if(AllowedFuntions.saveEepromDueInput == true)
         {
            unsigned int1 status = !InputStatus;
            if(status == EepromInputValueToSave)
            {
               Variables structNow;
               structNow = UpDateVariables();
               if(CompareStructs(ValoresIniciales, structNow))
               {
                  EepromSave(structNow);
                  unsigned int8 buf[1];
                  buf[0] = BloqueEeprom;
                  SentFormatMIMEPacket(EepromAllSaved, 1, &buf[0]);
               }
            }
         }
         
         InputEventDetected = false;
      }
      
      restart_wdt();
   }
}

Variables UpDateVariables()
{
   Variables ValorActual;
   ValorActual.MIMEackByGo            = GoIOXstatus.p_timeOutMime;
   ValorActual.MIMEackByServer        = GoIOXstatus.p_timeOutACKFromServer;
   ValorActual.MIMEmax                = GoIOXstatus.maxMIMEs;  
   ValorActual.MIMEoutEnabled         = AllowedFuntions.MIMEout;   
   ValorActual.MIMEinEnabled          = AllowedFuntions.MIMEin;   
   ValorActual.MIMEWaitACKtoSend      = AllowedFuntions.blockMIMEByACK;   
   ValorActual.MIMEautoResponseACK    = AllowedFuntions.MimeACKResponse;  
   
   ValorActual.CSackByGo              = GoIOXstatus.p_timeOutCustom;
   ValorActual.CSmax                  = GoIOXstatus.maxCustoms; 
   ValorActual.CScustomEnabled        = AllowedFuntions.customData;
   ValorActual.CSstatusEnabled        = AllowedFuntions.StatusData;
   
   ValorActual.HOStimeToResquest      = GoIOXstatus.p_TimetoRqstHOS;
   ValorActual.HOSrqstEnabled         = AllowedFuntions.rqstGoInfo;   
   ValorActual.HOSautoShowEnabled     = AllowedFuntions.showGoInfo;  
     
   ValorActual.ACTmode                = Actuator.OutputMode;
   ValorActual.ACTstatus              = Actuator.OutputState;
   ValorActual.ACTtoReset             = Actuator.TimeToReset;
   ValorActual.ACTinReset             = Actuator.TimeInReset;
   ValorActual.ACTtimeOutPIN          = Actuator.P_TimeOutPass;
   ValorActual.ACTenableClientRequest = AllowedFuntions.rqstActuator;
   
   ValorActual.DBItimeToHeartBeat     = GoIOXstatus.p_TimetoHearBeat;
   ValorActual.DBIheartBeatEnabled    = GoIOXstatus.HeartbeatEnable;
   
   ValorActual.InputEnableShow        = AllowedFuntions.showInputstatus;
   ValorActual.InputEnablestatus      = AllowedFuntions.sendInputstatus;
   ValorActual.InputEnableSaveEeprom  = AllowedFuntions.saveEepromDueInput;
   ValorActual.InputTimeToSense       = EntradaDigital.TimeToSense;
   ValorActual.InputStatusDataId      = EntradaDigital.StatusDataID;
   ValorActual.InputValueToSave       = EepromInputValueToSave;
   ValorActual.empty = 0;
   
   return ValorActual;
}



