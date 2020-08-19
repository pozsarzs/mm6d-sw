// +---------------------------------------------------------------------------+
// | MM6D v0.1 * Remote controlled switching device                            |
// | Copyright (C) 2020 Pozsár Zsolt <pozsar.zsolt@szerafingomba.hu>           |
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
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

// settings
const char* wifi_ssid       = "";
const char* wifi_password   = "";
const String www_username   = "";
const String allowedaddress = "";
const int alarmminlevel     = 0;
const int alarmmaxlevel     = 512;

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
const String msg02          = "Copyright (C) 2020";
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
const String msg15          = "";
const String msg16          = "MM6D";
const String msg17          = "Authentication error!";
const String msg18          = "* E03: Authentication error!";
const String msg19          = "Not allowed client IP address!";
const String msg20          = "* E04: Not allowed client IP address!";
const String msg21          = "Page not found!";
const String msg22          = "* E05: Page not found!";
const String msg23          = "* E02: Overcurrent protection error!";
const String msg24          = "";
const String msg25          = "";
const String msg26          = "";
const String msg27          = "Done.";
const String msg28          = "Pozsar Zsolt";
const String msg29          = "  device MAC address: ";

// general constants
const int interval          = 0;
const String swversion      = "0.1";

// variables
int alarm                   = 0;
int error                   = 0;
int heat                    = 0;
int lamp                    = 0;
int ocprot;
int opmode;
int swmanu;
int timeout;
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
    Serial.println(msg13 + server.client().remoteIP().toString() + ".");
    blinkblueled();
    line = "<html><head><title>" + msg01 + "</title></head>"
           "<body bgcolor=\"#e2f4fd\"><h2>" + msg01 + "</h2>""<br>"
           "Software version: v" + swversion + "<br>"
           "<hr><h3>Plain text data and control pages:</h3><br>"
           "<table border=\"0\" cellpadding=\"5\">"
           "<tr><td><a href=\"http://" + localipaddress + "/version\">http://" + localipaddress + "/version</a></td><td>Get software name and version</td></tr>"
           "<tr><td colspan=\"2\">&nbsp;</td></tr>"
           "<tr><td><a href=\"http://" + localipaddress + "/get/all\">http://" + localipaddress + "/get/all</a></td><td>Get all status<sup>*</sup></td></tr>"
           "<tr><td><a href=\"http://" + localipaddress + "/get/alarm\">http://" + localipaddress + "/get/alarm</a></td><td>Get status of alarm sensors<sup>*</sup></td></tr>"
           "<tr><td><a href=\"http://" + localipaddress + "/get/manualswitch\">http://" + localipaddress + "/get/manualswitch</a></td><td>Get status of manual switch<sup>*</sup></td></tr>"
           "<tr><td><a href=\"http://" + localipaddress + "/get/operationmode\">http://" + localipaddress + "/get/operationmode</a></td><td>Get operation mode<sup>*</sup></td></tr>"
           "<tr><td><a href=\"http://" + localipaddress + "/get/protection\">http://" + localipaddress + "/get/protection</a></td><td>Get status of overcurrent protection<sup>*</sup></td></tr>"
           "<tr><td colspan=\"2\">&nbsp;</td></tr>"
           "<tr><td><a href=\"http://" + localipaddress + "/set/all/off\">http://" + localipaddress + "/set/all/off</a></td><td>Switch off all outputs<sup>*</sup></td></tr>"
           "<tr><td><a href=\"http://" + localipaddress + "/set/alarm/off\">http://" + localipaddress + "/set/alarm/off</a></td><td>Restore alarm input<sup>*</sup></td></tr>"
           "<tr><td><a href=\"http://" + localipaddress + "/set/heater/off\">http://" + localipaddress + "/set/heater/off</a></td><td>Switch off heater<sup>*</sup></td></tr>"
           "<tr><td><a href=\"http://" + localipaddress + "/set/heater/on\">http://" + localipaddress + "/set/heater/on</a></td><td>Switch on heater<sup>*</sup></td></tr>"
           "<tr><td><a href=\"http://" + localipaddress + "/set/lamp/off\">http://" + localipaddress + "/set/lamp/off</a></td><td>Switch off lamp<sup>*</sup></td></tr>"
           "<tr><td><a href=\"http://" + localipaddress + "/set/lamp/on\">http://" + localipaddress + "/set/lamp/on</a></td><td>Switch on lamp<sup>*</sup></td></tr>"
           "<tr><td><a href=\"http://" + localipaddress + "/set/ventilator/off\">http://" + localipaddress + "/set/ventilator/off</a></td><td>Switch off ventilator<sup>*</sup></td></tr>"
           "<tr><td><a href=\"http://" + localipaddress + "/set/ventilator/on\">http://" + localipaddress + "/set/ventilator/on</a></td><td>Switch on ventilator<sup>*</sup></td></tr>"
           "</table><br><sup>*</sup>Use <i>username</i> argument!<br>"
           "<hr><center>" + msg02 + " <a href=\"mailto:" + msg03 + "\">" + msg28 + "</a> - <a href=\"" + msg04 + "\">Homepage</a><center><br><body></html>";
    server.send(200, "text/html", line);
    delay(100);
  });
  server.on("/version", []()
  {
    Serial.println(msg13 + server.client().remoteIP().toString() + ".");
    blinkblueled();
    line = msg16 + "\n" + swversion;
    server.send(200, "text/plain", line);
    delay(100);
  });
  server.on("/get/all", []()
  {
    blinkblueled();
    clientaddress = server.client().remoteIP().toString();
    Serial.println(msg13 + clientaddress + ".");
    if (clientaddress == allowedaddress)
    {
      if (checkusername() == 1)
      {
        prevtime = millis();
        line = String((int)alarm) + "\n" + String((int)opmode) + "\n" +  String((int)swmanu) + "\n" + String((int)ocprot);
        server.send(200, "text/plain", line);
      } else
      {
        server.send(401, "text/plain", msg17);
        Serial.println(msg18);
      }
    } else
    {
      server.send(401, "text/plain", msg19);
      beep();
      beep();
      beep();
      Serial.println(msg20);
    }
  });
  server.on("/get/alarm", []()
  {
    blinkblueled();
    clientaddress = server.client().remoteIP().toString();
    Serial.println(msg13 + clientaddress + ".");
    if (clientaddress == allowedaddress)
    {
      if (checkusername() == 1)
      {
        prevtime = millis();
        line = String((int)alarm);
        server.send(200, "text/plain", line);
      } else
      {
        server.send(401, "text/plain", msg17);
        Serial.println(msg18);
      }
    } else
    {
      server.send(401, "text/plain", msg19);
      beep();
      beep();
      beep();
      Serial.println(msg20);
    }
  });
  server.on("/get/operationmode", []()
  {
    blinkblueled();
    clientaddress = server.client().remoteIP().toString();
    Serial.println(msg13 + clientaddress + ".");
    if (clientaddress == allowedaddress)
    {
      if (checkusername() == 1)
      {
        prevtime = millis();
        line = String((int)opmode);
        server.send(200, "text/plain", line);
      } else
      {
        server.send(401, "text/plain", msg17);
        Serial.println(msg18);
      }
    } else
    {
      server.send(401, "text/plain", msg19);
      beep();
      beep();
      beep();
      Serial.println(msg20);
    }
  });
  server.on("/get/manualswitch", []()
  {
    blinkblueled();
    clientaddress = server.client().remoteIP().toString();
    Serial.println(msg13 + clientaddress + ".");
    if (clientaddress == allowedaddress)
    {
      if (checkusername() == 1)
      {
        prevtime = millis();
        line = String((int)swmanu);
        server.send(200, "text/plain", line);
      } else
      {
        server.send(401, "text/plain", msg17);
        Serial.println(msg18);
      }
    } else
    {
      server.send(401, "text/plain", msg19);
      beep();
      beep();
      beep();
      Serial.println(msg20);
    }
  });
  server.on("/get/protection", []()
  {
    blinkblueled();
    clientaddress = server.client().remoteIP().toString();
    Serial.println(msg13 + clientaddress + ".");
    if (clientaddress == allowedaddress)
    {
      if (checkusername() == 1)
      {
        prevtime = millis();
        line = String((int)ocprot);
        server.send(200, "text/plain", line);
      } else
      {
        server.send(401, "text/plain", msg17);
        Serial.println(msg18);
      }
    } else
    {
      server.send(401, "text/plain", msg19);
      beep();
      beep();
      beep();
      Serial.println(msg20);
    }
  });
  server.on("/set/all/off", []()
  {
    blinkblueled();
    clientaddress = server.client().remoteIP().toString();
    Serial.println(msg13 + clientaddress + ".");
    if (clientaddress == allowedaddress)
    {
      if (checkusername() == 1)
      {
        prevtime = millis();
        heat = 0;
        lamp = 0;
        vent = 0;
        server.send(200, "text/plain", "* All outputs are switched off.");
      } else
      {
        server.send(401, "text/plain", msg17);
        Serial.println(msg18);
      }
    } else
    {
      server.send(401, "text/plain", msg19);
      beep();
      beep();
      beep();
      Serial.println(msg20);
    }
  });
  server.on("/set/alarm/off", []()
  {
    blinkblueled();
    clientaddress = server.client().remoteIP().toString();
    Serial.println(msg13 + clientaddress + ".");
    if (clientaddress == allowedaddress)
    {
      if (checkusername() == 1)
      {
        prevtime = millis();
        alarm = 0;
        server.send(200, "text/plain", "* Alarm input is restored.");
      } else
      {
        server.send(401, "text/plain", msg17);
        Serial.println(msg18);
      }
    } else
    {
      server.send(401, "text/plain", msg19);
      beep();
      beep();
      beep();
      Serial.println(msg20);
    }
  });
  server.on("/set/heater/off", []()
  {
    blinkblueled();
    clientaddress = server.client().remoteIP().toString();
    Serial.println(msg13 + clientaddress + ".");
    if (clientaddress == allowedaddress)
    {
      if (checkusername() == 1)
      {
        prevtime = millis();
        heat = 0;
        server.send(200, "text/plain", "* Heater is switched off.");
      } else
      {
        server.send(401, "text/plain", msg17);
        Serial.println(msg18);
      }
    } else
    {
      server.send(401, "text/plain", msg19);
      beep();
      beep();
      beep();
      Serial.println(msg20);
    }
  });
  server.on("/set/heater/on", []()
  {
    blinkblueled();
    clientaddress = server.client().remoteIP().toString();
    Serial.println(msg13 + clientaddress + ".");
    if (clientaddress == allowedaddress)
    {
      if (checkusername() == 1)
      {
        prevtime = millis();
        heat = 1;
        server.send(200, "text/plain", "* Heater is switched on.");
      } else
      {
        server.send(401, "text/plain", msg17);
        Serial.println(msg18);
      }
    } else
    {
      server.send(401, "text/plain", msg19);
      beep();
      beep();
      beep();
      Serial.println(msg20);
    }
  });
  server.on("/set/lamp/off", []()
  {
    blinkblueled();
    clientaddress = server.client().remoteIP().toString();
    Serial.println(msg13 + clientaddress + ".");
    if (clientaddress == allowedaddress)
    {
      if (checkusername() == 1)
      {
        prevtime = millis();
        lamp = 0;
        server.send(200, "text/plain", "* Lamp is switched off.");
      } else
      {
        server.send(401, "text/plain", msg17);
        Serial.println(msg18);
      }
    } else
    {
      server.send(401, "text/plain", msg19);
      beep();
      beep();
      beep();
      Serial.println(msg20);
    }
  });
  server.on("/set/lamp/on", []()
  {
    blinkblueled();
    clientaddress = server.client().remoteIP().toString();
    Serial.println(msg13 + clientaddress + ".");
    if (clientaddress == allowedaddress)
    {
      if (checkusername() == 1)
      {
        prevtime = millis();
        lamp = 1;
        server.send(200, "text/plain", "* Lamp is switched on.");
      } else
      {
        server.send(401, "text/plain", msg17);
        Serial.println(msg18);
      }
    } else
    {
      server.send(401, "text/plain", msg19);
      beep();
      beep();
      beep();
      Serial.println(msg20);
    }
  });
  server.on("/set/ventilator/off", []()
  {
    blinkblueled();
    clientaddress = server.client().remoteIP().toString();
    Serial.println(msg13 + clientaddress + ".");
    if (clientaddress == allowedaddress)
    {
      if (checkusername() == 1)
      {
        prevtime = millis();
        vent = 0;
        server.send(200, "text/plain", "* Ventilator is switched off.");
      } else
      {
        server.send(401, "text/plain", msg17);
        Serial.println(msg18);
      }
    } else
    {
      server.send(401, "text/plain", msg19);
      beep();
      beep();
      beep();
      Serial.println(msg20);
    }
  });
  server.on("/set/ventilator/on", []()
  {
    blinkblueled();
    clientaddress = server.client().remoteIP().toString();
    Serial.println(msg13 + clientaddress + ".");
    if (clientaddress == allowedaddress)
    {
      if (checkusername() == 1)
      {
        prevtime = millis();
        vent = 1;
        server.send(200, "text/plain", "* Ventilator is switched on.");
      } else
      {
        server.send(401, "text/plain", msg17);
        Serial.println(msg18);
      }
    } else
    {
      server.send(401, "text/plain", msg19);
      beep();
      beep();
      beep();
      Serial.println(msg20);
    }
  });
  server.begin();
  Serial.println(msg08);
}

