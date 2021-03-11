#define Relay PIN_C4

#define ActuadorONOFF 3
#define ActuadorTemporizado 5

#define ON 3
#define OFF 4
#define TimedOutput 5
unsigned int8 A_LocalCMD = 0;
unsigned int1 valueToPutInOutput = false;
struct  
{
   unsigned int1  ActivateTimedOutput;
   unsigned int32 TimeInReset;
   unsigned int32 TimeToReset; //tiempo programado para resetear despues de mandar accion
   unsigned int32 TimeNow; //tiempo contado para hacer reset
   unsigned int1  ActivateOutput;
   unsigned int1  CancelPetition;
   unsigned int8  OutputMode; //3 = actuador 5 = Actuador temporizado
   unsigned int1  OutputState;
   unsigned int1  SaveValue;
   
   unsigned int8  LastPetition; //3 = on, 4 = off, 5 = timedOutput
   unsigned int16 Pass;
   unsigned int32 P_TimeOutPass;
   unsigned int32 N_TimeOutPass;
   unsigned int1  WaittingPass;
}Actuator;

void Actuator_Init();
void Actuator_SetTime(void);
void Actuator_Set_Value(unsigned int1 status);

void Actuator_Init()
{
   Actuator.ActivateTimedOutput = false;
   
   Actuator.TimeNow = 0;
   Actuator.N_TimeOutPass = 0;
   Actuator.ActivateOutput = false;
   Actuator.CancelPetition = false;
   Actuator.LastPetition = 0;
   Actuator.SaveValue = false; 
   Actuator.Pass = 10000;
   Actuator.WaittingPass = false;
   
   Actuator.OutputMode    = ValoresIniciales.ACTmode;
   Actuator.OutputState   = ValoresIniciales.ACTstatus;   
   Actuator.TimeInReset   = ValoresIniciales.ACTinReset;
   Actuator.TimeToReset   = ValoresIniciales.ACTtoReset;
   Actuator.P_TimeOutPass = ValoresIniciales.ACTtimeOutPIN;
   
   if(Actuator.OutputMode == ActuadorONOFF)
   {
      output_bit(Relay,Actuator.OutputState);
   }
   else if(Actuator.OutputMode == ActuadorTemporizado)
   {
      Actuator.OutputState = 0;
      output_bit(Relay,Actuator.OutputState);
   } 
}

void Actuator_SetTime()
{
   if(Actuator.WaittingPass == true)
   {
      Actuator.N_TimeOutPass +=interruptTime;
      if(Actuator.N_TimeOutPass >= Actuator.P_TimeOutPass) 
      {
         Actuator.N_TimeOutPass = 0;
         Actuator.WaittingPass = false;
      }
   }
   else
   {
      Actuator.N_TimeOutPass = 0;
   }
   if(Actuator.OutputMode == ActuadorTemporizado)
   {
      if(Actuator.ActivateTimedOutput == true)
      {
         Actuator.TimeNow +=interruptTime;
         
         switch(A_LocalCMD)
         {
            case 0:
            {
               if(Actuator.TimeNow > Actuator.TimeToReset)
               {
                  //se cumplio primer etapa, se apaga equipo
                  Actuator.OutputState = true;
                  Actuator.TimeNow = 0;
                  A_LocalCMD = 1;
               }
               break;
            }
            case 1:
            {
               if(Actuator.TimeNow > Actuator.TimeInReset)
               {
                  Actuator.ActivateTimedOutput = false;
                  A_LocalCMD = 0;
                  Actuator.TimeNow = 0;
               }
               break;
            }
         }
      }
      else
      {
         Actuator.OutputState = false;
         A_LocalCMD = 0;
         Actuator.TimeNow = 0;
      }
      output_bit(Relay,Actuator.OutputState);
   }
   else if(Actuator.OutputMode == ActuadorONOFF)
   {
      if(Actuator.ActivateOutput == true)
      {
         Actuator.TimeNow +=interruptTime;
         if(Actuator.TimeNow > 3000)
         {
            Actuator.OutputState = valueToPutInOutput;
            Actuator.TimeNow = 0;
            output_bit(Relay,Actuator.OutputState);
            Actuator.ActivateOutput = false;
            Actuator.SaveValue = true;            
         }
      }
   }  
}

void Actuator_Set_Value(unsigned int1 status)
{
   Actuator.TimeNow = 0;
   Actuator.ActivateOutput = true;
   valueToPutInOutput = status;
   return;
}

