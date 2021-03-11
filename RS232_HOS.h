#define HandshakeRequest             0x01  //Go to external device  confirma que el dispositivo esta emparejado
#define ThirdPartyDataACK            0x02  //Go to external device  confirma que el mensaje third party se recibio
#define GoDeviceData                 0x21  //Go to external device  contiene infomacion del go como rpm, vel, etc, si somos un dispositivo hos(id=4141) se envia cada 2 segundos y se le tiene que responder un el ACK con el comando 84, si no solo se envia como respuesta al comando requestDataMesage
#define BinaryDataResponse           0x22  //Go to external device  confirma que la transmision del"BinaryDataPacket (0x86)" se realizo exitoso o mal
#define BinaryDataPacketIN           0x23  //Go to external device  contiene el mensaje enviado desde el servidor
#define HandshakeConfirmation        0x81  //External device to Go  es la respuesta al comando "0x01" y contiene informacion como el external device que somos, si queremos ACK y si queremos el mensaje binary data formateado
#define ThirdPartyDataStatusData     0x80  //External device to Go  Se envia cuando se quiere reflejar un parametro en mygeotab y se le debe responder el comando 0x02 por parte del go
#define FreeFormatThirdPartyData     0x82  //External device to Go  Se envia cuando se quiere reflejar un customdata en mygeotab y se le debe responder el comando 0x02 por parte del go
#define DeviceDataACK                0x84  //External device to Go  Se envia como ACK al comando 0x21 cuando estamos en HOS device y tenemos hasta 30 segundos para enviarlo
#define RequestDeviceDataMessage     0x85  //External device to Go  Se envia cuando se quiere recibir el comando 0x21
#define BinaryDataPacketOUT          0x86  //External device to Go  Se envia cuando se enviar un binaryData al servidor
#define ThirdPartyPriorityStatusData 0x87  //External device to Go  Se envia cuando se quiere reflejar un parametro en mygeotab y se le debe responder el comando 0x02 por parte del go (si tiene el iridium puede que el mensaje salga por ahi)

typedef struct
{
   unsigned int8  MsgType;        //Tipo de mensaje segun la documentacion
   unsigned int8  MsgBodyLength;  //longitud
   unsigned int8  Data[250];      //datos de entrada
   unsigned int1  GoFormat;       //debido a que existe la opcion de que lleguen mensajes sin el formato que tiene normalmente  el go7, cualquier mensaje que llegue y no sea valido lo tomaremos pero indicaremos que no tiene el formato
}HOS_Message;

typedef struct
{
   unsigned int32 DateTime;       //Fecha y hora en segundos desde  de enero de 2002
   unsigned int32 Latitude;       //Latitud GPS
   unsigned int32 Longitud;       //Longitud GPS
   unsigned int8  Velocidad;      //Velocidad del Vehiculo, si no tiene datos de motor el dato sera del gps
   unsigned int16 RPM;            //
   unsigned int32 Odometro;       //Si el odometro no esta disponible se usara la distancia del gps
   unsigned int1  GpsValid;  
   unsigned int1  Ignition;  
   unsigned int1  EngineActivity;  
   unsigned int1  DateValid;  
   unsigned int1  SpeedFromEngine;  
   unsigned int1  OdometerFromEngine;  
   unsigned int32 TripOdometer;   
   unsigned int32 TotalEngineHours;  
   unsigned int32 TripDuration;   
   unsigned int32 GoID;  
   unsigned int32 DriverId;   
}HOS_DeviceData;

typedef struct
{
   unsigned int1  TransmissionSucces;  
}HOS_BinaryDataResponse;

typedef struct
{
   unsigned int16 ExternalDevice;
   unsigned int1  HandshakeConfirmationACK;  
   unsigned int1  BinaryDataWrapping;  
}HOS_HandShakeConfirmation;

typedef struct
{
   unsigned int16 DataID;
   unsigned int32 Data;  
}HOS_StatusData;

typedef struct
{
   unsigned int8  Size; //max 27
   unsigned int8  Data[27];  
}HOS_FreeFormat;

typedef struct
{
   unsigned int8  Size; //max 250
   unsigned int8  Data[250];  
}HOS_BinaryDataPacket;

struct
{
   unsigned int1  WattingACKmime; //bandera que indica si esta esperando un ack del go porque se envio un mime
   unsigned int16 p_timeOutMime; //tiempo programado para esperar ack del go7
   unsigned int16 n_timeOutMime; //tiempo actual para hacer comparacion
   unsigned int16 maxMIMEs; //maximo numero de mimes que se pueden enviar
   unsigned int16 MIMESent;//contador de customs en 10 minutos
   unsigned int32 MIMETime; //tiempo que ha transcurrido enviando mimes
   
