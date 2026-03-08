// --- Verificação de Compilação para a Placa ESP32 ---
#if !defined(ESP32)
  #error Este codigo foi feito para placas ESP32. Por favor, selecione uma placa ESP32 no menu Ferramentas > Placa.
#endif

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// --- Configuração ---
#define LED_PIN 2

// UUIDs para o serviço e característica BLE
#define SERVICE_UUID        "12345678-1234-5678-1234-56789abcdef0"
#define CHARACTERISTIC_UUID "abcdef01-1234-5678-1234-56789abcdef0"

BLEServer *pServer = nullptr;
BLECharacteristic *pCharacteristic = nullptr;
bool deviceConnected = false;

// --- Variáveis de Temporização (Software) ---
// Voláteis para comunicação segura entre o BLE (que escreve) e o loop() (que lê).
volatile unsigned long tempoLigadoFinal_us = 100;
volatile unsigned long tempoApagadoFinal_us = 1000000 - 100; // Valor inicial para 1 Hz

// Callback para eventos de conexão/desconexão
class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        Serial.println(">>> Dispositivo Conectado via BLE");
    }

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        Serial.println("<<< Dispositivo Desconectado");
        pServer->getAdvertising()->start();
    }
};

// Callback para eventos de escrita na característica BLE
class MyCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        Serial.println("--- onWrite Recebeu um Comando ---");
        std::string value = pCharacteristic->getValue().c_str();
        Serial.print("Valor recebido: '");
        Serial.print(value.c_str());
        Serial.println("'");

        if (value.length() > 0) {
            size_t commaPos1 = value.find(',');
            if (commaPos1 != std::string::npos) {
                size_t commaPos2 = value.find(',', commaPos1 + 1);
                if (commaPos2 != std::string::npos) {
                    std::string freqStr = value.substr(0, commaPos1);
                    std::string adjustStr = value.substr(commaPos1 + 1, commaPos2 - (commaPos1 + 1));
                    std::string pulseStr = value.substr(commaPos2 + 1);
                    
                    double frequencia_desejada = atof(freqStr.c_str());
                    unsigned int ajuste_periodo_us = atoi(adjustStr.c_str());
                    unsigned int tempo_pulso_us = atoi(pulseStr.c_str());

                    Serial.print("Frequência Base: ");
                    Serial.println(frequencia_desejada);
                    Serial.print("Ajuste de Período: ");
                    Serial.println(ajuste_periodo_us);
                    Serial.print("Tempo de Pulso: ");
                    Serial.println(tempo_pulso_us);

                    if (frequencia_desejada > 0 && tempo_pulso_us > 0) {
                        // O tempo ligado é definido diretamente pelo slider de "Tempo de Pulso"
                        tempoLigadoFinal_us = tempo_pulso_us;

                        // Calcula o tempo apagado
                        double periodo_base_us = 1000000.0 / frequencia_desejada;
                        double tempo_apagado_base_us = periodo_base_us - tempoLigadoFinal_us;
                        
                        // Aplica o ajuste fino do slider de "Ajuste de Período"
                        long tempo_final = (long)tempo_apagado_base_us + ajuste_periodo_us;

                        // Garante que o tempo final não seja negativo
                        if (tempo_final < 0) {
                            tempo_final = 0;
                        }
                        
                        tempoApagadoFinal_us = tempo_final;

                        Serial.print("Tempo Ligado Final: ");
                        Serial.println(tempoLigadoFinal_us);
                        Serial.print("Tempo Apagado Final: ");
                        Serial.println(tempoApagadoFinal_us);
                    }
                }
            }
        }
    }
};

void setup() {
    Serial.begin(115200);
    delay(2000); 
    Serial.println("\n\n--- INICIANDO SETUP (delayMicroseconds) ---");

    pinMode(LED_PIN, OUTPUT);
    
    Serial.println("--> Configurando o BLE...");
    BLEDevice::init("ESP32_Strobo_Software");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID,
                        BLECharacteristic::PROPERTY_WRITE
                      );
    pCharacteristic->setCallbacks(new MyCallbacks());
    pService->start();
    
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pServer->getAdvertising()->start();
    Serial.println("<-- BLE pronto. Aguardando conexão. 📡");
    Serial.println("--- SETUP CONCLUÍDO ---");
}

void loop() {
        digitalWrite(LED_PIN, HIGH);
        delayMicroseconds(tempoLigadoFinal_us);
        digitalWrite(LED_PIN, LOW);
        delayMicroseconds(tempoApagadoFinal_us);

}
