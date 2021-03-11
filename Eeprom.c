
unsigned int1 EepromInit()
{
   ValoresIniciales.ACTstatus     = false;
   ValoresIniciales.ACTmode       = ActuadorTemporizado;
   ValoresIniciales.ACTtoReset    = 5000;
   ValoresIniciales.ACTinReset    = 15000;
   ValoresIniciales.ACTtimeOutPIN = 60000;
   ValoresIniciales.ACTenableClientRequest = true;
   
   ValoresIniciales.DBItimeToHeartBeat  = 900000;
   ValoresIniciales.DBIheartBeatEnabled = true;
   
   ValoresIniciales.HOStimeToResquest  = 2000;
   ValoresIniciales.HOSrqstEnabled     = true;
   ValoresIniciales.HOSautoShowEnabled = false;
   
   ValoresIniciales.CSackByGo = 5000;
   ValoresIniciales.CSmax     = 200;
   ValoresIniciales.CScustomEnabled = true;
   ValoresIniciales.CSstatusEnabled = true;
   
   ValoresIniciales.MIMEackByGo     = 5000;
   ValoresIniciales.MIMEackByServer = 60000;
   ValoresIniciales.MIMEmax         = 2000;
   ValoresIniciales.MIMEoutEnabled  = true;
   ValoresIniciales.MIMEinEnabled   = true;
   ValoresIniciales.MIMEWaitACKtoSend   = true;
   ValoresIniciales.MIMEautoResponseACK = false;
   
   ValoresIniciales.InputEnableShow        = true;
   ValoresIniciales.InputEnablestatus      = true;
   ValoresIniciales.InputEnableSaveEeprom  = true;
   ValoresIniciales.InputTimeToSense       = 250;
   ValoresIniciales.InputStatusDataId      = 30091;
   ValoresIniciales.InputValueToSave       = false;
   ValoresIniciales.empty = 0;
   
   EepromInputValueToSave = ValoresIniciales.InputValueToSave;
   
   unsigned int1 BlockResult = false;
   do
   {
      BloqueEeprom++;
      if(EeepromleerBloque(BloqueEeprom,&ValoresIniciales))
      {
         BlockResult = true;
         //fprintf(Serial,"Valid Value in: %u%c%c",BloqueEeprom,0x1f,0x03);
      }
      else
      {
         //fprintf(Serial,"Invalid Value in: %u%c%c",BloqueEeprom,0x1f,0x03);
      }
      
   }while((BlockResult == false) && (BloqueEeprom < MaxBlocks));
   
   if(BloqueEeprom > 1)
   {  
      for(unsigned int8 i = 1; i < (BloqueEeprom + 3); i++)
      {
         if(i <= MaxBlocks)
         {
            EepromEscribirBloque(i, ValoresIniciales);
            //fprintf(Serial,"guardado en %u%c%c",i,0x1f,0x03);
         }
      }      
   }
   
   if(BlockResult == false)
   {
      BloqueEeprom = 255;
   }
   EepromInputValueToSave = ValoresIniciales.InputValueToSave;
   return BlockResult;
}

void EepromSave(Variables Structura)
{
   if(BloqueEeprom == 255)
   {
      BloqueEeprom = 250;
   }
   for(unsigned int8 i = 1; i < (BloqueEeprom + 3); i++)
   {
      if(i <= MaxBlocks)
      {
         EepromEscribirBloque(i, Structura);
         //fprintf(Serial,"guardado en %u%c%c",i,0x1f,0x03);
      }
   }
   if(BloqueEeprom == 250)
   {
      BloqueEeprom = 255;
   }
}

unsigned int1 CompareStructs(Variables StructA, Variables StructB)
{
   unsigned int1 result = false;
   unsigned int8 *punteroA = &StructA;
   unsigned int8 *punteroB = &StructB;
   //unsigned int8 sizee = sizeof(Variables);
   //fprintf(Serial,"Valores %u:  ",sizee);
   for(unsigned int8 i = 0; i < sizeof(Variables); i++)
   {
       unsigned int8 nByteA = *punteroA++;
       unsigned int8 nByteB = *punteroB++;
       //fprintf(Serial,"%X,%X ",nByteA,nByteB);
       if(nByteA != nByteB)
       {
          result = true;
       }
       restart_wdt();
   }
   //fprintf(Serial,"%c%c",0x1f,0x03);    
   return result;
}

