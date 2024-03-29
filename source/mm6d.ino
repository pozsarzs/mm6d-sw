// +---------------------------------------------------------------------------+
// | MM6D v0.4 * Grow house control device                                     |
// | Copyright (C) 2023 Pozsár Zsolt <pozsarzs@gmail.com>                      |
// | mm6d.ino                                                                  |
// | Program for Adafruit Huzzah Feather                                       |
// +---------------------------------------------------------------------------+

//   This program is free software: you can redistribute it and/or modify it
// under the terms of the European Union Public License 1.2 version.
//
//   This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.

#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ModbusIP_ESP8266.h>
#include <ModbusRTU.h>
#include <StringSplitter.h> // Note: Change MAX = 5 to MAX = 6 in StringSplitter.h.

// settings
const bool    SERIAL_CONSOLE    = true;           // enable/disable boot-time serial console
const bool    HTTP              = true;           // enable/disable HTTP access
const bool    MODBUS_TCP        = true;           // enable/disable Modbus/TCP access
const int     COM_SPEED         = 9600;           // baudrate of the serial port
const int     MB_UID            = 1;              // Modbus UID
  const char   *WIFI_SSID         = "";           // Wifi SSID
  const char   *WIFI_PASSWORD     = "";           // Wifi password
// ports
const int     PRT_AI_OPMODE     = 0;
const int     PRT_DI_ALARM      = 5;
const int     PRT_DI_OCPROT     = 13;
const int     PRT_DI_SWMANU     = 12;
const int     PRT_DO_BUZZER     = 4;
const int     PRT_DO_LEDBLUE    = 2;
const int     PRT_DO_STATUS     = 0;
const int     PRT_DO_HEAT       = 16;
const int     PRT_DO_LAMP       = 14;
const int     PRT_DO_VENT       = 15;

// name of the Modbus registers
const String  COIL_NAME[3]      =
{
  /* 00001 */       "lamp",
  /* 00002 */       "vent",
  /* 00003 */       "heat",
};
const String  DI_NAME[8]        =
{
  /* 10001 */       "gen_error",
  /* 10002 */       "alarm",
  /* 10003 */       "breaker",
  /* 10004 */       "timeout",
  /* 10005 */       "standby",
  /* 10006 */       "hyphae",
  /* 10007 */       "mushroom",
  /* 10008 */       "manual"
};
const String  HR_NAME[6]        =
{
  /* 40001-40008 */ "name",
  /* 40009-40011 */ "version",
  /* 40012-40017 */ "mac_address",
  /* 40018-40021 */ "ip_address",
  /* 40022       */ "modbus_uid",
  /* 40023-40028 */ "com_speed"
};

// other constants
const long    TIME_LOOP         = 100; // in ms
const long    TIME_TIMEOUT      = 60000; // in ms
const String  SWNAME            = "MM6D";
const String  SWVERSION         = "0.4.0";
const String  TEXTHTML          = "text/html";
const String  TEXTPLAIN         = "text/plain";
const String  DOCTYPEHTML       = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n";

// other variables
bool          logmbquery;
int           syslog[64]        = {};
String        htmlheader;
String        line;
String        myipaddress       = "0.0.0.0";
String        mymacaddress      = "00:00:00:00:00:00";
unsigned long currtime;
unsigned long prevtime_loop     = 0;
unsigned long prevtime_timeout  = 0;

