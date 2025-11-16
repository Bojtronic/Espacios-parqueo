document.querySelector('#btnSaludo').addEventListener('click', async () => {
    const response = await fetch('/api/saludo');
    const data = await response.json();

    document.querySelector('#respuesta').innerText = data.mensaje;
});
