# 🚗 Sistema de Control de Parqueo Inteligente

## 📌 Descripción del Proyecto

Este proyecto tiene como objetivo desarrollar un sistema para **detectar
la entrada y salida de vehículos en un parqueo** y llevar el control en
tiempo real de los espacios disponibles.

El sistema también contempla la gestión de **espacios especiales para
personas con discapacidad**, asegurando un control diferenciado.

------------------------------------------------------------------------

## 🎯 Objetivos

-   Detectar entrada y salida de vehículos
-   Calcular espacios disponibles en tiempo real
-   Gestionar espacios especiales (discapacidad)
-   Operar sin conexión a internet
-   Visualizar disponibilidad en pantalla o sistema web local

------------------------------------------------------------------------

## 🚧 Alcance

-   Control de vehículos en un único carril (entrada/salida compartida)
-   Capacidad para distinguir vehículos de objetos pequeños
    (personas/animales)
-   Exclusión temporal de espacios para motocicletas

------------------------------------------------------------------------

## 🧠 Arquitectura del Sistema

El sistema se compone de **2 dispositivos ESP32**:

### 🔹 Nodo 1: Entrada del parqueo

-   Detecta ingreso y salida de vehículos
-   Usa sensores ultrasónicos
-   Envía eventos mediante ESP-NOW

### 🔹 Nodo 2: Receptor / Visualización

Ubicación configurable: - Exterior del parqueo (pantalla de espacios
disponibles) - Central de monitoreo (interfaz web local)

Funciones: - Recibe eventos de entrada/salida - Calcula disponibilidad
de espacios - Muestra información al usuario

------------------------------------------------------------------------

## 📡 Comunicación

-   Protocolo: **ESP-NOW**
-   Alcance máximo: \~80 metros
-   Comunicación directa entre ESP32 (sin internet)

------------------------------------------------------------------------

## 🔌 Sensores

### Tipo:

-   Ultrasonidos **HC-SR04**

### Configuración:

-   4 sensores ultrasónicos

### Objetivo:

-   Detectar presencia y dirección del vehículo
-   Filtrar objetos pequeños (personas/animales)

------------------------------------------------------------------------

## 🧮 Lógica de Detección

El sistema utiliza un algoritmo basado en:

-   Tamaño del objeto detectado
-   Tiempo de detección
-   Secuencia de activación de sensores

### Consideraciones:

-   Un vehículo activa múltiples sensores en secuencia
-   Objetos pequeños no cumplen condiciones de tamaño/tiempo
-   Se determina:
    -   Entrada → resta espacios disponibles
    -   Salida → suma espacios disponibles

------------------------------------------------------------------------

## 🅿️ Gestión de Espacios

-   Conteo total de espacios
-   Subgrupo de espacios:
    -   Espacios para personas con discapacidad

### Reglas:

-   Se mantiene conteo independiente si es necesario
-   Posible expansión futura para motos (no incluido actualmente)

------------------------------------------------------------------------

## 📺 Visualización

### Opción 1: Pantalla externa

-   Display mostrando:
    -   Espacios disponibles
    -   Estado del parqueo

### Opción 2: Aplicación web local

-   Servida desde el ESP32 receptor
-   Acceso en red local mediante Ethernet
-   Información en tiempo real

------------------------------------------------------------------------

## 🔌 Conectividad del Nodo Receptor

-   Comunicación inalámbrica: ESP-NOW
-   Comunicación cableada: Ethernet (para interfaz web)

------------------------------------------------------------------------

## ⚙️ Tecnologías Utilizadas

-   **ESP32**
-   **ESP-NOW**
-   **Sensores HC-SR04**
-   **Arduino IDE**
-   **Visual Studio Code**

------------------------------------------------------------------------

## 🔋 Consideraciones Técnicas

-   Sin dependencia de internet
-   Baja latencia en comunicación
-   Sistema robusto ante interferencias
-   Diseño modular

------------------------------------------------------------------------

## 🧪 Estado del Proyecto

🚧 En desarrollo

-   [x] Definición de requerimientos
-   [ ] Diseño de arquitectura
-   [ ] Implementación de sensores
-   [ ] Comunicación ESP-NOW
-   [ ] Algoritmo de detección
-   [ ] Interfaz de visualización
-   [ ] Pruebas en campo

------------------------------------------------------------------------

## 🔮 Mejoras Futuras

-   Integración con cámaras
-   Reconocimiento de placas
-   Soporte para motocicletas
-   Registro histórico de ocupación
-   Integración con apps móviles

------------------------------------------------------------------------

## 📄 Licencia

Proyecto de uso interno / privado.
