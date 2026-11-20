# PS5 Wake – guía de uso

PS5 Wake es una pequeña placa ESP32-C6 que enciende tu PlayStation 5 cuando recibe un paquete Wake-on-LAN o cuando
pulsas su botón. Por Bluetooth se hace pasar por tu mando DualSense emparejado, igual que al pulsar el botón PS.
Funciona desde el modo de reposo y con la consola totalmente apagada.

## Qué necesitas

- Una placa ESP32-C6 con PS5 Wake (probada: ESP32-C6-DevKitM-1), alimentada con cualquier cargador USB.
- Un mando DualSense (o DualSense Edge) ya emparejado con tu PS5.
- Un ordenador y un cable USB **de datos**, solo una vez, para instalar el firmware y leer las direcciones del mando.
- Una red Wi-Fi de 2,4 GHz.

## 1. Instala el firmware

Abre el instalador web (enlace en el README del proyecto) en Chrome o Edge, conecta la placa con un cable USB y pulsa
**Install**. Un dispositivo nuevo se ilumina en **morado**. Inicia por sí solo la red del panel de configuración.

## 2. Abre el panel de configuración

1. Mantén pulsado el botón **BOOT** entre 5 y 20 segundos y suéltalo. El LED se pone **rojo**. En un dispositivo nuevo el panel ya está activo (LED morado): omite este paso.
2. En el ordenador al que conectarás el mando, conéctate a la red Wi-Fi **`PS5-WAKE-xxxx`**, contraseña
   **`ps5wol-setup`**.
3. En el navegador abre **`https://192.168.4.1`** (o `https://ps5-wake-xxxx.local`). El navegador avisará de que la conexión no es privada, porque el ESP32 usa su propio certificado: elige **Configuración avanzada → Continuar** (los nombres dependen del navegador).
4. Elige tu idioma en la parte superior de la página.

## 3. Lee las direcciones del mando

1. Conecta el DualSense al ordenador con el cable USB.
2. En la sección **Bluetooth** del panel, descarga el lector de direcciones:
   - **Windows:** `ps5-wake-reader.cmd`; ábrelo con doble clic. Si Windows avisa de un editor desconocido, elige
     **Más información → Ejecutar de todas formas**.
   - **macOS:** ejecuta `pip3 install hidapi` y después `python3 ps5_wake_reader.py`.
   - **Linux:** `sudo python3 ps5_wake_reader.py` (sin paquetes adicionales).
3. El lector muestra las direcciones Bluetooth de la consola y del mando y copia un **código de configuración**
   (`PS5WAKE:…`) al portapapeles. Pégalo en el campo **Código de configuración del lector**: se rellenan ambas
   direcciones.

Las direcciones solo cambian si vuelves a emparejar el mando con la consola.

## 4. Termina la configuración

1. Elige tu red Wi-Fi con **Buscar**, escribe su contraseña y elige tu país.
2. Escribe una **nueva contraseña para la red del panel** (al menos 8 caracteres). Es obligatoria, porque la
   contraseña predeterminada es pública.
3. Pulsa **Guardar y reiniciar**. El panel se cierra, el LED parpadea en **azul** mientras se conecta y se pone
   **verde** cuando está listo.

## 5. Configura Wake-on-LAN

- En tu aplicación de Wake-on-LAN (o en la página WoL de tu router) usa la **dirección MAC del ESP32** que aparece en
  el panel, el puerto UDP **9** y la **dirección de difusión** de tu red (por ejemplo, `192.168.1.255`).
- Reserva una IP fija para el ESP32 en tu router.
- Si tu aplicación o tu router ya usan la **MAC de red de la propia PS5**, puedes introducirla en el panel; el ESP32
  también reaccionará a ella. En redes mesh (por ejemplo, ASUS AiMesh) esos paquetes pueden no llegar al ESP32; en
  ese caso envía el WoL a la MAC del ESP32.

## Uso diario

- **Estado de la consola:** el punto del icono de encendido en la parte superior del panel es verde cuando la PS5 está encendida, naranja en modo de reposo y rojo cuando está apagada o no accesible (gris: aún no detectada). El modo de reposo solo se detecta si la consola sigue conectada a internet.
- **Despertar la PS5:** envía un paquete Wake-on-LAN o pulsa brevemente **BOOT**. El LED destella en blanco,
  parpadea en cian durante el intento y luego destella **3 veces en verde** (la consola respondió) o **3 veces en
  rojo** (sin respuesta).
