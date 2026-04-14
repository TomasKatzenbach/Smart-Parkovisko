# Smart Parkovisko

Webová aplikácia pre monitorovanie obsadenosti parkoviska v reálnom čase. Súčasť bakalárskej práce.
Foto a Videodokumentácia sa nachádza v priečinku Foto-Videodokumentácia.

## Technológie

- **Frontend:** Vue 3
- **Build:** Vite
- **Backend / DB:** Firebase Realtime Database
- **Autentifikácia:** Firebase Authentication
- **Hardware:** Arduino Uno Wifi s ultrazvukovými senzormi(HC-SR04)

## Funkcie

- Zobrazenie stavu parkovacích miest v reálnom čase (mapa + štatistiky)
- Prihlásenie a správa účtu (meno, priezvisko, reset hesla)
- Admin dashboard — správa používateľov, monitoring senzorov, ESP stav, AP mód
- Svetlý / tmavý režim
- Responzívny dizajn

## Inštalácia

```bash
npm install
```

Vytvor `.env` súbor podľa `.env.example` a vyplň Firebase konfiguráciu.

## Spustenie (development)

```bash
npm run dev
```

## Build (production)

```bash
npm run build
```

## Štruktúra projektu

```
Smart-Parkovisko/
├── Arduino - ESP/
│   ├── Arduino-Smart-Parkovisko-Arduino.ino   # Arduino kód (senzory)
│   └── Arduino_Smart_Parkovisko-ESP.ino       # ESP kód (WiFi + Firebase)
├── public/
│   ├── logo.png
│   └── parking-lot.png
├── src/
│   ├── App.vue              # Hlavný komponent s navigáciou
│   ├── main.js              # Vstupný bod aplikácie
│   ├── router.js            # Vue Router (SPA routovanie)
│   ├── auth.js              # Autentifikácia a správa používateľov
│   ├── firebase.js          # Firebase konfigurácia
│   ├── style.css            # Globálne štýly
│   └── components/
│       ├── ParkingData.vue      # Hlavná stránka — mapa a štatistiky
│       ├── AuthPage.vue         # Prihlásenie
│       ├── AccountPage.vue      # Správa účtu
│       ├── AdminDashboard.vue   # Admin panel
│       └── ESPStatusPanel.vue   # Monitoring ESP zariadenia
├── Foto-VideoDokumentácia/
│   ├── admin_dashboard_1_dark.png
│   ├── admin_dashboard_1.png
│   ├── admin_dashboard_2_dark.png
│   ├── admin_dashboard_2.png
│   ├── login_page_dark.png
│   ├── login_page.png
│   ├── main_page_1_dark.png
│   ├── main_page_1.png
│   ├── main_page_2_dark.png
│   ├── main_page_2.png
│   ├── profile_page_dark.png
│   ├── profile_page.png
│   ├── Videodokumentácia_Katzenbach.mp4
├── firebase.json
├── index.html
├── package.json
└── vite.config.js
```
