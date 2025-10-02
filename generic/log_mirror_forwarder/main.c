// main.c – WebSocket push, MQTT-safe publish, 200-line window, status endpoint
// Mongoose 7.x
#include "mongoose.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#define LOG_FILE      "/log/syslog"
#define WEB_ROOT      "/web"
#define DEFAULT_LINES 200
#define MAX_RESPONSE  (512 * 1024)
#define MQTT_HOST_DEFAULT "127.0.0.1"
#define MQTT_PORT_DEFAULT 1883

// ===== In-memory ring (last 200 lines) =====
static char *lines[DEFAULT_LINES];
static int line_count = 0;
pthread_mutex_t line_lock = PTHREAD_MUTEX_INITIALIZER;

// ===== MQTT state =====
static struct mg_connection *mqtt_conn = NULL;
static int mqtt_ready = 0;
static char mqtt_host[128] = MQTT_HOST_DEFAULT;
static int mqtt_port = MQTT_PORT_DEFAULT;

// ===== WebSocket state =====
static int ws_clients = 0;
struct qnode { char *s; size_t n; struct qnode *next; };
static struct { struct qnode *head, *tail; pthread_mutex_t mu; } wsq = {NULL, NULL, PTHREAD_MUTEX_INITIALIZER};

// Forward decls
static void ws_enqueue_line(const char *line, size_t len);
static void ws_broadcast_pending(struct mg_mgr *mgr);

// ===== Utilities =====
static size_t tail_last_n_lines(const char *path, int keep, char *out, size_t cap) {
  int fd = open(path, O_RDONLY);
  if (fd < 0) return 0;
  off_t end = lseek(fd, 0, SEEK_END);
  if (end < 0) { close(fd); return 0; }
  const size_t BUFSZ = 4096;
  char buf[BUFSZ];
  off_t pos = end;
  int nl = 0;

  while (pos > 0 && nl <= keep) {
    size_t chunk = (pos >= (off_t) BUFSZ) ? BUFSZ : (size_t) pos;
    pos -= chunk;
    if (lseek(fd, pos, SEEK_SET) < 0) break;
    ssize_t r = read(fd, buf, chunk);
    if (r <= 0) break;
    for (ssize_t i = r - 1; i >= 0; --i) {
      if (buf[i] == '\n' && ++nl > keep) { pos += i + 1; goto found; }
    }
  }
found:
  if (pos < 0) pos = 0;
  if (lseek(fd, pos, SEEK_SET) < 0) { close(fd); return 0; }
  size_t total = 0;
  for (;;) {
    ssize_t r = read(fd, buf, BUFSZ);
    if (r <= 0) break;
    size_t to_copy = (size_t) r;
    if (total + to_copy > cap) to_copy = cap - total;
    memcpy(out + total, buf, to_copy);
    total += to_copy;
    if (total >= cap) break;
  }
  close(fd);
  return total;
}

// Store a line into the in-memory ring ONLY (no MQTT/WS side-effects)
static void store_line_only(const char *line, size_t len) {
  pthread_mutex_lock(&line_lock);
  if (line_count == DEFAULT_LINES) {
    free(lines[0]);
    memmove(lines, lines + 1, sizeof(lines[0]) * (DEFAULT_LINES - 1));
    line_count--;
  }
  lines[line_count++] = strndup(line, len);
  pthread_mutex_unlock(&line_lock);
}

// Publish a line to MQTT if connected
static void mqtt_publish_line(const char *line, size_t len) {
  if (mqtt_ready && mqtt_conn) {
    struct mg_mqtt_opts pub = {0};
    pub.topic = mg_str("demo/syslog/lines");
    pub.message = mg_str_n(line, len);
    mg_mqtt_pub(mqtt_conn, &pub);
  }
}

// Process a NEW line: store + publish (if ready) + enqueue for WS
static void process_new_line(const char *line, size_t len) {
  store_line_only(line, len);
  mqtt_publish_line(line, len);
  ws_enqueue_line(line, len);
}

static void preload_last_lines() {
  static char buf[MAX_RESPONSE];
  size_t nbytes = tail_last_n_lines(LOG_FILE, DEFAULT_LINES, buf, sizeof(buf));
  char *p = buf, *line = p, *end = buf + nbytes;
  for (; p < end; p++) {
    if (*p == '\n') {
      size_t len = (size_t) (p - line + 1);
      store_line_only(line, len);     // PRELOAD: store only, no MQTT/WS
      line = p + 1;
    }
  }
}

