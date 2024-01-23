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

// ----------------------------
// Additional Libraries - each one of these will need to be installed.
// ----------------------------
#include <Arduino.h>

String timeString;
#include <RTClib.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
// This is the library for interfacing with the display

// Can be installed from the library manager (Search for "ESP32 MATRIX DMA")
// https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA

#include <TetrisMatrixDraw.h>
// This library draws out characters using a tetris block
// amimation
// Can be installed from the library manager
// https://github.com/toblum/TetrisAnimation

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

RTC_DS3231 rtc;
#include "usbcommands.h"
#include "timeManager.h"

unsigned long oneSecondLoopDue = 0;

bool showColon = true;
volatile bool finishedAnimating = false;
bool displayIntro = false;

String lastDisplayedTime = "";
String lastDisplayedAmPm = "";



const int y_offset = (panelResY / 2) - 6;

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
  serialInit();

  if (!rtc.begin())
  {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1)
      delay(10);
  }

  if (rtc.lostPower())
  {
    Serial.println("RTC lost power, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

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


  animationHandler();
  finishedAnimating = false;
  displayIntro = false;
  tetris.scale = 2;
}

void setMatrixTime()
{
  timeString = getTime();
  // Only update Time if its different
  if (lastDisplayedTime != timeString)
  {
    Serial.println("[⏲️TIME] - " + timeString);
    lastDisplayedTime = timeString;
    tetris.setTime(timeString, forceRefresh);
    checkEvent();
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

void loop()
{
  serialLoop();
  unsigned long now = millis();
  if (now > oneSecondLoopDue)
  {
    // We can call this often, but it will only
    // update when it needs to
    setMatrixTime();
    checkEvent();

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
