const express = require('express');
const path = require('path');
const app = express();

// Para parsear JSON en las peticiones
app.use(express.json());

// Servir archivos estáticos desde /public
app.use(express.static(path.join(__dirname, './public')));

let ultimoMovimiento = "—";
let espaciosDisponibles = 0;

// ---- RUTAS DEL API ----

// Recibe datos del ESP32
app.post('/api/movimiento', (req, res) => {
    const { movimiento, espacios } = req.body;

    if (!movimiento || espacios === undefined) {
        return res.status(400).json({ error: "Datos incompletos" });
    }

    ultimoMovimiento = movimiento;
    espaciosDisponibles = espacios;

    console.log("Movimiento recibido:", movimiento, " | Espacios:", espacios);

    res.json({ status: "OK" });
});

// Enviados al frontend cada segundo
app.get('/api/estado', (req, res) => {
    res.json({
        ultimoMovimiento,
        espaciosDisponibles
    });
});

// ---- INICIO DEL SERVIDOR ----
const PORT = process.env.PORT || 3000;

app.listen(PORT, '0.0.0.0', () => {
    console.log(`Servidor accesible en la red: http://192.168.18.26:${PORT}`);
});
