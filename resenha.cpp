#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

BLEServer* pServer = NULL;
BLECharacteristic* pTxCharacteristic;
bool deviceConnected = false;

// ============================================
// Configuração das pontes H
// ============================================
#define M1_IN1  25
#define M1_IN2  27
#define M3_IN1  12
#define M3_IN2  13

#define M2_IN1  26
#define M2_IN2  14
#define M4_IN1  32
#define M4_IN2  33

// Velocidade global (0-255)
int globalSpeed = 200;

// Último movimento ativo ('\0' = nenhum)
char lastMovement = '\0';

// Configuração PWM para ESP32 core 3.x
#define PWM_FREQ 5000
#define PWM_RES  8

// ============================================
// Controle individual de motor via PWM
// dir:  1 = frente | -1 = ré | 0 = parado
// ============================================
void setMotor(int in1, int in2, int dir) {
  if (dir == 1) {
    ledcWrite(in1, globalSpeed);
    ledcWrite(in2, 0);
  } else if (dir == -1) {
    ledcWrite(in1, 0);
    ledcWrite(in2, globalSpeed);
  } else {
    ledcWrite(in1, 0);
    ledcWrite(in2, 0);
  }
}

// ============================================
// Movimentos
// ============================================
void moverFrente() {
  setMotor(M1_IN1, M1_IN2,  1);
  setMotor(M2_IN1, M2_IN2,  1);
  setMotor(M3_IN1, M3_IN2,  1);
  setMotor(M4_IN1, M4_IN2,  1);
}

void moverRe() {
  setMotor(M1_IN1, M1_IN2, -1);
  setMotor(M2_IN1, M2_IN2, -1);
  setMotor(M3_IN1, M3_IN2, -1);
  setMotor(M4_IN1, M4_IN2, -1);
}

void virarEsquerda() {
  setMotor(M1_IN1, M1_IN2, -1);
  setMotor(M2_IN1, M2_IN2,  1);
  setMotor(M3_IN1, M3_IN2, -1);
  setMotor(M4_IN1, M4_IN2,  1);
}

void virarDireita() {
  setMotor(M1_IN1, M1_IN2,  1);
  setMotor(M2_IN1, M2_IN2, -1);
  setMotor(M3_IN1, M3_IN2,  1);
  setMotor(M4_IN1, M4_IN2, -1);
}

void pararTudo() {
  setMotor(M1_IN1, M1_IN2, 0);
  setMotor(M2_IN1, M2_IN2, 0);
  setMotor(M3_IN1, M3_IN2, 0);
  setMotor(M4_IN1, M4_IN2, 0);
}

// Reaplicar movimento atual após mudança de velocidade
void reapplyMovement() {
  switch (lastMovement) {
    case 'F': moverFrente();   break;
    case 'B': moverRe();       break;
    case 'L': virarEsquerda(); break;
    case 'R': virarDireita();  break;
    default:  break;
  }
}

// ============================================
// Enviar feedback via BLE TX
// ============================================
void sendFeedback(const String& msg) {
  if (deviceConnected) {
    pTxCharacteristic->setValue(msg.c_str());
    pTxCharacteristic->notify();
    Serial.print("TX → ");
    Serial.println(msg);
  }
}

// ============================================
// Callbacks BLE
// ============================================
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("Dispositivo conectado!");
    sendFeedback("OK:CONNECTED");
  }
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    pararTudo();
    lastMovement = '\0';
    Serial.println("Dispositivo desconectado!");
    pServer->startAdvertising();
  }
};

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    String cmd = String(pCharacteristic->getValue().c_str());
    cmd.trim();
    Serial.print("RECEBIDO: ");
    Serial.println(cmd);

    if (cmd.length() == 0) return;

    char c = toupper(cmd.charAt(0));

    if (c == 'F' && cmd.length() == 1) {
      moverFrente();
      lastMovement = 'F';
      Serial.println("FRENTE");
      sendFeedback("OK:F");

    } else if (c == 'B' && cmd.length() == 1) {
      moverRe();
      lastMovement = 'B';
      Serial.println("RÉ");
      sendFeedback("OK:B");

    } else if (c == 'L' && cmd.length() == 1) {
      virarEsquerda();
      lastMovement = 'L';
      Serial.println("ESQUERDA");
      sendFeedback("OK:L");

    } else if (c == 'R' && cmd.length() == 1) {
      virarDireita();
      lastMovement = 'R';
      Serial.println("DIREITA");
      sendFeedback("OK:R");

    } else if (c == 'P' && cmd.length() == 1) {
      pararTudo();
      lastMovement = 'P';
      Serial.println("PARADO");
      sendFeedback("OK:P");

    } else if (c == 'S') {
      String param = cmd.substring(1);

      if (param == "+") {
        globalSpeed = min(255, globalSpeed + 10);
        Serial.print("Velocidade +10 → ");
      } else if (param == "-") {
        globalSpeed = max(0, globalSpeed - 10);
        Serial.print("Velocidade -10 → ");
      } else {
        int newSpeed = param.toInt();
        if (newSpeed >= 0 && newSpeed <= 255) {
          globalSpeed = newSpeed;
          Serial.print("Velocidade ajustada para → ");
        } else {
          Serial.println("Velocidade inválida (use 0-255)");
          sendFeedback("ERR:SPEED_RANGE");
          return;
        }
      }

      Serial.println(globalSpeed);
      sendFeedback("OK:S" + String(globalSpeed));

      // Reaplicar somente se estiver em movimento
      if (lastMovement != '\0' && lastMovement != 'P') {
        reapplyMovement();
      }

    } else {
      Serial.println("Comando desconhecido");
      sendFeedback("ERR:UNKNOWN_CMD");
    }
  }
};

// ============================================
// Setup
// ============================================
void setup() {
  Serial.begin(115200);

  int pinos[] = {M1_IN1, M1_IN2, M2_IN1, M2_IN2, M3_IN1, M3_IN2, M4_IN1, M4_IN2};
  for (int i = 0; i < 8; i++) {
    ledcAttach(pinos[i], PWM_FREQ, PWM_RES);
    ledcWrite(pinos[i], 0);
  }

  BLEDevice::init("Resenha_Movel_BLE");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService* pService = pServer->createService(SERVICE_UUID);

  pTxCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_TX,
    BLECharacteristic::PROPERTY_NOTIFY
  );
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic* pRxCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_RX,
    BLECharacteristic::PROPERTY_WRITE
  );
  pRxCharacteristic->setCallbacks(new MyCallbacks());

  pService->start();
  pServer->startAdvertising();

  Serial.println("BLE iniciado! Aguardando conexão...");
  Serial.println("Comandos: F, B, L, R, P | Velocidade: S150, S+, S-");
}

// ============================================
// Loop
// ============================================
void loop() {
  delay(10);
}

/*

Primeiro L298N:
[L298N] [GPIO ESP32] [Motor]
  IN1       25         M1
  IN2       27         M1
  IN3       26         M2
  IN4       14         M2
  OUT1/OUT2  -       Motor 1
  OUT3/OUT4  -       Motor 2

Segundo L298N:
[L298N]  [GPIO ESP32] [Motor]
  IN1       12          M3
  IN2       13          M3
  IN3       32          M4
  IN4       33          M4
  OUT1/OUT2  -        Motor 3
  OUT3/OUT4  -        Motor 4

*/