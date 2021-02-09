// +---------------------------------------------------------------------------+
// | MM6D v0.2 * Remote controlled switching device                            |
// | Copyright (C) 2020-2021 Pozsár Zsolt <pozsar.zsolt@szerafingomba.hu>      |
// | mm6d.ino                                                                  |
// | Program for ESP8266 Huzzah Feather                                        |
// +---------------------------------------------------------------------------+

//   This program is free software: you can redistribute it and/or modify it
// under the terms of the European Union Public License 1.1 version.
//
//   This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.

#include <ESP8266WebServer.h>
#include <StringSplitter.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

// settings
const String serialnumber   = "";  // serial number
const char* wifi_ssid       = "";  // SSID of Wi-Fi AP
const char* wifi_password   = "";  // password of Wi-Fi AP
const String uid            = "";  // user ID
const String allowedaddress = "";  // client IP addresses with space delimiter

// GPIO ports
const int prt_buzzer        = 4;
const int prt_in_ocprot     = 13;
const int prt_in_opmode     = 5;
const int prt_in_swmanu     = 12;
const int prt_led_blue      = 2;
const int prt_led_red       = 0;
const int prt_out_heat      = 16;
const int prt_out_lamp      = 14;
const int prt_out_vent      = 15;

// ADC input
const int prt_in_adc        = 0;

// messages
const String msg01          = "MM6D * Remote controlled switching device";
const String msg02          = "Copyright (C) 2020-2021";
const String msg03          = "pozsar.zsolt@szerafingomba.hu";
const String msg04          = "http://www.szerafingomba.hu/equipments/";
const String msg05          = "* Initializing GPIO ports...";
const String msg06          = "* Initializing sensors...";
const String msg07          = "* Connecting to wireless network";
const String msg08          = "done.";
const String msg09          = "  my IP address:      ";
const String msg10          = "  subnet mask:        ";
const String msg11          = "  gateway IP address: ";
const String msg12          = "* Starting webserver...";
const String msg13          = "* HTTP request received from: ";
const String msg14          = "* E01: Control timeout error!";
const String msg15          = "* Alarm event detected!";
const String msg16          = "MM6D";
const String msg17          = "Authentication error!";
const String msg18          = "* E03: Authentication error!";
const String msg19          = "Not allowed client IP address!";
const String msg20          = "* E04: Not allowed client IP address!";
const String msg21          = "Page not found!";
const String msg22          = "* E05: Page not found!";
const String msg23          = "* E02: Overcurrent protection error!";
const String msg24          = "  heater is switched";
const String msg25          = "  lamp is switched";
const String msg26          = "  ventilator is switched";
const String msg27          = "Done.";
const String msg28          = "Pozsar Zsolt";
const String msg29          = "  device MAC address: ";
const String msg30          = "  alarm input is restored";
const String msg31          = " off.";
const String msg32          = " on.";
const String msg33          = "Serial number of hardware: ";
const String msg34          = "  get homepage";
const String msg35          = "  get version data";
const String msg36          = "  get all data, restore alarm and set outputs";
const String msg37          = "  get all data";
const String msg38          = "  get alarm status";
const String msg39          = "  get operation mode";
const String msg40          = "  get position of manual switch";
const String msg41          = "  get status of overcurrent protection";
const String msg42          = "  all output is switched off";
const String msg43          = "  status:";
const String msg44          = "    alarm:\t\t";
const String msg45          = "    operation mode:\t";
const String msg46          = "    manual switch:\t";
const String msg47          = "    protection:\t";

// general constants
const int alarmminlevel     = 0;
const int alarmmaxlevel     = 512;
const int interval          = 60000;
const String swversion      = "0.2";

// variables
int adcvalue;
int alarm                   = 0;
int alarmsign;
int error                   = 0;
int g;
int h1, h2, h3, h4;
int heat                    = 0;
int lamp                    = 0;
int ocprot;
int ocprotsign;
int opmode;
int swmanu;
int t1, t2, t3, t4;
int timeout;
int timeoutsign;
int vent                    = 0;
String clientaddress;
String devicemacaddress;
String line;
String localipaddress;
unsigned long currtime;
unsigned long prevtime      = 0;

ESP8266WebServer server(80);

