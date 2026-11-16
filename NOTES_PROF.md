# Notes du professeur — prochaine version de l'application (À NE PAS ENCORE CODER)

> Explication donnée au tableau. Simple note : rien n'est implémenté pour l'instant.
> L'application actuelle (seuil > 25 °C, Gmail + Telegram) reste inchangée.

## 1. Utilisateurs (4 personnes)

| Rôle | Ce qu'il fait |
|---|---|
| **Admin** (1) | crée et surveille l'application ; crée les 3 comptes opérateurs (nom, prénom, leur bot Telegram) ; peut **modifier les seuils** (seuil normal, seuil sévère, seuil critique) |
| **Opérateur 1, 2, 3** | reçoivent les alertes ; chacun a une variable **booléenne** (op1, op2, op3) |

## 2. Règle de température (chaîne du froid)

```
si 2 <= T <= 8  -> Normal
sinon           -> pas Normal
```

## 3. Variable `incident` (booléen)

```
if (T < 2 ou T > 8)   incident = 1
else                  incident = 0
```

## 4. Fonction `compteur` (int, par défaut 0)

```
if (incident)  compteur++
else           compteur = 0
```

Le compteur compte combien de mesures **de suite** sont hors de la plage 2–8 °C.
Il revient à 0 dès qu'une mesure redevient normale.

## 5. Tableau du professeur (une mesure toutes les 20 min)

| Temps | Température (°C) | incident | compteur | op1 | op2 | op3 |
|---|---|---|---|---|---|---|
| t = 0 | 5 | 0 | 0 | 0 | 0 | 0 |
| t = 20 min | 10 | 1 | 1 | 1 | 0 | 0 |
| t = 40 min | 4 | 0 | 0 | 0 | 0 | 0 |
| t = 60 min | 9 | 1 | 1 | 1 | 0 | 0 |
| t = 80 min | … | … | … | … | … | … |
| t = 100 min | … | … | … | … | … | … |
| t = 120 min | … | … | … | … | … | … |
| t = 140 min | … | … | … | … | … | … |
| t = 160 min | … | … | … | … | … | … |

Lignes 80 à 160 min : laissées vides au tableau (à compléter en cours).

## 6. Fréquence des mesures dans le projet final

- Dans le projet final, **une mesure toutes les 20 minutes** (comme dans le tableau du prof),
  et c'est à ce rythme que le journal d'incidents est tenu.
- Aujourd'hui l'ESP envoie toutes les 10 s (ligne 28 du .ino : `INTERVALLE_MS = 10000`).
  20 min = `INTERVALLE_MS = 1200000` (à changer seulement quand on codera le projet final).

## 7. Journal d'incidents

Un **incident** commence à la première mesure hors de la plage 2–8 °C
et se termine à la première mesure qui revient dans la plage.
Pour **chaque incident**, le journal enregistre :

| Information | Exemple |
|---|---|
| Début de l'incident (date + heure) | 09/11/2026 07:00 |
| Fin de l'incident (date + heure) | 09/11/2026 08:00 |
| Durée de l'incident | 1 h 00 (= fin − début) |
| Température(s) relevée(s) | 10 °C, 9 °C… |
| **Accusé de réception** | l'opérateur confirme qu'il a vu l'alerte — **une seule fois par incident** |
| Qui a accusé réception, et quand | Opérateur 1, 09/11/2026 07:05 |
| **Commentaires des opérateurs** | voir ci-dessous |

### Accusé de réception

- Quand un incident est déclaré, l'opérateur doit **accuser réception** (ex. bouton « J'ai vu »).
- **Une seule fois par incident** : pas un accusé par mesure, pas un par alerte.

### Commentaires des opérateurs

- **Chaque opérateur** (op1, op2, op3) a le **droit d'écrire** dans le journal
  et peut ajouter un **commentaire** sur l'incident.
- Exemple de ligne de journal :
  « Journalisation aujourd'hui à 7 h 00 : température 10 °C, porte du frigo restée ouverte,
  refermée à 7 h 20. — Opérateur 1 »
  (Exemple incomplet au tableau : « la journalisation aujourd'hui à 7 … » — à compléter en cours.)

## 8. URL en ligne dans l'Arduino (ESP8266)

**FAIT dans le code (2026-11-16), comme le prof :** le `.ino` utilise maintenant
`https://achraf.pythonanywhere.com/api/add/` avec `WiFiClientSecure` + `client.setInsecure()`
(HTTPS sans vérification du certificat). **Reste à téléverser** et vérifier « Code HTTP : 201 ».
Les explications ci-dessous (version HTTP) restent valables comme solution de secours.

L'application est hébergée sur PythonAnywhere : **http://achraf.pythonanywhere.com/**
Pour que l'ESP envoie ses mesures au site en ligne (et plus au PC), il faut changer
**une seule ligne** dans le `.ino` (version téléversée :
`C:\Users\HP ULTRA\Downloads\rapport1\ColdChain_ESP8266\ColdChain_ESP8266.ino`, ligne 19) :

```cpp
// avant (serveur sur le PC, meme Wi-Fi obligatoire)
const char* SERVER_URL    = "http://10.9.11.141:8000/api/post";
// apres (serveur en ligne, n'importe quel Wi-Fi avec Internet)
const char* SERVER_URL    = "http://achraf.pythonanywhere.com/api/post";
```

Puis **téléverser** sur l'ESP et vérifier « Code HTTP : 201 » dans le Moniteur série.

Points d'attention :
- Mettre **`http://`** et pas `https://` : le code Arduino actuel (`WiFiClient`) ne gère pas le HTTPS.
  Si l'option « Force HTTPS » est activée dans l'onglet Web de PythonAnywhere, la désactiver.
- Utiliser **`/api/post`** (sans slash final). `/api/add/` marche aussi, mais **avec** le slash final :
  un POST sur `/api/add` sans slash échoue (Django ne peut pas rediriger un POST).
- Les clés JSON restent `temp` et `hum`.
- Remplir Gmail et Telegram dans le `.env` **de PythonAnywhere** (onglet Files) puis Reload,
  sinon les alertes > 25 °C ne partent pas depuis le site en ligne.
- La base en ligne est séparée de `db.sqlite3` du PC.

## 9. Points à faire préciser au professeur

- Quand op2 et op3 passent-ils à 1 ? Probablement une **escalade** selon le compteur
  (ex. compteur ≥ 1 → op1, compteur plus grand → op2, puis op3), liée aux seuils
  « sévère » et « critique » réglés par l'admin. **Hypothèse à confirmer.**
- Le compteur avance à chaque mesure : avec l'ESP qui envoie toutes les 10 s, il montera
  beaucoup plus vite que dans le tableau (une mesure toutes les 20 min).
- Seuils 2 / 8 °C en dur ou modifiables par l'admin (en base de données = nouveau modèle).
- Accusé de réception : qui doit le faire (n'importe quel opérateur, ou celui qui a reçu l'alerte) ?
  Que se passe-t-il si personne n'accuse réception (escalade vers op2 puis op3 ?)
- Un opérateur peut-il modifier ou supprimer son commentaire, ou seulement en ajouter ?
- L'admin voit-il tout le journal ? Peut-il clôturer un incident ?
