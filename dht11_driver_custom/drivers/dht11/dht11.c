#define DT_DRV_COMPAT azuch_dht11

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
#include <string.h>

#include "dht11.h"

LOG_MODULE_REGISTER(AZUCH_DHT11, LOG_LEVEL_INF);

static int8_t dht_measure_signal_duration(const struct device *dev, bool active) {
	const struct dht_config *cfg = dev->config;
	uint32_t elapsed_cycles;
	uint32_t max_wait_cycles = (uint32_t)(
			(uint64_t)DHT_SIGNAL_MAX_WAIT_DURATION *
			(uint64_t)sys_clock_hw_cycles_per_sec() /
			(uint64_t)USEC_PER_SEC
			);
	uint32_t start_cycles = k_cycle_get_32();
	int rc;

	do {
		rc = gpio_pin_get_dt(&cfg->dio_gpio);
		elapsed_cycles = k_cycle_get_32() - start_cycles;

		if ((rc < 0) || (elapsed_cycles > max_wait_cycles)) {
			return -1;
		}
	} while ((bool)rc == active);

	return (uint64_t)elapsed_cycles *
		(uint64_t)USEC_PER_SEC /
		(uint64_t)sys_clock_hw_cycles_per_sec();
}

static int dht_sample_fetch(const struct device *dev, enum sensor_channel chan) {
	struct dht_data *drv_data = dev->data;
	const struct dht_config *cfg = dev->config;
	int ret = 0;
	int8_t signal_duration[DHT_DATA_BITS_NUM];
	int8_t max_duration, min_duration, avg_duration;
	uint8_t buf[5];
	unsigned int i,j;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

#if defined(CONFIG_DHT_LOCK_IRQS)
	int lock = irq_lock();
#endif
	//Send first start signal: LOW
	gpio_pin_set_dt(&cfg->dio_gpio, true);
	//Wait for a specific(i.e set in the datasheet) time
	k_busy_wait(DHT_START_SIGNAL_DURATION);
	//Send next start signal: HIGH
	gpio_pin_set_dt(&cfg->dio_gpio, false);
	//Config the pin to become a input
	gpio_pin_configure_dt(&cfg->dio_gpio, GPIO_INPUT);
	//Now we will check to see if the pin is HIGH now
	
	//If this check fail/==-1; this mean the HIGH is exceed the max wait duration -> meaning the sensor not response or not pull the line to LOW
	if (dht_measure_signal_duration(dev, false) == -1) {
		ret = -EIO;
		goto cleanup;
	}

	//The sensor is pull the line to LOW; we will check if it push it to HIGH within the max duration; if not; the sensor may not working properly
	if (dht_measure_signal_duration(dev, true) == -1) {
		ret = -EIO;
		goto cleanup;
	}

	//Now the final step in the start protocol, the sensor pull the line to HIGH
	if (dht_measure_signal_duration(dev, false) == -1) {
		ret = -EIO;
		goto cleanup;
	}

	//The start protocol is now completed.
	//Next we will loop for 40 bits/5 bytes of the sensor data
	//Each bit will has a fixed LOW and a variant HIGH;
	//A below avg duration mean 0; opposite is 1
	for (i = 0U; i < DHT_DATA_BITS_NUM; i++) {
		if (dht_measure_signal_duration(dev, true) == -1) {
			ret = -EIO;
			goto cleanup;
		}

		signal_duration[i] = dht_measure_signal_duration(dev,false);
		if (signal_duration[i] == -1) {
			ret = -EIO;
			goto cleanup;
		}
	}

	//Now we got all the duration; but those may not same for 0s and 1s
	//We need to calculate the average;
	//So that if the duration is below; it is a 0, else a 1
	min_duration = signal_duration[0];
	max_duration = signal_duration[0];
	for (i = 0U; i < DHT_DATA_BITS_NUM; i++) {
		if (min_duration > signal_duration[i]) {
			min_duration = signal_duration[i];
		}
		if (max_duration < signal_duration[i]) {
			max_duration = signal_duration[i];
		}
	}
	avg_duration = ((int16_t)min_duration + (int16_t)max_duration) / 2;

	j = 0U;
	(void)memset(buf, 0, sizeof(buf));
	for(i = 0U; i < DHT_DATA_BITS_NUM; i++) {
		if (signal_duration[i] >= avg_duration) {
			buf[j] = (buf[j] << 1) | 1;
		} else {
			buf[j] = (buf[j] << 1);
		}
		//this check if all 8 bits are compared, we will switch to next byte
		if ((i % 8) == 7) {
			j++;
		}
	}
	//Verify checksum, because of the maximum value of check sum is 0xFF; and the sum of other bytes may exceed 0xff; so we will just take the last 8 bits of the sum. For example: if the sum is 0x3FC; we will just take 0xFC; to do that we will & 0xFF (0x0FF).
	if (((buf[0] + buf[1] + buf[2] + buf[3]) & 0xFF) != buf[4]) {
		LOG_DBG("Invalid checksum");
		ret = -EIO;
	} else {
		memcpy(drv_data->sample, buf, 5);
	}
cleanup:
#if defined(CONFIG_DHT_LOCK_IRQS)
	irq_unlock(lock);
#endif
	//Done reading one sample; set the pin to ouput inactive - for denided a floating point of the line; and reduce the noise which may interfere the reading and reduce power consumption; of course when it is inactive; the pull up resitor will pull it to HIGH; which make sense for the next reading
	gpio_pin_configure_dt(&cfg->dio_gpio, GPIO_OUTPUT_INACTIVE);
	return ret;
}

