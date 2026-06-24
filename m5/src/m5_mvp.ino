#include <M5Unified.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <SD.h>
#include <time.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>


BLEScan* pBLEScan;
TaskHandle_t bleTaskHandle;

portMUX_TYPE sharedStateMux = portMUX_INITIALIZER_UNLOCKED;

// =========================
// 歩数
// =========================
volatile int stepCount = 0;

float gravity = 1.0f;
float filtered = 0.0f;
float prevFiltered = 0.0f;

bool rising = false;

unsigned long lastStepMillis = 0;

const float ALPHA = 0.92f;

const float STEP_THRESHOLD = 0.18f;

const unsigned long STEP_INTERVAL = 300;

// =========================
// BLE
// =========================
volatile int latestRSSI = -100;

volatile float distanceMeter = -1;



// =========================
// SD / CSV
// =========================
char csvFileName[32] = "";
unsigned long lastCSVMillis = 0;
const unsigned long CSV_INTERVAL = 10000;  // 10秒

void initSDCard()
{
    if (!SD.begin(4)) {  // M5Stack CoreS3のSDカード対応ピン
        Serial.println("SD card initialization failed");
        return;
    }
    Serial.println("SD card initialized");
}

void createNewCSVFile()
{
    // タイムスタンプ付きのファイル名を生成
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    
    sprintf(csvFileName, "/data_%04d%02d%02d_%02d%02d%02d.csv",
            timeinfo->tm_year + 1900,
            timeinfo->tm_mon + 1,
            timeinfo->tm_mday,
            timeinfo->tm_hour,
            timeinfo->tm_min,
            timeinfo->tm_sec);
    
    // ファイルを作成してヘッダーを書き込み
    File file = SD.open(csvFileName, FILE_WRITE);
    if (file) {
        file.println("Timestamp,Steps,Distance(m)");
        file.close();
        Serial.printf("New CSV file created: %s\n", csvFileName);
    }
}

void saveDataToCSV(unsigned long timestamp, int steps, float distance)
{
    if (csvFileName[0] == '\0') {
        return;  // ファイル名がまだ設定されていない
    }
    
    File file = SD.open(csvFileName, FILE_APPEND);
    if (file) {
        file.printf("%lu,%d,%.2f\n", timestamp, steps, distance);
        file.close();
        Serial.printf("Data saved to CSV: %lu, %d, %.2f\n", timestamp, steps, distance);
    } else {
        Serial.println("Failed to open CSV file");
    }
}

// =========================
// 距離計算
// =========================
float calculateDistance(int rssi)
{
    int txPower = -59;

    if (rssi == 0) return -1;

    float ratio = rssi * 1.0 / txPower;

    if (ratio < 1.0) {
        return pow(ratio, 10);
    }

    return 0.89976 * pow(ratio, 7.7095)
            + 0.111;
}

// =========================
// BLE callback
// =========================
class MyCallbacks
: public BLEAdvertisedDeviceCallbacks {

    void onResult(
        BLEAdvertisedDevice device
    ) {

        String name =
            device.getName().c_str();

        if (name == "M5_BEACON") {

            int rssi = device.getRSSI();
            float distance = calculateDistance(rssi);

            portENTER_CRITICAL(&sharedStateMux);
            latestRSSI = rssi;
            distanceMeter = distance;
            portEXIT_CRITICAL(&sharedStateMux);
        }
    }
};

// =========================
// BLE task
// Core1
// =========================
void bleTask(void *arg)
{
    while (true) {

        pBLEScan->start(
            1,
            false
        );

        pBLEScan->clearResults();

        vTaskDelay(
            200 / portTICK_PERIOD_MS
        );
    }
}

