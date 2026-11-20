# PS5 Wake – guide d’utilisation

PS5 Wake est une petite carte ESP32-C6 qui allume votre PlayStation 5 lorsqu’elle reçoit un paquet Wake-on-LAN ou
lorsque vous appuyez sur son bouton. En Bluetooth, elle se fait passer pour votre manette DualSense associée – comme
un appui sur la touche PS. Cela fonctionne depuis le mode repos et avec une console complètement éteinte.

## Ce qu’il vous faut

- Une carte ESP32-C6 avec PS5 Wake (testée : ESP32-C6-DevKitM-1), alimentée par n’importe quel chargeur USB.
- Une manette DualSense (ou DualSense Edge) déjà associée à votre PS5.
- Un ordinateur et un câble USB **de données** – une seule fois, pour installer le firmware et lire les adresses de
  la manette.
- Un réseau Wi-Fi 2,4 GHz.

## 1. Installer le firmware

Ouvrez l’installateur web (lien dans le README du projet) dans Chrome ou Edge, branchez la carte avec un câble USB
et cliquez sur **Install or update**. Un nouvel appareil s’allume en **violet**. Il lance tout seul le réseau du panneau de configuration.

## 2. Ouvrir le panneau de configuration

1. Maintenez le bouton **BOOT** 5 à 20 secondes, puis relâchez-le. La LED devient **rouge**. Sur un appareil neuf, le panneau est déjà actif (LED violette) – passez cette étape.
2. Sur l’ordinateur auquel vous brancherez la manette, connectez-vous au réseau Wi-Fi **`PS5-WAKE-xxxx`**, mot de
   passe **`ps5wol-setup`**.
3. Dans le navigateur, ouvrez **`https://192.168.4.1`** (ou `https://ps5-wake-xxxx.local`). Le navigateur indique que la connexion n’est pas privée, car l’ESP32 utilise son propre certificat – choisissez **Paramètres avancés → Continuer** (les libellés dépendent du navigateur).
4. Choisissez votre langue en haut de la page.

## 3. Lire les adresses de la manette

1. Branchez la DualSense à l’ordinateur avec le câble USB.
2. Dans la section **Bluetooth** du panneau, téléchargez le lecteur d’adresses :
   - **Windows :** `ps5-wake-reader.cmd` – double-cliquez dessus. Si Windows signale un éditeur inconnu, choisissez
     **Informations complémentaires → Exécuter quand même**.
   - **macOS :** lancez `pip3 install hidapi`, puis `python3 ps5_wake_reader.py`.
   - **Linux :** `sudo python3 ps5_wake_reader.py` (aucun paquet supplémentaire).
3. Le lecteur affiche les adresses Bluetooth de la console et de la manette et copie un **code de configuration**
   (`PS5WAKE:…`) dans le presse-papiers. Collez-le dans le champ **Code de configuration du lecteur d’adresses** –
   les deux adresses sont remplies.

Les adresses ne changent que si vous associez de nouveau la manette à la console.

## 4. Terminer la configuration

1. Choisissez votre réseau Wi-Fi avec **Rechercher**, saisissez son mot de passe et choisissez votre pays.
2. Saisissez un **nouveau mot de passe pour le réseau du panneau** (au moins 8 caractères). Il est obligatoire, car le
   mot de passe par défaut est public.
3. Cliquez sur **Enregistrer et redémarrer**. Le panneau se ferme, la LED clignote en **bleu** pendant la connexion
   puis devient **verte** quand tout est prêt.

## 5. Configurer le Wake-on-LAN

- Dans votre application Wake-on-LAN (ou la page WoL de votre routeur), utilisez l’**adresse MAC de l’ESP32**
  affichée dans le panneau, le port UDP **9** et l’**adresse de diffusion** de votre réseau (par ex.
  `192.168.1.255`).
- Réservez une adresse IP fixe pour l’ESP32 dans votre routeur.
- Si votre application ou votre routeur utilise déjà l’**adresse MAC réseau de la PS5 elle-même**, vous pouvez la
  saisir dans le panneau – l’ESP32 y réagira aussi. Dans les réseaux maillés (par ex. ASUS AiMesh), ces paquets
  peuvent ne pas atteindre l’ESP32 ; envoyez alors le WoL à la MAC de l’ESP32.

## Au quotidien