static int dht_channel_get(const struct device *dev,
			enum sensor_channel chan,
			struct sensor_value *val) {
	struct dht_data *drv_data = dev->data;
	const struct dht_config *cfg = dev->config;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_AMBIENT_TEMP ||
			chan == SENSOR_CHAN_HUMIDITY);
	if (cfg->is_dht22) {
		int16_t raw_val, sign;
                if (chan == SENSOR_CHAN_HUMIDITY) {
                        raw_val = (drv_data->sample[0] << 8)
                                + drv_data->sample[1];
                        val->val1 = raw_val / 10;
                        val->val2 = (raw_val % 10) * 100000;
                } else if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
                        raw_val = (drv_data->sample[2] << 8)
                                + drv_data->sample[3];

                        sign = raw_val & 0x8000;
                        raw_val = raw_val & ~0x8000;

                        val->val1 = raw_val / 10;
                        val->val2 = (raw_val % 10) * 100000;

                        /* handle negative value */
                        if (sign) {
                                val->val1 = -val->val1;
                                val->val2 = -val->val2;
                        }
                } else {
                        return -ENOTSUP;
                }
	} else {
		if (chan == SENSOR_CHAN_HUMIDITY) {
			val->val1 = drv_data->sample[0];
			val->val2 = 0;
		} else if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
			val->val1 = drv_data->sample[2];
			val->val2 = 0;
		} else {
			return -ENOTSUP;
		}
	}

	return 0;
}

static DEVICE_API(sensor, dht_api) = {
	.sample_fetch = &dht_sample_fetch,
	.channel_get = &dht_channel_get,
};

static int dht_init(const struct device *dev) {
	int rc = 0;
	const struct dht_config *cfg = dev->config;

	if (!gpio_is_ready_dt(&cfg->dio_gpio)) {
		LOG_ERR("GPIO device is not ready");
		return -ENODEV;
	}

	rc = gpio_pin_configure_dt(&cfg->dio_gpio, GPIO_OUTPUT_INACTIVE);
	return rc;
}

#define DHT_DEFINE(inst)							\
	static struct dht_data dht_data_##inst;					\
	static const struct dht_config dht_config_##inst = {			\
		.dio_gpio = GPIO_DT_SPEC_INST_GET(inst, dio_gpios),		\
		.is_dht22 = DT_INST_PROP(inst, dht22),				\
	};									\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, &dht_init, NULL,			\
				&dht_data_##inst, &dht_config_##inst,		\
				POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,	\
				&dht_api);					\

DT_INST_FOREACH_STATUS_OKAY(DHT_DEFINE)

