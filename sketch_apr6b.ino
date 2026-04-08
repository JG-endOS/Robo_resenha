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
String lastMovement = "";

// Configuração PWM para ESP32 core 3.x
#define PWM_FREQ 5000
#define PWM_RES  8

// Função auxiliar para escrever PWM diretamente no pino
void pwmWrite(int pin, int duty) {
  ledcWrite(pin, duty);
}

// Controla um motor individual
void setMotor(int in1, int in2, int dir) {
  if (dir == 1) {
    pwmWrite(in1, globalSpeed);
    pwmWrite(in2, 0);
  } else if (dir == -1) {
    pwmWrite(in1, 0);
    pwmWrite(in2, globalSpeed);
  } else {
    pwmWrite(in1, 0);
    pwmWrite(in2, 0);
  }
}

// Movimentos
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

void reapplyMovement() {
  if (lastMovement == "F") moverFrente();
  else if (lastMovement == "B") moverRe();
  else if (lastMovement == "L") virarEsquerda();
  else if (lastMovement == "R") virarDireita();
  else if (lastMovement == "P") pararTudo();
}

// Callbacks BLE
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("Dispositivo conectado!");
  }
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    pararTudo();
    lastMovement = "";
    Serial.println("Dispositivo desconectado!");
    pServer->startAdvertising();
  }
};

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    String cmd = pCharacteristic->getValue();
    cmd.trim();
    Serial.print("RECEBIDO: ");
    Serial.println(cmd.c_str());

    if      (cmd == "F" || cmd == "f") { moverFrente();   lastMovement = "F"; Serial.println("FRENTE"); }
    else if (cmd == "B" || cmd == "b") { moverRe();       lastMovement = "B"; Serial.println("RÉ"); }
    else if (cmd == "L" || cmd == "l") { virarEsquerda(); lastMovement = "L"; Serial.println("ESQUERDA"); }
    else if (cmd == "R" || cmd == "r") { virarDireita();  lastMovement = "R"; Serial.println("DIREITA"); }
    else if (cmd == "P" || cmd == "p") { pararTudo();     lastMovement = "P"; Serial.println("PARADO"); }
    else if (cmd.startsWith("S") || cmd.startsWith("s")) {
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
          return;
        }
      }
      Serial.println(globalSpeed);
      if (lastMovement != "" && lastMovement != "P") {
        reapplyMovement();
      }
    }
    else {
      Serial.println("Comando desconhecido");
    }
  }
};

void setup() {
  Serial.begin(115200);

  int pinos[] = {M1_IN1, M1_IN2, M2_IN1, M2_IN2, M3_IN1, M3_IN2, M4_IN1, M4_IN2};
  for (int i = 0; i < 8; i++) {
    pinMode(pinos[i], OUTPUT);
    digitalWrite(pinos[i], LOW);
    // API ESP32 core 3.x: ledcAttach(pin, freq, resolution)
    ledcAttach(pinos[i], PWM_FREQ, PWM_RES);
  }

  BLEDevice::init("Resenha_Móvel_BLE");
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

void loop() {
  delay(10);
}