// messages
const String  MSG[61]           =
{
  /*  0 */  "",
  /*  1 */  "MM6D * Grow house control device",
  /*  2 */  "Copyright (C) 2023 ",
  /*  3 */  "  software version:       ",
  /*  4 */  "Starting device...",
  /*  5 */  "* Initializing GPIO ports...",
  /*  6 */  "* Connecting to wireless network",
  /*  7 */  "done",
  /*  8 */  "  my MAC address:         ",
  /*  9 */  "  my IP address:          ",
  /* 10 */  "  subnet mask:            ",
  /* 11 */  "  gateway IP address:     ",
  /* 12 */  "* Starting Modbus/TCP server",
  /* 13 */  "* Starting Modbus/RTU slave",
  /* 14 */  "  slave Modbus UID:       ",
  /* 15 */  "  serial port speed:      ",
  /* 16 */  " baud (8N1)",
  /* 17 */  "* Starting webserver",
  /* 18 */  "* Ready, the serial console is off.",
  /* 19 */  "* HTTP query received ",
  /* 20 */  "  get help page",
  /* 21 */  "  get summary page",
  /* 22 */  "  get log page",
  /* 23 */  "  get all data",
  /* 24 */  "* E01: No such page!",
  /* 25 */  "Pozsar Zsolt",
  /* 26 */  "http://www.pozsarzs.hu",
  /* 27 */  "Error 404",
  /* 28 */  "No such page!",
  /* 29 */  "Help",
  /* 30 */  "Information and data access",
  /* 31 */  "Information pages",
  /* 32 */  "Data access with HTTP",
  /* 33 */  "all status in CSV format",
  /* 34 */  "all status in JSON format",
  /* 35 */  "all status in TXT format",
  /* 36 */  "all status in XML format",
  /* 37 */  "Data access with Modbus",
  /* 38 */  "Status: (read-only)",
  /* 39 */  "Outputs: (read/write)",
  /* 40 */  "Software version: (read-only)",
  /* 41 */  "device name",
  /* 42 */  "software version",
  /* 43 */  "Network settings: (read-only)",
  /* 44 */  "MAC address",
  /* 45 */  "IP address",
  /* 46 */  "Modbus UID",
  /* 47 */  "serial port speed",
  /* 48 */  "Summary",
  /* 49 */  "All status",
  /* 50 */  "Log",
  /* 51 */  "Last 64 lines of the system log:",
  /* 52 */  "back",
  /* 53 */  "Remote access:",
  /* 54 */  "  HTTP                    ",
  /* 55 */  "  Modbus/RTU              ",
  /* 56 */  "  Modbus/TCP              ",
  /* 57 */  "enable",
  /* 58 */  "disable",
  /* 59 */  "set outputs (coils)",
  /* 60 */  "* Modbus query received"
};
const String  COIL_DESC[3]      =
{
  /*  0 */  "status of the lamp output",
  /*  1 */  "status of the ventilator output",
  /*  2 */  "status of the heater output"
};
const String  DI_DESC[8]        =
{
  /*  0 */  "general error",
  /*  1 */  "alarm",
  /*  2 */  "overcurrent breaker (1: error)",
  /*  3 */  "connection timeout error",
  /*  4 */  "stand-by operation mode",
  /*  5 */  "growing hyphae operation mode",
  /*  6 */  "growing mushroom operation mode",
  /*  7 */  "manual switch (0/1: auto/manual)",
};

ESP8266WebServer httpserver(80);
ModbusIP mbtcp;
ModbusRTU mbrtu;

// --- SYSTEM LOG ---
// write a line to system log
void writetosyslog(int msgnum)
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

// --- STATIC MODBUS REGISTERS ---
// convert hex string to byte
byte hs2b(String recv) {
  return strtol(recv.c_str(), NULL, 16);
}

// fill holding registers with configuration data
void fillholdingregisters()
{
  int    itemCount;
  String s;
  // name
  s = SWNAME;
  while (s.length() < 8)
    s = char(0x00) + s;
  for (int i = 0; i < 8; i++)
    mbrtu.Hreg(i, char(s[i]));
  // version
  StringSplitter *splitter1 = new StringSplitter(SWVERSION, '.', 3);
  itemCount = splitter1->getItemCount();
  for (int i = 0; i < itemCount; i++)
  {
    String item = splitter1->getItemAtIndex(i);
    mbrtu.Hreg(8 + i, item.toInt());
  }
  delete splitter1;
  // MAC-address
  StringSplitter *splitter2 = new StringSplitter(mymacaddress, ':', 6);
  itemCount = splitter2->getItemCount();
  for (int i = 0; i < itemCount; i++)
  {
    String item = splitter2->getItemAtIndex(i);
    mbrtu.Hreg(11 + i, hs2b(item));
  }
  delete splitter2;
  // IP-address
  StringSplitter *splitter3 = new StringSplitter(myipaddress, '.', 4);
  itemCount = splitter3->getItemCount();
  for (int i = 0; i < itemCount; i++)
  {
    String item = splitter3->getItemAtIndex(i);
    mbrtu.Hreg(17 + i, item.toInt());
  }
  delete splitter3;
  // MB UID
  mbrtu.Hreg(21, MB_UID);
  // serial speed
  s = String(COM_SPEED);
  while (s.length() < 6)
    s = char(0x00) + s;
  for (int i = 0; i < 7; i++)
    mbrtu.Hreg(22 + i, char(s[i]));
}

