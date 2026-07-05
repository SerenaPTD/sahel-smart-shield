# SAHEL SMART SHIELD

**Système de surveillance thermique et de protection énergétique des armoires techniques critiques du Sahel**

---

Dans le Sahel, les équipements critiques tombent en panne non pas à cause d'un problème soudain, mais parce que personne ne surveille les signaux qui précèdent la panne. La température monte progressivement, le réseau électrique devient instable, la batterie se décharge silencieusement — et quand on s'en aperçoit, il est trop tard.

SAHEL SMART SHIELD est un système embarqué autonome qui surveille en permanence l'état d'une armoire technique, détecte ces signaux, déclenche des protections automatiques et alerte le technicien responsable par SMS — sans connexion internet, sans serveur distant, sans dépendance à une infrastructure numérique.

---

## Deux versions

| | SSS-Lite | SSS-Pro |
|---|---|---|
| **Contrôleur** | ESP32 DevKit v1 | Siemens LOGO! 12/24RC |
| **Langage** | C++ (PlatformIO) | Structured Text IEC 61131-3 |
| **Montage** | Breadboard / boîtier DIY | Rail DIN industriel |
| **Cible** | Dispensaires, écoles, solaire rural | Hôpitaux, télécoms, pompage |
| **Coût matériel** | ~312 € | ~750 € |
| **Statut** | Prototype en développement | Prévu pour déploiement terrain |

---

## Ce que le système fait

**5 grandeurs mesurées en continu**
- Température interne de l'armoire (SHT31)
- Température externe ambiante (DS18B20)
- Humidité relative — risque de condensation (SHT31)
- Présence ou absence du réseau secteur 230V (ZMPT101B)
- Niveau de charge de la batterie de secours (INA219)

**7 états de protection automatique**

| État | Seuil | Action |
|---|---|---|
| NORMAL | < 38 °C | Surveillance continue |
| CHAUD | 38 °C | Ventilation forcée activée |
| ALERTE | 45 °C | SMS d'alerte au technicien |
| CRITIQUE | 52 °C | Délestage des charges non prioritaires |
| COUPURE | Secteur absent | Bascule automatique sur batterie |
| URGENCE | Batterie < 20 % | SMS d'urgence + délestage supplémentaire |
| RETOUR | Secteur rétabli | Reprise normale + recharge batterie |

**Supervision locale sans internet**
- Raspberry Pi 4 + Mosquitto MQTT + Node-RED + SQLite
- Tableau de bord accessible depuis tout appareil sur le réseau local
- Historique complet des mesures et des événements

---

## Structure du projet

```
sahel-smart-shield/
├── firmware/
│   └── sss-firmware/
│       ├── src/
│       │   ├── main.cpp            # Point d'entrée principal
│       │   ├── sss_config.h        # Toutes les constantes (GPIO, seuils, MQTT...)
│       │   ├── sss_logic.h         # Machine à états — 7 niveaux de protection
│       │   ├── sss_sensors.h       # Lecture des capteurs réels
│       │   └── sss_simulation.h    # Simulation — 6 scénarios Sahel
│       └── platformio.ini
├── hardware/
│   ├── cablage/
│   │   └── SSS_Schema_Cablage.html # Schéma de câblage pin par pin
│   └── bom/
│       └── BOM_SSS_Lite.csv        # Liste des composants avec prix
├── docs/
│   ├── SSS_Documentation_Technique.docx
│   ├── SSS_Schema_Architecture.pdf
│   └── SSS_Schema_Fonctionnement.pdf
├── tests/
│   └── plan_de_tests.md            # Scénarios de validation
└── README.md
```

---

## Matériel requis (SSS-Lite)

