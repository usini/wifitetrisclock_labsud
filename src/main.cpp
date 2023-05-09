/*******************************************************************
    Tetris clock using a 64x32 RGB Matrix that fetches its
    time over WiFi using the EzTimeLibrary.

    For use with my I2S Matrix Shield.

    Parts Used:
    ESP32 D1 Mini * - https://s.click.aliexpress.com/e/_dSi824B
    ESP32 I2S Matrix Shield (From my Tindie) = https://www.tindie.com/products/brianlough/esp32-i2s-matrix-shield/

      = Affilate

    If you find what I do useful and would like to support me,
    please consider becoming a sponsor on Github
    https://github.com/sponsors/witnessmenow/

    Written by Brian Lough
    YouTube: https://www.youtube.com/brianlough
    Tindie: https://www.tindie.com/stores/brianlough/
    Twitter: https://twitter.com/witnessmenow
 *******************************************************************/

// Rename secrets.default.h to secrets.h
#include "secrets.h"

// ----------------------------
// Standard Libraries - Already Installed if you have ESP32 set up
// ----------------------------

#include <WiFi.h>

// ----------------------------
// Additional Libraries - each one of these will need to be installed.
// ----------------------------

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
// This is the library for interfacing with the display

// Can be installed from the library manager (Search for "ESP32 MATRIX DMA")
// https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA

#include <TetrisMatrixDraw.h>
// This library draws out characters using a tetris block
// amimation
// Can be installed from the library manager
// https://github.com/toblum/TetrisAnimation

#include <ezTime.h>
// Library used for getting the time and adjusting for DST
// Search for "ezTime" in the Arduino Library manager
// https://github.com/ropg/ezTime

#include <PubSubClient.h>
WiFiClient espClient;
PubSubClient client(espClient);
// ----------------------------
// Dependency Libraries - each one of these will need to be installed.
// ----------------------------

// Adafruit GFX library is a dependancy for the matrix Library
// Can be installed from the library manager
// https://github.com/adafruit/Adafruit-GFX-Library


// Set a timezone using the following list
// https://en.wikipedia.org/wiki/List_of_tz_database_time_zones
#define MYTIMEZONE "Europe/Paris"

// -------------------------------------
// -------   Matrix Config   ------
// -------------------------------------

const int panelResX = 64;  // Number of pixels wide of each INDIVIDUAL panel module.
const int panelResY = 32;  // Number of pixels tall of each INDIVIDUAL panel module.
const int panel_chain = 1; // Total number of panels chained one to another.

// -------------------------------------
// -------   Clock Config   ------
// -------------------------------------

// Sets whether the clock should be 12 hour format or not.
bool twelveHourFormat = false;

// If this is set to false, the number will only change if the value behind it changes
// e.g. the digit representing the least significant minute will be replaced every minute,
// but the most significant number will only be replaced every 10 minutes.
// When true, all digits will be replaced every minute.
bool forceRefresh = true;

// -----------------------------

MatrixPanel_I2S_DMA *dma_display = nullptr;

portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
hw_timer_t *animationTimer = NULL;

unsigned long animationDue = 0;
unsigned long animationDelay = 20; // Smaller number == faster

uint16_t myBLACK = dma_display->color565(0, 0, 0);

TetrisMatrixDraw tetris(*dma_display);  // Main clock
TetrisMatrixDraw tetris2(*dma_display); // The "M" of AM/PM
TetrisMatrixDraw tetris3(*dma_display); // The "P" or "A" of AM/PM

Timezone myTZ;
unsigned long oneSecondLoopDue = 0;

bool showColon = true;
volatile bool finishedAnimating = false;
bool displayIntro = false;

String lastDisplayedTime = "";
String lastDisplayedAmPm = "";

int text_offset = 0;

const int y_offset = (panelResY / 2) - 6;

