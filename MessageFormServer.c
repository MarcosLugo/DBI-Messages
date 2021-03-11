
#define MimeMsgFromServer        0xA1 //<
#define MimeMsgFromPC            0xA2 //>
#define MimeSetTimeOutBinary     0xA3 //<
#define MimeGetTimeOutBinary     0xA4 //<>
#define MimeSetTimeOutACKServer  0xA5 //<
#define MimeGetTimeOutACKServer  0xA6 //<>
#define MimeSetMaxMimes          0xA7 //<
#define MimeGetMaxMimes          0xA8 //<>
#define MimeSetEnableOut         0xA9 //<
#define MimeGetEnableOut         0xAA //<>
#define MimeSetEnableIn          0xAB //<
#define MimeGetEnableIn          0xAC //<>
#define MimeSetMsgACKFromServer  0xAD //>
#define MimeGetMsgACKFromServer  0xAE //<>
#define MimeSetWaitACKFromServer 0xAF //<
#define MimeGetWaitACKFromServer 0xA0 //<>

#define CSSetTimeOut             0xB0 //<
#define CSGetTimeOut             0xB1 //<>
#define CSSetMaxCustoms          0xB2 //<
#define CSGatMaxCustoms          0xB3 //<>
#define CSSetAllowedCustoms      0xB4 //<
#define CSGetAllowedCustoms      0xB5 //<>
#define CSSetAllowedStatusData   0xB6 //<
#define CSGetAllowedStatusData   0xB7 //<>

#define ActSetMode               0xC0 //<
#define ActGetMode               0xC1 //<>
#define ActSetConfigTimedOutput  0xC2 //<
#define ActGetConfigTimedOutput  0xC3 //<>
#define ActSetTimeOutPass        0xC4 //<
#define ActGetTimeOutPass        0xC5 //<>
#define ActGetActualValue        0xC6 //<>
#define ActSetOutput             0xC7 //<
#define ActSettimedOutput        0xC8 //<
#define ActSetPass               0xC9 //<>
#define ActError                 0xCA //>
#define ActSetEnableClientRqst   0xCB //>
#define ActGetEnableClientRqst   0xCD //<>

#define DBISetTimeHeartBeat      0xD0 //<
#define DBIGetTimeHeartBeat      0xD1 //<>
#define DBISetEnableHeartBeat    0xD2 //>
#define DBIGetEnableHeartBeat    0xD3 //<>
#define DBIGetFwVersion          0xD4 //<>
#define DBIGetHwVersion          0xD5 //<>
#define DBIResetHardware         0xD6 //<
#define DBIForceToSentHearbeat   0xD7 //<
#define DBIGetAllInfo            0xD8 //<>
#define DBIGetDeviceSerie        0xD9 //<>

#define GoInfoSetRequestTime     0xE0 //<
#define GoInfoGetRequestTime     0xE1 //>
#define GoInfoSetEnableShow      0xE2 //<
#define GoInfoGetEnableShow      0xE3 //>
#define GoInfoSetEnableRqst      0xE4 //<
#define GoInfoGetEnableRqst      0xE5 //>

#define InputSetEnableShow       0x80 //<
#define InputGetEnableShow       0x81 //<>
#define InputSetEnableStatus     0x82 //<
#define InputGetEnableStatus     0x83 //<>
#define InputSetStatusDataId     0x84 //<
#define InputGetStatusDataId     0x85 //<>
#define InputSetTimeToSense      0x86 //<
#define InputGetTimeToSense      0x87 //<>

#define InputSetEnableSaveEeprom 0x90 //<
#define InputGetEnableSaveEeprom 0x91 //<>
#define InputSetValueToSave      0x92 //<
#define InputGetValueToSave      0x93 //<>
#define EepromSaveAll            0x94 //<
#define EepromAllSaved           0x95 //>
#define EepromBlockError         0x96 //>

#define ACK                      0xFB //>

unsigned int1 isFormatedMessage(unsigned int8 *dato);

unsigned int1 isFormatedMessage(unsigned int8 *dato)
{
   unsigned int1 result = false;
   unsigned int8 cA = 0;
   unsigned int8 cB = 0;
   unsigned int8 ccA = 0;
   unsigned int8 ccB = 0;
   unsigned int8 comando = *dato++;
   unsigned int8 lengh = *dato++;
   
   ccA = ccA + comando;
   ccB = ccA + ccB;
   ccA = ccA + lengh;
   ccB = ccA + ccB;
   
   if(lengh <= 200)
   {
      unsigned int8 dt;
      for(unsigned int8 i = 0; i < lengh; i++)
      {
         dt = *dato++;
         ccA = ccA + dt;
         ccB = ccA + ccB;
         //fprintf(Serial,"%X",dt);
      }
      dt = *dato++;
      cA = dt;
      //fprintf(Serial,"%X",dt);
      dt = *dato++;
      cB = dt;
      //fprintf(Serial,"%X",dt);
      
      if(cA == ccA && cB == ccB)
      {
         result = true;
      }
   }
   
   return result;
}
