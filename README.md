# PICO-OS
Sistema operativo Unix-Like creado con Arduino IDE para RPI Pico W con motor de intérprete para lenguaje C integrado. <br>
Empezó como una obsesión por ver si era posible portar Linux 1.0 a la Rpi Pico sin necesidad de módulos MicroSD, pero al final terminé escribiendo un kernel monolítico bastante funcional.<br>

# Configuración
* Librería Raspberry Pi Pico/RP2040/RP2350 de <a href="https://github.com/earlephilhower/arduino-pico">Earle F. Philhower, III<a/>.
* Flash size: 2MB (1MB flash / 1MB sketch)
* CPU speed: 200MHZ
* IP/Bluetooth stack: IPv4 + IPv6 + Bluetooth stack
* Optimize: Small (-Os) standar
* Upload method: default (UF2)
* Operating system: None
* Profiling: disabled
* RTTI: disabled
* Stack protector: disabled
* USB stack: Pico SDK
* WiFi region: Worldwide

# Capacidades
* Sistema de almacenamiento permanente (LittleFS).
* Shell con manejo de archivos estilo Busybox.
* Terminal (Serial o Web).
* Servidor web local para gestión remota.
* Conectividad WiFi.
* Navegador web en ascii. (limitado a 500 caracteres)
* Intérprete para lenguaje C <b>minic</b> (soporte para tipos de datos, arreglos, funciones, etc). <b>Casi es un proyecto completo independiente.</b>
* Soporte para ejecutar aplicaciones. (usar intérprete <b>minic</b> incluido ya)
* Editor de texto estilo nano.

# Por hacer
* Soporte de video para salida VGA.
* Soporte de audio.
* Soporte para módulos LoRa y W5500 (redes).
* Optimizar el intérprete para que ocupe la mínima cantidad de memoria posible.
* Editor de texto estilo nano.

# Capturas
<img width="1230" height="475" alt="imagen" src="https://github.com/user-attachments/assets/4b38bda7-7d51-419f-81a4-d154aa38cd6c" />

<img width="635" height="457" alt="imagen" src="https://github.com/user-attachments/assets/96dd2a1a-103e-4c40-99a2-31a002685d13" />

<img width="847" height="443" alt="imagen" src="https://github.com/user-attachments/assets/ea468081-5746-4f0a-8b2e-685259a344cf" />

### Terminal Web
<img width="751" height="801" alt="imagen" src="https://github.com/user-attachments/assets/903cf94d-598b-4e70-bc5c-2877cea34a2a" />
