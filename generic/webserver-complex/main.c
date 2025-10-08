#include <stdio.h>
#include "mongoose.h"
#include <time.h>
#include <string.h>

#define HTTP_PORT "8000"
#define LISTEN_ADDRESS "http://0.0.0.0:" HTTP_PORT

unsigned int counter = 0;
time_t start_time;

// CSS styles embedded in C string
static const char* css_styles = 
  "body { font-family: 'Segoe UI', Arial, sans-serif; margin: 0; padding: 20px; "
       "background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; }"
  ".container { max-width: 800px; margin: 0 auto; background: rgba(255,255,255,0.1); "
              "border-radius: 15px; padding: 30px; backdrop-filter: blur(10px); }"
  "h1 { text-align: center; font-size: 2.5em; margin-bottom: 30px; text-shadow: 2px 2px 4px rgba(0,0,0,0.3); }"
  ".card { background: rgba(255,255,255,0.2); border-radius: 10px; padding: 20px; margin: 20px 0; }"
  ".counter { font-size: 3em; text-align: center; color: #FFD700; text-shadow: 2px 2px 4px rgba(0,0,0,0.5); }"
  ".button { background: #4CAF50; color: white; border: none; padding: 10px 20px; "
           "border-radius: 5px; cursor: pointer; margin: 5px; font-size: 16px; }"
  ".button:hover { background: #45a049; }"
  ".status { display: flex; justify-content: space-between; flex-wrap: wrap; }"
  ".status-item { background: rgba(255,255,255,0.1); padding: 15px; border-radius: 8px; "
                "margin: 5px; flex: 1; min-width: 200px; text-align: center; }"
  ".nav { text-align: center; margin: 20px 0; }"
  ".nav a { color: white; text-decoration: none; margin: 0 15px; padding: 10px 20px; "
           "background: rgba(255,255,255,0.2); border-radius: 25px; }"
  ".nav a:hover { background: rgba(255,255,255,0.3); }"
  "#ws-status { color: #FFD700; font-weight: bold; }"
  "#messages { background: rgba(0,0,0,0.3); padding: 15px; border-radius: 8px; "
             "height: 200px; overflow-y: auto; margin: 10px 0; }";

static void serve_home_page(struct mg_connection *c) {
  time_t uptime = time(NULL) - start_time;
  mg_printf(c,
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Transfer-Encoding: chunked\r\n\r\n");
  
  mg_http_printf_chunk(c,
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
            ".catch(e => console.log('Uptime update failed:', e));"
        "}"
        "connectWebSocket();"
        "updateUptime(); setInterval(updateUptime, 1000);" // Update every second
        "setInterval(() => { if(ws && ws.readyState === WebSocket.OPEN) "
                           "ws.send(JSON.stringify({type: 'ping'})); }, 30000);"
      "</script>"
    "</body>"
    "</html>", css_styles, counter, uptime, HTTP_PORT);
  
  mg_http_printf_chunk(c, "");
}

static void serve_status_page(struct mg_connection *c) {
  time_t uptime = time(NULL) - start_time;
  mg_printf(c,
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Transfer-Encoding: chunked\r\n\r\n");
  
  mg_http_printf_chunk(c,
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
  
  mg_http_printf_chunk(c, "");
}

static void serve_websocket_demo(struct mg_connection *c) {
  mg_printf(c,
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Transfer-Encoding: chunked\r\n\r\n");
  
  mg_http_printf_chunk(c,
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
  
  mg_http_printf_chunk(c, "");
}

static void fn(struct mg_connection *c, int ev, void *ev_data) {
  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    
    if (mg_match(hm->uri, mg_str("/"), NULL)) {
      counter++; // Increment on home page visit
      serve_home_page(c);
    } else if (mg_match(hm->uri, mg_str("/status"), NULL)) {
      serve_status_page(c);
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
    } else {
      mg_http_reply(c, 404, "Content-Type: text/html\r\n", 
                   "<html><body><h1>404 - Page Not Found</h1><a href='/'>Go Home</a></body></html>");
    }
  } else if (ev == MG_EV_WS_MSG) {
    struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
    // Echo back the message, or send counter updates
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
  printf("    OCRE Embedded Web Server - Enhanced Demo\n");
  printf("=============================================== <<\n");
  printf("[*] Server Status: ONLINE\n");
  printf("[*] Listening on port: %s\n", HTTP_PORT);
  printf("[*] Started: %s", ctime(&start_time));
  printf("===============================================\n");
  printf("[+] Available endpoints:\n");
  printf("   - http://<IP>:%s/         - Main dashboard\n", HTTP_PORT);
  printf("   - http://<IP>:%s/status   - System status\n", HTTP_PORT);
  printf("   - http://<IP>:%s/websocket - WebSocket demo\n", HTTP_PORT);
  printf("   - http://<IP>:%s/api/counter - Counter JSON API\n", HTTP_PORT);
  printf("   - http://<IP>:%s/api/status  - Status JSON API\n", HTTP_PORT);
  printf("===============================================\n");
  printf("[!] Features:\n");
  printf("   + Responsive web interface\n");
  printf("   + Real-time WebSocket communication\n");
  printf("   + RESTful API endpoints\n");
  printf("   + Interactive counter controls\n");
  printf("===============================================\n");
  fflush(stdout);

  for (;;) mg_mgr_poll(&mgr, 1000);           // Infinite event loop
  mg_mgr_free(&mgr);
  return 0;
}