void callback(char *topic, byte *payload, unsigned int length)
{
  // handle message arrived
  payload[length] = '\0';
  String s = String((char*)payload);
  int state = s.toInt();

  Serial.print("STATE:");
  Serial.println(state);
  switch(state){
    case 0:
    Serial.println("Ferme");
    text_offset = 15;
    tetris2.setText("FERME");

    break;
    case 1:
    Serial.println("Ouvert");
    text_offset = 10;
    tetris2.setText("OUVERT");
    break;
    case 2:
    Serial.println("Ouvert Pro");
    text_offset = 0;
    tetris2.setText("OUVERTPRO");
    break;
    default:
    Serial.println("Bug");
    tetris2.setText("FERME");
    break;
  }
  //finishedAnimating = true;
}



// This method is for controlling the tetris library draw calls
void animationHandler()
{
  // Not clearing the display and redrawing it when you
  // dont need to improves how the refresh rate appears
  if (!finishedAnimating)
  {
    dma_display->fillScreen(myBLACK);
    if (displayIntro)
    {
      finishedAnimating = tetris.drawText(1, 5 + y_offset);
    }
    else
    {
      if (twelveHourFormat)
      {
        // Place holders for checking are any of the tetris objects
        // currently still animating.
        bool tetris1Done = false;
        bool tetris2Done = false;
        bool tetris3Done = false;

        tetris1Done = tetris.drawNumbers(-6, 10 + y_offset, showColon);
        tetris2Done = tetris2.drawText(56, 9 + y_offset);

        // Only draw the top letter once the bottom letter is finished.
        if (tetris2Done)
        {
          tetris3Done = tetris3.drawText(56, -1 + y_offset);
        }

        finishedAnimating = tetris1Done && tetris2Done && tetris3Done;
      }
      else
      {
        bool tetris2Done = false;
        tetris2Done = tetris2.drawText(text_offset, 30);
        finishedAnimating = tetris.drawNumbers(2, 10 + y_offset, showColon);
      }
    }
    dma_display->flipDMABuffer();
  }
}

void drawIntro(int x = 0, int y = 0)
{
  dma_display->fillScreen(myBLACK);
  tetris.drawChar("L", x, y, tetris.tetrisWHITE);
  tetris.drawChar("A", x + 5, y, tetris.tetrisWHITE);
  tetris.drawChar("B", x + 11, y, tetris.tetrisWHITE);
  tetris.drawChar("S", x + 17, y, tetris.tetrisORANGE);
  tetris.drawChar("U", x + 22, y, tetris.tetrisORANGE);
  tetris.drawChar("D", x + 27, y, tetris.tetrisORANGE);
  dma_display->flipDMABuffer();
}

void drawConnecting(int x = 0, int y = 0)
{
  dma_display->fillScreen(myBLACK);
  tetris.drawChar("C", x, y, tetris.tetrisCYAN);
  tetris.drawChar("o", x + 5, y, tetris.tetrisMAGENTA);
  tetris.drawChar("n", x + 11, y, tetris.tetrisYELLOW);
  tetris.drawChar("n", x + 17, y, tetris.tetrisGREEN);
  tetris.drawChar("e", x + 22, y, tetris.tetrisBLUE);
  tetris.drawChar("x", x + 27, y, tetris.tetrisRED);
  tetris.drawChar("i", x + 32, y, tetris.tetrisWHITE);
  tetris.drawChar("o", x + 37, y, tetris.tetrisMAGENTA);
  tetris.drawChar("n", x + 42, y, tetris.tetrisYELLOW);
  tetris.drawChar(".", x + 47, y, tetris.tetrisGREEN);
  dma_display->flipDMABuffer();
}

