#define msjOK       1
#define chkFail     2
#define formatFail  3
#define free        255

#define PingRequest            0x00    //<
#define Pingresponse           0x01    //>

#define MsgFailACK             0x08    //>
#define MsgFailFormat          0x09    //>

#define TextMsg                0x10    //<
#define TextMsgFromServer      0x11    //>
#define TextMsgReceived        0x12    //>
#define TextMsgSent            0x13    //>
#define TextMsgDelivered       0x15    //>
#define TextMsgFailed          0x16    //>
#define TextMsgFromServerNotSupported      0x17    //>
#define TextMsgLimitReached    0x18    //>


#define ShowGoInfoError        0x29    //>
#define GetVehicleInfo         0x30    //<
#define ShowVehicleInfo        0x31    //>
#define GetIOXStatus           0x32    //<
#define ShowIOXstatus          0x33    //>
#define SendStatusData         0x34    //<
#define StatusDataReceived     0x35    //>
#define StatusDataSent         0x36    //>
#define SendCustomData         0x37    //<
#define CustomDataReceived     0x38    //>
#define CustomDataSent         0x39    //>
#define CustomDataLimitReached 0x40    //>
#define CommandNotAllowed      0x41    //>
#define StopWattingMIMEACK     0x42    //< 
#define MIMEACKWattingStopped  0x43    //>
#define MIMEWattingACK         0x44    //>
#define StopWattingCSACK       0x45    //<
#define CSACKWattingStopped    0x46    //>
#define CSWattingACK           0x47    //>
#define GetIgnitionstatus      0x48    //>
#define ShowIgnitionStatus     0x49    //>

#define GetActuatorMode        0x50  //<
#define ShowActuatorMode       0x51  //>
#define SetTimedOutput         0x52  //<
#define SetOuputOnOff          0x53  //<
#define ShowTimedOutputReason  0x54  //>
#define ShowOutputReason       0x55  //>
#define CancelOutput           0x56  //<
#define CancelResult           0x57  //>
#define GetOutputStatus        0x58  //<
#define ShowOutputStatus       0x59  //>

#define ResetDBI            0x60   //<
#define ResetDBIByServer    0x61   //>
#define ResetDBIByClient    0x62   //>
#define ResetDBIByAutomatic 0x63   //>
#define DBIInitialized      0x64   //>
#define RqstFW              0x65   //<
#define AsnwFW              0x66   //>
#define RqstHW              0x67   //<
#define AsnwHW              0x68   //>
#define DBIGetSerie         0x69   //<>

#define InputGetStatus      0x70   //< 
#define InputShowStatus     0x71   //>

#define LengthInvalid       0xFE   //>
#define ComandoUnidentified 0xFF   //>

unsigned int1 sentByPC = false;

unsigned int8 pc_CMD = 0;
unsigned int8 pc_Index  = 0;
unsigned int8 chkA = 0;
unsigned int8 chkB = 0;
unsigned int8 pc_result = free;
struct  
{
   unsigned int8 chkA;
   unsigned int8 chkB;
   unsigned int8 comando;
   unsigned int8 longitud;
   unsigned int8 Dato[200];
}msgFromPC;

void set_BufferPC(unsigned int8 data);
void sendToPC(unsigned int8 Command, unsigned int8 length, unsigned int8 *Data);

void set_BufferPC(unsigned int8 data)
{
   switch(pc_CMD)
   {
      case 0:
      {
         if(data == 0x02) //Inicio de trama
         {
            pc_CMD++;
         }
         chkA = data;
         chkB = data;
         msgFromPC.comando = 0;
         msgFromPC.longitud = 0;
         memset(msgFromPC.Dato, 0xFF, 200);
         pc_Index = 0;
         break;  
      }
      case 1:
      {
         msgFromPC.comando = data;
         pc_CMD++;
         chkA = chkA + data;
         chkB = chkA + chkB;
         memset(msgFromPC.Dato, 0xFF, 200); //esto lo hago aqui para no estar limpiando el buffer en cada byte de msj incompleto
         break;  
      }
      case 2:
      {
         msgFromPC.longitud = data;
         chkA = chkA + data;
         chkB = chkA + chkB;
         pc_CMD++;
         break;  
      }
      case 3:
      {
         if(pc_Index < msgFromPC.longitud)
         {
            msgFromPC.Dato[pc_Index] = data;
            chkA = chkA + data;
            chkB = chkA + chkB;
            pc_Index++;
         }
         else
         {
            msgFromPC.chkA = data;
            pc_CMD++;
         }
         break;  
      }
      case 4:
      {
         msgFromPC.chkB = data;
         pc_CMD++;
         break;  
      }
      case 5:
      {
         if(data == 0x03) //Final de trama
         {
            if(msgFromPC.chkA == chkA && msgFromPC.chkB == chkB)
            {
               pc_result = msjOK;
               //enviar formato ok
            }
            else
            {
               pc_result = chkFail;
               //enviar checksum fail
            }
         }
         else
         {
            pc_result = formatFail;
            //enviar formato invalido
         }
         pc_CMD = 0;
         break;  
      }
      default:
      {
         pc_CMD = 0;
         break;
      }
      
   }
}

void sendToPC(unsigned int8 Command, unsigned int8 length, unsigned int8 *Data)
{
   char aux[10];
   unsigned int8 auxSize = 0;
   char datos[250];
   unsigned int8 index = 0;
   
   datos[index++] = 0x02;
   datos[index++] = 0x1F;
   auxSize = sprintf(aux, "%X", Command);
   for(unsigned int8 i = 0; i < auxSize; i++)
   {
      datos[index++] = aux[i];
   }
   
   datos[index++] = 0x1F;
   auxSize = sprintf(aux, "%u", length);
   for(unsigned int8 i = 0; i < auxSize; i++)
   {
      datos[index++] = aux[i];
   }
   
   datos[index++] = 0x1F;
   for(unsigned int8 i = 0; i < length; i++)
   {
      datos[index++] = *Data++;
   }
   datos[index++] = 0x1F;
   
   unsigned int8 localchkA = 0;
   unsigned int8 localchkB = 0;
   for(unsigned int8 i = 0; i < index; i++)
   {  
      unsigned int8 out = datos[i];
      localchkA = localchkA + out;
      localchkB = localchkA + localchkB;
      putc(out,Serial);
   }
   
   unsigned int16 checksum = make16(localchkA,localchkB);
   auxSize = sprintf(aux, "%LX", checksum);
   for(unsigned int8 i = 0; i < auxSize; i++)
   {
      putc(aux[i],Serial);
   }
   putc(0x1F,Serial);
   putc(0x03,Serial);
}
