from django.urls import path
from . import api, views

urlpatterns = [
    path("", views.dashboard, name='dashboard'),  # tableau de bord

    path("api/get", api.Dlist, name='json'),
    path("api/post", api.Dhtviews.as_view(), name='json_post'),   # utilise par l'ESP8266
    path("api/add/", api.Dhtviews.as_view(), name='json_add'),    # meme vue, adresse du TP du prof
]