// initializing function
void setup(void)
{
  // setting serial port
  Serial.begin(115200);
  Serial.println("");
  Serial.println("");
  Serial.println(msg01 + " * v" + swversion );
  Serial.println(msg02 +  " " + msg28 + " <" + msg03 + ">");
  Serial.println(msg33 + serialnumber );
  // initializing ports
  Serial.print(msg05);
  pinMode(prt_buzzer, OUTPUT);
  pinMode(prt_in_swmanu, INPUT);
  pinMode(prt_in_opmode, INPUT);
  pinMode(prt_in_ocprot, INPUT);
  pinMode(prt_led_blue, OUTPUT);
  pinMode(prt_led_red, OUTPUT);
  pinMode(prt_out_heat, OUTPUT);
  pinMode(prt_out_lamp, OUTPUT);
  pinMode(prt_out_vent, OUTPUT);
  portwrite();
  digitalWrite(prt_led_blue, LOW);
  Serial.println(msg08);
  // connect to wireless network
  Serial.print(msg07);
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(300);
    Serial.print(".");
  }
  Serial.println(msg08);
  localipaddress = WiFi.localIP().toString();
  devicemacaddress = WiFi.macAddress();
  Serial.println(msg29 + devicemacaddress);
  Serial.println(msg09 + localipaddress);
  Serial.println(msg10 + WiFi.subnetMask().toString());
  Serial.println(msg11 + WiFi.gatewayIP().toString());
  // start webserver
  Serial.print(msg12);
  server.onNotFound(handleNotFound);
  server.on("/", []()
  {
    writeclientipaddress();
    Serial.println(msg34);
    line = "<html>\n"
           "  <head>\n"
           "    <title>" + msg01 + "</title>\n"
           "  </head>\n"
           "  <body bgcolor=\"#e2f4fd\">\n"
           "    <h2>" + msg01 + "</h2>\n"
           "    <br>\n"
           "    Hardware serial number: " + serialnumber + "<br>\n"
           "    Software version: v" + swversion + "<br>\n"
           "    <hr>\n"
           "    <h3>Plain text data and control pages:</h3>\n"
           "    <br>\n"
           "    <table border=\"0\" cellpadding=\"5\">\n"
           "      <tr>\n"
           "        <td>http://" + localipaddress + "/version</td>\n"
           "        <td>Get software name and version</td>\n"
           "      </tr>\n"
           "      <tr><td colspan=\"2\">&nbsp;</td></tr>\n"
           "      <tr>\n"
           "        <td>http://" + localipaddress + "/operation?uid=abcdef&a=0&h=0&l=0&v=0</td>\n"
           "        <td>Get all data, restore alarm and switch on/off outputs</td>\n"
           "      </tr>\n"
           "      <tr><td colspan=\"2\">&nbsp;</td></tr>\n"
           "      <tr>\n"
           "        <td>http://" + localipaddress + "/get/all?uid=abcdef</td>\n"
           "        <td>Get all status</td>\n"
           "      </tr>\n"
           "      <tr>\n"
           "        <td>http://" + localipaddress + "/get/alarm?uid=abcdef</td>\n"
           "        <td>Get status of alarm sensors</td>\n"
           "      </tr>\n"
           "      <tr>\n"
           "        <td>http://" + localipaddress + "/get/manualswitch?uid=abcdef</td>\n"
           "        <td>Get status of manual switch</td>\n"
           "      </tr>\n"
           "      <tr>\n"
           "        <td>http://" + localipaddress + "/get/operationmode?uid=abcdef</td>\n"
           "        <td>Get operation mode</td>\n"
           "      </tr>\n"
           "      <tr>\n"
           "        <td>http://" + localipaddress + "/get/protection?uid=abcdef</td>\n"
           "        <td>Get status of overcurrent protection</td>\n"
           "      </tr>\n"
           "      <tr><td colspan=\"2\">&nbsp;</td></tr>\n"
           "      <tr>\n"
           "        <td>http://" + localipaddress + "/set/all/off?uid=abcdef</td>\n"
           "        <td>Switch off all outputs</td>\n"
           "      </tr>\n"
           "      <tr>\n"
           "        <td>http://" + localipaddress + "/set/alarm/off?uid=abcdef</td>\n"
           "        <td>Restore alarm input</td>\n"
           "      </tr>\n"
           "      <tr>\n"
           "        <td>http://" + localipaddress + "/set/heater/off?uid=abcdef</td>\n"
           "        <td>Switch off heater</td>\n"
           "      </tr>\n"
           "      <tr>\n"
           "        <td>http://" + localipaddress + "/set/heater/on?uid=abcdef</td>\n"
           "        <td>Switch on heater</td>\n"
           "      </tr>\n"
           "      <tr>\n"
           "        <td>http://" + localipaddress + "/set/lamp/off?uid=abcdef</td>\n"
           "        <td>Switch off lamp</td>\n"
           "      </tr>\n"
           "      <tr>\n"
           "        <td>http://" + localipaddress + "/set/lamp/on?uid=abcdef</td>\n"
           "        <td>Switch on lamp</td>\n"
           "      </tr>\n"
           "      <tr>\n"
           "        <td>http://" + localipaddress + "/set/ventilator/off?uid=abcdef</td>\n"
           "        <td>Switch off ventilator</td>\n"
           "      </tr>\n"
           "      <tr>\n"
           "        <td>http://" + localipaddress + "/set/ventilator/on?uid=abcdef</td>\n"
           "        <td>Switch on ventilator</td>\n"
           "      </tr>\n"
           "    </table>\n"
           "    <br>\n"
           "    <hr>\n"
           "    <center>" + msg02 + " <a href=\"mailto:" + msg03 + "\">" + msg28 + "</a> - <a href=\"" + msg04 + "\">Homepage</a><center>\n"
           "    <br>\n"
           "  <body>\n"
           "</html>\n";
    server.send(200, "text/html", line);
    delay(100);
  });
  server.on("/version", []()
  {
    writeclientipaddress();
    Serial.println(msg35);
    line = msg16 + "\n" + swversion + "\n" + serialnumber;
    server.send(200, "text/plain", line);
    delay(100);
  });
  server.on("/operation", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg36);
        Serial.println(msg37);
        Serial.println(msg43);
        Serial.println(msg44 + String((int)alarm));
        Serial.println(msg45 + String((int)opmode));
        Serial.println(msg46 + String((int)swmanu));
        Serial.println(msg47 + String((int)ocprot));
        prevtime = millis();
        line = String((int)alarm) + "\n" + String((int)opmode) + "\n" +  String((int)swmanu) + "\n" + String((int)ocprot);
        server.send(200, "text/plain", line);
        String arg;
        arg = server.arg("a");
        if (arg.length() != 0)
          if ( arg == "0") alarm = 0;
        arg = server.arg("h");
        if (arg.length() != 0)
        {
          if ( arg == "0") heat = 0;
          if ( arg == "1") heat = 1;
        }
        arg = server.arg("l");
        if (arg.length() != 0)
        {
          if ( arg == "0") lamp = 0;
          if ( arg == "1") lamp = 1;
        }
        arg = server.arg("v");
        if (arg.length() != 0)
        {
          if ( arg == "0") vent = 0;
          if ( arg == "1") vent = 1;
        }
      }
  });
  server.on("/get/all", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg37);
        Serial.println(msg43);
        Serial.println(msg44 + String((int)alarm));
        Serial.println(msg45 + String((int)opmode));
        Serial.println(msg46 + String((int)swmanu));
        Serial.println(msg47 + String((int)ocprot));
        prevtime = millis();
        line = String((int)alarm) + "\n" + String((int)opmode) + "\n" +  String((int)swmanu) + "\n" + String((int)ocprot);
        server.send(200, "text/plain", line);
      }
  });
  server.on("/get/alarm", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg38);
        Serial.println(msg43);
        Serial.println(msg44 + String((int)alarm));
        prevtime = millis();
        line = String((int)alarm);
        server.send(200, "text/plain", line);
      }
  });
  server.on("/get/operationmode", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg39);
        Serial.println(msg43);
        Serial.println(msg45 + String((int)opmode));
        prevtime = millis();
        line = String((int)opmode);
        server.send(200, "text/plain", line);
      }
  });
  server.on("/get/manualswitch", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg40);
        Serial.println(msg43);
        Serial.println(msg46 + String((int)swmanu));
        prevtime = millis();
        line = String((int)swmanu);
        server.send(200, "text/plain", line);
      }
  });
  server.on("/get/protection", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg41);
        Serial.println(msg43);
        Serial.println(msg47 + String((int)ocprot));
        prevtime = millis();
        line = String((int)ocprot);
        server.send(200, "text/plain", line);
      }
  });
  server.on("/set/all/off", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg42);
        prevtime = millis();
        heat = 0;
        lamp = 0;
        vent = 0;
        server.send(200, "text/plain", msg27);
      }
  });
  server.on("/set/alarm/off", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg30);
        prevtime = millis();
        alarm = 0;
        server.send(200, "text/plain", msg27);
      }
  });
  server.on("/set/heater/off", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg24 + msg31);
        prevtime = millis();
        heat = 0;
        server.send(200, "text/plain", msg27);
      }
  });
  server.on("/set/heater/on", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg24 + msg32);
        prevtime = millis();
        heat = 1;
        server.send(200, "text/plain", msg27);
      }
  });
  server.on("/set/lamp/off", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg25 + msg31);
        prevtime = millis();
        lamp = 0;
        server.send(200, "text/plain", msg27);
      }
  });
  server.on("/set/lamp/on", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg25 + msg32);
        prevtime = millis();
        lamp = 1;
        server.send(200, "text/plain", msg27);
      }
  });
  server.on("/set/ventilator/off", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg26 + msg31);
        prevtime = millis();
        vent = 0;
        server.send(200, "text/plain", msg27);
      }
  });
  server.on("/set/ventilator/on", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg26 + msg32);
        prevtime = millis();
        vent = 1;
        server.send(200, "text/plain", msg27);
      }
  });
  server.begin();
  Serial.println(msg08);
}