- Entre intentos pasan al menos 30 segundos; una petición ignorada destella una vez en naranja.

## El botón BOOT

| Tiempo pulsado | LED mientras se pulsa | Resultado |
|---|---|---|
| menos de 1 s | – | despertar la PS5 |
| 1–5 s | blanco | nada (cancelar) |
| 5–20 s, luego soltar | rojo (parpadea desde 15 s) | abrir el panel de configuración |
| 20 s | morado, parpadeo rápido | borrar todos los ajustes y reiniciar |

Mantener BOOT pulsado al conectar la alimentación inicia un modo seguro sin Wi-Fi ni Bluetooth (LED amarillo).

## Colores del LED

| Color | Comportamiento | Significado |
|---|---|---|
| morado | fijo | sin configurar – panel de configuración activo |
| azul | parpadeo lento | conectando al Wi-Fi |
| naranja | parpadeo lento | Wi-Fi configurado pero sin conexión durante más de 30 s |
| verde (tenue) | fijo | listo (se puede apagar en el panel) |
| rojo | fijo | panel de configuración activo |
| amarillo | fijo | modo seguro |
| blanco | un destello | Wake-on-LAN recibido o BOOT pulsado |
| cian | parpadeo rápido | intento de despertar en curso |
| verde | tres destellos | la consola respondió |
| rojo | tres destellos | la consola no respondió |
| naranja | un destello | petición ignorada (intento en curso o pausa de 30 s) |

## El panel de configuración

- El panel y su red se apagan al guardar, al pulsar **Cerrar sin cambios** o tras 10 minutos sin actividad.
  **Ampliar (+10 min)** da más tiempo.
- **Apagar el LED por completo**: Sin colores, parpadeos ni destellos. Solo mientras mantienes pulsado BOOT el LED sigue mostrando el tiempo de pulsación.
- **Nombre del dispositivo (opcional)**: Por ej. Salón. Se usa en la dirección del panel (`salon.local`), en Home Assistant y en el título de la página; útil con varios dispositivos.
- **Acceso desde la red doméstica**: Activado de forma predeterminada. La página seguirá disponible en tu red doméstica en `<IP del ESP32>` y `ps5-wake-xxxx.local`: inicia sesión con la contraseña del panel (usuario admin, salvo que lo hayas cambiado). La red Wi-Fi del panel no se activa; mantén BOOT 5 s para abrirla.
- **Conexión cifrada (HTTPS)**: Recomendado. El ESP32 crea su propio certificado, así que en la primera visita el navegador avisará de que la conexión no es privada: compara la huella de abajo y continúa. Sin HTTPS, la contraseña y los ajustes viajan sin cifrar por tu red. El certificado se renueva automáticamente antes de caducar (el ESP32 obtiene la hora de internet); **Nuevo certificado** crea uno nuevo al instante.
- **Despertar la PS5 ahora** prueba el despertar desde el panel.

## Solución de problemas

- **LED naranja parpadeando:** contraseña Wi-Fi incorrecta, red solo de 5 GHz o señal débil. Abre el panel, revisa
  el Wi-Fi y acerca el ESP32 al router. El ESP32 se reinicia solo si pasa 2 minutos sin conexión.
- **No pasa nada al enviar Wake-on-LAN (sin destello blanco):** el paquete no llega al ESP32. Usa la MAC del ESP32,
  la dirección de difusión y el puerto 9; comprueba que el ESP32 no esté en una red de invitados.
- **Destellos rojos tras un intento:** revisa las direcciones Bluetooth (vuelve a leerlas si emparejaste de nuevo el
  mando) y acerca el ESP32 a la consola.
- **El lector no encuentra el mando:** usa un cable de datos (no solo de carga) y conecta el mando directamente al
  ordenador.
- **El navegador avisa del certificado:** es normal; el panel se cifra con el propio certificado del ESP32. Continúa a la página; el panel muestra la huella del certificado.
