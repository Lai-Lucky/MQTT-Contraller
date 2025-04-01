// 攀登：补光灯控制 - OneNet 物联网接入
// 供电：12V
// 通信：RS485-TTL

 
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>

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
const char* getTopic = "$sys/61041c855G/test-v1/thing/property/get";
const char* get_replyTopic = "$sys/61041c855G/test-v1/thing/property/get_reply";
const char* setTopic = "$sys/61041c855G/test-v1/thing/property/set";
const char* set_replyTopic ="$sys/61041c855G/test-v1/thing/property/set_reply";


int last_time=0;


WiFiClient espClient;
PubSubClient client(espClient);


void setup() {

  Serial.begin(9600);

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  last_time=millis();
}

void loop() {

  String serial_data;

  if (!client.connected()) 
  {
    reconnect();
  }
  client.loop();

  if(millis()-last_time >= 2000)
  {
    getSensorData();
    last_time=millis();
  }
    
  if(Serial.available())
  {
    serial_data=Serial.readString();
    setSensorData(serial_data);
  }

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

  JsonArray params = doc.createNestedArray("params");
  params.add("led");

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


}

/**** 设置子设备属性 ****/
void setSensorData(String set_data) 
{
  JsonDocument doc;
  doc["id"] = millis();
  doc["version"] = "1.0";

  doc["params"]["led-controller"]=set_data;

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

}


