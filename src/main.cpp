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
void sendSensorData(int data) ;

/************** WiFi 配置 **************/
const char* ssid = "abc";         // WiFi SSID
const char* password = "12345678"; // WiFi 密码

/************ OneNet MQTT 配置 ************/
const char* mqtt_server = "mqtts.heclouds.com";  
const int mqtt_port = 1883; 
const char* device_id = "test-v1";    
const char* product_id = "w2118dMTYQ"; 
const char* api_key = "version=2018-10-31&res=products%2Fw2118dMTYQ%2Fdevices%2Ftest-v1&et=999986799814791288&method=md5&sign=r68UE6C6rP%2FkQoTUd8TiOg%3D%3D";

/********* MQTT 主题 *********/
const char* pubTopic = "$sys/w2118dMTYQ/test-v1/dp/post/json";
const char* replyTopic= "$sys/w2118dMTYQ/test-v1/dp/post/json/accepted";
const char* getTopic = "$sys/w2118dMTYQ/test-v1/cmd/#";
const char* backTopic = "$sys/w2118dMTYQ/test-v1/cmd/response/#";

/*平台下发信息变量存储*/
byte data_get[32];


WiFiClient espClient;
PubSubClient client(espClient);


void setup() {
  Serial.begin(9600);

  pinMode(19,OUTPUT);

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {

  if (!client.connected()) 
  {
    reconnect();
  }
  client.loop();

  /* 接受平台下发命令 */
  client.subscribe(getTopic); // 订阅属性下发

 
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
    data_get[i]=payload[i];
  }

  if(payload[1]== '1')
  {
    digitalWrite(19,HIGH);
  }
  else if(payload[1]== '0')
  {
    digitalWrite(19,LOW);
  }

  // if((String)* data_get == "led_ON")
  // {
  //   digitalWrite(19,HIGH);

  //   if(digitalRead(19)==HIGH)
  //     client.publish(backTopic, "ON");
  //   else
  //     client.publish(backTopic, "ERROR");

  // }
  // else if( (String)* data_get == "led_OFF")
  // {
  //   digitalWrite(19,LOW);

  // if(digitalRead(19)==HIGH)
  //   client.publish(backTopic, "OFF");
  // else
  //   client.publish(backTopic, "ERROR");
    
  // }
 
 

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
      client.subscribe(replyTopic); // 订阅属性下发
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

/************JSON数据构建************/
void sendSensorData(int data) 
{
  JsonDocument doc;
  doc["id"] = String(millis());  // 使用时间戳作为唯一ID
  doc["version"] = "1.0";
  doc["dp"]["led"]["v"]= data;

  String payload;
  serializeJson(doc, payload);
  if (client.publish(pubTopic, payload.c_str())) 
  {
    Serial.println("数据已发送: " + payload);
  } 
  else 
  {
    Serial.println("发送失败");
  }

  delay(500);
}

