const ctx = document.getElementById("grafico").getContext("2d");

const chart = new Chart(ctx, {
    type: 'bar',
    data: {
        labels: ['Espacios disponibles'],
        datasets: [{
            label: 'Cantidad',
            data: [0],
            backgroundColor: '#4fa3ff',
        }]
    },
    options: {
        scales: {
            y: { beginAtZero: true }
        }
    }
});

async function obtenerDatos() {
    try {
        const res = await fetch('/api/estado');
        const data = await res.json();

        document.getElementById('movimiento').innerText = data.ultimoMovimiento || "—";
        document.getElementById('espacios').innerText = data.espaciosDisponibles;

        chart.data.datasets[0].data[0] = data.espaciosDisponibles;
        chart.update();

    } catch (e) {
        console.error("Error obteniendo datos:", e);
    }
}

// actualiza cada segundo
setInterval(obtenerDatos, 1000);
obtenerDatos();
