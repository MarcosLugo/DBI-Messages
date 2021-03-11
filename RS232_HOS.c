#include <RS232_HOS.h>
/*
/////Eeprom registers defin
#byte TXSTA1 = 0xFAC
#byte RCSTA1 = 0xFAB
#byte INTCON = 0xFF2
//#byte PIE3   = 0xFA3

#define  Sync  4
#define  SPEN  7
#define  CREN  4

//#define  GIE  7
#define  PEIE 6
#define  RC2IE 5

#bit serialInterrupt=PIE3.RC2IE
#bit serialReception=RCSTA1.CREN
#bit GlobalInterrupt=INTCON.GIE
#bit Interrupt=INTCON.PEIE
///////////////////////////*/

void HOS_Init(void)
{
   GoIOXstatus.synchronized = false;
   GoIOXstatus.msgReady = false;
   GoIOXstatus.p_timeOutCustom = ValoresIniciales.CSackByGo;
   GoIOXstatus.p_timeOutMime = ValoresIniciales.MIMEackByGo; 
   GoIOXstatus.n_timeOutCustom = 0;
   GoIOXstatus.n_timeOutMime = 0;
   GoIOXstatus.synchronized = False;
   GoIOXstatus.WattingACK = 0;
   GoIOXstatus.timeToSync = 0;
   GoIOXstatus.lastSynchronized = false;
   
   GoIOXstatus.maxCustoms = ValoresIniciales.CSmax;
   GoIOXstatus.CustomsTime = 0;
   GoIOXstatus.customsSent = 0;
   
   GoIOXstatus.p_timeOutACKFromServer = ValoresIniciales.MIMEackByServer;
   GoIOXstatus.n_timeOutACKFromServer = 0;
   GoIOXstatus.WattingACKmimeFromServer = false;
   GoIOXstatus.maxMIMEs = ValoresIniciales.MIMEmax;
   GoIOXstatus.MIMESent = 0;
   GoIOXstatus.MIMETime = 0;
   
   GoIOXstatus.p_TimetoRqstHOS = ValoresIniciales.HOStimeToResquest;
   GoIOXstatus.n_TimetoRqstHOS = 0;
   GoIOXstatus.lastIgnition = false;
   
   
   GoIOXstatus.WattingGoInfoResponse = false;
   GoIOXstatus.p_timeOutGoInfoResponse = 5000;
   GoIOXstatus.n_timeOutGoInfoResponse = 0;
   
   GoIOXstatus.p_TimetoHearBeat = ValoresIniciales.DBItimeToHeartBeat; //cada 15 min
   GoIOXstatus.n_TimetoHeratBeat = 0;
   GoIOXstatus.HeartbeatEnable = ValoresIniciales.DBIheartBeatEnabled;
   
   HOS_Command   = 0;
   Msg_Type  = 0;
   Msg_Leng  = 0;
   Msg_Index = 0;
   Msg_ChkA  = 0;
   Msg_ChkB  = 0;
}

void HOS_Set_Time(void)
{
   GoIOXstatus.timeToSync += interruptTime;
   GoIOXstatus.n_TimetoRqstHOS += interruptTime;
   
   if( GoIOXstatus.HeartbeatEnable == true)
   {
      GoIOXstatus.n_TimetoHeratBeat += interruptTime;
   }
   
   GoIOXstatus.CustomsTime += interruptTime;
   if(GoIOXstatus.CustomsTime > 600000) 
   {
      GoIOXstatus.CustomsTime = 0;
      GoIOXstatus.customsSent = 0;
   }
   
   GoIOXstatus.MIMETime += interruptTime;
   if(GoIOXstatus.MIMETime > 600000)
   {
      GoIOXstatus.MIMESent = 0;
      GoIOXstatus.MIMETime = 0;
   }
   
   if(GoIOXstatus.WattingACK != 0)
   {
      GoIOXstatus.n_timeOutCustom += interruptTime;
      if(GoIOXstatus.n_timeOutCustom > GoIOXstatus.p_timeOutCustom)
      {
         GoIOXstatus.synchronized = False;
         GoIOXstatus.n_timeOutCustom = 0;
         GoIOXstatus.WattingACK = 0;
      }
   }
   
   if(GoIOXstatus.WattingACKmime == true)
   {
      GoIOXstatus.n_timeOutMime+= interruptTime;
      if(GoIOXstatus.n_timeOutMime > GoIOXstatus.p_timeOutMime)
      {
         GoIOXstatus.synchronized = False;
         GoIOXstatus.WattingACKmime = false;
         GoIOXstatus.n_timeOutMime = 0;
      }
   }
   
   if(GoIOXstatus.WattingACKmimeFromServer == true)
   {
      GoIOXstatus.n_timeOutACKFromServer+= interruptTime;
      if(GoIOXstatus.n_timeOutACKFromServer >= GoIOXstatus.p_timeOutACKFromServer)
      {
         GoIOXstatus.WattingACKmimeFromServer = false;
         GoIOXstatus.n_timeOutACKFromServer = 0;
      }
   }
   
   if(GoIOXstatus.WattingGoInfoResponse == true)
   {
      GoIOXstatus.n_timeOutGoInfoResponse += interruptTime;
      if(GoIOXstatus.n_timeOutGoInfoResponse >= GoIOXstatus.p_timeOutGoInfoResponse)
      {
         GoIOXstatus.WattingGoInfoResponse = false;
         GoIOXstatus.n_timeOutGoInfoResponse = 0;
         GoIOXstatus.synchronized = False;
      }
   }
}

