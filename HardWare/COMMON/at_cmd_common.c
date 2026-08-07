#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "i2c.h" 
#include "systick.h"
#include "at_cmd_common.h"
#include "flash.h"


/************************************************************************************/
char g_at_response[50];
int g_sgm_debug_flag = 0;
int g_sgm_test_mode_flag = 0;
extern uint8_t g_sgm_slave_addr;
static uint8_t g_test_count = 0;

extern int sgm_irq_flag;
extern struct test_param s_read_write_buff[20];

/************************************************************************************/

static AT_DEAL g_at_deal[AT_END] = {
{ AT_CMD_GVSION,        (uint8_t*)"GVSION",        get_fw_version                },
{ AT_CMD_GADDR,         (uint8_t*)"GADDR",         device_get_addr_index         },
{ AT_CMD_SADDR,         (uint8_t*)"SADDR",         device_addr_setting           },
{ AT_CMD_SSPD,          (uint8_t*)"SSPD",          device_baudrate_setting       },
{ AT_CMD_I2CTRANS,      (uint8_t*)"I2CTRANS",      sgm_i2c_transmit      		 },
{ AT_CMD_RREG,          (uint8_t*)"RREG",          device_read_reg               },
{ AT_CMD_WREG,          (uint8_t*)"WREG",          device_write_reg              },
{ AT_CMD_DWREG,         (uint8_t*)"DWREG",         device_16bit_wreg             },
{ AT_CMD_DRREG,         (uint8_t*)"DRREG",         device_16bit_rreg             },
{ AT_CMD_JWREG,         (uint8_t*)"JWREG",         device_jump_wreg              },
{ AT_CMD_CWREG,         (uint8_t*)"CWREG",         device_write_reg_crc          },
{ AT_CMD_MWREG,         (uint8_t*)"MWREG",         Mult_write_reg                },
{ AT_CMD_PWREG,         (uint8_t*)"PWREG",         page_write_reg                },
{ AT_CMD_PRREG,         (uint8_t*)"PRREG",         page_read_reg                 },
{ AT_CMD_JRREG,         (uint8_t*)"JRREG",         jump_read_reg                 },
{ AT_CMD_CMWREG,        (uint8_t*)"CMWREG",        CMult_write_reg               },
{ AT_CMD_ETEST,         (uint8_t*)"ETEST",         enter_test_mode               },
{ AT_CMD_TESTRREG,      (uint8_t*)"TESTRREG",      device_test_read_reg          },
{ AT_CMD_IICM,          (uint8_t*)"IICM",          i2c_mode_setting              },
{ AT_CMD_SWIRE,         (uint8_t*)"SWIRE",         swire_ctrl_vo                 },
{ AT_CMD_DEBUG,         (uint8_t*)"DEBUG",         device_debug_setting          },
{ AT_CMD_WREGX,         (uint8_t*)"WREGX",         device_wregx_test             },
{ AT_CMD_RREGX,         (uint8_t*)"RREGX",         device_rregx_test             },
{ AT_CMD_GPIOX,         (uint8_t*)"GPIOX",         device_gpio_ctrl_save         },
{ AT_CMD_RUNX,          (uint8_t*)"RUNX",          device_run_test               },
{ AT_CMD_GPIOEN,        (uint8_t*)"GPIOEN",        gpio_en                       },
{ AT_CMD_GSWIRE,        (uint8_t*)"GSWIRE",        gswire_set                    },
{ AT_CMD_SENDARA,     	(uint8_t*)"SENDARA", 	   sgm_i2c_transmit_ara      	 },
{ AT_CMD_GPINSTAT,    	(uint8_t*)"GPINSTAT", 	   sgm_get_pin_stat      	 	 }
};


int8_t at_cmd_search(char *p){

    uint8_t i, n;
   
    for(i=0;g_at_deal[i].cmd != AT_END;i++){
        n = strlen(p);
        if(!strncmp(p, (char*)g_at_deal[i].at_str, n)){
            if (1 == g_sgm_debug_flag)
                printf("find at cmd %s : %s\r\n",g_at_deal[i].at_str,p);
            return i;
        }
    }
    return -1;
}

void at_cmd_parse(uint8_t* cmd_str,uint32_t* param,uint8_t count)
{
    int8_t index;
    index = at_cmd_search((char*)cmd_str);
    if(index >= 0)
    {
        if(g_at_deal[index].function != NULL)
        {
            g_at_deal[index].function(index,param,count);
        }
    }
}

void uart_buff_parse(uint8_t *str,uint16_t str_len)
{
	int i,equal_count = 0;
	bool equal_flag = 0;
	uint32_t position = 0;
	uint8_t param_temp[13] = {0};
	uint32_t param[0xFF] = {0};
	uint16_t index = 0;
	char *p = (char*)str;
	uint8_t cmd_str[20] = {0};
  
    if((str_len > 4) && (memcmp(str,"AT+",3) == 0))
	{
		if(strstr((char*)str,"=") != NULL)
		{
			for(i =0;i < str_len;i++) 
			{
				if(!equal_flag && str[i] == '=')
				{
						equal_count = i;
						equal_flag = 1;
						position = i;
						continue;
				}
				if(equal_flag && str[i] == ',')
				{
					strncpy((char*)param_temp,(char*)str+position+1,i - position-1);
					param[index++] = atoll((char*)param_temp);
					//printf("param_temp11 %s,position = %d i=%d,param[index] =%u\r\n",
					//param_temp,position,i,param[index-1]);
					memset(param_temp,0,10);
					position = i;
				}
			}
			if(equal_flag && strstr((char*)str+position+1,",") == NULL)
			{
				strncpy((char*)param_temp,(char*)str+position+1,str_len - position-1);
				param[index] = atoll((char*)param_temp);
				//printf("param_temp22:%s,position = %d str_len=%d,param[index] =%u\r\n",
				//										 param_temp,position,str_len,param[index]);
			}
			strncpy((char*)cmd_str,&p[3],equal_count -3);
			if (1 == g_sgm_debug_flag)
			{
				printf("str %s,cmd_str = %s equal_count =%d\r\n",str,cmd_str,equal_count);
				printf("%s param %lu,%lu,%lu,%lu,index=%d \r\n",str,param[0],param[1],param[2],param[3],index);
			}
			at_cmd_parse(cmd_str,param,index+1);
		}else {             
			strncpy((char*)cmd_str,(char*)str+3,str_len -3 - 2);// "AT+","//r/n" =3 + 2
			if (1 == g_sgm_debug_flag)
				printf("cmd_str else = %s\r\n",cmd_str);
			at_cmd_parse(cmd_str,NULL,0);
		}
    }
}

