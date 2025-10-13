#include <stdio.h>
#include "mongoose.h"
#include <time.h>
#include <string.h>

#define HTTP_PORT "8000"
#define LISTEN_ADDRESS "http://0.0.0.0:" HTTP_PORT

// Uncomment for embedded systems with limited resources
#define EMBEDDED_MODE

unsigned int counter = 0;
time_t start_time;

// Simplified CSS for embedded systems
static const char* css_styles = 
  "body{font-family:Arial;margin:20px;background:#2c3e50;color:white;}"
  ".container{max-width:600px;margin:0 auto;padding:20px;}"
  "h1{text-align:center;color:#3498db;}"
  ".card{background:#34495e;padding:20px;margin:20px 0;border-radius:5px;}"
  ".counter{font-size:2em;text-align:center;color:#f39c12;}"
  ".button{background:#27ae60;color:white;border:none;padding:10px 20px;margin:5px;cursor:pointer;}"
  ".nav{text-align:center;margin:20px 0;}"
  ".nav a{color:#3498db;text-decoration:none;margin:0 15px;}"
  ".status{text-align:center;}"
  "#messages{background:#2c3e50;padding:10px;height:150px;overflow-y:auto;border:1px solid #555;}";

static void serve_home_page(struct mg_connection *c) {
#ifdef EMBEDDED_MODE
  time_t uptime = time(NULL) - start_time;
  mg_http_reply(c, 200, "Content-Type: text/html\r\n",
    "<html><head><title>OCRE Server</title>"
    "<style>"
    "body{margin:20px;font-family:Arial;background:#f8f9fa;color:#333;}"
    "h1{color:#2c3e50;text-align:center;}"
    "button{padding:8px 15px;background:#007bff;color:white;border:none;border-radius:3px;margin:5px;}"
    "a{color:#007bff;text-decoration:none;margin:0 10px;}"
    "</style>"
    "</head><body>"
    "<h1>OCRE Embedded Server</h1>"
    "<p>Counter: %u</p>"
    "<p>Uptime: %ld seconds</p>"
    "<p><a href='/status'>Status</a> | <a href='/api/counter'>API</a> | <a href='/websocket'>WebSocket</a></p>"
    "<form method='POST' action='/increment' style='display:inline;'><button>+</button></form>"
    "<form method='POST' action='/reset' style='display:inline;'><button>Reset</button></form>"
    "</body></html>", counter, (long)uptime);
#else
  time_t uptime = time(NULL) - start_time;
  mg_http_reply(c, 200, "Content-Type: text/html\r\n",
    "<!DOCTYPE html>"
    "<html lang='en'>"
    "<head>"
      "<meta charset='UTF-8'>"
      "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
      "<title>OCRE Embedded Web Server</title>"
      "<style>%s</style>"
    "</head>"
    "<body>"
      "<div class='container'>"
        "<h1>&#128640; OCRE Embedded Web Server</h1>"
        
        "<div class='nav'>"
          "<a href='/'>Home</a>"
          "<a href='/status'>Status</a>"
          "<a href='/websocket'>WebSocket Demo</a>"
          "<a href='/api/counter'>Counter API</a>"
          "<a href='/api/status'>Status API</a>"
        "</div>"
        
        "<div class='card'>"
          "<h2>&#128202; Live Counter</h2>"
          "<div class='counter' id='counter'>%u</div>"
          "<div style='text-align: center;'>"
            "<button class='button' onclick='updateCounter(1)'>&#10133; Increment</button>"
            "<button class='button' onclick='updateCounter(-1)'>&#10134; Decrement</button>"
            "<button class='button' onclick='updateCounter(0)'>&#128260; Reset</button>"
          "</div>"
        "</div>"
        
        "<div class='status'>"
          "<div class='status-item'>"
            "<h3>&#9200;&#65039; Uptime</h3>"
            "<div id='uptime'>%ld seconds</div>"
          "</div>"
          "<div class='status-item'>"
            "<h3>&#127760; Server Port</h3>"
            "<div>%s</div>"
          "</div>"
          "<div class='status-item'>"
            "<h3>&#128279; WebSocket</h3>"
            "<div id='ws-status'>Disconnected</div>"
          "</div>"
        "</div>"
      "</div>"
      
      "<script>"
        "let ws = null; let reconnectInterval = null;"
        "function connectWebSocket() {"
          "ws = new WebSocket('ws://' + window.location.host + '/ws');"
          "ws.onopen = function() { document.getElementById('ws-status').textContent = 'Connected'; };"
          "ws.onclose = function() { document.getElementById('ws-status').textContent = 'Disconnected'; "
                                   "reconnectInterval = setTimeout(connectWebSocket, 3000); };"
          "ws.onmessage = function(event) { "
            "try { let data = JSON.parse(event.data); "
                  "if(data.counter !== undefined) document.getElementById('counter').textContent = data.counter; } "
            "catch(e) {} };"
        "}"
        "function updateCounter(action) {"
          "fetch('/api/counter', { method: 'POST', "
                                  "headers: {'Content-Type': 'application/json'}, "
                                  "body: JSON.stringify({action: action}) })"
            ".then(r => r.json()).then(data => document.getElementById('counter').textContent = data.counter);"
        "}"
        "function updateUptime() {"
          "fetch('/api/status')"
            ".then(r => r.json())"
            ".then(data => {"
              "document.getElementById('uptime').textContent = data.uptime + ' seconds';"
            "})"
            ".catch(e => {});" // Simplified error handling
        "}"
        "try { connectWebSocket(); } catch(e) {}" // Graceful WebSocket failure
        "updateUptime(); setInterval(updateUptime, 5000);" // Update every 5 seconds instead of 1
      "</script>"
    "</body>"
    "</html>", css_styles, counter, uptime, HTTP_PORT);
#endif
}

