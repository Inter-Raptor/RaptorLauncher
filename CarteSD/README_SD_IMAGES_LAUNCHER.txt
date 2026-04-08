RaptorLauncher v0.9 - Images du launcher (RAW)
===============================================

Placez les images a la racine de la carte SD (dossier CarteSD), pas dans /games :

- /boot.raw      -> image de lancement (affichee au demarrage)
- /home_bg.raw   -> image de fond de l'accueil (icones + noms dessines par-dessus)

Configuration dans /settings.json :

{
  "boot_splash_raw": "/boot.raw",
  "boot_splash_ms": 1500,
  "home_bg_raw": "/home_bg.raw"
}

Notes :
- boot_splash_ms est en millisecondes (ex: 1000 = 1 seconde, 2000 = 2 secondes).
- Si boot.raw ou home_bg.raw n'existe pas (ou est invalide), le launcher reste en mode classique.
- Format attendu : RAW RGB565.
- Taille conseillee : 320x240 pour eviter recadrage/perte.
