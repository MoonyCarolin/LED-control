/*
TCP-Server for RGB-Control
Change SSID and PASSWORD.
*/

#define SSID ""
#define PASSWORD ""

#define PIN 9
#define NUMPIXELS 16

#define PIN_2 10
#define NUMPIXELS_2 16

#define LED_WLAN 13

#define RED 3
#define GREEN 5
#define BLUE 6
#define GND 4

#define DEBUG true

//Alternative Progmem: Converted with http://www.percederberg.net/tools/text_converter.html to C-String Text and saved as char
const char site[] PROGMEM = {
"<HTML><HEAD>\n<link rel=\"icon\" href=\"data:;base64,iVBORw0KGgo=\">\n<meta name=\"viewport\" content=\"width=device-width, initial-scale=2.0, user-scalable=yes\">\n<title>\nRGB LED\n</title>\n</HEAD>\n\n<BODY bgcolor=\"#FFFF99\" text=\"#000000\">\n<FONT size=\"6\" FACE=\"Verdana\">\nSelect Color\n</FONT>\n\n<HR>\n<BR>\n<FONT size=\"3\" FACE=\"Verdana\">\nChange the Color<BR>\nof the RGB-LED\n<BR>\n<BR>\n<form method=\"GET\">\n\t<input type=\"color\" name=\"rgb\" onchange=\"this.form.submit()\"><BR>\n</form>\n<HR>\n<BR>\n<FONT size=\"3\" FACE=\"Verdana\">\nChange the Color<BR>\nof the RGB-LED 2\n<BR>\n<BR>\n<form method=\"GET\">\n\t<input type=\"color\" name=\"rgb_2\" onchange=\"this.form.submit()\"><BR>\n</form>\n<HR>\n<BR>\n\n</font>\n</HTML>\0"
};

#include <Adafruit_NeoPixel.h>

#include <SoftwareSerial.h>
SoftwareSerial esp8266(11, 12); // RX, TX

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

Adafruit_NeoPixel pixels_2 = Adafruit_NeoPixel(NUMPIXELS_2, PIN_2, NEO_GRB + NEO_KHZ800);

void setup() {
  pixels.begin();
  pixels.show(); // Initialize all pixels to 'off'
  pixels_2.begin();
  pixels_2.show(); // Initialize all pixels to 'off'
  Serial.begin(19200);
  esp8266.begin(19200);

  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);

  pinMode(GND, OUTPUT);
  digitalWrite(GND, LOW);

  if (!espConfig()) serialDebug();
  else digitalWrite(LED_WLAN, HIGH);

  if (configTCPServer())  debug("Server Aktiv"); else debug("Server Error");
}

void loop() {
  String xBuffer;
  if (esp8266.available()) // check if the esp is sending a message
  {
    if (esp8266.find("+IPD,"))
    {
      debug("Incomming Request");
      int connectionId = esp8266.parseInt();

      esp8266.find("?");
    //  Serial.println(esp8266);

      String holestring = esp8266.readStringUntil(' ');
     // Serial.println(holestring);

String marker1 = "rgb=";

      if (holestring.startsWith(String(marker1)))
      {
        String hexstring = holestring.substring(marker1.length());
        Serial.println(hexstring);
        long number = (long) strtol( &hexstring[3], NULL, 16); //Convert String to Hex http://stackoverflow.com/questions/23576827/arduino-convert-a-sting-hex-ffffff-into-3-int
        // Split them up into r, g, b values
        int r = number >> 16;
        int g = number >> 8 & 0xFF;
        int b = number & 0xFF;

        for(int i=0;i<NUMPIXELS;i++){
          pixels.setPixelColor(i, pixels.Color(r, g, b));
          }
        pixels.show();
      }

String marker2 = "rgb_2=";

      if (holestring.startsWith(String(marker2)))
      {
        String hexstring = holestring.substring(marker2.length());
        Serial.println(hexstring);
        long number = (long) strtol( &hexstring[3], NULL, 16); //Convert String to Hex http://stackoverflow.com/questions/23576827/arduino-convert-a-sting-hex-ffffff-into-3-int
        // Split them up into r, g, b values
        int r = number >> 16;
        int g = number >> 8 & 0xFF;
        int b = number & 0xFF;

        for(int i=0;i<NUMPIXELS_2;i++){
          pixels_2.setPixelColor(i, pixels_2.Color(r, g, b));
          }
        pixels_2.show();
      }









      if (sendWebsite(connectionId, createWebsite())) debug("Website send OK"); else debug("Website send Error");
    }
  }
}

