// ============================================================
//  SAHEL SMART SHIELD — SSS-Lite
//  sss_config.h — Configuration centralisée du projet
//
//  Toutes les constantes du système sont définies ici.
//  Ne jamais écrire de valeur "en dur" ailleurs dans le code.
//  Pour modifier un seuil ou un GPIO, on ne touche qu'à ce fichier.
// ============================================================

#pragma once

// ─────────────────────────────────────────────────────────────
//  1. BROCHES GPIO
// ─────────────────────────────────────────────────────────────

// Capteurs
#define PIN_DS18B20       4    // 1-Wire — température externe
#define PIN_SDA           21   // I²C Data  — SHT31 + INA219
#define PIN_SCL           22   // I²C Clock — SHT31 + INA219
#define PIN_ZMPT101B      34   // ADC — détection tension secteur AC

// Communication GSM
#define PIN_SIM800L_RX    16   // UART2 RX (reçoit depuis SIM800L TX)
#define PIN_SIM800L_TX    17   // UART2 TX (envoie vers SIM800L RX)

// Relais (logique Active LOW : LOW = relais fermé = actionneur ON)
#define PIN_RELAY_VENTIL  25   // Relais 1 — Ventilation forcée
#define PIN_RELAY_DETEST  26   // Relais 2 — Délestage charges
#define PIN_RELAY_BASCULE 27   // Relais 3 — Bascule réseau / batterie


// ─────────────────────────────────────────────────────────────
//  2. ADRESSES I²C
// ─────────────────────────────────────────────────────────────

#define ADDR_SHT31        0x44  // SHT31 — température + humidité interne
#define ADDR_INA219       0x40  // INA219 — tension + courant batterie


// ─────────────────────────────────────────────────────────────
//  3. SEUILS THERMIQUES (°C)
//     Alignés sur les normes de gestion thermique d'armoire
// ─────────────────────────────────────────────────────────────

#define SEUIL_CHAUD       38.0f  // Activation ventilation forcée
#define SEUIL_ALERTE      45.0f  // SMS d'alerte technicien
#define SEUIL_CRITIQUE    52.0f  // Délestage charges non prioritaires

// Hystérésis : écart avant retour à l'état inférieur
// Ex : ventilation s'active à 38°C, se coupe à 38 - 3 = 35°C
#define HYSTERESIS        3.0f


// ─────────────────────────────────────────────────────────────
//  4. SEUILS BATTERIE (%)
// ─────────────────────────────────────────────────────────────

#define SEUIL_BAT_URGENCE 20.0f  // SMS urgence + délestage supplémentaire
#define SEUIL_BAT_FAIBLE  40.0f  // Avertissement préventif (pas d'action)
#define TENSION_BAT_MAX   12.6f  // Tension batterie 12V pleine (V)
#define TENSION_BAT_MIN   10.5f  // Tension batterie 12V vide (V)


// ─────────────────────────────────────────────────────────────
//  5. SEUIL DÉTECTION SECTEUR AC
// ─────────────────────────────────────────────────────────────

// Valeur ADC en dessous de laquelle on considère le secteur absent
// À calibrer selon le module ZMPT101B et la tension locale
#define SEUIL_ADC_SECTEUR 100    // Valeur brute ADC (0–4095)


// ─────────────────────────────────────────────────────────────
//  6. ALERTES SMS
// ─────────────────────────────────────────────────────────────

// Numéro du technicien responsable (format international)
#define TEL_TECHNICIEN    "+23566XXXXXXX"

// Délai minimum entre deux SMS d'alerte (ms) — évite le spam
#define DELAI_MIN_SMS     300000UL   // 5 minutes


// ─────────────────────────────────────────────────────────────
//  7. MQTT — SUPERVISION LOCALE
// ─────────────────────────────────────────────────────────────

// Adresse IP du Raspberry Pi sur le réseau local
#define MQTT_BROKER_IP    "192.168.1.100"
#define MQTT_BROKER_PORT  1883
#define MQTT_CLIENT_ID    "SSS-Lite-01"

// Topics de publication (ESP32 → Broker)
#define TOPIC_TEMP_INT    "sss/capteurs/temperature_interne"
#define TOPIC_TEMP_EXT    "sss/capteurs/temperature_externe"
#define TOPIC_HUMIDITE    "sss/capteurs/humidite"
#define TOPIC_SECTEUR     "sss/capteurs/secteur_present"
#define TOPIC_BATTERIE    "sss/capteurs/batterie_pct"
#define TOPIC_ETAT        "sss/systeme/etat"
#define TOPIC_ALERTE      "sss/systeme/alerte"

// Topics d'abonnement (Broker → ESP32, pour commandes futures)
#define TOPIC_CMD         "sss/commandes/#"


// ─────────────────────────────────────────────────────────────
//  8. WIFI
// ─────────────────────────────────────────────────────────────

#define WIFI_SSID         "NOM_RESEAU"
#define WIFI_PASSWORD     "MOT_DE_PASSE"

// Timeout de connexion WiFi (ms)
#define WIFI_TIMEOUT      15000UL


// ─────────────────────────────────────────────────────────────
//  9. TIMING — CYCLES DE LECTURE
// ─────────────────────────────────────────────────────────────

// Intervalle entre deux lectures des capteurs (ms)
#define INTERVALLE_LECTURE    5000UL    // 5 secondes

// Intervalle entre deux publications MQTT (ms)
#define INTERVALLE_MQTT       10000UL   // 10 secondes

// Timeout réponse SIM800L (ms)
#define SIM800L_TIMEOUT       10000UL


// ─────────────────────────────────────────────────────────────
//  10. UART SIM800L
// ─────────────────────────────────────────────────────────────

#define SIM800L_BAUD      9600


// ─────────────────────────────────────────────────────────────
//  11. DÉBOGAGE
// ─────────────────────────────────────────────────────────────

// Mettre à 1 pour activer les messages série détaillés
// Mettre à 0 en production (gain de performance)
#define DEBUG_MODE        1

#define SERIAL_BAUD       115200