uint8_t at_response_no_param(uint8_t index,bool result)
{
	memset(g_at_response,0,50);
    strcpy(g_at_response,(char*)g_at_deal[index].at_str);
    strcat(g_at_response,AT_RESPONSE_CON);
    if(result)
		strcat(g_at_response,"OK\n");
    else 
		strcat(g_at_response,"ERROR\n");
	printf("at_response_no_param is %s\r\n",g_at_response);
	return 0;
}


uint8_t at_response_param(uint8_t index,uint16_t *response_buff,bool result)
{
	memset(g_at_response,0,50);
	strcpy((char*)g_at_response,(char*)g_at_deal[index].at_str);
	strcat(g_at_response,AT_RESPONSE_CON);
	if(result)
		strcat((char*)g_at_response,(char*)response_buff);
	else
		strcat(g_at_response,"ERROR\n");

	printf("%s",g_at_response);
	return 0;
}

int get_fw_version(uint8_t index,uint32_t* param,uint8_t count)
{	
	printf("%s\r\n",FW_VERSION_NUM);
	return 0;	
}

int device_addr_setting(uint8_t index,uint32_t* param,uint8_t count)
{	
	if (1 == count && param[0] >0 && param[0] < 255){
		g_sgm_slave_addr = (uint8_t)param[0] << 1; 
		printf("%s: OK\n",g_at_deal[index].at_str);
	}
	else
	{
		printf("%s: ERROR\n",g_at_deal[index].at_str);
	}	
	return 0;
}

int device_get_addr_index(uint8_t index, uint32_t* param, uint8_t count)
{
	int ret = 0;
	uint8_t tmp;
	uint8_t data[1024] = {0};
	uint8_t para_data[32];
	int i, type;
	int reg = 0;
	
	if (count >= 2) {
		type = param[0];
		for(i = 0; i < count - 1; i ++)
			para_data[i] = param[1 + i];
		reg = param[count - 1];
		if(type == 0) { // index specified addr, para_data
			for(i = 0; i < count - 1; i++){
				ret = i2c_buffer_read(para_data[i] << 1, reg, &tmp, 1, 1000);
				if (!ret)
					break;
			}
			if(ret != 0){
				printf("ERROR\n");
			} else {
				g_sgm_slave_addr = para_data[i]; 
				snprintf((char*)data+strlen((char*)data),2048,"[%d]",g_sgm_slave_addr);
				printf("GADDR: %s\n",data);
				g_sgm_slave_addr <<= 1;
			}
		} else if (type == 1){ // index all addr 0x01~0x7E
			for(i = 1; i <= 0x7E; i++){
				ret = i2c_buffer_read(i << 1, reg, &tmp, 1, 1000);
				if (!ret)
					break;
			}
			if(ret != 0){
				printf("ERROR\n");
			} else {
				g_sgm_slave_addr = i << 1; 
				snprintf((char*)data+strlen((char*)data),2048,"[%d]",g_sgm_slave_addr);
				printf("GADDR: %s\n",data);
			}
		}
	}
	return 0;
}
int device_baudrate_setting(uint8_t index,uint32_t* param,uint8_t count)
{
	if (1 == count && param[0] >0 && param[0] <= 3400000){
		i2c_init(param[0]);
		printf("%s: OK\n",g_at_deal[index].at_str);
	}
	else
	{
		printf("%s: ERROR\n",g_at_deal[index].at_str);
	}
	return 0;
}

/*

	param[0]            ===> write_len; 
	param[1]-param[n-1] ===> write data; //8 bit
	param[n]            ===> read_len;
*/

int sgm_i2c_transmit(uint8_t index,uint32_t* param,uint8_t count)
{
	uint32_t delay_time;
	uint32_t i2c_delay_time = 1000*200;
	uint8_t i = 0;
	uint8_t ret = 0;
    uint8_t error_code;
	uint8_t write_buffer[255];
	uint8_t read_buffer[255];
	uint8_t data[255] = {'\0'};
	uint8_t write_len = 0;
	uint8_t read_len = 0;
	uint8_t receive_count = 0;

	write_len = param[0];
	if ((count < 2) ||(write_len > count)) {
		printf("%s: ERROR\n",g_at_deal[index].at_str);
		return 0;
	}
	read_len = param[count-1];
	if(write_len > 0){
		for(i = 0;i < write_len;i++)
		{
			write_buffer[i] = (uint8_t)param[1 + i];
		}
		if(read_len == 0)
			ret = i2c_buffer_write(g_sgm_slave_addr, write_buffer[0], write_buffer+1, write_len - 1, 1000, true);
		else
			ret = i2c_buffer_write(g_sgm_slave_addr, write_buffer[0], write_buffer+1, write_len - 1, 1000, false);
		//ret = sgm_i2c_write_block_data(write_buffer[0],write_buffer+1,write_len -1);
		if (ret){
			printf("%s: ERROR\n",g_at_deal[index].at_str);
			return -1;
		}
	}
	if(read_len == 0) {
		printf("%s: OK.\n",g_at_deal[index].at_str);
		return 0;
	}
	if(2 == read_len)
		i2c_ackpos_config(I2C0,I2C_ACKPOS_NEXT);

	 /* send a start condition to I2C bus */
    i2c_start_on_bus(I2C0);

	delay_time = i2c_delay_time;
    /* wait until SBSEND bit is set */
    while(SET!=i2c_flag_get(I2C0, I2C_FLAG_SBSEND)) {
		if((i2c_delay_time--) == 0){
			error_code = 1;
			goto err;
		}
	}

    /* send slave address to I2C bus */
    i2c_master_addressing(I2C0, g_sgm_slave_addr, I2C_RECEIVER);
	//i2c_data_transmit(I2C0, (send_val << 1) | 0x01);
	
	if(read_len < 3){
        /* disable acknowledge */
        i2c_ack_config(I2C0,I2C_ACK_DISABLE);
    }
    /* wait until ADDSEND bit is set */
    delay_time = i2c_delay_time;
    while(SET!=i2c_flag_get(I2C0, I2C_FLAG_ADDSEND)) {
		if((i2c_delay_time--) == 0){
			error_code = 2;
			goto err;
		}
	}

    /* clear the ADDSEND bit */
    i2c_flag_clear(I2C0,I2C_FLAG_ADDSEND);

    /* while there is data to be read */
    delay_time = i2c_delay_time;


	if(1 == read_len){
        /* send a stop condition to I2C bus */
        i2c_stop_on_bus(I2C0);
    }

    while((--delay_time) && read_len) {
        if(3 == read_len){
            /* wait until BTC bit is set */
            while(!i2c_flag_get(I2C0, I2C_FLAG_BTC)) {
				if((delay_time--) == 0){
					error_code = 3;
					goto err;
				}
			}

            /* disable acknowledge */
            i2c_ack_config(I2C0,I2C_ACK_DISABLE);
        }
        if(2 == read_len){
            /* wait until BTC bit is set */
            while(!i2c_flag_get(I2C0, I2C_FLAG_BTC)) {
				if((delay_time--) == 0){
					error_code = 4;
					goto err;
				}
			}
            /* send a stop condition to I2C bus */
            i2c_stop_on_bus(I2C0);
        }

        /* wait until the RBNE bit is set and clear it */
        if(i2c_flag_get(I2C0, I2C_FLAG_RBNE)){
            /* read a byte from the EEPROM */
            read_buffer[receive_count++] = i2c_data_receive(I2C0);

            /* point to the next location where the byte read will be saved */
            //read_buffer++;

            /* decrement the read bytes counter */
            read_len--;
        }
    }

    /* wait until the stop condition is finished */
	delay_time = i2c_delay_time;
    while((I2C_CTL0(I2C0)&0x0200)) {
		if((delay_time--) == 0){
			error_code = 5;
			goto err;
		}
	}

	/* enable acknowledge */
    i2c_ack_config(I2C0,I2C_ACK_ENABLE);
    i2c_ackpos_config(I2C0,I2C_ACKPOS_CURRENT);
	
	for (int i = 0; i < receive_count; i++) {
		snprintf((char*)data+strlen((char*)data),255,"[%d]",read_buffer[i]);
	}
	printf("%s: %s\n",g_at_deal[index].at_str,data);	
	
	return 0;
err:
	i2c_init(400000);
	printf("%s: ERROR,error_code = %d\n",g_at_deal[index].at_str, error_code);
	return error_code;
}

