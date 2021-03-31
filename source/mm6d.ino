// +---------------------------------------------------------------------------+
// | MM6D v0.3 * Remote controlled switching device                            |
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
const String  serialnumber      = "";  // serial number
const char   *wifi_ssid         = "";  // SSID of Wi-Fi AP
const char   *wifi_password     = "";  // password of Wi-Fi AP
const String  uid               = "";  // user ID
const String  allowedaddress    = "";  // client IP addresses with space delimiter
// GPIO ports
const int     prt_buzzer        = 4;
const int     prt_in_ocprot     = 13;
const int     prt_in_opmode     = 5;
const int     prt_in_swmanu     = 12;
const int     prt_led_blue      = 2;
const int     prt_led_red       = 0;
const int     prt_out_heat      = 16;
const int     prt_out_lamp      = 14;
const int     prt_out_vent      = 15;
// ADC input
const int     prt_in_adc        = 0;
// general constants
const int     alarmminlevel     = 0;
const int     alarmmaxlevel     = 512;
const int     interval          = 60000;
const String  texthtml          = "text/html";
const String  textplain         = "text/plain";
const String  swversion         = "0.3";
// general variables
int           adcvalue;
int           alarm             = 0;
int           alarmsign;
int           error             = 0;
int           g;
int           heat              = 0;
int           lamp              = 0;
int           ocprot;
int           ocprotsign;
int           opmode;
int           syslog[64]        = {};
int           swmanu;
int           timeout;
int           timeoutsign;
int           vent              = 0;
String        clientaddress;
String        deviceipaddress;
String        devicemacaddress;
String        line;
unsigned long currtime;
unsigned long prevtime          = 0;
// messages
String msg[60]                  =
{
  /*  0 */  "",
  /*  1 */  "MM6D * Remote controlled switching device",
  /*  2 */  "Copyright (C) 2020-2021",
  /*  3 */  "pozsar.zsolt@szerafingomba.hu",
  /*  4 */  "http://www.szerafingomba.hu/equipments/",
  /*  5 */  "* Initializing GPIO ports...",
  /*  6 */  "* Initializing sensors...",
  /*  7 */  "* Connecting to wireless network",
  /*  8 */  "done.",
  /*  9 */  "  my IP address:      ",
  /* 10 */  "  subnet mask:        ",
  /* 11 */  "  gateway IP address: ",
  /* 12 */  "* Starting webserver...",
  /* 13 */  "* HTTP request received from: ",
  /* 14 */  "* E01: Control timeout error!",
  /* 15 */  "* Alarm event detected!",
  /* 16 */  "MM6D",
  /* 17 */  "Authentication error!",
  /* 18 */  "* E03: Authentication error!",
  /* 19 */  "Not allowed client IP address!",
  /* 20 */  "* E04: Not allowed client IP address!",
  /* 21 */  "Page not found!",
  /* 22 */  "* E05: Page not found!",
  /* 23 */  "* E02: Overcurrent protection error!",
  /* 24 */  "  heater is switched",
  /* 25 */  "  lamp is switched",
  /* 26 */  "  ventilator is switched",
  /* 27 */  "Done.",
  /* 28 */  "Pozsar Zsolt",
  /* 29 */  "  device MAC address: ",
  /* 30 */  "  alarm input is restored",
  /* 31 */  " off.",
  /* 32 */  " on.",
  /* 33 */  "Serial number of hardware: ",
  /* 34 */  "  get homepage",
  /* 35 */  "  get device information",
  /* 36 */  "  get all data, restore alarm and set outputs",
  /* 37 */  "  get all data",
  /* 38 */  "  get alarm status",
  /* 39 */  "  get status of operation mode swith",
  /* 40 */  "  get status of manual switches",
  /* 41 */  "  get status of overcurrent protection",
  /* 42 */  "  all output is switched off",
  /* 43 */  "  status:",
  /* 44 */  "    alarm:\t\t",
  /* 45 */  "    operation mode:\t",
  /* 46 */  "    manual switch:\t",
  /* 47 */  "    protection:\t",
  /* 48 */  "* HTTP request received.",
  /* 49 */  "  get summary",
  /* 50 */  "  get system log",
  /* 51 */  "  get status of lamp output",
  /* 52 */  "  get status of ventilator output",
  /* 53 */  "  get status of heater output",
  /* 54 */  "    lamp:\t\t",
  /* 55 */  "    ventilator:\t\t",
  /* 56 */  "    heater:\t\t",
  /* 57 */  "  set status of lamp output",
  /* 58 */  "  set status of ventilator output",
  /* 59 */  "  set status of heater output",
};