// loop function
void loop(void)
{
  int adcvalue;

  server.handleClient();
  currtime = millis();
  if (currtime - prevtime >= interval)
  {
    timeout = 1;
    digitalWrite(prt_led_blue, HIGH);
    Serial.print(msg14);
  } else
  {
    timeout = 0;
    digitalWrite(prt_led_blue, LOW);
  }
  portread();
  adcvalue = analogRead(prt_in_adc);
  delay(100);
  // alarm
  if ((adcvalue < alarmminlevel) || (adcvalue > alarmmaxlevel))
  {
    alarm = 1;
  }
  // warning and error
  error = 0;
  if ((swmanu == 1) || (ocprot == 1) || (alarm == 1) || (timeout == 1))
  {
    error = 1;
  }
  // error message
  if (ocprot == 1)
  {
    Serial.println(msg23);
  }
  // error sound
  if ((ocprot == 1) || (alarm == 1))
  {
    beep();
  }
  portwrite();
}

void handleNotFound()
{
  server.send(404, "text/plain", msg21);
  Serial.println(msg22);
}

// blink blue LED
void blinkblueled()
{
  digitalWrite(prt_led_blue, HIGH);
  delay(500);
  digitalWrite(prt_led_blue, LOW);
}

// read inputs
void portread()
{
  swmanu = digitalRead(prt_in_swmanu);
  ocprot = digitalRead(prt_in_ocprot);
  opmode = digitalRead(prt_in_opmode);
}

// write outputs
void portwrite()
{
  digitalWrite(prt_led_red, error);
  digitalWrite(prt_out_lamp, lamp);
  digitalWrite(prt_out_vent, vent);
  digitalWrite(prt_out_heat, heat);
}

// authentication
int checkusername()
{
  if (server.arg("username") == www_username)
  {
    return 1;
  } else
  {
    beep();
    beep();
    return 0;
  }
}

// beep
void beep()
{
  tone(prt_buzzer, 880);
  delay (100);
  noTone(prt_buzzer);
  delay (100);
}
