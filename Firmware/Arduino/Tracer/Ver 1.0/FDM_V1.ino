#include <SoftwareSerial.h>

#include "biblioteca.h"
 // ------------------------------------------------------------------------------------------------------------------
// MÁQUINA DE ESTADOS PRINCIPAL
// ------------------------------------------------------------------------------------------------------------------
enum estadosistema {
  ETAPA0, // Etapa inicial associada à inicialização do sistema
  ETAPA1, // Etapa associada ao envio de um comando 
  ETAPA2, // Etapa associada à eliminação de eventuais ecos
  ETAPA3, // Etapa associada à receção da resposta
  ETAPA4, // Etapa associada ao processamento da resposta
  ETAPA5, // Etapa associada à formatação do payload json
  ETAPA6, // Etapa associada ao envio do payload via LoRa
  ETAPA7, // Etapa associada ao delay para cumprir período de amostragem
  ETAPAX // Etapa associada à gestão de erros
}
estado = ETAPA0;

// ------------------------------------------------------------------------------------------------------------------
// SÍMBOLOS
// ------------------------------------------------------------------------------------------------------------------
#define NbrMaxTentativas 5
#define SampleTime 12000 // Período de amostragem em milissegundos.
// ------------------------------------------------------------------------------------------------------------------
// CONSTANTES
// ------------------------------------------------------------------------------------------------------------------
const byte rxPin = 2;
const byte txPin = 3;

// ------------------------------------------------------------------------------------------------------------------
// VARIÁVEIS
// ------------------------------------------------------------------------------------------------------------------
// Envia sequencia:
// 01 0F 00 05 00 02 01 03 52 96 (Liga carga)
// 01 0F 00 05 00 02 01 00 12 97 (desliga carga)
byte ReadBatteryVoltage[] = {
  0x01,
  0x04,
  0x33,
  0x1A,
  0x00,
  0x01
};
byte liga[] = {
  0x01,
  0x0F,
  0x00,
  0x05,
  0x00,
  0x02,
  0x01,
  0x03
};
byte desliga[] = {
  0x01,
  0x0F,
  0x00,
  0x05,
  0x00,
  0x02,
  0x01,
  0x00
};
byte FrameToSend[20] = {};
byte recvData[20];

byte nbrRecBytes = 0; // Numero de bytes recebidos (depois do eco)

//byte   recvData[]={0x01,0x04,0x02,0x04,0xCE,0x3A,0x64};

byte NbrTentativas = 0; // Conta o número de tentativas inválidas para associado a um determinado comando.

unsigned long timCleanSerial;
unsigned long sampleTimeCounter;
// ------------------------------------------------------------------------------------------------------------------
// OBJETOS
// ------------------------------------------------------------------------------------------------------------------
SoftwareSerial mySerial(rxPin, txPin);

void setup() {
  FrameToSend[19] = 0; // No último byte coloca-se o numero de bytes a transmitir. Inicialmente é zero.
  Serial.begin(2400); // Instancia porta série (hardware)  
  Serial.setTimeout(50); // Timeout de 10 ms. Experimentalmente verifiquei que o intervalo de tempo  
  // entre o fim do envio de uma frame e a receção da resposta anda entre 20 e 30 ms.
  // 50ms é para garantir que a frame de resposta se consegue captar.
  mySerial.begin(115200); // Instancia porta série (software)

}

void loop() {

  switch (estado) {
    case ETAPA0: // Etapa inicial onde se podem incluir eventuais operações de housekeeping
      while (Serial.available()) Serial.read(); // Limpa buffer do porto série
      estado = ETAPA1;
      break;
    
    case ETAPA1: // Envia comando para o TRACER
      createModbusFrame(FrameToSend, ReadBatteryVoltage, 6); // Gera frame
      Serial.write(FrameToSend, FrameToSend[19]); // Envia frame
      Serial.flush(); // Aguarda que todos os bytes sejam enviados...               
      estado = ETAPA2;
      break;
    
    case ETAPA2: // Elimna ecos do envio do comando
      timCleanSerial = millis();
      while ((millis() - timCleanSerial) < 1) Serial.read(); // Lê o que houver no porto série durante 1 ms (independentemente do que lá existir). 
      // Admitindo que o Serial.read requer 5us para ser executado (80 ciclos de clock), em
      // 1 ms é capaz de ler 200 Bytes.                
      estado = ETAPA3;
      break;
    
    case ETAPA3: // Aguarda pela receção da resposta
      nbrRecBytes = Serial.readBytes(recvData, 20); // Aguarda 50 ms por 20 bytes de dados
      // Tempo terminou... x tem o número de bytes lidos
      if (!nbrRecBytes) { // O numero de bytes é zero... ou o Tracer não respondeu ou respondeu e
        NbrTentativas++; // a frame não pode ser lida... aguarda um tempo e volta a tentar
        delay(100); // caso ainda não tenham sido excedidas as tentativas para esta frame.                 
        if (NbrTentativas < NbrMaxTentativas) {
          estado = ETAPA1; // Volta a enviar o mesmo comando
        } else {
          estado = ETAPAX; // Vai para etapa de gestão de erro...
        }
      } else {
        estado = ETAPA4; // Recebeu uma resposta e agora é preciso processá-la
      }
      break;
    
    case ETAPA4: // Processa resposta
      // Neste momento, a variável x tem o número de bytes que foram lidos
      // Primeiro, confirmar se o CRC está correto
      if (CRC16_2(recvData, nbrRecBytes)) { // CRC incorreto... pede para repetir
        NbrTentativas++;
        delay(100);
        if (NbrTentativas < NbrMaxTentativas) {
          estado = ETAPA1; // Volta a enviar o mesmo comando
        } else {
          estado = ETAPAX; // Vai para etapa de gestão de erro...
        }
      } else {
        estado = ETAPA5; // CRC correto... organiza os dados para o envio
      }
      break;
    
    case ETAPA5: // Organiza Payload
      NbrTentativas = 0;
      estado = ETAPA6;
      break;
    
    case ETAPA6: // Transmite Payload
      sampleTimeCounter = millis();
      for (int i = 0; i < nbrRecBytes; i++) mySerial.print(recvData[i],HEX);
      mySerial.println("");
      mySerial.flush();
      estado = ETAPA7;
      break;
    
    case ETAPA7: // Aguarda para cumprir período de amostragem
      if ((millis() - sampleTimeCounter) > SampleTime) estado = ETAPA0; // Delay de 2 minutos
      break;
    
    case ETAPAX: // Erro
      NbrTentativas = 0;
      mySerial.println("Erro");
      sampleTimeCounter = millis();
      estado = ETAPA7;
      break;
  
    default: // agarra-me se cair ;)
      estado = ETAPA0;
      break;
    }
}