void HOS_Set_Buffer(unsigned int8 Dato)
{
   switch (HOS_Command)
   {
      case 0:
      {
         if(Dato == 0x02)
         {
            Msg_Index = 0;
            HOS_Command++;
            GoIOXstatus.msgReady = False;
         }
         break;
      }
      case 1:
      {
         Msg_Type  = Dato;
         HOS_Command++;
         break;
      }
      case 2:
      {
         Msg_Leng  = Dato;
         HOS_Command++;
         break;
      }
      case 3:
      {
         if(Msg_Index >= Msg_Leng)
         {
            Msg_ChkA = Dato;
            HOS_Command++;
         }
         else
         {
            Msg_Data[Msg_Index++] = Dato;
         }
         break;
      }
      case 4:
      {
         Msg_ChkB = Dato;
         HOS_Command++;
         break;
      }
      case 5:
      {
         if(Dato == 0x03)
         {
            unsigned int8 A = 2, B = 2;
            
            A = A + Msg_Type;
            B = A + B;
            A = A + Msg_Leng;
            B = A + B;
            for(unsigned int i = 0; i < Msg_Leng; i++)
            {
               A = A + Msg_Data[i];
               B = A + B;
            }
            
            if((A == Msg_ChkA) && (B == Msg_ChkB))
            {
              GoIOXstatus.msgReady = True;
               //output_toggle(Led);
            }            
         }
         HOS_Command = 0;
         break;
      }
      default:
      {
         Msg_Index = 0;
         HOS_Command = 0;
         GoIOXstatus.msgReady = False;
         break;
      }
   }   
}

void SentThirdPartyDataStatusData(unsigned int1 Prioritario, unsigned int16 Data_Id, unsigned int32 Data)
{
   HOS_CHKA = 0;
   HOS_CHKB = 0;
   
   unsigned int8 DataOut;
   
   DataOut = 0x02;
   CheckSum(DataOut);
   
   if(Prioritario == true){DataOut = ThirdPartyPriorityStatusData;}
   else                   {DataOut = ThirdPartyDataStatusData;}
   CheckSum(DataOut);
   
   DataOut = 0x06; //Longitud
   CheckSum(DataOut);
   
   DataOut = MAKE8(Data_Id, 0); //Id
   CheckSum(DataOut);
   
   DataOut = MAKE8(Data_Id, 1); //Id
   CheckSum(DataOut);
   
   DataOut = Data;
   CheckSum(DataOut);
   
   DataOut = Data >> 8;
   CheckSum(DataOut);
   
   DataOut = Data >> 16;
   CheckSum(DataOut);
   
   DataOut = Data >> 24;
   CheckSum(DataOut);
   
   putC(HOS_CHKA,IOX);
   putC(HOS_CHKB,IOX);
   putC(0x03,IOX);
   GoIOXstatus.WattingACK = 2;
   return ;
}

void SentFreeFormatThirdPartyData(unsigned int8 Size, unsigned int8 *Data)
{
   HOS_CHKA = 0;
   HOS_CHKB = 0;
   
   unsigned int8 DataOut;
   
   DataOut = 0x02;
   CheckSum(DataOut);
   
   DataOut = FreeFormatThirdPartyData;
   CheckSum(DataOut);
   
   if(Size > 27){Size = 27;}
   DataOut = Size; //Longitud
   CheckSum(DataOut);
   
   for(unsigned int8 i = 0; i < Size; i++)
   {
      DataOut = *Data++;
      CheckSum(DataOut);
   }
   
   putC(HOS_CHKA,IOX);
   putC(HOS_CHKB,IOX);
   putC(0x03,IOX);
   GoIOXstatus.WattingACK = 1;
   GoIOXstatus.customsSent++;
   return ;
}

