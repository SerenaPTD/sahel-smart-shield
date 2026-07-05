# SAHEL SMART SHIELD — SSS-Lite
# Plan de tests formel v1.0

---

## Objectif

Vérifier que chaque comportement du système correspond exactement à ce qui est défini dans le cahier des charges, avant tout déploiement terrain. Chaque test est indépendant et reproductible.

---

## Environnement de test

- Carte : ESP32 DevKit v1
- Firmware : SSS-Lite v1.0
- Mode : capteurs réels (MODE_SIMULATION = 0)
- Moniteur série : 115200 bauds
- Téléphone de test : numéro renseigné dans sss_config.h
- Réseau local : Raspberry Pi actif avec Mosquitto + Node-RED

---

## Statuts possibles

| Symbole | Signification |
|---|---|
| ✅ | Test passé |
| ❌ | Test échoué |
| ⏳ | Non encore exécuté |
| ⚠️ | Passé avec réserve |

---

## MODULE 1 — Démarrage et initialisation

| ID | Description | Comment provoquer | Résultat attendu | Résultat obtenu | Statut |
|---|---|---|---|---|---|
| T-01 | Démarrage normal | Brancher l'ESP32 | Tous les capteurs initialisés, connexion WiFi établie, message "SSS-Lite prêt" sur moniteur série | | ⏳ |
| T-02 | Connexion WiFi | Démarrage avec SSID correct | Connexion en moins de 15 secondes, IP affichée sur moniteur | | ⏳ |
| T-03 | Connexion MQTT | Raspberry Pi actif sur le réseau | Connexion au broker en moins de 5 secondes après WiFi | | ⏳ |
| T-04 | Initialisation relais | Démarrage du système | Les 3 relais sont ouverts (OFF) au boot, aucun actionneur ne s'active | | ⏳ |
| T-05 | Démarrage sans WiFi | Retirer le réseau avant démarrage | Le système démarre quand même, surveille et peut envoyer des SMS, pas de blocage | | ⏳ |

---

## MODULE 2 — Lecture des capteurs

| ID | Description | Comment provoquer | Résultat attendu | Résultat obtenu | Statut |
|---|---|---|---|---|---|
| T-06 | Lecture SHT31 | Démarrage normal | Température et humidité affichées toutes les 5 secondes | | ⏳ |
| T-07 | Précision SHT31 | Comparer avec thermomètre de référence | Écart inférieur à 1 °C | | ⏳ |
| T-08 | Lecture DS18B20 | Démarrage normal | Température externe affichée, distincte de la température interne | | ⏳ |
| T-09 | Lecture INA219 | Brancher la batterie 12V | Tension et pourcentage de charge affichés | | ⏳ |
| T-10 | Détection secteur présent | Réseau secteur branché | État "SECTEUR OK" sur moniteur | | ⏳ |
| T-11 | Détection secteur absent | Débrancher le secteur | Absence détectée en moins de 1 seconde | | ⏳ |
| T-12 | Publication MQTT | Capteurs actifs, broker en ligne | Les 5 topics publient toutes les 10 secondes, visibles dans Node-RED | | ⏳ |

---

## MODULE 3 — Machine à états et seuils thermiques

| ID | Description | Comment provoquer | Résultat attendu | Résultat obtenu | Statut |
|---|---|---|---|---|---|
| T-13 | État NORMAL | Température < 38 °C | Aucun relais actif, état "NORMAL" publié en MQTT | | ⏳ |
| T-14 | Passage à CHAUD | Chauffer SHT31 à 38 °C (haleine ou sèche-cheveux à distance) | Relais ventilation (GPIO25) se ferme, état "CHAUD" publié | | ⏳ |
| T-15 | Hystérésis ventilation | Chauffer à 38 °C puis laisser refroidir | Ventilation ne s'éteint qu'en dessous de 35 °C | | ⏳ |
| T-16 | Passage à ALERTE | Chauffer à 45 °C | SMS envoyé, état "ALERTE" publié | | ⏳ |
| T-17 | Anti-spam SMS | Maintenir 45 °C 10 minutes | Un seul SMS, pas de répétition avant le délai configuré | | ⏳ |
| T-18 | Passage à CRITIQUE | Chauffer à 52 °C | Relais délestage (GPIO26) se ferme, état "CRITIQUE" publié | | ⏳ |
| T-19 | Retour à NORMAL | Laisser refroidir après T-18 | Relais s'ouvrent progressivement selon hystérésis | | ⏳ |