static void serve_status_page(struct mg_connection *c) {
  time_t uptime = time(NULL) - start_time;
#ifdef EMBEDDED_MODE
  mg_http_reply(c, 200, "Content-Type: text/html\r\n",
    "<html><head><title>Status</title>"
    "<style>"
    "body{margin:20px;font-family:Arial;background:#f8f9fa;color:#333;}"
    "h1{color:#28a745;text-align:center;}"
    "a{color:#007bff;text-decoration:none;}"
    "</style>"
    "</head><body>"
    "<h1>System Status</h1>"
    "<p>Uptime: %ld seconds</p>"
    "<p>Counter: %u</p>"
    "<p>Port: %s</p>"
    "<p><a href='/'>Back</a></p>"
    "</body></html>", (long)uptime, counter, HTTP_PORT);
#else
  mg_http_reply(c, 200, "Content-Type: text/html\r\n",
    "<!DOCTYPE html>"
    "<html><head><title>System Status</title><style>%s</style></head>"
    "<body><div class='container'>"
    "<h1>&#128200; System Status</h1>"
    "<div class='nav'><a href='/'>&larr; Back to Home</a></div>"
    "<div class='card'>"
      "<h2>System Information</h2>"
      "<p><strong>Uptime:</strong> %ld seconds</p>"
      "<p><strong>Counter Value:</strong> %u</p>"
      "<p><strong>Server Port:</strong> %s</p>"
      "<p><strong>Build Time:</strong> %s %s</p>"
    "</div></div></body></html>", 
    css_styles, (long)uptime, counter, HTTP_PORT, __DATE__, __TIME__);
#endif
}