void SentBinaryDataPacket(unsigned int8 Size, unsigned int8 *Data)
{ 
   HOS_CHKA = 0;
   HOS_CHKB = 0;
   
   unsigned int8 DataOut;
   
   DataOut = 0x02;
   CheckSum(DataOut);
   //fprintf(Serial,"Binary %X ",DataOut);
   
   DataOut = BinaryDataPacketOUT;
   CheckSum(DataOut);
   //fprintf(Serial," %X ",DataOut);
   
   if(Size > 250){Size = 250;}
   DataOut = Size; //Longitud
   CheckSum(DataOut);
   //fprintf(Serial," %X ",DataOut);
   
   for(unsigned int8 i = 0; i < Size; i++)
   {
      DataOut = *Data++;
      CheckSum(DataOut);
      //fprintf(Serial," %X ",DataOut);
   }
   
   putC(HOS_CHKA,IOX);
   //fprintf(Serial," %X ",HOS_CHKA);
   putC(HOS_CHKB,IOX);
   //fprintf(Serial," %X ",HOS_CHKB);
   putC(0x03,IOX);
   //fprintf(Serial,"03\n\r");
   GoIOXstatus.WattingACKmime = True;
   GoIOXstatus.WattingACKmimeFromServer = true;
   GoIOXstatus.n_timeOutACKFromServer = 0;
   return ;
}

void SentMIMEPacket(unsigned int16 Size, unsigned int8 *Data)
{
   unsigned int16 IndexDataIN = 0;
   unsigned int8  BufferOut[250];
   unsigned int8  indexPacket = 0;
   unsigned int8  index_dataOut = 0;
   
   BufferOut[index_dataOut++]  = indexPacket;
   BufferOut[index_dataOut++]  = Size_type;
   for(unsigned int8 i = 0; i < Size_type; i++)
   {
      BufferOut[index_dataOut++] = (unsigned int8)type[i];
   }
   
   unsigned int32 Payload = Size;
   BufferOut[index_dataOut++]  = MAKE8(Payload, 0);
   BufferOut[index_dataOut++]  = MAKE8(Payload, 1);
   BufferOut[index_dataOut++]  = MAKE8(Payload, 2);
   BufferOut[index_dataOut++]  = MAKE8(Payload, 3);
   
   unsigned int1 final = false;
   for(unsigned int16 j = 0; j <= (244 - Size_type) ; j++)
   {
      if(index_dataOut < (Size + 6 + Size_type))
      {
         BufferOut[index_dataOut] = *Data++;
      }
      else
      {
         final = true;
         j = 255; //aseguramos salir del ciclo
      }
      restart_wdt();      
      IndexDataIN   ++;
      index_dataOut ++;
   }
   index_dataOut--;
   Data--;
   SentBinaryDataPacket(index_dataOut, &BufferOut);
   delay_ms(100);
   
   unsigned int8 index = 1;
   while(final == false)
   {     
      index_dataOut = 0;
      BufferOut[0]  = index;
      index++;
      index_dataOut++;
      
      for(unsigned int8 i = 0; i < 249; i++)
      {
         BufferOut[index_dataOut++] = *Data++;
         IndexDataIN ++;
         if(IndexDataIN >= (Size + 1))
         {
            final = true;
            break;
         }
      }
      
      SentBinaryDataPacket(index_dataOut, &BufferOut);
      delay_ms(100);
      restart_wdt();
   }
   return ;
}

void SentRequestDeviceDataMessage(void)
{
   HOS_CHKA = 0;
   HOS_CHKB = 0;
   
   unsigned int8 DataOut;
   
   DataOut = 0x02;
   CheckSum(DataOut);
   
   DataOut = RequestDeviceDataMessage;
   CheckSum(DataOut);
   
   DataOut = 0; //Longitud
   CheckSum(DataOut);
   
   putC(HOS_CHKA,IOX);
   putC(HOS_CHKB,IOX);
   putC(0x03,IOX);
   GoIOXstatus.WattingGoInfoResponse = true;
   
   return ;
}

void SentDeviceDataACK(void)
{
   HOS_CHKA = 0;
   HOS_CHKB = 0;
   
   unsigned int8 DataOut;
   
   DataOut = 0x02;
   CheckSum(DataOut);
   
   DataOut = DeviceDataACK;
   CheckSum(DataOut);
   
   DataOut = 0; //Longitud
   CheckSum(DataOut);
   
   putC(HOS_CHKA,IOX);
   putC(HOS_CHKB,IOX);
   putC(0x03,IOX);
   return ;
}

void SentHandShakeRequest(void)
{
   putC(0x55,IOX);
   return;
}

