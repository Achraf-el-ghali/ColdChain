from django.conf import settings
from django.shortcuts import render


# Page "tableau de bord" (http://IP_DU_PC:8000/).
# Elle ne lit pas la base elle-meme : le JavaScript de la page interroge
# /api/get toutes les 5 secondes et affiche la derniere mesure.
def dashboard(request):
    return render(request, "dashboard.html", {
        "seuil": settings.ALERT_THRESHOLD,
    })
