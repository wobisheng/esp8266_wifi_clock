#include <LedControl.h>
#include <cmath>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Metro.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

ESP8266WebServer server( 80 );


LedControl lc = LedControl( 12, 14, 15, 4 );

WiFiUDP ntpUDP;

NTPClient timeClient( ntpUDP, "ntp1.aliyun.com", 60 * 60 * 8, 30 * 60 * 1000 );


Metro  heart   = Metro( 500 );
Metro checkclient = Metro( 20 );
Metro time_check  = Metro( 1000 );
Metro weather_check = Metro( 1200000 );

boolean wifistatus    = false;
boolean heartstatus   = false;
boolean user_login    = false;
boolean time_and_weather_init = false;

const String  my_key  = "";//高德开发平台key
const String  city  = "";//城市代码
const char  * host  = "restapi.amap.com";

int time_temp;

int NUMBER[10][8] =
{
  { 0x00, 0x07, 0x05, 0x05, 0x05, 0x07, 0x00, 0x0F },
  { 0x00, 0x02, 0x02, 0x02, 0x02, 0x02, 0x00, 0x0F },
  { 0x00, 0x07, 0x01, 0x07, 0x04, 0x07, 0x00, 0x0F },
  { 0x00, 0x07, 0x01, 0x07, 0x01, 0x07, 0x00, 0x0F },
  { 0x00, 0x05, 0x05, 0x07, 0x01, 0x01, 0x00, 0x0F },
  { 0x00, 0x07, 0x04, 0x07, 0x01, 0x07, 0x00, 0x0F },
  { 0x00, 0x07, 0x04, 0x07, 0x05, 0x07, 0x00, 0x0F },
  { 0x00, 0x07, 0x01, 0x01, 0x01, 0x01, 0x00, 0x0F },
  { 0x00, 0x07, 0x05, 0x07, 0x05, 0x07, 0x00, 0x0F },
  { 0x00, 0x07, 0x05, 0x07, 0x01, 0x01, 0x00, 0x0F },
};
int Weathericon[20][8] =
{
  { 0x08, 0x6A, 0x3E, 0xFC, 0x3F, 0x7C, 0x56, 0x10 },     /* 晴 */
  { 0x44, 0x66, 0xFF, 0xFF, 0x40, 0x12, 0x80, 0x09 },     /* 雨 */
  { 0x00, 0x7E, 0x00, 0x7E, 0x00, 0x7E, 0x00, 0x00 },     /* 雾 */
  { 0x00, 0x08, 0x5C, 0xFE, 0xFF, 0x00, 0x00, 0x00 },     /* 阴 */
  { 0x00, 0x04, 0x2E, 0x74, 0xFE, 0xFF, 0x00, 0x00 },     /* 多云 */
  { 0x18, 0x78, 0x03, 0x7F, 0x00, 0x7C, 0x0C, 0x00 },     /* 风 */
  { 0x04, 0x4E, 0xFF, 0xFF, 0x42, 0x17, 0x82, 0x10 },     /* 夹 */
  { 0x2A, 0x5D, 0x2A, 0xFF, 0x2A, 0x5D, 0x2A, 0x08 },     /* 雪 */
  { 0x19, 0xF8, 0x0B, 0xFF, 0x04, 0xFD, 0x0C, 0x01 },     /* 沙 */
  { 0xFF, 0x7F, 0x7E, 0x3E, 0x3C, 0x1C, 0x18, 0x08 },     /* 龙卷风 */
  { 0x01, 0xAE, 0xAA, 0xEA, 0x2A, 0x2E, 0x00, 0x00 },     /* 热 */
  { 0x02, 0x1C, 0x14, 0xD4, 0x14, 0x1C, 0x00, 0x00 },     /* 冷 */
  { 0x00, 0x1C, 0x14, 0x14, 0x04, 0x00, 0x04, 0x00 },     /* 未知 */
};
int initial[20][8] =
{
  { 0x5F, 0xE2, 0x42, 0x42, 0x62, 0xC2, 0x42, 0xC6 },
  { 0xFF, 0x44, 0xFE, 0x44, 0x44, 0x44, 0x44, 0x84 },
  { 0xAA, 0xA8, 0xAA, 0xAA, 0xAA, 0xAA, 0x52, 0x52 },
  { 0xFA, 0x80, 0x82, 0xFA, 0x82, 0x82, 0x82, 0x82 },
  { 0x90, 0x3E, 0xD8, 0x5E, 0x88, 0x5E, 0x88, 0x7F },
  { 0x44, 0xFF, 0x4A, 0x5F, 0x64, 0xDF, 0x4A, 0xCC },
  { 0x7E, 0x42, 0x44, 0x78, 0x50, 0x48, 0x44, 0x42 },
  { 0x00, 0x7E, 0x40, 0x40, 0x40, 0x40, 0x7E, 0x00 },
  { 0x7E, 0x7E, 0x60, 0x7E, 0x7E, 0x60, 0x60, 0x60 },
  { 0x18, 0x18, 0x66, 0x66, 0x7E, 0x7E, 0x66, 0x66 },
  { 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18 },
  { 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7E, 0x7E },
  { 0x0A, 0x7E, 0x48, 0x44, 0xF7, 0x94, 0xBA, 0xB1 },
  { 0x04, 0x04, 0x1F, 0xE9, 0x49, 0x69, 0xD1, 0x13 },
};
int heartbeat[2][8] =
{
  { 0x00, 0x66, 0xFF, 0xFF, 0xFF, 0x7E, 0x3C, 0x18 },
  { 0x00, 0x00, 0x24, 0x7E, 0x7E, 0x3C, 0x18, 0x00 },
};

