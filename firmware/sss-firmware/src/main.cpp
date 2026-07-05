// ============================================================
//  SAHEL SMART SHIELD — SSS-Lite
//  main.cpp — Point d'entrée principal du firmware
//
//  Ce fichier assemble toutes les couches du système :
//    - Lecture des capteurs (sss_sensors.h)
//    - Machine à états    (sss_logic.h)
//    - Commande des relais
//    - Publication MQTT   (supervision Raspberry Pi)
//    - Alertes SMS        (module SIM800L)
// ============================================================

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

#include "sss_config.h"
#include "sss_sensors.h"
#include "sss_logic.h"


// ─────────────────────────────────────────────────────────────
//  OBJETS GLOBAUX
// ─────────────────────────────────────────────────────────────

WiFiClient   wifi_client;
PubSubClient mqtt(wifi_client);

SSS_Etat     etat_actuel    = ETAT_NORMAL;
SSS_Mesures  mesures_actuelles;

unsigned long derniere_lecture  = 0;
unsigned long dernier_mqtt      = 0;
unsigned long dernier_sms       = 0;


// ─────────────────────────────────────────────────────────────
//  WIFI — Connexion au réseau local
// ─────────────────────────────────────────────────────────────

void connecter_wifi() {
    if (DEBUG_MODE) Serial.printf("\nConnexion WiFi : %s ", WIFI_SSID);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long debut = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - debut > WIFI_TIMEOUT) {
            if (DEBUG_MODE) Serial.println("\n[WARN] WiFi non disponible — mode autonome");
            return;
        }
        delay(500);
        if (DEBUG_MODE) Serial.print(".");
    }

    if (DEBUG_MODE) {
        Serial.println(" OK");
        Serial.printf("[WiFi] IP : %s\n", WiFi.localIP().toString().c_str());
    }
}


// ─────────────────────────────────────────────────────────────
//  MQTT — Connexion au broker Raspberry Pi
// ─────────────────────────────────────────────────────────────

void connecter_mqtt() {
    if (WiFi.status() != WL_CONNECTED) return;

    mqtt.setServer(MQTT_BROKER_IP, MQTT_BROKER_PORT);

    if (DEBUG_MODE) Serial.print("[MQTT] Connexion broker... ");

    if (mqtt.connect(MQTT_CLIENT_ID)) {
        if (DEBUG_MODE) Serial.println("OK");
        mqtt.subscribe(TOPIC_CMD);
    } else {
        if (DEBUG_MODE) Serial.printf("Echec (code %d)\n", mqtt.state());
    }
}

void assurer_connexion_mqtt() {
    if (WiFi.status() != WL_CONNECTED) {
        connecter_wifi();
        return;
    }
    if (!mqtt.connected()) {
        connecter_mqtt();
    }
}


// ─────────────────────────────────────────────────────────────
//  MQTT — Publication des mesures
// ─────────────────────────────────────────────────────────────

void publier_mqtt(const SSS_Mesures& m, SSS_Etat etat) {
    if (!mqtt.connected()) return;

    char buf[16];

    snprintf(buf, sizeof(buf), "%.1f", m.temp_interne);
    mqtt.publish(TOPIC_TEMP_INT, buf, true);

    snprintf(buf, sizeof(buf), "%.1f", m.temp_externe);
    mqtt.publish(TOPIC_TEMP_EXT, buf, true);

    snprintf(buf, sizeof(buf), "%.1f", m.humidite);
    mqtt.publish(TOPIC_HUMIDITE, buf, true);

    snprintf(buf, sizeof(buf), "%s", m.secteur_present ? "1" : "0");
    mqtt.publish(TOPIC_SECTEUR, buf, true);

    snprintf(buf, sizeof(buf), "%.0f", m.pct_batterie);
    mqtt.publish(TOPIC_BATTERIE, buf, true);

    // Nom de l'état courant
    mqtt.publish(TOPIC_ETAT, sss_nom_etat(etat), true);

    if (DEBUG_MODE) Serial.println("[MQTT] Mesures publiées");
}


// ─────────────────────────────────────────────────────────────
//  SMS — Envoi via SIM800L
// ─────────────────────────────────────────────────────────────