static void serve_websocket_demo(struct mg_connection *c) {
  mg_http_reply(c, 200, "Content-Type: text/html\r\n",
    "<!DOCTYPE html>"
    "<html><head><title>WebSocket Demo</title><style>%s</style></head>"
    "<body><div class='container'>"
    "<h1>&#128172; WebSocket Demo</h1>"
    "<div class='nav'><a href='/'>&larr; Back to Home</a></div>"
    "<div class='card'>"
      "<div id='messages'></div>"
      "<input type='text' id='messageInput' placeholder='Type a message...' style='width: 70%%; padding: 10px;'>"
      "<button class='button' onclick='sendMessage()'>Send</button>"
    "</div></div>"
    "<script>"
      "let ws = new WebSocket('ws://' + window.location.host + '/ws');"
      "let messages = document.getElementById('messages');"
      "ws.onopen = function() { messages.innerHTML += '<div><strong>Connected to WebSocket!</strong></div>'; };"
      "ws.onclose = function() { messages.innerHTML += '<div><strong>WebSocket disconnected.</strong></div>'; };"
      "ws.onerror = function(error) { messages.innerHTML += '<div><strong>WebSocket error: ' + error + '</strong></div>'; };"
      "ws.onmessage = function(event) {"
        "messages.innerHTML += '<div>Echo: ' + event.data + '</div>';"
        "messages.scrollTop = messages.scrollHeight;"
      "};"
      "function sendMessage() {"
        "let input = document.getElementById('messageInput');"
        "if(input.value) { ws.send(input.value); input.value = ''; }"
      "}"
      "document.getElementById('messageInput').addEventListener('keypress', function(e) {"
        "if(e.key === 'Enter') sendMessage();"
      "});"
    "</script></body></html>", css_styles);
}

static void fn(struct mg_connection *c, int ev, void *ev_data) {
  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    
    if (mg_match(hm->uri, mg_str("/"), NULL)) {
#ifndef EMBEDDED_MODE
      counter++; // Only increment on home page visit in desktop mode
#endif
      serve_home_page(c);
    } else if (mg_match(hm->uri, mg_str("/status"), NULL)) {
      serve_status_page(c);
#ifdef EMBEDDED_MODE
    } else if (mg_match(hm->uri, mg_str("/increment"), NULL)) {
      counter++;
      mg_http_reply(c, 302, "Location: /\r\n", "");
    } else if (mg_match(hm->uri, mg_str("/reset"), NULL)) {
      counter = 0;
      mg_http_reply(c, 302, "Location: /\r\n", "");
    } else if (mg_match(hm->uri, mg_str("/websocket"), NULL)) {
      mg_http_reply(c, 200, "Content-Type: text/html\r\n",
        "<html><head><title>WebSocket</title></head><body>"
        "<h1>WebSocket Test</h1>"
        "<p>Status: <span id='status'>Connecting...</span></p>"
        "<input type='text' id='msg'>"
        "<button onclick='send()'>Send</button>"
        "<div id='log'></div>"
        "<script>"
        "var ws = new WebSocket('ws://' + location.host + '/ws');"
        "ws.onopen = function() { document.getElementById('status').innerHTML = 'Connected'; };"
        "ws.onclose = function() { document.getElementById('status').innerHTML = 'Disconnected'; };"
        "ws.onmessage = function(e) { document.getElementById('log').innerHTML += '<div>' + e.data + '</div>'; };"
        "function send() { var msg = document.getElementById('msg'); if(ws.readyState === 1 && msg.value) { ws.send(msg.value); msg.value = ''; } }"
        "</script></body></html>");
    } else if (mg_match(hm->uri, mg_str("/ws"), NULL)) {
      // Simple WebSocket upgrade for embedded
      mg_ws_upgrade(c, hm, NULL);
    } else if (mg_match(hm->uri, mg_str("/api/counter"), NULL)) {
      time_t current_uptime = time(NULL) - start_time;
      mg_http_reply(c, 200, "Content-Type: application/json\r\n", 
                   "{\"counter\": %u, \"uptime\": %ld}", counter, (long)current_uptime);
#else
    } else if (mg_match(hm->uri, mg_str("/websocket"), NULL)) {
      serve_websocket_demo(c);
    } else if (mg_match(hm->uri, mg_str("/ws"), NULL)) {
      // Upgrade HTTP to WebSocket
      mg_ws_upgrade(c, hm, NULL);
    } else if (mg_match(hm->uri, mg_str("/api/counter"), NULL)) {
      time_t current_uptime = time(NULL) - start_time;
      if (mg_strcmp(hm->method, mg_str("GET")) == 0) {
        mg_http_reply(c, 200, "Content-Type: application/json\r\n", 
                     "{\"counter\": %u, \"uptime\": %ld}", counter, (long)current_uptime);
      } else if (mg_strcmp(hm->method, mg_str("POST")) == 0) {
        // Parse JSON body for counter operations
        if (hm->body.len > 0) {
          if (strstr(hm->body.buf, "\"action\":1")) counter++;
          else if (strstr(hm->body.buf, "\"action\":-1") && counter > 0) counter--;
          else if (strstr(hm->body.buf, "\"action\":0")) counter = 0;
        }
        current_uptime = time(NULL) - start_time; // Recalculate after potential delay
        mg_http_reply(c, 200, "Content-Type: application/json\r\n", 
                     "{\"counter\": %u, \"uptime\": %ld}", counter, (long)current_uptime);
      }
    } else if (mg_match(hm->uri, mg_str("/api/status"), NULL)) {
      // Dedicated status endpoint for real-time data
      time_t current_uptime = time(NULL) - start_time;
      mg_http_reply(c, 200, "Content-Type: application/json\r\n", 
                   "{\"counter\": %u, \"uptime\": %ld, \"port\": \"%s\", \"start_time\": %ld}", 
                   counter, (long)current_uptime, HTTP_PORT, (long)start_time);
#endif
    } else {
      mg_http_reply(c, 404, "Content-Type: text/html\r\n", 
                   "<html><body><h1>404 - Page Not Found</h1><a href='/'>Go Home</a></body></html>");
    }
  } else if (ev == MG_EV_WS_MSG) {
    struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
    // Simple echo for both embedded and desktop modes
    mg_ws_send(c, wm->data.buf, wm->data.len, WEBSOCKET_OP_TEXT);
  }
}