int device_read_reg(uint8_t index,uint32_t* param,uint8_t count)
{
	uint8_t j = 0;
	uint8_t k = (uint8_t)param[1];
	uint8_t reg = (uint8_t)param[0];
	uint8_t data[2048] = {0};

	uint8_t val = 0;
	uint8_t ret = 0;	
				
    if (2 == count){
		for(j = 0;j < k && j < 0xFF;j++){
			ret = sgm_i2c_read_byte(reg+j, &val);
			if (ret){
				printf("RREG: ERROR\n");
				return -1;
			}
			snprintf((char*)data+strlen((char*)data),2048,"[%d]",val);
		}
		printf("RREG: %s\n",data);
		return 0;
	}
	printf("RREG: %s parameter ERROR\n",data);
	return -2;
}


int device_test_read_reg(uint8_t index,uint32_t* param,uint8_t count)
{
	uint8_t j = 0;
	uint8_t k = (uint8_t)param[1];
	uint8_t reg = (uint8_t)param[0];
	uint8_t data[2048] = {0};
	uint8_t val = 0;
	uint8_t ret = 0;

    if (2 == count){
		for(j = 0;j < k && j < 0xFF;j++){
			ret = sgm_i2c_read_byte(reg+j, &val);
			if (ret){
				printf("RREG: ERROR\n");
				return -1;
			}

			snprintf((char*)data+strlen((char*)data),2048,"[%d]",val);
		}

		// test reg  0xF0,F1
		for(j = 0;j < 2;j++){
			ret = sgm_i2c_read_byte(0xF0+j, &val);
			if (ret){
				printf("RREG: ERROR\n");
				return -1;
			}

			snprintf((char*)data+strlen((char*)data),2048,"[%d]",val);
		}
				
		//0x8A-0x8E
		for(j = 0;j < 5;j++){
			ret = sgm_i2c_read_byte(0x8A+j, &val);
			if (ret){
				printf("RREG: ERROR\n");
				return -1;
			}
			snprintf((char*)data+strlen((char*)data),2048,"[%d]",val);
		}

		//0xA0-0xAB
		for(j = 0;j < 12;j++){
			ret = sgm_i2c_read_byte(0xA0+j, &val);
			if (ret){
				printf("RREG: ERROR\n");
				return -1;
			}
			snprintf((char*)data+strlen((char*)data),2048,"[%d]",val);
		}

		//0x90-0x97
		for(j = 0;j < 8;j++){
			ret = sgm_i2c_read_byte(0x90+j, &val);
			if (ret){
				printf("RREG: ERROR\n");
				return -1;
			}
			snprintf((char*)data+strlen((char*)data),2048,"[%d]",val);
		}
		printf("RREG: %s\n",data);
		return 0;
	}
	printf("RREG: %s parameter ERROR\n",data);
	return -2;
}

int device_write_reg(uint8_t index,uint32_t* param,uint8_t count)
{
	uint8_t reg = (uint8_t)param[0];
	uint8_t val = 0;
	uint8_t H = 0;
	uint8_t L = 0;
	uint8_t ret = 0;

    if (2 == count){
		val = (uint8_t)param[1];
		ret = sgm_i2c_write_byte(reg, val);
	}
	else if (4 == count)
	{
		H = (uint8_t)param[1];
		L = (uint8_t)param[2];
		if (H > 7 || L > 7 || H < L){
			printf("%s: parameter ERROR\n",g_at_deal[index].at_str);
			return -2;
		}
		val = (uint8_t)param[3]<<L;
		ret = sgm_update_bits(reg,GENMASK(H,L),val);
	} else {
		printf("%s: parameter ERROR\n",g_at_deal[index].at_str);
		return -2;
	}
	if (ret){
		printf("%s: ERROR\n",g_at_deal[index].at_str);
		return -1;
	}
	printf("%s: OK\n",g_at_deal[index].at_str);

	return 0;
}

