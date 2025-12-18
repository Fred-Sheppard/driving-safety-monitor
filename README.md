# Driving Safety Monitor

## Contributing

1. Clone the repo
2. Run the server found in `docs/mqtt-sqlite-bridge` using `node server.js`. Make sure to run `npm install` beforehand.
3. Copy config_local_example.h to `config_local.h`
4. Enter your WiFi details in `config_local.h`
5. Enter server details in `config.h`.
6. Run `pio run` to build the project
7. Run `pio run --target upload --environment esp32dev` to flash the device