ESP8266WebServer server(80);

// initializing function
void setup(void)
{
  // set serial port
  Serial.begin(115200);
  // write program information
  Serial.println("");
  Serial.println("");
  Serial.println(msg[1] + " * v" + swversion );
  Serial.println(msg[2] +  " " + msg[28] + " <" + msg[3] + ">");
  Serial.println(msg[33] + serialnumber );
  // initialize GPIO ports
  writesyslog(5);
  Serial.print(msg[5]);
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
  Serial.println(msg[8]);
  // connect to wireless network
  writesyslog(7);
  Serial.print(msg[7]);
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(300);
    Serial.print(".");
  }
  Serial.println(msg[8]);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  deviceipaddress = WiFi.localIP().toString();
  devicemacaddress = WiFi.macAddress();
  Serial.println(msg[29] + devicemacaddress);
  Serial.println(msg[9] + deviceipaddress);
  Serial.println(msg[10] + WiFi.subnetMask().toString());
  Serial.println(msg[11] + WiFi.gatewayIP().toString());
  // start webserver
  writesyslog(12);
  Serial.print(msg[12]);
  server.onNotFound(handleNotFound);

  // Group #1: information pages
  // write help page
  server.on("/", []()
  {
    writeclientipaddress();
    Serial.println(msg[34]);
    writesyslog(34);
    line =
      "<html>\n"
      "  <head>\n"
      "    <title>" + msg[1] + " | Help page</title>\n"
      "  </head>\n"
      "  <body bgcolor=\"#e2f4fd\" style=\"font-family:\'sans\'\">\n"
      "    <h2>" + msg[1] + "</h2>\n"
      "    <br>\n"
      "    IP address: " + deviceipaddress + "<br>\n"
      "    MAC address: " + devicemacaddress + "<br>\n"
      "    Hardware serial number: " + serialnumber + "<br>\n"
      "    Software version: v" + swversion + "<br>\n"
      "    <hr>\n"
      "    <h3>Pages:</h3>\n"
      "    <table border=\"1\" cellpadding=\"3\" cellspacing=\"0\">\n"
      "      <tr><td colspan=\"3\" align=\"center\"><b>Information pages</b></td></tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "</td>\n"
      "        <td>This page</td>\n"
      "        <td>" + texthtml + "</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/summary?uid=abcdef</td>\n"
      "        <td>Summary of status</td>\n"
      "        <td>" + texthtml + "</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/log?uid=abcdef</td>\n"
      "        <td>System log</td>\n"
      "        <td>" + texthtml + "</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/version</td>\n"
      "        <td>Device information</td>\n"
      "        <td>" + textplain + "</td>\n"
      "      </tr>\n"
      "      <tr><td colspan=\"3\" align=\"center\"><b>Alarm</b></td></tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/get/alarm?uid=abcdef</td>\n"
      "        <td>Get alarm status</td>\n"
      "        <td>" + textplain + "</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/set/alarm/off?uid=abcdef</td>\n"
      "        <td>Restore alarm status</td>\n"
      "        <td>" + textplain + "</td>\n"
      "      </tr>\n"
      "      <tr><td colspan=\"3\" align=\"center\"><b>Get data</b></td></tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/get/all?uid=abcdef</td>\n"
      "        <td>Get all status</td>\n"
      "        <td>" + textplain + "</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/get/operationmode?uid=abcdef</td>\n"
      "        <td>Get status of operation mode switch</td>\n"
      "        <td>" + textplain + "</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/get/manualswitch?uid=abcdef</td>\n"
      "        <td>Get status of manual mode switches</td>\n"
      "        <td>" + textplain + "</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/get/protection?uid=abcdef</td>\n"
      "        <td>Get status of overcurrent protection</td>\n"
      "        <td>" + textplain + "</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/get/lamp?uid=abcdef</td>\n"
      "        <td>Get status of lamp output</td>\n"
      "        <td>" + textplain + "</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/get/ventilator?uid=abcdef</td>\n"
      "        <td>Get status of ventilator output</td>\n"
      "        <td>" + textplain + "</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/get/heater?uid=abcdef</td>\n"
      "        <td>Get status of heater output</td>\n"
      "        <td>" + textplain + "</td>\n"
      "      </tr>\n"
      "      <tr><td colspan=\"3\" align=\"center\"><b>Operation</b></td></tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/operation?uid=abcdef&a=0&h=0&l=0&v=0</td>\n"
      "        <td>Get all data and set status of outputs</td>\n"
      "        <td>" + textplain + "</td>\n"
      "      </tr>\n"
      "      <tr><td colspan=\"3\" align=\"center\"><b>Manual operation</b></td></tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/set/all/off?uid=abcdef</td>\n"
      "        <td>Switch off all outputs</td>\n"
      "        <td>" + textplain + "</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/set/lamp/off?uid=abcdef</td>\n"
      "        <td>Switch off lamp</td>\n"
      "        <td>" + textplain + "</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/set/lamp/on?uid=abcdef</td>\n"
      "        <td>Switch on lamp</td>\n"
      "        <td>" + textplain + "</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/set/ventilator/off?uid=abcdef</td>\n"
      "        <td>Switch off ventilator</td>\n"
      "        <td>" + textplain + "</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/set/ventilator/on?uid=abcdef</td>\n"
      "        <td>Switch on ventilator</td>\n"
      "        <td>" + textplain + "</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/set/heater/off?uid=abcdef</td>\n"
      "        <td>Switch off heater</td>\n"
      "        <td>" + textplain + "</td>\n"
      "      </tr>\n"
      "      <tr>\n"
      "        <td>http://" + deviceipaddress + "/set/heater/on?uid=abcdef</td>\n"
      "        <td>Switch on heater</td>\n"
      "        <td>" + textplain + "</td>\n"
      "      </tr>\n"
      "    </table>\n"
      "    <br>\n"
      "    <hr>\n"
      "    <center>" + msg[2] + " <a href=\"mailto:" + msg[3] + "\">" + msg[28] + "</a> - <a href=\"" + msg[4] + "\">Homepage</a><center>\n"
      "    <br>\n"
      "  <body>\n"
      "</html>\n";
    server.send(200, texthtml, line);
    delay(100);
  });
  // write summary of status
  server.on("/summary", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg[49]);
        writesyslog(49);
        line =
          "<html>\n"
          "  <head>\n"
          "    <title>" + msg[1] + " | Summary of status</title>\n"
          "    <meta http-equiv=\"refresh\" content=\"60\">\n"
          "  </head>\n"
          "  <body bgcolor=\"#e2f4fd\" style=\"font-family:\'sans\'\">\n"
          "    <h2>" + msg[1] + "</h2>\n"
          "    <br>\n"
          "    IP address: " + deviceipaddress + "<br>\n"
          "    MAC address: " + devicemacaddress + "<br>\n"
          "    Hardware serial number: " + serialnumber + "<br>\n"
          "    Software version: v" + swversion + "<br>\n"
          "    <hr>\n"
          "    <h3>Summary of status:</h3>\n"
          "    <table border=\"1\" cellpadding=\"3\" cellspacing=\"0\">\n"
          "      <tr><td colspan=\"3\" align=\"center\"><b>Alarm</b></td></tr>\n"
          "      <tr>\n"
          "        <td>Alarm event:</td>\n"
          "        <td>";
        if (alarm == 1) line = line + "Detected"; else line = line + "Not detected";
        line = line +
               "        </td>\n"
               "      </tr>\n"
               "      <tr><td colspan=\"3\" align=\"center\"><b>Status</b></td></tr>\n"
               "      <tr>\n"
               "        <td>Operation mode:</td>\n"
               "        <td>";
        if (opmode == 1) line = line + "Hyphae"; else line = line + "Mushroom";
        line = line +
               "        </td>\n"
               "      </tr>\n"
               "      <tr>\n"
               "        <td>Manual mode:</td>\n"
               "        <td>";
        if (swmanu == 1) line = line + "ON"; else line = line + "OFF";
        line = line +
               "        </td>\n"
               "      </tr>\n"
               "      <tr>\n"
               "        <td>Overcurrent protection:</td>\n"
               "        <td>";
        if (ocprot == 1) line = line + "Opened"; else line = line + "Closed";
        line = line +
               "        </td>\n"
               "      </tr>\n"
               "      <tr><td colspan=\"3\" align=\"center\"><b>Outputs</b></td></tr>\n"
               "      <tr>\n"
               "        <td>Lamp:</td>\n"
               "        <td>";
        if (lamp == 1) line = line + "ON"; else line = line + "OFF";
        line = line +
               "        </td>\n"
               "      </tr>\n"
               "      <tr>\n"
               "        <td>Ventilator:</td>\n"
               "        <td>";
        if (vent == 1) line = line + "ON"; else line = line + "OFF";
        line = line +
               "        </td>\n"
               "      </tr>\n"
               "      <tr>\n"
               "        <td>Heater:</td>\n"
               "        <td>";
        if (heat == 1) line = line + "ON"; else line = line + "OFF";
        line = line +
               "        </td>\n"
               "      </tr>\n"
               "    </table>\n"
               "    <br>\n"
               "    <hr>\n"
               "    <center>" + msg[2] + " <a href=\"mailto:" + msg[3] + "\">" + msg[28] + "</a> - <a href=\"" + msg[4] + "\">Homepage</a><center>\n"
               "    <br>\n"
               "  </body>\n"
               "</html>\n";
        server.send(200, texthtml, line);
      }
  });
  // write log
  server.on("/log", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg[50]);
        writesyslog(50);
        line =
          "<html>\n"
          "  <head>\n"
          "    <title>" + msg[1] + " | System log</title>\n"
          "  </head>\n"
          "  <body bgcolor=\"#e2f4fd\" style=\"font-family:\'sans\'\">\n"
          "    <h2>" + msg[1] + "</h2>\n"
          "    <br>\n"
          "    IP address: " + deviceipaddress + "<br>\n"
          "    MAC address: " + devicemacaddress + "<br>\n"
          "    Hardware serial number: " + serialnumber + "<br>\n"
          "    Software version: v" + swversion + "<br>\n"
          "    <hr>\n"
          "    <h3>Last 64 lines of system log:</h3>\n"
          "    <table border=\"0\" cellpadding=\"3\" cellspacing=\"0\">\n";
        for (int i = 0; i < 64; i++)
          if (syslog[i] > 0)
            line = line + "      <tr><td><pre>" + String(i) + "</pre></td><td><pre>" + msg[syslog[i]] + "</pre></td></tr>\n";
        line = line +
               "    </table>\n"
               "    <br>\n"
               "    <hr>\n"
               "    <center>" + msg[2] + " <a href=\"mailto:" + msg[3] + "\">" + msg[28] + "</a> - <a href=\"" + msg[4] + "\">Homepage</a><center>\n"
               "    <br>\n"
               "  </body>\n"
               "</html>\n";
        server.send(200, texthtml, line);
      }
  });
  // write device information
  server.on("/version", []()
  {
    writeclientipaddress();
    Serial.println(msg[35]);
    writesyslog(35);
    line = msg[16] + "\n" + swversion + "\n" + serialnumber;
    server.send(200, textplain, line);
    delay(100);
  });

  // Group #2: alarm
  // get alarm status
  server.on("/get/alarm", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg[38]);
        Serial.println(msg[43]);
        writesyslog(38);
        Serial.println(msg[44] + String((int)alarm));
        prevtime = millis();
        line = String((int)alarm);
        server.send(200, textplain, line);
      }
  });
  // restore alarm status
  server.on("/set/alarm/off", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg[30]);
        writesyslog(30);
        prevtime = millis();
        alarm = 0;
        server.send(200, textplain, msg[27]);
      }
  });

  // Group #3: get data
  // get all status (except outputs)
  server.on("/get/all", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg[37]);
        writesyslog(37);
        Serial.println(msg[43]);
        Serial.println(msg[44] + String((int)alarm));
        Serial.println(msg[45] + String((int)opmode));
        Serial.println(msg[46] + String((int)swmanu));
        Serial.println(msg[47] + String((int)ocprot));
        prevtime = millis();
        line = String((int)alarm) + "\n" + String((int)opmode) + "\n" +  String((int)swmanu) + "\n" + String((int)ocprot);
        server.send(200, textplain, line);
      }
  });
  // get status of operation mode switch
  server.on("/get/operationmode", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg[39]);
        writesyslog(39);
        Serial.println(msg[43]);
        Serial.println(msg[45] + String((int)opmode));
        prevtime = millis();
        line = String((int)opmode);
        server.send(200, textplain, line);
      }
  });
  // get status of manual mode switches
  server.on("/get/manualswitch", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg[40]);
        writesyslog(40);
        Serial.println(msg[43]);
        Serial.println(msg[46] + String((int)swmanu));
        prevtime = millis();
        line = String((int)swmanu);
        server.send(200, textplain, line);
      }
  });
  // get status of overcurrent protection
  server.on("/get/protection", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg[41]);
        writesyslog(41);
        Serial.println(msg[43]);
        Serial.println(msg[47] + String((int)ocprot));
        prevtime = millis();
        line = String((int)ocprot);
        server.send(200, textplain, line);
      }
  });
  // get status of heater output
  server.on("/get/lamp", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg[51]);
        writesyslog(51);
        Serial.println(msg[43]);
        Serial.println(msg[54] + String((int)lamp));
        prevtime = millis();
        line = String((int)lamp);
        server.send(200, textplain, line);
      }
  });
  // get status of heater output
  server.on("/get/ventilator", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg[52]);
        writesyslog(52);
        Serial.println(msg[43]);
        Serial.println(msg[55] + String((int)vent));
        prevtime = millis();
        line = String((int)vent);
        server.send(200, textplain, line);
      }
  });
  // get status of heater output
  server.on("/get/heater", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg[53]);
        writesyslog(53);
        Serial.println(msg[43]);
        Serial.println(msg[56] + String((int)heat));
        prevtime = millis();
        line = String((int)heat);
        server.send(200, textplain, line);
      }
  });

  // Group #4: operation
  // set status of outputs and get all status (except outputs)
  server.on("/operation", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg[36]);
        writesyslog(36);
        Serial.println(msg[37]);
        Serial.println(msg[43]);
        Serial.println(msg[44] + String((int)alarm));
        Serial.println(msg[45] + String((int)opmode));
        Serial.println(msg[46] + String((int)swmanu));
        Serial.println(msg[47] + String((int)ocprot));
        prevtime = millis();
        line = String((int)alarm) + "\n" + String((int)opmode) + "\n" +  String((int)swmanu) + "\n" + String((int)ocprot);
        server.send(200, textplain, line);
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

  // Group #5: manual operation
  // switch of all outputs
  server.on("/set/all/off", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg[42]);
        writesyslog(42);
        prevtime = millis();
        heat = 0;
        lamp = 0;
        vent = 0;
        server.send(200, textplain, msg[27]);
      }
  });
  // switch off lamp
  server.on("/set/lamp/off", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg[25] + msg[31]);
        writesyslog(57);
        prevtime = millis();
        lamp = 0;
        server.send(200, textplain, msg[27]);
      }
  });
  // switch on lamp
  server.on("/set/lamp/on", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg[25] + msg[32]);
        writesyslog(57);
        prevtime = millis();
        lamp = 1;
        server.send(200, textplain, msg[27]);
      }
  });
  // switch off ventilator
  server.on("/set/ventilator/off", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg[26] + msg[31]);
        writesyslog(58);
        prevtime = millis();
        vent = 0;
        server.send(200, textplain, msg[27]);
      }
  });
  // switch on ventilator
  server.on("/set/ventilator/on", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg[26] + msg[32]);
        writesyslog(58);
        prevtime = millis();
        vent = 1;
        server.send(200, textplain, msg[27]);
      }
  });
  // switch off heater
  server.on("/set/heater/off", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg[24] + msg[31]);
        writesyslog(59);
        prevtime = millis();
        heat = 0;
        server.send(200, textplain, msg[27]);
      }
  });
  // switch on heater
  server.on("/set/heater/on", []()
  {
    if (checkipaddress() == 1)
      if (checkuid() == 1)
      {
        Serial.println(msg[24] + msg[32]);
        writesyslog(59);
        prevtime = millis();
        heat = 1;
        server.send(200, textplain, msg[27]);
      }
  });
  server.begin();
  Serial.println(msg[8]);
  beep(1);
}