int device_16bit_wreg(uint8_t index,uint32_t* param,uint8_t count)
{
	uint8_t reg = (uint8_t)param[0];
	uint16_t val = 0;
	uint8_t data[2] = {0};
	uint8_t H = 0;	
	uint8_t L = 0;
	uint8_t ret = 0;
		
    if (2 == count){
		data[0] = param[1] & 0xFF;
		data[1] = param[1] >> 8 & 0xFF;
        ret = sgm_i2c_write_block_data(reg,data, 2);
		if (ret){
			printf("%s: ERROR\n",g_at_deal[index].at_str);
			return -1;
		}
	} else if (4 == count) {
		H = (uint8_t)param[1];
		L = (uint8_t)param[2];
		if (H > 15 || L > 15 || H < L){
			printf("%s: parameter ERROR\n",g_at_deal[index].at_str);
			return -2;
		}
		val = (uint16_t)param[3]<<L;
		ret = sgm_update_16bits(reg,GENMASK(H,L),val);
	} else {
		printf("%s: parameter ERROR\n",g_at_deal[index].at_str);
		return -3;
	}
	if (ret){
		printf("%s: ERROR\n",g_at_deal[index].at_str);
		return -4;
	}
	printf("%s: OK\n",g_at_deal[index].at_str);

	return 0;
}

int device_16bit_rreg(uint8_t index,uint32_t* param,uint8_t count)
{
	uint8_t reg = (uint8_t)param[0];
	uint16_t val = 0;
	uint8_t data[2] = {0};
	uint8_t ret = 0;
		
    if (1 == count){
        ret = sgm_i2c_read_block_data(reg,data, 2);
		if (ret){
			printf("%s: ERROR\n",g_at_deal[index].at_str);
			return -1;
		}
        val = data[0] | (data[1] << 8);
	}  else {
		printf("%s: parameter ERROR\n",g_at_deal[index].at_str);
		return -2;
	}
	
	printf("%s: [%d]\n",g_at_deal[index].at_str,val);

	return 0;
}

int device_jump_wreg(uint8_t index,uint32_t* param,uint8_t count)
{
	uint8_t num = 0;
	uint8_t byte = 0;
	uint16_t addr[255] = {0};
	uint16_t data[255] = {0};
	uint8_t buff[2] = {0};
	uint16_t i = 0;
	uint8_t len = 0;
	uint8_t size = 0;
	int ret = 0;
	
	if (6 > count || count % 2 || param[0] < 2 || 2 < param[1]){
		printf("%s: ERROR\n",g_at_deal[index].at_str);
		return -1;
	} else {
		num = param[0];
		byte = param[1];

		for (i = 2;i < (num + 1) * 2;i++ )
		{
			if (0 == i % 2)
				addr[len++] = param[i];
			else
				data[size++] = param[i];
		}
		
		for (i = 0;i < num;i++ )
		{
			if (1 == byte){
				buff[0] = data[i] & 0xFF;
                ret = sgm_i2c_write_block_data((uint8_t)addr[i],&buff[0],byte);
			} else if (2 == byte){
				buff[0] = data[i] & 0xFF;
				buff[1] = data[i] >> 8 & 0xFF;
                ret = sgm_i2c_write_block_data((uint8_t)addr[i],buff,byte);
			}

			if (ret){
				printf("%s: ERROR\n",g_at_deal[index].at_str);
				return -2;
			}
		}
	}
	printf("%s: OK\n",g_at_deal[index].at_str);
	return 0;
}

#define CRC_8_POLYNOMIAL 0x07 //X8 + X5 + X3 + 1
unsigned char CRC8_Calculate(unsigned char *pData, unsigned char Size)
{
	unsigned char i;
	unsigned char rCRC = 0;

	while(Size--) {
		rCRC ^= *pData++;
		for (i = 8; i > 0; --i) {
			if (rCRC & 0x80)
				rCRC = (rCRC << 1) ^ CRC_8_POLYNOMIAL;
			else
				rCRC = (rCRC << 1);
		}
	}
	return(rCRC);
}


int device_write_reg_crc(uint8_t index,uint32_t* param,uint8_t count)
{
	uint8_t reg = (uint8_t)param[0];
	uint8_t val = 0;
	uint8_t H = 0;
	uint8_t L = 0;
	uint8_t ret = 0;

	uint8_t crc_buff[4];
	uint8_t write_buff[2];
	
    if (2 == count){
		val = (uint8_t)param[1];
		crc_buff[0] = g_sgm_slave_addr;
		crc_buff[1] = reg;
		crc_buff[2] = val;
		crc_buff[3] = 0;

		write_buff[1] = CRC8_Calculate(crc_buff,4);
		write_buff[0] = val;
		//printf("crc test =%d,%d,%d,%d",crc_buff[0],crc_buff[1],crc_buff[2], write_buff[1]); 
		ret = sgm_i2c_write_block_data(reg, write_buff,2);
	} else if (4 == count){
		H = (uint8_t)param[1];
		L = (uint8_t)param[2];
		if (H > 7 || L > 7 || H < L){
			printf("%s: parameter ERROR\n",g_at_deal[index].at_str);
			return -2;
		}
		val = (uint8_t)param[3]<<L;
		ret = sgm_update_bits_crc(reg,GENMASK(H,L),&val);
		
	} else {
		printf("%s: parameter ERROR\n",g_at_deal[index].at_str);
		return -2;
	}
	if (ret){
		printf("%s: ERROR\n",g_at_deal[index].at_str);
		return -1;
	}
	printf("%s: OK\n",g_at_deal[index].at_str);

	return 0;
}

int Mult_write_reg(uint8_t index,uint32_t* param,uint8_t count)
{
	uint8_t reg = (uint8_t)param[0];
	uint8_t i = 0;
	uint8_t ret = 0;

    if (3 <= count) {
		for(i = 0;i < count -1;i++)
		{
			ret = sgm_i2c_write_byte(reg + i, (uint8_t)param[1 + i]);
			if (ret){
				printf("%s: ERROR\n",g_at_deal[index].at_str);
				return -1;
			}
		}
	} else {
		printf("%s: parameter ERROR\n",g_at_deal[index].at_str);
		return -2;
	}
	printf("%s: OK\n",g_at_deal[index].at_str);

	return 0;
}

int page_write_reg(uint8_t index,uint32_t* param,uint8_t count)
{
	uint8_t reg = (uint8_t)param[0];
	uint8_t i = 0;
	uint8_t ret = 0;
	uint8_t buff[255] = {0};

  if (2 <= count) {
		for(i = 0;i < count - 1;i++)
		{
			buff[i] = (uint8_t)param[1 + i];
		}
        ret = sgm_i2c_write_block_data(reg,buff,count - 1);
		if (ret){
			printf("%s: ERROR\n",g_at_deal[index].at_str);
			return -1;
		}
	} else {
		printf("%s: parameter ERROR\n",g_at_deal[index].at_str);
		return -2;
	}
	printf("%s: OK\n",g_at_deal[index].at_str);

	return 0;
}