- **État de la console :** le point sur l’icône d’alimentation en haut du panneau est vert quand la PS5 est allumée, orange en mode repos et rouge quand elle est éteinte ou injoignable (gris : pas encore détectée). Le mode repos n’est détecté que si la console reste connectée à Internet.
- **Réveiller la PS5 :** envoyez un paquet Wake-on-LAN ou appuyez brièvement sur **BOOT**. La LED clignote une fois
  en blanc, clignote en cyan pendant la tentative, puis **3× en vert** (la console a répondu) ou **3× en rouge**
  (pas de réponse).
- Les tentatives sont espacées d’au moins 30 secondes ; une demande ignorée donne un flash orange.

## Le bouton BOOT

| Durée d’appui | LED pendant l’appui | Résultat |
|---|---|---|
| moins de 1 s | – | réveiller la PS5 |
| 1–5 s | blanche | rien (annulation) |
| 5–20 s, puis relâcher | rouge (clignote dès 15 s) | ouvrir le panneau de configuration |
| 20 s | violette, clignote vite | effacer tous les réglages et redémarrer |

Maintenir BOOT en branchant l’alimentation lance un mode sans échec sans Wi-Fi ni Bluetooth (LED jaune).

## Couleurs de la LED

| Couleur | Comportement | Signification |
|---|---|---|
| violet | fixe | non configuré – panneau de configuration actif |
| bleu | clignote lentement | connexion au Wi-Fi |
| orange | clignote lentement | Wi-Fi configuré mais non connecté depuis plus de 30 s |
| vert (atténué) | fixe | prêt (désactivable dans le panneau) |
| rouge | fixe | panneau de configuration actif |
| jaune | fixe | mode sans échec |
| blanc | un flash | Wake-on-LAN reçu ou BOOT pressé |
| cyan | clignote vite | tentative de réveil en cours |
| vert | trois flashs | la console a répondu |
| rouge | trois flashs | la console n’a pas répondu |
| orange | un flash | demande ignorée (tentative en cours ou pause de 30 s) |

## Le panneau de configuration

- Le panneau et son réseau s’arrêtent après l’enregistrement, après **Fermer sans modifier** ou après 10 minutes
  sans action. **Prolonger (+10 min)** donne plus de temps.
- **Éteindre complètement la LED**: Aucune couleur, aucun clignotement ni flash. La LED indique seulement la durée d’appui pendant que vous maintenez BOOT.
- **Nom de l’appareil (facultatif)**: Par ex. Salon. Utilisé dans l’adresse du panneau (`salon.local`), dans Home Assistant et dans le titre de la page – utile avec plusieurs appareils.
- **Accès depuis le réseau domestique**: Activé par défaut. La page reste accessible sur votre réseau domestique à `<IP de l’ESP32>` et `ps5-wake-xxxx.local` – connexion avec le mot de passe du panneau (nom d’utilisateur admin, sauf si vous l’avez changé). Le réseau Wi-Fi du panneau n’est pas lancé ; maintenez BOOT 5 s pour l’ouvrir.
- **Connexion chiffrée (HTTPS)**: Recommandé. L’ESP32 crée son propre certificat : lors de la première visite, le navigateur indique que la connexion n’est pas privée – comparez l’empreinte ci-dessous et continuez. Sans HTTPS, le mot de passe et les réglages circulent en clair sur votre réseau. Le certificat est renouvelé automatiquement avant son expiration (l’ESP32 obtient l’heure sur Internet) ; **Nouveau certificat** en crée un nouveau immédiatement.
- **Réveiller la PS5** teste le réveil depuis le panneau.

## Dépannage

- **LED orange clignotante :** mauvais mot de passe Wi-Fi, réseau uniquement 5 GHz ou signal faible. Ouvrez le
  panneau, vérifiez les réglages Wi-Fi et rapprochez l’ESP32 du routeur. L’ESP32 redémarre seul s’il reste hors
  ligne 2 minutes.
- **Rien ne se passe après l’envoi du Wake-on-LAN (pas de flash blanc) :** le paquet n’atteint pas l’ESP32. Utilisez
  la MAC de l’ESP32, l’adresse de diffusion et le port 9 ; vérifiez que l’ESP32 n’est pas sur un réseau invité.
- **Flashs rouges après une tentative :** vérifiez les adresses Bluetooth (relisez-les après une nouvelle
  association de la manette) et rapprochez l’ESP32 de la console.
- **Le lecteur ne trouve pas la manette :** utilisez un câble de données (pas seulement de charge) et branchez la
  manette directement à l’ordinateur.
- **Le navigateur signale le certificat :** c’est normal – le panneau est chiffré avec le propre certificat de l’ESP32. Continuez vers la page ; le panneau affiche l’empreinte du certificat.