void weather_to_screen( String &response )
{
  int temp    = 0;
  String  comp[30]  = { "晴", "0", "云", "4", "阴", "3", "风", "5", "霾", "2", "雨", "1", "夹", "6", "雪", "7", "沙", "8", "卷", "9", "雾", "2", "热", "10", "冷", "11", "未", "12" };//伪字典
  for ( int i = 0; i <= 27; i = i + 2 )
  {
    if ( response.indexOf( comp[i] ) != -1 )
    {
      temp = comp[i + 1].toInt();
      break;
    }
  }
  lc.clearDisplay( 1 );
  Serial.println( temp );
  for ( int i = 0; i <= 7; i++ )
  {
    lc.setRow( 1, i, Weathericon[temp][i] );
  }
}

void weather()
{
  WiFiClient client;
  client.connect( host, 80 );
  String request = (String) ("GET ") + "/v3/weather/" + "weatherInfo?" + "city=" + city + "&key=" + my_key + " HTTP/1.1\r\n" +
       "Content-Type: text/html;charset=utf-8\r\n" +
       "Host: " + host + "\n" +
       "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36 Edg/91.0.864.70" +
       "Connection: Keep Alive\r\n\r\n";
  client.print( request );
  String response = client.readString();
  weather_to_screen( response );
  client.stop();
}


void updata_to_screen( int &hours, int &minu )
{
  lc.clearDisplay( 2 );
  lc.clearDisplay( 3 );
  for ( int i = 0; i <= 7; i++ )
  {
    lc.setRow( 3, i, NUMBER[hours / 10][i] * pow( 2, 5 ) + NUMBER[hours - (hours / 10) * 10][i] * 2 );
    lc.setRow( 2, i, NUMBER[minu / 10][i] * pow( 2, 4 ) + NUMBER[minu - (minu / 10) * 10][i] );
  }
  lc.setRow( 3, 7, B11111111 );
}

void wifi_set()
{
  if ( user_login )
  {
    String  ssid    = server.arg( "ssid" );
    String  password  = server.arg( "password" );
    WiFi.begin( ssid, password );
    for ( int i = 0; i <= 3; i++ )
      lc.clearDisplay( i );
    for ( int i = 3; i >= 0; i-- )
    {
      lc.setRow( i, 7, B00000001 );
      delay( 500 );
    }
    delay( 5000 );
    if ( WiFi.status() != WL_CONNECTED )
    {
      for ( int i = 0; i <= 3; i++ )
        lc.clearDisplay( i );
      for ( int i = 0; i <= 7; i++ )
      {
        lc.setRow( 3, i, initial[8][i] );
        lc.setRow( 2, i, initial[9][i] );
        lc.setRow( 1, i, initial[10][i] );
        lc.setRow( 0, i, initial[11][i] );
      }
    }else  {
      lc.clearDisplay( 3 );
      lc.clearDisplay( 2 );
      for ( int i = 0; i <= 7; i++ )
      {
        lc.setRow( 3, i, initial[12][i] );
        lc.setRow( 2, i, initial[13][i] );
      }
      wifistatus = true;
      timeClient.begin();
      delay( 500 );
    }
  }
  user_login = true;
}

