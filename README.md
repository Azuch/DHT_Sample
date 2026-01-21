# DHT11 Sensor Sample for ESP32-S3

## Build Command

The command to make it work is:

```
west build -b esp32s3_devkitc/esp32s3/procpu -p always -- -DDTC_OVERLAY_FILE=boards/esp32s3_devkitc.overlay
```

### Command Options Explained:
- `-b` is for the specific board
- `-p` is pristine which makes a new build (it may not be efficient when you just need to change some minor things in .c files; so omit it in those cases)
- `--` means done with the west parts
- `-DDTC_OVERLAY_FILE` is a flag for CMake

For other cases, you may choose some specific .conf files like secrets.prj, using `-DCONF_FILE="prj;secrets.conf;tls.conf"`.

## Common Issues

### Board Overlay File
The error in this project occurs when I keep forgetting to include my board's .overlay file, so this keeps yelling at the DEVICE_DT_GET function.

### DHT Driver Behavior
The driver for dht.c is confusing too; you should look at the dht_init() first. It configures to `gpio_pin_configure_dt(&cfg->dio_gpio, GPIO_OUTPUT_INACTIVE)` so it seems kind of backward; like true is LOW and false is HIGH. But yeah, it saves us some while loop, it runs efficiently, so we will leave it as is.

However, I will try to write my own driver version, which may not be as efficient, but if it works then I will learn a lot.

## ESP32-S3 Ports

There are 2 ports for ESP32-S3; if you're not seeing any log, switch to the other port.