// =========================
// UI / I2C task
// Core1
// =========================
// UI
// =========================
void drawUI()
{
    
    M5.Display.fillScreen(TFT_NAVY);
    M5.Display.setBrightness(1);

    M5.Display.setTextColor(WHITE);

    M5.Display.setTextSize(6);

    M5.Display.setCursor(20, 60);
    M5.Display.println("TARO");

    M5.Display.setTextSize(3);

    M5.Display.setCursor(20, 140);
    M5.Display.printf(
        "STEP: %d",
        stepCount
    );

    // Battery level at bottom right (small)
    int battery = M5.Power.getBatteryLevel();
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(TFT_YELLOW);
    
    int x = M5.Display.width() - 100;
    int y = M5.Display.height() - 40;
    
    M5.Display.setCursor(x, y);
    M5.Display.printf("BAT: %d%%", battery);
}

// =========================
// setup
// =========================
void setup()
{
    auto cfg = M5.config();
    cfg.serial_baudrate = 115200;
    cfg.clear_display = true;
    M5.begin(cfg);

    Serial.begin(115200);

    delay(100);

    M5.Display.setRotation(1);

    // SD Card init
    initSDCard();
    
    // Create new CSV file
    createNewCSVFile();

    // BLE
    BLEDevice::init("");

    pBLEScan =
        BLEDevice::getScan();

    pBLEScan->setAdvertisedDeviceCallbacks(
        new MyCallbacks()
    );

    pBLEScan->setActiveScan(true);

    pBLEScan->setInterval(100);

    pBLEScan->setWindow(80);

    // BLE scan to Core1
    xTaskCreatePinnedToCore(
        bleTask,
        "BLE",
        4096,
        NULL,
        2,
        &bleTaskHandle,
        1
    );
   
}

// =========================
// loop
// Core1: I2C / IMU / UI
// =========================
void loop()
{
    static float gravity = 1.0f;
    static float filtered = 0.0f;
    static float prevFiltered = 0.0f;
    static bool rising = false;
    static unsigned long lastStepMillis = 0;
    static unsigned long lastUI = 0;
    static bool initialUIShown = false;

    const float ALPHA = 1.92f;
    const float STEP_THRESHOLD = 0.18f;
    const unsigned long STEP_INTERVAL = 300;

    // 起動直後の静止状態を使って重力をならす
    static bool calibrated = false;
    if (!calibrated) {
        for (int i = 0; i < 40; i++) {
            float ax, ay, az;
            M5.Imu.getAccelData(&ax, &ay, &az);
            float accelMagnitude = sqrt(ax * ax + ay * ay + az * az);
            gravity = gravity * 0.9f + accelMagnitude * 0.1f;
            delay(20);
        }
        calibrated = true;
    }

    float ax, ay, az;

    // IMU取得
    M5.Imu.getAccelData(
        &ax,
        &ay,
        &az
    );

    float accelMagnitude = sqrt(ax * ax + ay * ay + az * az);

    // 重力除去
    gravity =
        gravity * ALPHA +
        accelMagnitude * (1.0f - ALPHA);

    filtered = accelMagnitude - gravity;

    bool currentRising = filtered > prevFiltered;

    // Peak
    if (
        rising &&
        !currentRising &&
        prevFiltered > STEP_THRESHOLD
    ) {

        unsigned long now = millis();

        if (now - lastStepMillis > STEP_INTERVAL) {

            stepCount++;

            lastStepMillis = now;

            Serial.printf(
                "STEP %d\n",
                stepCount
            );
        }
    }

    rising = currentRising;
    prevFiltered = filtered;

    if (!initialUIShown) {
        drawUI();
        lastUI = millis();
        initialUIShown = true;
    }

    // CSV保存（10秒ごと）
    unsigned long now = millis();
    if (now - lastCSVMillis > CSV_INTERVAL) {
        float distanceSnapshot;
        portENTER_CRITICAL(&sharedStateMux);
        distanceSnapshot = distanceMeter;
        portEXIT_CRITICAL(&sharedStateMux);
        
        saveDataToCSV(now / 1000, stepCount, distanceSnapshot);
        lastCSVMillis = now;
    }

    // UI update every 5 seconds
    if (millis() - lastUI > 5000) {
        drawUI();
        lastUI = millis();
    }

    delay(30);
}