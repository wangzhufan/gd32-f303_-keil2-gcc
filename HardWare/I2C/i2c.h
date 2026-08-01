

#ifndef GD32F303_I2C_H
#define GD32F303_I2C_H

#include "gd32f30x.h"
#include <stdbool.h>

#define I2C0_SPEED			400000

#undef GENMASK
#define BITS_PER_LONG 		32
#define GENMASK(h, l) \
        (((~0UL) - (1UL << (l)) + 1) & (~0UL >> (BITS_PER_LONG - 1 - (h))))

#undef BIT 
#define BIT(bit)			(unsigned int) (1 << (bit))

void i2c_config(uint32_t clkspeed);

void i2c_init(uint32_t clkspeed);

int32_t i2c_buffer_read(uint8_t device_address,uint8_t read_address, uint8_t* p_buffer, uint16_t number_of_byte, uint16_t timeout);

int i2c_buffer_write(uint8_t device_address,uint8_t reg, uint8_t* p_buffer, uint8_t number_of_byte,uint16_t timeout, bool is_stop);

int i2c_buffer_write_25720(uint8_t device_address,uint8_t reg, uint8_t* p_buffer, uint8_t number_of_byte,uint16_t timeout, bool is_stop);

int sgm_i2c_read_byte(uint8_t reg, uint8_t *val);

int sgm_i2c_write_byte(uint8_t reg, uint8_t val);

int sgm_update_bits(uint8_t reg,uint8_t mask, uint8_t val);

int sgm_update_bits_crc(uint8_t reg,uint8_t mask, uint8_t *val);

int sgm_update_16bits(uint8_t reg,uint16_t mask, uint16_t val);

int sgm_i2c_read_block_data(uint8_t reg, uint8_t *data,uint8_t len);

int sgm_i2c_write_block_data(uint8_t reg, uint8_t *data,uint8_t len);

void swire_init(void);

void set_vo1_vo2_output_volt(uint8_t pulse_count);

void set_vo3_output_volt(uint8_t pulse_count);

void set_vo4_output_volt(uint8_t pulse_count);

void set_vo4_output_volt2(uint8_t pulse_count);

#endif

