#include <Wire.h>
#include <string.h>
#include <stdio.h>
#include <wiinunchuk.h>
#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)

#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic https://github.com/tzapu/WiFiManager#quick-start
#include <ESP8266HTTPClient.h>

const int sclPin = D6; //wemos GPIO every GPIO except d0 is valid for i2c
const int sdaPin = D7; //wemos GPIO every GPIO except d0 is valid for i2c
const int xZero = 142;
const int yZero = 138;

const String user="<YOUR IP CAM USER>";
const String pwd="<YOUR IP CAM PWD>";
char *ySSID = "<YOUR SSID>";

WiFiManager wifiManager;
HTTPClient http;

int httpGet(String url) {
  http.begin(url); //HTTP
  int httpCode = http.GET();
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.print (payload);
    }
    else {
      Serial.print ("Staus code is: ");
      Serial.print (httpCode, DEC);
      Serial.print ("\n\r");
    }
  }
  http.end();
  return httpCode;
}

void calibrate() {
  if (nunchuk_get_data()) {
    nunchuk_calibrate_joy();
    nunchuk_calibrate_accelxy();
    nunchuk_calibrate_accelz();
  }
}

void nunchuk_init_wemos()
{
  Wire.begin(sdaPin, sclPin);
  delay(1);
  Wire.beginTransmission(0x52);  // device address
#if (ARDUINO >= 100)
  Wire.write((uint8_t)0xF0);  // 1st initialisation register
  Wire.write((uint8_t)0x55);  // 1st initialisation value
  Wire.endTransmission();
  delay(1);
  Wire.beginTransmission(0x52);
  Wire.write((uint8_t)0xFB);  // 2nd initialisation register
  Wire.write((uint8_t)0x00);  // 2nd initialisation value
#else
  Wire.send((uint8_t)0xF0);   // 1st initialisation register
  Wire.send((uint8_t)0x55);   // 1st initialisation value
  Wire.endTransmission();
  delay(1);
  Wire.beginTransmission(0x52);
  Wire.send((uint8_t)0xFB);   // 2nd initialisation register
  Wire.send((uint8_t)0x00);   // 2nd initialisation value
#endif
  Wire.endTransmission();
  delay(1);
  //
  // Set default calibration centres:
  //
  joy_zerox = DEFAULT_CENTRE_JOY_X;
  joy_zeroy = DEFAULT_CENTRE_JOY_Y;
  accel_zerox = ACCEL_ZEROX;
  accel_zeroy = ACCEL_ZEROY;
  accel_zeroz = ACCEL_ZEROZ;
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());

  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void connect_wifi() {
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.autoConnect(ySSID);
  Serial.print ("Wifi connected");
  Serial.print ("\n\r");
}



void init_serial_and_calibrate() {
  Serial.begin (115200);
  nunchuk_init_wemos();
  Serial.print ("Nunchuk intialized");
  Serial.print ("\n\r");

  int i = -1;
  Serial.print ("Calibrating nunchuk....please wait");
  Serial.print ("\n\r");
  for (i = 0; i < 100; i++) {
    calibrate();
    delay(100);
  }
  Serial.print ("Nunchuk calibrated");
  Serial.print ("\n\r");
}

void setup() {
  init_serial_and_calibrate();

}



int detect_x_pos() {
  int xpos = nunchuk_cjoy_x();
  int normX = xpos - xZero;
  if (normX == 0) {
    return 0;
  }
  else if (normX > 0) {
    return 1;
  }
  else if (normX < 0) {
    return -1;
  }
}

int detect_y_pos() {
  int ypos = nunchuk_cjoy_y();
  int normY = ypos - yZero;
  if (normY == 0) {
    return 0;
  }
  else if (normY > 0) {
    return 1;
  }
  else if (normY < 0) {
    return -1;
  }
}

int i = 0;
int fR = 0;
int fL = 0;
int fU = 0;
int fD = 0;

