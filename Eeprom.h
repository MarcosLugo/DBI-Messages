/////Eeprom registers defin
#byte EEADRH = 0xF75
#byte EEADR  = 0xF74
#byte EEDATA = 0xF73
#byte EECON1 = 0xF7F 
#byte INTCON = 0xFF2
#byte EECON2 = 0xF7E
#define   EEPGD 7
#define  CFGS  6
#define  WREN  2
#define  WR    1
#define  GIE   7
#define  RD    0
///////////////////////////

#define MaxAddres  1023
#define MaxBlocks  24
//#define BlockSize  36

typedef struct
{
   unsigned int16 MIMEackByGo;
   unsigned int32 MIMEackByServer;
   unsigned int16 MIMEmax;
   unsigned int1  MIMEoutEnabled;
   unsigned int1  MIMEinEnabled;
   unsigned int1  MIMEWaitACKtoSend;
   unsigned int1  MIMEautoResponseACK;

   unsigned int1  CScustomEnabled;
   unsigned int1  CSstatusEnabled;
   
   unsigned int1  HOSautoShowEnabled;
   unsigned int1  HOSrqstEnabled;

   unsigned int16 CSackByGo;
   unsigned int16 CSmax;
   
   unsigned int32 HOStimeToResquest;
   
   unsigned int8  ACTmode;
   unsigned int32 ACTtoReset;
   unsigned int32 ACTinReset;
   unsigned int32 ACTtimeOutPIN;
   unsigned int1  ACTstatus;
   unsigned int1  ACTenableClientRequest;
  
   unsigned int1  DBIheartBeatEnabled;
   
   unsigned int1  InputEnableShow;
   unsigned int1  InputEnablestatus;
   unsigned int1  InputEnableSaveEeprom;
   unsigned int1  InputValueToSave;
   unsigned int1  empty;
   unsigned int32 DBItimeToHeartBeat;
   
   unsigned int16 InputStatusDataId;
   unsigned int16 InputTimeToSense;
   
}Variables;

Variables ValoresIniciales;
unsigned int1 EepromInputValueToSave;
unsigned int8 BloqueEeprom = 0;

unsigned int1 EepromInit(void);
void          EepromSave(Variables Structura);
unsigned int1 CompareStructs(Variables StructA, Variables StructB);
unsigned int1 EeepromleerBloque(unsigned int8 bloque, Variables *Structura);
void          EepromEscribirBloque(unsigned int8 bloque, Variables Structura);
void          EcribirEeprom(unsigned int16 add, unsigned int8 value);
unsigned int8 LeerEeprom(unsigned int16 add);
