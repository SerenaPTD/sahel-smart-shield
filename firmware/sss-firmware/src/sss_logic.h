// ============================================================
//  SAHEL SMART SHIELD — SSS-Lite
//  sss_logic.h — Machine à états à 7 niveaux de protection
//
//  Cette machine à états décide de l'état du système
//  en fonction des mesures des capteurs. Elle intègre
//  une logique d'hystérésis pour éviter les oscillations.
// ============================================================

#pragma once

#include <Arduino.h>
#include "sss_config.h"


// ─────────────────────────────────────────────────────────────
//  ÉNUMÉRATION DES ÉTATS
// ─────────────────────────────────────────────────────────────

enum SSS_Etat {
    ETAT_NORMAL,    // < 38°C  — surveillance continue
    ETAT_CHAUD,     // ≥ 38°C  — ventilation forcée
    ETAT_ALERTE,    // ≥ 45°C  — SMS technicien
    ETAT_CRITIQUE,  // ≥ 52°C  — délestage charges
    ETAT_COUPURE,   // secteur absent — bascule batterie
    ETAT_URGENCE,   // batterie < 20% — SMS urgence
    ETAT_RETOUR     // secteur rétabli — retour normal
};


// ─────────────────────────────────────────────────────────────
//  NOM DE L'ÉTAT — pour affichage et MQTT
// ─────────────────────────────────────────────────────────────

const char* sss_nom_etat(SSS_Etat etat) {
    switch (etat) {
        case ETAT_NORMAL:   return "NORMAL";
        case ETAT_CHAUD:    return "CHAUD";
        case ETAT_ALERTE:   return "ALERTE";
        case ETAT_CRITIQUE: return "CRITIQUE";
        case ETAT_COUPURE:  return "COUPURE";
        case ETAT_URGENCE:  return "URGENCE";
        case ETAT_RETOUR:   return "RETOUR";
        default:            return "INCONNU";
    }
}


// ─────────────────────────────────────────────────────────────
//  MACHINE À ÉTATS — Calcul du nouvel état
//
//  Paramètres :
//    etat_actuel      — état courant du système
//    temp             — température interne en °C (SHT31)
//    pct_batterie     — niveau batterie en % (INA219)
//    secteur_present  — true si le réseau 230V est détecté
//
//  Retourne le nouvel état calculé.
// ─────────────────────────────────────────────────────────────

SSS_Etat sss_calculer_etat(
    SSS_Etat etat_actuel,
    float    temp,
    float    pct_batterie,
    bool     secteur_present
) {
    // ── Priorité 1 : Gestion énergétique ───────────────────
    // La coupure secteur est prioritaire sur tout le reste

    if (!secteur_present) {
        // Secteur absent — on vérifie l'état de la batterie
        if (pct_batterie <= SEUIL_BAT_URGENCE) {
            return ETAT_URGENCE;
        }
        return ETAT_COUPURE;
    }

    // Secteur vient de revenir (on était en coupure ou urgence)
    if (etat_actuel == ETAT_COUPURE || etat_actuel == ETAT_URGENCE) {
        return ETAT_RETOUR;
    }

    // Après RETOUR, on repasse à NORMAL au cycle suivant
    if (etat_actuel == ETAT_RETOUR) {
        return ETAT_NORMAL;
    }

    // ── Priorité 2 : Gestion thermique ─────────────────────
    // Seuils avec hystérésis pour éviter les oscillations

    // Montée vers CRITIQUE
    if (temp >= SEUIL_CRITIQUE) {
        return ETAT_CRITIQUE;
    }

    // Descente depuis CRITIQUE avec hystérésis
    if (etat_actuel == ETAT_CRITIQUE) {
        if (temp < SEUIL_CRITIQUE - HYSTERESIS) {
            return ETAT_ALERTE;
        }
        return ETAT_CRITIQUE;
    }

    // Montée vers ALERTE
    if (temp >= SEUIL_ALERTE) {
        return ETAT_ALERTE;
    }

    // Descente depuis ALERTE avec hystérésis
    if (etat_actuel == ETAT_ALERTE) {
        if (temp < SEUIL_ALERTE - HYSTERESIS) {
            return ETAT_CHAUD;
        }
        return ETAT_ALERTE;
    }

    // Montée vers CHAUD
    if (temp >= SEUIL_CHAUD) {
        return ETAT_CHAUD;
    }

    // Descente depuis CHAUD avec hystérésis
    if (etat_actuel == ETAT_CHAUD) {
        if (temp < SEUIL_CHAUD - HYSTERESIS) {
            return ETAT_NORMAL;
        }
        return ETAT_CHAUD;
    }

    // ── État par défaut ─────────────────────────────────────
    return ETAT_NORMAL;
}
