#include "Arduino.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include "mbedtls/sha256.h"
#include "mbedtls/pk.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/base64.h"
#include "key.h"
#include "uuid.h"
#include "ble_config.h"

BLECharacteristic *pSigCharacteristic;

// Global variable to store the last interaction time
unsigned long lastInteractionTime = 0;
// Global variable to store the BLEServer pointer
BLEServer* pServerInstance = nullptr;

mbedtls_pk_context pk;
mbedtls_ctr_drbg_context ctr_drbg;
mbedtls_entropy_context entropy;

class ServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        Serial.println("Client connected");
        lastInteractionTime = millis(); // Reset timer on new connection
    }

    void onDisconnect(BLEServer* pServer) {
        Serial.println("Client disconnected, restarting advertising");
        delay(100);
        BLEDevice::startAdvertising();
    }
};

class MessageCallback : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        lastInteractionTime = millis(); // Update interaction time on write

        if (value.length() > 0) {
            Serial.print("Received via BLE: ");
            Serial.println(value.c_str());

            // ===== SIGNING PROCESS =====
            static unsigned char hash[32];
            static unsigned char signature[MBEDTLS_MPI_MAX_SIZE];
            static unsigned char b64_output[512];
            size_t sig_len = 0;

            // Hash the input
            if (mbedtls_sha256_ret((unsigned char *)value.c_str(), value.length(), hash, 0) != 0) {
                Serial.println("Hashing failed");
                return;
            }

            // Sign it
            if (mbedtls_pk_sign(&pk, MBEDTLS_MD_SHA256, hash, 0, signature, &sig_len, mbedtls_ctr_drbg_random, &ctr_drbg) != 0) {
                Serial.println("Signing failed");
                return;
            }

            // Base64 encode
            size_t b64_len = 0;
            if (mbedtls_base64_encode(b64_output, sizeof(b64_output), &b64_len, signature, sig_len) != 0) {
                Serial.println("Base64 encoding failed");
                return;
            }

            // Send signature back via BLE
            pSigCharacteristic->setValue((char *)b64_output);
            pSigCharacteristic->notify();
            Serial.println("Signature sent via BLE.");
        }
    }
};

void setup() {
    Serial.begin(115200);

    // ===== KEY & RNG SETUP =====
    mbedtls_pk_init(&pk);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    const char *pers = "ble_signature";
    mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, 
        (const unsigned char *)pers, strlen(pers));
    mbedtls_pk_parse_key(&pk, private_key, sizeof(private_key), NULL, 0);

    // ===== BLE SETUP =====
    BLEDevice::init(BLE_DEVICE_NAME);
    pServerInstance = BLEDevice::createServer(); // Assign to global pointer
    pServerInstance->setCallbacks(new ServerCallbacks());
    BLEService *pService = pServerInstance->createService(SERVICE_UUID);

    // Characteristic to receive the message
    BLECharacteristic *pMessageCharacteristic = pService->createCharacteristic(
        MESSAGE_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );
    pMessageCharacteristic->setCallbacks(new MessageCallback());

    // Characteristic to return the signature
    pSigCharacteristic = pService->createCharacteristic(
        SIG_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pSigCharacteristic->addDescriptor(new BLE2902());

    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->start();
    Serial.println("BLE ready");
}

void loop() {
    // Check for inactivity and disconnect if timeout is reached
    if (pServerInstance != nullptr && pServerInstance->getConnectedCount() > 0 && (millis() - lastInteractionTime > AUTO_DISCONNECT_TIMEOUT_MS)) {
        Serial.println("Client inactive, disconnecting.");
        std::vector<uint16_t> connIds = pServerInstance->getConnIdVec();
        for (uint16_t connId : connIds) {
            pServerInstance->disconnect(connId);
        }
        lastInteractionTime = millis(); // Reset timer after disconnect
    }
    delay(1000); // Check every second
}