int page_read_reg(uint8_t index,uint32_t* param,uint8_t count)
{
	uint8_t j = 0;
	uint8_t len = (uint8_t)param[1];
	uint8_t reg = (uint8_t)param[0];
	uint8_t val[255] = {0};
	uint8_t data[1500] = {0};
	uint8_t ret = 0;

	if (2 == count && len > 0){
        ret = sgm_i2c_read_block_data(reg,val,len);
		if (ret){
			printf("%s: ERROR\n",g_at_deal[index].at_str);
			return -1;
		}

		for(j = 0;j < len && j < 0xFF;j++){
			snprintf((char*)data+strlen((char*)data),1500,"[%d]",val[j]);
		}
		printf("%s: %s\n",g_at_deal[index].at_str,data);
		return 0;
	}    
	printf("%s: parameter ERROR\n",g_at_deal[index].at_str);
	return -2;
}

int jump_read_reg(uint8_t index,uint32_t* param,uint8_t count)
{
	uint8_t num = 0;
	uint8_t byte = 0;
	uint16_t addr[255] = {0};
	uint8_t data[1500] = {0};
	uint8_t buff[2] = {0};
	uint16_t i = 0;
	uint8_t len = 0;
	uint16_t val = 0;
	int ret = 0;
	
	if (4 > count || param[0] < 2 || param[1] > 2){
		printf("%s: ERROR\n",g_at_deal[index].at_str);
		return -1;
	} else {
		num = param[0];
		byte = param[1];
		for (i = 2;i < num + 2;i++ ) {
			addr[len++] = param[i];
		}		
		for (i = 0;i < num;i++ ) {
			if (1 == byte){
                ret = sgm_i2c_read_block_data(addr[i],&buff[0],byte);
				val = buff[0];
			} else if (2 == byte){
                ret = sgm_i2c_read_block_data(addr[i],buff,byte);
				val = buff[0] | (buff[1] << 8);
			}
			if (ret){
				printf("%s: ERROR\n",g_at_deal[index].at_str);
					return -2;
			}
			snprintf((char*)data+strlen((char*)data),1500,"[%d]",val);
		}
	}
	printf("%s: %s\n",g_at_deal[index].at_str,data);
	return 0;
}

int CMult_write_reg(uint8_t index,uint32_t* param,uint8_t count)
{
	uint8_t reg = (uint8_t)param[0];
	//uint8_t val = 0;
	uint8_t i = 0;
	uint8_t ret = 0;
	uint8_t crc_buff[4];
	uint8_t write_buff[2];

    if (3 <= count){
		//val = (uint8_t)param[1];
		for(i = 0;i < count - 1;i++) {
			crc_buff[0] = g_sgm_slave_addr;
			crc_buff[1] = reg+i;
			crc_buff[2] = param[1 + i];
			crc_buff[3] = 0;
				
			write_buff[1] = CRC8_Calculate(crc_buff,4);
			write_buff[0] = param[1 + i];
			//printf("crc test =%d,%d,%d,%d",crc_buff[0],crc_buff[1],crc_buff[2], write_buff[1]); 
            ret = sgm_i2c_write_block_data(reg+i, write_buff,2);
			//ret = sgm_i2c_write_byte(reg + i, param[1 + i]);
			if (ret){
				printf("%s: ERROR\n",g_at_deal[index].at_str);
				return -1;
			}
		}
	} else {
		printf("%s: parameter ERROR\n",g_at_deal[index].at_str);
		return -2;
	}
	printf("%s: OK\n",g_at_deal[index].at_str);
	return 0;
}


int enter_test_mode(uint8_t index,uint32_t* param,uint8_t count)
{
	uint8_t reg = 0;
	uint8_t val = 0;
	uint8_t ret = 0;

	uint8_t crc_buff[4];
	uint8_t write_buff[2];
	
    if (5 == count){
		g_sgm_test_mode_flag = param[0];

		if(param[0] == 0){
			printf("%s: OK\n",g_at_deal[index].at_str);
			return 0;
		}
		reg = (uint8_t)param[1];
		val = (uint8_t)param[2];
		crc_buff[0] = g_sgm_slave_addr;
		crc_buff[1] = reg;
		crc_buff[2] = val;
		crc_buff[3] = 0;
		write_buff[1] = CRC8_Calculate(crc_buff,4);
		write_buff[0] = val;
		//printf("crc test =%d,%d,%d,%d",crc_buff[0],crc_buff[1],crc_buff[2], write_buff[1]);    
		ret = sgm_i2c_write_block_data(reg, write_buff,2);
        
		reg = (uint8_t)param[3];
		val = (uint8_t)param[4];
		crc_buff[0] = g_sgm_slave_addr;
		crc_buff[1] = reg;
		crc_buff[2] = val;
		crc_buff[3] = 0;
			
		write_buff[1] = CRC8_Calculate(crc_buff,4);
		write_buff[0] = val;
		//	printf("crc test =%d,%d,%d,%d",crc_buff[0],crc_buff[1],crc_buff[2], write_buff[1]);    
		ret = sgm_i2c_write_block_data(reg, write_buff,2);
	} else {
		printf("%s: parameter ERROR\n",g_at_deal[index].at_str);
		return -2;
	}
	if (ret){
		printf("%s: ERROR\n",g_at_deal[index].at_str);
		return -1;
	}
	printf("%s: OK\n",g_at_deal[index].at_str);

	return 0;
}



int i2c_mode_setting(uint8_t index,uint32_t* param,uint8_t count)
{
	if (1 == count){
		if (1 == param[0])
			i2c_init(400000);
		else if (2 == param[0])
			swire_init();
		else {
			printf("%s: parameter ERROR\n",g_at_deal[index].at_str);
			return -1;
		}
	} else {
		printf("%s: parameter ERROR\n",g_at_deal[index].at_str);
		return -2;
	}
	printf("%s: OK\n",g_at_deal[index].at_str);
	return 0;
}


