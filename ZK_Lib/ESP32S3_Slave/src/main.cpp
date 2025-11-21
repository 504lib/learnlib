#include <Arduino.h>
#include "./protocol/protocol.hpp"
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid            = "黑A123456"; // AP 鍚嶇О
const char* password        = "12345678"; // AP 瀵嗙爜
const char* station_server  = "http://192.168.4.1";

String target_ssid                  = "ESP32-Access-Point"; // AP 鍚嶇О
String target_password              = "12345678"; // AP 瀵嗙爜
String target_station_server        = "http://192.168.4.1";

#define LED_Pin LED_BUILTIN

class VehicleTester {
private:
    String vehiclePlate;
    int vehicleRoute;
    String serverURL;
    
    struct TestResult {
        String testName;
        bool success;
        String details;
        unsigned long duration;
    };
    
    std::vector<TestResult> testResults;

public:
    VehicleTester(const String& plate, int route, const String& url) 
        : vehiclePlate(plate), vehicleRoute(route), serverURL(url) {}
    
    void runComprehensiveTest() {
        Serial.println("\n" + String(60, '='));
        Serial.println("🚌 车辆通信综合测试工具");
        Serial.println(String(60, '='));
        
        // 测试WiFi连接
        if (!testWiFiConnection()) {
            Serial.println("❌ WiFi测试失败，停止后续测试");
            return;
        }
        
        // 运行各项测试
        testServerReachability();
        testPostRequests();
        testErrorConditions();
        testPerformance();
        
        // 生成测试报告
        generateTestReport();
    }
    
private:
    bool testWiFiConnection() {
        Serial.println("\n--- WiFi连接测试 ---");
        
        unsigned long startTime = millis();
        
        Serial.print("连接SSID: ");
        Serial.println("ESP32-Access-Point");
        
        WiFi.begin("ESP32-Access-Point", "12345678");
        
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 30) {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        
        unsigned long duration = millis() - startTime;
        
        TestResult result;
        result.testName = "WiFi连接";
        result.duration = duration;
        
        if (WiFi.status() == WL_CONNECTED) {
            result.success = true;
            result.details = "连接成功, IP: " + WiFi.localIP().toString() + 
                           ", RSSI: " + String(WiFi.RSSI()) + "dBm";
            Serial.println("\n✅ " + result.details);
        } else {
            result.success = false;
            result.details = "连接失败，尝试次数: " + String(attempts);
            Serial.println("\n❌ " + result.details);
        }
        
        testResults.push_back(result);
        return result.success;
    }
    
    void testServerReachability() {
        Serial.println("\n--- 服务器可达性测试 ---");
        
        TestResult result;
        result.testName = "服务器可达性";
        
        if (WiFi.status() != WL_CONNECTED) {
            result.success = false;
            result.details = "WiFi未连接";
            testResults.push_back(result);
            return;
        }
        
        unsigned long startTime = millis();
        
        // 方法A：使用实际存在的GET接口（如/api/info）
        HTTPClient http;
        http.begin("http://192.168.4.1/api/info"); // 使用已知存在的接口
        
        int httpCode = http.GET();
        unsigned long duration = millis() - startTime;
        
        result.duration = duration;
        
        // 合理的成功条件：200表示正常，404/500表示接口不存在但服务器在运行
        if (httpCode == 200) {
            result.success = true;
            result.details = "服务器响应正常，HTTP代码: " + String(httpCode);
            Serial.println("✅ " + result.details);
        } else if (httpCode == 404 || httpCode == 500) {
            result.success = true; // 服务器在运行，只是接口不存在
            result.details = "服务器运行中（接口不存在），HTTP代码: " + String(httpCode);
            Serial.println("⚠️ " + result.details);
        } else {
            result.success = false;
            result.details = "服务器无响应，HTTP代码: " + String(httpCode);
            Serial.println("❌ " + result.details);
        }
        
        http.end();
        testResults.push_back(result);
    }
    
    void testPostRequests() {
        Serial.println("\n--- POST请求测试 ---");
        
        String testCases[][3] = {
            {"waiting", "京A12345", "0"},
            {"arriving", "沪B56789", "1"}, 
            {"leaving", "沪B56789", "1"},
            {"waiting", "黑A12345", "2"},
            {"waiting", "测试车牌", "3"}
        };
        
        int totalTests = sizeof(testCases) / sizeof(testCases[0]);
        int passedTests = 0;
        
        for (int i = 0; i < totalTests; i++) {
            String status = testCases[i][0];
            String plate = testCases[i][1];
            int route = testCases[i][2].toInt();
            
            Serial.print("测试 ");
            Serial.print(i + 1);
            Serial.print("/");
            Serial.print(totalTests);
            Serial.print(": ");
            Serial.print(plate);
            Serial.print(" 路线");
            Serial.print(route);
            Serial.print(" ");
            Serial.print(status);
            Serial.print(" ... ");
            
            unsigned long startTime = millis();
            bool success = sendSinglePost(route, plate, status);
            unsigned long duration = millis() - startTime;
            
            if (success) {
                Serial.println("✅ 成功 (" + String(duration) + "ms)");
                passedTests++;
            } else {
                Serial.println("❌ 失败 (" + String(duration) + "ms)");
            }
            
            delay(500); // 测试间隔
        }
        
        TestResult result;
        result.testName = "POST请求";
        result.success = (passedTests == totalTests);
        result.details = String(passedTests) + "/" + String(totalTests) + " 通过";
        result.duration = 0; // 每个子测试已有独立时长
        
        testResults.push_back(result);
    }
    
