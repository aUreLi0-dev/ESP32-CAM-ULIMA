# ESP32-CAM ULIMA: detección de objetos en tiempo real con YOLO

Proyecto educativo del **Taller ULIMA 2026 – Laboratorio de Internet de las Cosas**. Integra una cámara ESP32-CAM con un programa Python que recibe el video por Wi-Fi, analiza los fotogramas mediante un modelo YOLO y presenta los resultados en una página web local.

## ¿Qué hace el proyecto?

1. La ESP32-CAM captura video y publica un flujo MJPEG en la red local.
2. El programa Python se conecta al flujo de la cámara y procesa los fotogramas con YOLO.
3. La aplicación web muestra el video con cajas de detección, los objetos identificados y su nivel de confianza.
4. Permite modificar la confianza mínima, consultar métricas del procesamiento y guardar una fotografía original y otra con las detecciones.

La detección se ejecuta en la **PC**, no en la ESP32-CAM. La cámara captura y transmite; Python y YOLO realizan el análisis.

## Arquitectura

```text
ESP32-CAM AI-Thinker
  └── Captura y transmite video por Wi-Fi
       └── http://IP_DE_LA_ESP32:81/stream
            └── PC con Python
                 ├── Requests: recibe el flujo MJPEG
                 ├── OpenCV y NumPy: decodifican los fotogramas
                 ├── Ultralytics YOLO: detecta objetos
                 └── Flask: publica la interfaz web
                      └── http://localhost:5000
```

## Estructura del repositorio

```text
ESP32-CAM-ULIMA/
├── CameraWebServer_proyecto/
│   ├── CameraWebServer_proyecto.ino  # Firmware de la cámara
│   ├── app_httpd.cpp                 # Servidor web y endpoints de cámara
│   ├── board_config.h                # Selección del modelo de cámara
│   ├── camera_index.h                # Interfaz web del CameraWebServer
│   ├── camera_pins.h                 # Asignación de pines por placa
│   ├── ci.yml                        # Configuración de integración continua
│   └── partitions.csv                # Tabla de particiones
├── Codigo python/
│   ├── servidor_web.py               # Captura del video, YOLO y Flask
│   ├── README.md                     # Notas del experimento web
│   ├── yolo26n.pt                    # Pesos del modelo de detección
│   └── templates/
│       └── index.html                # Interfaz de visualización
└── README.md
```

El programa crea automáticamente las carpetas `fotos/` y `resultados_yolo/` dentro de `Codigo python/` cuando se ejecuta y se guardan imágenes.

## Componentes y herramientas

### Hardware

- ESP32-CAM compatible con el perfil **AI-Thinker** (la placa seleccionada en el código).
- Sensor de cámara compatible con la placa.
- Adaptador USB a serial/FTDI para programar la ESP32-CAM, si la placa no incorpora USB.
- PC con Windows, Linux o macOS para ejecutar Python.
- Conexión Wi-Fi compartida entre la cámara y la PC.

### Software

- Arduino IDE y soporte de placas ESP32.
- Python 3.
- OpenCV, NumPy y Requests para recibir y decodificar el flujo de imágenes.
- Ultralytics YOLO para detección de objetos.
- Flask para publicar la interfaz web.
- Navegador web moderno, como Chrome o Edge.

## 1. Configurar y cargar el firmware de la ESP32-CAM

1. Instala Arduino IDE y el soporte de placas ESP32.
2. Descarga o clona este repositorio.
3. Abre `CameraWebServer_proyecto/CameraWebServer_proyecto.ino` en Arduino IDE. Mantén los archivos auxiliares en la misma carpeta del sketch.
4. En `CameraWebServer_proyecto/board_config.h`, verifica que esté seleccionado `CAMERA_MODEL_AI_THINKER`. Si utilizas otra placa, configura el modelo y los pines adecuados antes de compilar.
5. En el archivo `CameraWebServer_proyecto.ino`, configura el nombre y la contraseña de tu red Wi-Fi en las variables correspondientes.
6. Selecciona la placa y el puerto serial correctos, y carga el firmware.
7. Abre el Monitor Serial a **115200 baudios** y reinicia la cámara si hace falta.
8. Espera el mensaje de conexión Wi-Fi y anota la dirección IP que muestra el Monitor Serial.

