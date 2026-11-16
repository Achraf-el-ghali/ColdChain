# Démonstration Cold Chain — 5 minutes

**Avant le professeur** : serveur lancé (`.\.venv\Scripts\python.exe manage.py runserver 0.0.0.0:8000`),
ESP branché, `.env` rempli, `ALERT_COOLDOWN_SECONDS=0` (serveur redémarré), Gmail ouvert,
Telegram ouvert sur le bot, navigateur ouvert. Préparer un deuxième terminal PyCharm.

---

## 1. Le code Django — PyCharm (1 min 30)

| Je montre | Je dis |
|---|---|
| `DHT/models.py` | « Le modèle `Dht11` a trois champs : `temp`, `hum`, et `dt`, la date ajoutée automatiquement par Django. C'est la table de la base SQLite. » |
| `DHT/serializers.py` | « Le serializer transforme un objet Python en JSON et inversement, et il vérifie que les données reçues sont valides. » |
| `DHT/api.py` → `Dlist` | « `Dlist` répond aux GET : il renvoie toutes les mesures en JSON. » |
| `DHT/api.py` → `Dhtviews` + `perform_create` | « `Dhtviews` reçoit les POST de l'ESP. J'ai ajouté `perform_create` : il enregistre la mesure, puis si `temp` est **strictement** supérieure à 25 °C, il envoie un e-mail avec `send_mail` et un message Telegram avec `telebot`. Chaque envoi est dans son propre `try/except` : si Gmail tombe, Telegram part quand même, et l'ESP reçoit toujours 201. Il y a aussi un anti-spam : une alerte par minute au maximum, grâce au cache Django. » |
| `DHT/urls.py` | « Deux routes : `api/get` et `api/post`. » |
| `.env.example` + `settings.py` | « Les mots de passe ne sont pas dans le code : ils sont dans `.env`, lu par python-dotenv, et `.env` est dans le `.gitignore`. » |

## 2. L'ESP8266 — Moniteur série (45 s)

Je montre : le Moniteur série à 115200 bauds.
Je dis : « Toutes les 10 secondes, l'ESP lit le DHT11, construit le JSON `{"temp": ..., "hum": ...}`
et l'envoie en POST. Django répond **201 Created** : la mesure est enregistrée. »

## 3. Le navigateur — /api/get (30 s)

Je montre : `http://IP_DU_PC:8000/api/get`.
Je dis : « Voici toutes les mesures en JSON. La dernière vient d'arriver de l'ESP. »

Puis j'ouvre le **tableau de bord** `http://IP_DU_PC:8000/` et je clique sur « Activer le son d'alarme ».
Je dis : « Cette page HTML interroge la même API `/api/get` toutes les 5 secondes en JavaScript. Elle affiche
la dernière mesure et depuis combien de secondes elle est arrivée. Au-dessus de 25 °C, elle passe en rouge et sonne. »
(Pendant le test 30 °C de l'étape 5, laisser cette page visible : elle devient rouge et bipe.)

## 4. L'administration — /admin (30 s)

Je montre : `http://127.0.0.1:8000/admin/` → Dht11s.
Je dis : « Les mêmes données dans l'interface d'administration de Django, à l'heure de Casablanca. »

## 5. Test des alertes 20 / 25 / 30 °C (1 min)

Je lance dans le deuxième terminal :

```powershell
& C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe -ExecutionPolicy Bypass -File .\test_alertes.ps1
```

Je dis : « J'envoie 20, 25 puis 30 °C. Les trois reçoivent 201. Dans la console du serveur,
seule la mesure à 30 °C affiche `[ALERTE] Gmail envoyé` et `[ALERTE] Telegram envoyé`.
25 ne déclenche rien car le seuil est strict. »

| Température | Gmail | Telegram |
|---|---|---|
| 20 °C | Non | Non |
| 25 °C | Non | Non |
| 30 °C | Oui | Oui |

## 6. Les notifications reçues (30 s)

Je montre : l'e-mail dans Gmail et le message Telegram (ou leurs captures d'écran).
Je dis : « Message reçu : *⚠️ Alerte Cold Chain : température = 30.0 °C (seuil 25 °C), humidité = 50.0 %, à 16:45:02*. »

## 7. Test d'erreur volontaire — 404 (15 s)

Je montre : `http://IP_DU_PC:8000/api/postt` dans le navigateur.
Je dis : « Une mauvaise URL donne 404 Not Found : Django ne connaît que `api/get` et `api/post`. »

---

## Le rôle de l'API REST (à dire en conclusion)

L'API REST est le point de rencontre entre le capteur et le serveur. L'ESP8266 ne connaît rien
de Django ni de la base de données : il envoie simplement du JSON en HTTP (POST), et n'importe quel
client (navigateur, application mobile, tableau de bord) peut relire les données en JSON (GET).
Les codes HTTP (201, 400, 404, 405…) disent au client si sa demande a réussi. C'est aussi au moment
de la réception (POST) qu'on peut ajouter de la logique métier, comme ici les alertes de température.

## Questions probables du professeur

**Pourquoi `perform_create` et pas une nouvelle vue ?**
C'est la méthode que `CreateAPIView` appelle pour enregistrer l'objet. En la redéfinissant, on garde
tout le reste (validation, réponse 201) et on ajoute seulement l'alerte après `serializer.save()`.

**Pourquoi `mesure.temp` et pas `data.temperature` comme dans le TP ?**
Parce que notre champ s'appelle `temp` dans le modèle et dans le JSON de l'ESP.

**Pourquoi 25 °C ne déclenche pas d'alerte ?**
La condition est `temp > 25` (strictement supérieur), comme demandé dans le tableau.

**Que se passe-t-il si Gmail ne marche pas ?**
L'erreur est écrite dans la console, Telegram part quand même, la mesure est enregistrée et l'ESP reçoit 201.

**Pourquoi un cooldown ?**
L'ESP envoie toutes les 10 s : si la salle est à 28 °C, on recevrait 360 e-mails par heure.
On limite à une alerte par minute ; `ALERT_COOLDOWN_SECONDS=0` redonne le comportement du TP.

**Où sont les mots de passe ?**
Dans `.env`, lu par python-dotenv, et exclu de Git par `.gitignore`. `.env.example` montre les variables sans valeurs.

**Pourquoi un mot de passe d'application Gmail ?**
Google refuse le mot de passe normal pour SMTP. Il faut la validation en deux étapes et un mot de passe
d'application dédié, qu'on peut révoquer sans changer le mot de passe du compte.

**Pourquoi `MAILERS` au lieu de `EMAIL_HOST` dans settings.py ?**
Django 6.1 a remplacé les réglages `EMAIL_*` par `MAILERS` et refuse les deux en même temps.
Les valeurs sont les mêmes : SMTP, smtp.gmail.com, port 587, TLS.

**Comment trouver le CHAT_ID Telegram ?**
Envoyer `/start` au bot, puis ouvrir `https://api.telegram.org/bot<TOKEN>/getUpdates` et lire `chat.id`.

**Que signifie `0.0.0.0:8000` ?**
Le serveur écoute sur toutes les cartes réseau, donc l'ESP peut le joindre par l'IP Wi-Fi du PC.

**Pourquoi l'heure dans /api/get finit par `Z` ?**
Django stocke en UTC (`USE_TZ = True`) ; l'admin et le message d'alerte convertissent en heure de Casablanca.