void SentHandshakeConfirmation(unsigned int16 DeviceId, int1 ACKConfirmation, int1 BinaryDataWrapping)
{
   HOS_CHKA = 0;
   HOS_CHKB = 0;
   
   unsigned int8 DataOut;
   
   DataOut = 0x02;
   CheckSum(DataOut);
   
   DataOut = HandshakeConfirmation;
   CheckSum(DataOut);
   
   DataOut = 4; //Longitud
   CheckSum(DataOut);
   
   DataOut = MAKE8(DeviceId, 0); //Id
   CheckSum(DataOut);
   
   DataOut = MAKE8(DeviceId, 1); //Id
   CheckSum(DataOut);
   
   DataOut = ((0b00000000) | (ACKConfirmation & 0b00000001) | ((BinaryDataWrapping << 1) & 0b00000010));
   CheckSum(DataOut);
   
   DataOut = 0;
   CheckSum(DataOut);
   
   putC(HOS_CHKA,IOX);
   putC(HOS_CHKB,IOX);
   putC(0x03,IOX);
   GoIOXstatus.synchronized = True;
   return ;
}

void CheckSum(unsigned int8 Data)
{
   HOS_CHKA = HOS_CHKA + Data;
   HOS_CHKB = HOS_CHKA + HOS_CHKB;
   putC(Data,IOX);
   //fprintf(Serial,"%X ",Data);
}

void GetParameters(HOS_DeviceData *Dato)
{
   HOS_DeviceData Result;   
   Result.DateTime  = Make32(Msg_Data[0],Msg_Data[1],Msg_Data[2],Msg_Data[3]);
   Result.Latitude  = Make32(Msg_Data[4],Msg_Data[5],Msg_Data[6],Msg_Data[7]);
   Result.Longitud  = Make32(Msg_Data[8],Msg_Data[9],Msg_Data[10],Msg_Data[11]);
   Result.Velocidad = Msg_Data[12];
   Result.RPM       = Make16(Msg_Data[13],Msg_Data[14]);
   Result.Odometro  = Make32(Msg_Data[15],Msg_Data[16],Msg_Data[17],Msg_Data[18]);
   Result.GpsValid           =  Msg_Data[19] & 0b00000001;
   Result.Ignition           = (Msg_Data[19] & 0b00000010) >> 1;
   Result.EngineActivity     = (Msg_Data[19] & 0b00000100) >> 2;
   Result.DateValid          = (Msg_Data[19] & 0b00001000) >> 3;
   Result.SpeedFromEngine    = (Msg_Data[19] & 0b00010000) >> 4;
   Result.OdometerFromEngine = (Msg_Data[19] & 0b00100000) >> 5;
   Result.TripOdometer       = Make32(Msg_Data[20],Msg_Data[21],Msg_Data[22],Msg_Data[23]);
   Result.TotalEngineHours   = Make32(Msg_Data[24],Msg_Data[25],Msg_Data[26],Msg_Data[27]);
   Result.TripDuration       = Make32(Msg_Data[28],Msg_Data[29],Msg_Data[30],Msg_Data[31]);
   Result.GoID               = Make32(Msg_Data[32],Msg_Data[33],Msg_Data[34],Msg_Data[35]);
   Result.DriverId           = Make32(Msg_Data[36],Msg_Data[37],Msg_Data[38],Msg_Data[39]);
   memcpy(Dato, &Result, sizeof(HOS_DeviceData));
   return;
}

void SentFormatMIMEPacket(unsigned int8 comando, unsigned int8 Size, unsigned int8 *Data)
{
   unsigned int8  BufferOut[250];
   unsigned int8  index_dataOut = 0;
   unsigned int8 ca = 0;
   unsigned int8 cb = 0;
   
   BufferOut[index_dataOut++]  = 0;
   BufferOut[index_dataOut++]  = Size_type;
   for(unsigned int8 i = 0; i < Size_type; i++)
   {
      BufferOut[index_dataOut++] = (unsigned int8)type[i];
   }
   unsigned int32 Payload = Size + 4;
   BufferOut[index_dataOut++]  = MAKE8(Payload, 0);
   BufferOut[index_dataOut++]  = MAKE8(Payload, 1);
   BufferOut[index_dataOut++]  = MAKE8(Payload, 2);
   BufferOut[index_dataOut++]  = MAKE8(Payload, 3);
   
   BufferOut[index_dataOut++]  = comando;
   ca = ca + comando;
   cb = cb + ca;
   BufferOut[index_dataOut++]  = Size;
   ca = ca + Size;
   cb = cb + ca;
   for(unsigned int8 j = 0; j < Size ; j++)
   {
      unsigned int8 var = *Data++;
      BufferOut[index_dataOut] = var;
      ca = ca + var;
      cb = cb + ca;  
      index_dataOut ++;
   }
   BufferOut[index_dataOut++]  = ca;
   BufferOut[index_dataOut++]  = cb;
   
   SentBinaryDataPacket(index_dataOut, &BufferOut);
   GoIOXstatus.MIMESent++;
   return ;
}