void setting()
{
  String  lumi    = server.arg( "lumi" );
  for ( int i = 0; i <= 3; i++ )
    { 
      lc.setIntensity( i, lumi.toInt() );
    }
    weather();
}

void handleRoot()
{
  if ( !wifistatus )
  {
    server.send( 200, "text/html", "<h1>请输入WIFI信息<form action=\"\" method=\"GET\"><br><input type=\"text\" name=\"ssid\"><br><input type=\"text\" name=\"password\"><br><input type=\"submit\" value=\"Submit\"></form></h1>" );
    wifi_set();
  }else  {
    server.send( 200, "text/html", "<h1>显示屏设置<form action=\"\" method=\"GET\"><br>亮度<input type=\"text\" name=\"lumi\">(0-8)<br><input type=\"submit\" value=\"Submit\"></form></h1>" );
    setting();
  }
}


void init_lattice()//显示屏初始化
{
  while ( !user_login )
  {
    for ( int i = 0; i <= 3; i++ )
    {
      lc.shutdown( i, false );
      lc.clearDisplay( i );
      lc.setIntensity( i, 1 );
    }
    for ( int i = 0; i <= 7; i++ )
    {
      lc.setRow( 3, i, initial[0][i] );
      lc.setRow( 2, i, initial[1][i] );
      lc.setRow( 1, i, initial[2][i] );
      lc.setRow( 0, i, initial[3][i] );
    }
    server.handleClient();
    delay( 1000 );
    for ( int i = 0; i <= 3; i++ )
      lc.clearDisplay( i );
    for ( int i = 0; i <= 7; i++ )
    {
      lc.setRow( 3, i, initial[4][i] );
      lc.setRow( 2, i, initial[5][i] );
      lc.setRow( 1, i, initial[6][i] );
      lc.setRow( 0, i, initial[7][i] );
    }
    server.handleClient();
    delay( 1000 );
  }
   for ( int i = 0; i <= 3; i++ )
      lc.clearDisplay( i );
     for ( int i = 0; i <= 7; i++ )
     {
      lc.setRow( 3, i, NUMBER[1][i]*pow(2,4)+NUMBER[9][i] );
      lc.setRow( 2, i, NUMBER[2][i]*pow(2,4)+NUMBER[1][i]);
      lc.setRow( 1, i, NUMBER[6][i]*pow(2,4)+NUMBER[8][i] );
      lc.setRow( 0, i, NUMBER[4][i]*pow(2,4)+NUMBER[1][i] );
     }
      lc.setRow( 3, 7, B00000000 );
      lc.setRow( 1, 7, B00000000 );
      lc.setRow( 2, 7, B00001000 );
      lc.setRow( 0, 7, B10001000 );
  while ( user_login && !wifistatus )
    server.handleClient();
}


void wifi_init()
{
  String apName = ("RC");
  WiFi.softAP( apName, "" );
  server.on( "/", handleRoot );
  server.begin();
}


void heart_beat()
{
  heartstatus = !heartstatus;
  lc.clearDisplay( 0 );
  for ( int i = 0; i <= 7; i++ )
  {
    lc.setRow( 0, i, heartbeat[heartstatus][i] );
  }
}


void clock_and_weather()//总控
{
  if ( heart.check() )
  {
    heart_beat();
  }
  if ( checkclient.check() )
  {
    server.handleClient();
  }
  if ( time_check.check() )
  {
    timeClient.update();
    int minu  = timeClient.getMinutes();
    int hour  = timeClient.getHours();
    if ( minu != time_temp )
      updata_to_screen( hour, minu );
    time_temp = minu;
  }
  if ( weather_check.check() )
    weather();
}


void setup()
{
  Serial.begin( 9600 );
  wifi_init();
  init_lattice();
  timeClient.update();
  int minu  = timeClient.getMinutes();
  int hour  = timeClient.getHours();
  updata_to_screen( hour, minu );
  weather();
}


void loop()
{
  clock_and_weather();
}