int main(void) {
  start_time = time(NULL);                     // Record start time
  mg_log_set(MG_LL_ERROR);                     // Set log level
  setvbuf(stdout, NULL, _IONBF, 0);            // Disable stdout buffering
  struct mg_mgr mgr;                           // Event manager
  mg_mgr_init(&mgr);                           // Initialize event manager
  mg_http_listen(&mgr, LISTEN_ADDRESS, fn, NULL); // Create HTTP listener

  printf("\n>> ===============================================\n");
#ifdef EMBEDDED_MODE
  printf("    OCRE Embedded Web Server - Embedded Mode\n");
#else
  printf("    OCRE Embedded Web Server - Enhanced Mode\n");
#endif
  printf("=============================================== <<\n");
  printf("[*] Server Status: ONLINE\n");
  printf("[*] Listening on port: %s\n", HTTP_PORT);
  printf("[*] Started: %s", ctime(&start_time));
#ifdef EMBEDDED_MODE
  printf("[*] Mode: EMBEDDED (lightweight)\n");
#else
  printf("[*] Mode: ENHANCED (full features)\n");
#endif
  printf("===============================================\n");
  printf("[+] Available endpoints:\n");
  printf("   - http://<IP>:%s/         - Main page\n", HTTP_PORT);
  printf("   - http://<IP>:%s/status   - System status\n", HTTP_PORT);
  printf("   - http://<IP>:%s/websocket - WebSocket demo\n", HTTP_PORT);
  printf("   - http://<IP>:%s/api/counter - Counter JSON API\n", HTTP_PORT);
#ifdef EMBEDDED_MODE
  printf("   - http://<IP>:%s/increment - Increment counter\n", HTTP_PORT);
  printf("   - http://<IP>:%s/reset     - Reset counter\n", HTTP_PORT);
#else
  printf("   - http://<IP>:%s/api/status  - Status JSON API\n", HTTP_PORT);
#endif
  printf("===============================================\n");
  printf("[!] Features:\n");
#ifdef EMBEDDED_MODE
  printf("   + Simple HTML interface\n");
  printf("   + Form-based interactions\n");
  printf("   + Basic WebSocket support\n");
  printf("   + Minimal resource usage\n");
#else
  printf("   + Responsive web interface\n");
  printf("   + Real-time WebSocket communication\n");
  printf("   + RESTful API endpoints\n");
  printf("   + Interactive counter controls\n");
#endif
  printf("===============================================\n");
  fflush(stdout);

  for (;;) mg_mgr_poll(&mgr, 1000);           // Infinite event loop
  mg_mgr_free(&mgr);
  return 0;
}
