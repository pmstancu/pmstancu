// Pini pentru ESP32-S3-DevKitC-1 (modul WROOM-1, inclusiv variantele N16R8).
// Evitați: 0, 3, 45, 46 (strapping), 19/20 (USB), 26–32 (flash),
// 33–37 (PSRAM octal pe variantele R8), 43/44 (UART0 al convertorului USB),
// 38/48 (LED RGB de pe placă, după revizie). [VERIFICĂ] pe placa ta.
//
// Pinii display-urilor (SCLK, MOSI, DC, RST) sunt în platformio.ini,
// cerință a bibliotecii TFT_eSPI. Aici sunt doar cele trei CS-uri.
#pragma once

// Magistrală SPI software pentru senzori (separată de display-uri și SD)
#define PIN_SENS_SCK   12
#define PIN_SENS_MOSI  11
#define PIN_SENS_MISO  13
#define PIN_CS_ECT     10   // MAX31865 apă
#define PIN_CS_EOT     14   // MAX31865 ulei
#define PIN_CS_IAT     15   // MAX31865 aer după intercooler
#define PIN_CS_EGT     16   // MAX31856 EGT tip K

// I2C pentru convertorul analog-digital
#define PIN_I2C_SDA    8
#define PIN_I2C_SCL    9
#define ADC_ADDR       0x48  // ADDR la GND

// Ce convertor e montat: 0 = ADS1115 (16 biți), 1 = ADS1015 (12 biți).
// Cablajul e identic. Cu ADS1015 pasul e ~2 mV în loc de 0.125 mV, tot mult
// sub precizia senzorilor (vezi docs/02-lista-componente.md).
#define ADC_CHIP_ADS1015 0

// Canalele ADS1115 / ADS1015
#define ADC_CH_AEM     0    // AEM ieșire analogică 0–5 V (prin divizor)
#define ADC_CH_BOOST   1    // Auber 0.1–4.85 V (prin divizor)
#define ADC_CH_VBAT    2    // baterie (divizor 100k/22k)
#define ADC_CH_5VREF   3    // alimentarea de 5 V a senzorului Auber (prin divizor)

// Display-uri GC9A01: câte un CS pentru fiecare (comandate manual)
#define PIN_CS_DISP1   6
#define PIN_CS_DISP2   1
#define PIN_CS_DISP3   2

// Card microSD pe al doilea SPI hardware (FSPI)
#define PIN_SD_SCK     39
#define PIN_SD_MOSI    40
#define PIN_SD_MISO    41
#define PIN_SD_CS      42

// AEM RS232 -> MAX3232 -> UART1 RX (TX nu e folosit)
#define PIN_AEM_RX     47

// Interfață
#define PIN_BUTTON     21   // la GND, pull-up intern
#define PIN_BUZZER     18   // HIGH = buzzer pornit (prin tranzistor NPN)
