// ============================================================
//  SAHEL SMART SHIELD — SSS-Lite
//  sss_sensors.h — Lecture des capteurs physiques
//
//  Capteurs gérés :
//    SHT31    — Température interne + Humidité (I²C 0x44)
//    DS18B20  — Température externe ambiante  (1-Wire GPIO4)
//    INA219   — Tension + Courant batterie    (I²C 0x40)
//    ZMPT101B — Présence réseau secteur AC    (ADC GPIO34)
// ============================================================

#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SHT31.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <INA219_WE.h>

#include "sss_config.h"


// ─────────────────────────────────────────────────────────────
//  STRUCTURE — Toutes les mesures en un seul objet
// ─────────────────────────────────────────────────────────────

struct SSS_Mesures {
    // Valeurs mesurées
    float temp_interne;       // °C  — SHT31   (température dans l'armoire)
    float humidite;           // %   — SHT31   (humidité relative interne)
    float temp_externe;       // °C  — DS18B20 (température ambiante extérieure)
    float tension_batterie;   // V   — INA219  (tension batterie 12V)
    float courant_batterie;   // mA  — INA219  (courant de charge/décharge)
    float pct_batterie;       // %   — calculé depuis la tension
    bool  secteur_present;    // —   — ZMPT101B (true = réseau 230V détecté)

    // Statut de chaque capteur (true = lecture OK)
    bool sht31_ok;
    bool ds18b20_ok;
    bool ina219_ok;
    bool zmpt_ok;
};


// ─────────────────────────────────────────────────────────────
//  OBJETS CAPTEURS (déclarés une seule fois ici)
// ─────────────────────────────────────────────────────────────

static Adafruit_SHT31    sht31;
static OneWire            oneWire(PIN_DS18B20);
static DallasTemperature  ds18b20(&oneWire);
static INA219_WE          ina219(ADDR_INA219);


// ─────────────────────────────────────────────────────────────
//  INITIALISATION — à appeler dans setup()
// ─────────────────────────────────────────────────────────────

bool sss_sensors_init() {
    bool tout_ok = true;

    // Démarrage du bus I²C
    Wire.begin(PIN_SDA, PIN_SCL);
    delay(100);

    // ── SHT31 ──────────────────────────────────────────────
    if (!sht31.begin(ADDR_SHT31)) {
        if (DEBUG_MODE) Serial.println("[ERREUR] SHT31 non détecté (adresse 0x44)");
        tout_ok = false;
    } else {
        sht31.heater(false); // Désactiver le chauffage interne du capteur
        if (DEBUG_MODE) Serial.println("[OK]     SHT31 initialisé");
    }

    // ── DS18B20 ────────────────────────────────────────────
    ds18b20.begin();
    if (ds18b20.getDeviceCount() == 0) {
        if (DEBUG_MODE) Serial.println("[ERREUR] DS18B20 non détecté (GPIO4) — vérifier résistance 4.7kΩ");
        tout_ok = false;
    } else {
        ds18b20.setResolution(12); // Résolution maximale 12 bits (0.0625 °C)
        ds18b20.setWaitForConversion(false); // Mode asynchrone
        if (DEBUG_MODE) Serial.printf("[OK]     DS18B20 initialisé (%d capteur détecté)\n",
                                       ds18b20.getDeviceCount());
    }

    // ── INA219 ─────────────────────────────────────────────
    if (!ina219.init()) {
        if (DEBUG_MODE) Serial.println("[ERREUR] INA219 non détecté (adresse 0x40)");
        tout_ok = false;
    } else {
        // Calibration pour une batterie 12V avec shunt 0.1 Ohm
        ina219.setADCMode(SAMPLE_MODE_128);  // Moyenne sur 128 échantillons
        ina219.setPGain(PG_320);             // Plage ±320 mV
        ina219.setBusRange(BRNG_16);         // Plage bus 16V (suffisant pour 12V)
        if (DEBUG_MODE) Serial.println("[OK]     INA219 initialisé");
    }

    // ── ZMPT101B (ADC) ─────────────────────────────────────
    // GPIO34 = entrée uniquement sur ESP32, pas de configuration nécessaire
    pinMode(PIN_ZMPT101B, INPUT);
    analogReadResolution(12);        // ADC 12 bits (0–4095)
    analogSetAttenuation(ADC_11db);  // Plage 0–3.3V
    if (DEBUG_MODE) Serial.println("[OK]     ZMPT101B prêt (ADC GPIO34)");

    return tout_ok;
}


// ─────────────────────────────────────────────────────────────
//  LECTURE SHT31 — Température interne + Humidité
// ─────────────────────────────────────────────────────────────

void lire_sht31(SSS_Mesures& m) {
    float t = sht31.readTemperature();
    float h = sht31.readHumidity();

    if (!isnan(t) && !isnan(h) && t > -40.0f && t < 125.0f) {
        m.temp_interne = t;
        m.humidite     = h;
        m.sht31_ok     = true;
    } else {
        if (DEBUG_MODE) Serial.println("[ERREUR] Lecture SHT31 échouée — valeur NaN ou hors plage");
        m.sht31_ok = false;
        // On conserve la dernière valeur connue — ne pas mettre à 0
    }
}


// ─────────────────────────────────────────────────────────────
//  LECTURE DS18B20 — Température externe
// ─────────────────────────────────────────────────────────────

