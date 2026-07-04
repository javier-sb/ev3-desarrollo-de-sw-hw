# ev3-desarrollo-de-sw-hw

## Integración entre ESP32, MQTT y PostgreSQL

Se desarrolló un sistema de adquisición de datos capaz de capturar señales analógicas desde un acelerómetro y almacenarlas automáticamente en una base de datos PostgreSQL utilizando el protocolo MQTT como intermediario.

El flujo general de la información se muestra a continuación:

**Acelerómetro --> ESP32 --> Broker MQTT --> Aplicación Python --> PostgreSQL**

### Adquisición y transmisión de datos

El ESP32 adquiere tres canales analógicos correspondientes a los ejes **X**, **Y** y **Z** del acelerómetro a una frecuencia de **200 Hz**. Cada muestra se encapsula en un mensaje con formato JSON y se publica mediante MQTT en el tópico:

```text
lab/daq/nano1/raw
```

De esta forma, el microcontrolador únicamente se encarga de adquirir y transmitir los datos, sin interactuar directamente con la base de datos.

<img width="724" height="462" alt="mosquitto" src="https://github.com/user-attachments/assets/cc524d98-01c5-4f1a-bb0d-fca602abc6ac" />

### Almacenamiento de datos

Una aplicación desarrollada en Python actúa como cliente MQTT, suscribiéndose al tópico donde el ESP32 publica las mediciones. Cada mensaje recibido es procesado e insertado automáticamente en una tabla de PostgreSQL, permitiendo registrar de forma continua las muestras adquiridas.

<img width="1585" height="987" alt="postgresql_integration" src="https://github.com/user-attachments/assets/a379b8be-f9fe-4870-b90a-e523d320101f" />

### Monitoreo del sistema

Como complemento, el ESP32 incorpora un servidor web embebido que permite visualizar el estado del sistema y las últimas muestras adquiridas desde cualquier navegador conectado a la red, ya sea directamente al Access Point (AP) del ESP32 o el Wi-Fi del servidor con Mosquitto. Además, se implementó un sistema de indicadores LED para informar el estado de la conectividad (Wi-Fi, punto de acceso y comunicación MQTT), facilitando la supervisión del funcionamiento del dispositivo.

<img width="1919" height="1170" alt="AP_ESP32" src="https://github.com/user-attachments/assets/543c3b76-294e-4270-a734-f9ec7454f5e0" />

### Demo

La siguiente imagen muestra el funcionamiento real del sistema durante la inicialización. Se observa la creación del punto de acceso, la conexión a la red Wi-Fi configurada y el establecimiento exitoso de la comunicación con el broker MQTT. Esto resulta en que el LED indicador verde se active luego de lograr realizar la inicialización completa de manera exitosa

<img width="600" alt="poc" src="https://github.com/user-attachments/assets/0eb3a9a0-4e48-44d2-84b3-48567f524e44" />

```
22:48:00.054 -> ....
22:48:00.825 -> AP conectado.
22:48:00.825 -> AP SSID: NanoESP32-DAQ
22:48:00.825 -> AP IP: 192.168.4.1
22:48:00.825 -> STA conectado: 172.27.27.178
22:48:01.563 -> MQTT conectando...OK
```

Esta arquitectura desacopla la adquisición de datos del almacenamiento, permitiendo que otros clientes puedan suscribirse al mismo tópico MQTT sin necesidad de modificar el firmware del ESP32, facilitando así la escalabilidad del sistema.

## Instrucciones para ejecutar el proyecto

### Requisitos

- Arduino Nano ESP32
- Acelerometro
- Arduino IDE 2.x
- Python 3.10 o superior
- Broker MQTT (Mosquitto)
- PostgreSQL
- Librerías de Arduino:
  - WiFi
  - WebServer
  - PubSubClient
- Librerías de Python:
  - paho-mqtt
  - psycopg2

### 1. Clonar el repositorio

```bash
git clone https://github.com/javier-sb/ev3-desarrollo-de-sw-hw.git
cd ev3-desarrollo-de-sw-hw
```

### 2. Configurar el broker MQTT

Iniciar el servicio Mosquitto y verificar que se encuentre escuchando en el puerto **1883**.

### 3. Configurar PostgreSQL

Crear la tabla donde se almacenarán las muestras:

```sql
CREATE TABLE daq (
    created_at TIMESTAMP DEFAULT NOW(),
    id INTEGER PRIMARY KEY,
    valorx INTEGER,
    valory INTEGER,
    valorz INTEGER
);
```

### 4. Configurar el ESP32

Modificar en el archivo `.ino` los siguientes parámetros:

- SSID y contraseña de la red Wi-Fi.
- Dirección IP del broker MQTT.
- Tópico MQTT.

Compilar y cargar el firmware al ESP32.

### 5. Configurar la aplicación Python

Editar la variable `DB_URL` con las credenciales de PostgreSQL.

Crear un entorno virtual e instalar las dependencias:

```bash
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
```

### 6. Ejecutar el sistema

1. Iniciar Mosquitto.
2. Energizar el ESP32.
3. Verificar que el LED de estado sea verde.
4. Ejecutar la aplicación Python.
5. Comprobar que las muestras comiencen a almacenarse en PostgreSQL.

### 7. Visualización

Conectarse al punto de acceso del ESP32 o a la misma red Wi-Fi y acceder desde un navegador a la dirección IP del ESP32 para visualizar el estado del sistema y las últimas muestras adquiridas.
