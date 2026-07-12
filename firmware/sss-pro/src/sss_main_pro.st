// ============================================================
//  SAHEL SMART SHIELD — SSS-Pro
//  sss_main_pro.st — Programme principal
//
//  Machine à états à 7 niveaux de protection
//  Langage : Structured Text IEC 61131-3
//  Automate : Siemens LOGO! 12/24RC
//
//  Auteur  : Serena Katcheba Bianzeube
//  Version : 1.0
//  Date    : Juin 2026
// ============================================================

PROGRAM SSS_PRO

// ─────────────────────────────────────────────────────────────
//  VARIABLES — Entrées physiques
// ─────────────────────────────────────────────────────────────

VAR_INPUT
    I1_TEMP_INTERNE  : REAL;    // °C  — PT100 via transmetteur 4-20mA
    I2_TEMP_EXTERNE  : REAL;    // °C  — SHT35 IP65
    I3_HUMIDITE      : REAL;    // %   — SHT35 IP65
    I4_SECTEUR       : BOOL;    // —   — TRUE = secteur 230V présent
    I5_BAT_PCT       : REAL;    // %   — Niveau batterie 24V via shunt
END_VAR

// ─────────────────────────────────────────────────────────────
//  VARIABLES — Sorties physiques
// ─────────────────────────────────────────────────────────────

VAR_OUTPUT
    Q1_VENTILATION   : BOOL;    // Contacteur ventilation forcée
    Q2_DELESTAGE     : BOOL;    // Contacteur délestage charges
    Q3_DELESTAGE2    : BOOL;    // Contacteur délestage secondaire
    Q4_BASCULE       : BOOL;    // ATS bascule réseau / batterie
END_VAR

// ─────────────────────────────────────────────────────────────
//  VARIABLES — Internes
// ─────────────────────────────────────────────────────────────

VAR
    // État courant du système
    ETAT            : INT := 0;
    // 0 = NORMAL
    // 1 = CHAUD
    // 2 = ALERTE
    // 3 = CRITIQUE
    // 4 = COUPURE
    // 5 = URGENCE
    // 6 = RETOUR

    ETAT_PRECEDENT  : INT := 0;

    // Temporisateurs
    TON_VENTIL      : TON;      // Retard activation ventilation
    TON_DELEST      : TON;      // Retard activation délestage
    TON_SECTEUR     : TON;      // Anti-rebond détection secteur
    TON_ALARME      : TON;      // Anti-spam alarme GSM

    // Flags internes
    ALARME_ENVOYEE  : BOOL := FALSE;
    SECTEUR_STABLE  : BOOL := TRUE;

    // Variables de calcul
    TEMP            : REAL;
    BAT             : REAL;
END_VAR

// ─────────────────────────────────────────────────────────────
//  CORPS DU PROGRAMME
// ─────────────────────────────────────────────────────────────

// Lecture des valeurs courantes
TEMP := I1_TEMP_INTERNE;
BAT  := I5_BAT_PCT;

// ── Anti-rebond détection secteur ──────────────────────────
// On attend 2 secondes avant de confirmer une coupure
TON_SECTEUR(IN := NOT I4_SECTEUR, PT := T_SECTEUR_OFF);
IF TON_SECTEUR.Q THEN
    SECTEUR_STABLE := FALSE;
ELSE
    IF I4_SECTEUR THEN
        SECTEUR_STABLE := TRUE;
    END_IF;
END_IF;

// ── Sauvegarde de l'état précédent ─────────────────────────
ETAT_PRECEDENT := ETAT;

// ── MACHINE À ÉTATS ────────────────────────────────────────

// Priorité 1 : Gestion énergétique
IF NOT SECTEUR_STABLE THEN
    IF BAT <= SEUIL_BAT_URGENCE THEN
        ETAT := 5;  // URGENCE
    ELSE
        ETAT := 4;  // COUPURE
    END_IF;

// Retour secteur (on était en coupure ou urgence)
ELSIF (ETAT_PRECEDENT = 4 OR ETAT_PRECEDENT = 5) AND SECTEUR_STABLE THEN
    ETAT := 6;  // RETOUR

// Après RETOUR, repasser à NORMAL
ELSIF ETAT_PRECEDENT = 6 THEN
    ETAT := 0;  // NORMAL

// Priorité 2 : Gestion thermique avec hystérésis
ELSIF TEMP >= SEUIL_CRITIQUE THEN
    ETAT := 3;  // CRITIQUE

ELSIF ETAT_PRECEDENT = 3 AND TEMP < (SEUIL_CRITIQUE - HYSTERESIS) THEN
    ETAT := 2;  // Descente CRITIQUE → ALERTE

