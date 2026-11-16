# Test des alertes Cold Chain : envoie 20, 25 puis 30 degres sur l'API.
# Avant de lancer :
#   1) mettre ALERT_COOLDOWN_SECONDS=0 dans .env (sinon une seule alerte par minute)
#   2) (re)demarrer le serveur : .\.venv\Scripts\python.exe manage.py runserver 0.0.0.0:8000
# Lancer : & C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe -ExecutionPolicy Bypass -File .\test_alertes.ps1
# Resultat attendu dans la console du serveur :
#   20 et 25 -> aucune ligne [ALERTE] ; 30 -> "[ALERTE] Gmail envoyé" et "[ALERTE] Telegram envoyé"

$url = "http://127.0.0.1:8000/api/post"

# Lire le cooldown dans .env pour prevenir si ce n'est pas 0
$cooldown = $null
if (Test-Path ".env") {
    $ligne = Select-String -Path ".env" -Pattern '^\s*ALERT_COOLDOWN_SECONDS\s*=\s*(\d+)' | Select-Object -First 1
    if ($ligne) { $cooldown = [int]$ligne.Matches[0].Groups[1].Value }
}
if ($cooldown -ne 0) {
    Write-Host "ATTENTION : ALERT_COOLDOWN_SECONDS n'est pas a 0 dans .env (valeur : $cooldown)." -ForegroundColor Yellow
    Write-Host "L'alerte a 30 degres peut etre 'ignoree (cooldown)' si une alerte est partie recemment." -ForegroundColor Yellow
    Write-Host ""
}

$tests = @(
    @{ temp = 20.0; hum = 50.0; attendu = "Pas d'alerte" },
    @{ temp = 25.0; hum = 50.0; attendu = "Pas d'alerte (seuil strict > 25)" },
    @{ temp = 30.0; hum = 50.0; attendu = "Alerte Gmail + Telegram" }
)

foreach ($t in $tests) {
    $json = @{ temp = $t.temp; hum = $t.hum } | ConvertTo-Json -Compress
    try {
        $rep = Invoke-RestMethod -Uri $url -Method Post -Body $json -ContentType "application/json"
        Write-Host ("{0} °C -> 201 Created, id={1}, dt={2}  | attendu : {3}" -f $t.temp, $rep.id, $rep.dt, $t.attendu) -ForegroundColor Green
    }
    catch {
        $code = $_.Exception.Response.StatusCode.value__
        if (-not $code) { $code = "aucune reponse (serveur arrete ?)" }
        Write-Host ("{0} °C -> ECHEC, code HTTP : {1}" -f $t.temp, $code) -ForegroundColor Red
    }
    Start-Sleep -Seconds 1
}

Write-Host ""
Write-Host "Regarder la console du serveur pour les lignes [ALERTE], puis la boite Gmail et Telegram."