// ===== MQTT =====
static void mqtt_handler(struct mg_connection *c, int ev, void *ev_data) {
  (void) ev_data;
  switch (ev) {
    case MG_EV_OPEN:
      mqtt_ready = 0;
      break;
    case MG_EV_MQTT_OPEN:
      mqtt_ready = 1;
      break;
    case MG_EV_ERROR:
    case MG_EV_CLOSE:
      mqtt_ready = 0;
      break;
    default: break;
  }
}

static void mqtt_connect(struct mg_mgr *mgr) {
  char url[160];
  snprintf(url, sizeof(url), "mqtt://%s:%d", mqtt_host, mqtt_port);
  struct mg_mqtt_opts opts = {0};
  mqtt_conn = mg_mqtt_connect(mgr, url, &opts, mqtt_handler, NULL);
}

// ===== HTTP helpers =====
static void serve_config(struct mg_connection *c, struct mg_http_message *hm, struct mg_mgr *mgr) {
  char hostbuf[128], portbuf[16];
  int okh = mg_http_get_var(&hm->body, "host", hostbuf, sizeof(hostbuf));
  int okp = mg_http_get_var(&hm->body, "port", portbuf, sizeof(portbuf));
  if (okh > 0 && okp > 0) {
    strncpy(mqtt_host, hostbuf, sizeof(mqtt_host) - 1);
    mqtt_host[sizeof(mqtt_host) - 1] = 0;
    mqtt_port = atoi(portbuf);
    mqtt_connect(mgr);
    mg_http_reply(c, 200, "Content-Type: application/json\r\n", "{\"ok\":true}\n");
  } else {
    mg_http_reply(c, 400, "Content-Type: application/json\r\n", "{\"ok\":false,\"error\":\"missing host or port\"}\n");
  }
}

static void serve_status(struct mg_connection *c) {
  mg_http_reply(
    c, 200, "Content-Type: application/json\r\nCache-Control: no-store\r\n",
    "{"
      "\"mqtt_host\":\"%s\","
      "\"mqtt_port\":%d,"
      "\"mqtt_ready\":%s,"
      "\"ws_clients\":%d"
    "}\n",
    mqtt_host, mqtt_port, mqtt_ready ? "true" : "false", ws_clients
  );
}

static void serve_log(struct mg_connection *c) {
  // Respond with the last 200 lines (ephemeral window)
  pthread_mutex_lock(&line_lock);
  size_t total = 0;
  for (int i = 0; i < line_count; i++) total += strlen(lines[i]);
  char *resp = (char *) malloc(total + 1);
  if (!resp) { pthread_mutex_unlock(&line_lock); mg_http_reply(c, 500, "", "OOM\n"); return; }
  resp[0] = 0;
  for (int i = 0; i < line_count; i++) strcat(resp, lines[i]);
  pthread_mutex_unlock(&line_lock);
  mg_http_reply(c, 200, "Content-Type: text/plain; charset=utf-8\r\nCache-Control: no-store\r\n", "%s", resp);
  free(resp);
}

static void serve_download(struct mg_connection *c, struct mg_http_message *hm) {
  struct mg_http_serve_opts opts = {
    .extra_headers = "Content-Type: text/plain\r\n"
                     "Content-Disposition: attachment; filename=\"syslog\"\r\n"
  };
  mg_http_serve_file(c, hm, LOG_FILE, &opts);
}

// ===== WS queue helpers =====
static void ws_enqueue_line(const char *line, size_t len) {
  struct qnode *nn = (struct qnode *) malloc(sizeof(*nn));
  if (!nn) return;
  nn->s = (char *) malloc(len);
  if (!nn->s) { free(nn); return; }
  memcpy(nn->s, line, len);
  nn->n = len;
  nn->next = NULL;
  pthread_mutex_lock(&wsq.mu);
  if (wsq.tail) { wsq.tail->next = nn; wsq.tail = nn; }
  else { wsq.head = wsq.tail = nn; }
  pthread_mutex_unlock(&wsq.mu);
}

static void ws_broadcast_pending(struct mg_mgr *mgr) {
  for (;;) {
    pthread_mutex_lock(&wsq.mu);
    struct qnode *n = wsq.head;
    if (n) { wsq.head = n->next; if (!wsq.head) wsq.tail = NULL; }
    pthread_mutex_unlock(&wsq.mu);
    if (!n) break;

    for (struct mg_connection *c = mgr->conns; c != NULL; c = c->next) {
      if (c->is_websocket) mg_ws_send(c, n->s, n->n, WEBSOCKET_OP_TEXT);
    }
    free(n->s);
    free(n);
  }
}

