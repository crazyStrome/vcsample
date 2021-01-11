#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <Adafruit_ADS1015.h>

// WiFi parameters
const char* ssid = "MI8pro";
const char* password = "12345600";
// HotSpot config
IPAddress local_IP(192,168,1,100);
IPAddress gateway(192,168,1,100);
IPAddress subnet(255,255,255,0);

//Web Server
ESP8266WebServer server(8080);

String rootpage = String("")+
"<!DOCTYPE html>\n" +
"<script src='https://cdn.bootcdn.net/ajax/libs/jquery/3.5.1/jquery.js'></script>\n" +
"<h1>current and voltage sample</h1>\n" +
"<br/>\n" +
"<body>\n" +
"<script type='text/javascript'>\n" +
"    $(function () {\n" +
"        setInterval(reflush, 1000)\n" +
"    })\n" +
"    function reflush() {\n" +
"        $.getJSON('./get', function(json) {\n" +
"            $('#tmp')[0].innerHTML = json.voltage\n" +
"            $('#voltage').html($('#tmp').html())\n" +
"\n" +
"            $('#tmp')[0].innerHTML = json.current\n" +
"            $('#current').html($('#tmp').html())\n" +
"\n" +
"            $('#tmp')[0].innerHTML = json.power\n" +
"            $('#power').html($('#tmp').html())\n" +
"\n" +
"            $('#tmp')[0].innerHTML = json.serial\n" +
"            $('#serial').html($('#tmp').html())\n" +
"        })\n" +
"    }\n" +
"</script>\n" +
"    <label id='tmp' hidden></label>\n" +
"    <label>voltage: </label>\n" +
"    <b>\n" +
"        <i>\n" +
"            <label id='voltage'>0.00</label>\n" +
"        </i>\n" +
"    </b>\n" +
"    V<br/>\n" +
"    <label>current: </label>\n" +
"    <b>\n" +
"        <i>\n" +
"            <label id='current'>0.00</label>\n" +
"        </i>\n" +
"    </b>\n" +
"    A<br/>\n" +
"    <label>power: </label>\n" +
"    <b>\n" +
"        <i>\n" +
"            <label id='power'>0.00</label>\n" +
"        </i>\n" +
"    </b>\n" +
"    W<br/>\n" +
"    <label>serial-number: </label>\n" +
"    <b>\n" +
"        <i>\n" +
"            <label id='serial'>0.00</label>\n" +
"        </i>\n" +
"    </b>\n" +
"</body>\n";

// 电压电流测量
Adafruit_ADS1115 ads(0x48);
float voltage = 5.00;
float current = 1.11;
float power = voltage * current;
void setup()
{
  //设置端口号
  Serial.begin(115200);
  Serial.println();

  Serial.println("Config WIFI, first connect to WIFI, second release a WIFI spot");

  //设置WiFi连接
  Serial.printf("start connecting to WIFI:%s\n", ssid);
  WiFi.begin(ssid, password);
  //失败重试次数
  int count = 30;
  bool flag = false;
  while (count >= 0) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("\nWiFi %s is connected\n", ssid);
      Serial.print("device IP is ");
      Serial.println(WiFi.localIP());
      flag = true;
      break;
    }
    delay(500);
    count --;
    Serial.print(".");
  }
  Serial.println("end connecting to WiFi");

//  if (!flag) {
//    //设置热点AP
//    Serial.println("WiFi is not connected, so start setting SoftAP");
//    Serial.println(WiFi.softAP("ESPsoftAP", "12345600", 1, false, 4) ? "Ready" : "Failed!");
//
//    Serial.print("Soft-AP IP address = ");
//    Serial.println(WiFi.softAPIP());
//    Serial.println("end setting SoftAP");
//  }
  // 设置服务器
  
  server.on("/", HTTP_GET, handleRoot);
  server.on("/get", HTTP_GET, handleGet);
  server.begin();

  ads.begin();
}
void handleRoot() {
  server.send(200, "text/html", rootpage);  
}
void handleGet() {
  StaticJsonDocument<100> json;
  json["voltage"] = voltage;
  json["current"] = current;
  json["power"] = power;
  char serial[100];
  sprintf(serial, "0x%x", millis());
  json["serial"] = serial;
  char msg[measureJson(json) + 1];
  serializeJson(json, msg, measureJson(json) + 1);
  server.send(200, "text/plain", msg);
  Serial.printf("handleGet: %s\n", msg);
}
void loop() {
  //ADC电压电流采集，功率计算
  int16_t adc0, adc1, adc2, adc3;
  adc0 = ads.readADC_SingleEnded(0);
  adc1 = ads.readADC_SingleEnded(1);
  adc2 = ads.readADC_SingleEnded(2);
  adc3 = ads.readADC_SingleEnded(3);
  // AD测得的值
  voltage = ((adc0 - adc1) * 0.1875) / 1000;
  current = (adc3 - adc2)*0.1875 / 1000;

  //矫正，通过取两个点求斜率y=ax+b,a=2.028,b=1.1568,x是测量值,y是实际输入电压值
  voltage = 2.028 * voltage + 1.1568;
  //current就是测得的电压除以采样电阻0.2
  current = current * 5;
  
  power = voltage * current;
  //server处理客户端的请求
  server.handleClient();
}