---

## MODULE 4 — Coupure et bascule énergétique

| ID | Description | Comment provoquer | Résultat attendu | Résultat obtenu | Statut |
|---|---|---|---|---|---|
| T-20 | Coupure secteur | Débrancher le secteur pendant que le système tourne | Relais bascule (GPIO27) se ferme en moins de 2 secondes, SMS envoyé | | ⏳ |
| T-21 | Continuité sur batterie | Suite de T-20 | Système continue de fonctionner, toutes les mesures continuent | | ⏳ |
| T-22 | Retour secteur | Rebrancher le secteur après T-20 | Relais bascule s'ouvre, SMS "Secteur rétabli" | | ⏳ |
| T-23 | Urgence batterie | Simuler batterie à 20 % | SMS d'urgence envoyé, délestage supplémentaire activé | | ⏳ |

---

## MODULE 5 — Alertes SMS

| ID | Description | Comment provoquer | Résultat attendu | Résultat obtenu | Statut |
|---|---|---|---|---|---|
| T-24 | SMS à 45 °C | Suite de T-16 | SMS reçu avec température et état lisibles | | ⏳ |
| T-25 | SMS coupure secteur | Suite de T-20 | SMS reçu en moins de 30 secondes | | ⏳ |
| T-26 | SMS urgence batterie | Suite de T-23 | SMS reçu avec niveau de batterie mentionné | | ⏳ |
| T-27 | SIM800L sans SIM | Retirer la carte SIM | Le système ne plante pas, erreur loggée, protection locale continue | | ⏳ |

---

## MODULE 6 — Supervision locale

| ID | Description | Comment provoquer | Résultat attendu | Résultat obtenu | Statut |
|---|---|---|---|---|---|
| T-28 | Tableau de bord | Ouvrir navigateur sur IP Raspberry Pi port 1880 | Toutes les valeurs affichées en temps réel | | ⏳ |
| T-29 | Historique SQLite | Laisser tourner 30 minutes | Mesures enregistrées et consultables dans Node-RED | | ⏳ |
| T-30 | Supervision sans internet | Couper internet sur le Raspberry Pi | Tableau de bord reste accessible sur réseau local | | ⏳ |

---

## MODULE 7 — Robustesse

| ID | Description | Comment provoquer | Résultat attendu | Résultat obtenu | Statut |
|---|---|---|---|---|---|
| T-31 | Redémarrage | Appuyer sur RST de l'ESP32 | Système redémarre proprement, relais repassent à OFF | | ⏳ |
| T-32 | Perte MQTT | Éteindre le Raspberry Pi | Système ne plante pas, reconnexion automatique au retour | | ⏳ |
| T-33 | Capteur déconnecté | Débrancher SHT31 en cours de fonctionnement | Erreur sur moniteur série, système continue avec valeurs disponibles | | ⏳ |

---

## Résumé de validation

| Module | Tests | Passés | Échoués | En attente |
|---|---|---|---|---|
| 1. Initialisation | 5 | 0 | 0 | 5 |
| 2. Capteurs | 7 | 0 | 0 | 7 |
| 3. Seuils thermiques | 7 | 0 | 0 | 7 |
| 4. Énergie | 4 | 0 | 0 | 4 |
| 5. SMS | 4 | 0 | 0 | 4 |
| 6. Supervision | 3 | 0 | 0 | 3 |
| 7. Robustesse | 3 | 0 | 0 | 3 |
| **Total** | **33** | **0** | **0** | **33** |

---

*SAHEL SMART SHIELD — SSS-Lite — Plan de tests v1.0*