// --- LEDS AND BUZZER ---
// switch on/off blue LED
void blueled(boolean b)
{
  digitalWrite(PRT_DO_LEDBLUE, b);
}

// blinking blue LED
void blinkblueled()
{
  blueled(true);
  delay(25);
  blueled(false);
}

// beep sign
void beep(int num)
{
  for (int i = 0; i < num; i++)
  {
    tone(PRT_DO_BUZZER, 880);
    delay (100);
    noTone(PRT_DO_BUZZER);
    delay (100);
  }
}

// ---  CONTROL ---
// read GPIO ports
void getinputs()
{
  int ai_opmode;
  bool gen_error;
  logmbquery = false;
  // alarm
  mbrtu.Ists(1, digitalRead(PRT_DI_ALARM)); // -> 10002
  // overcurrent protection (breakers)
  mbrtu.Ists(2, digitalRead(PRT_DI_OCPROT)); // -> 10003
  // operation mode
  ai_opmode = int(analogRead(PRT_AI_OPMODE));
  for (int i = 4; i < 7; i++) mbrtu.Ists(i, false);
  if (ai_opmode > 850) mbrtu.Ists(4, true); // -> 10005
  if (ai_opmode < 150) mbrtu.Ists(5, true); // -> 10006
  if ((ai_opmode > 150) && (ai_opmode < 850)) mbrtu.Ists(6, true); // -> 10007
  // manual mode switch
  mbrtu.Ists(7, digitalRead(PRT_DI_SWMANU)); // -> 10008
  // general error
  gen_error = gen_error || mbrtu.Ists(1); // alarm
  gen_error = gen_error || mbrtu.Ists(2); // breaker
  gen_error = gen_error || mbrtu.Ists(3); // timeout
  gen_error = gen_error || mbrtu.Ists(7); // manual
  mbrtu.Ists(0, gen_error); // -> 10001
  logmbquery = true;
}

// write GPIO ports
void setoutputs()
{
  logmbquery = false;
  digitalWrite(PRT_DO_STATUS, mbrtu.Ists(0));
  if (mbrtu.Ists(4) || mbrtu.Ists(3)) // standby or timeout
  {
    digitalWrite(PRT_DO_LAMP, false);
    digitalWrite(PRT_DO_VENT, false);
    digitalWrite(PRT_DO_HEAT, false);
  } else
  {
    digitalWrite(PRT_DO_LAMP, mbrtu.Coil(0));
    digitalWrite(PRT_DO_VENT, mbrtu.Coil(1));
    digitalWrite(PRT_DO_HEAT, mbrtu.Coil(2));
  }
  logmbquery = true;
}

// --- DATA RETRIEVING ---
// blink blue LED and write to log
void httpquery()
{
  blinkblueled();
  writetosyslog(19);
  prevtime_timeout = currtime;
}

uint16_t modbusquery(TRegister* reg, uint16_t val)
{
  if (logmbquery)
  {
    blinkblueled();
    writetosyslog(60);
    prevtime_timeout = currtime;
  }
  return val;
}

// --- WEBPAGES ---
// header for pages
void header_html()
{
  htmlheader = 
    "    <table border=\"0\">\n"
    "      <tbody>\n"
    "      <tr><td><i>" + MSG[8] + "</i></td><td>" + mymacaddress + "</td></tr>\n"
    "      <tr><td><i>" + MSG[9] + "</i></td><td>" + myipaddress + "</td></tr>\n"
    "      <tr><td><i>" + MSG[3] + "</i></td><td>v" + SWVERSION + "</td></tr>\n"
    "      <tr><td><i>" + MSG[14] + "</i></td><td>" + String(MB_UID) + "</td></tr>\n"
    "      <tr><td><i>" + MSG[15] + "</i></td><td>" + String(COM_SPEED) + MSG[16] + "</td></tr>\n"
    "      </tbody>\n"
    "    </table>\n";
}

