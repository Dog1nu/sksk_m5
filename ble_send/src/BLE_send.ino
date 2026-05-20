#include <M5Unified.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEBeacon.h>
#include <BLEAdvertising.h>

BLEAdvertising *pAdvertising;

void setup() {

    auto cfg = M5.config();
    cfg.serial_baudrate = 115200;
    cfg.clear_display = true;
    M5.begin(cfg);

    M5.Display.setRotation(1);
    M5.Display.fillScreen(BLACK);

    M5.Display.setTextColor(WHITE);
    M5.Display.setTextSize(3);

    M5.Display.setCursor(20, 40);
    M5.Display.println("M5 BEACON");

    BLEDevice::init("M5_BEACON");

    BLEServer *pServer =
        BLEDevice::createServer();

    BLEBeacon oBeacon;

    oBeacon.setManufacturerId(0x4C00); // Apple
    oBeacon.setProximityUUID(
        BLEUUID("12345678-1234-1234-1234-1234567890AB")
    );

    oBeacon.setMajor(1);
    oBeacon.setMinor(1);
    oBeacon.setSignalPower(-59);

    BLEAdvertisementData advertisementData;

    advertisementData.setFlags(0x1A);

    std::string strServiceData = "";

    strServiceData += (char)26;
    strServiceData += (char)0xFF;

    strServiceData += oBeacon.getData();

    advertisementData.addData(strServiceData);

    pAdvertising =
        BLEDevice::getAdvertising();

    pAdvertising->setAdvertisementData(
        advertisementData
    );

    pAdvertising->start();

    M5.Display.setCursor(20, 100);
    M5.Display.println("Advertising...");
}

void loop() {

    static bool blue = false;

    if (blue) {
        M5.Display.fillCircle(300, 20, 10, BLUE);
    } else {
        M5.Display.fillCircle(300, 20, 10, BLACK);
    }

    blue = !blue;

    delay(500);
}