void lire_ds18b20(SSS_Mesures& m) {
    ds18b20.requestTemperatures();

    // Attendre la conversion (94 ms en 12 bits)
    unsigned long debut = millis();
    while (!ds18b20.isConversionComplete()) {
        if (millis() - debut > 1000) {
            if (DEBUG_MODE) Serial.println("[ERREUR] DS18B20 timeout conversion");
            m.ds18b20_ok = false;
            return;
        }
    }

    float t = ds18b20.getTempCByIndex(0);

    if (t != DEVICE_DISCONNECTED_C && t > -55.0f && t < 125.0f) {
        m.temp_externe = t;
        m.ds18b20_ok   = true;
    } else {
        if (DEBUG_MODE) Serial.println("[ERREUR] DS18B20 déconnecté ou hors plage");
        m.ds18b20_ok = false;
    }
}


// ─────────────────────────────────────────────────────────────
//  LECTURE INA219 — Tension + Courant + % batterie
// ─────────────────────────────────────────────────────────────

void lire_ina219(SSS_Mesures& m) {
    float tension = ina219.getBusVoltage_V();
    float courant = ina219.getCurrent_mA();

    // Vérification de cohérence
    if (tension > 0.0f && tension <= 20.0f) {
        m.tension_batterie = tension;
        m.courant_batterie = courant;

        // Calcul du pourcentage basé sur la plage de tension définie dans sss_config.h
        float pct = (tension - TENSION_BAT_MIN) /
                    (TENSION_BAT_MAX - TENSION_BAT_MIN) * 100.0f;
        m.pct_batterie = constrain(pct, 0.0f, 100.0f);
        m.ina219_ok    = true;
    } else {
        if (DEBUG_MODE) Serial.printf("[ERREUR] INA219 tension hors plage : %.2f V\n", tension);
        m.ina219_ok = false;
    }
}


// ─────────────────────────────────────────────────────────────
//  LECTURE ZMPT101B — Présence réseau secteur AC
//
//  Principe : on échantillonne l'ADC sur une période complète
//  du réseau 50Hz (20 ms). Si l'amplitude dépasse le seuil,
//  le secteur est considéré présent.
// ─────────────────────────────────────────────────────────────

void lire_zmpt101b(SSS_Mesures& m) {
    int valeur_max = 0;
    int valeur_min = 4095;

    // 100 échantillons sur 20 ms = 1 période complète à 50 Hz
    for (int i = 0; i < 100; i++) {
        int val = analogRead(PIN_ZMPT101B);
        if (val > valeur_max) valeur_max = val;
        if (val < valeur_min) valeur_min = val;
        delayMicroseconds(200);
    }

    // L'amplitude crête-à-crête indique la présence du signal AC
    int amplitude = valeur_max - valeur_min;
    m.secteur_present = (amplitude > SEUIL_ADC_SECTEUR);
    m.zmpt_ok         = true;

    if (DEBUG_MODE) {
        Serial.printf("[ZMPT] Amplitude ADC : %d — Secteur : %s\n",
                      amplitude,
                      m.secteur_present ? "PRÉSENT" : "ABSENT");
    }
}


// ─────────────────────────────────────────────────────────────
//  LECTURE COMPLÈTE — Appel unique depuis la boucle principale
// ─────────────────────────────────────────────────────────────

SSS_Mesures lire_tous_les_capteurs() {
    SSS_Mesures m;

    // Valeurs par défaut (sécurité si un capteur est absent)
    m.temp_interne     = 25.0f;
    m.humidite         = 50.0f;
    m.temp_externe     = 25.0f;
    m.tension_batterie = 12.0f;
    m.courant_batterie = 0.0f;
    m.pct_batterie     = 100.0f;
    m.secteur_present  = true;
    m.sht31_ok         = false;
    m.ds18b20_ok       = false;
    m.ina219_ok        = false;
    m.zmpt_ok          = false;

    lire_sht31(m);
    lire_ds18b20(m);
    lire_ina219(m);
    lire_zmpt101b(m);

    return m;
}


// ─────────────────────────────────────────────────────────────
//  AFFICHAGE — Résumé des mesures sur le moniteur série
// ─────────────────────────────────────────────────────────────

void afficher_mesures(const SSS_Mesures& m) {
    if (!DEBUG_MODE) return;

    Serial.println("════════════════════════════════════");
    Serial.printf("  T° interne   : %.1f °C  %s\n",
                  m.temp_interne, m.sht31_ok ? "" : "[ERREUR CAPTEUR]");
    Serial.printf("  Humidité     : %.1f %%\n", m.humidite);
    Serial.printf("  T° externe   : %.1f °C  %s\n",
                  m.temp_externe, m.ds18b20_ok ? "" : "[ERREUR CAPTEUR]");
    Serial.printf("  Batterie     : %.2f V  —  %.0f %%  %s\n",
                  m.tension_batterie, m.pct_batterie,
                  m.ina219_ok ? "" : "[ERREUR CAPTEUR]");
    Serial.printf("  Courant      : %.1f mA  (%s)\n",
                  m.courant_batterie,
                  m.courant_batterie > 0 ? "décharge" : "charge");
    Serial.printf("  Secteur AC   : %s\n",
                  m.secteur_present ? "PRÉSENT" : "ABSENT");
    Serial.println("════════════════════════════════════");
}
