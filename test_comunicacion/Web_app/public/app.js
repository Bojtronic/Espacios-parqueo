const led = document.getElementById("led");
const texto = document.getElementById("estado-texto");
const listaIps = document.getElementById("lista-ips");

// ==========================
// ACTUALIZAR UI
// ==========================
function actualizarUI(valor) {
    if (valor === 1) {
        led.classList.remove("off");
        led.classList.add("on");
        texto.innerText = "Encendido";
    } else {
        led.classList.remove("on");
        led.classList.add("off");
        texto.innerText = "Apagado";
    }
}

// ==========================
// WEBSOCKET
// ==========================
let socket;

function conectarWebSocket() {
    socket = new WebSocket(`ws://${window.location.host}`);

    socket.onopen = function () {
        console.log("WebSocket conectado");
    };

    socket.onmessage = function (event) {
        console.log("Mensaje recibido:", event.data);

        try {
            const data = JSON.parse(event.data);

            // Asegurar que sea número
            const valor = Number(data.v);

            actualizarUI(valor);

        } catch (error) {
            console.error("Error procesando mensaje:", error);
        }
    };

    socket.onerror = function (error) {
        console.error("Error WebSocket:", error);
    };

    socket.onclose = function () {
        console.warn("⚠️ WebSocket desconectado");

        // Reconexión automática
        setTimeout(() => {
            console.log("Reintentando conexión...");
            conectarWebSocket();
        }, 2000);
    };
}

// ==========================
// OBTENER ESTADO INICIAL
// ==========================
async function obtenerEstadoInicial() {
    try {
        const res = await fetch('/api/datos');
        const data = await res.json();

        const valor = Number(data.v);
        actualizarUI(valor);

        console.log("Estado inicial cargado:", data);

    } catch (error) {
        console.error("Error obteniendo estado inicial:", error);
    }
}

// ==========================
// OBTENER IPS
// ==========================
async function obtenerIPs() {
    try {
        const res = await fetch('/api/ips');
        const ips = await res.json();

        listaIps.innerHTML = "";

        ips.forEach(ip => {
            const li = document.createElement("li");
            li.innerText = `http://${ip}:3000`;
            listaIps.appendChild(li);
        });

        const liLocal = document.createElement("li");
        liLocal.innerText = "http://localhost:3000";
        listaIps.appendChild(liLocal);

    } catch (error) {
        console.error("Error obteniendo IPs:", error);
    }
}

// ==========================
// INICIO
// ==========================
obtenerIPs();
obtenerEstadoInicial();
conectarWebSocket();