### Parámetros de cámara del firmware actual

El sketch configura JPEG, solicita resolución **UXGA (1600 × 1200)**, fija la frecuencia XCLK en 20 MHz y usa PSRAM para los búferes de imagen. La disponibilidad real de la resolución y de los búferes depende de la placa, la PSRAM y la configuración de compilación.

Después de iniciar el servidor, el firmware muestra una dirección web para la cámara y la dirección del flujo de video, con formato:

- Página de cámara: `http://IP_DE_LA_ESP32`
- Flujo MJPEG: `http://IP_DE_LA_ESP32:81/stream`

Antes de continuar, comprueba desde la PC que ambos dispositivos están en la misma red y que el flujo se puede abrir en el navegador.

> **Seguridad:** el firmware actual define las credenciales Wi-Fi en el código fuente. Sustitúyelas por las de tu entorno antes de usarlo. No publiques contraseñas reales en GitHub; si ya se expusieron credenciales válidas, cámbialas.

## 2. Preparar Python

Abre una terminal en la carpeta del proyecto y entra en la carpeta de Python:

```bash
cd "Codigo python"
```

Instala las dependencias:

```bash
python -m pip install ultralytics flask requests opencv-python numpy
```

El repositorio incluye el archivo de pesos `yolo26n.pt`. El programa intenta cargar ese modelo desde la carpeta actual; si no lo encuentra, Ultralytics puede intentar descargarlo. La descarga requiere conexión a Internet.

## 3. Configurar la conexión a la ESP32-CAM

Abre `Codigo python/servidor_web.py` y localiza estas variables de configuración:

```python
ESP32_IP = "192.168.51.164"  # Cambia este valor por la IP real de tu ESP32-CAM
FUENTE = "esp32"
CONFIANZA_INICIAL = 0.30
TAMANO_IMAGEN = 640
PUERTO_WEB = 5000
COMPARTIR_EN_RED = False
```

Cambia `ESP32_IP` por la dirección que muestra el Monitor Serial. No incluyas `http://` ni el puerto en ese valor; el programa construye la dirección del flujo automáticamente.

| Parámetro | Función |
|---|---|
| `ESP32_IP` | Dirección IP de la ESP32-CAM dentro de la red local. |
| `FUENTE` | Usa `"esp32"` para la cámara o `"webcam"` para probar con la cámara de la PC. |
| `CONFIANZA_INICIAL` | Umbral inicial de confianza; en el código está establecido en 0.30 (30 %). |
| `TAMANO_IMAGEN` | Tamaño usado por YOLO durante la inferencia. El código comenta que 320 puede ser más rápido y 640 puede ofrecer más detalle. |
| `MAX_OBJETOS` | Límite de detecciones por fotograma; actualmente es 15. |
| `PUERTO_WEB` | Puerto de la aplicación Flask; actualmente es 5000. |
| `COMPARTIR_EN_RED` | En `False`, la página queda disponible solo en la PC. En `True`, puede compartirse con otros dispositivos de la red local. |

## 4. Ejecutar la aplicación

Desde la carpeta `Codigo python`, ejecuta:

```bash
python servidor_web.py
```

El programa carga el modelo, inicia en segundo plano la recepción del video y la detección, levanta el servidor Flask y trata de abrir el navegador automáticamente.

La interfaz se encuentra en:

**http://localhost:5000**

Si el navegador no se abre automáticamente, escribe esa dirección manualmente. Para detener el servidor, vuelve a la terminal y pulsa `Ctrl + C`.

### Probar sin una ESP32-CAM

Para comprobar la interfaz sin la cámara de la placa, cambia temporalmente la configuración en `servidor_web.py`:

```python
FUENTE = "webcam"
```

Ejecuta el programa de nuevo. La cámara integrada o USB de la PC debe estar disponible para OpenCV.

### Compartir la página con otros dispositivos de la red

Por defecto, `COMPARTIR_EN_RED` está en `False` y la aplicación se limita a la propia PC. Para habilitar el acceso desde móviles u otras computadoras conectadas a la misma red Wi-Fi:

1. Cambia `COMPARTIR_EN_RED = True`.
2. Si deseas mostrar el código QR, instala el paquete opcional con `python -m pip install "qrcode[pil]"`.
3. Permite el acceso de Python a través del firewall de la PC cuando corresponda.
4. Abre desde el otro dispositivo la dirección de red que imprime la terminal.

Activa esta opción solo en una red confiable: la aplicación no incorpora autenticación de usuarios.

## Funciones de la interfaz web

- **Video en vivo:** muestra los fotogramas procesados con cajas y etiquetas sobre los objetos detectados.
- **Lista de detecciones:** presenta el nombre del objeto en español, el nombre de la clase del modelo y el porcentaje de confianza.
- **Métricas:** muestra el número de objetos detectados en el fotograma, los fotogramas procesados por segundo y el tiempo de inferencia.
- **Confianza mínima:** permite ajustar el umbral desde la página. Un umbral más bajo suele mostrar más detecciones, pero puede aumentar los falsos positivos; uno más alto filtra detecciones menos seguras.
- **Guardar foto:** guarda el fotograma original en `fotos/` y la imagen con las cajas de detección en `resultados_yolo/`.
- **Reto de las 80 clases:** registra las clases COCO encontradas durante la sesión y permite reiniciar ese conteo.

El modelo preentrenado reconoce las categorías con las que fue entrenado; no identifica automáticamente cualquier objeto ni realiza un inventario personalizado sin entrenamiento o configuración adicional.

## Problemas frecuentes

| Síntoma | Qué comprobar |
|---|---|
| La página permanece en “Esperando el video” | Verifica que el firmware esté ejecutándose y que `ESP32_IP` coincida con la IP actual. Prueba `http://IP_DE_LA_ESP32:81/stream` desde la PC. |
| La cámara aparece desconectada | Confirma que la ESP32-CAM y la PC estén en la misma red, revisa la dirección IP y reinicia la placa. La terminal muestra los errores de conexión y reintenta la conexión. |
| La cámara inicia, pero falla la carga del firmware | Verifica la placa seleccionada, los pines, el puerto serial, la alimentación y la configuración de PSRAM. |
| El video o la detección son lentos | Prueba reducir `TAMANO_IMAGEN` a `320` y revisa la resolución configurada en el firmware. El rendimiento depende de la PC, la red y el tamaño de imagen. |
| El puerto 5000 está ocupado | Cierra otra instancia de la aplicación o cambia `PUERTO_WEB` en `servidor_web.py`. |
| No aparece el QR | Instala el paquete opcional `qrcode[pil]` y habilita `COMPARTIR_EN_RED`. |
| El modelo no se encuentra | Asegúrate de ejecutar el programa desde `Codigo python` o de tener conexión a Internet para que Ultralytics pueda descargarlo. |

## Alcance y limitaciones

- La ESP32-CAM transmite el video; la inferencia de YOLO se ejecuta en la PC.
- La visualización es continua mientras el servidor y la cámara funcionan, pero la velocidad efectiva de detección depende del hardware y de la red.
- El sistema utiliza un modelo preentrenado y las clases configuradas en el código; los resultados pueden ser incorrectos y no sustituyen una verificación humana.
- El acceso a la aplicación web no está protegido por autenticación. No la publiques directamente en Internet ni la habilites en una red no confiable.

## Nota sobre el nombre del modelo

El código de `servidor_web.py` carga `yolo26n.pt`, que también está incluido en el repositorio. Sin embargo, algunas etiquetas informativas de `templates/index.html` y el README de esa carpeta todavía mencionan “YOLO11n”. Por eso, para identificar los pesos que se ejecutan actualmente, toma como referencia el nombre de archivo configurado en `servidor_web.py`.

## Créditos y referencias

- [Ultralytics YOLO](https://github.com/ultralytics/ultralytics) — implementación de detección de objetos.
- [Documentación de Ultralytics](https://docs.ultralytics.com/) — instalación y uso de modelos.
- [COCO Dataset](https://cocodataset.org/) — conjunto de datos asociado a las clases comunes del modelo.
- [ESP32 Camera Driver de Espressif](https://github.com/espressif/esp32-camera) — controlador de cámara usado por los ejemplos de ESP32-CAM.

---

Proyecto educativo del **Taller ULIMA 2026 – Laboratorio de Internet de las Cosas**.