// error 404 page
void handleNotFound()
{
  httpquery;
  writetosyslog(27);
  line = DOCTYPEHTML +
         "<html>\n"
         "  <head>\n"
         "    <title>" + MSG[1] + " | " + MSG[27] + "</title>\n"
         "  </head>\n"
         "  <body bgcolor=\"#e2f4fd\" style=\"font-family:\'sans\'\">\n"
         "    <h2>" + MSG[1] + "</h2>\n"
         "    <br>\n" + htmlheader + "    <hr>\n"
         "    <h3>" + MSG[27] + "</h3>\n" + MSG[28] + "\n"
         "    <br>\n"
         "    <hr>\n"
         "    <div align=\"right\"><a href=\"/\">" + MSG[51] + "</a></div>\n"
         "    <br>\n"
         "    <center>" + MSG[2] + " <a href=\"" + MSG[26] + "\">" + MSG[25] + "</a></center>\n"
         "    <br>\n"
         "  </body>\n"
         "</html>\n";
  httpserver.send(404, TEXTHTML, line);
  delay(100);
}

// help page
void handleHelp()
{
  String s;
  httpquery;
  writetosyslog(20);
  line = DOCTYPEHTML +
         "<html>\n"
         "  <head>\n"
         "    <title>" + MSG[1] + " | " + MSG[29] + "</title>\n"
         "  </head>\n"
         "  <body bgcolor=\"#e2f4fd\" style=\"font-family:\'sans\'\">\n"
         "    <h2>" + MSG[1] + "</h2>\n"
         "    <br>\n" + htmlheader + "    <hr>\n"
         "    <h3>" + MSG[30] + "</h3>\n"
         "    <table border=\"1\" cellpadding=\"3\" cellspacing=\"0\">\n"
         "      <tr><td colspan=\"3\" align=\"center\"><b>" + MSG[31] + "</b></td></tr>\n"
         "      <tr>\n"
         "        <td><a href=\"http://" + myipaddress + "/\">http://" + myipaddress + "/</a></td>\n"
         "        <td>" + MSG[29] + "</td>\n"
         "        <td>" + TEXTHTML + "</td>\n"
         "      </tr>\n"
         "      <tr>\n"
         "        <td><a href=\"http://" + myipaddress + "/summary\">http://" + myipaddress + "/summary</a></td>\n"
         "        <td>" + MSG[48] + "</td>\n"
         "        <td>" + TEXTHTML + "</td>\n"
         "      </tr>\n"
         "      <tr>\n"
         "        <td><a href=\"http://" + myipaddress + "/log\">http://" + myipaddress + "/log</a></td>\n"
         "        <td>" + MSG[50] + "</td>\n"
         "        <td>" + TEXTHTML + "</td>\n"
         "      </tr>\n"
         "      <tr><td colspan=\"3\" align=\"center\"><b>" + MSG[32] + "</b></td>\n"
         "      <tr>\n"
         "        <td>\n"
         "          <a href=\"http://" + myipaddress + "/get/csv\">http://" + myipaddress + "/get/csv</a>"
         "        </td>\n"
         "        <td>" + MSG[33] + "</td>\n"
         "        <td>" + TEXTPLAIN + "</td>\n"
         "      </tr>\n"
         "      <tr>\n"
         "        <td><a href=\"http://" + myipaddress + "/get/json\">http://" + myipaddress + "/get/json</a></td>\n"
         "        <td>" + MSG[34] + "</td>\n"
         "        <td>" + TEXTPLAIN + "</td>\n"
         "      </tr>\n"
         "      <tr>\n"
         "        <td><a href=\"http://" + myipaddress + "/get/txt\">http://" + myipaddress + "/get/txt</a></td>\n"
         "        <td>" + MSG[35] + "</td>\n"
         "        <td>" + TEXTPLAIN + "</td>\n"
         "      </tr>\n"
         "      <tr>\n"
         "        <td><a href=\"http://" + myipaddress + "/get/xml\">http://" + myipaddress + "/get/xml</a></td>\n"
         "        <td>" + MSG[36] + "</td>\n"
         "        <td>" + TEXTPLAIN + "</td>\n"
         "      </tr>\n"
         "      <tr>\n"
         "        <td><a href=\"http://" + myipaddress + "/set?lamp=0&vent=0&heat=0\">http://" + myipaddress + "/set?lamp=0&vent=0&heat=0</a></td>\n"
         "        <td>" + MSG[59] + "</td>\n"
         "        <td>" + TEXTPLAIN + "</td>\n"
         "      </tr>\n"
         "      <tr><td colspan=\"3\" align=\"center\"><b>" + MSG[37] + "</b></td>\n"
         "      <tr><td colspan=\"3\"><i>" + MSG[38] + "</i></td>\n";
  for (int i = 0; i < 8; i++)
  {
    line +=
      "      <tr>\n"
      "        <td>" + String(i + 10001) + "</td>\n"
      "        <td>" + DI_DESC[i] + "</td>\n"
      "        <td>bit</td>\n"
      "      </tr>\n";
  }
  line += "      <tr><td colspan=\"3\"><i>" + MSG[39] + "</i></td>\n";
  for (int i = 0; i < 3; i++)
  {
    s = COIL_DESC[i];
    line +=
      "      <tr>\n"
      "        <td>0000" + String(i + 1) + "</td>\n"
      "        <td>" + s + "</td>\n"
      "        <td>bit</td>\n"
      "      </tr>\n";
  }
  line += "      <tr><td colspan=\"3\"><i>" + MSG[40] + "</i></td>\n"
          "      <tr>\n"
          "        <td>40001-40008</td>\n"
          "        <td>" + MSG[41] + "</td>\n"
          "        <td>8 char</td>\n"
          "      </tr>\n"
          "      <tr>\n"
          "        <td>40009-40011</td>\n"
          "        <td>" + MSG[42] + "</td>\n"
          "        <td>3 byte</td>\n"
          "      </tr>\n"
          "      <tr><td colspan=\"3\"><i>" + MSG[43] + "</i></td>\n"
          "      <tr>\n"
          "        <td>40012-40017</td>\n"
          "        <td>" + MSG[44] + "</td>\n"
          "        <td>6 byte</td>\n"
          "      </tr>\n"
          "      <tr>\n"
          "        <td>40018-40021</td>\n"
          "        <td>" + MSG[45] + "</td>\n"
          "        <td>4 byte</td>\n"
          "      </tr>\n"
          "      <tr>\n"
          "        <td>40022</td>\n"
          "        <td>" + MSG[46] + "</td>\n"
          "        <td>1 byte</td>\n"
          "      </tr>\n"
          "      <tr>\n"
          "        <td>40023-40028</td>\n"
          "        <td>" + MSG[47] + "</td>\n"
          "        <td>6 char</td>\n"
          "      </tr>\n"
          "    </table>\n"
          "    <br>\n"
          "    <hr>\n"
          "    <br>\n"
          "    <center>" + MSG[2] + " <a href=\"" + MSG[26] + "\">" + MSG[25] + "</a></center>\n"
          "    <br>\n"
          "  </body>\n"
          "</html>\n";
  httpserver.send(200, TEXTHTML, line);
  delay(100);
}