    void testErrorConditions() {
        Serial.println("\n--- 错误条件测试 ---");
        
        // 测试无效URL
        testInvalidURL();
        
        // 测试超长数据
        testOversizedData();
        
        // 测试WiFi断开情况
        testDisconnectedWiFi();
    }
    
    void testInvalidURL() {
        Serial.print("测试无效URL ... ");
        
        HTTPClient http;
        http.begin("http://192.168.4.1/invalid_url");
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
        
        int httpCode = http.POST("route=1&plate=测试&status=waiting");
        
        if (httpCode == 404) {
            Serial.println("✅ 正确返回404");
        } else {
            Serial.println("❌ 异常响应: " + String(httpCode));
        }
        
        http.end();
    }
    
    void testOversizedData() {
        Serial.print("测试超长数据 ... ");
        
        String longPlate = "超长车牌号";
        for (int i = 0; i < 50; i++) {
            longPlate += "非常长";
        }
        
        bool success = sendSinglePost(1, longPlate, "waiting");
        
        if (!success) {
            Serial.println("✅ 正确拒绝超长数据");
        } else {
            Serial.println("⚠️ 接受了超长数据");
        }
    }
    
    void testDisconnectedWiFi() {
        Serial.print("测试WiFi断开 ... ");
        
        WiFi.disconnect();
        delay(100);
        
        bool success = sendSinglePost(1, "测试", "waiting");
        
        if (!success) {
            Serial.println("✅ 正确处理断开情况");
        } else {
            Serial.println("❌ 在断开状态下错误报告成功");
        }
        
        // 重新连接
        WiFi.begin("ESP32-Access-Point", "12345678");
        while (WiFi.status() != WL_CONNECTED) {
            delay(100);
        }
    }
    
    void testPerformance() {
        Serial.println("\n--- 性能测试 ---");
        
        const int numRequests = 3;
        unsigned long totalTime = 0;
        int successCount = 0;
        
        for (int i = 0; i < numRequests; i++) {
            unsigned long startTime = millis();
            bool success = sendSinglePost(i % 3, "性能测试" + String(i), "waiting");
            unsigned long duration = millis() - startTime;
            
            if (success) {
                totalTime += duration;
                successCount++;
            }
            
            delay(200);
        }
        
        TestResult result;
        result.testName = "性能测试";
        result.success = (successCount == numRequests);
        
        if (successCount > 0) {
            result.details = "平均响应: " + String(totalTime / successCount) + "ms, " +
                           String(successCount) + "/" + String(numRequests) + " 成功";
        } else {
            result.details = "所有请求失败";
        }
        
        Serial.println(result.details);
        testResults.push_back(result);
    }
    
    bool sendSinglePost(int route, const String& plate, const String& status) {
        if (WiFi.status() != WL_CONNECTED) {
            return false;
        }
        
        HTTPClient http;
        http.begin(serverURL);
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
        
        String postData = "route=" + String(route) + 
                         "&plate=" + plate + 
                         "&status=" + status;
        
        int httpCode = http.POST(postData);
        bool success = (httpCode == 200);
        
        http.end();
        return success;
    }
    
    void generateTestReport() {
        Serial.println("\n" + String(60, '='));
        Serial.println("📊 测试报告");
        Serial.println(String(60, '='));
        
        int totalTests = testResults.size();
        int passedTests = 0;
        
        for (const auto& result : testResults) {
            String status = result.success ? "✅ 通过" : "❌ 失败";
            Serial.printf("%-20s %-10s %-30s %lums\n",
                         result.testName.c_str(),
                         status.c_str(),
                         result.details.c_str(),
                         result.duration);
            
            if (result.success) passedTests++;
        }
        
        Serial.println(String(60, '-'));
        Serial.printf("总计: %d/%d 测试通过 (%.1f%%)\n", 
                     passedTests, totalTests, 
                     (passedTests * 100.0) / totalTests);
        
        if (passedTests == totalTests) {
            Serial.println("🎉 所有测试通过！通信基础功能正常。");
        } else {
            Serial.println("⚠️  部分测试失败，请检查问题。");
        }
    }
};

// 使用示例
VehicleTester tester("京A12345", 1, "http://192.168.4.1/api/vehicle_report");

void simpleConnectionTest() {
    Serial.println("\n=== 简化连接测试 ===");
    
    // 测试每个路线
    for (int route = 0; route <= 4; route++) {
        Serial.printf("测试路线 %d: ", route);
        
        HTTPClient http;
        http.begin("http://192.168.4.1/api/vehicle_report");
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
        
        String postData = "route=" + String(route) + 
                         "&plate=测试车牌" + String(route) + 
                         "&status=waiting";
        
        int httpCode = http.POST(postData);
        String response = http.getString();
        
        Serial.printf("HTTP %d, 响应: %s\n", httpCode, response.c_str());
        http.end();
        
        delay(500);
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000); // 等待串口连接
    
    tester.runComprehensiveTest();
    
    Serial.println("\n输入 't' 重新运行测试，输入 's' 发送单个请求");
}

void loop() {
    if (Serial.available()) {
        String input = Serial.readString();
        input.trim();
        
        if (input == "t") {
            tester.runComprehensiveTest();
        } else if (input == "s") {
            // 发送单个测试请求
            simpleConnectionTest();
        }
    }
    
    delay(100);
}