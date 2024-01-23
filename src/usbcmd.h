void cmdUnrecognized(SerialCommands *sender, const char *cmd)
{
    sender->GetSerial()->println("Unrecognized command - " + String(cmd));
}

void cmdGetTime(SerialCommands *sender)
{
    Serial.println("[⏲️TIME] - " + timeString);
}

void cmdSetTime(SerialCommands *sender)
{
    char *value;
    int time[6];
    for (int i = 0; i < 6; i++)
    {
        value = sender->Next();
        time[i] = String(value).toInt();
        Serial.print("TIME-- ");
        Serial.print(i);
        Serial.print(" - ");
        Serial.println(time[i]);
    }
    Serial.println("-----");
    rtc.adjust(DateTime(time[5],time[4],time[3],time[0],time[1],time[2]));
    DateTime now = rtc.now();
    Serial.print(now.year());
    Serial.print("/");
    Serial.print(now.month());
    Serial.print("/");
    Serial.print(now.day());
    Serial.print(" ");
    Serial.print(now.hour());
    Serial.print(":");
    Serial.print(now.minute());
    Serial.print(":");
    Serial.println(now.second());
}

