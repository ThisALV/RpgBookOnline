# RpgBookOnline

Serveur multiplateforme de RpgBookOnline (Rbo), sous forme d'application console basique.

## Rbo

Rbo est le projet global auquel appartient ce serveur. Il s'agit-là d'une suite d'outils permettant de créer, héberger et jouer à des Livres dont vous êtes le Héro en ligne.

### Client

Le client est accessible depuis [ce dépôt](https://github.com/ThisALV/RboClient).

### Éditeur de livres

*Le développement n'a pas encore commencé.*


## Fonctionnement du serveur

### Les bases

Un Livre choisi est décomposé en plusieurs scènes. Chaque scène possède un identifiant unique dans le livre et une liste d'instructions exécutées comme un algorithme classique lorsque l'on entre dans la scène.

Le serveur possède un jeu d'instructions par défaut inclus au projet Rbo, il est cependant possible de l'élargir à l'aide du [modding](#le-modding).
Une instruction est ni plus ni moins qu'une fonctionnalité prête à être utilisée, à condition de lui fournir les bons paramètres.
*(Exemple : l'instruction Text affichant le message dans le paramètre text aux joueurs)*

Ces instructions sont les briques utilisées côté joueur pour créer et assembler des Livres dont vous êtes le Héros.

Un Livre possède également une liste de propriétés donnant les informations nécessaires pour un jeu, comme par exemple les statistiques, les inventaires, les objets, les évènements, etc. .

### Le modding

Afin de palier à un maximum de besoins en terme de Gameplay, Rbo est conçu pour être facilement moddable de par la possibilités d'ajouter de nouvelles instructions.

Celles-ci sont alors programmées en Lua, utilisant un objet interface entre le C++ (compilé) et le mod, permettant d'user de toutes les fonctionnalités de gameplay disponibles (envoyer du texte, changer le leader, les statistiques et/ou objets d'un joueur, etc.).

Il est par exemple possible, grâce à cette interface de modding, d'implémenter son propre système de combat, pour un boss final unique ou pour un type d'ennemi particulier.

Plus de précisions seront données sur l'API du serveur ainsi que sur comment mettre en place le modding lorsque le projet aura avancé un peu plus.

