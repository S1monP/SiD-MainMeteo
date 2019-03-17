/*
 * 
 MetOSMM Beta 4.2

  Добавлен бета тест ответчика ИИ
  автоматический перевод времени
  доработан тач скрин
*/
#include <genieArduino.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include "WiFiEsp.h"
#include "WiFiEspUdp.h"
#include <rtc_clock.h>
#include <DS3231.h>
#include <SFE_BMP180.h>
#include "DHT.h"
#include <SPI.h>
#include <SD.h>
#include <time.h>
#include "Timer.h"
#include <Scheduler.h>

#define GSMpow 28
#define GSMport Serial2
#define MAX_LUX 54612
#define MAX_NOS 54612.0
#define MAX_OS 54612.0
#define DHTPIN 31
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
#define TIMEZONE 2
#define RESETLINE 4
#define BVPin A10
#define BPPin A9
#define SOPin A11
#define BPLPin 53
#define SOLPin 49
#define RSPin 51
#define CSPin 52
#define GSMring 50 

byte symW[8] = {
0b00000,
0b11100,
0b11110,
0b01011,
0b01001,
0b01000,
0b01000,
0b01000
};

struct AllData{
  int hh,mm,ss,dow,dd,mon,yyyy; // время и дата
  float Volt;
  int Bat_lvl;
  float hum,temp;       // влажность температура от DHT22
  double p,p0;        // давление и температура ще BMP180
  double TV;
  uint16_t lux; // осв в люксах BH1750
  double alt;    //высота над ур моря
  double longi , lati; // долгота и широта
  int nos_napr;
  float nos_sil;  // напр и сила ветра
  float os_mm; // осадки в мм
  float radio;
  };

char ssid[] = "AP_123";            // your network SSID (name)
char pass[] = "123home123home123home";        // your network password
int status = WL_IDLE_STATUS;     // the Wifi radio's status

// Defines the time for the watchdog in ms
int watchdogTime = 16000;

RingBufferEsp buf(8);
byte strn=0;
char statusPT;

// Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
const unsigned long seventyYears = 2208988800UL;
unsigned long actual_unixtime;

static const uint32_t GPSBaud = 9600; 
String s_tl = "+380632753389";
String gsm_bal, sms_text, sms_num;
int sms_kol;
String GSM_Net;
int Time_Zone;
time_t tsd_epot,cur_epot, wsd_epot, his_epot, zsd_epot;
tm tim;
tm* ptim;
int Tren_den=0;
float Tos_den=0.0;
int Zren_den=0;
float Zos_den=0.0;
int on_ring=0;
uint32_t reset_status;

AllData msd, tsd, hsd,wsd,zsd;
boolean isPost=false;
String wrt1, wrt2;
int wrt=1;
String bufPost, wrtSD, flSD;
boolean LCD_change=TRUE;
int LCD_mode=0;
int LCD_timer;
int cur_hh,cur_mm, cur_ss,cur_dow,cur_dd, cur_mon, cur_yyyy;
int his_dd, his_mon, his_yyyy, his_dow;
int prevt_hh=0,prevt_mm=0, prevt_ss=0,prevt_dow=0,prevt_dd=0, prevt_mon=0, prevt_yyyy=0;
int InputV , InputB , InputS , Bat_lvl;
float Volt;
int Prev_ss1 =0;
int Prev_mm1=0;
int genie_F0S2_mode=0;
String genie_F0S3_text="Нет данных", genie_F0S4_text="               Нет данных",genie_F0S5_text,genie_F0S6_text, genie_F0S7_text;
String genie_F0S8_text,genie_F0S9_text, genie_F0S10_text;

File myWFile , myRFile, logFile;
IPAddress my_IP(192,168,0,170);
char Google_IP[]="8,8,8,8";
DS3231 clock_DS3231;
RTCDateTime dt,reset_dt;
RTC_clock rtc_clock(XTAL);
WiFiEspServer server(80);
WiFiEspClient client;
DHT dht(DHTPIN, DHTTYPE);
SFE_BMP180 pressure;
Genie genie;
LiquidCrystal_I2C lcd(0x27, 20, 4);
Timer LCD_switch;

//прерыванме при звонке
void ri(){
  on_ring=1;
//  SerialUSB.println("Kall");
//  SerialUSB.print("Set ");  SerialUSB.println(on_ring);
//  delay(100);
}

//Название напр из град
String vet_nap(int ddegr){
  int ind;
  String napr[]={"N","NE","E","SE","S","SW","W","NW"};  
  for(ind =1 ; ind<=7 ; ind++){
  int im=(450*ind)-225;
  int ix=(450*ind)+225;
  if((ddegr*10>=im)&&(ddegr*10<=ix)){
       break;
   }         
  }
  if((ddegr*10>=3375)||( ddegr*10<=225)){
    ind=0; 
  }
  return napr[ind];
}

//nomer напр из град
int vet_nap_n(int ddegr){
  int ind;
  for(ind =1 ; ind<=7 ; ind++){
  int im=(450*ind)-225;
  int ix=(450*ind)+225;
  if((ddegr*10>=im)&&(ddegr*10<=ix)){
       break;
   }         
  }
  if((ddegr*10>=3375)||( ddegr*10<=225)){
    ind=0; 
  }
  return ind;
}

// конверсия double (float) в строку
String DtoS(double a, byte dig) {
  double b,c;
  double* pc =&c;
  String s="";
  long bef;
  int i,digit;
  b=modf(a,pc);
  bef = c;
  s+=String(bef)+String(".");
  if (b<0) {b=-b;};
  for (i = 0; i < dig; i++) {
         b *= 10.0; 
         digit = (int) b;
         b = b - (float) digit;
     s+= String(digit);
     }
  return s;  
}

// преобразование структуры всех данных в строку
String con_DtoS(AllData d){
  String buf; 
  String s = "{";
  s +=String (d.hh); s+=";";
  s +=String (d.mm); s+=";";
  s +=String (d.ss); s+=";";
  s +=String (d.dow); s+=";";
  s +=String (d.dd); s+=";";
  s +=String (d.mon); s+=";";
  s +=String (d.yyyy); s+=";";
  buf = DtoS(d.Volt, 2); s += buf; s+=";";
  s +=String (d.Bat_lvl); s+=";";
  buf = DtoS(d.hum,3); s += buf; s+=";";
  buf = DtoS(d.temp,3); s += buf; s+=";";
  buf = DtoS(d.p,3); s += buf; s+=";";
  buf = DtoS(d.p0,3); s += buf; s+=";";
  buf = DtoS(d.TV,3); s += buf; s+=";";
  s +=String (d.lux); s+=";";
  buf = DtoS(d.alt,2); s += buf; s+=";";
  buf = DtoS(d.longi,7); s += buf; s+=";";
  buf = DtoS(d.lati,7); s += buf; s+=";";
  s +=String (d.nos_napr); s+=";"; 
  buf = DtoS(d.nos_sil,3); s += buf; s+=";"; 
  buf = DtoS(d.os_mm,2); s += buf; s+=";"; 
  buf = DtoS(d.radio,3); s += buf; s+=";"; 
  s +="}"; 
  
  return s;
}


//преобразование строки в массив данных
AllData StoD(String s){ 
 AllData d;
 int i=s.indexOf('{')+1; int k=s.indexOf(';',i); String ss=s.substring(i,k); d.hh=ss.toInt();
 i=k+1; k=s.indexOf(';',i); ss=s.substring(i,k); d.mm=ss.toInt();
 i=k+1; k=s.indexOf(';',i); ss=s.substring(i,k); d.ss=ss.toInt();
 i=k+1; k=s.indexOf(';',i); ss=s.substring(i,k); d.dow=ss.toInt();
 i=k+1; k=s.indexOf(';',i); ss=s.substring(i,k); d.dd=ss.toInt();
 i=k+1; k=s.indexOf(';',i); ss=s.substring(i,k); d.mon=ss.toInt();
 i=k+1; k=s.indexOf(';',i); ss=s.substring(i,k); d.yyyy=ss.toInt();
 i=k+1; k=s.indexOf(';',i); ss=s.substring(i,k); d.Volt=ss.toFloat();
 i=k+1; k=s.indexOf(';',i); ss=s.substring(i,k); d.Bat_lvl=ss.toInt(); 
 i=k+1; k=s.indexOf(';',i); ss=s.substring(i,k); d.hum=ss.toFloat();
 i=k+1; k=s.indexOf(';',i); ss=s.substring(i,k); d.temp=ss.toFloat();
 i=k+1; k=s.indexOf(';',i); ss=s.substring(i,k); d.p=ss.toFloat();
 i=k+1; k=s.indexOf(';',i); ss=s.substring(i,k); d.p0=ss.toFloat();
 i=k+1; k=s.indexOf(';',i); ss=s.substring(i,k); d.TV=ss.toFloat();
 i=k+1; k=s.indexOf(';',i); ss=s.substring(i,k); d.lux=ss.toInt(); 
 i=k+1; k=s.indexOf(';',i); ss=s.substring(i,k); d.alt=ss.toFloat();
 i=k+1; k=s.indexOf(';',i); ss=s.substring(i,k); d.longi=ss.toFloat();
 i=k+1; k=s.indexOf(';',i); ss=s.substring(i,k); d.lati=ss.toFloat();
 i=k+1; k=s.indexOf(';',i); ss=s.substring(i,k); d.nos_napr=ss.toInt(); 
 i=k+1; k=s.indexOf(';',i); ss=s.substring(i,k); d.nos_sil=ss.toFloat();
 i=k+1; k=s.indexOf(';',i);
 if(k==-1){
   d.os_mm=0.0;
 } else {
   ss=s.substring(i,k); d.os_mm=ss.toFloat();}
 i=k+1; k=s.indexOf(';',i);
 if(k==-1){
   d.radio=0.0;
 } else {
   ss=s.substring(i,k); d.radio=ss.toFloat();}
 return d; 
}


// преобразование времени в строку
String Tstr(int h, int m, int sec){
  String s= "";
  if (h<10){ s+="0"; }
  s += String (h);
  s+=":";
  
   if (m<10){ s+="0"; }
  s += String (m);
  s+=":";
  
   if (sec<10){ s+="0"; }
  s += String (sec);
  return s;
}

// преобразование даты в строку
String Dstr(int den, int chis, int mes, int god){
char* daynames[]={"Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"};
  String s= "";
  s+=String (daynames[den]);
  s+=":";
  if (chis<10){ s+="0"; }
  s += String (chis);
  s+=".";
  if (mes<10){ s+="0"; }
  s += String (mes);
  s+=".";
  god=god-2000;
   if (god<10){ s+="0"; }
  s += String (god);
  return s;
}

// преобразование даты в имя файла 
String SDdat(int chis, int mes, int god){
  String s= "";
  if (chis<10){ s+="0"; }
  s += String (chis); 
  if (mes<10){ s+="0"; }
  s += String (mes);
  god=god-2000;
   if (god<10){ s+="0"; }
  s += String (god);
  return s;
}

//Чтение из буфера GSM
String GSM_buf()
{
    do { delay(50) ; watchdogReset(); } while (GSMport.available()==0);
    String s="";
    for (int k=0; k<10000; k++) { watchdogReset();
          char c = GSMport.read();
//          Serial.write(c);
          delayMicroseconds(50);
          if ((byte)c!=255) s+=c;
          if (s.endsWith("OK\r\n")) break;
       }
//   Serial.println(s);
//   Serial.println("==============");
   
   return s;
}

// запуск GSM
String GSM_start(){
  
    GSMport.begin(9600);          
    GSMport.println("AT");
    delay(200);
    String r=GSM_buf();
    if ( r.indexOf("OK")==-1) {
      pinMode(GSMpow, OUTPUT);
      digitalWrite(GSMpow, HIGH);
      delay(1000);
      digitalWrite(GSMpow, LOW);
      delay(5000);
       }
    GSMport.println("AT"); r=GSM_buf();   
    GSMport.println("ATE0"); r=GSM_buf();   
    GSMport.println("AT+CMGF=1");  r=GSM_buf(); 
    GSMport.println("AT+DDET=1");  r=GSM_buf();
    delay(500); 
    for (int i=0 ; i<=20; i++){
      GSMport.println("AT+COPS?");
      r=GSM_buf();
      if ( r.indexOf("0,0,")!=-1) { break;}
      delay(1000);
    }
        int opp=r.indexOf("\"")+1;
    int opk=r.lastIndexOf("\"");
    String op=r.substring(opp,opk);
    GSMport.println("AT+CPMS=\"SM\"");  r=GSM_buf(); 
    return op;
 }

  // отправка смс  GSM
void GSM_ssms(String tl, String txt){
        GSMport.println(String("AT+CMGS=\"")+tl+String("\""));
        delay(100);
        String r=GSM_buf();
        GSMport.println(txt+String (char(26)));
 }
  
  // получение силы  GSM
int GSM_sign(){
        GSMport.println("AT+CSQ");
          String r=GSM_buf();
          int tn=r.lastIndexOf("CSQ")+4;
          int tk=r.indexOf(",");     
          String s=r.substring(tn,tk);         
          int sn=s.toInt();    
          return sn;  
 }

  // получение баланса счета
