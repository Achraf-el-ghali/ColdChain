from .models import Dht11
from .serializers import DHT11serialize
from rest_framework.decorators import api_view
from rest_framework import generics
from rest_framework.response import Response

from django.conf import settings
from django.core.cache import cache
from django.core.mail import send_mail
from django.utils import timezone
import telebot


@api_view(['GET'])
def Dlist(request):
    all_data = Dht11.objects.all()
    data = DHT11serialize(all_data, many=True).data
    return Response({'data': data})


class Dhtviews(generics.CreateAPIView):
    queryset = Dht11.objects.all()
    serializer_class = DHT11serialize

    # Appelee par Django REST juste apres la validation du JSON recu en POST.
    # On enregistre la mesure, puis on decide s'il faut envoyer une alerte.
    def perform_create(self, serializer):
        # 1) Enregistrer la mesure dans la base (comme avant)
        mesure = serializer.save()

        # 2) Pas d'alerte si la temperature n'est pas STRICTEMENT > seuil
        #    (25 degres -> pas d'alerte, 30 degres -> alerte)
        seuil = settings.ALERT_THRESHOLD
        if mesure.temp is None or mesure.temp <= seuil:
            return

        # 3) Anti-spam : au plus une alerte toutes les ALERT_COOLDOWN_SECONDS.
        #    On note dans le cache Django qu'une alerte vient de partir ;
        #    cache.add() echoue si la note existe deja (= cooldown en cours).
        #    Avec 0 seconde, pas de cooldown : une alerte par mesure (TP).
        delai = settings.ALERT_COOLDOWN_SECONDS
        if delai > 0:
            if not cache.add("derniere_alerte", timezone.now(), timeout=delai):
                print(f"[ALERTE] ignorée (cooldown {delai} s) : "
                      f"{mesure.temp:.1f} °C", flush=True)
                return

        # 4) Construire le message (heure locale de Casablanca)
        heure = timezone.localtime(mesure.dt).strftime("%H:%M:%S")
        humidite = f"{mesure.hum:.1f}" if mesure.hum is not None else "?"
        message = (f"⚠️ Alerte Cold Chain : température = {mesure.temp:.1f} °C "
                   f"(seuil {seuil:g} °C), humidité = {humidite} %, à {heure}")

        # 5) E-mail Gmail. Le try/except protege le reste :
        #    si Gmail echoue, Telegram part quand meme et l'ESP recoit 201.
        try:
            if not settings.ALERT_EMAIL_TO:
                raise ValueError("ALERT_EMAIL_TO est vide dans .env")
            send_mail(
                subject="⚠️ Alerte Cold Chain : température trop haute",
                message=message,
                from_email=settings.DEFAULT_FROM_EMAIL,
                recipient_list=[settings.ALERT_EMAIL_TO],
                fail_silently=False,
            )
            print("[ALERTE] Gmail envoyé", flush=True)
        except Exception as e:
            print(f"[ALERTE] ERREUR Gmail : {e}", flush=True)

        # 6) Message Telegram, dans un try/except separe
        try:
            if not settings.TELEGRAM_TOKEN or not settings.TELEGRAM_CHAT_ID:
                raise ValueError("TELEGRAM_TOKEN ou TELEGRAM_CHAT_ID vide dans .env")
            bot = telebot.TeleBot(settings.TELEGRAM_TOKEN)
            bot.send_message(settings.TELEGRAM_CHAT_ID, message)
            print("[ALERTE] Telegram envoyé", flush=True)
        except Exception as e:
            print(f"[ALERTE] ERREUR Telegram : {e}", flush=True)
