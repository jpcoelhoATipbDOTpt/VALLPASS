// VER 1.1
// CPU: STM32
//
// RX ligado ao pino 11 (PB3 - JTD0 - SCK1 - T2C2)
// TX ligado ao pino 12 (PB4 - JTRST - MISO1 - T3C1)
//
// payload:
// {                               [Addr/FC]
//    OT:   over_temperature_flag  /  B [2000/02]
//    PVIV: PV_array_input_voltage /  V [3100/04]
//    PVIA: PV_array_input_current /  A [3101/04]
//    LDV:  load_voltage           /  V [310C/04]
//    LDA:  load_current           /  A [310D/04]
//    DT:   device_temperature     / ºC [3111/04]
//    BAT:  bat_remaining_capacity /  % [311A/04]
// }


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
  ETAPAX  // Etapa associada à gestão de erros
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
const byte rxPin = PB3;
const byte txPin = PB4;

//const byte rxPin = 2; // usado para testar o código no Arduino Uno
//const byte txPin = 3;

// ------------------------------------------------------------------------------------------------------------------
// VARIÁVEIS
// ------------------------------------------------------------------------------------------------------------------
// Envia sequencia:
// 01 0F 00 05 00 02 01 03 52 96 (Liga carga)
// 01 0F 00 05 00 02 01 00 12 97 (desliga carga)
// ---------------------------------------------------------------------Função D
// REAL-TIME CLOCK
//*******************************************************************************
// Timetag (read) [Funcionou]
//*******************************************************************************
//byte ReadBatteryVoltage[] = {
//  0x01,
//  0x03,
//  0x90, // ADD_INI_H
//  0x13, // ADD_INI_L
//  0x00, // ADD_NBR_H
//  0x03  // ADD_NBR_L
//};
// // Respondeu com: 0x01 0x03 0x06 0x0B 0x03 0x02 0x0D 0x0D 0x01 0x03 0x01 0x02 0x05
// //                               -----------------------------
// //                               12 3 2 13 13 1
// //
//*******************************************************************************
// Timetag: mês e ano (read) [Funcionou]
//*******************************************************************************
//byte ReadBatteryVoltage[] = {
//  0x01,
//  0x03,
//  0x90, // ADD_INI_H
//  0x15, // ADD_INI_L
//  0x00, // ADD_NBR_H
//  0x01  // ADD_NBR_L
//};
// Respondeu com: 0x01 0x03 0x02 0x0D 0x01 0x07 0x0D 0x01 0x04
//                               ---------
//                                Y    M
//                               13   Jan


// Depois de alterar com (*1)
// Respondeu com: 0x01 0x03 0x02 0x17 0x02 0x03 0x06 0x07 0x05
//                               ---------
//                                Y    M
//                               23   Fev
//*******************************************************************************
// Timetag: data e hora (write) [Funcionou]
//*******************************************************************************
//byte ReadBatteryVoltage[] = {
//  0x01,
//  0x10,
//  0x90, // ADD_INI_H
//  0x13, // ADD_INI_L
//  0x00, // ADD_NBR_H
//  0x03, // ADD_NBR_L
//  0x06, // Envia 6 bytes
//  0x00, // Minutos 0
//  0x00, // Segundos 0
//  0x0F, // Dia 15
//  0x0C, // Hora 12
//  0x17, // Ano ( 20) 23
//  0x02  // Mes (Fev) 02
//};
//  // Respondeu com: 0x01 0x10 0x90 0x13 0x00 0x03 0x05 0x0C 0x0C 0x0D


// ---------------------------------------------------------------------Função C
//
//*******************************************************************************
// Load control mode (read) {Funcionou]
//*******************************************************************************