String GSM_money(){
         GSMport.println("AT+CUSD=1,\"*111#\"");
         String r=GSM_buf();
         delay(1000);
         r=GSM_buf();
         int tn = r.indexOf("\"Balans")+1; 
        int tk = r.indexOf(",",tn); 
        String s=r.substring(tn,tk);
         return s;  
 }

 //КОЛВО смс
 int GSM_csms(){
        GSMport.println("AT+CPMS?");
        String r=GSM_buf(); //Serial.println(r);
        int tn = r.indexOf("\"SM\",")+5; 
        int tk = r.indexOf(",",tn); 
        String s=r.substring(tn,tk);
        int sn=s.toInt(); 
        return sn;  
}

// ЧТЕНИЕ смс №
String GSM_rsms(int nom_sms , String *tel_nom){
        GSMport.println("AT+CMGR="+String(nom_sms));
        String r=GSM_buf(); //Serial.println(r);
        String s,sn;
        int tn = r.indexOf("+CMGR:"); 
        if(tn==-1){
           s="";
           sn="";
        } else {
            tn = r.indexOf("\"",tn); 
            tn = r.indexOf("\"",tn+1); 
            tn = r.indexOf("\"",tn+1); 
        int tk = r.indexOf("\"",tn+1); 
            tn+=1;
            sn=r.substring(tn,tk); 
            tn = r.indexOf("\r\n",tn)+2; 
            tk = r.indexOf("\r\n",tn); 
            s=r.substring(tn,tk);};
        *tel_nom=sn;
        return s;
}

// Удаление смс №
void GSM_dsms(int nom_sms){
        GSMport.println("AT+CMGD="+String(nom_sms));
        String r=GSM_buf(); //  Serial.println(r);
}

// сет интернал тиме from www
  void TimeSET2_www()
  {
  String ans;
  for(int j=1; j<=10; j++){
  for(int i=1; i<=10; i++){ watchdogReset();
      client.connect("api.timezonedb.com", 80);
      if(client.connected()){
        break;
      }
   }
  client.println("GET /v2/get-time-zone?key=4JZ68AWE4A9D&format=json&by=zone&zone=Europe/Kiev HTTP/1.1");
  client.println("Host: api.timezonedb.com");
  client.println("Connection: close");
  client.println(); 
  long _startMillis = millis();
  while (!client.available() and (millis()-_startMillis < 2000))
  {
  };
  ans="";
  if (client.available()) {
       int av_char=client.available();
       char c;
       for (int k=0; k<av_char; k++) {
        c=client.read();  // Serial.println(c); 
        ans+=String(c);
       }
  }
  client.stop();
//  SerialUSB.println(ans);
  int tpos=ans.indexOf("timestamp");
  if( tpos!=-1){
      tpos+=11;
      int tpose=ans.indexOf(",", tpos);
      String time_www= ans.substring(tpos, tpose);
      long e_time_www=time_www.toInt();
      rtc_clock.set_clock( e_time_www );
      clock_DS3231.setDateTime(e_time_www-3600); 
      break;
  }
  }
  }


// посылает начальника веб страницу
void sendmain_page(WiFiEspClient client)
{
  // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
  // and a content-type so the client knows what's coming, then a blank line:
  client.print(
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n"  // the connection will be closed after completion of the response
            "Refresh: 60\r\n"        
            "\r\n");
          client.print("<!DOCTYPE HTML>\r\n");
          client.print("<html><head><meta charset=\"utf-8\"><title>SiD</title></head><body>\r\n");
          client.print("<p align=\"center\"><font size=\"+2\" color=\"cornflowerblue\">");
          client.print("<marquee behavior=\"scroll\" direction=\"left\">");
          if(tsd.yyyy!=0){
          client.print("Одесса центр: "); client.print(tsd.temp); client.print (" C* "); client.print(tsd.hum); client.print (" % &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp;"); 
          }
          client.print("Дома: "); client.print(msd.temp); client.print (" C* "); client.print(msd.hum); client.print (" %  &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp;"); 
          if(zsd.yyyy!=0){
          client.print("Одесса завод: "); client.print(zsd.temp); client.print (" C* "); client.print(zsd.hum); client.print (" % &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp;"); 
          }
          if(wsd.yyyy!=0){
          client.print("Походная: "); client.print(wsd.temp); client.print (" C* "); client.print(wsd.hum); client.print (" % &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp;"); 
          }
          client.print("</marquee>");
          client.print ("</font></p>\r\n");
          client.print("<center><font size=\"+4\" color=\"chartreuse\">SiD meteo stations</font></center>\r\n");
          client.print("<center><font color=\"red\">");
          client.print(Tstr(cur_hh,cur_mm,cur_ss));  client.print(" ");
          client.print(Dstr(cur_dow,cur_dd,cur_mon,cur_yyyy));
          client.println("</font></center><br>");        
          client.println("<br>");
          client.println("<center>Перейти на <a href=\"/H\">главную</a> метеостанцию<br>");
          client.println("Перейти на <a href=\"/L\">городскую</a> метеостанцию<br>");
          client.println("Перейти на <a href=\"/F\">заводскую</a> метеостанцию<br>");
          client.println("Перейти на <a href=\"/B\">походную</a> метеостанцию<br>");
          client.println("<br>");
          client.println("Перейти на <a href=\"/J\">истории</a> страницу<br><br>");
          client.println("Перейти на <a href=\"/I\">служебную</a> страницу</center><br>");
          client.print("<br>\r\n");
          client.print("</body></html>\r\n");
}

// посылает истории страницу
void sendhis_met_page(WiFiEspClient client)
{
  // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
  // and a content-type so the client knows what's coming, then a blank line:
  client.print(
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n"  // the connection will be closed after completion of the response
            "Refresh: 60\r\n"        
            "\r\n");
          client.print("<!DOCTYPE HTML>\r\n");
          client.print("<html><head><meta charset=\"utf-8\"><title>SiD</title></head><body>\r\n");
          client.println("Возврат к <a href=\"/S\">главному</a> меню<br>");
          client.print("<center><font size=\"+4\" color=\"chartreuse\">SiD meteo stations history</font></center>\r\n");
          client.print("<center><font color=\"red\">");
          client.print(Tstr(cur_hh,cur_mm,cur_ss));  client.print(" ");
          client.print(Dstr(cur_dow,cur_dd,cur_mon,cur_yyyy));
          client.println("</font></center><br>");        
          client.println("<br>");
          
          client.println("<center>Перейти на историю главной метеостанции за <a href=\"/A\">сегодня</a> <a href=\"/E\">5 дней</a> <br>");
          client.println("<center>Перейти на историю городской метеостанции за <a href=\"/C\">сегодня</a> <a href=\"/K\">5 дней</a> <br>");
          client.println("<center>Перейти на историю заводской метеостанции за <a href=\"/G\">сегодня</a> <a href=\"/M\">5 дней</a> <br>");
          client.println("<center>Перейти на историю походной метеостанции за <a href=\"/D\">сегодня</a> <a href=\"/N\">5 дней</a> <br>");

          client.print("<br>\r\n");
          client.print("</body></html>\r\n");
}

// посылает веб страницу
void sendmain_met_page(WiFiEspClient client)
{
  // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
  // and a content-type so the client knows what's coming, then a blank line:
  client.print(
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n"  // the connection will be closed after completion of the response
            "Refresh: 60\r\n"        // refresh the page automatically every 20 sec
            "\r\n");
          client.print("<!DOCTYPE HTML>\r\n");
          client.print("<html><head><meta charset=\"utf-8\"><title>SiD</title></head><body>\r\n");
          client.println("Возврат к <a href=\"/S\">главному</a> меню<br>");       
          client.print("<center><font size=\"+4\" color=\"chartreuse\">SiD MAIN meteo station</font></center><br>\r\n");
          client.print("<center><font color=\"red\">");
          client.print(Tstr(msd.hh,msd.mm,msd.ss));  client.print(" ");
          client.print(Dstr(msd.dow,msd.dd,msd.mon,msd.yyyy));
          client.println("</font></center><br>"); 
          client.println("<br> <br>");
          client.print("<center><font size=\"+5\" color=\"cornflowerblue\">");
          client.print(msd.hum);
          client.print (" % &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp");
          client.print(msd.temp);
          client.print (" &deg;C &nbsp; &nbsp; &nbsp; &nbsp; &nbsp");
          client.print(msd.p0);
          client.print (" mm.Hg");
          client.println("</font></center><br>");
          client.print("<br>\r\n");
          client.println("<p align=\"center\" font color=\"orange\">-<a href=\"/A\">История</a>-</font></p><br>");
          client.println("<p align=\"right\" font color=\"orange\"><a href=\"/L\">городская</a> &equiv;&rArr;</font></p><br>");
          client.print("</body></html>\r\n");
}

// посылает веб страницу гор мет станции
void sendtown_met_page(WiFiEspClient client)
{
  // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
  // and a content-type so the client knows what's coming, then a blank line:
  client.print(
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n"  // the connection will be closed after completion of the response
            "Refresh: 60\r\n"        // refresh the page automatically every 20 sec
            "\r\n");
          client.print("<!DOCTYPE HTML>\r\n");
          client.print("<html><head><meta charset=\"utf-8\"><title>SiD</title></head><body>\r\n");
          client.println("Возврат к <a href=\"/S\">главному</a> меню<br>");       
          client.print("<center><font size=\"+4\" color=\"chartreuse\">SiD TOWN meteo station</font></center><br>\r\n");
          if (tsd.yyyy==0) {
          client.print("<br><center><font size=\"+5\" color=\"red\">"); 
          client.print("Нет данных!"); 
          client.println("</font></center><br>");
          client.print("<br>\r\n"); 
          }
          else {
          client.print("<center><font color=\"red\">");
          client.print(Tstr(tsd.hh,tsd.mm,tsd.ss));  client.print(" ");
          client.print(Dstr(tsd.dow,tsd.dd,tsd.mon,tsd.yyyy));
          client.println("</font></center><br>"); 
          client.println("<br> <br>");
          client.print("<center><font size=\"+5\" color=\"cornflowerblue\">");
          client.print(tsd.hum);
          client.print (" % &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp");
          client.print(tsd.temp);
          client.print (" &deg;C &nbsp; &nbsp; &nbsp; &nbsp; &nbsp");
          client.print(tsd.p0);
          client.print (" mm.Hg");
          client.print("<br><br>\r\n");
          client.print(tsd.lux);
          client.print (" lux &nbsp; &nbsp; &nbsp; &nbsp; &nbsp");
          client.print(tsd.nos_sil);
          client.print (" m/s ");          
          client.print(vet_nap(tsd.nos_napr));
          client.print ("&nbsp; &nbsp; &nbsp; &nbsp; &nbsp");
          client.print(tsd.os_mm);
          client.print (" mm");
          client.print ("&nbsp; &nbsp; &nbsp; &nbsp; &nbsp");
          client.print(tsd.radio);
          client.print (" µSv/h");          
          client.println("</font></center><br>");
          client.print("<br>\r\n");
          }
          client.println("<p align=\"center\" font color=\"orange\">-<a href=\"/C\">История</a>-</font></p><br>");
          client.print("<p align=\"center\" font color=\"orange\">&lArr;&equiv;<a href=\"/H\">главная</a>");
          client.print("&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp");
          client.print("<a href=\"/F\">заводская</a> &equiv;&rArr;</font></p><br>");
          client.print("</body></html>\r\n");
}

// посылает веб страницу зав мет станции
void sendzavod_met_page(WiFiEspClient client)
{
  // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
  // and a content-type so the client knows what's coming, then a blank line:
  client.print(
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n"  // the connection will be closed after completion of the response
            "Refresh: 60\r\n"        // refresh the page automatically every 20 sec
            "\r\n");
          client.print("<!DOCTYPE HTML>\r\n");
          client.print("<html><head><meta charset=\"utf-8\"><title>SiD</title></head><body>\r\n");
          client.println("Возврат к <a href=\"/S\">главному</a> меню<br>");       
          client.print("<center><font size=\"+4\" color=\"chartreuse\">SiD ZAVOD meteo station</font></center><br>\r\n");
          if (zsd.yyyy==0) {
          client.print("<br><center><font size=\"+5\" color=\"red\">"); 
          client.print("Нет данных!"); 
          client.println("</font></center><br>");
          client.print("<br>\r\n"); 
          }
          else {
          client.print("<center><font color=\"red\">");
          client.print(Tstr(zsd.hh,zsd.mm,zsd.ss));  client.print(" ");
          client.print(Dstr(zsd.dow,zsd.dd,zsd.mon,zsd.yyyy));
          client.println("</font></center><br>"); 
          client.println("<br> <br>");
          client.print("<center><font size=\"+5\" color=\"cornflowerblue\">");
          client.print(zsd.hum);
          client.print (" % &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp");
          client.print(zsd.temp);
          client.print (" &deg;C &nbsp; &nbsp; &nbsp; &nbsp; &nbsp");
          client.print(zsd.p0);
          client.print (" mm.Hg");
          client.print("<br><br>\r\n");
          client.print(zsd.lux);
          client.print (" lux &nbsp; &nbsp; &nbsp; &nbsp; &nbsp");
          client.print(zsd.nos_sil);
          client.print (" m/s ");          
          client.print(vet_nap(zsd.nos_napr));
          client.print ("&nbsp; &nbsp; &nbsp; &nbsp; &nbsp");
          client.print(zsd.os_mm);
          client.print (" mm");
          client.print ("&nbsp; &nbsp; &nbsp; &nbsp; &nbsp");
          client.print(zsd.radio);
          client.print (" µSv/h");     
          client.println("</font></center><br>");
          client.print("<br>\r\n");
          }
          client.println("<p align=\"center\" font color=\"orange\">-<a href=\"/G\">История</a>-</font></p><br>");
          client.print("<p align=\"center\" font color=\"orange\">&lArr;&equiv;<a href=\"/L\">городская</a>");
          client.print("&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp");
          client.print("<a href=\"/B\">походная</a> &equiv;&rArr;</font></p><br>");
          client.print("</body></html>\r\n");
}

