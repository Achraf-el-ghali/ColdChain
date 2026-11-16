from django.urls import path
from . import api, views

urlpatterns = [
    path("", views.dashboard, name='dashboard'),  # tableau de bord

    path("api/get", api.Dlist, name='json'),
    path("api/post", api.Dhtviews.as_view(), name='json_post'),
]
