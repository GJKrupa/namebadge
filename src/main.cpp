#include <Arduino.h>

#include <GxEPD.h>
#include <GxGDEH029A1/GxGDEH029A1.cpp>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <GxIO/GxIO.cpp>
#include "GxIO/GxIO_SPI/GxIO_SPI.cpp"

#ifndef BUTTON_PIN
#define BUTTON_PIN 2
#define RTC_BUTTON_PIN GPIO_NUM_2
#endif

GxIO_Class io(SPI, SS, 17, 16);

GxEPD_Class display(io, 16, 4);

struct page_t {
    const char *text;
    const GFXfont *font;
};

const struct page_t DATA[] = {
    {"Gerard Krupa", &FreeMonoBold18pt7b},
    {"BRB", &FreeMonoBold24pt7b},
    {"AFK", &FreeMonoBold24pt7b},
    {"Out to Lunch", &FreeMonoBold18pt7b},
    {"Do Not Disturb", &FreeMonoBold18pt7b},
    {"Busy", &FreeMonoBold24pt7b},
    {NULL, NULL}
};

RTC_DATA_ATTR int page_index = -1;

long lastPress = millis();
bool first_wake_boot = false;

void show_text(const int index) {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setFont(DATA[index].font);
    display.setRotation(1);

    int16_t x, y;
    uint16_t w, h;
    display.getTextBounds((char*)(DATA[index].text), 0, 0, &x, &y, &w, &h);

    int16_t x2 = (display.width() - w) / 2;
    int16_t y2 = (display.height() - h) / 2;

    display.setCursor(x2-x, y2-y);
    display.println(DATA[index].text);
    display.update();
}

void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println("setup");

    display.init();
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    if (page_index == -1) {
        Serial.println("Fresh boot - showing initial screen");
        show_text(0);
        page_index = 0;
    } else {
        if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
            Serial.println("Woke due to interrupt");
            
            first_wake_boot = true;
        }
    }

    Serial.println("setup done");
}

void loop() {
    int buttonState = digitalRead(BUTTON_PIN);
    if (buttonState == LOW || first_wake_boot) {
        first_wake_boot = false;
        lastPress = millis();
        Serial.println("Pressed");
        page_index = (page_index + 1);
        if (DATA[page_index].text == NULL) {
            page_index = 0;
        }
        show_text(page_index);
        Serial.println("Refreshed");
    } else if (millis() - lastPress > 5000) {
        Serial.println("Entering sleep");
        gpio_pulldown_dis(RTC_BUTTON_PIN);
        gpio_pullup_en(RTC_BUTTON_PIN);
        esp_sleep_enable_ext0_wakeup(RTC_BUTTON_PIN, LOW);
        esp_deep_sleep_start();
    } else {
        delay(50);
    }
}