// 攀登：补光灯控制 - OneNet 物联网接入
// 供电：12V
// 通信：RS485-TTL

 
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

/************** 函数声明 ***************/
void setup_wifi();
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();
void getSensorData();
void setSensorData(String set_data);
void parseJSON(const char* json);

/************** WiFi 配置 **************/
const char* ssid = "abc";         // WiFi SSID
const char* password = "12345678"; // WiFi 密码

/************ OneNet MQTT 配置 ************/
const char* mqtt_server = "mqtts.heclouds.com";  
const int mqtt_port = 1883; 
const char* device_id = "controller";    
const char* product_id = "61041c855G"; 
const char* api_key = "version=2018-10-31&res=products%2F61041c855G%2Fdevices%2Fcontroller&et=999986799814791288&method=md5&sign=09ZpQDMPvLiXxKr5JQiYeg%3D%3D";

/********* MQTT 主题 *********/
const char* pubTopic = "$sys/61041c855G/controller/thing/property/post";
const char* replyTopic="$sys/61041c855G/controller/thing/property/post/reply";
const char* getTopic = "$sys/61041c855G/controller/thing/sub/property/get";//获取子设备的属性
const char* get_replyTopic = "$sys/61041c855G/controller/thing/sub/property/get_reply";//获取子设备属性和回复
const char* setTopic ="$sys/61041c855G/controller/thing/sub/property/set";//设置子设备的属性
const char* set_replyTopic ="$sys/61041c855G/controller/thing/sub/property/set_reply";//获取设置属性回复


WiFiClient espClient;
PubSubClient client(espClient);


void setup() {

  Serial.begin(9600);

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {

  String serial_data;

  if (!client.connected()) 
  {
    reconnect();
  }
  client.loop();

  getSensorData();

  if(Serial.available())
  {
    serial_data=Serial.readString();
    setSensorData(serial_data);
  }

  delay(500);
}


/************* WiFi 连接 *************/
void setup_wifi() {
  Serial.println("连接 WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\nWiFi 连接成功!");
  Serial.print("IP 地址: ");
  Serial.println(WiFi.localIP());
}

/************* MQTT 订阅回调函数 *************/
void callback(char* topic, byte* payload, unsigned int length) {


  Serial.print("收到 MQTT 消息，主题: \n");
  Serial.println(topic);
  Serial.print("内容: ");
  for (int i = 0; i < length; i++) 
  {
    Serial.print((char)payload[i]);
  }

  // // 转换 payload 为字符串
  // char json[length + 1];
  // memcpy(json, payload, length);
  // json[length] = '\0';

  // // 调用解析函数
  // parseJSON(json);

  Serial.println();
  Serial.println();

}

/************* 连接 MQTT 服务器 *************/
void reconnect() {
  while (!client.connected()) 
  {
    Serial.print("连接 OneNet MQTT...");

    if (client.connect(device_id, product_id, api_key)) 
    {
      Serial.println("连接成功!");
      client.subscribe(replyTopic); // 订阅系统回复属性下发
      client.subscribe(get_replyTopic); // 订阅子设备属性下发
      client.subscribe(set_replyTopic); // 订阅子设备设置属性反馈下发

    } 
    else 
    {
      Serial.printf("连接失败, 状态码=%d, 5秒后重试...\n", client.state());
      switch (client.state()) 
      {
        case -4: Serial.println("连接超时"); break;
        case -3: Serial.println("连接丢失"); break;
        case -2: Serial.println("连接失败"); break;
        case -1: Serial.println("断开连接"); break;
        case 1: Serial.println("协议错误"); break;
        case 2: Serial.println("客户端标识无效"); break;
        case 3: Serial.println("服务器不可用"); break;
        case 4: Serial.println("用户名或密码错误"); break;
        case 5: Serial.println("未授权"); break;
        default: Serial.println("未知错误");
      }
      delay(5000);
    }
  }
}

/************JSON数据相关函数************/

/**** 获取子设备属性 ****/
void getSensorData() 
{
  JsonDocument doc;
  // 设置顶层字段
  doc["id"] = millis();
  doc["version"] = "1.0";

  // 创建嵌套对象
  JsonObject params = doc["params"].to<JsonObject>();
  params["deviceName"] = "led";
  params["productID"] = "61041c855G";

  // 创建嵌套数组
  JsonArray paramList = params["params"].to<JsonArray>();
  paramList.add("led");


  String payload;
  serializeJson(doc, payload);
  if (client.publish(getTopic, payload.c_str())) 
  {
    Serial.println("send data success");
    Serial.printf(payload.c_str());
    Serial.println();
  } 
  else 
  {
    Serial.println("send data fail");
    Serial.println();
  }

  delay(200);
}

/**** 设置子设备属性 ****/
void setSensorData(String set_data) 
{
  JsonDocument doc;
  doc["id"] = String(millis());  // 使用时间戳作为唯一ID
  doc["version"] = "1.0";
  doc["params"]["deviceName"] = "led";
  doc["params"]["productID"] = "61041c855G";
  doc["params"]["params"]["led"] = set_data;

  

  String payload;
  serializeJson(doc, payload);
  if (client.publish(setTopic, payload.c_str())) 
  {
    Serial.println("send data success");
    Serial.printf(payload.c_str());
    Serial.println();
  } 
  else 
  {
    Serial.println("send data fail");
    Serial.println();
  }

  delay(500);
}


/**** 解析平台指令 ****/
void parseJSON(const char* json) {
  // 1. 修正 StaticJsonDocument 声明
  JsonDocument doc; // 确保足够内存空间

  // 2. 解析 JSON
  DeserializationError error = deserializeJson(doc, json);
  
  if (error) {
    Serial.println(error.c_str());
    return;
  }

  if (doc["params"].isNull()) { 
    return;
  }

  JsonObject params = doc["params"];
  
  if (params["led-controller"].isNull()) {
    return;
  }

  const char* ledState = params["led-controller"];
  Serial.print("LED : ");
  Serial.println(ledState);

  String ID = doc["id"];

  if (strcmp(ledState, "led-ON") == 0) 
  {
    digitalWrite(19, HIGH);

  } 
  else if (strcmp(ledState, "led-OF") == 0)
  {
    digitalWrite(19, LOW);
  }

}