// error 404 page
void handleNotFound()
{
  writeclientipaddress();
  Serial.println(msg[22]);
  writesyslog(22);
  server.send(404, textplain, msg[21]);
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
    Serial.println(msg[14]);
    writesyslog(14);
    timeoutsign = 1;
  }
  if (alarm == 0) alarmsign = 0;
  if ((alarm == 1) && (alarmsign == 0))
  {
    Serial.println(msg[15]);
    writesyslog(15);
    alarmsign = 1;
  }
  if (ocprot == 0) ocprotsign = 0;
  if ((ocprot == 1) && (ocprotsign == 0))
  {
    Serial.println(msg[23]);
    writesyslog(23);
    ocprotsign = 1;
  }
  // error sound
  if ((ocprot == 1) || (alarm == 1)) beep(1);
  portwrite();
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

// blink blue LED and write client IP address to serial console
void writeclientipaddress()
{
  digitalWrite(prt_led_blue, HIGH);
  delay(100);
  digitalWrite(prt_led_blue, LOW);
  clientaddress = server.client().remoteIP().toString();
  Serial.println(msg[13] + clientaddress + ".");
  writesyslog(48);
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
    server.send(401, textplain, msg[19]);
    Serial.println(msg[20]);
    writesyslog(20);
    beep(3);
    return 0;
  }
}

// check UID
int checkuid()
{
  if (server.arg("uid") == uid) return 1; else
  {
    server.send(401, textplain, msg[17]);
    Serial.println(msg[18]);
    writesyslog(18);
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

// write a line to system log
void writesyslog(int msgnum)
{
  if (syslog[63] == 0)
  {
    for (int i = 0; i < 64; i++)
    {
      if (syslog[i] == 0)
      {
        syslog[i] = msgnum;
        break;
      }
    }
  } else
  {
    for (int i = 1; i < 64; i++)
      syslog[i - 1] = syslog[i];
    syslog[63] = msgnum;
  }
}
