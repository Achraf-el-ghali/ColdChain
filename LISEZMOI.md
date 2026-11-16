# ColdChain — Django REST + ESP8266 + DHT11 + alertes Gmail / Telegram

Un ESP8266 mesure la température et l'humidité (DHT11) et les envoie toutes les 10 s
à une API REST Django. Si la température dépasse **25 °C** (strictement), Django envoie
un **e-mail Gmail** et un **message Telegram**.

## 1. Structure du projet

```
ColdChain/
├── .env                  secrets (Gmail, Telegram) — NE PAS publier
├── .env.example          modèle de .env, sans valeurs
├── .gitignore            ignore .env, db.sqlite3, .venv
├── manage.py
├── requirements.txt      Django, DRF, python-dotenv, pyTelegramBotAPI
├── test_alertes.ps1      test automatique 20 / 25 / 30 °C
├── db.sqlite3            base SQLite
├── projet/
│   ├── settings.py       config : apps, fuseau, Gmail (MAILERS), Telegram, seuil, cooldown
│   └── urls.py           admin/ + include("DHT.urls")
├── DHT/
│   ├── models.py         Dht11 : temp, hum, dt
│   ├── serializers.py    DHT11serialize (objet <-> JSON)
│   ├── api.py            Dlist (GET) + Dhtviews (POST) + perform_create (alertes)
│   ├── views.py          dashboard : page du tableau de bord
│   ├── templates/dashboard.html   HTML + JavaScript (lit /api/get toutes les 5 s, bip si alerte)
│   ├── urls.py           "", api/get, api/post
│   └── admin.py          Dht11 visible dans /admin
└── arduino/ColdChain_ESP8266/ColdChain_ESP8266.ino
```

## 2. Installation et lancement (terminal PyCharm, PowerShell)

```powershell
python -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements.txt
Copy-Item .env.example .env        # puis remplir .env (voir section 6)
.\.venv\Scripts\python.exe manage.py migrate
.\.venv\Scripts\python.exe manage.py createsuperuser
.\.venv\Scripts\python.exe manage.py runserver 0.0.0.0:8000
```

