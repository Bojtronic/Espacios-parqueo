const express = require('express');
const path = require('path');
const app = express();

// Para parsear JSON en las peticiones
app.use(express.json());

// Servir archivos estáticos desde /public
app.use(express.static(path.join(__dirname, './public')));

// ---- RUTAS DEL API ----
app.get('/api/movimiento', (req, res) => {
    res.json({ mensaje: "Hola desde la API!" });
});

app.post('/api/espacios', (req, res) => {
    const { texto } = req.body;
    res.json({ recibido: texto });
});

// ---- INICIO DEL SERVIDOR ----
const PORT = process.env.PORT || 3000;

app.listen(PORT, '0.0.0.0', () => {
    console.log(`Servidor accesible en la red: http://192.168.18.26:${PORT}`);
});

