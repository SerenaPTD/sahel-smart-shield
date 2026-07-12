// ============================================================
//  SAHEL SMART SHIELD — SSS-Pro
//  sss_config_pro.st — Configuration centralisée
//
//  Toutes les constantes du système SSS-Pro sont définies ici.
//  Langage : Structured Text IEC 61131-3
//  Automate : Siemens LOGO! 12/24RC
//
//  Auteur  : Serena Katcheba Bianzeube
//  Version : 1.0
//  Date    : Juin 2026
// ============================================================

// ─────────────────────────────────────────────────────────────
//  ENTRÉES LOGO! 12/24RC
// ─────────────────────────────────────────────────────────────

// I1  — PT100 via transmetteur 4-20mA  (Température interne)
// I2  — SHT35 IP65 via I²C             (Température externe + Humidité)
// I3  — Détection secteur AC           (0V = absent, 24V = présent)
// I4  — Shunt 4-20mA                   (Niveau batterie 24V)
// I5  — Réservé (extension future)
// I6  — Réservé (extension future)
// I7  — Réservé (extension future)
// I8  — Réservé (extension future)

// ─────────────────────────────────────────────────────────────
//  SORTIES LOGO! 12/24RC
// ─────────────────────────────────────────────────────────────

// Q1  — Contacteur ventilation forcée
// Q2  — Contacteur délestage charges non prioritaires
// Q3  — Contacteur délestage charges secondaires
// Q4  — ATS — Bascule réseau / batterie

// ─────────────────────────────────────────────────────────────
//  SEUILS THERMIQUES (°C)
// ─────────────────────────────────────────────────────────────

VAR CONSTANT
    SEUIL_CHAUD     : REAL := 38.0;   // Activation ventilation Q1
    SEUIL_ALERTE    : REAL := 45.0;   // Alarme GSM technicien
    SEUIL_CRITIQUE  : REAL := 52.0;   // Délestage Q2
    HYSTERESIS      : REAL := 3.0;    // Écart avant retour état inférieur

// ─────────────────────────────────────────────────────────────
//  SEUILS BATTERIE (%)
// ─────────────────────────────────────────────────────────────

    SEUIL_BAT_URGENCE : REAL := 20.0; // Alarme urgence + délestage Q3
    SEUIL_BAT_FAIBLE  : REAL := 40.0; // Avertissement préventif

// ─────────────────────────────────────────────────────────────
//  CONVERSION SIGNAL 4-20mA → VALEUR PHYSIQUE
//  Signal 4mA  = valeur minimale (ex: 0°C ou 0% batterie)
//  Signal 20mA = valeur maximale (ex: 100°C ou 100% batterie)
// ─────────────────────────────────────────────────────────────

    // PT100 — plage de mesure configurée
    PT100_TEMP_MIN  : REAL := 0.0;    // °C à 4mA
    PT100_TEMP_MAX  : REAL := 100.0;  // °C à 20mA

    // Batterie 24V AGM
    BAT_TENSION_MIN : REAL := 20.0;   // V à 4mA (batterie vide)
    BAT_TENSION_MAX : REAL := 28.8;   // V à 20mA (batterie pleine)

// ─────────────────────────────────────────────────────────────
//  TEMPORISATIONS
// ─────────────────────────────────────────────────────────────

    // Délai avant activation ventilation (éviter démarrages intempestifs)
    T_VENTIL_ON     : TIME := T#5S;

    // Délai avant délestage (laisser le temps à la ventilation d'agir)
    T_DELEST_ON     : TIME := T#30S;

    // Délai anti-rebond détection secteur
    T_SECTEUR_OFF   : TIME := T#2S;

    // Délai minimum entre deux alarmes GSM (anti-spam)
    T_ALARME_MIN    : TIME := T#5M;

END_VAR