`0.0.0.0` = le serveur écoute sur le réseau (nécessaire pour que l'ESP le joigne).
Après toute modification de `.env`, **redémarrer le serveur** (Ctrl+C puis relancer).

## 3. Adresses

| Usage | URL |
|---|---|
| **Tableau de bord** (température, humidité, alarme sonore) | http://IP_DU_PC:8000/ |
| Lire les mesures (GET) | http://IP_DU_PC:8000/api/get |
| Envoyer une mesure (POST) | http://IP_DU_PC:8000/api/post |
| Administration | http://127.0.0.1:8000/admin/ |

## 4. Format JSON

Envoyé par l'ESP (POST `/api/post`) :

```json
{"temp": 28.5, "hum": 55.2}
```

Réponse de Django (code **201 Created**) : la mesure enregistrée, avec `id` et `dt` ajoutés :

```json
{"id": 84, "temp": 28.5, "hum": 55.2, "dt": "2026-11-09T16:45:21.575860Z"}
```

`/api/get` renvoie `{"data": [ ...toutes les mesures... ]}`. L'heure `dt` est en UTC (le `Z`) ;
l'admin et les alertes l'affichent à l'heure de Casablanca.

## 5. Alertes (DHT/api.py → perform_create)

À chaque POST valide :

1. la mesure est enregistrée (`serializer.save()`) ;
2. si `temp > ALERT_THRESHOLD` (25 par défaut, **seuil strict**) :
   - anti-spam : au plus une alerte toutes les `ALERT_COOLDOWN_SECONDS` (60 s par défaut),
     mémorisé dans le cache Django. `ALERT_COOLDOWN_SECONDS=0` = une alerte par mesure (comportement exact du TP) ;
   - e-mail Gmail (`send_mail`) dans un `try/except` ;
   - message Telegram (`telebot`) dans un autre `try/except`.
3. Si Gmail ou Telegram échoue, l'erreur est affichée dans la console, l'autre canal part
   quand même, et l'ESP reçoit **toujours 201**.

Message envoyé :
`⚠️ Alerte Cold Chain : température = 30.0 °C (seuil 25 °C), humidité = 50.0 %, à 16:45:02`

Lignes visibles dans la console du serveur :

```
[ALERTE] Gmail envoyé
[ALERTE] Telegram envoyé
[ALERTE] ignorée (cooldown 60 s) : 28.0 °C
[ALERTE] ERREUR Gmail : ...        (en cas de problème)
```

### Tableau de bord et son

- Page http://IP_DU_PC:8000/ : dernière température, humidité, date, et « Donnée reçue il y a X secondes »
  (en rouge « capteur hors ligne ? » après 30 s sans mesure).
- Si `temp > seuil` : tuile rouge + bandeau « ALERTE » clignotant.
- Cliquer une fois sur **« Activer le son d'alarme »** (les navigateurs interdisent le son sans clic) :
  ensuite la page fait 3 bips à chaque nouvelle mesure en alerte.
- Telegram sonne comme une notification normale. Pour une vraie sonnerie : dans Telegram, ouvrir la
  conversation du bot → nom du bot → **Notifications** → **Son** → choisir un son fort (ou importer un
  fichier d'alarme). Un bot ne peut pas passer d'appel téléphonique.

## 6. Configuration du fichier .env

| Variable | Rôle |
|---|---|
| DJANGO_SECRET_KEY | clé secrète de Django (obligatoire, sinon le serveur ne démarre pas) ; en créer une : `python -c "import secrets; print(secrets.token_urlsafe(50))"` |
| EMAIL_HOST_USER | adresse Gmail qui envoie |
| EMAIL_HOST_PASSWORD | **mot de passe d'application** Google (16 lettres) |
| ALERT_EMAIL_TO | adresse qui reçoit l'alerte |
| TELEGRAM_TOKEN | token donné par @BotFather |
| TELEGRAM_CHAT_ID | identifiant de la conversation (getUpdates) |
| ALERT_THRESHOLD | seuil en °C (défaut 25) |
| ALERT_COOLDOWN_SECONDS | délai entre deux alertes (défaut 60, 0 = pas de délai) |

Remarque Django 6.1 : la configuration e-mail se fait avec `MAILERS` dans `settings.py`.
Les anciens réglages du TP (`EMAIL_BACKEND`, `EMAIL_HOST`, `EMAIL_PORT`, `EMAIL_USE_TLS`…)
sont **refusés** par Django 6.1 quand `MAILERS` existe ; on y a mis exactement les mêmes valeurs
(smtp, smtp.gmail.com, 587, TLS).

### Créer le mot de passe d'application Gmail

1. https://myaccount.google.com → **Sécurité** → activer la **Validation en deux étapes** (obligatoire).
2. https://myaccount.google.com/apppasswords → nom « ColdChain » → **Créer**.
3. Copier le code de 16 lettres dans `EMAIL_HOST_PASSWORD` (les espaces sont retirés automatiquement).
4. Le mot de passe normal du compte Gmail **ne marche pas** (erreur 535).

### Créer le bot Telegram et trouver le CHAT_ID

1. Dans Telegram, chercher **@BotFather** → `/newbot` → donner un nom, puis un identifiant finissant par `bot`.
2. BotFather renvoie le **token** (`123456789:AA...`) → le mettre dans `TELEGRAM_TOKEN`.
3. Ouvrir la conversation avec **votre** bot et lui envoyer `/start` (sinon le bot n'a pas le droit de vous écrire).
4. Dans le navigateur : `https://api.telegram.org/bot<TOKEN>/getUpdates` (coller le token juste après `bot`, sans `<>`).
5. Repérer `"chat":{"id":123456789,...}` → mettre ce nombre dans `TELEGRAM_CHAT_ID`.
   Si la réponse est `"result":[]`, renvoyer un message au bot et recharger la page.

## 7. Test des alertes (20 / 25 / 30 °C)

1. Dans `.env`, mettre `ALERT_COOLDOWN_SECONDS=0`, puis redémarrer le serveur.
2. Dans un deuxième terminal PyCharm :
   ```powershell
   & C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe -ExecutionPolicy Bypass -File .\test_alertes.ps1
   ```
3. Résultat attendu :

| Température | Code HTTP | Gmail | Telegram |
|---|---|---|---|
| 20 °C | 201 | Non | Non |
| 25 °C | 201 | Non | Non (seuil strict > 25) |
| 30 °C | 201 | Oui | Oui |

4. Remettre `ALERT_COOLDOWN_SECONDS=60` après la démo (sinon une alerte toutes les 10 s si la salle est > 25 °C).

## 8. ESP8266

1. Arduino IDE → carte « NodeMCU 1.0 (ESP-12E Module) », driver CP2102 installé.
2. Bibliothèques : « DHT sensor library » (Adafruit) + « Adafruit Unified Sensor ».
3. Branchement DHT11 : VCC → 3V3, GND → GND, DATA → D2.
4. Ouvrir `arduino/ColdChain_ESP8266/ColdChain_ESP8266.ino`, modifier WIFI_SSID, WIFI_PASSWORD
   et l'IP dans SERVER_URL (résultat de `ipconfig`, carte Wi-Fi).
5. Téléverser, Moniteur série à 115200 bauds → on doit voir « Code HTTP : 201 ».

## 9. Dépannage

| Symptôme | Cause / solution |
|---|---|
| **201** | OK : mesure enregistrée. |
| **400** Bad Request | valeur non numérique (ex. `"temp": "abc"`) ou JSON mal formé. Attention : une clé mal nommée (`temperature` au lieu de `temp`) donne 201 mais avec `temp` vide (champs `null=True`) — vérifier /api/get. |
| **404** Not Found | mauvaise URL : c'est `/api/post`, sans slash final. |
| **405** Method Not Allowed | POST envoyé sur `/api/get` (ou GET sur `/api/post` hors navigateur). |
| **500** Server Error | erreur Python : lire le message rouge dans la console du serveur. |
| **-1** (ou code négatif) sur l'ESP | serveur arrêté, mauvais IP, pare-feu Windows (port 8000 / Python), PC et ESP pas sur le même Wi-Fi 2,4 GHz. |
| L'IP a changé | `ipconfig` → « Adresse IPv4 » de la carte Wi-Fi → corriger SERVER_URL dans le .ino et téléverser. |
| `ERREUR Gmail : (535, ... Username and Password not accepted)` | utiliser un **mot de passe d'application**, pas le mot de passe normal ; validation en deux étapes activée. |
| `ERREUR Gmail : ALERT_EMAIL_TO est vide` | remplir `.env` puis redémarrer le serveur. |
| `ERREUR Telegram : ... vide dans .env` | TELEGRAM_TOKEN ou TELEGRAM_CHAT_ID vide. |
| `ERREUR Telegram : ... chat not found` | mauvais CHAT_ID, ou `/start` jamais envoyé au bot. |
| `ERREUR Telegram : ... Unauthorized` | token faux ou incomplet. |
| `[ALERTE] ignorée (cooldown)` | normal : une alerte est déjà partie il y a moins de ALERT_COOLDOWN_SECONDS. |
