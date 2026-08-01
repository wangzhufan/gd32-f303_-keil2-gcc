
#ifndef __AT_CMD_SUPPORT__
#define __AT_CMD_SUPPORT__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

 
#define FALSE					0
#define TRUE					1
#define MAX_AT_INSTRUCTION		80
#define AT_RESPONSE_CON			":"

/*
例如："V1.1.1.R" 分别代表
V1：主版本号，不同硬件板的固件
1：次版本号，有重大修改
1：修订版本号，bug修复或新功能添加
R：代表版本类型，分为release、debug、specification三种，简称R/D/S
*/

/* FW_V1.4.16.R Add MIPI REFF BUS Communication*/
/* FW_V1.5.16.R Add ADC FUNC*/
/* FW_V1.6.0.R SPI AT CMD Unification*/
/* FW_V1.7.0.R Simulate 3.4M high-speed I2C*/

#define FW_VERSION_NUM			"FW_V1.7.3.R"

#define SGM6030_TEST

typedef enum{
	AT_CMD_GVSION = 0,
	AT_CMD_GADDR,
	AT_CMD_SADDR,
	AT_CMD_SSPD,
	AT_CMD_RREG,
	AT_CMD_WREG,
	AT_CMD_DWREG,
    AT_CMD_DRREG,
	AT_CMD_JWREG,
	AT_CMD_CWREG,
	AT_CMD_MWREG,
	AT_CMD_PWREG,
	AT_CMD_PRREG,
	AT_CMD_JRREG,
	AT_CMD_CMWREG,
	AT_CMD_ETEST,
	AT_CMD_TESTRREG,
	AT_CMD_IICM,
	AT_CMD_SWIRE,
	AT_CMD_DEBUG,
	AT_CMD_SPIEN,	
	AT_CMD_SPIT,	

	AT_CMD_WREGX,
	AT_CMD_RREGX,
    AT_CMD_GPIOX,
	AT_CMD_RUNX,
	AT_CMD_GPIOEN,
	AT_CMD_GSWIRE,
	AT_CMD_LEDE,
	AT_CMD_I2CTRANS,
#ifdef SGM6030_TEST
	AT_CMD_STEST,
	AT_CMD_SRUN,
#endif

	AT_CMD_SENDARA,
	AT_CMD_BROADCAST,
	AT_CMD_GPINSTAT,
    AT_CMD_ADCEN,
    AT_CMD_GADC,

    AT_END
}AT_Cmd;


//第一个用来存放AT的指令
typedef struct node{
	AT_Cmd cmd;   //位置根据命令首字母来进行位置区分 B = 2
	uint8_t *at_str;    //存放AT命令
	int (*function)(uint8_t index,uint32_t* param,uint8_t count);  //存放命令所用的函数

}AT_DEAL;


struct test_param{
	uint8_t flag; 
	uint8_t	reg;
	uint8_t	len;
	uint8_t	buff[10];
	uint32_t delay;
};

void uart_buff_parse(uint8_t *str,uint16_t str_len);
uint8_t at_response_no_param(uint8_t index,bool result);
uint8_t at_response_param(uint8_t index,uint16_t *response_buff,bool result);
unsigned char CRC8_Calculate(unsigned char *pData, unsigned char Size);



/****************************************************/
int sgm_upgrade(uint8_t index,uint32_t* param,uint8_t count);

int device_get_addr_index(uint8_t index,uint32_t* param,uint8_t count);
int sgm_i2c_transmit(uint8_t index,uint32_t* param,uint8_t count);
int device_baudrate_setting(uint8_t index,uint32_t* param,uint8_t count);
int get_fw_version(uint8_t index,uint32_t* param,uint8_t count);
int device_addr_setting(uint8_t index,uint32_t* param,uint8_t count);
int device_read_reg(uint8_t index,uint32_t* param,uint8_t count);
int device_write_reg(uint8_t index,uint32_t* param,uint8_t count);
int device_16bit_wreg(uint8_t index,uint32_t* param,uint8_t count);
int device_16bit_rreg(uint8_t index,uint32_t* param,uint8_t count);
int device_jump_wreg(uint8_t index,uint32_t* param,uint8_t count);
int Mult_write_reg(uint8_t index,uint32_t* param,uint8_t count);
int i2c_mode_setting(uint8_t index,uint32_t* param,uint8_t count);
int swire_ctrl_vo(uint8_t index,uint32_t* param,uint8_t count);
int device_debug_setting(uint8_t index,uint32_t* param,uint8_t count);
int device_write_reg_crc(uint8_t index,uint32_t* param,uint8_t count);
int CMult_write_reg(uint8_t index,uint32_t* param,uint8_t count);
int enter_test_mode(uint8_t index,uint32_t* param,uint8_t count);
int device_test_read_reg(uint8_t index,uint32_t* param,uint8_t count);
int page_write_reg(uint8_t index,uint32_t* param,uint8_t count);
int page_read_reg(uint8_t index,uint32_t* param,uint8_t count);
int jump_read_reg(uint8_t index,uint32_t* param,uint8_t count);

int device_wregx_test(uint8_t index,uint32_t* param,uint8_t count);
int device_rregx_test(uint8_t index,uint32_t* param,uint8_t count);
int device_gpio_ctrl_save(uint8_t index,uint32_t* param,uint8_t count);
int device_run_test(uint8_t index,uint32_t* param,uint8_t count);


int gpio_en(uint8_t index,uint32_t* param,uint8_t count);
int gswire_set(uint8_t index,uint32_t* param,uint8_t count);

int sgm_i2c_transmit_ara(uint8_t index,uint32_t* param,uint8_t count);
int sgm_get_pin_stat(uint8_t index,uint32_t* param,uint8_t count);

#endif