int swire_ctrl_vo(uint8_t index,uint32_t* param,uint8_t count)
{
	uint8_t swire = 0;
	uint16_t pulse_count = 0;
	
	if (2 == count){
		swire = (uint8_t)param[0];
		pulse_count = (uint16_t)param[1];
		if (1 == swire){
			if ((pulse_count>0 && pulse_count < 8) 
				|| 16 == pulse_count || 25 == pulse_count
					|| (pulse_count>25 && pulse_count < 104)
						|| (pulse_count>105 && pulse_count < 109)
							|| (pulse_count>114 && pulse_count < 125)) {
								set_vo1_vo2_output_volt(pulse_count);
			} else {
				printf("%s1: parameter ERROR\n",g_at_deal[index].at_str);
				return -1;
			}
			printf("%s1: OK\n",g_at_deal[index].at_str);
		}
		if (2 == swire){
			if (pulse_count>5 && pulse_count < 32)
				set_vo3_output_volt(pulse_count);
			else if (pulse_count>31 && pulse_count < 65)
				set_vo4_output_volt(pulse_count);
			else{
				printf("%s2: parameter ERROR\n",g_at_deal[index].at_str);
				return -2;
			}
			printf("%s2: OK\n",g_at_deal[index].at_str);
		}
	} else {
		printf("%s: parameter ERROR\n",g_at_deal[index].at_str);
		return -3;
	}

	return 0;
}

int device_debug_setting(uint8_t index,uint32_t* param,uint8_t count)
{
	if (1 == count && param[0] < 255){
		if (param[0] == 1) {
			g_sgm_debug_flag = 1;
		} else {
			g_sgm_debug_flag = 0;
		}
		printf("%s: OK\n",g_at_deal[index].at_str);
	}
    else {       
		printf("%s: ERROR\n",g_at_deal[index].at_str);
	}
	return 0;
}

int device_wregx_test(uint8_t index,uint32_t* param,uint8_t count)
{
	uint8_t i = 0;
	
	if (g_test_count == 20)
	{
		printf("CMD is FULL,count should < 20 \n");
		return 0;
	}
	if (4 <= count && param[count - 1] <= 1000000){	
		s_read_write_buff[g_test_count].flag = 0;
		s_read_write_buff[g_test_count].reg = (uint8_t)param[0];
		s_read_write_buff[g_test_count].len = (uint8_t)param[1];

		for(i = 2; i < count - 1 && i < 12;i++)
		{
		   s_read_write_buff[g_test_count].buff[i - 2] = (uint8_t)param[i];
		}
		s_read_write_buff[g_test_count].delay = param[count - 1];
        
		g_test_count += 1;
		printf("%s: OK\n",g_at_deal[index].at_str);
	} else {
			printf("%s: ERROR\n",g_at_deal[index].at_str);
	}
	return 0;
}
int device_rregx_test(uint8_t index,uint32_t* param,uint8_t count)
{
	if (g_test_count == 20)
	{
		printf("CMD is FULL,count should <= 20 \n");
		return 0;
	}
	if (3 == count && param[count - 1] <= 1000000){
		s_read_write_buff[g_test_count].flag = 1;
		s_read_write_buff[g_test_count].reg = (uint8_t)param[0];
		s_read_write_buff[g_test_count].len = (uint8_t)param[1];
		s_read_write_buff[g_test_count].delay = param[2];
    //printf("index = %d reg %d:len = %d\n",g_test_count,s_read_write_buff[g_test_count].reg,s_read_write_buff[g_test_count].len);
		g_test_count += 1;
		printf("%s: OK\n",g_at_deal[index].at_str);
	} else {
		printf("%s: ERROR\n",g_at_deal[index].at_str);
	}
	return 0;
}

int device_gpio_ctrl_save(uint8_t index,uint32_t* param,uint8_t count)
{
	
	if (g_test_count == 20)
	{
		printf("CMD is FULL,count should < 20 \n");
		return 0;
	}
	if (3 == count && param[0] < 3 && param[count - 1] <= 1000000){
		s_read_write_buff[g_test_count].flag = 2;
		s_read_write_buff[g_test_count].buff[0] = (uint8_t)param[0];
		s_read_write_buff[g_test_count].buff[1] = (uint8_t)param[1];
		s_read_write_buff[g_test_count].delay = param[2];
    //printf("index = %d reg %d:len = %d\n",g_test_count,s_read_write_buff[g_test_count].reg,s_read_write_buff[g_test_count].len);
		g_test_count += 1;
		printf("%s: OK\n",g_at_deal[index].at_str);
	} else {
		printf("%s: ERROR\n",g_at_deal[index].at_str);
	}
	return 0;
}

int device_run_test(uint8_t index,uint32_t* param,uint8_t count)
{	
	uint8_t data[255] = {'\0'};
    uint8_t temp[255] = {'\0'};
    uint32_t gpio_pin = 0;
    uint32_t str_len = 0;
    uint32_t temp_len = 0;
	int ret = 0;
	
	//printf("device_run_test count %d\n",g_test_count);
    
	for(uint8_t i = 0; i < g_test_count;i++)
	{
        if(0 == s_read_write_buff[i].flag){
            ret = sgm_i2c_write_block_data(s_read_write_buff[i].reg,s_read_write_buff[i].buff,s_read_write_buff[i].len);
            if (ret){
                memset(s_read_write_buff,0,sizeof(s_read_write_buff));
                g_test_count = 0;
                printf("%s: ERROR\n",g_at_deal[index].at_str);
                return -1;
            }
        }
        else if (1 == s_read_write_buff[i].flag)
        {           
           str_len = 0;
          //printf("%s: index = %d reg %d:len = %d\n",g_at_deal[index].at_str,i,s_read_write_buff[i].reg,s_read_write_buff[i].len);
            ret = sgm_i2c_read_block_data(s_read_write_buff[i].reg,data,s_read_write_buff[i].len);
            if (ret){
                    memset(s_read_write_buff,0,sizeof(s_read_write_buff));
                    g_test_count = 0;
                    printf("%s: ERROR\n",g_at_deal[index].at_str);
                    return -1;
            }
            
            for(uint8_t j = 0;j < s_read_write_buff[i].len && j < 0xFF;j++){
                str_len += snprintf((char*)data+str_len,255,"[%d]",data[j]);
            }
            
            temp_len += snprintf((char*)temp + temp_len, 255 - temp_len, "%s:%s",g_at_deal[index].at_str,data);
            memset(data,'\0',str_len); 
            //printf("%s: %s\n",g_at_deal[index].at_str,data);            
        }        
        else if(2 == s_read_write_buff[i].flag)
        {            
			if(1 == s_read_write_buff[i].buff[0])
				gpio_pin =GPIO_PIN_4; //PB4 SWIRE1
			else if(2 == s_read_write_buff[i].buff[0])    
				gpio_pin =GPIO_PIN_5; //PB5	SWIRE2		

			if (s_read_write_buff[i].buff[1] == 0)
				gpio_bit_write(GPIOB, gpio_pin, RESET);
			else
				gpio_bit_write(GPIOB, gpio_pin, SET);        
        }
        
        if(s_read_write_buff[i].delay != 0)
        {            
            delay_1us(s_read_write_buff[i].delay);
        }        
	}	
	g_test_count = 0;
	memset(s_read_write_buff,0,sizeof(s_read_write_buff));
	printf("%s: %s OK\n",g_at_deal[index].at_str,temp);
	return 0;
}

