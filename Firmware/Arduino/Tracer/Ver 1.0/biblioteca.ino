// ------------------------------------------------------------------------------------------------------------------
// FUNÇÕES
// ------------------------------------------------------------------------------------------------------------------

void createModbusFrame(byte *FrameToSend, byte *FrameData, byte FrameSize){
  // Calcula CRC
  unsigned int modbus_crc = CRC16_2(FrameData,(int)FrameSize);
  // Preenche Frame com o cabeçalho e dados
  for(int i=0;i<FrameSize;i++) FrameToSend[i]=FrameData[i];
  // Adiciona o CRC 
  FrameToSend[FrameSize]=(byte) modbus_crc;
  FrameToSend[FrameSize+1]=(byte) (modbus_crc>>8);
  FrameToSend[19]=FrameSize+2;
}


unsigned int CRC16_2(unsigned char *buf, int len)
{  
  unsigned int crc = 0xFFFF;
  for (int pos = 0; pos < len; pos++)
  {
  crc ^= (unsigned int)buf[pos];    // XOR byte into least sig. byte of crc

  for (int i = 8; i != 0; i--) {    // Loop over each bit
    if ((crc & 0x0001) != 0) {      // If the LSB is set
      crc >>= 1;                    // Shift right and XOR 0xA001
      crc ^= 0xA001;
    }
    else                            // Else LSB is not set
      crc >>= 1;                    // Just shift right
    }
  }
  return crc;
}

void pisca(){
        for(;;){
        digitalWrite(LED_BUILTIN,HIGH);
        delay(500);
        digitalWrite(LED_BUILTIN,LOW);
        delay(500);
      }
}