// summary page
void handleSummary()
{
  String s;
  int ii;
  httpquery;
  writetosyslog(21);
  line = DOCTYPEHTML +
         "<html>\n"
         "  <head>\n"
         "    <title>" + MSG[1] + " | " + MSG[48] + "</title>\n"
         "  </head>\n"
         "  <body bgcolor=\"#e2f4fd\" style=\"font-family:\'sans\'\">\n"
         "    <h2>" + MSG[1] + "</h2>\n"
         "    <br>\n" + htmlheader + "    <hr>\n"
         "    <h3>" + MSG[49] + "</h3>\n"
         "    <table border=\"1\" cellpadding=\"3\" cellspacing=\"0\">\n";
  for (int i = 0; i < 8; i++)
  {
    line +=
      "      <tr>\n"
      "        <td>" + DI_DESC[i] + "</td>\n"
      "        <td align=\"right\">" + String(mbrtu.Ists(i)) + "</td>\n"
      "      </tr>\n";
  }
  for (int i = 0; i < 3; i++)
  {
    line +=
      "      <tr>\n"
      "        <td>" + COIL_DESC[i] + "</td>\n"
      "        <td align=\"right\">" + String(mbrtu.Coil(i)) + "</td>\n"
      "      </tr>\n";
  }
  line +=
    "    </table>\n"
    "    <br>\n"
    "    <hr>\n"
    "    <div align=\"right\"><a href=\"/\">" + MSG[52] + "</a></div>\n"
    "    <br>\n"
    "    <center>" + MSG[2] + " <a href=\"" + MSG[26] + "\">" + MSG[25] + "</a></center>\n"
    "    <br>\n"
    "  </body>\n"
    "</html>\n";
  httpserver.send(200, TEXTHTML, line);
  delay(100);
}