// ===== HTTP/WS handler =====
static void ev_handler(struct mg_connection *c, int ev, void *ev_data) {
  switch (ev) {
    case MG_EV_HTTP_MSG: {
      struct mg_http_message *hm = (struct mg_http_message *) ev_data;
      fprintf(stderr, "[HTTP] %.*s %.*s\n",
              (int) hm->method.len, hm->method.buf,
              (int) hm->uri.len, hm->uri.buf);

      if (mg_match(hm->uri, mg_str("/ws"), NULL)) {
        fprintf(stderr, "[WS] Upgrade requested\n");
        mg_ws_upgrade(c, hm, NULL);
        return;
      }
      if (mg_match(hm->uri, mg_str("/log"), NULL)) { serve_log(c); return; }
      if (mg_match(hm->uri, mg_str("/download"), NULL)) { serve_download(c, hm); return; }
      if (mg_match(hm->uri, mg_str("/config"), NULL)) { serve_config(c, hm, c->mgr); return; }
      if (mg_match(hm->uri, mg_str("/status"), NULL)) { serve_status(c); return; }

      struct mg_http_serve_opts opts = {
        .root_dir = WEB_ROOT,
        .extra_headers = "Cache-Control: no-store\r\n"
      };
      mg_http_serve_dir(c, hm, &opts);
      break;
    }
    case MG_EV_WS_OPEN:
      ws_clients++;
      fprintf(stderr, "[WS] Client connected (total %d)\n", ws_clients);

      // Send the backlog immediately
      pthread_mutex_lock(&line_lock);
      for (int i = 0; i < line_count; i++) {
        mg_ws_send(c, lines[i], strlen(lines[i]), WEBSOCKET_OP_TEXT);
      }
      pthread_mutex_unlock(&line_lock);
      break;
    case MG_EV_WS_MSG:
    {
      struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
      fprintf(stderr, "[WS] Message from client: %.*s\n", (int) wm->data.len, wm->data.buf);
      break;
    }
    case MG_EV_CLOSE:
      if (c->is_websocket && ws_clients > 0) {
        ws_clients--;
        fprintf(stderr, "[WS] Client disconnected (total %d)\n", ws_clients);
      }
      break;
    case MG_EV_ERROR:
      fprintf(stderr, "[ERR] Connection error: %s\n", (char *) ev_data);
      break;
  }
}

// ===== Log tail timer =====
static int log_fd = -1;
static size_t linelen = 0;
static char linebuf[4096];

static void log_timer_fn(void *arg) {
  struct mg_mgr *mgr = (struct mg_mgr *) arg;
  if (log_fd < 0) {
    log_fd = open(LOG_FILE, O_RDONLY);
    if (log_fd >= 0) {
      lseek(log_fd, 0, SEEK_END);
      fprintf(stderr, "[LOG] Opened %s for tailing\n", LOG_FILE);
    } else {
      fprintf(stderr, "[LOG] Failed to open %s\n", LOG_FILE);
      return;
    }
  }

  char buf[512];
  ssize_t r = read(log_fd, buf, sizeof(buf));
  if (r > 0) {
    for (ssize_t i = 0; i < r; i++) {
      linebuf[linelen++] = buf[i];
      if (buf[i] == '\n' || linelen == sizeof(linebuf)) {
        if (linebuf[linelen - 1] != '\n') linebuf[linelen - 1] = '\n';
        process_new_line(linebuf, linelen);
        linelen = 0;
      }
    }
  }
  // else nothing new: timer will fire again
}


int main(void) {
  setvbuf(stdout, NULL, _IONBF, 0);
  preload_last_lines();

  struct mg_mgr mgr;
  mg_mgr_init(&mgr);
  const char *addr = "http://0.0.0.0:8000";
  if (!mg_http_listen(&mgr, addr, ev_handler, NULL)) {
    fprintf(stderr, "Failed to listen on %s\n", addr);
    return 1;
  }
  fprintf(stderr, "Serving static from %s and log from %s on %s\n", WEB_ROOT, LOG_FILE, addr);

  // Threads not working for now
  // pthread_t tid;
  // int thr_ret = pthread_create(&tid, NULL, log_tailer, NULL);
  // if(thr_ret) {
  //   printf("Failed to create log tailer thread: %d (%s)\n", thr_ret, strerror(thr_ret));
  //   return 1;
  // }
  // printf("pthread_create returned %d\n", thr_ret);
  // thr_ret = pthread_detach(tid);
  // printf("pthread_detach returned %d\n", thr_ret);

  // New: poll every 200 ms
  mg_timer_add(&mgr, 200, MG_TIMER_REPEAT, log_timer_fn, &mgr);

  for (;;) {
    mg_mgr_poll(&mgr, 100);
    ws_broadcast_pending(&mgr);
  }
  mg_mgr_free(&mgr);
  return 0;
}
