##  前言

毕设需要测量一个电路的电压和电流，从而计算出来功率。这个电路是和移动设备绑在一起的，所以测量的信号要通过WiFi传递出来。而接收信号的方法，可以是网页接收或者app，app太复杂了，所以写一个前端小网页要简单得多。

##  1.  硬件设计

硬件电路主要就是一个采样电路和一个单片机：NodeMCU。

###  1.1.  采样电路

![image-20210111154101333](https://gitee.com/crazstom/pics/raw/master/img/image-20210111154101333.png)

这个采样电路的作用就是让需要采样的数据从它流过，然后采集返回数据。

电路图的上半部分就是对经过的电压电流采样的电路。P1输入的电压范围为0\~10V，所以在红框1中使用两个20kΩ的分压电阻使得采样的电压范围为0\~5V，为什么分压到这个范围，是因为之后的ADS1115只能采样到0\~VCC的电压，而该芯片的电压在该电路中设计为5V。如果输入电压范围更宽的话，可以适当调整分压电阻的大小。

红框1中还有一个电流采样点R3，它是0.2Ω的采样电阻，这样会将电流转化为电压信号，传递给ADS1115采样。在这个电路中，如果过3A的电流，采样电阻上就有0.6V的电压传递给ADS1115。采样电阻可以更大，这样采得的电流数据就更准确。但是，电阻太大导致损耗也会增大，同时压降也会很大。主电路是要给后续设备供电的，采样压降太大，后续设备也会受影响。

红框2中就是采样信号的芯片：ADS1115，它有四个采样脚，可以差分采样和单独采样。通过SCL和SDA两个管脚以I2C的方式和单片机连接，从而将采样的信号传输出去。在本文中，单片机用的是NodeMCU。

采样电路通过稳压芯片产生一路5V输出给NodeMCU供电，同时SCL、SDA两个管脚接受NodeMCU的控制，将采样数据传递出去。

![image-20210104093005747](https://gitee.com/crazstom/pics/raw/master/img/image-20210104093005747.png)

###  1.2.  NodeMCU

> NodeMCU是一个开源的[物联网](https://baike.baidu.com/item/物联网)平台。它使用[Lua](https://baike.baidu.com/item/Lua)脚本语言编程。该平台基于eLua开源项目，底层使用ESP8266 sdk 0.9.5版本。该平台使用了很多开源项目，例如 lua-cjson，spiffs。NodeMCU包含了可以运行在esp8266 WiFi SoC芯片之上的固件,以及基于ESP-12模组的硬件。

本文中使用的是D1 mini版的NodeMCU，该开发板上集成了esp8266芯片，它可以用来产生热点或者连接WiFi，从而构成一个服务器。这个开发板可以让开发者以arduino的方式进行编程设计，只不过需要在arduino IDE上导入对应的开发板即可。

![image-20210111194033869](https://gitee.com/crazstom/pics/raw/master/img/image-20210111194033869.png)

##  2.  软件设计

###  2.1.  arduino ide添加开发板

进入arduino ide的首选项

![image-20210101142931508](https://gitee.com/crazstom/pics/raw/master/img/image-20210101142931508.png)

在首选项的"附加开发板管理器网址"中添加网址：

> http://arduino.esp8266.com/stable/package_esp8266com_index.json

回到主面板，进入"工具->开发板->开发板管理器"，在如下图中输入esp：

![image-20210101143214853](https://gitee.com/crazstom/pics/raw/master/img/image-20210101143214853.png)

安装即可实现新开发板的添加。

在开发板中就可以看到NodeMCU的选择。

![image-20210111200327976](https://gitee.com/crazstom/pics/raw/master/img/image-20210111200327976.png)

###  2.2.  建立网络

arduino有相关的[ESP8266WIFI库](https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/readme.html)来操作WiFi，建立网络。

首先，这个开发板既可以连接WiFi，也可以创建热点让其他设备连接。两种方式都可以为之后的服务器做准备。

####  2.2.1.  连接WiFi

首先导入ESP8266WiFi.h头文件，该文件可以在库管理中添加WiFi库获得。

在`setup`函数中通过`WiFi.begin(ssid, password);`发起连接WiFi的调用，然后通过`while`不断轮询`WiFi.status()`检测是否连接成功。

如果连接成功，在串口上打印出它获得的IP。

```c
// Import required libraries
#include <ESP8266WiFi.h>

// WiFi parameters
const char* ssid = "MI8pro";
const char* password = "12345600";

void setup(void)
{
  // Start Serial
  Serial.begin(115200);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  // Print the IP address
  Serial.println(WiFi.localIP());
}
       
void loop() {
}
```

####  2.2.2.  开启热点

使用NodeMCU产生一个热点，因为NodeMCU没有和外部网络连接，所以这个热点是局域网式的。

很简单，就是在`setup`中通过`WiFi.softAP`函数来开启热点。

在此基础上可以进一步实现网络的连接。可以在2.2.1节代码中添加重试次数，如果重试多次还无法连接到对应WiFi，就自己建立热点。

不过后期，为了便捷开发，只是使用WiFi连接来建立网络。

```c
#include <ESP8266WiFi.h>

IPAddress local_IP(192,168,0,100);
IPAddress gateway(192,168,0,100);
IPAddress subnet(255,255,255,0);

void setup()
{
  Serial.begin(115200);
  Serial.println();

  Serial.print("Setting soft-AP configuration ... ");
  Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Ready" : "Failed!");

  Serial.print("Setting soft-AP ... ");
  Serial.println(WiFi.softAP("ESPsoftAP", "12345600", 1, false, 10) ? "Ready" : "Failed!");

  Serial.print("Soft-AP IP address = ");
  Serial.println(WiFi.softAPIP());
}

void loop() {}
```

###  2.3.  服务器

arduino也有相应的库来建立HTTP服务器。

这个服务器主要处理两个路径的请求：`/`和`/get`。其中，根目录就是返回给浏览器一个网页，而网页中会通过js不停向`/get`发送请求，获取采集的电压电流等数据。

网页如下。

```html
<!DOCTYPE html>
<script src='https://cdn.bootcdn.net/ajax/libs/jquery/3.5.1/jquery.js'></script>
<h1>current and voltage sample</h1>
<br/>
<body>
<script type='text/javascript'>
    $(function () {
        setInterval(reflush, 1000)
    })
    function reflush() {
        $.getJSON('./get', function(json) {
            //解决原生js更新数据产生页面闪烁的问题
            $('#tmp')[0].innerHTML = json.voltage
            $('#voltage').html($('#tmp').html())

            $('#tmp')[0].innerHTML = json.current
            $('#current').html($('#tmp').html())

            $('#tmp')[0].innerHTML = json.power
            $('#power').html($('#tmp').html())

            $('#tmp')[0].innerHTML = json.serial
            $('#serial').html($('#tmp').html())
        })
    }
</script>
    <label id='tmp' hidden></label>
    <label>voltage: </label>
    <b>
        <i>
            <label id='voltage'>0.00</label>
        </i>
    </b>
    V<br/>
    <label>current: </label>
    <b>
        <i>
            <label id='current'>0.00</label>
        </i>
    </b>
    A<br/>
    <label>power: </label>
    <b>
        <i>
            <label id='power'>0.00</label>
        </i>
    </b>
    W<br/>
    <label>serial-number: </label>
    <b>
        <i>
            <label id='serial'>0.00</label>
        </i>
    </b>
</body>
```

首先导入头文件，并定义全局变量server并定义对应的handler。在`handleRoot`中会直接将上述网页返回，而在`handleGet`中会将电压、电流、功率等数据通过json的形式返回给客户端。为了表示数据的实时性，还附带了一个序列号，序列号在网页上的变化，表示了数据是实时接收的。

```C
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

//Web Server
ESP8266WebServer server(8080);

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
```

然后在`setup()`函数中配置对应的handler。

```C
  server.on("/", HTTP_GET, handleRoot);
  server.on("/get", HTTP_GET, handleGet);
  server.begin();
```

在`loop()`函数中处理客户端的请求，其实也就是调用一个函数，在loop这个死循环中不停处理请求。

```C
  //server处理客户端的请求
  server.handleClient();
```

###  2.4.  采样

采样是通过arduino的一个ADS1115库来实现的，也是很简单。

首先导入需要的库，并实例化一些对象。

```C
#include <Adafruit_ADS1015.h>

// 电压电流测量
Adafruit_ADS1115 ads(0x48);
float voltage = 5.00;
float current = 1.11;
float power = voltage * current;
```

在`setup()`函数中开启ADS芯片，这个是很重要的。

```C
  ads.begin();
```

然后在`loop()`函数中，获取硬件电路中的ADS1115芯片四个采样脚的电压，进一步获得电压和电流值，0.1875是电压因数。

因为ADS采样脚的输入阻抗问题，测得的电压值和实际的电压值是有偏移的，不过是线性偏移。只要求两组采样值x和实际值y，然后通过y=ax+b来求得其系数。

```C
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
```

##  最后

最后所有的工作就完成了。

代码见[github](https://github.com/crazyStrome/vcsample)