// log page
void handleLog()
{
  httpquery;
  writetosyslog(22);
  line = DOCTYPEHTML +
         "<html>\n"
         "  <head>\n"
         "    <title>" + MSG[1] + " | " + MSG[50] + "</title>\n"
         "  </head>\n"
         "  <body bgcolor=\"#e2f4fd\" style=\"font-family:\'sans\'\">\n"
         "    <h2>" + MSG[1] + "</h2>\n"
         "    <br>\n" + htmlheader + "    <hr>\n"
         "    <h3>" + MSG[51] + "</h3>\n"
         "    <table border=\"0\" cellpadding=\"3\" cellspacing=\"0\">\n";
  for (int i = 0; i < 64; i++)
    if (syslog[i] > 0)
      line += "      <tr><td align=right><b>" + String(i) + "</b></td><td>" + MSG[syslog[i]] + "</td></tr>\n";
  line +=
    "    </table>\n"
    "    <br>\n"
    "    <hr>\n"
    "    <div align=\"right\"><a href=\"/\">" + MSG[52] + "</a></div>\n"
    "    <br>\n"
    "    <center>" + MSG[2] + " <a href=\"" + MSG[26] + "\">" + MSG[25] + "</a></center>\n"
    "    <br>\n"
    "  </body>\n"
    "</html>\n";
  httpserver.send(200, TEXTHTML, line);
  delay(100);
}

// get all measured data in CSV format
void handleGetCSV()
{
  httpquery;
  writetosyslog(23);
  line = "\"" + HR_NAME[0] + "\",\"" + SWNAME + "\"\n"
         "\"" + HR_NAME[1] + "\",\"" + SWVERSION + "\"\n"
         "\"" + HR_NAME[2] + "\",\"" + mymacaddress + "\"\n"
         "\"" + HR_NAME[3] + "\",\"" + myipaddress + "\"\n"
         "\"" + HR_NAME[4] + "\",\"" + String(MB_UID) + "\"\n"
         "\"" + HR_NAME[5] + "\",\"" + String(COM_SPEED) + "\"\n";
  for (int i = 0; i < 8; i++)
    line += "\"" + DI_NAME[i] + "\",\"" + String(mbrtu.Ists(i)) + "\"\n";
  for (int i = 0; i < 3; i++)
    line += "\"" + COIL_NAME[i] + "\",\"" + String(mbrtu.Coil(i)) + "\"\n";
  httpserver.send(200, TEXTPLAIN, line);
  delay(100);
}

// get all measured values in JSON format
void handleGetJSON()
{
  httpquery;
  writetosyslog(23);
  line = "{\n"
         "  \"software\": {\n"
         "    \"" + HR_NAME[0] + "\": \"" + SWNAME + "\",\n"
         "    \"" + HR_NAME[1] + "\": \"" + SWVERSION + "\"\n"
         "  },\n"
         "  \"hardware\": {\n"
         "    \"" + HR_NAME[2] + "\": \"" + mymacaddress + "\",\n"
         "    \"" + HR_NAME[3] + "\": \"" + myipaddress + "\",\n"
         "    \"" + HR_NAME[4] + "\": \"" + String(MB_UID) + "\",\n"
         "    \"" + HR_NAME[5] + "\": \"" + String(COM_SPEED) + "\"\n"
         "  },\n"
    "  \"data\": {\n"
    "    \"bit\": {\n";
  for (int i = 0; i < 8; i++)
  {
    line += "      \"" + DI_NAME[i] + "\": \"" + String(mbrtu.Ists(i));
    if (i < 21 ) line += "\",\n"; else  line += "\"\n";
  }
  for (int i = 0; i < 3; i++)
  {
    line += "      \"" + COIL_NAME[i] + "\": \"" + String(mbrtu.Coil(i));
    if (i < 21 ) line += "\",\n"; else  line += "\"\n";
  }
  line +=
    "    }\n"
    "  }\n"
    "}\n";
  httpserver.send(200, TEXTPLAIN, line);
  delay(100);
}