boolean sendWebsite(int connectionId, String webpage)
{
  boolean succes = true;

  if (sendCom("AT+CIPSEND=" + String(connectionId) + "," + String(webpage.length()), ">"))
  {
    esp8266.print(webpage);
    esp8266.find("SEND OK");
    succes &= sendCom("AT+CIPCLOSE=" + String(connectionId), "OK");
  }
  else
  {
    succes = false;
  }
  return succes;
}

String createWebsite()
{
  String xBuffer;

  for (int i = 0; i <= sizeof(site); i++)
  {
    char myChar = pgm_read_byte_near(site + i);
    xBuffer += myChar;
  }

  return xBuffer;
}

//-----------------------------------------Config ESP8266------------------------------------

boolean espConfig()
{
  boolean succes = true;
  esp8266.setTimeout(5000);
  succes &= sendCom("AT+RST", "ready");
  esp8266.setTimeout(1000);
  if (configStation(SSID, PASSWORD)) {
    succes &= true;
    debug("WLAN Connected");
    debug("My IP is:");
    debug(sendCom("AT+CIFSR"));
  }
  else
  {
    succes &= false;
  }
  //shorter Timeout for faster wrong UPD-Comands handling
  succes &= sendCom("AT+CIPMODE=0", "OK");
  succes &= sendCom("AT+CIPMUX=0", "OK");

  return succes;
}

boolean configTCPServer()
{
  boolean succes = true;

  succes &= (sendCom("AT+CIPMUX=1", "OK"));
  succes &= (sendCom("AT+CIPSERVER=1,80", "OK"));

  return succes;

}

boolean configTCPClient()
{
  boolean succes = true;

  succes &= (sendCom("AT+CIPMUX=0", "OK"));
  //succes &= (sendCom("AT+CIPSERVER=1,80", "OK"));

  return succes;

}


boolean configStation(String vSSID, String vPASSWORT)
{
  boolean succes = true;
  succes &= (sendCom("AT+CWMODE=1", "OK"));
  esp8266.setTimeout(20000);
  succes &= (sendCom("AT+CWJAP=\"" + String(vSSID) + "\",\"" + String(vPASSWORT) + "\"", "OK"));
  esp8266.setTimeout(1000);
  return succes;
}

boolean configAP()
{
  boolean succes = true;

  succes &= (sendCom("AT+CWMODE=2", "OK"));
  succes &= (sendCom("AT+CWSAP=\"NanoESP\",\"\",5,0", "OK"));

  return succes;
}

boolean configUDP()
{
  boolean succes = true;

  succes &= (sendCom("AT+CIPMODE=0", "OK"));
  succes &= (sendCom("AT+CIPMUX=0", "OK"));
  succes &= sendCom("AT+CIPSTART=\"UDP\",\"192.168.255.255\",90,91,2", "OK"); //Importand Boradcast...Reconnect IP
  return succes;
}

//-----------------------------------------------Controll ESP-----------------------------------------------------

boolean sendUDP(String Msg)
{
  boolean succes = true;

  succes &= sendCom("AT+CIPSEND=" + String(Msg.length() + 2), ">");    //+",\"192.168.4.2\",90", ">");
  if (succes)
  {
    succes &= sendCom(Msg, "OK");
  }
  return succes;
}


boolean sendCom(String command, char respond[])
{
  esp8266.println(command);
  if (esp8266.findUntil(respond, "ERROR"))
  {
    return true;
  }
  else
  {
    debug("ESP SEND ERROR: " + command);
    return false;
  }
}

String sendCom(String command)
{
  esp8266.println(command);
  return esp8266.readString();
}



//-------------------------------------------------Debug Functions------------------------------------------------------
void serialDebug() {
  while (true)
  {
    if (esp8266.available())
      Serial.write(esp8266.read());
    if (Serial.available())
      esp8266.write(Serial.read());
  }
}

void debug(String Msg)
{
  if (DEBUG)
  {
    Serial.println(Msg);
  }
}