// посылает веб страницу история мет станции
void send_his1_met_page(WiFiEspClient client, String MName)
{
           int hour_offset;
           float tot_os_mm=0.0;
           float tot_radio=0.0;
           client.print(
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n"  // the connection will be closed after completion of the response
            "\r\n");
          client.print("<!DOCTYPE HTML>\r\n");
          client.print("<html><head><meta charset=\"utf-8\"><title>SiD</title></head><body>\r\n");
          client.println("Возврат к <a href=\"/S\">главному</a> меню<br>");       
          client.print("<center><font size=\"+4\" color=\"chartreuse\">SiD ");
          if(MName=="MT"){ client.print("TOWN "); hour_offset=2;}
          if(MName=="MZ"){ client.print("ZAVOD "); hour_offset=3;}
          client.println("meteo station history</font></center><br>\r\n");
          client.println("<br><center><table border=\"1\">");
          client.println("<tr><td>");
          client.println(Dstr(cur_dow,cur_dd,cur_mon,cur_yyyy));
          client.println("</td><td>Температура</td><td>Влажность</td><td>Давление</td><td>Освещ.</td><td>Ветер</td><td>Осадки</td><td>Радиация</td></tr>");
            flSD=String("/"+MName+"/")+SDdat(cur_dd, cur_mon, cur_yyyy)+String(".")+MName;
            myRFile = SD.open(flSD);
            wrtSD="";
            while (myRFile.available()) {
            char c=myRFile.read(); 
            wrtSD+=c;
            if(wrtSD.endsWith("}")){
              hsd=StoD(wrtSD);
              tot_os_mm=tot_os_mm+hsd.os_mm;
              tot_radio=tot_radio+hsd.radio;
              wrtSD="";
                if(hsd.mm==hour_offset){
                client.print("<tr><td>");
                client.print(Tstr(hsd.hh,hsd.mm,hsd.ss));
                client.print("</td><td>");
                client.print(hsd.temp);
                client.print("</td><td>");
                client.print(hsd.hum); 
                client.print("</td><td>");
                client.print(hsd.p0);               
                client.print("</td><td>");
                client.print(hsd.lux);
                client.print("</td><td>");
                client.print(hsd.nos_sil); 
                client.print(" ");
                client.print(vet_nap(hsd.nos_napr));                                              
                client.print("</td><td>");
                client.print(hsd.os_mm);                                                                
                client.println("</td><td>"); 
                client.print(hsd.radio);                                                                
                client.println("</td></tr>");   
                }             
            }
            }             

            myRFile.close();
                client.print("<tr><td>");
                client.print("Всего:");
                client.print("</td><td>");
                client.print("</td><td>");
                client.print("</td><td>");              
                client.print("</td><td>");
                client.print("</td><td>");                                            
                client.print("</td><td>");
                client.print(tot_os_mm);                                                                
                client.println("</td><td>"); 
                client.print(tot_radio/6.0
                );                                                                
                client.println("</td></tr>");      
          client.println("</table></center>");
          client.print("<br>\r\n");
          client.println("<p align=\"center\" font color=\red\">-<a href=\"");
          if(MName=="MT"){ client.print("/L");}
          if(MName=="MZ"){ client.print("/F");}
          client.println("\">Назад</a> к метеостанции - <a href=\"/J\">Назад</a> к истории -</font></p><br>");
          client.print("<p align=\"center\" font color=\"orange\">&lArr;&equiv;");
          if(MName=="MT"){ client.print("<a href=\"/A\">главная</a>");}
          if(MName=="MZ"){ client.print("<a href=\"/C\">городская</a>");}
          client.print("&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp");
          if(MName=="MT"){ client.print("<a href=\"/G\">заводская</a>");}
          if(MName=="MZ"){ client.print("<a href=\"/D\">походная</a>");}
          client.print(" &equiv;&rArr;</font></p><br>");
          client.print("</body></html>\r\n");
}

// посылает веб страницу походной мет станции
void sendworld_met_page(WiFiEspClient client)
{

          client.print(
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n"  // the connection will be closed after completion of the response
            "Refresh: 60\r\n"        // refresh the page automatically every 20 sec
            "\r\n");
          client.print("<!DOCTYPE HTML>\r\n");
          client.print("<html><head><meta charset=\"utf-8\"><title>SiD</title></head><body>\r\n");
          client.println("Возврат к <a href=\"/S\">главному</a> меню<br>");       
          client.print("<center><font size=\"+4\" color=\"chartreuse\">SiD WoRLD meteo station</font></center><br>\r\n");
          if (wsd.yyyy==0) {
          client.print("<br><center><font size=\"+5\" color=\"red\">"); 
          client.print("Нет данных!"); 
          client.println("</font></center><br>");
          client.print("<br>\r\n"); 
          }
          else {
          client.print("<center><font color=\"red\">");
          client.print(Tstr(wsd.hh,wsd.mm,wsd.ss));  client.print(" ");
          client.print(Dstr(wsd.dow,wsd.dd,wsd.mon,wsd.yyyy));
          client.println("</font></center><br>"); 
          client.println("<br> <br>");
          client.print("<center><font size=\"+5\" color=\"cornflowerblue\">");
          client.print(wsd.hum);
          client.print (" % &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp");
          client.print(wsd.temp);
          client.print (" &deg;C &nbsp; &nbsp; &nbsp; &nbsp; &nbsp");
          client.print(wsd.p0);
          client.print (" mm.Hg");
          client.print("<br><br>\r\n");
          client.print(wsd.lux);
          client.print (" lux");
          client.println("</font></center><br>");
          client.print("<br>\r\n");
          }
          client.println("<p align=\"center\" font color=\"orange\">-<a href=\"/D\">История</a>-</font></p><br>");
          client.println("<p align=\"left\" font color=\"orange\">&lArr;&equiv;<a href=\"/F\">заводская</a></font></p><br>");
          client.print("</body></html>\r\n");
}

// посылает веб страницу история походной мет станции
void sendworld_his1_met_page(WiFiEspClient client)
{
  // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
  // and a content-type so the client knows what's coming, then a blank line:
  client.print(
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n"  // the connection will be closed after completion of the response
            "\r\n");
          client.print("<!DOCTYPE HTML>\r\n");
          client.print("<html><head><meta charset=\"utf-8\"><title>SiD</title></head><body>\r\n");
          client.println("Возврат к <a href=\"/S\">главному</a> меню<br>");       
          client.print("<center><font size=\"+4\" color=\"chartreuse\">SiD WoRLD meteo station history</font></center><br>\r\n");
          client.println("<br><center><table border=\"1\">");
          client.println("<tr><td>");
          client.println(Dstr(cur_dow,cur_dd,cur_mon,cur_yyyy));
          client.println("</td><td>Температура</td><td>Влажность</td><td>Давление</td><td>Освещённость</td></tr>");
            flSD=String("/WT/")+SDdat(cur_dd, cur_mon, cur_yyyy)+String(".WT"); 
            myRFile = SD.open(flSD);
            wrtSD="";
            while (myRFile.available()) {
            char c=myRFile.read(); 
            wrtSD+=c;
            if(wrtSD.endsWith("}")){ 
              hsd=StoD(wrtSD); 
              wrtSD="";
                if(hsd.mm==0){
                client.print("<tr><td>");
                client.print(Tstr(hsd.hh,hsd.mm,hsd.ss));
                client.print("</td><td>");
                client.print(hsd.temp);
                client.print("</td><td>");
                client.print(hsd.hum); 
                client.print("</td><td>");
                client.print(hsd.p0);               
                client.print("</td><td>");
                client.print(hsd.lux);                                                              
                client.println("</td></tr>");  
                }             
            }
            }             
            myRFile.close(); 
          client.println("</table></center>");
          client.print("<br>\r\n");
          client.println("<p align=\"center\" font color=\red\">-<a href=\"/B\">Назад</a> к метеостанции - <a href=\"/J\">Назад</a> к истории -</font></p><br>");
          client.println("<p align=\"left\" font color=\"orange\">&lArr;&equiv;<a href=\"/G\">заводская</a></font></p><br>");
          client.print("</body></html>\r\n");
}

// посылает веб страницу история глав мет станции
void sendmain_his1_met_page(WiFiEspClient client)
{
  // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
  // and a content-type so the client knows what's coming, then a blank line:
          client.print(
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n"  // the connection will be closed after completion of the response
            "\r\n");
          client.print("<!DOCTYPE HTML>\r\n");
          client.print("<html><head><meta charset=\"utf-8\"><title>SiD</title></head><body>\r\n");
          client.println("Возврат к <a href=\"/S\">главному</a> меню<br>");       
          client.print("<center><font size=\"+4\" color=\"chartreuse\">SiD MAIN meteo station history</font></center><br>\r\n");
          client.println("<br><center><table border=\"1\">");
          client.println("<tr><td>");
          client.println(Dstr(cur_dow,cur_dd,cur_mon,cur_yyyy));
          client.println("</td><td>Температура</td><td>Влажность</td><td>Давление</td></tr>");
            flSD=String("/MM/")+SDdat(cur_dd, cur_mon, cur_yyyy)+String(".MM");
            myRFile = SD.open(flSD);
            wrtSD="";
            while (myRFile.available()) {
            char c=myRFile.read(); 
            wrtSD+=c;
            if(wrtSD.endsWith("}")){
              hsd=StoD(wrtSD);
              wrtSD="";
                if(hsd.mm==5){
                client.print("<tr><td>");
                client.print(Tstr(hsd.hh,hsd.mm,hsd.ss));
                client.print("</td><td>");
                client.print(hsd.temp);
                client.print("</td><td>");
                client.print(hsd.hum); 
                client.print("</td><td>");
                client.print(hsd.p0);                                                                            
                client.println("</td></tr>");  
                }             
            }
            }             

            myRFile.close(); 
          client.println("</table></center>");
          client.print("<br>\r\n");
          client.println("<p align=\"center\" font color=\red\">-<a href=\"/H\">Назад</a> к метеостанции - <a href=\"/J\">Назад</a> к истории -</font></p><br>");
          client.println("<p align=\"right\" font color=\"orange\"><a href=\"/C\">городская</a> &equiv;&rArr;</font></p><br>");
          client.print("</body></html>\r\n");
}

