// =============================================================================
// Configurare dispozitiv - TOATE valorile pe care le ajustezi sunt aici.
// Valorile marcate cu [VERIFICĂ] provin din presupuneri sau fișe tehnice
// neconfirmate; confirmă-le cu documentația piesei tale înainte de utilizare.
// =============================================================================
#pragma once

#define FW_VERSION "0.1.0"

// -----------------------------------------------------------------------------
// Pini ESP32-S3-DevKitC-1 (evită pinii de strapping 0/3/45/46 și USB 19/20)
// -----------------------------------------------------------------------------
// Magistrala SPI pentru senzori (software SPI, separată de display)
#define PIN_SENS_SCK   12
#define PIN_SENS_MOSI  11
#define PIN_SENS_MISO  13
#define PIN_CS_ECT     10   // MAX31865 - temperatură lichid răcire (apă)
#define PIN_CS_EOT     14   // MAX31865 - temperatură ulei
#define PIN_CS_IAT     15   // MAX31865 - temperatură aer după intercooler
#define PIN_CS_EGT     16   // MAX31856 - EGT tip K (pre-turbo)

// I2C pentru ADS1115
#define PIN_I2C_SDA    8
#define PIN_I2C_SCL    9
#define ADS1115_ADDR   0x48 // ADDR legat la GND

// Interfață
#define PIN_BUTTON     1    // buton schimbare pagină (la GND, pull-up intern)
#define PIN_BUZZER     21   // buzzer activ prin tranzistor (HIGH = sunet)

// Pinii display-ului GC9A01 sunt în platformio.ini (cerință TFT_eSPI).

// -----------------------------------------------------------------------------
// Pt100 cu MAX31865
// -----------------------------------------------------------------------------
#define RTD_NOMINAL    100.0f  // Pt100
// [VERIFICĂ] Rezistența de referință de pe placa ta MAX31865.
// Adafruit folosește 430 Ω pentru Pt100 (marcaj "431"). Unele clone au 400 Ω.
#define RTD_RREF       430.0f
// 3 fire recomandat (necesită jumperii corespunzători pe placă). Valori: 2, 3, 4
#define RTD_WIRES      3

// -----------------------------------------------------------------------------
// ADS1115 - canale și divizoare de tensiune
// ADS1115 e alimentat la 3.3 V, deci NICIO intrare nu are voie peste 3.3 V.
// Semnalele de 0-5 V trec prin divizor 10k (sus) / 15k (jos) => factor 25/15.
// -----------------------------------------------------------------------------
#define ADC_CH_AFR      0
#define ADC_CH_BOOST    1
#define ADC_CH_VBAT     2
#define ADC_CH_5VREF    3   // tensiunea de 5 V care alimentează senzorul de presiune

#define DIV_5V_SIGNALS  (25.0f / 15.0f)    // 10k + 15k
#define DIV_VBAT        (122.0f / 22.0f)   // 100k + 22k -> ~18 V max la intrare

// -----------------------------------------------------------------------------
// Senzor presiune boost (Auber)
// [VERIFICĂ] Toate cele de mai jos din fișa tehnică a senzorului tău Auber.
// Presupunere: alimentare 5 V, ieșire 0.5-4.5 V liniară, ratiometrică.
// -----------------------------------------------------------------------------
#define BOOST_V_MIN          0.5f    // tensiune la presiune minimă (la 5.00 V alimentare)
#define BOOST_V_MAX          4.5f    // tensiune la presiune maximă
#define BOOST_P_MIN_BAR      0.0f
#define BOOST_P_MAX_BAR      6.895f  // 100 psi - [VERIFICĂ] domeniul real
// 1 dacă senzorul este ABSOLUT (citește ~1 bar la motor oprit), 0 dacă e relativ (gauge)
#define BOOST_SENSOR_ABSOLUTE 0
#define ATMOSPHERIC_BAR      1.013f
// Corecție ratiometrică folosind ADC_CH_5VREF (recomandat: 1)
#define BOOST_RATIOMETRIC    1

// -----------------------------------------------------------------------------
// AEM 30-0310 - ieșire analogică 0-5 V
// [VERIFICĂ] Scalarea din manualul AEM. Valorile de mai jos sunt presupuneri.
// AEM afișează AFR pe scară de benzină (stoichiometric 14.7). Pentru diesel
// interpretează valoarea ca lambda = AFR / 14.7.
// -----------------------------------------------------------------------------
#define AFR_AT_0V     8.5f
#define AFR_AT_5V     18.0f
#define AFR_STOICH_GAS 14.7f

// -----------------------------------------------------------------------------
// Unități afișaj
// -----------------------------------------------------------------------------
#define BOOST_DISPLAY_PSI  1     // 1 = psi, 0 = bar

// -----------------------------------------------------------------------------
// Praguri alarmă: {avertizare, critic, histerezis}
// [VERIFICĂ] Acestea sunt valori de pornire rezonabile, NU limite oficiale
// Toyota sau GTurbo. Ajustează-le după ce afli limitele recomandate.
// "high" = alarmă când valoarea CREȘTE peste prag; "low" = când SCADE sub prag.
// -----------------------------------------------------------------------------
#define ALARM_ECT_WARN   100.0f   // °C
#define ALARM_ECT_CRIT   105.0f
#define ALARM_EOT_WARN   120.0f   // °C
#define ALARM_EOT_CRIT   130.0f
#define ALARM_IAT_WARN    60.0f   // °C după intercooler
#define ALARM_IAT_CRIT    70.0f
#define ALARM_EGT_WARN   650.0f   // °C pre-turbo
#define ALARM_EGT_CRIT   720.0f
#define ALARM_BOOST_WARN   1.59f  // bar relativ (~23 psi); turbina e setată la 22 psi
#define ALARM_BOOST_CRIT   1.72f  // bar relativ (~25 psi)
#define ALARM_VBAT_LOW_WARN  12.0f
#define ALARM_VBAT_LOW_CRIT  11.5f
#define ALARM_VBAT_HIGH_WARN 14.9f
#define ALARM_VBAT_HIGH_CRIT 15.5f

#define HYST_TEMP   2.0f
#define HYST_EGT   15.0f
#define HYST_BOOST  0.05f
#define HYST_VBAT   0.2f

// -----------------------------------------------------------------------------
// Ritmuri și filtrare
// -----------------------------------------------------------------------------
#define TELEMETRY_PERIOD_MS   100   // 10 Hz către CM5
#define DISPLAY_PERIOD_MS      50   // 20 fps țintă
#define ANALOG_PERIOD_MS       25   // citire ADS1115
#define EMA_ALPHA_FAST       0.35f  // boost / AFR
#define EMA_ALPHA_SLOW       0.15f  // tensiune baterie
