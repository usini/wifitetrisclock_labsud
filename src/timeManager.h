int text_offset = 0;
String getTime()
{
    DateTime now = rtc.now();
    String hour = "";
    String minute = "";

    if (now.hour() < 10)
    {
        hour = "0" + String(now.hour());
    }
    else
    {
        hour = String(now.hour());
    }
    if (now.minute() < 10)
    {
        minute = "0" + String(now.minute());
    }
    else
    {
        minute = now.minute();
    }
    timeString = String(hour + ":" + minute);
    Serial.println("[⏲️TIME] - " + timeString);
    return timeString;
}

int lastMinute = -1;
int lastDay = -1;
int lastState = -1;

void checkEvent()
{
    Serial.println("Check Event");
    DateTime now = rtc.now();
    if (lastMinute != now.minute())
    {
        Serial.println("Minute check");
        lastMinute = now.minute();
        int state = -1;
        if (now.dayOfTheWeek() == 1 || now.dayOfTheWeek() == 3 || now.dayOfTheWeek() == 4)
        {
            Serial.println("Normal Day");
            if (now.hour() >= 14 && now.hour() <= 19)
            {
                state = 1;
            }
            else if (now.hour() >= 9 && now.hour() < 14)
            {
                state = 2;
            } else {
                state = 0;
            }
        }
        else if (now.dayOfTheWeek() == 2 || now.dayOfTheWeek() == 5)
        {
            Serial.println("Nocturne");
            if (now.hour() >= 14 && now.hour() <= 23)
            {
                state = 1;
            }
            else if (now.hour() >= 9 && now.hour() < 14)
            {
                state = 2;
            } else {
                state = 0;
            }
        }
        else
        {
            Serial.println("WeekEnd");
            state = 0;
        }

        Serial.println(lastState);
        Serial.println(state);
        if (lastState != state)
        {
            Serial.println("Change state");
            lastState = state;
            switch (state)
            {
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
        }
    }
}