void envoyer_sms(const char* message) {
    // Respecter le délai anti-spam
    if (millis() - dernier_sms < DELAI_MIN_SMS) {
        if (DEBUG_MODE) Serial.println("[SMS] Délai anti-spam actif — SMS ignoré");
        return;
    }

    if (DEBUG_MODE) Serial.printf("[SMS] Envoi vers %s : %s\n", TEL_TECHNICIEN, message);

    Serial2.begin(SIM800L_BAUD, SERIAL_8N1, PIN_SIM800L_RX, PIN_SIM800L_TX);
    delay(100);

    // Mode texte
    Serial2.println("AT+CMGF=1");
    delay(500);

    // Numéro destinataire
    Serial2.printf("AT+CMGS=\"%s\"\r\n", TEL_TECHNICIEN);
    delay(500);

    // Contenu du message
    Serial2.print(message);
    delay(100);

    // Envoi (Ctrl+Z = caractère 26)
    Serial2.write(26);
    delay(3000);

    dernier_sms = millis();
    if (DEBUG_MODE) Serial.println("[SMS] Envoyé");
}


// ─────────────────────────────────────────────────────────────
//  RELAIS — Commande des actionneurs
//  Logique Active LOW : LOW = relais fermé = actionneur ON
// ─────────────────────────────────────────────────────────────

void appliquer_relais(SSS_Etat etat) {
    switch (etat) {
        case ETAT_NORMAL:
            digitalWrite(PIN_RELAY_VENTIL,  HIGH); // Ventilation OFF
            digitalWrite(PIN_RELAY_DETEST,  HIGH); // Délestage OFF
            digitalWrite(PIN_RELAY_BASCULE, HIGH); // Réseau secteur
            break;

        case ETAT_CHAUD:
            digitalWrite(PIN_RELAY_VENTIL,  LOW);  // Ventilation ON
            digitalWrite(PIN_RELAY_DETEST,  HIGH); // Délestage OFF
            digitalWrite(PIN_RELAY_BASCULE, HIGH); // Réseau secteur
            break;

        case ETAT_ALERTE:
            digitalWrite(PIN_RELAY_VENTIL,  LOW);  // Ventilation ON
            digitalWrite(PIN_RELAY_DETEST,  HIGH); // Délestage OFF
            digitalWrite(PIN_RELAY_BASCULE, HIGH); // Réseau secteur
            break;

        case ETAT_CRITIQUE:
            digitalWrite(PIN_RELAY_VENTIL,  LOW);  // Ventilation ON
            digitalWrite(PIN_RELAY_DETEST,  LOW);  // Délestage ON
            digitalWrite(PIN_RELAY_BASCULE, HIGH); // Réseau secteur
            break;

        case ETAT_COUPURE:
        case ETAT_URGENCE:
            digitalWrite(PIN_RELAY_VENTIL,  HIGH); // Ventilation OFF (économie)
            digitalWrite(PIN_RELAY_DETEST,  LOW);  // Délestage ON
            digitalWrite(PIN_RELAY_BASCULE, LOW);  // Bascule batterie
            break;

        case ETAT_RETOUR:
            digitalWrite(PIN_RELAY_VENTIL,  HIGH); // Ventilation OFF
            digitalWrite(PIN_RELAY_DETEST,  HIGH); // Délestage OFF
            digitalWrite(PIN_RELAY_BASCULE, HIGH); // Retour réseau
            break;
    }
}


// ─────────────────────────────────────────────────────────────
//  ALERTES — SMS selon l'état
// ─────────────────────────────────────────────────────────────

