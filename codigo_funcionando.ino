//CÓDIGO FUNCIONANDO TODOS OS COMANDOS VIA THUNKABLE
//APENAS COPIE E COLE NO COMPUTADOR 

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

#define M1_IN1  25
#define M1_IN2  27

#define M2_IN1  26
#define M2_IN2  14

char lastMovement = '\0';


// MOVIMENTOS
void moverFrente() {
  digitalWrite(M2_IN2, LOW);
  digitalWrite(M2_IN1, HIGH);
  digitalWrite(M1_IN1, LOW);
  digitalWrite(M1_IN2, HIGH);
}

void moverRe() {
  digitalWrite(M2_IN2, HIGH);
  digitalWrite(M2_IN1, LOW);
  digitalWrite(M1_IN1, HIGH);
  digitalWrite(M1_IN2, LOW);
}

void virarEsquerda() {
  digitalWrite(M2_IN2, HIGH);
  digitalWrite(M2_IN1, LOW);
  digitalWrite(M1_IN1, LOW);
  digitalWrite(M1_IN2, HIGH);
}

void virarDireita() {
  digitalWrite(M2_IN2, LOW);
  digitalWrite(M2_IN1, HIGH);
  digitalWrite(M1_IN1, HIGH);
  digitalWrite(M1_IN2, LOW);
}

void pararTudo() {
  digitalWrite(M2_IN2, LOW);
  digitalWrite(M2_IN1, LOW);
  digitalWrite(M1_IN1, LOW);
  digitalWrite(M1_IN2, LOW);
}

//Feedback
void sendFeedback(const String& msg) {
  if (deviceConnected) {
    pTxCharacteristic->setValue(msg.c_str());
    pTxCharacteristic->notify();
    Serial.print("TX → ");
    Serial.println(msg);
  }
}

//Callback
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

    if (c == 'F') {
      moverFrente();
      lastMovement = 'F';
      Serial.println("FRENTE");
      sendFeedback("OK:F");

    } else if (c == 'B') {
      moverRe();
      lastMovement = 'B';
      Serial.println("RÉ");
      sendFeedback("OK:B");

    } else if (c == 'L') {
      virarEsquerda();
      lastMovement = 'L';
      Serial.println("ESQUERDA");
      sendFeedback("OK:L");

    } else if (c == 'R') {
      virarDireita();
      lastMovement = 'R';
      Serial.println("DIREITA");
      sendFeedback("OK:R");

    } else if (c == 'P') {
      pararTudo();
      lastMovement = 'P';
      Serial.println("PARADO");
      sendFeedback("OK:P");

    } else if (c == 'S') {
      String param = cmd.substring(1);

    } else {
      Serial.println("Comando desconhecido");
      sendFeedback("ERR:UNKNOWN_CMD");
    }
  }
};

void setup() {
  Serial.begin(115200);

  pinMode(M1_IN1, OUTPUT);
  pinMode(M1_IN2, OUTPUT);
  pinMode(M2_IN1, OUTPUT);
  pinMode(M2_IN2, OUTPUT);

  moverFrente();
  delay(100);
  pararTudo();

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
  Serial.println("Comandos: F, B, L, R, P");
}

void loop(){}

