// 补光灯控制 - OneNet物联网接入（HTTP API版）
// 供电：12V
// 通信：RS485-TTL

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

/************** 函数声明 ***************/
void setup_wifi();
void getSensorData();
void getnewSensorData();
void setSensorData(String value);

/************** WiFi配置 **************/
const char* ssid = "abc";         // WiFi SSID
const char* password = "12345678"; // WiFi密码

/************* OneNet API配置 *************/
const char* api_key = "version=2018-10-31&res=products%2F61041c855G%2Fdevices%2Fcontroller&et=2032360000&method=md5&sign=HWXh9%2BSETyFY7tNHvIS1qQ%3D%3D";  
const char* device_id = "controller"; // 主设备ID
const char* sub_device_id = "test-v1"; // 子设备ID
const char* product_id = "61041c855G"; //PID

// API端点配置
const char* get_property_url = "https://iot-api.heclouds.com/thingmodel/query-device-property-detail"; // 属性获取地址
const char* set_property_url = "https://iot-api.heclouds.com/thingmodel/set-device-property"; // 属性设置地址
const char* getnew_property_url = "https://iot-api.heclouds.com/thingmodel/query-device-property";// 最新属性查询地址
// 全局定时变量
unsigned long last_get_time = 0;
const long get_interval = 5000; // 5秒获取间隔

WiFiClientSecure client;
HTTPClient http;

void setup() {

  Serial.begin(9600);
  setup_wifi();
  last_get_time = millis();

  // 配置HTTPS（生产环境需配置CA证书）
  client.setInsecure(); // 临时跳过证书验证
  // client.setCACert(onenet_root_ca); // 正式环境需配置

}

void loop() {
  String serial_data;
  // 定时获取设备状态
  if (millis() - last_get_time >= get_interval) 
  {
    getnewSensorData();
    last_get_time = millis();
  }

  // 串口控制指令处理
  if (Serial.available())
  {
    serial_data=Serial.readString().substring(0,6);
    setSensorData(serial_data); 
  }
}

/************* WiFi连接 *************/
void setup_wifi() {
  Serial.println("连接WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\nWiFi连接成功!");
  Serial.print("IP地址: ");
  Serial.println(WiFi.localIP());
}

/************* 获取设备属性 *************/
// （设备响应过慢，容易响应超时，不建议使用）
void getSensorData() {
  if (WiFi.status() != WL_CONNECTED) 
  {
    Serial.println("WiFi连接已断开");
    return;
  }
    // 构造请求体
    JsonDocument doc;
    doc["product_id"] = product_id;
    doc["device_name"] = sub_device_id;
    
    JsonArray params = doc["params"].to<JsonArray>();
    params.add("led");
    
    String payload;
    serializeJson(doc, payload);
    
    http.begin(client, get_property_url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", api_key);
    http.setTimeout(25000);  // 设置20秒超时

    int httpCode = http.POST(payload);
    if (httpCode == HTTP_CODE_OK) 
    {
      String response = http.getString();
      Serial.print("[Get] 响应: ");
      Serial.println(response);
    } 
    else
    {
      Serial.printf("[Get] HTTP错误码: %d\n", httpCode);
    }
    http.end();
  }

/************* 设备属性最新数据查询 *************/
void getnewSensorData() {
  if (WiFi.status() != WL_CONNECTED) 
  {
    Serial.println("WiFi连接已断开");
    return;
  }
    
  // 构造带查询参数的URL
  String url = String(getnew_property_url) + 
  "?product_id=" + product_id + 
  "&device_name=" + sub_device_id;

  // 发送GET请求
  http.begin(client, url);
  http.addHeader("Authorization", api_key); // 使用动态Token
  http.setTimeout(10000); // 10秒超时


  int httpCode = http.GET(); 
  if (httpCode == HTTP_CODE_OK) 
  {
    String response = http.getString();

    // Serial.print("[GetNew] 响应: ");
    // Serial.println(response);
    // Serial.println();

    // 解析JSON响应
    JsonDocument doc;
    deserializeJson(doc, response);
    JsonArray data = doc["data"];
    Serial.println("-----属性值列表-----");
    for (JsonObject item : data) 
    {
      if (item["identifier"] == "led" && !item["value"].isNull()) 
      {
        Serial.print("LED状态: ");
        Serial.println(item["value"].as<String>());
      }
    }
  } 
  else 
  {
    Serial.printf("[GetNew] HTTP错误码: %d\n", httpCode);
    Serial.println("错误详情: " + http.getString());
  }
  http.end();
  }


/************* 设置设备属性 *************/
void setSensorData(String value) {
  if (WiFi.status() != WL_CONNECTED) 
  {
    Serial.println("WiFi连接已断开");
    return;
  }

  JsonDocument doc;
  doc["product_id"] = product_id;
  doc["device_name"] = sub_device_id;
  doc["params"]["led-controller"] = value;  

  String payload;
  serializeJson(doc, payload);

  http.begin(client, set_property_url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", api_key);

  int httpCode = http.POST(payload);
  if (httpCode == HTTP_CODE_OK) {
    String response = http.getString();
    JsonDocument resDoc;
    DeserializationError error = deserializeJson(resDoc, response);
    
    if (!error && resDoc["code"] == 200) 
    {
      Serial.println("设置成功!消息ID: " + resDoc["data"]["id"].as<String>());
    } 
    else 
    {
      Serial.println("设置失败: " + response);
    }
  } 
  else 
  {
    Serial.printf("[Set] HTTP错误码: %d\n", httpCode);
  }
  http.end();
}