void gerer_alertes(SSS_Etat etat, const SSS_Mesures& m) {
    char msg[160];

    switch (etat) {
        case ETAT_ALERTE:
            snprintf(msg, sizeof(msg),
                "SSS-Lite ALERTE : Temperature interne %.1f C. "
                "Ventilation active. Verifier l'armoire.",
                m.temp_interne);
            envoyer_sms(msg);
            break;

        case ETAT_CRITIQUE:
            snprintf(msg, sizeof(msg),
                "SSS-Lite CRITIQUE : Temperature %.1f C. "
                "Delestage enclenche. Intervention requise.",
                m.temp_interne);
            envoyer_sms(msg);
            break;

        case ETAT_COUPURE:
            snprintf(msg, sizeof(msg),
                "SSS-Lite COUPURE SECTEUR. "
                "Systeme sur batterie (%.0f %%). "
                "Verifier alimentation.",
                m.pct_batterie);
            envoyer_sms(msg);
            break;

        case ETAT_URGENCE:
            snprintf(msg, sizeof(msg),
                "SSS-Lite URGENCE : Batterie critique %.0f %%. "
                "Delestage maximum. Intervention immediate.",
                m.pct_batterie);
            envoyer_sms(msg);
            break;

        case ETAT_RETOUR:
            snprintf(msg, sizeof(msg),
                "SSS-Lite : Secteur retabli. "
                "Systeme en mode normal. Batterie %.0f %%.",
                m.pct_batterie);
            envoyer_sms(msg);
            break;

        default:
            break;
    }
}


// ─────────────────────────────────────────────────────────────
//  SETUP
// ─────────────────────────────────────────────────────────────

void setup() {
    if (DEBUG_MODE) {
        Serial.begin(SERIAL_BAUD);
        delay(500);
        Serial.println("\n╔══════════════════════════════════╗");
        Serial.println("║   SAHEL SMART SHIELD — SSS-Lite  ║");
        Serial.println("║   Démarrage du système...        ║");
        Serial.println("╚══════════════════════════════════╝");
    }

    // Initialisation relais (HIGH = OFF au démarrage — sécurité)
    pinMode(PIN_RELAY_VENTIL,  OUTPUT); digitalWrite(PIN_RELAY_VENTIL,  HIGH);
    pinMode(PIN_RELAY_DETEST,  OUTPUT); digitalWrite(PIN_RELAY_DETEST,  HIGH);
    pinMode(PIN_RELAY_BASCULE, OUTPUT); digitalWrite(PIN_RELAY_BASCULE, HIGH);

    // Initialisation capteurs
    bool capteurs_ok = sss_sensors_init();
    if (DEBUG_MODE && !capteurs_ok) {
        Serial.println("[WARN] Certains capteurs non détectés — vérifier le câblage");
    }

    // Connexion réseau
    connecter_wifi();
    connecter_mqtt();

    if (DEBUG_MODE) Serial.println("[OK] SSS-Lite prêt\n");
}


// ─────────────────────────────────────────────────────────────
//  LOOP
// ─────────────────────────────────────────────────────────────

void loop() {
    unsigned long maintenant = millis();

    // ── Maintenir la connexion MQTT ─────────────────────────
    assurer_connexion_mqtt();
    mqtt.loop();

    // ── Lecture des capteurs toutes les INTERVALLE_LECTURE ms
    if (maintenant - derniere_lecture >= INTERVALLE_LECTURE) {
        derniere_lecture = maintenant;

        // Lire tous les capteurs
        mesures_actuelles = lire_tous_les_capteurs();

        if (DEBUG_MODE) afficher_mesures(mesures_actuelles);

        // Calculer le nouvel état
        SSS_Etat nouvel_etat = sss_calculer_etat(
            etat_actuel,
            mesures_actuelles.temp_interne,
            mesures_actuelles.pct_batterie,
            mesures_actuelles.secteur_present
        );

        // Si l'état a changé
        if (nouvel_etat != etat_actuel) {
            if (DEBUG_MODE) {
                Serial.printf("[ÉTAT] %s → %s\n",
                    sss_nom_etat(etat_actuel),
                    sss_nom_etat(nouvel_etat));
            }

            etat_actuel = nouvel_etat;

            // Appliquer les relais
            appliquer_relais(etat_actuel);

            // Envoyer les alertes SMS si nécessaire
            gerer_alertes(etat_actuel, mesures_actuelles);

            // Publier immédiatement l'alerte MQTT
            mqtt.publish(TOPIC_ALERTE, sss_nom_etat(etat_actuel), true);
        }
    }

    // ── Publication MQTT toutes les INTERVALLE_MQTT ms ──────
    if (maintenant - dernier_mqtt >= INTERVALLE_MQTT) {
        dernier_mqtt = maintenant;
        publier_mqtt(mesures_actuelles, etat_actuel);
    }
}
