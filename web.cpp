#include "web.h"
#include "io.h"
#include "shell.h"

WebServer server(80);
String inputBuffer = "";
bool newCommandFromWeb = false;

void initWebServer() {
  // Configura rutas del servidor web
  server.on("/", handleRoot);          // Página principal con terminal
  server.on("/cmd", handleCommand);    // Endpoint POST para enviar comandos
  server.on("/output", handleOutput);  // Endpoint GET para polling de salida
  server.begin();
  Serial.print("Servidor web iniciado en http://");
  Serial.println(WiFi.localIP());
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Pico Shell Remoto</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: monospace; background: #000; color: #0f0; margin: 10px; }
    #output { white-space: pre-wrap; height: 70vh; overflow-y: scroll; border: 1px solid #0f0; padding: 10px; background: #111; }
    #input { width: 100%; padding: 8px; background: #111; color: #0f0; border: 1px solid #0f0; font-family: monospace; }
    button { background: #0f0; color: black; border: none; padding: 8px 16px; cursor: pointer; }
  </style>
</head>
<body>
  <h2>Pico W - Shell Remoto</h2>
  <div id="output"></div>
  <input id="input" type="text" placeholder="Escribe comando y presiona Enter..." autofocus>
  <button onclick="sendCommand()">Enviar</button>
 
  <script>
    const output = document.getElementById('output');
    const input = document.getElementById('input');
   
    input.addEventListener('keydown', function(e) {
      if (e.key === 'Enter') {
        sendCommand();
        e.preventDefault();
      }
    });
   
    function sendCommand() {
      let cmd = input.value.trim();
      if (!cmd) return;
     
      fetch('/cmd', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: 'cmd=' + encodeURIComponent(cmd)
      }).then(() => {
        input.value = '';
      });
    }
   
    function updateOutput() {
      fetch('/output').then(r => r.text()).then(text => {
        if (text) {
          output.innerHTML += text.replace(/\n/g, '<br>') + '<br>';
          output.scrollTop = output.scrollHeight;
        }
      }).catch(e => console.log('Error polling:', e));
    }
   
    setInterval(updateOutput, 600);  // Polling cada 600ms
    updateOutput();                  // Primera carga
  </script>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", html);
}
void handleCommand() {
  if (server.hasArg("cmd")) {
    String cmd = server.arg("cmd");
    cmd.trim();
    if (cmd.length() > 0) {
      // Simulamos ejecución en el loop principal
      // (por seguridad y simplicidad, procesamos en el loop)
      inputBuffer = cmd + "\n";  // Añadimos \n como si fuera enter
      newCommandFromWeb = true;
      server.send(200, "text/plain", "Comando recibido");
    }
  } else {
    server.send(400, "text/plain", "Falta comando");
  }
}
void handleOutput() {
  String response = outputBuffer;
  outputBuffer = "";  // Limpia después de enviar (o puedes mantener un buffer circular)
  server.send(200, "text/plain", response);
}