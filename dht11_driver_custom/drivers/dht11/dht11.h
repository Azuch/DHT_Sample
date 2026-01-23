#ifndef ZEPHYR_DRIVERS_SENSOR_AZUCH_DHT11_H
#define ZEPHYR_DRIVERS_SENSOR_AZUCH_DHT11_H

#include <zephyr/device.h>
#include <stdint.h>

#define DHT_START_SIGNAL_DURATION		18000
#define DHT_SIGNAL_MAX_WAIT_DURATION 		100
#define DHT_DATA_BITS_NUM			40

struct dht_data {
	uint8_t sample[5];
};

struct dht_config {
	struct gpio_dt_spec dio_gpio;
	bool is_dht22;
};
#endif