// error 404
void handleNotFound()
{
  server.send(404, "text/plain", msg21);
  Serial.println(msg22);
}

// loop function
void loop(void)
{
  server.handleClient();
  // detect timeout
  currtime = millis();
  if (currtime - prevtime >= interval)
  {
    timeout = 1;
    digitalWrite(prt_led_blue, HIGH);
  } else
  {
    timeout = 0;
    digitalWrite(prt_led_blue, LOW);
  }
  // get status
  portread();
  delay(100);
  // alarm
  if ((adcvalue < alarmminlevel) || (adcvalue > alarmmaxlevel)) alarm = 1;
  // warning and error
  error = 0;
  if ((swmanu == 1) || (ocprot == 1) || (alarm == 1) || (timeout == 1)) error = 1;
  // messages
  if (timeout == 0) timeoutsign = 0;
  if ((timeout == 1) && (timeoutsign == 0))
  {
    Serial.println(msg14);
    timeoutsign = 1;
  }
  if (alarm == 0) alarmsign = 0;
  if ((alarm == 1) && (alarmsign == 0))
  {
    Serial.println(msg15);
    alarmsign = 1;
  }
  if (ocprot == 0) ocprotsign = 0;
  if ((ocprot == 1) && (ocprotsign == 0))
  {
    Serial.println(msg23);
    ocprotsign = 1;
  }
  // error sound
  if ((ocprot == 1) || (alarm == 1)) beep(1);
  portwrite();
}