   unsigned int16 p_timeOutCustom; //tiempo programado para esperar ack del go7
   unsigned int16 n_timeOutCustom; //tiempo actual para hacer comparacion
   unsigned int8  WattingACK; //este valor si es cero equivale a no esta esperando ACK si es 1 esta esperando ACK de un custom, si es 2 esta esperando ACK de status data
   
   unsigned int16 timeToSync; //contador de tiempo para enviar comando de sincronia cada segundo si no esta sincronizado
   unsigned int1  synchronized;  //bandera que indica si esta en sicronia
   unsigned int1  lastSynchronized;  //bandera para identificar cambios en la sincronia
   unsigned int1  msgReady;  //bandera para saber si hay un mensaje completo desde el go7   
   
   unsigned int16 maxCustoms; //maximo numero de customs data que se pueden enviar
   unsigned int16 customsSent;//contador de customs en 10 minutos
   unsigned int32 CustomsTime; //tiempo que ha transcurrido enviando customs
   
   unsigned int32 p_TimetoRqstHOS; //este tiempo es el configurado para preguntar por la info del go7
   unsigned int32 n_TimetoRqstHOS; //este tiempo que ha transcurrido para preguntar por el comando
   unsigned int1  WattingGoInfoResponse; //bandera que indica si esta esperando un ack del servidor
   unsigned int32 p_timeOutGoInfoResponse; //tiempo programado para esperar ack del servidor
   unsigned int32 n_timeOutGoInfoResponse; //tiempo transcurrido esperando ACK del servidor
   unsigned int1  lastIgnition; //bandera para hacer comparacion y detectar cambios
   
   unsigned int1  WattingACKmimeFromServer; //bandera que indica si esta esperando un ack del servidor
   unsigned int32 p_timeOutACKFromServer; //tiempo programado para esperar ack del servidor
   unsigned int32 n_timeOutACKFromServer; //tiempo transcurrido esperando ACK del servidor
   
   unsigned int32 p_TimetoHearBeat; //tiempo programado para hacer el heartbeat
   unsigned int32 n_TimetoHeratBeat; //tiempo actual para hacer el heartbeat
   unsigned int1  HeartbeatEnable; //bandera que indica si envia en heartbeat por tiempo
}GoIOXstatus;

unsigned int8 HOS_CHKA, HOS_CHKB;
unsigned int1 GoInfoDataReady = false;

//int1 HOS_WattingBinaryDataACK;
//int1 HOS_Binary_Succes; //esta bandera se pondra en 1 cuando el mensaje se haya enviado correctamente

unsigned int8 HOS_Command = 0; //comando de maquina de estados para procesar mensaje
unsigned int8 Msg_Type  = 0;
unsigned int8 Msg_Leng  = 0;
unsigned int8 Msg_Data[250]; 
unsigned int8 Msg_Index = 0;
unsigned int8 Msg_ChkA  = 0;
unsigned int8 Msg_ChkB  = 0;

unsigned int8 GoInfo_Leng  = 0;
unsigned int8 GoInfo_Data[250]; 

int8 Size_type = 10;
char type[10] = {'t','e','x','t','/','p','l','a','i','n'}; 


/*int8 Size_type = 8;
char type[10] = {'t','e','x','t','/','t','x','t'}; */

//int8 Size_type = 10;
//char type[10] = {'i','m','a','g','e','/','j','p','e','g'}; 

void HOS_Init();
void HOS_Set_Time(void);
void HOS_Set_Buffer(unsigned int8 Dato);

void SentThirdPartyDataStatusData(int1 Prioritario, unsigned int16 Data_Id, int32 Data);
void SentFreeFormatThirdPartyData(unsigned int8 Size, unsigned int8 *Data);
void SentBinaryDataPacket(unsigned int8 Size, unsigned int8 *Data);
void SentMIMEPacket(unsigned int16 Size, unsigned int8 *Data);
void SentRequestDeviceDataMessage(void);
void SentDeviceDataACK(void);
void SentHandShakeRequest(void);
void SentHandshakeConfirmation(unsigned int16 DeviceId, int1 ACKConfirmation, int1 BinaryDataWrapping);
void CheckSum(unsigned int8 Data);
void GetParameters(HOS_DeviceData *Result);