void loop() {
  if (i == 0) {
    i = 1;
    connect_wifi();
  }

  // put your main code here, to run repeatedly:
  if (nunchuk_get_data()) {
    //log data for debug purpose log_data();
    int jX = detect_x_pos();
    int jY = detect_y_pos();
    //Serial.print ("X: ");
    //Serial.print (jY, DEC);
    //Serial.print ("\t");
    //Serial.print ("\r\n");

    //right
    //move right
    if (jX > 0 && fR == 0) {
      httpGet("http://"+user+":"+pwd+"@10.0.0.4/decoder_control.cgi?loginuse="+user+"&loginpas="+pwd+"&command=4&onestep=0");
      fR = 1;
    }
    //stop right
    else if (jX == 0 && fR == 1) {
      httpGet("http://"+user+":"+pwd+"@10.0.0.4/decoder_control.cgi?loginuse="+user+"&loginpas="+pwd+"&command=7&onestep=0");
      fR = 0;
    }

    //left
    //move left
    if (jX < 0 && fL ==0) {
      httpGet("http://"+user+":"+pwd+"@10.0.0.4/decoder_control.cgi?loginuse="+user+"&loginpas="+pwd+"&command=6&onestep=0");
      fL = 1;
    }
    //stop left
    else if (jX == 0 && fL == 1) {
      httpGet("http://"+user+":"+pwd+"@10.0.0.4/decoder_control.cgi?loginuse="+user+"&loginpas="+pwd+"&command=5&onestep=0");
      fL = 0;
    }

    //up
    //move up
    if (jY > 0 && fU == 0) {
      httpGet("http://"+user+":"+pwd+"@10.0.0.4/decoder_control.cgi?loginuse="+user+"&loginpas="+pwd+"&command=0&onestep=0");
      fU = 1;
    }
    //stop up
    else if (jY == 0 && fU == 1) {
      httpGet("http://"+user+":"+pwd+"@10.0.0.4/decoder_control.cgi?loginuse="+user+"&loginpas="+pwd+"&command=3&onestep=0");
      fU = 0;
    }

    //down
    //move down
    if (jY < 0 && fD == 0) {
      httpGet("http://"+user+":"+pwd+"@10.0.0.4/decoder_control.cgi?loginuse="+user+"&loginpas="+pwd+"&command=2&onestep=0");
      fD = 1;
    }
    //stop down
    else if (jY == 0 && fD == 1) {
      httpGet("http://"+user+":"+pwd+"@10.0.0.4/decoder_control.cgi?loginuse="+user+"&loginpas="+pwd+"&command=1&onestep=0");
      fD = 0;
    }

    delay(60);
  }
}

void log_data() {
  Serial.print ("X: ");
  Serial.print (nunchuk_cjoy_x(), DEC);
  Serial.print ("\t");

  Serial.print ("Y: ");
  Serial.print (nunchuk_cjoy_y(), DEC);
  Serial.print ("\t");

  Serial.print ("aX: ");
  Serial.print (nunchuk_caccelx(), DEC);
  Serial.print ("\t");

  Serial.print ("aY: ");
  Serial.print (nunchuk_caccely(), DEC);
  Serial.print ("\t");

  Serial.print ("aZ: ");
  Serial.print (nunchuk_caccelz(), DEC);
  Serial.print ("\t");

  Serial.print ("C: ");
  Serial.print (nunchuk_cbutton(), DEC);
  Serial.print ("\t");

  Serial.print ("C: ");
  Serial.print (nunchuk_zbutton(), DEC);
  Serial.print ("\t");

  Serial.print ("j: ");
  Serial.print (nunchuk_joyangle(), DEC);
  Serial.print ("\t");

  Serial.print ("r: ");
  Serial.print (nunchuk_rollangle(), DEC);
  Serial.print ("\t");

  Serial.print ("p: ");
  Serial.print (nunchuk_pitchangle(), DEC);
  Serial.print ("\t");




  Serial.print ("\r\n");
}