// посылает веб страницу история  за 5 dney мет станции
void send_his5_met_page(WiFiEspClient client,  String MName)
{
          float d_temp,n_temp, m_temp, x_temp;
          String m_temp_t, x_temp_t;
          float d_hum,n_hum, m_hum, x_hum;
          String m_hum_t, x_hum_t;
          float d_nos_sil,n_nos_sil, m_nos_sil, x_nos_sil;
          String m_nos_sil_t, x_nos_sil_t;
          int d_nos_napr,n_nos_napr, m_nos_napr, x_nos_napr;
          float d_os_mm,n_os_mm, m_os_mm, x_os_mm, tot_os_mm;
          String m_os_mm_t, x_os_mm_t; 
          float d_radio,n_radio, m_radio, x_radio, tot_radio; 
          String m_radio_t, x_radio_t;        
          double d_p0,n_p0, m_p0, x_p0;
          String m_p0_t, x_p0_t;
          uint16_t d_lux, n_lux, m_lux, x_lux;
          String m_lux_t, x_lux_t;
          client.print(
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n"  // the connection will be closed after completion of the response
            "\r\n");
          client.print("<!DOCTYPE HTML>\r\n");
          client.print("<html><head><meta charset=\"utf-8\"><title>SiD</title></head><body>\r\n");
          client.println("Возврат к <a href=\"/S\">главному</a> меню<br>");       
          client.print("<center><font size=\"+4\" color=\"chartreuse\">SiD "); 
          if (MName=="MM"){ client.println("MAIN");}
          if (MName=="MT"){ client.println("TOWN");}
          if (MName=="MZ"){ client.println("ZAVOD");}
          if (MName=="WT"){ client.println("WORLD");}
          client.println(" meteo station history</font></center><br>\r\n");
          client.println("<br><center><table border=\"1\">");
          client.println("<tr><td>Дата</td><td>");
          client.println("</td><td>Температура</td><td>Влажность</td><td>Давление</td>");
          if ((MName=="MT") || (MName=="MZ") || (MName=="WT")){
          client.println("<td>Освещ.</td>"); 
          }
          if ((MName=="MT") || (MName=="MZ")){
          client.println("<td>Ветер</td><td>Осадки</td><td>Радиация</td>"); 
          }
          client.println("</tr>");
          for(int i=0; i<=4; i++){
            his_epot=cur_epot-86400*i;
              ptim = gmtime(&his_epot);
            his_yyyy=ptim->tm_year+1900;
            his_mon=ptim->tm_mon+1;
            his_dd=ptim->tm_mday;
            his_dow=ptim->tm_wday-1;
            if(his_dow==-1){his_dow=6;}
                      
            flSD=String("/"+MName+"/")+SDdat(his_dd, his_mon, his_yyyy)+String(".")+MName;
            myRFile = SD.open(flSD);
            wrtSD="";
            d_temp=0.0; n_temp=0.0; m_temp=0.0; x_temp=0.0; m_temp_t=""; x_temp_t="";
            d_hum=0.0; n_hum=0.0; m_hum=0.0; x_hum=0.0;m_hum_t=""; x_hum_t="";
            d_p0=0.0; n_p0=0.0; m_p0=0.0; x_p0=0.0;m_p0_t=""; x_p0_t="";
            d_nos_sil=MAX_NOS; n_nos_sil=MAX_NOS; m_nos_sil=MAX_NOS; x_nos_sil=-MAX_NOS;m_nos_sil_t=""; x_nos_sil_t="";
            d_nos_napr=0; n_nos_napr=0; m_nos_napr=0; x_nos_napr=0;
            d_os_mm=MAX_OS; n_os_mm=MAX_OS; m_os_mm=MAX_OS; x_os_mm=-MAX_OS;m_os_mm_t=""; x_os_mm_t="";            
            d_lux=MAX_LUX; n_lux=MAX_LUX; m_lux=MAX_LUX; x_lux=0;m_lux_t=""; x_lux_t="";
            d_radio=MAX_NOS; n_radio=MAX_NOS; m_radio=MAX_NOS; x_radio=-MAX_NOS; m_radio_t=""; x_radio_t="";
            tot_os_mm=0.0;
            tot_radio=0.0;
             
            while (myRFile.available()) {
            char c=myRFile.read(); 
            wrtSD+=c;
            if(wrtSD.endsWith("}")){
              hsd=StoD(wrtSD);
              wrtSD="";
                if((hsd.hh==0)&&(n_temp==0.0)){
                  n_temp=hsd.temp; 
                  n_hum=hsd.hum;
                  n_p0=hsd.p0;
                  n_lux=hsd.lux;
                  n_nos_sil=hsd.nos_sil;
                  n_nos_napr=hsd.nos_napr;
                  n_os_mm=hsd.os_mm;
                  n_radio=hsd.radio;
                 }   
                if((hsd.hh==12)&&(d_temp==0.0)){
                  d_temp=hsd.temp; 
                  d_hum=hsd.hum;
                  d_p0=hsd.p0;
                  d_lux=hsd.lux;
                  d_nos_sil=hsd.nos_sil;
                  d_nos_napr=hsd.nos_napr;
                  d_os_mm=hsd.os_mm;
                  d_radio=hsd.radio;
                }  
                if((m_temp==0.0)||(m_temp>hsd.temp)){ m_temp=hsd.temp; m_temp_t=Tstr(hsd.hh, hsd.mm, hsd.ss); }
                if((x_temp==0.0)||(x_temp<hsd.temp)){ x_temp=hsd.temp; x_temp_t=Tstr(hsd.hh, hsd.mm, hsd.ss); }
                if((m_hum==0.0)||(m_hum>hsd.hum)){ m_hum=hsd.hum; m_hum_t=Tstr(hsd.hh, hsd.mm, hsd.ss); }
                if((x_hum==0.0)||(x_hum<hsd.hum)){ x_hum=hsd.hum; x_hum_t=Tstr(hsd.hh, hsd.mm, hsd.ss); }
                if((m_p0==0.0)||(m_p0>hsd.p0)){ m_p0=hsd.p0; m_p0_t=Tstr(hsd.hh, hsd.mm, hsd.ss); }
                if((x_p0==0.0)||(x_p0<hsd.p0)){ x_p0=hsd.p0; x_p0_t=Tstr(hsd.hh, hsd.mm, hsd.ss); }
                if((m_nos_sil==MAX_NOS)||(m_nos_sil>hsd.nos_sil)){ m_nos_sil=hsd.nos_sil; m_nos_napr=hsd.nos_napr; m_nos_sil_t=Tstr(hsd.hh, hsd.mm, hsd.ss);}
                if((x_nos_sil==-MAX_NOS)||(x_nos_sil<hsd.nos_sil)){ x_nos_sil=hsd.nos_sil; x_nos_napr=hsd.nos_napr; x_nos_sil_t=Tstr(hsd.hh, hsd.mm, hsd.ss);}
                if((m_os_mm==MAX_OS)||(m_os_mm>hsd.os_mm)){ m_os_mm=hsd.os_mm; m_os_mm_t=Tstr(hsd.hh, hsd.mm, hsd.ss); }
                if((x_os_mm==-MAX_OS)||(x_os_mm<hsd.os_mm)){ x_os_mm=hsd.os_mm; x_os_mm_t=Tstr(hsd.hh, hsd.mm, hsd.ss); }
                if((m_lux==MAX_LUX)||(m_lux>hsd.lux)){ m_lux=hsd.lux; m_lux_t=Tstr(hsd.hh, hsd.mm, hsd.ss); }
                if((x_lux==0)||(x_lux<hsd.lux)){ x_lux=hsd.lux; x_lux_t=Tstr(hsd.hh, hsd.mm, hsd.ss); }
                if((m_radio==MAX_OS)||(m_radio>hsd.radio)){ m_radio=hsd.radio; m_radio_t=Tstr(hsd.hh, hsd.mm, hsd.ss); }
                if((x_radio==-MAX_OS)||(x_radio<hsd.radio)){ x_radio=hsd.radio; x_radio_t=Tstr(hsd.hh, hsd.mm, hsd.ss); }
                tot_os_mm=hsd.os_mm+tot_os_mm;
                tot_radio=hsd.radio+tot_radio;
                  
            }
            }             
            myRFile.close();
                client.print("<tr><td rowspan=\"");
                if ((MName=="MT") || (MName=="MZ")){
                client.println("5"); 
                } else {
                client.println("4"); 
                }
                client.print("\">");               
                client.println(Dstr(his_dow,his_dd,his_mon,his_yyyy));
                client.print("</td><td>&nbsp;&#127769;&nbsp;");
                client.print("</td><td>");
                if (n_temp==0.0){client.print("No data"); } else{ client.print(n_temp);}
                client.print("</td><td>");
                if (n_hum==0.0){client.print("No data"); } else{client.print(n_hum);}
                client.print("</td><td>");
                if (n_p0==0.0){client.print("No data"); } else{client.print(n_p0); }                                                                           
                client.println("</td>");
                if ((MName=="MT") || (MName=="MZ") || (MName=="WT")){
                client.print("</td><td>");
                if (n_lux==MAX_LUX){client.print("No data"); } else{client.print(n_lux); }                                                                           
                client.println("</td>"); } 
                if ((MName=="MT") || (MName=="MZ")){
                client.print("</td><td>");
                if (n_nos_sil==MAX_NOS){client.print("No data"); } else{client.print(n_nos_sil); client.print(" "); client.print(vet_nap(n_nos_napr)); }                                                                           
                client.println("</td>");
                client.print("</td><td>");
                if (n_os_mm==MAX_OS){client.print("No data"); } else{client.print(n_os_mm); }                                                                           
                client.println("</td>");  
                client.print("</td><td>");
                if (n_radio==MAX_OS){client.print("No data"); } else{client.print(n_radio); }                                                                           
                client.println("</td>"); 
                }                  
                client.println("</tr>");       
                client.print("<tr><td>&nbsp;&#9728;&nbsp;");
                client.print("</td><td>");
                if (d_temp==0.0){client.print("No data"); } else{client.print(d_temp);}
                client.print("</td><td>");
                if (d_hum==0.0){client.print("No data"); } else{client.print(d_hum); }
                client.print("</td><td>");
                if (d_p0==0.0){client.print("No data"); } else{client.print(d_p0); }                                                                           
                client.println("</td>");
                if ((MName=="MT") || (MName=="MZ") || (MName=="WT")){
                client.print("</td><td>");
                if (d_lux==MAX_LUX){client.print("No data"); } else{client.print(d_lux); }                                                                           
                client.println("</td>"); }
                if ((MName=="MT") || (MName=="MZ")){
                client.print("</td><td>");
                if (d_nos_sil==MAX_NOS){client.print("No data"); } else{client.print(d_nos_sil); client.print(" "); client.print(vet_nap(d_nos_napr)); }                                                                           
                client.println("</td>");
                client.print("</td><td>");
                if (d_os_mm==MAX_OS){client.print("No data"); } else{client.print(d_os_mm); }                                                                           
                client.println("</td>"); 
                client.print("</td><td>");
                if (d_radio==MAX_OS){client.print("No data"); } else{client.print(d_radio); }                                                                           
                client.println("</td>");  
                }                   
                client.println("</tr>");       
                client.print("<tr><td>&nbsp;MAX&nbsp;");
                client.print("</td><td>");
                if (x_temp==0.0){client.print("No data"); } else{client.print(x_temp);  client.print("<br>"); client.print(x_temp_t);}
                client.print("</td><td>");
                if (x_hum==0.0){client.print("No data"); } else{client.print(x_hum); client.print("<br>"); client.print(x_hum_t);} 
                client.print("</td><td>");
                if (x_p0==0.0){client.print("No data"); } else{client.print(x_p0); client.print("<br>"); client.print(x_p0_t);}                                                                           
                client.println("</td>");
                if ((MName=="MT") || (MName=="MZ") || (MName=="WT")){
                client.print("</td><td>");
                if (x_lux==0){client.print("No data"); } else{client.print(x_lux); client.print("<br>"); client.print(x_lux_t);}                                                                           
                client.println("</td>"); }
                if ((MName=="MT") || (MName=="MZ")){
                client.print("</td><td>");
                if (x_nos_sil==-MAX_NOS){client.print("No data"); } else{client.print(x_nos_sil); client.print(" "); client.print(vet_nap(x_nos_napr)); client.print("<br>"); client.print(x_nos_sil_t);}                                                                           
                client.println("</td>");
                client.print("</td><td>");
                if (x_os_mm==-MAX_OS){client.print("No data"); } else{client.print(x_os_mm); client.print("<br>"); client.print(x_os_mm_t);}                                                                           
                client.println("</td>");  
                client.print("</td><td>");
                if (x_radio==MAX_OS){client.print("No data"); } else{client.print(x_radio); client.print("<br>"); client.print(x_radio_t);}                                                                           
                client.println("</td>"); 
                }                   
                client.println("</tr>");         
                client.print("<tr><td>&nbsp;MIN&nbsp;");
                client.print("</td><td>");
                if (m_temp==0.0){client.print("No data"); } else{client.print(m_temp); client.print("<br>"); client.print(m_temp_t);}
                client.print("</td><td>");
                if (m_hum==0.0){client.print("No data"); } else{client.print(m_hum);client.print("<br>"); client.print(m_hum_t);} 
                client.print("</td><td>");
                if (m_p0==0.0){client.print("No data"); } else{client.print(m_p0);client.print("<br>"); client.print(m_p0_t); }                                                                           
                client.println("</td>");
                if ((MName=="MT") || (MName=="MZ") || (MName=="WT")){
                client.print("</td><td>");
                if (m_lux==MAX_LUX){client.print("No data"); } else{client.print(m_lux);  client.print("<br>"); client.print(m_lux_t); }                                                                           
                client.println("</td>"); } 
                if ((MName=="MT") || (MName=="MZ")){
                client.print("</td><td>");
                if (m_nos_sil==MAX_NOS){client.print("No data"); } else{client.print(m_nos_sil); client.print(" "); client.print(vet_nap(m_nos_napr)); client.print("<br>"); client.print(m_nos_sil_t); }                                                                           
                client.println("</td>");
                client.print("</td><td>");
                if (m_os_mm==MAX_NOS){client.print("No data"); } else{client.print(m_os_mm); client.print("<br>"); client.print(m_os_mm_t); }                                                                           
                client.println("</td>");
                client.print("</td><td>");
                if (m_radio==MAX_OS){client.print("No data"); } else{client.print(m_radio); client.print("<br>"); client.print(m_radio_t); }                                                                           
                client.println("</td>");   
                }                
                client.println("</tr>"); 
                if ((MName=="MT") || (MName=="MZ")){
                client.print("<tr><td>&nbsp;TOTAL&nbsp;");
                client.print("</td><td></td><td></td><td></td><td></td><td></td><td>");
                client.print(tot_os_mm);                                                                          
                client.print("</td><td>");
                client.print(tot_radio/6.0);                                                                           
                client.println("</td>");                
                client.println("</tr>"); }
                       
          }
          client.println("</table></center>");
          client.print("<br>\r\n");
          client.println("<p align=\"center\" font color=\red\">-<a href=\"");
          if(MName=="MT"){ client.print("/L");}
          if(MName=="MZ"){ client.print("/F");}
          if(MName=="MM"){ client.print("/H");}
          if(MName=="WT"){ client.print("/B");}          
          client.println("\">Назад</a> к метеостанции - <a href=\"/J\">Назад</a> к истории -</font></p><br>");
          client.print("</body></html>\r\n");
}