int gpio_en(uint8_t index,uint32_t* param,uint8_t count)
{
	uint32_t gpio_pin = 0;

	if (2 == count ){
		if(1 == param[0])
			gpio_pin =GPIO_PIN_3; //PA3
		else if(2 == param[0])    
			gpio_pin =GPIO_PIN_12; //PA12
		else if(3 == param[0])
			gpio_pin =GPIO_PIN_11;  //PA11

		if (param[1] == 0)
			gpio_bit_write(GPIOA, gpio_pin, RESET);
		else
			gpio_bit_write(GPIOA, gpio_pin, SET);
		printf("%s: OK\n",g_at_deal[index].at_str);
	} else {
		printf("%s: ERROR\n",g_at_deal[index].at_str);
	}
	return 0;
}

//swire general settings
int gswire_set(uint8_t index,uint32_t* param,uint8_t count)
{
	uint32_t gpio_pin = 0;
	uint8_t i = 0;
	int j = 0;
	uint32_t delay_us;
	//0-- swire1;  1-- swire2; 2-- swire1 && swire2;
	int swire_type = 0;
	int	ins_size = 4;
	// use 4 parameters as a instruction group
	uint32_t instruction[20][4];
	memset(instruction,0,80);
	uint8_t ins_rec_len = 0;

	if (5 <= count ){
		swire_type = param[0];
		ins_rec_len = (count-1) / ins_size;
		for (i = 0; i < ins_rec_len; i++) {
    		for (j = 0; j < ins_size; j++) {
    			instruction[i][j] = *(param+1 + i * ins_size + j);
				//printf("instruction[%d][%d] = %d\n",i,j,instruction[i][j]);
    		}
		}
#if 1
		if(swire_type == 0)
			gpio_pin = GPIO_PIN_4;
		else if(swire_type == 1)
			gpio_pin = GPIO_PIN_5;
		else if(swire_type == 2)
			gpio_pin = GPIO_PIN_4 | GPIO_PIN_5;
		for (i = 0; i < ins_rec_len; i++) {
			if(instruction[i][0] == 0){
				// output pulse
				for (j = 0; j < instruction[i][3]; j++) {
					gpio_bit_write(GPIOB, gpio_pin, SET);
					delay_1us(instruction[i][1]);
					gpio_bit_write(GPIOB, gpio_pin, RESET);
					delay_1us(instruction[i][2]);
				}
			} else if(instruction[i][0] == 1){
				// high voltage
				delay_us = instruction[i][2] * instruction[i][3];
				if(delay_us > 1864135)
					printf("%s: ERROR\n",g_at_deal[index].at_str);
				else {
					gpio_bit_write(GPIOB, gpio_pin, SET);
					delay_1us(delay_us);
				}

			} else if(instruction[i][0] == 2){
				// low voltage
				delay_us = instruction[i][2] * instruction[i][3];
				if(delay_us > 1864135)
					printf("%s: ERROR\n",g_at_deal[index].at_str);
				else {
					gpio_bit_write(GPIOB, gpio_pin, RESET);
					delay_1us(delay_us);
				}
			} else{
				// end output
			}

		}
#endif
		printf("%s: OK\n",g_at_deal[index].at_str);
	} else {
		printf("%s: ERROR\n",g_at_deal[index].at_str);
	}
	
	return 0;
}