//byte ReadBatteryVoltage[] = {
//  0x01,
//  0x03,
//  0x90, // ADD_INI_H
//  0x3D, // ADD_INI_L
//  0x00, // ADD_NBR_H
//  0x01  // ADD_NBR_L
//};
//   // Respondeu com 0x01 0x03 0x02 0x00 0x04 0x0B 0x09 0x08 0x07
//   //                         2Byt           -------------------
//   //                              ----------        CRC
//   //                                0004H Light ON+ Timer1
//   //
//
//                            0x01 0x03 0x02 0x00 0x00 B844
//*******************************************************************************
// Load control mode (write) [APARENTEMENTE FUNCIONA MAS NÃO RESPONDE]
//*******************************************************************************
//byte ReadBatteryVoltage[] = {
//  0x01,
//  0x10,
//  0x90, // ADD_INI_H
//  0x3D, // ADD_INI_L
//  0x00, // ADD_NBR_H
//  0x01, // ADD_NBR_L
//  0x02,
//  0x00,
//  0x00
//};




//byte ReadBatteryVoltage[] = {
//  0x01,
//  0x05,
//  0x00, // ADD_INI_H
//  0x02, // ADD_INI_L
//  0xFF, // ADD_NBR_H
//  0x00  // ADD_NBR_L
//}; // Manual control the load

//byte ReadBatteryVoltage[] = {
//  0x01,
//  0x05,
//  0x00, // ADD_INI_H
//  0x13, // ADD_INI_L
//  0xFF, // ADD_NBR_H
//  0x00  // ADD_NBR_L
//}; // Restore defaults

//byte ReadBatteryVoltage[] = {
//  0x01,
//  0x03,
//  0x90, // ADD_INI_H
//  0x3D, // ADD_INI_L
//  0x00, // ADD_NBR_H
//  0x01  // ADD_NBR_L
//}; // resposta a este comando: 0x01 0x03 0x02 0x00 0x04 (o que indica que Light ON+ Timer1)


//-----------------------------------------------------------------------------------------------
//                               COMANDOS PARA CONTROLO DA CARGA
//-----------------------------------------------------------------------------------------------
byte LoadControlMode[] = {
  0x01, // Device ID
  0x10, // Write
  0x90, // ADD_INI_H
  0x3D, // ADD_INI_L
  0x00, // ADD_NBR_H
  0x01, // ADD_NBR_L
  0x02, // Número de bytes
  0x00, // Manual Control
  0x00  //
};

byte ManualLoadControl[] = {
  0x01, // Device ID
  0x05, // Função (write)
  0x00, // ADD_INI_H
  0x02, // ADD_INI_L
  0xFF, // ADD_NBR_H ??
  0x00  // ADD_NBR_L ??
}; // Manual control the load (Comando C1, página 27)

byte ReadDefaultLoad[] = {
  0x01, // Device ID
  0x03, // Leitura
  0x90, // ADD_INI_H
  0x6A, // ADD_INI_L
  0x00,
  0x01
}; // Default load On/Off in manual mode (read)
// Resposta: 01 03 02 00 01 (7984)CRC

byte WriteDefaultLoadOff[] = {
  0x01, // Device ID
  0x10, // Escrita
  0x90, // ADD_INI_H
  0x6A, // ADD_INI_L
  0x00,
  0x01,
  0x02, // Número de bytes
  0x00,
  0x00
}; // Obriga a que, se estiver no modo manual, o estado inicial do LED é OFF
   // Aparentemente, não há qualquer resposta a este comando


byte WriteDefaultLoadOn[] = {
  0x01, // Device ID
  0x10, // Leitura
  0x90, // ADD_INI_H
  0x6A, // ADD_INI_L
  0x00,
  0x01,
  0x02, // Número de bytes
  0x00,
  0x01
};// Obriga a que, se estiver no modo manual, o estado inicial do LED é ON


//byte ???????[] = {
//  0x01, // Device ID
//  0x10,
//  0x90, // ADD_INI_H
//  0x3D, // ADD_INI_L
//  0x00, // ADD_NBR_H
//  0x01, // ADD_NBR_L
//  0x02,
//  0x00,
//  0x04
//}; // Default load On/Off in manual mode (write)

byte RestoreDefaults[] = {
  0x01,
  0x05,
  0x00, // ADD_INI_H
  0x13, // ADD_INI_L
  0xFF, // ADD_NBR_H
  0x00  // ADD_NBR_L
}; // Restore defaults

