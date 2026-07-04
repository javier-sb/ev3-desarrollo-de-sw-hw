from flask import Flask, render_template_string, request, redirect
import paho.mqtt.client as mqtt
from collections import deque

MQTT_BROKER = "172.27.27.90"
MQTT_PORT = 1883

TOPIC_DATA = "lab/daq/nano1/raw"
TOPIC_CMD = "lab/daq/nano1/cmd"

app = Flask(__name__)

logs = deque(maxlen=50) # ultimos 50 mensajes


# -------------------------
# MQTT
# -------------------------
def on_connect(client, userdata, flags, rc):
    print("MQTT conectado")
    client.subscribe(TOPIC_DATA)


def on_message(client, userdata, msg):
    text = msg.payload.decode()
    logs.append(f"{msg.topic} -> {text}")
    print(text)


client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect(MQTT_BROKER, MQTT_PORT)
client.loop_start()


# -------------------------
# Flask UI
# -------------------------
HTML = """
<!doctype html>
<html>
<head>
    <title>ESP32 DAQ Terminal</title>
    <meta http-equiv="refresh" content="1">
    <style>
        body { font-family: monospace; background: #111; color: #0f0; }
        .box { padding: 10px; }
        .btn { padding: 10px; background: red; color: white; border: none; }
    </style>
</head>
<body>
    <h2>MQTT Live Terminal</h2>

    <form method="POST" action="/restart">
        <button class="btn">Reiniciar ESP32</button>
    </form>

    <div class="box">
        {% for l in logs %}
            <div>{{ l }}</div>
        {% endfor %}
    </div>
</body>
</html>
"""


@app.route("/")
def index():
    return render_template_string(HTML, logs=list(logs))


@app.route("/restart", methods=["POST"])
def restart():
    client.publish(TOPIC_CMD, "restart")
    logs.append(">>> ENVIADO: restart")
    return redirect("/")


# -------------------------
# Run
# -------------------------
if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)
