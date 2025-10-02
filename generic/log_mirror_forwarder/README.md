# Log Mirror and Forwarder
This example tails a provided log file and mirrors any updates to it to a webpage, and also forwards
new log lines to a provided MQTT broker.

The below instructions assume you are running on a Linux host, but it will work on an embedded device
if you wish to run it there. Note, it does use the websocket library in Mongoose which is fairly
large and requires a reasonable amount of RAM, so your mileage may vary if running on a non-Linux
target device.

## Dependencies
### Mosquitto (MQTT)
1. Install:
    ```bash
    sudo apt install mosquitto
    ```
2. Add to `/etc/mosquitto/mosquitto.conf`:
    ```
    # Using this instead of 0.0.0.0 since we don't have firewall rules for blocking external connections
    listener 1883 127.0.0.1       # only loopback

    # Testing mode: no auth
    allow_anonymous true

    log_type all
    ```
3. Restart
    ```bash
    sudo service mosquitto restart
    ```

## Container Setup
1. Make sure directories exist in the root dir of where the Atym runtime exists:
    - `ocre/cfs/log`
    - `ocre/cfs/web`
2. Copy `index.html` from repository into `ocre/cfs/web/index.html` (we do not yet have
   the ability to send down files alongside the application to an end device, so this step
   is unfortunately manual)
3. Mirror syslog to the log directory:
    ```bash
    sudo journalctl -f -o short-iso > ocre/cfs/log/syslog
    ```
4. Deploy the container using `atym run`
5. Open webpage at port 8000
6. Connect to MQTT broker in web interface using IP and port

## Test
(Optional) Subscribe to MQTT:
    ```
    mosquitto_sub -t "demo/syslog/lines"
    ```

To test logging, either of the following commands will output logs to syslog:
    ```
    sudo systemctl restart avahi-daemon
    ```

    ```
    sudo apt update
    ```
