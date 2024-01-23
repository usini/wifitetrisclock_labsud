#include <SerialCommands.h>
#include "usbcmd.h"

void serialInit();
void serialLoop();

char usbCommandBuffer[256];
SerialCommands usbCommands(&Serial, usbCommandBuffer,
                               sizeof(usbCommandBuffer), "\r\n", "§");
SerialCommand serialgetTime("getTime", cmdGetTime);
SerialCommand serialsetTime("setTime", cmdSetTime);

void serialInit()
{
    usbCommands.SetDefaultHandler(cmdUnrecognized);
    usbCommands.AddCommand(&serialgetTime);
    usbCommands.AddCommand(&serialsetTime);
}

void serialLoop()
{
    usbCommands.ReadSerial();
}