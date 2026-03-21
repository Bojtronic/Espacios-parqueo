const express = require('express');
const os = require('os');
const path = require('path');

const app = express();
app.use(express.json());

// Servir archivos estáticos desde /public
app.use(express.static(path.join(__dirname, './public')));

// ==========================
// OBTENER IPs LOCALES
// ==========================
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

// ==========================
// DATOS
// ==========================
let estado = {
    mensaje: "Sin datos",
    valor: 0
};

// ==========================
// API
// ==========================
app.get('/api/ips', (req, res) => {
    res.json(obtenerIPs());
});

app.post('/api/datos', (req, res) => {
    estado = req.body;

    console.log("Datos recibidos:", estado);

    res.json({ ok: true });
});

app.get('/api/datos', (req, res) => {
    res.json(estado);
});

// ==========================
// SERVIDOR
// ==========================
const PORT = 3000;

app.listen(PORT, '0.0.0.0', () => {
    const ips = obtenerIPs();

    console.log("Servidor corriendo en:");

    ips.forEach(ip => {
        console.log(`👉 http://${ip}:${PORT}`);
    });

    console.log(`👉 http://localhost:${PORT}`);
});
