#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

int interruptcenter=0;
hw_timer_t *timer=NULL;             //timer
void timerevent()
{
  interruptcenter++;
}


#include <WiFi.h>
const char* ssid = "jhk";
const char* password = "258369147";



DynamicJsonDocument doc(2048);        //分配内存空间？？
DynamicJsonDocument rx(1024);
long w , aq ;
long aqi;
int wight1 , wight2 , out , tem ;
int humi;




#include <PubSubClient.h>

const char* mqtt_server = "t.yoyolife.fun";  //CHANGE
const char* mqtt_username = "?";
const char* mqtt_password = "?";

WiFiClient espClient;

PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];

#define RELAY_PIN  5

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


                                                          //电机模块
#define PUL 21
#define DIR 22
#define ENA 23
#define xifen1 25
#define xifen2 26


void step_90_degrees ()
{
  for (int i = 0; i < 6400; i++)                    //800转90度，1600转180度（执行一次）
  {
    if(i%2==0)digitalWrite(PUL,HIGH);
    if(i%2!=0)digitalWrite(PUL,LOW);
    delay(1);
  }
  
}



void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] \r\n");
  String str;
  for (int i = 0; i < length; i++) {
    str += String((char)payload[i]);
    //Serial.print((char)payload[i]);
  }
  Serial.println();
  Serial.print(str);

  deserializeJson(rx,str);

  String mode=rx["target"];                                                          //接收数据处
  int swi=rx["value"];

  if(swi == 1){
    step_90_degrees();
    client.publish("ADDRESS","{\"led\":0}");
  }
}

//重新链接客户端
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("connected");
      // Once connected, publish an announcement...

      // ... and resubscribe
      client.subscribe("ADDRESS");                                      ///subscribe
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

//发送JSON数据
void sendjson ()
{
  char* msgo=(char*)malloc(sizeof(char)*100);
  assert(msgo);
  // int a=22,b=3,c=52;

  String skyoutput[]={"晴朗","晴朗","多云","多云","阴天","轻度雾霾","中度雾霾","重度雾霾","小雨","中雨","大雨","暴雨","雾天","小雪","中雪","大雪","暴雪","浮尘","沙尘","大风"};
  String aqizn[]={"优秀","良好","轻度","中度","重度"};

  int i=sprintf(msgo,"{\"temp\":%d,\"humi\":%d,\"lengtn\":%d,\"weather\":\"%s\",\"aqizn\":\"%s\",\"aqi\":%d,\"weight\":%d}",tem,humi,52,skyoutput[w],aqizn[aq],aqi,out/450);
  Serial.printf(msgo);

  //char *json = &msgo[0];

  //Serial.println(json);
  //boolean d = client.publish("ADDRESS","This is a message");
  delay(2000);

  boolean d=client.publish("ADDRESS",msgo);                      //01号设备号用来发送数据，02号口接收电机数据
  if(d)Serial.println("publish:success");
  else Serial.println("publish:default");
  delay(2000);
}



//天气检查模块
void weathercheck()
{
  float hum;
  String key="KEY";
  String location="LOCATION";   
  String url="https://api.caiyunapp.com/v2.4/"+key+"/"+location+"/realtime.json";

  String aqizh[]={"优","良","轻度污染","中度污染","重度污染"};
  String skyco[]={"CLEAR_DAY","CLEAR_NIGHT","PARTLY_CLOUDY_DAY","PARTLY_CLOUDY_NIGHT","CLOUDY","LIGHT_HAZE","MODERATE_HAZE","HEAVY_HAZE",
  "LIGHT_RAIN","MODERATE_RAIN","HEAVY_RAIN","STORM_RAIN","FOG","LIGHT_SNOW","MODERATE_SNOWHEAVY_SNOW","STORM_SNOW","DUST","SAND,WIND"};
  //char aa[]={'a'};
  HTTPClient http;
  http.begin(url);
  int httpcode=http.GET();
  if (httpcode>0)
  {
    Serial.printf("HTTP Get Code: %d\r\n",httpcode);

    if (httpcode == HTTP_CODE_OK)
    {
      String resbuff =http.getString();

      //Serial.println(resbuff);    //输出返回值

      deserializeJson(doc,resbuff);

      String skycon=doc["result"]["realtime"]["skycon"];
      String aqiz=doc["result"]["realtime"]["air_quality"]["description"]["usa"];
      int aqiL=doc["result"]["realtime"]["air_quality"]["aqi"]["usa"];
      tem=doc["result"]["realtime"]["temperature"];
      hum=doc["result"]["realtime"]["humidity"];
      humi=hum*100;
      aqi=aqiL;



      for (  int i = 0; i < 17; i++)
      {
        if(aqiz==aqizh[i])
        {
          aq=i;
        }
      }

      for (  int i = 0; i < 17; i++)
      {
        if(skycon==skyco[i])
        {
          w=i;
        }
      }
    }
  }
  else Serial.printf("HTTPERROR\r\n");
}

#define ADDO 19   
#define ADSK 18
//称重模块
unsigned long elewight ()
{
  unsigned long count;
  unsigned char i;
  int d;
  digitalWrite(ADDO,HIGH);
  delay(2);
  digitalWrite(ADSK,LOW);
  count=0;
  // Serial.printf("02");3
  
  while(digitalRead(ADDO)); //AD转换未结束则等待，否则开始读取
  // Serial.printf("01");
  for (i=0;i<24;i++)
  {
  digitalWrite(ADSK,HIGH); //PD_SCK 置高（发送脉冲）
  count=count<<1; //下降沿来时变量Count左移一位，右侧补零
  digitalWrite(ADSK,LOW); //PD_SCK 置低
  if(digitalRead(ADDO)) count++;
  }
  digitalWrite(ADSK,HIGH);
  count=count^0x800000;//第25个脉冲下降沿来时，转换数据
  delay(2);
  digitalWrite(ADSK,LOW);
  return(count);
  }



void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);       //串口初始化


  timer = timerBegin(0,80,true);
  timerAttachInterrupt(timer,&timerevent,true);
  timerAlarmWrite(timer,1000000,true);
  timerAlarmEnable(timer);

  pinMode(ADSK,OUTPUT);       //称重模块
  pinMode(ADDO,INPUT);
  delay(1000);
  wight1=elewight();

  pinMode(PUL,OUTPUT);        //电机模块初始化
  pinMode(DIR,OUTPUT);
  pinMode(ENA,OUTPUT);
  pinMode(xifen1,OUTPUT);
  pinMode(xifen2,OUTPUT);
  digitalWrite(xifen1,LOW);
  digitalWrite(xifen2,HIGH);
  digitalWrite(DIR,LOW);
  digitalWrite(ENA,LOW);


  setup_wifi();               //初始化WiFi
  
  weathercheck();             //初始化运行天气检查


  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}



void loop() {
  // put your main code here, to run repeatedly:
  int lo;
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  if(interruptcenter>3600)
  {
    weathercheck();
    interruptcenter=0;
  }
  
  out=elewight()-wight1;
  //Serial.printf("%d\r\n",out/448);
  
  if(interruptcenter%2==0)sendjson();
}

