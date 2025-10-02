mkdir -p fs
iwasm --addr-pool=0.0.0.0/0 --map-dir=/::./fs build/syslog_webserver.wasm