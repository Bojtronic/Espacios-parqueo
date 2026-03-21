const led = document.getElementById("led");
const texto = document.getElementById("estado-texto");
const listaIps = document.getElementById("lista-ips");

// ==========================
// ACTUALIZAR ESTADO DEL LED
// ==========================
async function obtenerEstado() {
    try {
        const res = await fetch('/api/datos');
        const data = await res.json();

        // Se asume que "valor" = 1 encendido, 0 apagado
        if (data.valor === 1) {
            led.classList.remove("off");
            led.classList.add("on");
            texto.innerText = "Encendido";
        } else {
            led.classList.remove("on");
            led.classList.add("off");
            texto.innerText = "Apagado";
        }

    } catch (error) {
        console.error("Error obteniendo estado:", error);
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

        // localhost también
        const liLocal = document.createElement("li");
        liLocal.innerText = "http://localhost:3000";
        listaIps.appendChild(liLocal);

    } catch (error) {
        console.error("Error obteniendo IPs:", error);
    }
}

// ==========================
// LOOP
// ==========================
setInterval(obtenerEstado, 100);

obtenerEstado();
obtenerIPs();