| Composant | Référence | Quantité | Rôle |
|---|---|---|---|
| Microcontrôleur | ESP32 DevKit v1 | 1 | Cerveau du système |
| Capteur T° + Humidité | SHT31-D | 1 | Température + humidité interne |
| Sonde T° externe | DS18B20 (étanche) | 1 | Température ambiante |
| Capteur tension AC | ZMPT101B | 1 | Présence secteur 230V |
| Capteur batterie | INA219 | 1 | Niveau de charge |
| Module GSM | SIM800L | 1 | Alertes SMS |
| Module relais | 5V 3 canaux | 1 | Ventilation, délestage, bascule |
| Supervision | Raspberry Pi 4 (2 Go) | 1 | MQTT, Node-RED, SQLite |
| Résistance pull-up | 4.7 kΩ | 3 | DS18B20 + I²C |
| Condensateur | 1000 µF / 10V | 1 | Filtrage alimentation SIM800L |
| Batterie secours | 12V 7Ah VRLA | 1 | Continuité énergétique |
| Alimentation SIM800L | Buck 4.1V / 2A min | 1 | Source dédiée obligatoire |

---

## Câblage GPIO (résumé)

| GPIO | Rôle | Composant |
|---|---|---|
| GPIO 4 | 1-Wire DATA | DS18B20 |
| GPIO 21 (SDA) | I²C Data | SHT31 + INA219 |
| GPIO 22 (SCL) | I²C Clock | SHT31 + INA219 |
| GPIO 34 | ADC entrée | ZMPT101B |
| GPIO 16 (RX) | UART2 | SIM800L TX |
| GPIO 17 (TX) | UART2 | SIM800L RX |
| GPIO 25 | Relais 1 | Ventilation |
| GPIO 26 | Relais 2 | Délestage |
| GPIO 27 | Relais 3 | Bascule batterie |

> Le SIM800L requiert une alimentation dédiée capable de fournir 2A en pic. Ne pas alimenter depuis la broche 3.3V de l'ESP32.

---

## Installation et configuration

### Prérequis logiciels

- [Visual Studio Code](https://code.visualstudio.com/)
- Extension [PlatformIO IDE](https://platformio.org/)
- Bibliothèques (déclarées dans `platformio.ini`) :
  - `adafruit/Adafruit SHT31 Library`
  - `paulstoffregen/OneWire`
  - `milesburton/DallasTemperature`
  - `wollewald/INA219_WE`
  - `knolleary/PubSubClient` (MQTT)

### Cloner le projet

```bash
git clone https://github.com/SerenaPTD/sahel-smart-shield.git
cd sahel-smart-shield/firmware/sss-firmware
```

### Configurer

Ouvrir `src/sss_config.h` et renseigner :

```cpp
#define WIFI_SSID         "nom_de_ton_reseau"
#define WIFI_PASSWORD     "mot_de_passe"
#define MQTT_BROKER_IP    "192.168.x.x"     // IP du Raspberry Pi
#define TEL_TECHNICIEN    "+23566XXXXXXX"   // Numéro d'alerte SMS
```

### Compiler et flasher

Dans VS Code avec PlatformIO :
1. Brancher l'ESP32 en USB
2. Cliquer sur ✓ (Build) puis → (Upload) dans la barre PlatformIO
3. Ouvrir le moniteur série (115200 bauds) pour vérifier le démarrage

---

## Démarrage du mode simulation

Pour tester la logique sans matériel, activer la simulation dans `main.cpp` :

```cpp
#define MODE_SIMULATION 1   // 1 = simulation, 0 = capteurs réels
```

Six scénarios Sahel sont disponibles dans `sss_simulation.h` :
- Canicule progressive (38°C → 55°C en 30 minutes)
- Coupure secteur soudaine
- Coupure secteur + batterie faible
- Nuit froide avec condensation
- Délestage et retour secteur
- Scénario combiné journée type Sahel

---

## Feuille de route

- [x] Phase 00 — Cadrage et cahier des charges
- [x] Phase 01 — Architecture technique et documentation
- [x] Phase 02 — Prototypage firmware ESP32
- [ ] Phase 03 — Tests et validation (en cours)
- [ ] Phase 04 — Déploiement pilote SSS-Pro au Tchad
- [ ] Phase 05 — Évaluation et mesure d'impact
- [ ] Phase 06 — Extension multi-sites

---

## Licence

MIT — libre d'utilisation, de modification et de redistribution.

---

*SAHEL SMART SHIELD — github.com/SerenaPTD/sahel-smart-shield*