// get all measured data in TXT format
void handleGetTXT()
{
  httpquery;
  writetosyslog(23);
  line = SWNAME + "\n" +
         SWVERSION + "\n" +
         mymacaddress + "\n" +
         myipaddress + "\n" + \
         String(MB_UID) + "\n" + \
         String(COM_SPEED) + "\n";
  for (int i = 0; i < 8; i++)
    line += String(mbrtu.Ists(i)) + "\n";
  for (int i = 0; i < 3; i++)
    line += String(mbrtu.Coil(i)) + "\n";
  httpserver.send(200, TEXTPLAIN, line);
  delay(100);
}

// get all measured values in XML format
void handleGetXML()
{
  httpquery;
  writetosyslog(23);
  line = "<xml>\n"
         "  <software>\n"
         "    <" + HR_NAME[0] + ">" + SWNAME + "</" + HR_NAME[0] + ">\n"
         "    <" + HR_NAME[1] + ">" + SWVERSION + "</" + HR_NAME[1] + ">\n"
         "  </software>\n"
         "  <hardware>\n"
         "    <" + HR_NAME[2] + ">" + mymacaddress + "</" + HR_NAME[2] + ">\n"
         "    <" + HR_NAME[3] + ">" + myipaddress + "</" + HR_NAME[3] + ">\n"
         "    <" + HR_NAME[4] + ">" + String(MB_UID) + "</" + HR_NAME[4] + ">\n"
         "    <" + HR_NAME[5] + ">" + String(COM_SPEED) + "</" + HR_NAME[5] + ">\n"
         "  </hardware>\n"
         "  <data>\n"
         "    <bit>\n";
  for (int i = 0; i < 8; i++)
    line += "      <" + DI_NAME[i] + ">" + String(mbrtu.Ists(i)) + "</" + DI_NAME[i] + ">\n";
  for (int i = 0; i < 3; i++)
    line += "      <" + COIL_NAME[i] + ">" + String(mbrtu.Coil(i)) + "</" + COIL_NAME[i] + ">\n";
  line +=
         "    </bit>\n"
         "  </data>\n"
         " </xml> ";
  httpserver.send(200, TEXTPLAIN, line);
  delay(100);
}

// get all measured values in XML format
void handleSetCoils()
{
  const String  ARGS[3] = {"lamp", "vent","heat"};
  String arg;
  httpquery;
  writetosyslog(59);
  line = "";
  for (int i = 0; i < 3; i++)
  {
    arg = httpserver.arg(ARGS[i]);
    if (arg.length() != 0)
    {
      if (arg == "0") mbrtu.Coil(i, false);
      if (arg == "1") mbrtu.Coil(i, true);
      line += " " + ARGS[i] + ": " + arg + "\n";
    }
  }
  httpserver.send(200, TEXTPLAIN, line);
  delay(100);
}