// посылает служ страницу
void sendservice_page(WiFiEspClient client)
{
  // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
  // and a content-type so the client knows what's coming, then a blank line:
  client.print(
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n"  
            "Refresh: 60\r\n"        
            "\r\n");
          client.print("<!DOCTYPE HTML>\r\n");
          client.print("<html><head><meta charset=\"utf-8\"><title>SiD</title></head><body bgcolor=\"#000000\" text=\"lime\">\r\n");
          client.println("Возврат к <a href=\"/S\">главному</a> меню<br>");       
          client.print("<center><font size=\"+4\" color=\"chartreuse\">SiD Service</font></center><br><br>\r\n");
          client.println("<table border=\"0\">");
          client.println("<tr><td rowspan=\"4\" align=\"left\" valign=\"top\">");
          client.print("░░░░░░░░░░░░░░░░░░░░░░░▄▄<br>");
          client.print("░░░░░░░░░░░░░░░░░░░░░▄▀░░▌<br>");
          client.print("░░░░░░░░░░░░░░░░░░░▄▀▐░░░▌<br>");
          client.print("░░░░░░░░░░░░░░░░▄▀▀▒▐▒░░░▌<br>");
          client.print("░░░░░▄▀▀▄░░░▄▄▀▀▒▒▒▒▌▒▒░░▌<br>");
          client.print("░░░░▐▒░░░▀▄▀▒▒▒▒▒▒▒▒▒▒▒▒▒█<br>");
          client.print("░░░░▌▒░░░░▒▀▄▒▒▒▒▒▒▒▒▒▒▒▒▒▀▄<br>");
          client.print("░░░░▐▒░░░░░▒▒▒▒▒▒▒▒▒▌▒▐▒▒▒▒▒▀▄<br>");
          client.print("░░░░▌▀▄░░▒▒▒▒▒▒▒▒▐▒▒▒▌▒▌▒▄▄▒▒▐<br>");
          client.print("░░░▌▌▒▒▀▒▒▒▒▒▒▒▒▒▒▐▒▒▒▒▒█▄█▌▒▒▌<br>");
          client.print("░▄▀▒▐▒▒▒▒▒▒▒▒▒▒▒▄▀█▌▒▒▒▒▒▀▀▒▒▐░░░▄<br>");
          client.print("▀▒▒▒▒▌▒▒▒▒▒▒▒▄▒▐███▌▄▒▒▒▒▒▒▒▄▀▀▀▀<br>");
          client.print("▒▒▒▒▒▐▒▒▒▒▒▄▀▒▒▒▀▀▀▒▒▒▒▄█▀░░▒▌▀▀▄▄<br>");
          client.print("▒▒▒▒▒▒█▒▄▄▀▒▒▒▒▒▒▒▒▒▒▒░░▐▒▀▄▀▄░░░░▀<br>");
          client.print("▒▒▒▒▒▒▒█▒▒▒▒▒▒▒▒▒▄▒▒▒▒▄▀▒▒▒▌░░▀▄<br>");
          client.print("▒▒▒▒▒▒▒▒▀▄▒▒▒▒▒▒▒▒▀▀▀▀▒▒▒▄▀<br>");
          client.print("▒▒▒▒▒▒▒▒▒▒▀▄▄▒▒▒▒▒▒▒▒▒▒▒▐<br>");
          client.print("▒▒▒▒▒▒▒▒▒▒▒▒▒▀▀▄▄▄▒▒▒▒▒▒▌<br>");
          client.print("▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▐<br>");
          client.print("▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▐<br>");
          client.print("</td><td align=\"left\" valign=\"top\">");
          client.print("&nbsp; &nbsp;&#9673");
          client.print("</td><td align=\"left\" valign=\"top\">");
          client.print("Главная метеостанция: <br>"); 
          client.print("Power Line: "); 
          if ((InputB>300)&&(InputS>300)){
          client.print("up. <br>"); 
          }  
          if ((InputB>300)&&(InputS<300)){
          client.print("down. <br>"); 
          }
          client.print("Battery: "); client.print(msd.Volt);  client.print(" V, ");  client.print(msd.Bat_lvl); client.print(" %.<br> "); 
          client.print("WiFi net> "); client.print(WiFi.SSID()); client.print(", sign: ");client.print(WiFi.RSSI()); client.print(" dBm.<br>");
          int kek= GSM_sign(); 
          client.print("GsM  net> "); client.print(GSM_Net); client.print(", sign: "); client.print((int)(-110+(kek*56)/30)); client.print(" dBm. "); 
          client.print(gsm_bal);  client.print("<br> ");
          client.print("Reset time: ");
          client.print(reset_dt.year);   client.print("-");
          client.print(reset_dt.month);  client.print("-");
          client.print(reset_dt.day);    client.print(" ");
          client.print(reset_dt.hour);   client.print(":");
          client.print(reset_dt.minute); client.print(":");
          client.print(reset_dt.second); 
          client.print(". Reset code: ");
          client.println(reset_status, BIN);  client.print(".<br> ");                                                                                       
          client.println("</td></tr>");
          client.print("<tr><td align=\"left\" valign=\"top\">");
          tim.tm_year = tsd.yyyy-1900;
          tim.tm_mon = tsd.mon-1;           // Month, 0 - jan
          tim.tm_mday = tsd.dd;         // Day of the month
          tim.tm_hour = tsd.hh;
          tim.tm_min = tsd.mm;
          tim.tm_sec = tsd.ss;
          tim.tm_isdst = -1;        // Is DST on? 1 = yes, 0 = no, -1 = unknown
          tsd_epot = mktime(&tim); 
          if ((tsd_epot<0) || (cur_epot-tsd_epot)>3600) {
            client.print("<font color=\"red\" >&nbsp; &nbsp; &#9673</font>");
          } else {
          client.print("&nbsp; &nbsp;&#9673");
          }
          client.print("</td><td align=\"left\" valign=\"top\">");       
          client.print("Городская метеостанция:<br>");
          if (tsd_epot<0){
          client.print("No data!");  
          } else {
             if ((cur_epot-tsd_epot)>3600)  {
             client.print("Last received ");
             client.print(Tstr(tsd.hh,tsd.mm,tsd.ss));
             client.print(Dstr(tsd.dow,tsd.dd,tsd.mon,tsd.yyyy));
             }
            client.print("Battery: "); client.print(tsd.Volt);  client.print(" V, ");  client.print(tsd.Bat_lvl); client.print(" %.<br> ");
            client.print("Internal TemP ");   client.print(tsd.TV); client.print("*C."); 
          }
          client.print("<tr><td align=\"left\" valign=\"top\">");
          tim.tm_year = zsd.yyyy-1900;
          tim.tm_mon = zsd.mon-1;           // Month, 0 - jan
          tim.tm_mday = zsd.dd;         // Day of the month
          tim.tm_hour = zsd.hh;
          tim.tm_min = zsd.mm;
          tim.tm_sec = zsd.ss;
          tim.tm_isdst = -1;        // Is DST on? 1 = yes, 0 = no, -1 = unknown
          zsd_epot = mktime(&tim); 
          if ((zsd_epot<0) || (cur_epot-zsd_epot)>3600) {
            client.print("<font color=\"red\" >&nbsp; &nbsp; &#9673</font>");
          } else {
          client.print("&nbsp; &nbsp;&#9673");
          }
          client.print("</td><td align=\"left\" valign=\"top\">");       
          client.print("Заводская метеостанция:<br>");
          if (zsd_epot<0){
          client.print("No data!");  
          } else {
             if ((cur_epot-zsd_epot)>3600)  {
             client.print("Last received ");
             client.print(Tstr(zsd.hh,zsd.mm,zsd.ss));
             client.print(Dstr(zsd.dow,zsd.dd,zsd.mon,zsd.yyyy));
             }
            client.print("Battery: "); client.print(zsd.Volt);  client.print(" V, ");  client.print(zsd.Bat_lvl); client.print(" %.<br> ");
            client.print("Internal TemP ");   client.print(zsd.TV); client.print("*C."); 
          }                                                                                  
          client.print("<tr><td align=\"left\" valign=\"top\">");
          tim.tm_year = wsd.yyyy-1900;
          tim.tm_mon = wsd.mon-1;           // Month, 0 - jan
          tim.tm_mday = wsd.dd;         // Day of the month
          tim.tm_hour = wsd.hh;
          tim.tm_min = wsd.mm;
          tim.tm_sec = wsd.ss;
          tim.tm_isdst = -1;        // Is DST on? 1 = yes, 0 = no, -1 = unknown
          wsd_epot = mktime(&tim); 
          if ((wsd_epot<0) || (cur_epot-wsd_epot)>3600) {
            client.print("<font color=\"red\" >&nbsp; &nbsp; &#9673</font>");
          } else {
          client.print("&nbsp; &nbsp;&#9673");
          }
          client.print("</td><td align=\"left\" valign=\"top\">");       
          client.print("Походная метеостанция:<br>");
          if (wsd_epot<0){
          client.print("No data!");  
          } else {
             if ((cur_epot-wsd_epot)>3600)  {
             client.print("Last received ");
             client.print(Tstr(wsd.hh,wsd.mm,wsd.ss));
             client.print(Dstr(wsd.dow,wsd.dd,wsd.mon,wsd.yyyy));
             }
            client.print(" Battery: "); client.print(wsd.Volt);  client.print(" V, ");  client.print(wsd.Bat_lvl); client.print(" %.<br> ");
            client.print("Internal TemP ");   client.print(wsd.TV); client.print("*C.<br>");
            client.print("Position: long "); client.print(wsd.longi);  client.print(", lat ");  client.print(wsd.lati); client.print(".<br>");
            client.print("Altitude: "); client.print(wsd.alt);  client.print("M.");             
          }
                                                                                   
          client.println("</td></tr>");
          client.println("</table>");
          client.print("</body></html>\r\n");
}


// формирует график для SCOPE
void scope_form(int SN)  // Номер SCOPE - городская - 0 
{
           String MName;
           int hour_offset, k;
           float h_temp[72], h_p0[72], h_hum[72];
           for (int i=0; i<72; i++) { h_temp[i]=0.0; h_p0[i]=0.0; h_hum[i]=0.0; }
           if (SN==0) { MName="MT"; hour_offset=2; }
           if (SN==1) { MName="MZ"; hour_offset=3; }
            flSD=String("/"+MName+"/")+SDdat(cur_dd, cur_mon, cur_yyyy)+String(".")+MName;
            myRFile = SD.open(flSD);
            wrtSD="";
            while (myRFile.available()) {
            char c=myRFile.read(); 
            wrtSD+=c;
            if(wrtSD.endsWith("}")){
              hsd=StoD(wrtSD);
              watchdogReset();
              wrtSD="";
                if((hsd.mm%10)==hour_offset){
                h_temp[hsd.hh*3+hsd.mm/10]=hsd.temp;
                h_p0[hsd.hh*3+hsd.mm/10]=hsd.p0;
                h_hum[hsd.hh*3+hsd.mm/10]=hsd.hum;
                }             
            }
            } 
            myRFile.close();  
            for (int i=0; i<72; i++) {
            if ((i % 3)==0) k=5; else k=6;
            for (int j=0; j<k; j++) {
              watchdogReset(); 
              genie.WriteObject(GENIE_OBJ_SCOPE,SN, int((h_temp[i]+20.0)*3.7)); 
              genie.WriteObject(GENIE_OBJ_SCOPE,SN, int((h_p0[i]-720.0)*3.17)); 
              genie.WriteObject(GENIE_OBJ_SCOPE,SN, int(h_hum[i]*2.22)); 
              }
            }
}

void myGenieEventHandler(void)
{
  genieFrame Event;
  watchdogReset();
  genie.DequeueEvent(&Event); // Remove the next queued event from the buffer, and process it below

  //If the cmd received is from a Reported Event (Events triggered from the Events tab of Workshop4 objects)

  if (Event.reportObject.cmd == GENIE_REPORT_EVENT)
  {

 SerialUSB.print("Report event. Object "); SerialUSB.print(Event.reportObject.object, HEX); SerialUSB.print("  Index "); SerialUSB.println(Event.reportObject.index, HEX);

    
    if (Event.reportObject.object == GENIE_OBJ_4DBUTTON)               
    {
      if (Event.reportObject.index == 0)              // Кнопка 0 форм 0 - переключение режимов ЛСД             
      {
        LCD_change=TRUE;
        LCD_mode+=1;
        if (LCD_mode >4) { LCD_mode=0; }
     
      }
    }

    if (Event.reportObject.object == GENIE_OBJ_FORM)               
    {
      if (Event.reportObject.index == 0)              // Активация форм 0 - начальный экран             
      { genie.WriteStr(3,genie_F0S3_text); genie.WriteStr(4,genie_F0S4_text);  }
      if (Event.reportObject.index == 1)              // Активация форм 1 - городская             
      { genie.WriteStr(5,genie_F0S5_text); genie.WriteStr(6,genie_F0S6_text); genie.WriteStr(7,genie_F0S7_text); }
      if (Event.reportObject.index == 4)              // Активация форм 4 - заводская             
      { genie.WriteStr(8,genie_F0S8_text); genie.WriteStr(9,genie_F0S9_text); genie.WriteStr(10,genie_F0S10_text); }
      if (Event.reportObject.index == 3)              // Активация форм 3 - график городской             
      { scope_form(0); }
      if (Event.reportObject.index == 5)              // Активация форм 5 - график заводской             
      { scope_form(1); }
    }
    }
  }