void setup()
{
  Serial.begin(115200);

  HUB75_I2S_CFG mxconfig(
      panelResX,  // Module width
      panelResY,  // Module height
      panel_chain // Chain length
  );

  mxconfig.double_buff = true;
  mxconfig.gpio.e = 18;

  // May or may not be needed depending on your matrix
  // Example of what needing it looks like:
  // https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA/issues/134#issuecomment-866367216
  // mxconfig.clkphase = false;

  // mxconfig.driver = HUB75_I2S_CFG::FM6126A;

  // Display Setup
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();
  dma_display->setRotation(2);
  dma_display->fillScreen(myBLACK);

  tetris.display = dma_display;  // Main clock
  tetris2.display = dma_display; // The "M" of AM/PM
  tetris3.display = dma_display; // The "P" or "A" of AM/PM

  // Attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);

  // Set WiFi to station mode and disconnect from an AP if it was Previously
  // connected
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer(ip_mqtt, 1883);
  client.setCallback(callback);

  // "connecting"
  drawConnecting(5, -6 + y_offset);

  // Setup EZ Time
  setDebug(INFO);
  waitForSync();

  Serial.println();
  Serial.println("UTC:             " + UTC.dateTime());

  myTZ.setLocation(F(MYTIMEZONE));
  Serial.print(F("Time in your set timezone:         "));
  Serial.println(myTZ.dateTime());

  // "Powered By"
  //drawIntro(6, -4 + y_offset);
  //delay(2000);

  // Start the Animation Timer
  //tetris.setText("TRINITY");
  // tetris2.setText("");
  // Wait for the animation to finish
  while (!finishedAnimating)
  {
    delay(animationDelay);
    animationHandler();
  }
  delay(2000);
  finishedAnimating = false;
  displayIntro = false;
  tetris.scale = 2;
  // tetris.setTime("00:00", true);
}

void setMatrixTime()
{
  String timeString = "";
  String AmPmString = "";
  if (twelveHourFormat)
  {
    // Get the time in format "1:15" or 11:15 (12 hour, no leading 0)
    // Check the EZTime Github page for info on
    // time formatting
    timeString = myTZ.dateTime("g:i");

    // If the length is only 4, pad it with
    //  a space at the beginning
    if (timeString.length() == 4)
    {
      timeString = " " + timeString;
    }

    // Get if its "AM" or "PM"
    AmPmString = myTZ.dateTime("A");
    if (lastDisplayedAmPm != AmPmString)
    {
      Serial.println(AmPmString);
      lastDisplayedAmPm = AmPmString;
      // Second character is always "M"
      // so need to parse it out
      tetris2.setText("M", forceRefresh);

      // Parse out first letter of String
      tetris3.setText(AmPmString.substring(0, 1), forceRefresh);
    }
  }
  else
  {
    // Get time in format "01:15" or "22:15"(24 hour with leading 0)
    timeString = myTZ.dateTime("H:i");
  }

  // Only update Time if its different
  if (lastDisplayedTime != timeString)
  {
    Serial.println(timeString);
    lastDisplayedTime = timeString;
    tetris.setTime(timeString, forceRefresh);

    // Must set this to false so animation knows
    // to start again
    finishedAnimating = false;
  }
}

void handleColonAfterAnimation()
{
  // It will draw the colon every time, but when the colour is black it
  // should look like its clearing it.
  uint16_t colour = showColon ? tetris.tetrisWHITE : tetris.tetrisBLACK;
  // The x position that you draw the tetris animation object
  int x = twelveHourFormat ? -4 : 2;
  // The y position adjusted for where the blocks will fall from
  // (this could be better!)
  int y = 10 + y_offset - (TETRIS_Y_DROP_DEFAULT * tetris.scale);
  tetris.drawColon(x, y, colour);
  dma_display->flipDMABuffer();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(),mqttuser, mqttpassword)) {
      Serial.println("connected");
      // ... and resubscribe
      client.subscribe("labsud/common/isLabOpen");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop()
{

  if(!client.connected()){
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now > oneSecondLoopDue)
  {
    // We can call this often, but it will only
    // update when it needs to
    setMatrixTime();
    showColon = !showColon;

    // To reduce flicker on the screen we stop clearing the screen
    // when the animation is finished, but we still need the colon to
    // to blink
    if (finishedAnimating)
    {
      handleColonAfterAnimation();
    }
    oneSecondLoopDue = now + 1000;
  }
  now = millis();
  if (now > animationDue)
  {
    animationHandler();
    animationDue = now + animationDelay;
  }
}
