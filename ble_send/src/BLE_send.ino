#include <M5Unified.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEBeacon.h>
#include <BLEAdvertising.h>

BLEAdvertising *pAdvertising;

void drawBattery()
{
    int battery = M5.Power.getBatteryLevel();

    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(TFT_YELLOW, BLACK);

    int x = M5.Lcd.width() - 100;
    int y = M5.Lcd.height() - 24;

    M5.Lcd.setCursor(x, y);
    M5.Lcd.printf("BAT: %d%%", battery);
}

void setup() {

    M5.begin();

    M5.Lcd.setRotation(1);
    M5.Lcd.fillScreen(BLACK);

    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(3);

    M5.Lcd.setCursor(20, 40);
    M5.Lcd.println("M5 BEACON");

    drawBattery();

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

    M5.Lcd.setCursor(20, 100);
    M5.Lcd.println("Advertising...");
}

void loop() {

    static bool blue = false;
    static unsigned long lastBatteryUpdate = 0;

    if (blue) {
        M5.Lcd.fillCircle(300, 20, 10, BLUE);
    } else {
        M5.Lcd.fillCircle(300, 20, 10, BLACK);
    }

    blue = !blue;

    if (millis() - lastBatteryUpdate > 5000) {
        drawBattery();
        lastBatteryUpdate = millis();
    }

    delay(500);
}