/*
	param[0]            ===> write_len; 
	param[1]-param[n-1] ===> write data; //8 bit
	param[n]            ===> read_len;
	eg:
	//AT+SENDARA=1,128,0  write_len = 1...n
	//AT+SENDARA=0,1,1 send broadcast start commond:0x01
	//AT+SENDARA=1,128,1 send ARA and read stat. read_len = 1...n
*/
int sgm_i2c_transmit_ara(uint8_t index,uint32_t* param,uint8_t count)
{
	uint32_t delay_time;
	uint32_t i2c_delay_time = 1000*50;
	uint8_t i = 0;
    uint8_t error_code;
	uint8_t write_buffer[255];
	uint8_t read_buffer[255];
	uint8_t data[255] = {'\0'};
	uint8_t write_len = 0;
	uint8_t read_len = 0;
	uint8_t receive_count = 0;

	write_len = param[0];
	if ((count < 2) ||(write_len > count)) {
		printf("%s: ERROR\n",g_at_deal[index].at_str);
		return 0;
	}
	read_len = param[count-1];
	
	 /* wait until I2C bus is idle */
    delay_time = i2c_delay_time;
    while(i2c_flag_get(I2C0, I2C_FLAG_I2CBSY))
    {	if((delay_time--) == 0){	error_code = 1; goto err;	}	}
    
	if(2 == read_len)
		i2c_ackpos_config(I2C0,I2C_ACKPOS_NEXT);
	
    /* send a start condition to I2C bus */
    i2c_start_on_bus(I2C0);
    
    /* wait until SBSEND bit is set */
    delay_time = i2c_delay_time;
    while(!i2c_flag_get(I2C0, I2C_FLAG_SBSEND))
    {	if((delay_time--) == 0){	error_code = 2; goto err;	}	}
    
	if(write_len > 0 && read_len == 0){ //AT+SENDARA=1,128,0  write_len = 1...n
		for(i = 0;i < write_len;i++)
		{
			write_buffer[i] = (uint8_t)param[1 + i];
		}
		/* send slave address to I2C bus */
		i2c_master_addressing(I2C0, write_buffer[0], I2C_TRANSMITTER);
		 /* wait until ADDSEND bit is set */
		delay_time = i2c_delay_time;
		while(!i2c_flag_get(I2C0, I2C_FLAG_ADDSEND))
		{	if((delay_time--) == 0){	error_code = 3; goto err;	}	}
    
		/* clear the ADDSEND bit */
		i2c_flag_clear(I2C0,I2C_FLAG_ADDSEND);
		write_len--;
	} else if (write_len == 0 && read_len == 1) {  //AT+SENDARA=0,1,1 send broadcast start commond:0x01
		write_buffer[0] = (uint8_t)param[1];
		/* send slave address to I2C bus */
		i2c_master_addressing(I2C0, write_buffer[0], I2C_RECEIVER);
		 /* wait until ADDSEND bit is set */
		delay_time = i2c_delay_time;
		while(!i2c_flag_get(I2C0, I2C_FLAG_ADDSEND))
		{	if((delay_time--) == 0){	error_code = 4; goto err;	}	}
    
		/* clear the ADDSEND bit */
		i2c_flag_clear(I2C0,I2C_FLAG_ADDSEND);
		read_len--;
		if(read_len == 0){ /* disable acknowledge */
			i2c_ack_config(I2C0,I2C_ACK_DISABLE);
		}
	} else if (write_len == 1 && read_len > 0) {	//AT+SENDARA=1,128,1 send ARA and read stat. read_len = 1...n
		write_buffer[0] = (uint8_t)param[1];
		/* send slave address to I2C bus */
		i2c_master_addressing(I2C0, write_buffer[0], I2C_RECEIVER);
		
		if(read_len < 3)
			i2c_ack_config(I2C0,I2C_ACK_DISABLE);	/* disable acknowledge */

		 /* wait until ADDSEND bit is set */
		delay_time = i2c_delay_time;
		while(!i2c_flag_get(I2C0, I2C_FLAG_ADDSEND))
		{	if((delay_time--) == 0){	error_code = 5; goto err;	}	}
    
		/* clear the ADDSEND bit */
		i2c_flag_clear(I2C0,I2C_FLAG_ADDSEND);
		
		if(1 == read_len)
			i2c_stop_on_bus(I2C0); /* send a stop condition to I2C bus */

		write_len--;
	}
	// Send Data
	if (write_len) {
		for(i = 0; i < write_len; i ++) {
			i2c_data_transmit(I2C0, write_buffer[1 + i]);

			/* wait until BTC bit is set */
			delay_time = i2c_delay_time;
			while(!i2c_flag_get(I2C0, I2C_FLAG_BTC))
			{	if((delay_time--) == 0){	error_code = 6; goto err;	}	}
		}
		write_len = 0;
	}
	
	// Send Cmmond Finished
	if (write_len == 0 && read_len == 0) {
		/* send a stop condition to I2C bus */
		i2c_stop_on_bus(I2C0);
		/* wait until the stop condition is finished */
		delay_time = i2c_delay_time;
		while(I2C_CTL0(I2C0)&0x0200)
		{	if((delay_time--) == 0){	error_code = 7; goto err;	}	}
		
		printf("%s: OK.\n",g_at_deal[index].at_str);
		return 0;
	}

    /* while there is data to be read */
    delay_time = i2c_delay_time * 6;
    while((--delay_time) && read_len) {
        if(3 == read_len){
            /* wait until BTC bit is set */
            while(!i2c_flag_get(I2C0, I2C_FLAG_BTC))
			{	if((delay_time--) == 0){	error_code = 8;		goto err;	}	}

            /* disable acknowledge */
            i2c_ack_config(I2C0,I2C_ACK_DISABLE);
        }
        if(2 == read_len){
            /* wait until BTC bit is set */
            while(!i2c_flag_get(I2C0, I2C_FLAG_BTC))
			{	if((delay_time--) == 0){	error_code = 9;		goto err;	}	}
            /* send a stop condition to I2C bus */
            i2c_stop_on_bus(I2C0);
        }

        /* wait until the RBNE bit is set and clear it */
        if(i2c_flag_get(I2C0, I2C_FLAG_RBNE)){
            /* read a byte from the EEPROM */
            read_buffer[receive_count++] = i2c_data_receive(I2C0);
            /* decrement the read bytes counter */
            read_len--;
        }
    }

    /* wait until the stop condition is finished */
	delay_time = i2c_delay_time;
    while((I2C_CTL0(I2C0)&0x0200)) {
		if((delay_time--) == 0){
			error_code = 9;
			goto err;
		}
	}

	/* enable acknowledge */
    i2c_ack_config(I2C0,I2C_ACK_ENABLE);
    i2c_ackpos_config(I2C0,I2C_ACKPOS_CURRENT);
	
	for (int i = 0; i < receive_count; i++) {
		snprintf((char*)data+strlen((char*)data),255,"[%d]",read_buffer[i]);
	}
	printf("%s: %s\n",g_at_deal[index].at_str,data);	
	
	return 0;
err:
	i2c_stop_on_bus(I2C0);
	/* wait until the stop condition is finished */
	delay_time = i2c_delay_time;
	while(I2C_CTL0(I2C0)&0x0200)
	{	if((delay_time--) == 0){	error_code = 10; goto err;	}	}
	
	i2c_init(400000);
	printf("%s: ERROR,error_code = %d\n",g_at_deal[index].at_str, error_code);
	return error_code;
}


//get gpio or irq pin stat
int sgm_get_pin_stat(uint8_t index,uint32_t* param,uint8_t count)
{
	int type = 0;	
	
	if (count == 1){ //init pin type
		type = param[0];
		if (type == 0) {
			gpio_init(GPIOB,GPIO_MODE_IPU,GPIO_OSPEED_50MHZ,GPIO_PIN_5);
			nvic_irq_enable(EXTI5_9_IRQn, 0, 0);
			exti_interrupt_flag_clear(EXTI_5);
			/* connect EXTI line to GPIO pin */
			gpio_exti_source_select(GPIO_PORT_SOURCE_GPIOB, GPIO_PIN_SOURCE_5);
			/* configure EXTI line */
			exti_init(EXTI_5, EXTI_INTERRUPT, EXTI_TRIG_FALLING);
			exti_interrupt_flag_clear(EXTI_5);
		} else {
			gpio_pin_remap_config(GPIO_SWJ_NONJTRST_REMAP,ENABLE);
			gpio_init(GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_5);
			gpio_bit_set(GPIOB, GPIO_PIN_5);
		}
	} else if (count == 2) { //get pin stat
		type = param[0];
		//level = param[1];
		if (type == 0 && sgm_irq_flag == 1) { //trigger irq
			printf("%s: [%d]\n",g_at_deal[index].at_str, sgm_irq_flag);	
			sgm_irq_flag = 0;
		} else if (type == 1){
			int stat = gpio_output_bit_get(GPIOB, GPIO_PIN_5);
			printf("%s: [%d]\n",g_at_deal[index].at_str, stat);	
		}
	} else {
		printf("%s: ERROR\n",g_at_deal[index].at_str);
		return 0;
	}
	return 0;
}


