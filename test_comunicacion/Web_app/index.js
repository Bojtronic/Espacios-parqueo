const express = require('express');
const os = require('os');
const path = require('path');
const WebSocket = require('ws');

const app = express();
app.use(express.json());

app.use(express.static(path.join(__dirname, './public')));

function obtenerIPs() {
    const interfaces = os.networkInterfaces();
    const ips = [];

    for (let nombre in interfaces) {
        for (let iface of interfaces[nombre]) {
            if (iface.family === 'IPv4' && !iface.internal) {
                ips.push(iface.address);
            }
        }
    }

    return ips;
}

let estado = {
    mensaje: "Sin datos",
    valor: 0
};

// ==========================
// WEBSOCKET SERVER
// ==========================
const server = app.listen(3000, '0.0.0.0', () => {
    const ips = obtenerIPs();

    console.log("Servidor corriendo en:");
    ips.forEach(ip => console.log(`👉 http://${ip}:3000`));
    console.log(`👉 http://localhost:3000`);
});

const wss = new WebSocket.Server({ server });

wss.on('connection', (ws) => {
    console.log("Cliente WebSocket conectado");

    // Enviar estado actual inmediatamente
    ws.send(JSON.stringify(estado));
});

function enviarATodos(data) {
    wss.clients.forEach(client => {
        if (client.readyState === WebSocket.OPEN) {
            client.send(JSON.stringify(data));
        }
    });
}

// ==========================
// API
// ==========================
app.get('/api/ips', (req, res) => {
    res.json(obtenerIPs());
});

app.post('/api/datos', (req, res) => {
    estado = req.body;

    console.log("Datos recibidos:", estado);

    // Enviar a todos los clientes en tiempo real
    enviarATodos(estado);

    res.json({ ok: true });
});

app.get('/api/datos', (req, res) => {
    res.json(estado);
});