// --- MAIN ---
// initializing function
void setup(void)
{
  // set serial port
  Serial.begin(COM_SPEED, SERIAL_8N1);
  // write program information
  if (SERIAL_CONSOLE)
  {
    Serial.println("");
    Serial.println("");
    Serial.println(MSG[1]);
    Serial.println(MSG[2] + MSG[25]);
    Serial.println(MSG[3] + "v" + SWVERSION );
    Serial.println(MSG[53]);
    Serial.print(MSG[54]);
    if (HTTP) Serial.println(MSG[57]); else Serial.println(MSG[58]);
    Serial.println(MSG[55] + MSG[57]);
    Serial.print(MSG[56]);
    if (MODBUS_TCP) Serial.println(MSG[57]); else Serial.println(MSG[58]);
  }
  writetosyslog(4);
  if (SERIAL_CONSOLE) Serial.println(MSG[4]);
  // initialize GPIO ports
  writetosyslog(5);
  if (SERIAL_CONSOLE) Serial.println(MSG[5]);
  pinMode(PRT_DI_ALARM, INPUT);
  pinMode(PRT_DI_OCPROT, INPUT);
  pinMode(PRT_DI_SWMANU, INPUT);
  pinMode(PRT_DO_BUZZER, OUTPUT);
  pinMode(PRT_DO_LEDBLUE, OUTPUT);
  pinMode(PRT_DO_STATUS, OUTPUT);
  pinMode(PRT_DO_LAMP, OUTPUT);
  pinMode(PRT_DO_VENT, OUTPUT);
  pinMode(PRT_DO_HEAT, OUTPUT);
  digitalWrite(PRT_DO_LEDBLUE, LOW);
  digitalWrite(PRT_DO_STATUS, LOW);
  digitalWrite(PRT_DO_LAMP, LOW);
  digitalWrite(PRT_DO_VENT, LOW);
  digitalWrite(PRT_DO_HEAT, LOW);
  // connect to wireless network
  if (HTTP || MODBUS_TCP)
  {
    writetosyslog(6);
    if (SERIAL_CONSOLE) Serial.print(MSG[6]);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
      blinkblueled();
      if (SERIAL_CONSOLE) Serial.print(".");
    }
    if (SERIAL_CONSOLE) Serial.println(MSG[7]);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    myipaddress = WiFi.localIP().toString();
    mymacaddress = WiFi.macAddress();
    if (SERIAL_CONSOLE)
    {
      Serial.println(MSG[8] + mymacaddress);
      Serial.println(MSG[9] + myipaddress);
      Serial.println(MSG[10] + WiFi.subnetMask().toString());
      Serial.println(MSG[11] + WiFi.gatewayIP().toString());
    }
  }
  if (MODBUS_TCP)
  { 
    writetosyslog(12);
    if (SERIAL_CONSOLE) Serial.println(MSG[12]);
    mbtcp.server();
  }
  // start Modbus/RTU slave
  writetosyslog(13);
  if (SERIAL_CONSOLE) Serial.println(MSG[13]);
  mbrtu.begin(&Serial);
  mbrtu.setBaudrate(COM_SPEED);
  mbrtu.slave(MB_UID);
  if (SERIAL_CONSOLE)
  {
    Serial.println(MSG[14] + String(MB_UID));
    Serial.println(MSG[15] + String(COM_SPEED) + MSG[16]);
  }
  // set Modbus registers
  mbrtu.addCoil(0, false, 4);
  mbrtu.addIsts(0, false, 8);
  mbrtu.addHreg(0, 0, 28);
  // set Modbus callback
  mbrtu.onGetCoil(0, modbusquery, 4);
  mbrtu.onSetCoil(0, modbusquery, 4);
  mbrtu.onGetIsts(0, modbusquery, 8);
  mbrtu.onGetHreg(0, modbusquery, 28);
  // fill Modbus holding registers
  fillholdingregisters();
  // start webserver
  if (HTTP)
  {
    writetosyslog(17);
    if (SERIAL_CONSOLE) Serial.println(MSG[17]);
    header_html();
    httpserver.onNotFound(handleNotFound);
    httpserver.on("/", handleHelp);
    httpserver.on("/summary", handleSummary);
    httpserver.on("/log", handleLog);
    httpserver.on("/get/csv", handleGetCSV);
    httpserver.on("/get/json", handleGetJSON);
    httpserver.on("/get/txt", handleGetTXT);
    httpserver.on("/get/xml", handleGetXML);
    httpserver.on("/set", handleSetCoils);
    httpserver.begin();
  }
  if (SERIAL_CONSOLE) Serial.println(MSG[18]);
  beep(1);
}

// loop function
void loop(void)
{
  if (HTTP) httpserver.handleClient();
  currtime = millis();
  if (currtime - prevtime_timeout >= TIME_TIMEOUT) mbrtu.Ists(3, true); else mbrtu.Ists(3, false);
  if (currtime - prevtime_loop >= TIME_LOOP)
  {
    prevtime_loop = currtime;
    getinputs();
    setoutputs();
  }
  if (MODBUS_TCP) mbtcp.task();
  delay(10);
  mbrtu.task();
  yield();
}