void printWifiStatus() {
  // print the SSID of the network you're attached to:
  SerialUSB.print("SSID: ");
  SerialUSB.print(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  SerialUSB.print(". IP Address: ");
  SerialUSB.print(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  SerialUSB.print(". signal strength (RSSI):");
  SerialUSB.print(rssi);
  SerialUSB.println(" dBm.");
}

// смена времени и названия
void out_LCD1()
{     lcd.setCursor(0, 0);
      if (wrt==1){
      lcd.print(wrt1);
      wrt=2;
      } else {
      lcd.print(wrt2);
      wrt=1;
      }

}

// this function has to be present, otherwise watchdog won't work
void watchdogSetup(void) 
{
// do what you want here
}

void setup()
{ 
  // initialize serial for debuggig
  SerialUSB.begin(115200);
  SerialUSB.println("Starting...");
  /* while (!SerialUSB) ;*/

  reset_status = (RSTC->RSTC_SR & RSTC_SR_RSTTYP_Msk) >> 8; // Get status from the last Reset
  SerialUSB.print("RSTTYP = 0b"); SerialUSB.println(status, BIN);  // Should be 0b010 after first watchdog reset
  
  msd.alt = 55.0; 
  msd.lati=46.4834404;
  msd.longi=30.7407722;
  msd.nos_napr=0;
  msd.nos_sil=0.0;
  msd.os_mm=0.0;
  msd.radio=0; 
  msd.Volt=0;
  msd.Bat_lvl=0;
  msd.lux=0;     

pinMode (BPLPin,OUTPUT);
pinMode (SOLPin,OUTPUT);
pinMode (RSPin,OUTPUT);
digitalWrite (BPLPin,LOW);
digitalWrite (SOLPin,LOW);
digitalWrite (RSPin,LOW);

  // инициализация датчиков температуры, влажности, давления
  dht.begin();
  pressure.begin();
  watchdogReset();

  // запуск гсм и счит времени
  GSM_Net=GSM_start(); SerialUSB.println(GSM_Net);
  digitalWrite (BPLPin,HIGH);
  // проверка баланса
    gsm_bal=GSM_money(); SerialUSB.println(gsm_bal);
          
//  Time_Zone=GSM_clock();
//          Serial.println(Time_Zone);  

  lcd.begin();
  LCD_timer=LCD_switch.every(5000, out_LCD1);
  lcd.createChar(0, symW);
  watchdogReset();

  Serial.begin(200000);  // Serial0 @ 200000 (200K) Baud
  genie.Begin(Serial);   // Use Serial0 for talking to the Genie Library, and to the 4D Systems display
  genie.AttachEventHandler(myGenieEventHandler); // Attach the user function Event Handler for processing events
  pinMode(RESETLINE, OUTPUT);  // Set D4 on Arduino to Output (4D Arduino Adaptor V2 - Display Reset)
  digitalWrite(RESETLINE, 1);  // Reset the Display via D4
  delay(100);
  digitalWrite(RESETLINE, 0);  // unReset the Display via D4
  delay (3500); //let the display start up after the reset (This is important)
  genie.WriteContrast(15);
  digitalWrite (SOLPin,HIGH);
  watchdogReset();
   
  // initialize serial for ESP module
  Serial3.begin(115200);
  // initialize ESP module
  WiFi.init(&Serial3);
  watchdogReset();
  
  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    SerialUSB.println("WiFi shield not present");
    // don't continue
    while (true);
  }

  // attempt to connect to WiFi network
  while ( status != WL_CONNECTED) {
    SerialUSB.print("Attempting to connect to WPA SSID: ");
    SerialUSB.println(ssid);
    WiFi.config(my_IP);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
  }
  watchdogReset();
  // you're connected now, so print out the data
  SerialUSB.println("You're connected to the network");
  printWifiStatus();
  digitalWrite (RSPin,HIGH);

  // инициализация и установка внутренних и внешних часов
  rtc_clock.init();
  clock_DS3231.begin();
  SerialUSB.println("Internal & external clock init OK.");
  
    TimeSET2_www();

  SerialUSB.print("Internal data: ");
  SerialUSB.print(rtc_clock.get_years());   SerialUSB.print("-");
  SerialUSB.print(rtc_clock.get_months());  SerialUSB.print("-");
  SerialUSB.print(rtc_clock.get_days());    SerialUSB.print(" ");
  SerialUSB.print(rtc_clock.get_day_of_week());   SerialUSB.print(" ");
  SerialUSB.print(rtc_clock.get_hours());   SerialUSB.print(":");
  SerialUSB.print(rtc_clock.get_minutes()); SerialUSB.print(":");
  SerialUSB.print(rtc_clock.get_seconds()); SerialUSB.println("");

  dt = clock_DS3231.getDateTime(); reset_dt=dt;

  SerialUSB.print("External data: ");
  SerialUSB.print(dt.year);   SerialUSB.print("-");
  SerialUSB.print(dt.month);  SerialUSB.print("-");
  SerialUSB.print(dt.day);    SerialUSB.print(" ");
  SerialUSB.print(dt.hour);   SerialUSB.print(":");
  SerialUSB.print(dt.minute); SerialUSB.print(":");
  SerialUSB.print(dt.second); SerialUSB.println("");

  SerialUSB.print("Ext clock temperature: ");
  SerialUSB.println(clock_DS3231.readTemperature());

  server.begin();
  SerialUSB.println("Server started.");
  watchdogReset(); 

  if (!SD.begin(CSPin)) {
    SerialUSB.println("SD initialization failed!");
    return;
  }
  SerialUSB.println("SD initialization done.");
  watchdogReset();

  logFile = SD.open("reset.log", FILE_WRITE);
  logFile.print("Reset time: ");
  logFile.print(rtc_clock.get_years());   logFile.print("-");
  logFile.print(rtc_clock.get_months());  logFile.print("-");
  logFile.print(rtc_clock.get_days());    logFile.print(" ");
  logFile.print(rtc_clock.get_day_of_week());   logFile.print(" ");
  logFile.print(rtc_clock.get_hours());   logFile.print(":");
  logFile.print(rtc_clock.get_minutes()); logFile.print(":");
  logFile.print(rtc_clock.get_seconds()); logFile.print(". Reset code: ");
  logFile.println(reset_status, BIN);
  logFile.close();
  watchdogReset();
  SerialUSB.print("Reset status: "); SerialUSB.println(reset_status, BIN);

  rtc_clock.get_date(&cur_dow,&cur_dd,&cur_mon,&cur_yyyy);
  flSD=String("/MT/")+SDdat(cur_dd, cur_mon, cur_yyyy)+String(".MT");
  myRFile = SD.open(flSD);
  wrtSD="";
            while (myRFile.available()) {
            char c=myRFile.read(); 
            wrtSD+=c;
            if(wrtSD.endsWith("}")){
              hsd=StoD(wrtSD);
              Tren_den=int(hsd.radio*100)/6+Tren_den;
              Tos_den+=hsd.os_mm;
              wrtSD="";
             }
            }             
  myRFile.close();
  SerialUSB.print("Accum town ren "); SerialUSB.print(Tren_den); SerialUSB.print(" os "); SerialUSB.println(Tos_den);
  flSD=String("/MZ/")+SDdat(cur_dd, cur_mon, cur_yyyy)+String(".MZ");
  myRFile = SD.open(flSD);
  wrtSD="";
            while (myRFile.available()) {
            char c=myRFile.read(); 
            wrtSD+=c;
            if(wrtSD.endsWith("}")){
              hsd=StoD(wrtSD);
              Zren_den=int(hsd.radio*100)/6+Zren_den;
              Zos_den+=hsd.os_mm;
              wrtSD="";
             }
            }             
  myRFile.close();
  SerialUSB.print("Accum zavod ren "); SerialUSB.print(Zren_den); SerialUSB.print(" os "); SerialUSB.println(Zos_den);

  Scheduler.startLoop(loop2);
  attachInterrupt(GSMring, ri, FALLING);
  on_ring=0;
  SerialUSB.println("Call reciever initialised");
  watchdogReset();

  // Enable watchdog.
  watchdogEnable(watchdogTime);

  digitalWrite (BPLPin,LOW);
  digitalWrite (SOLPin,LOW);
  digitalWrite (RSPin,LOW);
}

void loop()
{ 
  watchdogReset();

  genie.DoEvents(); // This calls the library each loop to process the queued responses from the display

  LCD_switch.update();
  
  // измерение напряжения батареи
  InputV = analogRead(BVPin);
  Volt = ((float)InputV/1023)*6.6;
  Bat_lvl = ((Volt - 2.4)/1.8)*100; 
  
  InputB = analogRead(BPPin);
  InputS = analogRead(SOPin);
  if ((InputB>300)&&(InputS>300)){
   digitalWrite (BPLPin,LOW);
   digitalWrite (SOLPin,HIGH);
  }  
  if ((InputB>300)&&(InputS<300)){
   digitalWrite (BPLPin,HIGH);
   digitalWrite (SOLPin,LOW);
  }
  if (InputB<300){
   digitalWrite (BPLPin,LOW);
   digitalWrite (SOLPin,LOW);
  }  
  
  rtc_clock.get_time(&cur_hh,&cur_mm,&cur_ss);
  rtc_clock.get_date(&cur_dow,&cur_dd,&cur_mon,&cur_yyyy);
  cur_epot=rtc_clock.unixtime();
  cur_dow = cur_dow - 1;

  if (cur_hh==0 && cur_mm==1 && cur_ss==10) {
   Tren_den=0; Tos_den=0.0; Zren_den=0; Zos_den=0.0;
   } 
  
  if (cur_ss==0){
  msd.hh=cur_hh; msd.mm=cur_mm; msd.ss=cur_ss;
  msd.dow=cur_dow; msd.dd=cur_dd; msd.mon=cur_mon; msd.yyyy=cur_yyyy;
    
  // измерение влажности и температуры
  delay (1000); 
  watchdogReset();
  msd.hum = dht.readHumidity();
  msd.temp = dht.readTemperature();
   
  //проверка давления 
  statusPT = pressure.startTemperature();
  if (statusPT != 0)
  {
    // Wait for the measurement to complete:
    delay(statusPT); 
  statusPT = pressure.getTemperature(msd.TV); }
  if (statusPT != 0)
    {
      statusPT = pressure.startPressure(3);
      if (statusPT != 0)
      {
        // Wait for the measurement to complete:
        delay(statusPT);
        statusPT = pressure.getPressure(msd.p,msd.TV);
        msd.p0 = pressure.sealevel(msd.p,msd.alt); 
        msd.p= msd.p*0.7500637554;
        msd.p0= msd.p0*0.7500637554;
   
      }
    }

  msd.Volt = Volt;
  msd.Bat_lvl = Bat_lvl; 
    
  // запись на сд для главной метео станции
  watchdogReset();
  if ((msd.mm %20)==5) {
  wrtSD=String("MM")+con_DtoS(msd);
  flSD=String("/MM/")+SDdat(msd.dd, msd.mon, msd.yyyy)+String(".MM");
  myWFile = SD.open(flSD, FILE_WRITE);
  myWFile.println(wrtSD);
  myWFile.close();
  } 

  // проверка баланса
  if((cur_mm%30)==0) {
  gsm_bal=GSM_money();
  }
  watchdogReset();

  // обработка смс с карточки
  if((cur_mm%5)==0) {
  SerialUSB.print("On_ring "); SerialUSB.println(on_ring);
    if (on_ring==0) {

  printWifiStatus(); 

  SerialUSB.print(rtc_clock.get_years());   SerialUSB.print("-");
  SerialUSB.print(rtc_clock.get_months());  SerialUSB.print("-");
  SerialUSB.print(rtc_clock.get_days());    SerialUSB.print(" ");
  SerialUSB.print(rtc_clock.get_day_of_week());   /*  SerialUSB.print(daynames[dt.dow]);    */SerialUSB.print(" ");
  SerialUSB.print(rtc_clock.get_hours());   SerialUSB.print(":");
  SerialUSB.print(rtc_clock.get_minutes()); SerialUSB.print(":");
  SerialUSB.print(rtc_clock.get_seconds()); SerialUSB.print(" -  ");
  SerialUSB.print(Bat_lvl); SerialUSB.print("% ->  ");

    watchdogReset();
    sms_kol=GSM_csms();   
    SerialUSB.print("Kol SMS "); SerialUSB.print(sms_kol);
    for(int i= 1; i <=sms_kol; i++){ watchdogReset();
      sms_text=GSM_rsms(i,&sms_num);    SerialUSB.println(i); SerialUSB.println(sms_text); SerialUSB.println(sms_num);
      if (sms_text.startsWith("POSTWT")) {
            sms_text.replace("(","{");
            sms_text.replace(")","}");
            wsd=StoD(sms_text); 
            flSD=String("/WT/")+SDdat(wsd.dd, wsd.mon, wsd.yyyy)+String(".WT");
            myWFile = SD.open(flSD, FILE_WRITE);
            myWFile.println(sms_text);
            myWFile.close(); 
            if(LCD_mode==3){ LCD_change=TRUE; }
            };
      GSM_dsms(i);  
      }
    watchdogReset();
    sms_kol=GSM_csms();
    SerialUSB.print("   Kol SMS check "); SerialUSB.println(sms_kol);   
    if (sms_kol!=0) {
    for(int i= 1; i <=50; i++){ watchdogReset();
      sms_text=GSM_rsms(i,&sms_num);    SerialUSB.println(i); SerialUSB.println(sms_text); SerialUSB.println(sms_num);
      if (sms_text.startsWith("POSTWT")) {
            sms_text.replace("(","{");
            sms_text.replace(")","}");
            wsd=StoD(sms_text); 
            flSD=String("/WT/")+SDdat(wsd.dd, wsd.mon, wsd.yyyy)+String(".WT");
            myWFile = SD.open(flSD, FILE_WRITE);
            myWFile.println(sms_text);
            myWFile.close(); 
            if(LCD_mode==3){ LCD_change=TRUE; }
            };
      GSM_dsms(i);  
      } 
    } 
    }     
  } 
  
  if(LCD_mode==1){ LCD_change=TRUE; }
  }

  // вывод на тач скрин
  if ( cur_ss!=Prev_ss1) { watchdogReset();
   Prev_ss1=cur_ss;
   genie.WriteStr(1,Tstr(cur_hh,cur_mm,cur_ss));
   }
   
  if ( cur_mm!=Prev_mm1) {
   Prev_mm1=cur_mm;
   genie_F0S2_mode+=1;
   
   if (genie_F0S2_mode>2){genie_F0S2_mode=0;}

   if(genie_F0S2_mode==0){ watchdogReset();
   genie.WriteObject(GENIE_OBJ_STRINGS,2, 0);     
   genie_F0S3_text="     ";
   genie_F0S3_text+= DtoS(msd.hum,2);
   genie_F0S3_text+="%   ";
   genie_F0S3_text+=DtoS(msd.temp,2);
   genie_F0S3_text+="*C   ";
   genie_F0S3_text+=DtoS(msd.p0,2);
   genie_F0S3_text+="mm.Hg";
   genie_F0S4_text=" ";
   genie.WriteStr(3,genie_F0S3_text); 
   genie.WriteStr(4,genie_F0S4_text); 
   }

   if(genie_F0S2_mode==1){ watchdogReset();
   genie.WriteObject(GENIE_OBJ_STRINGS,2, 1); 
   genie_F0S3_text="  ";
   genie_F0S3_text+= DtoS(tsd.hum,2);
   genie_F0S3_text+="%  ";
   genie_F0S3_text+=DtoS(tsd.temp,2);
   genie_F0S3_text+="*C  ";
   genie_F0S3_text+=DtoS(tsd.p0,2);
   genie_F0S3_text+="mm.Hg ";
   if (tsd.lux<1000) {
   genie_F0S3_text+=String(tsd.lux); }
   else { genie_F0S3_text+=DtoS(tsd.lux/1000,1); genie_F0S3_text+="K";}
   genie_F0S3_text+=" lux";
   genie_F0S4_text="  ";
   genie_F0S4_text+=DtoS(tsd.radio,2);
   genie_F0S4_text+=" uSv/h  ";
   genie_F0S4_text+=DtoS(tsd.nos_sil,1);
   genie_F0S4_text+=" m/s ";
   genie_F0S4_text+=vet_nap(tsd.nos_napr);
   genie_F0S4_text+="  ";
   genie_F0S4_text+=DtoS(tsd.os_mm,1);
   genie_F0S4_text+=" mm";
   genie.WriteStr(3,genie_F0S3_text); 
   genie.WriteStr(4,genie_F0S4_text); 
   }

   if(genie_F0S2_mode==2){ watchdogReset();
   genie.WriteObject(GENIE_OBJ_STRINGS,2, 2); 
   genie_F0S3_text="  ";
   genie_F0S3_text+= DtoS(zsd.hum,2);
   genie_F0S3_text+="%  ";
   genie_F0S3_text+=DtoS(zsd.temp,2);
   genie_F0S3_text+="*C  ";
   genie_F0S3_text+=DtoS(zsd.p0,2);
   genie_F0S3_text+="mm.Hg ";
   if (zsd.lux<1000) {
   genie_F0S3_text+=String(zsd.lux); }
   else { genie_F0S3_text+=DtoS(zsd.lux/1000,1); genie_F0S3_text+="K";}
   genie_F0S3_text+=" lux";
   genie_F0S4_text="  ";
   genie_F0S4_text+=DtoS(zsd.radio,2);
   genie_F0S4_text+=" uSv/h  ";
   genie_F0S4_text+=DtoS(zsd.nos_sil,1);
   genie_F0S4_text+=" m/s ";
   genie_F0S4_text+=vet_nap(zsd.nos_napr);
   genie_F0S4_text+="  ";
   genie_F0S4_text+=DtoS(zsd.os_mm,1);
   genie_F0S4_text+=" mm";
   genie.WriteStr(3,genie_F0S3_text); 
   genie.WriteStr(4,genie_F0S4_text); 
   }

   // форм 2 - главная
   genie.WriteObject(GENIE_OBJ_THERMOMETER,0x01, (int(msd.temp)+20)); 
   genie.WriteObject(GENIE_OBJ_COOL_GAUGE,0x02, (int(msd.p0)-720)); 
   genie.WriteObject(GENIE_OBJ_COOL_GAUGE,0x03, int(msd.hum)); 

   // форм 1 - городская   
   genie.WriteObject(GENIE_OBJ_THERMOMETER,0x00, (int(tsd.temp)+20)); 
   genie.WriteObject(GENIE_OBJ_COOL_GAUGE,0x00, (int(tsd.p0)-720)); 
   genie.WriteObject(GENIE_OBJ_COOL_GAUGE,0x01, int(tsd.hum));
   int Tlux_gog;
   if (tsd.lux<=2000){
    Tlux_gog=tsd.lux/80;
   }
   if ((tsd.lux>2000) && (tsd.lux<=10000)){
    Tlux_gog=25+(tsd.lux-2000)/800;
   }
    if (tsd.lux>10000){
    Tlux_gog=35+(tsd.lux-10000)/2667;
   }
   genie.WriteObject(GENIE_OBJ_GAUGE,0x00, Tlux_gog); 
   genie.WriteObject(GENIE_OBJ_GAUGE,0x01, int(tsd.os_mm));
   genie.WriteObject(GENIE_OBJ_USERIMAGES, 0x00,vet_nap_n(tsd.nos_napr));
   genie_F0S5_text=DtoS(tsd.nos_sil,2);
   genie_F0S5_text+=" m/s";
   genie.WriteStr(5,genie_F0S5_text); 
   genie.WriteObject(GENIE_OBJ_METER,0x00, tsd.radio*100);
   if (Tren_den<1000)
   genie.WriteObject(GENIE_OBJ_METER,0x01, Tren_den);
   else
   genie.WriteObject(GENIE_OBJ_METER,0x01, 999);
   genie.WriteObject(GENIE_OBJ_TANK,0x00, int(Tos_den));
   genie_F0S6_text=DtoS(Tos_den,2);
   genie_F0S6_text+=" mm";
   genie.WriteStr(6,genie_F0S6_text);
   genie_F0S7_text=Tstr(tsd.hh,tsd.mm,tsd.ss); genie_F0S7_text=genie_F0S7_text.substring(0,5); 
   genie.WriteStr(7,genie_F0S7_text);
   
   // форм 3 - заводская
   genie.WriteObject(GENIE_OBJ_THERMOMETER,2, (int(zsd.temp)+20)); 
   genie.WriteObject(GENIE_OBJ_COOL_GAUGE,4, (int(zsd.p0)-720)); 
   genie.WriteObject(GENIE_OBJ_COOL_GAUGE,5, int(zsd.hum));
   int Zlux_gog;
   if (zsd.lux<=2000){
    Zlux_gog=zsd.lux/80;
   }
   if ((zsd.lux>2000) && (zsd.lux<=10000)){
    Zlux_gog=25+(zsd.lux-2000)/800;
   }
    if (zsd.lux>10000){
    Zlux_gog=35+(zsd.lux-10000)/2667;
   }
   genie.WriteObject(GENIE_OBJ_GAUGE,2, Zlux_gog); 
   genie.WriteObject(GENIE_OBJ_GAUGE, 3, int(zsd.os_mm));
   genie.WriteObject(GENIE_OBJ_USERIMAGES, 1,vet_nap_n(zsd.nos_napr));
   genie_F0S8_text=DtoS(zsd.nos_sil,2);
   genie_F0S8_text+=" m/s";
   genie.WriteStr(8,genie_F0S8_text); 
   genie.WriteObject(GENIE_OBJ_METER,2, zsd.radio*100);
   if (Zren_den<1000)
   genie.WriteObject(GENIE_OBJ_METER,3, Zren_den);
   else
   genie.WriteObject(GENIE_OBJ_METER,3, 999);
   genie.WriteObject(GENIE_OBJ_TANK,1, int(Zos_den));
   genie_F0S9_text=DtoS(Zos_den,2);
   genie_F0S9_text+=" mm";
   genie.WriteStr(9,genie_F0S9_text);
   genie_F0S10_text=Tstr(zsd.hh,zsd.mm,zsd.ss); genie_F0S10_text=genie_F0S10_text.substring(0,5); 
   genie.WriteStr(10,genie_F0S10_text);
   }
  

  // вывод на лсд
    if (LCD_change){ watchdogReset();
    LCD_change=FALSE;     
    lcd.clear();
    if(LCD_mode==0){ genie.WriteObject(GENIE_OBJ_STRINGS,0, 0); 
    lcd.noBacklight();}
    if(LCD_mode==1){ genie.WriteObject(GENIE_OBJ_STRINGS,0, 1); 
      lcd.backlight();
      wrt1="SiD MAIN Met Station";
      wrt2=Tstr(msd.hh,msd.mm,msd.ss)+" "+Dstr(msd.dow,msd.dd,msd.mon,msd.yyyy);
      out_LCD1();
      lcd.setCursor(0, 2);  
      lcd.print(DtoS(msd.temp,2)+String("*C"));
      lcd.setCursor(14, 2); 
      lcd.print(DtoS(msd.hum,2)+String("%"));
      lcd.setCursor(5, 3);
      lcd.print(DtoS(msd.p0,2)+String("mm.Hg"));
    }
    if(LCD_mode==2){ genie.WriteObject(GENIE_OBJ_STRINGS,0, 2); 
      lcd.backlight();
      
      if (tsd.yyyy==0){
        wrt1="SiD TOWN Met Station";
        wrt2="                    ";
        out_LCD1();
        lcd.setCursor(6, 1);  
        lcd.print("NO DATA");
        lcd.setCursor(4, 3);
        lcd.print(Dstr(msd.dow,msd.dd,msd.mon,msd.yyyy));
       } else{
      wrt1="SiD TOWN Met Station";
      wrt2=Tstr(tsd.hh,tsd.mm,tsd.ss)+" "+Dstr(tsd.dow,tsd.dd,tsd.mon,tsd.yyyy);
      out_LCD1();
      lcd.setCursor(0, 1);  
      lcd.print(DtoS(tsd.temp,1)+String("*C "));
      lcd.print(DtoS(tsd.hum,1)+String("% "));
      if (tsd.lux < 10000) { lcd.print(String(tsd.lux)+String("lx")); }
      else {  lcd.print(String(tsd.lux/1000)+String("Klx"));  }      
      lcd.setCursor(0, 2);
      lcd.print(DtoS(tsd.p0,1)+String("mm.Hg "));
      lcd.print(DtoS(tsd.radio,2)+String("uSv/h"));
      lcd.setCursor(0,3);
      lcd.write(byte(0));
      lcd.print(DtoS(tsd.nos_sil,1)+"m/s "+vet_nap(tsd.nos_napr)+" "+DtoS(tsd.os_mm,2)+"mm");
       }
    }
    if(LCD_mode==3){ genie.WriteObject(GENIE_OBJ_STRINGS,0, 3); 
     lcd.backlight();
      if (wsd.yyyy==0){
        wrt1="SiD WoRLD MetStation";
        wrt2="                    ";
        out_LCD1();
        lcd.setCursor(6, 1);  
        lcd.print("NO DATA");
        lcd.setCursor(4, 3);
        lcd.print(Dstr(msd.dow,msd.dd,msd.mon,msd.yyyy));
       } else{ 
        
      wrt1="SiD WoRLD MetStation";
      wrt2=Tstr(wsd.hh,wsd.mm,wsd.ss)+" "+Dstr(wsd.dow,wsd.dd,wsd.mon,wsd.yyyy);
      out_LCD1();
      lcd.setCursor(0, 1);  
      lcd.print(DtoS(wsd.temp,2)+String("*C"));
      lcd.setCursor(14, 1); 
      lcd.print(DtoS(wsd.hum,2)+String("%"));
      lcd.setCursor(0, 2);
      lcd.print(DtoS(wsd.p0,2)+String("mm.Hg"));
      lcd.setCursor(12, 2);
      if (wsd.lux < 10000) { lcd.print(String(wsd.lux)+String(" lux")); }
      else {  lcd.print(String(wsd.lux/1000)+String("K lux"));  }
      }
    }
    if(LCD_mode==4){ genie.WriteObject(GENIE_OBJ_STRINGS,0, 4); 
      lcd.backlight();
      
      if (zsd.yyyy==0){
        wrt1="SiD ZAVOD MetStation";
        wrt2="                    ";
        out_LCD1();
        lcd.setCursor(6, 1);  
        lcd.print("NO DATA");
        lcd.setCursor(4, 3);
        lcd.print(Dstr(msd.dow,msd.dd,msd.mon,msd.yyyy));
       } else{
      wrt1="SiD ZAVOD MetStation";
      wrt2=Tstr(zsd.hh,zsd.mm,zsd.ss)+" "+Dstr(zsd.dow,zsd.dd,zsd.mon,zsd.yyyy);
      out_LCD1();
      lcd.setCursor(0, 1);  
      lcd.print(DtoS(zsd.temp,1)+String("*C "));
      lcd.print(DtoS(zsd.hum,1)+String("% "));
      if (zsd.lux < 10000) { lcd.print(String(zsd.lux)+String("lx")); }
      else {  lcd.print(String(zsd.lux/1000)+String("Klx"));  }
      lcd.setCursor(0, 2);
      lcd.print(DtoS(zsd.p0,1)+String("mm.Hg "));
      lcd.print(DtoS(zsd.radio,2)+String("uSv/h"));
       lcd.setCursor(0,3);
       lcd.write(byte(0));
      lcd.print(DtoS(zsd.nos_sil,1)+"m/s "+vet_nap(zsd.nos_napr)+" "+DtoS(zsd.os_mm,2)+"mm ");
       }
    }
  }

  // Прием запросов и вывод веб-страниц

  WiFiEspClient client = server.available();  // listen for incoming clients
  
  if (client) {                               // if you get a client,
    SerialUSB.println("New client");          // print a message out the serial port
    buf.init();                               // initialize the circular buffer
    isPost=false;
    bufPost="";
    strn=0;
    while (client.connected()) { watchdogReset();  // loop while the client's connected
      if (client.available()) {  watchdogReset();  // if there's bytes to read from the client,
        char c = client.read();                    // read a byte, then
        buf.push(c);                               // push it to the ring buffer
        if (isPost) {
          bufPost+=String(c);
        }
        
        // printing the stream to the serial monitor will slow down
        // the receiving of data from the ESP filling the serial buffer
        SerialUSB.write(c);
        
        // you got two newline characters in a row
        // that's the end of the HTTP request, so send a response
          if (buf.endsWith("\r\n\r\n")) {
          if (isPost && (bufPost.indexOf('}')!=-1)) {
          String ans = String("654321\r\n");
          client.print(ans);  
          if (bufPost.startsWith("MT")) {
            tsd=StoD(bufPost);
            if (!((prevt_hh==tsd.hh) && (prevt_mm==tsd.mm) && (prevt_ss==tsd.ss) && (prevt_dd==tsd.dd) && (prevt_mon==tsd.mon) && (prevt_yyyy==tsd.yyyy))) {
//                      SerialUSB.println(bufPost);
            prevt_hh=tsd.hh; prevt_mm=tsd.mm; prevt_ss=tsd.ss; prevt_dd=tsd.dd; prevt_mon=tsd.mon; prevt_yyyy=tsd.yyyy;
            flSD=String("/MT/")+SDdat(tsd.dd, tsd.mon, tsd.yyyy)+String(".MT");
            Tren_den=int(tsd.radio*100)/6+Tren_den;
            Tos_den+=tsd.os_mm;
            myWFile = SD.open(flSD, FILE_WRITE);
            myWFile.println(bufPost);
            myWFile.close(); 
  
            if(LCD_mode==2){ LCD_change=TRUE; }
            }
          };
          if (bufPost.startsWith("MZ")) {
            zsd=StoD(bufPost);
            if (!((prevt_hh==zsd.hh) && (prevt_mm==zsd.mm) && (prevt_ss==zsd.ss) && (prevt_dd==zsd.dd) && (prevt_mon==zsd.mon) && (prevt_yyyy==zsd.yyyy))) {
//                      SerialUSB.println(bufPost);
            prevt_hh=zsd.hh; prevt_mm=zsd.mm; prevt_ss=zsd.ss; prevt_dd=zsd.dd; prevt_mon=zsd.mon; prevt_yyyy=zsd.yyyy;
            flSD=String("/MZ/")+SDdat(zsd.dd, zsd.mon, zsd.yyyy)+String(".MZ");
            Zren_den=int(zsd.radio*100)/6+Zren_den;
            Zos_den+=zsd.os_mm;
            myWFile = SD.open(flSD, FILE_WRITE);
            myWFile.println(bufPost);
            myWFile.close(); 
  
            if(LCD_mode==4){ LCD_change=TRUE; }
            }
          };
          break;   
          } else { watchdogReset();
          if (strn==0)  {  sendmain_page(client);           break; }
          if (strn==1)  {  sendmain_met_page(client);       break; }
          if (strn==2)  {  sendtown_met_page(client);       break; }
          if (strn==3)  {  send_his1_met_page(client, "MT");  break; }
          if (strn==4)  {  sendmain_his1_met_page(client);  break; }
          if (strn==5)  {  sendworld_met_page(client);  break; }
          if (strn==6)  {  sendworld_his1_met_page(client);  break; }
          if (strn==7)  {  sendservice_page(client);  break; }
          if (strn==8)  {  send_his5_met_page(client,"MM");  break; }
          if (strn==9)  {  sendzavod_met_page(client);  break; }
          if (strn==10)  {  send_his1_met_page(client, "MZ");  break; }
          if (strn==11)  {  sendhis_met_page(client);  break; }
          if (strn==12)  {  send_his5_met_page(client,"MT");  break; }
          if (strn==13)  {  send_his5_met_page(client,"MZ");  break; }
          if (strn==14)  {  send_his5_met_page(client,"WT");  break; }          
          }
        }

        // Check to see if the client request was "GET /...<C>
        if (buf.endsWith("POST")) {
         isPost=true;
        }
        if (buf.endsWith("GET /H")) { strn=1; }  // страница главной метео станции
        if (buf.endsWith("GET /L")) { strn=2; }  // страница городской метео станции
        if (buf.endsWith("GET /B")) { strn=5; }  // страница походной метео станции        
        if (buf.endsWith("GET /S")) { strn=0; }  // начальная страница
        if (buf.endsWith("GET /C")) { strn=3; }  // страница истории городской метео станции
        if (buf.endsWith("GET /A")) { strn=4; }  // страница истории главной метео станции
        if (buf.endsWith("GET /D")) { strn=6; }  // страница истории походной метео станции
        if (buf.endsWith("GET /I")) { strn=7; }  // страница системная
        if (buf.endsWith("GET /E")) { strn=8; }  // страница истории за 5 glavnoy
        if (buf.endsWith("GET /F")) { strn=9; }  // страница заводской метео станции
        if (buf.endsWith("GET /G")) { strn=10; }  // страница истории заводской метео станции
        if (buf.endsWith("GET /J")) { strn=11; }  // страница историЙ
        if (buf.endsWith("GET /K")) { strn=12; }  // страница истории за 5 gorodskoy
        if (buf.endsWith("GET /M")) { strn=13; }  // страница истории за 5 zavodskoy
        if (buf.endsWith("GET /N")) { strn=14; }  // страница истории за 5 pohodnoy
        
     }
    }
    
    // close the connection
    client.stop();
    //SerialUSB.println("Client disconnected");
  }
  yield(); 
 }


// проигрование сообщения 
void play(String pl){ watchdogReset();
      String r;
          GSMport.println("AT+CPAMR=\""+pl+".amr\",0");
          r=GSM_buf();
          SerialUSB.println("AMR started 1"); SerialUSB.println(r);
          r=GSM_buf();
          SerialUSB.println("AMR started 2"); SerialUSB.println(r);
      watchdogReset(); }  

String voice_cmd[10];
int scht_str=0;

/*********************/
int sklon(int dd)    /* Определение склонения числа */
{ int n, r;
 
    r = 2;
    n = dd % 10;  
    if      (n==1)       r = 0;
    else if (n>1 && n<5) r = 1;
    return(r);
}
/* ------------------ */
void Propis(unsigned long L) // Число прописью
{ int R, ns, nd, r; unsigned long M; String des;
 String tus[]={"1000a","1000i","1000"};
   scht_str=0;
   if (L==0) {
      des=String(L);
      scht_str++; 
      voice_cmd[scht_str]=des;
    return;
   }
   M = L / 1000;   // тісяці
   if(M!=0){
   r=sklon(M);
   
   ns = M / 100;  // Сотни
   if(ns!=0){
   des=String(ns*100);
   scht_str++;
   voice_cmd[scht_str]=des;
   }
   M = M % 100;  // < 100
   nd = M / 10;  // Десятки
   if (nd >= 2) {
      des=String(nd*10);
      scht_str++;
      voice_cmd[scht_str]=des;
      M = M % 10;
   }
   if(M>2){
   des=String(M);
   scht_str++;
   voice_cmd[scht_str]=des;
   }
   if(M==2){
   scht_str++;
   voice_cmd[scht_str]="2e";
   }
   scht_str++;
   voice_cmd[scht_str]=tus[r];
   }
   
   R = L % 1000;
   ns = R / 100;  // Сотни
   if(ns!=0){
   des=String(ns*100);
   scht_str++;
   voice_cmd[scht_str]=des;
   }
   R = R % 100;  // < 100
   nd = R / 10;  // Десятки
   if (nd >= 2) {
      des=String(nd*10);
      scht_str++;
      voice_cmd[scht_str]=des;
      R = R % 10;
   }
   if(R!=0){
   des=String(R);
   scht_str++;
   voice_cmd[scht_str]=des;
   }
}

// ответ на входящий звонок
void loop2() { watchdogReset();
  unsigned long per;
  String gr[]={"gr","gra","grov"};
  String pr[]={"pr","pra","prov"};
  String hg[]={"hg","hga","hgov"};
  String ms[]={"ms","msa","msov"};
  String mm[]={"mm","mma","mmov"};
  String wt[]={"n","no","o","so","s","sw","w","nw"};
 delay(100);
 if(on_ring==1){
    SerialUSB.print("In l2 onring="); SerialUSB.println(on_ring);
    String r=GSM_buf();
    int ch=0;
    SerialUSB.println(r);
    if (r.indexOf("RING")!=-1){
          SerialUSB.println("Input noempty"); //-----<
          digitalWrite(RSPin, HIGH);
          GSMport.println("ATA");
          r=GSM_buf();
          SerialUSB.println(r);
          play("start");
          for (int i=0; i<50; i++) { watchdogReset();
              delay(100); 
              r=GSM_buf();        
              if (r.indexOf("DTMF")!=-1){
               SerialUSB.println(r); 
               String d = r.substring(r.indexOf("DTMF")+5); 
               SerialUSB.println(d.toInt()); 
               ch=d.toInt();
               if (ch==1){
                play("gorod");
                play("t");
                if(tsd.temp<0.0){
                  play("minus");
                }
                per=long(tsd.temp);
                Propis(per);
                for(int i=1;i<=scht_str;i++){
                    play(voice_cmd[i]);  
                }
                play(gr[sklon(per)]);
                play("h");
                per=long(tsd.hum);
                Propis(per);
                for(int i=1;i<=scht_str;i++){
                    play(voice_cmd[i]);  
                }
                play(pr[sklon(per)]);
                play("p");
                per=long(tsd.p0);
                Propis(per);
                for(int i=1;i<=scht_str;i++){
                    play(voice_cmd[i]);  
                }
                play(hg[sklon(per)]);
                play("l");
                 per=long(tsd.lux);
                Propis(per);
                for(int i=1;i<=scht_str;i++){
                    play(voice_cmd[i]);  
                }
                play("lux");
                play("v");
                per=long(tsd.nos_sil);
                Propis(per);
                for(int i=1;i<=scht_str;i++){
                    play(voice_cmd[i]);  
                }
                play(ms[sklon(per)]);
                play(wt[vet_nap_n(tsd.nos_napr)]);
                play("r");
                per=long(tsd.radio*100);
                play("0");
                if(per<10){
                  play("0");
                }
                Propis(per);
                for(int i=1;i<=scht_str;i++){
                    play(voice_cmd[i]);  
                }
                play("mz");
                play("os");
                per=long(tsd.os_mm);
                Propis(per);
                for(int i=1;i<=scht_str;i++){
                    play(voice_cmd[i]);  
                }
                play(mm[sklon(per)]);               
                break;
               }
               else if (ch==2){
                play("oblast");
                play("t");
                if(zsd.temp<0.0){
                  play("minus");
                }
                per=long(zsd.temp);
                Propis(per);
                for(int i=1;i<=scht_str;i++){
                    play(voice_cmd[i]);  
                }
                play(gr[sklon(per)]);
                play("h");
                per=long(zsd.hum);
                Propis(per);
                for(int i=1;i<=scht_str;i++){
                    play(voice_cmd[i]);  
                }
                play(pr[sklon(per)]);
                play("p");
                per=long(zsd.p0);
                Propis(per);
                for(int i=1;i<=scht_str;i++){
                    play(voice_cmd[i]);  
                }
                play(hg[sklon(per)]);
                play("l");
                 per=long(zsd.lux);
                Propis(per);
                for(int i=1;i<=scht_str;i++){
                    play(voice_cmd[i]);  
                }
                play("lux");
                play("v");
                per=long(zsd.nos_sil);
                Propis(per);
                for(int i=1;i<=scht_str;i++){
                    play(voice_cmd[i]);  
                }
                play(ms[sklon(per)]);
                play(wt[vet_nap_n(zsd.nos_napr)]);
                play("r");
                per=long(zsd.radio*100);
                play("0");
                if(per<10){
                  play("0");
                }
                Propis(per);
                for(int i=1;i<=scht_str;i++){
                    play(voice_cmd[i]);  
                }
                play("mz");
                play("os");
                per=long(zsd.os_mm);
                Propis(per);
                for(int i=1;i<=scht_str;i++){
                    play(voice_cmd[i]);  
                }
                play(mm[sklon(per)]);                       
                break;
               }
               break;
              }
             }
             play("end");
             delay(100);       
             GSMport.println("ATH");
             r=GSM_buf();
             digitalWrite(RSPin, LOW);
             on_ring=0;
          }
    SerialUSB.println("In l2 flag to zero"); //-----<      
    on_ring=0;      
    }
 yield();
}