//-----------------------------------------------------------------------------------------------
//                               COMANDOS PARA LIGAR E DESLIGAR
//-----------------------------------------------------------------------------------------------
byte DesligaModoManual[] = {
  0x01, // Device ID
  0x05, // código da função
  0x00, // ADD_INI_H
  0x02, // ADD_INI_L
  0x00, // ADD_NBR_H
  0x00, // ADD_NBR_L
}; // manda desligar (modo manual)

byte LigaModoManual[] = {
  0x01, // Device ID
  0x05, // código da função
  0x00, // ADD_INI_H
  0x02, // ADD_INI_L
  0xFF, // ADD_NBR_H
  0x00, // ADD_NBR_L
}; // manda ligar (modo manual)

byte DesligaCarga[] = {
  0x01, // Device ID
  0x05, // código da função
  0x00, // ADD_INI_H
  0x06, // ADD_INI_L
  0x00, // ADD_NBR_H
  0x00, // ADD_NBR_L
}; // manda desligar a carga

byte LigaCarga[] = {
  0x01, // Device ID
  0x05, // código da função
  0x00, // ADD_INI_H
  0x13, // ADD_INI_L
  0xFF, // ADD_NBR_H
  0x00, // ADD_NBR_L
}; // manda desligar a carga

byte LigaViaComandoRemoto[] = {
  0x01, // Device ID
  0x0F,
  0x00,
  0x05,
  0x00,
  0x02,
  0x01,
  0x03
};  // Liga (via comando remoto)

byte DesligaViaComandoRemoto[] = {
  0x01, // Device ID
  0x0F,
  0x00,
  0x05,
  0x00,
  0x02,
  0x01,
  0x00
}; // Desliga (via comando remoto)
//-----------------------------------------------------------------------------------------------
//                               COMANDOS PARA TELEMETRIA
//-----------------------------------------------------------------------------------------------
byte ReadBatteryVoltage[] = {
  0x01, // Device ID
  0x04,
  0x33, // ADD_INI_H
  0x1A, // ADD_INI_L
  0x00, // ADD_NBR_H
  0x01  // ADD_NBR_L
}; // Solicita o valor da tensão da bateria

// REAL-TIME CLOCK
byte ReadTimeTag[] = {
  0x01,
  0x03,
  0x90, // ADD_INI_H
  0x13, // ADD_INI_L
  0x00, // ADD_NBR_H
  0x03  // ADD_NBR_L
};

//-----------------------------------------------------------------------------------------------


byte *Comando;

byte FrameToSend[20] = {};
byte recvData[20];

byte MenuOption[3] = {0, 0};

int rlen = 0;

byte nbrRecBytes = 0;  // Numero de bytes recebidos (depois do eco)
byte nbrSentBytes = 0; // Numero de bytes a enviar

//byte   recvData[]={0x01,0x04,0x02,0x04,0xCE,0x3A,0x64};

byte NbrTentativas = 0; // Conta o número de tentativas inválidas para associado a um determinado comando.

unsigned long timCleanSerial;
unsigned long sampleTimeCounter;
// ------------------------------------------------------------------------------------------------------------------
// OBJETOS
// ------------------------------------------------------------------------------------------------------------------
SoftwareSerial Tracer(rxPin, txPin);

void setup() {
  FrameToSend[19] = 0; // No último byte coloca-se o numero de bytes a transmitir. Inicialmente é zero.
  Tracer.begin(2400); // Instancia porta série (hardware)
  Tracer.setTimeout(50); // Timeout de 10 ms. Experimentalmente verifiquei que o intervalo de tempo
  // entre o fim do envio de uma frame e a receção da resposta anda entre 20 e 30 ms.
  // 50ms é para garantir que a frame de resposta se consegue captar.
  Serial.begin(115200);
  Serial.setTimeout(10);

}