unsigned int1 EeepromleerBloque(unsigned int8 bloque, Variables *Structura)
{
   Variables ValuesSaved;
   unsigned int16 BlockSize = sizeof(Variables);
   unsigned int8 *Puntero = &ValuesSaved;
   unsigned int1 result = false;
   unsigned int8 cA = 0, cB=0;
   unsigned int16 startAddress = (bloque - 1) * BlockSize;
   if(bloque > 1)
   {
      startAddress = ((bloque - 1) * BlockSize) + ((bloque - 1) * 2);
   }
   
   //fprintf(Serial,"Bloque %u ",bloque);
   for(unsigned int8 i = 0; i < BlockSize; i++)
   {
      unsigned int8 nByte = LeerEeprom(startAddress + i);
      *Puntero = nByte;
      Puntero++;
      cA = cA + nByte;
      cB = cA + cB;
      restart_wdt();
      //fprintf(Serial,"%X ",nByte);
   }
   unsigned int8 cAreal = LeerEeprom(startAddress + BlockSize);
   unsigned int8 cBreal = LeerEeprom(startAddress + BlockSize + 1);
   //fprintf(Serial," -%x%x- -%x%X-%c%c",cAreal,cBreal,cA,cB,0x1f,0x03);
   if(cA == cAreal && cB == cBreal)
   {
      //fprintf(Serial,"Checksum OK%c%c",0x1f,0x03);
      memcpy(Structura, &ValuesSaved, sizeof(Variables));
      result = true;
   }
   return result;
}

void EepromEscribirBloque(unsigned int8 bloque, Variables Structura)
{
   unsigned int16 BlockSize = sizeof(Variables);
   unsigned int8 cA = 0, cB=0;
   unsigned int16 startAddress = 0;
   if(bloque > 1)
   {
      startAddress = ((bloque - 1) * BlockSize) + ((bloque - 1) * 2);
   }
   
   //fprintf(Serial,"Escribo en Bloque %u, direccion %lu ",bloque,startAddress);
   unsigned int8 *puntero = &Structura;
   for(unsigned int8 i = 0; i < BlockSize; i++)
   {
       unsigned int8 nByte = *puntero++;
       EcribirEeprom((startAddress + i),nByte);
       cA = cA + nByte;
       cB = cA + cB;
       restart_wdt();
   }
   unsigned int16 addChk = startAddress + BlockSize;
   EcribirEeprom(addChk,cA);
   addChk++;
   EcribirEeprom(addChk,cB);
   //fprintf(Serial,"Termino en %lu %c%c",addChk,0x1f,0x03);
   return;
}

void EcribirEeprom(unsigned int16 add, unsigned int8 value)
{
   //write_eeprom (add, value); 
   
   unsigned int8 aH = (add >> 8) & 0xFF;
   unsigned int8 aL = (add >> 0) & 0xFF;
   #asm 
      MOVF aH,W ; 
      MOVWF EEADRH ; //Upper bits of Data Memory Address to write
      MOVF aL,W ; 
      MOVWF EEADR ; //Lower bits of Data Memory Address to write
      MOVF value,W ; 
      MOVWF EEDATA ; //Data Memory Value to write
      BCF EECON1, EEPGD; //Point to DATA memory
      BCF EECON1, CFGS ; //Access EEPROM
      BSF EECON1, WREN ; //Enable writes
        
      BCF INTCON, GIE ; //Disable Interrupts
      MOVLW 0x55 ;
      MOVWF EECON2 ; //Write 55h
      MOVLW 0xAA ;
      MOVWF EECON2 ; //Write 0AAh
      BSF EECON1, WR ; //Set WR bit to begin write
      loop:
      BTFSC EECON1, WR ; //Wait for write to complete 
      goto loop;
      BSF INTCON, GIE ; //Enable Interrupts
      BCF EECON1, WREN ; //Disable writes on write complete (EEIF set)
   #endasm
   restart_wdt();
   return;
}

unsigned int8 LeerEeprom(unsigned int16 add)
{
   unsigned int8 result; //= read_EEPROM(add);
   
   unsigned int8 aH = (add >> 8) & 0xFF;
   unsigned int8 aL = (add >> 0) & 0xFF;
   #asm 
      MOVF aH,W ; 
      MOVWF EEADRH ; //Upper bits of Data Memory Address to write
      MOVF aL,W ; 
      MOVWF EEADR ; //Lower bits of Data Memory Address to write
      BCF EECON1, EEPGD ; Point to DATA memory
      BCF EECON1, CFGS ; Access EEPROM
      BSF EECON1, RD ; EEPROM Read
      NOP
      MOVFF EEDATA, result;
   #endasm
   restart_wdt();
   return result;
}
