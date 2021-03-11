unsigned int1 InputEventDetected;
unsigned int1 InputStatus;
struct
{
   unsigned int1  ActualSatus;
   unsigned int1  LastStatus;
   unsigned int16 TimeToSense;
   unsigned int16 TimeNow;
   unsigned int16 StatusDataID;
   
}EntradaDigital;

void InputInit(void);
void InputSetTime(void);

void InputInit()
{
   EntradaDigital.ActualSatus = true;
   EntradaDigital.LastStatus  = true;
   EntradaDigital.TimeToSense = ValoresIniciales.InputTimeToSense;
   EntradaDigital.TimeNow     = 0;
   EntradaDigital.StatusDataID = ValoresIniciales.InputStatusDataId;
   
   InputEventDetected = false;
   InputStatus = true;
}

void InputSetTime()
{
   EntradaDigital.ActualSatus = input(PIN_B0);
   if(EntradaDigital.ActualSatus != EntradaDigital.LastStatus)
   {
      EntradaDigital.TimeNow += interruptTime;
   }
   else
   {
      EntradaDigital.TimeNow = 0;
   }
   
   if(EntradaDigital.TimeNow >= EntradaDigital.TimeToSense)
   {
      EntradaDigital.LastStatus = EntradaDigital.ActualSatus;
      InputStatus = EntradaDigital.ActualSatus;
      InputEventDetected = true;
   }
}