void loop() {

  switch (estado) {
    case ETAPA0:
      Serial.println("***************** Opções *****************");
      Serial.println("1 - Liga LED (comando)");
      Serial.println("2 - Desliga LED (comando)");
      Serial.println("3 - Liga LED (manual)");
      Serial.println("4 - Desliga LED (manual)");
      Serial.println("5 - Liga LED (carga)");
      Serial.println("6 - Desliga LED (carga)");
      Serial.println("----------------------------------");
      Serial.println("7 - Manual Load Control");
      Serial.println("8 - Load Control Mode");
      Serial.println("----------------------------------");
      Serial.println("10 - Tensão da bateria");
      Serial.println("11 - Read Default On/Off in Manual Mode");
      Serial.println("12 - Default Load in Manual Mode = OFF");
      Serial.println("13 - Default Load in Manual Mode = ON");
      Serial.println("14 - Restore Defaults");
      Serial.println("15 - Read Time Tag");
      Serial.println("*******************************************");
      estado = ETAPA1;
      break;
    case ETAPA1:
      while (Tracer.available()) Tracer.read(); // Limpa buffer do porto série
      if (Serial.available() > 0) rlen = Serial.readBytesUntil('\n', MenuOption, 2);
      if (rlen > 0) {
        //------------------------------------------------------------------MENU----
        int opcao = atoi((const char *)MenuOption);
        switch (opcao) {
          case 1:
            Serial.println("(1)");
            Comando = LigaViaComandoRemoto;
            nbrSentBytes =  sizeof(LigaViaComandoRemoto) / sizeof(LigaViaComandoRemoto[0]);
            estado = ETAPA2;
            break;
          case 2:
            Serial.println("(2)");
            Comando = DesligaViaComandoRemoto;
            nbrSentBytes =  sizeof(DesligaViaComandoRemoto) / sizeof(DesligaViaComandoRemoto[0]);
            estado = ETAPA2;
            break;
          case 3:
            // Para que o modo manual (ligar e desligar) funcione é necessário ativar o LoadControlMode
            Serial.println("(3)");
            Comando = LigaModoManual;
            nbrSentBytes =  sizeof(LigaModoManual) / sizeof(LigaModoManual[0]);
            estado = ETAPA2;
            break;
          case 4:
            Serial.println("(4)");
            Comando = DesligaModoManual;
            nbrSentBytes =  sizeof(DesligaModoManual) / sizeof(DesligaModoManual[0]);
            estado = ETAPA2;
            break;
          case 5:
            Serial.println("(5)");
            Comando = LigaCarga;
            nbrSentBytes =  sizeof(LigaCarga) / sizeof(LigaCarga[0]);
            estado = ETAPA2;
            break;
          case 6:
            Serial.println("(6)");
            Comando = DesligaCarga;
            nbrSentBytes =  sizeof(DesligaCarga) / sizeof(DesligaCarga[0]);
            estado = ETAPA2;
            break;
          case 7:
            Serial.println("(7)");
            Comando = ManualLoadControl;
            nbrSentBytes =  sizeof(ManualLoadControl) / sizeof(ManualLoadControl[0]);
            estado = ETAPA2;
            break;
          case 8:
            Serial.println("(8)");
            Comando = LoadControlMode;
            nbrSentBytes =  sizeof(LoadControlMode) / sizeof(LoadControlMode[0]);
            estado = ETAPA2;
            break;
          case 10:
            Serial.println("(10)");
            Comando = ReadBatteryVoltage;
            nbrSentBytes =  sizeof(ReadBatteryVoltage) / sizeof(ReadBatteryVoltage[0]);
            estado = ETAPA2;
            break;
          case 11:
            Serial.println("(11)");
            Comando = ReadDefaultLoad;
            nbrSentBytes =  sizeof(ReadDefaultLoad) / sizeof(ReadDefaultLoad[0]);
            estado = ETAPA2;
            break;
          case 12:
            Serial.println("(12)");
            Comando = WriteDefaultLoadOff;
            nbrSentBytes =  sizeof(WriteDefaultLoadOff) / sizeof(WriteDefaultLoadOff[0]);
            NbrTentativas = NbrMaxTentativas ;
            estado = ETAPA2;
            break;
          case 13:
            Serial.println("(13)");
            Comando = WriteDefaultLoadOn;
            nbrSentBytes =  sizeof(WriteDefaultLoadOn) / sizeof(WriteDefaultLoadOn[0]);
            //NbrTentativas = NbrMaxTentativas ;            
            estado = ETAPA2;
            break;
          case 14:
            Serial.println("(14)");
            Comando = RestoreDefaults;
            nbrSentBytes =  sizeof(RestoreDefaults) / sizeof(RestoreDefaults[0]);
            estado = ETAPA2;
            break;
          case 15:
            Serial.println("(15)");
            Comando = ReadTimeTag;
            nbrSentBytes =  sizeof(ReadTimeTag) / sizeof(ReadTimeTag[0]);
            estado = ETAPA2;
            break;
          default:
            Serial.print("nenhum");
            break;
        }
        // --------------------------------------------------------------------------
        MenuOption[0] = 0; MenuOption[1] = 0;
        rlen = 0;
      }

      break;

    case ETAPA2: // Envia comando para o TRACER
      createModbusFrame(FrameToSend, Comando, nbrSentBytes); // Gera frame
      //for (int i = 0 ; i < FrameToSend[19]; i++) Serial.print(FrameToSend[i], HEX);
      Tracer.write(FrameToSend, FrameToSend[19]); // Envia frame
      Tracer.flush(); // Aguarda que todos os bytes sejam enviados...
      estado = ETAPA3;
      break;

    case ETAPA3: // Elimna ecos do envio do comando
      timCleanSerial = millis();
      while ((millis() - timCleanSerial) < 1) Tracer.read(); // Lê o que houver no porto série durante 1 ms (independentemente do que lá existir).
      // Admitindo que o Serial.read requer 5us para ser executado (80 ciclos de clock), em
      // 1 ms é capaz de ler 200 Bytes.
      estado = ETAPA4;
      break;

    case ETAPA4: // Aguarda pela receção da resposta
      nbrRecBytes = Tracer.readBytes(recvData, 20); // Aguarda 50 ms por 20 bytes de dados
      // Serial.println( nbrRecBytes);
      // Tempo terminou... x tem o número de bytes lidos
      if (!nbrRecBytes) { // O numero de bytes é zero... ou o Tracer não respondeu ou respondeu e
        NbrTentativas++; // a frame não pode ser lida... aguarda um tempo e volta a tentar
        delay(100); // caso ainda não tenham sido excedidas as tentativas para esta frame.
        if (NbrTentativas < NbrMaxTentativas) {
          estado = ETAPA2; // Volta a enviar o mesmo comando
        } else {
          estado = ETAPAX; // Vai para etapa de gestão de erro...
        }
      } else {
        estado = ETAPA5; // Recebeu uma resposta e agora é preciso processá-la
      }
      break;

    case ETAPA5: // Processa resposta
      // Neste momento, a variável x tem o número de bytes que foram lidos
      // Primeiro, confirmar se o CRC está correto
      if (CRC16_2(recvData, nbrRecBytes)) { // CRC incorreto... pede para repetir
        NbrTentativas++;
        delay(100);
        if (NbrTentativas < NbrMaxTentativas) {
          estado = ETAPA2; // Volta a enviar o mesmo comando
        } else {
          estado = ETAPAX; // Vai para etapa de gestão de erro...
        }
      } else {
        estado = ETAPA6; // CRC correto... organiza os dados para o envio
      }
      break;

    case ETAPA6: // Organiza Payload
      NbrTentativas = 0;
      estado = ETAPA7;
      break;

    case ETAPA7: // Mostra Payload
      sampleTimeCounter = millis();
      for (int i = 0; i < nbrRecBytes; i++) Serial.print(recvData[i], HEX);
      Serial.println("");
      Serial.flush();
      estado = ETAPA0;
      break;

    case ETAPAX: // Erro
      NbrTentativas = 0;
      Serial.println("Erro");
      sampleTimeCounter = millis();
      estado = ETAPA0;
      break;

    default: // agarra-me se cair ;)
      Serial.println("xipo");
      estado = ETAPA0;
      break;
  }
}