ELSIF TEMP >= SEUIL_ALERTE THEN
    ETAT := 2;  // ALERTE

ELSIF ETAT_PRECEDENT = 2 AND TEMP < (SEUIL_ALERTE - HYSTERESIS) THEN
    ETAT := 1;  // Descente ALERTE → CHAUD

ELSIF TEMP >= SEUIL_CHAUD THEN
    ETAT := 1;  // CHAUD

ELSIF ETAT_PRECEDENT = 1 AND TEMP < (SEUIL_CHAUD - HYSTERESIS) THEN
    ETAT := 0;  // Descente CHAUD → NORMAL

ELSE
    ETAT := 0;  // NORMAL par défaut
END_IF;

// ── COMMANDE DES SORTIES selon l'état ──────────────────────

CASE ETAT OF

    0:  // NORMAL
        TON_VENTIL(IN := FALSE, PT := T_VENTIL_ON);
        Q1_VENTILATION := FALSE;
        Q2_DELESTAGE   := FALSE;
        Q3_DELESTAGE2  := FALSE;
        Q4_BASCULE     := FALSE;  // Réseau secteur

    1:  // CHAUD — Ventilation avec temporisation
        TON_VENTIL(IN := TRUE, PT := T_VENTIL_ON);
        Q1_VENTILATION := TON_VENTIL.Q;
        Q2_DELESTAGE   := FALSE;
        Q3_DELESTAGE2  := FALSE;
        Q4_BASCULE     := FALSE;

    2:  // ALERTE — Ventilation + alarme GSM
        TON_VENTIL(IN := TRUE, PT := T_VENTIL_ON);
        Q1_VENTILATION := TON_VENTIL.Q;
        Q2_DELESTAGE   := FALSE;
        Q3_DELESTAGE2  := FALSE;
        Q4_BASCULE     := FALSE;

    3:  // CRITIQUE — Ventilation + délestage avec temporisation
        TON_VENTIL(IN := TRUE, PT := T_VENTIL_ON);
        TON_DELEST(IN := TRUE, PT := T_DELEST_ON);
        Q1_VENTILATION := TON_VENTIL.Q;
        Q2_DELESTAGE   := TON_DELEST.Q;
        Q3_DELESTAGE2  := FALSE;
        Q4_BASCULE     := FALSE;

    4:  // COUPURE — Bascule batterie + délestage
        TON_VENTIL(IN := FALSE, PT := T_VENTIL_ON);
        Q1_VENTILATION := FALSE;   // Économie énergie batterie
        Q2_DELESTAGE   := TRUE;    // Délestage immédiat
        Q3_DELESTAGE2  := FALSE;
        Q4_BASCULE     := TRUE;    // Bascule sur batterie

    5:  // URGENCE — Bascule batterie + délestage maximum
        TON_VENTIL(IN := FALSE, PT := T_VENTIL_ON);
        Q1_VENTILATION := FALSE;
        Q2_DELESTAGE   := TRUE;
        Q3_DELESTAGE2  := TRUE;    // Délestage secondaire aussi
        Q4_BASCULE     := TRUE;

    6:  // RETOUR secteur
        TON_VENTIL(IN := FALSE, PT := T_VENTIL_ON);
        TON_DELEST(IN := FALSE, PT := T_DELEST_ON);
        Q1_VENTILATION := FALSE;
        Q2_DELESTAGE   := FALSE;
        Q3_DELESTAGE2  := FALSE;
        Q4_BASCULE     := FALSE;   // Retour réseau

END_CASE;

// ── GESTION ALARME GSM ─────────────────────────────────────
// Le module GSM est commandé via la sortie analogique du LOGO!
// ou via le module CMR2040 (Modbus)
// L'alarme est envoyée uniquement lors d'un changement d'état
// et avec un délai minimum entre deux envois

TON_ALARME(IN := ALARME_ENVOYEE, PT := T_ALARME_MIN);

IF ETAT <> ETAT_PRECEDENT THEN
    IF NOT TON_ALARME.Q THEN
        // Nouvel état — déclencher l'alarme GSM
        // La commande effective dépend du module GSM connecté
        // Sur LOGO! CMR2040 : écrire dans un registre Modbus
        // Sur module UART : envoyer commande AT via bloc COM
        ALARME_ENVOYEE := TRUE;
        TON_ALARME(IN := FALSE, PT := T_ALARME_MIN); // Reset timer
        TON_ALARME(IN := TRUE,  PT := T_ALARME_MIN); // Redémarrage
    END_IF;
ELSE
    IF TON_ALARME.Q THEN
        ALARME_ENVOYEE := FALSE; // Réarmer après délai
    END_IF;
END_IF;

END_PROGRAM