// blink blue LED and write client IP address to serial console
void writeclientipaddress()
{
  digitalWrite(prt_led_blue, HIGH);
  delay(500);
  digitalWrite(prt_led_blue, LOW);
  clientaddress = server.client().remoteIP().toString();
  Serial.println(msg13 + clientaddress + ".");
}

// read input ports
void portread()
{
  adcvalue = analogRead(prt_in_adc);
  swmanu = digitalRead(prt_in_swmanu);
  ocprot = digitalRead(prt_in_ocprot);
  opmode = digitalRead(prt_in_opmode);
}

// write output ports
void portwrite()
{
  digitalWrite(prt_led_red, error);
  digitalWrite(prt_out_lamp, lamp);
  digitalWrite(prt_out_vent, vent);
  digitalWrite(prt_out_heat, heat);
}

// check IP address of client
int checkipaddress()
{
  int allowed = 0;
  writeclientipaddress();
  StringSplitter *splitter = new StringSplitter(allowedaddress, ' ', 3);
  int itemCount = splitter->getItemCount();
  for (int i = 0; i < itemCount; i++)
  {
    String item = splitter->getItemAtIndex(i);
    if (clientaddress == String(item)) allowed = 1;
  }
  if (allowed == 1) return 1; else
  {
    server.send(401, "text/plain", msg19);
    Serial.println(msg20);
    beep(3);
    return 0;
  }
}

// authentication
int checkuid()
{
  if (server.arg("uid") == uid) return 1; else
  {
    server.send(401, "text/plain", msg17);
    Serial.println(msg18);
    beep(2);
    return 0;
  }
}

// beep sign
void beep(int num)
{
  for (int i = 0; i < num; i++)
  {
    tone(prt_buzzer, 880);
    delay (100);
    noTone(prt_buzzer);
    delay (100);
  }
}
