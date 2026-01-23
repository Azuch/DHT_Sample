#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

// Register a log module named "main"
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define DHT0_NODE DT_ALIAS(dht11)
static const struct device *dev = DEVICE_DT_GET(DHT0_NODE);

int main(void)
{
	if (!device_is_ready(dev)) {
		LOG_ERR("DHT is not ready");
		return 0;
	}

	while (1) {
		struct sensor_value temp, hum;

		if (sensor_sample_fetch(dev) < 0) {
			LOG_ERR("sensor_sample_fetch failed");
			goto sleep;
		}

		if (sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp) < 0) {
			LOG_ERR("Failed to read temperature");
			goto sleep;
		}

		if (sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &hum) < 0) {
			LOG_ERR("Failed to read humidity");
			goto sleep;
		}

		LOG_INF("Temp: %d.%06d C; Humidity: %d.%06d%%",
		        temp.val1, temp.val2, hum.val1, hum.val2);

	sleep:
		k_sleep(K_SECONDS(2));
	}
}

