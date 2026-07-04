import json
import psycopg2
import paho.mqtt.client as mqtt

DB_URL = "postgresql://postgres.lochrpnqgsoiluyepatj:[YOUR-PASSWORD]@aws-1-us-east-1.pooler.supabase.com:6543/postgres"

MQTT_BROKER = "172.27.27.90"
MQTT_PORT = 1883
MQTT_TOPIC = "lab/daq/nano1/raw"

# -------------------------
# PostgreSQL
# -------------------------
conn = psycopg2.connect(DB_URL)
cursor = conn.cursor()

print("PostgreSQL conectado")

# -------------------------
# MQTT callback
# -------------------------
def on_connect(client, userdata, flags, rc, properties=None):
    if rc == 0:
        print("MQTT conectado")
        client.subscribe(MQTT_TOPIC)
    else:
        print(f"Error MQTT: {rc}")

def on_message(client, userdata, msg):
    try:
        data = json.loads(msg.payload.decode())

        sample_id = data["id"]
        x = data["x"]
        y = data["y"]
        z = data["z"]

        cursor.execute("""INSERT INTO daq (id, valorx, valory, valorz) VALUES (%s, %s, %s, %s)""", (sample_id, x, y, z))

        conn.commit()

        print(f"Guardado: ID={sample_id} X={x} Y={y} Z={z}")

    except Exception as e:
        print("Error:", e)

# -------------------------
# MQTT client
# -------------------------
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)

client.on_connect = on_connect
client.on_message = on_message

client.connect(MQTT_BROKER, MQTT_PORT)

try:
    client.loop_forever()

except KeyboardInterrupt:
    print("\nPrograma interrumpido")

finally:
    client.disconnect()
    conn.close()
    print("Conexiones cerradas")
