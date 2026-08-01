// Add by Allen

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "scp_protocol.h"
#include "at_cmd_common.h"
#include "i2c.h"
#include "systick.h"

/* Legacy function aliases (ARMCC allowed implicit declarations) */
#define sgm_write_byte(reg, val)  sgm_i2c_write_byte(reg, val)
#define sgm_read_byte(reg, pval)  sgm_i2c_read_byte(reg, pval)
/***********************************************************************************************
                            Global variable
***********************************************************************************************/


struct scp_recv_t g_recv_scp_msg = {0};

static bool s_scp_output_mode = 0;
uint8_t g_sgm_scp_debug_flag = 0;

struct scp_send_t scp_msg_cmd = {0};
//struct scp_recv_t scp_msg_ack = {0};

/***********************************************************************************************
                            Function declaration
***********************************************************************************************/


/***********************************************************************************************
                            Function  interface
***********************************************************************************************/
#define SGM41600A_TEST_SCP

#ifdef SGM41600A_TEST_SCP

int sgm_send_scp_cmd(enum SCP_CMD_T cmd, enum SCP_REG_ADDR reg, uint8_t len,uint8_t *buff)
{
	//int ret = 0;
	struct scp_send_t scp_msg_cmd = {0};
	
	scp_msg_cmd.sbrwr = cmd;	
	scp_msg_cmd.reg   = reg;	
	scp_msg_cmd.len   = len;
	
	sgm_write_byte(0x2B,scp_msg_cmd.sbrwr);
	switch (cmd){		
		case RT_SBRWR_CMD://单字节写命令
			memcpy(scp_msg_cmd.data,buff,1);
			sgm_write_byte(0x2C,scp_msg_cmd.reg);
			sgm_write_byte(0x2D,1);
			sgm_write_byte(0x30,scp_msg_cmd.data[0]);
		  printf("sgm_send_scp_cmd reg = 0x%x,data = %d\n",scp_msg_cmd.reg,scp_msg_cmd.data[0]);
			break;
			
		case RT_SBRRD_CMD://单字节读命令
			sgm_write_byte(0x2C,scp_msg_cmd.reg);
		  //sgm_write_byte(0x2D,1);
			break;
			
		case RT_MBRWR_CMD: //多字节写命令						
			sgm_write_byte(0x2C,scp_msg_cmd.reg);			
			if (len && len <= RT_BUFF_MAX_LEN ){				
				memcpy(scp_msg_cmd.data,buff,len);
				sgm_write_byte(0x2D,scp_msg_cmd.len);
				for (int i = 0;i < len;i++){
					sgm_write_byte(0x30 + i,buff[i]);
					printf("sgm_send_scp_cmd reg = 0x%x,data = %d\n",0x30 + i,buff[i]);
				}
			}			
			break;
			
		case RT_MBRRD_CMD: //多字节读命令
			sgm_write_byte(0x2C,scp_msg_cmd.reg);
			if (len){
				sgm_write_byte(0x2D,scp_msg_cmd.len);
			}
			break;
			
		default:
			break;
	}	
	sgm_update_bits(0x2A,1,1);
	if (1 == g_sgm_scp_debug_flag)
		printf("sgm_send_scp_cmd cmd = 0x%x,reg = 0x%x,len = 0x%x\n",cmd,reg,len);
	return 0;
		
}

bool sgm_recv_scp_ack(uint8_t cmd)
{
	int ret = 0;
	uint8_t len;
	uint8_t val;
	struct scp_recv_t recv_msg = {0};
	
	memset(&g_recv_scp_msg,0,sizeof(g_recv_scp_msg));
	
	//if (RT_SBRWR_CMD == cmd || RT_MBRWR_CMD == cmd)
		 //ret = TRUE;
	//else
		//ret = sgm_field_read(F_BIT_SCP_RDATA_READY);
	  //ret = sgm_read_byte(0x29, &val);
	ret = sgm_read_byte(0x2E, &val);
	if (0 == ret){
		printf("sgm_recv_scp_ack val = %d\n",val);		
		
		if (val & BIT(6)){			
				recv_msg.ack = SGM41600T_SCP_PROTO_ACK;
		}			
		else
		{
			recv_msg.ack = SGM41600T_SCP_PROTO_NACK;				
			return FALSE;
		}
		
	}
			
		switch (cmd){		
			case RT_SBRWR_CMD://单字节写 ACK				
			case RT_MBRWR_CMD: //多字节写 ACK					
				break;
				
			case RT_SBRRD_CMD://单字节读 ACK
				ret = sgm_read_byte(0x40, &val);
				if (!ret){
					recv_msg.data[0] = val;
				}
				printf("RT_SBRRD_CMD val = %d\n",val);				
				break;
				
			case RT_MBRRD_CMD: //多字节读 ACK
				ret = sgm_read_byte(0x2D, &len);
				if (!ret){
					for (int j = 0;j < len;j++){
						sgm_read_byte(0x40 + j, &recv_msg.data[j]);	
						printf("sgm_recv_scp_ack data = 0x%x,len = %d\n",recv_msg.data[j],len);
					}
					
				}				
				break;
				
			default:
				break;
		}	
	
	memcpy(&g_recv_scp_msg,&recv_msg,sizeof(g_recv_scp_msg));
	
	return TRUE;
	
}

int sgm_en_scp_protocol(uint8_t index,uint32_t *param,uint8_t len)
{
   bool ack = 0;
	 uint8_t val =0;
	 int k = 0;
	 uint8_t word[2] = {0x41,0x60};
	 
	 #if 0 
	 //k = sgm_enter_test_mode();
	 k = sgm_write_word(0xEE,word);
	 if (k)
	 {
			printf("SCPHS: Enter test mode ERROR\n");
			return 0;
	 }
	 #endif
	 //sgm_update_bits(0xF5,BIT(7),BIT(7));	
	//sgm_update_bits(0xF0,BIT(7),BIT(7));
  
  //sgm_update_bits(0xF0,GENMASK(2, 0),0x3);	
	if (1 == len && 1 == param[0])
	{
		sgm_update_bits(0x50,GENMASK(7, 6),BIT(6));
		sgm_update_bits(0x25,GENMASK(0, 0),1);
		//sgm_field_write(F_BIT_EN_PROTOCOL,1);    		
		//sgm_field_write(F_BIT_SCP_DET_EN,1);
	
	}
	else if(1 == len && 0 == param[0]){
		
		sgm_update_bits(0x50,GENMASK(7, 6),0);
		sgm_update_bits(0x25,GENMASK(0, 0),0);
		 //sgm_field_write(F_BIT_EN_PROTOCOL,0);
		 //sgm_field_write(F_BIT_SCP_DET_EN,0);
		 printf("SCPHS: OK\n");
		 return 0;
	}	
	
	k = 0;
	do {
		
		if (k > 10){
			printf("SCPHS: ERROR\n");			
			return -1;
		}		
		delay_1ms(200);
		sgm_read_byte(0x27, &val);
		ack = !!(val & BIT(3));
		printf("SCPHS: k = %d,val = %d\n",k,val);
		k++;
	}while(!ack);
	
	printf("SCPHS: OK\n");
	return 0;
}

#else

int sgm_send_scp_cmd(enum SCP_CMD_T cmd, enum SCP_REG_ADDR reg, uint8_t len,uint8_t *buff)
{
	//int ret = 0;
	struct scp_send_t scp_msg_cmd = {0};
	
	scp_msg_cmd.sbrwr = cmd;	
	scp_msg_cmd.reg   = reg;	
	scp_msg_cmd.len   = len;
	
	sgm_write_byte(0x22,scp_msg_cmd.sbrwr);
	//sgm_write_byte(SGM41600T_PUMP_RT_BUFFER_0,scp_msg_cmd.sbrwr);
	switch (cmd){		
		case RT_SBRWR_CMD://单字节写命令
			memcpy(scp_msg_cmd.data,buff,1);
			sgm_write_byte(0x23,scp_msg_cmd.reg);
		  sgm_write_byte(0x25,scp_msg_cmd.data[0]);
			//sgm_write_byte(SGM41600T_PUMP_RT_BUFFER_1,scp_msg_cmd.reg);
			//sgm_write_byte(SGM41600T_PUMP_RT_BUFFER_3,scp_msg_cmd.data[0]);
			break;
			
		case RT_SBRRD_CMD://单字节读命令
			sgm_write_byte(0x23,scp_msg_cmd.reg);
			//sgm_write_byte(SGM41600T_PUMP_RT_BUFFER_1,scp_msg_cmd.reg);
			break;
			
		case RT_MBRWR_CMD: //多字节写命令	
      sgm_write_byte(0x23,scp_msg_cmd.reg);			
			//sgm_write_byte(SGM41600T_PUMP_RT_BUFFER_1,scp_msg_cmd.reg);			
			if (len && len <= 16 ){				
				memcpy(scp_msg_cmd.data,(uint8_t *)buff,len);
				sgm_write_byte(0x24,scp_msg_cmd.len);
				//sgm_write_byte(SGM41600T_PUMP_RT_BUFFER_2,scp_msg_cmd.len);
				for (int i = 0;i < len;i++){
					//sgm_write_byte(SGM41600T_PUMP_RT_BUFFER_3 + i,scp_msg_cmd.data[i]);
					sgm_write_byte(0x25 + i,scp_msg_cmd.data[i]);
				}
			}			
			break;
			
		case RT_MBRRD_CMD: //多字节读命令
			sgm_write_byte(0x23,scp_msg_cmd.reg);
			//sgm_write_byte(SGM41600T_PUMP_RT_BUFFER_1,scp_msg_cmd.reg);
			if (len){
				sgm_write_byte(0x24,scp_msg_cmd.len);
				//sgm_write_byte(SGM41600T_PUMP_RT_BUFFER_2,scp_msg_cmd.len);
			}
			break;
			
		default:
			break;
	}
	sgm_update_bits(0x1B,GENMASK(1, 1),BIT(1));
	//sgm_field_write(F_BIT_SND_CMD,1);
	if (1 == g_sgm_scp_debug_flag)
		printf("sgm_send_scp_cmd cmd = 0x%x,reg = 0x%x,len = 0x%x\n",cmd,reg,len);
	return 0;
		
}

bool sgm_recv_scp_ack(uint8_t cmd)
{
	int ret = 0;
	uint8_t len;
	uint8_t val;
	struct scp_recv_t recv_msg = {0};
	
	memset(&g_recv_scp_msg,0,sizeof(g_recv_scp_msg));
	
	if (RT_SBRWR_CMD == cmd || RT_MBRWR_CMD == cmd)
		 ret = TRUE;
	else
	{
		ret = sgm_read_byte(0x29, &val);
		
		//ret = sgm_field_read(F_BIT_SCP_RDATA_READY);
	}
		
	if (!ret){
		
		//ret = sgm_read_byte(SGM41600T_PUMP_RT_BUFFER_11, &val);
		//ret = sgm_read_byte(0x2E, &val);
		if (!ret){
			if (SGM41600T_SCP_PROTO_ACK == val)
			{
				recv_msg.ack = SGM41600T_SCP_PROTO_ACK;
			}			
			else if (SGM41600T_SCP_PROTO_NACK == val)
			{
				recv_msg.ack = SGM41600T_SCP_PROTO_NACK;				
				return FALSE;
			}
		}
			
		switch (cmd){		
			case RT_SBRWR_CMD://单字节写 ACK				
			case RT_MBRWR_CMD: //多字节写 ACK					
				break;
				
			case RT_SBRRD_CMD://单字节读 ACK
				ret = sgm_read_byte(SGM41600T_PUMP_RT_BUFFER_12, &val);
				if (!ret){
					recv_msg.data[0] = val;
				}				
				break;
				
			case RT_MBRRD_CMD: //多字节读 ACK
				ret = sgm_read_byte(SGM41600T_PUMP_RT_BUFFER_2, &len);
				if (!ret){
					for (int j = 0;j < len;j++){
						sgm_read_byte(SGM41600T_PUMP_RT_BUFFER_12 + j, &recv_msg.data[j]);	
					}
					
				}				
				break;
				
			default:
				break;
		}
	}
	else{
		return FALSE;
	}
	
	memcpy(&g_recv_scp_msg,&recv_msg,sizeof(g_recv_scp_msg));
	
	return TRUE;
	
}

int sgm_en_scp_protocol(uint8_t index,uint32_t *param,uint8_t len)
{
   bool ack = 0;
	 int k = 0;
	 #if 0 
	 k = sgm_enter_test_mode();
	 if (1 == k)
	 {
			printf("SCPHS: ERROR\n");
			return 0;
	 }
	 #endif
	//sgm_update_bits(0xF3,BIT(7),0);	
	//sgm_update_bits(0xF0,BIT(7),BIT(7));
  
  //sgm_update_bits(0xF0,GENMASK(2, 0),0x3);	
	if (1 == len && 1 == param[0])
	{
		sgm_field_write(F_BIT_EN_PROTOCOL,1);	
		sgm_field_write(F_BIT_SCP_DET_EN,1);
	
	}
	else if(1 == len && 0 == param[0]){
		 sgm_field_write(F_BIT_EN_PROTOCOL,0);
		 sgm_field_write(F_BIT_SCP_DET_EN,0);
		 printf("SCPHS: OK\n");
		 return 0;
	}
	
	//delay_1ms(1000);
	//sgm_field_write(F_BIT_DCP_EN,1);
	do {
		
		if (k > 6){
			printf("SCPHS: ERROR\n");			
			return -1;
		}		
		delay_1ms(200);
		ack = sgm_field_read(F_BIT_EN_HAND_SUCCESS);
		k++;
	}while(!ack);
	
	printf("SCPHS: OK\n");
	return 0;
}
#endif
int sgm_set_scp_output_mode()
{
		uint8_t val[2] = {0xC1,0x00};
		bool ack = 0;
		uint8_t k = 0;
		
		sgm_send_scp_cmd(RT_MBRWR_CMD,SCP_CTRL_BYTE0,2,val);
		
		do {
				delay_1ms(10);				
				if (k > 10){
					return -1;
				}
				k++;
				ack = sgm_recv_scp_ack(RT_MBRWR_CMD);

		}while(!ack);
		s_scp_output_mode = 1;
		if (1 == g_sgm_scp_debug_flag)
				printf("sgm_set_scp_output_mode,ack=%d\n",ack);
		return 0;
}

int sgm_get_scp_dev_type()
{
		uint8_t ack = 0;
		uint8_t data[128] = {0};
		uint8_t k = 0;
		uint8_t i = 0;
		int8_t flag = 0;
#ifdef SGM41600A_TEST_SCP		
		sgm_read_byte(0x27, &ack);
		flag = !!(ack & BIT(3));	
#else		
		flag = sgm_field_read(F_BIT_EN_HAND_SUCCESS);
#endif
		if (!flag){
				printf("GADPT: HAND ERROR\n");
				return 0;
		}			
		
		memcpy(data,"GADPT:",6);
		sgm_send_scp_cmd(RT_SBRRD_CMD, SCP_ADP_TYPE0,0,NULL);
		
		do{
				delay_1ms(10);
				ack = sgm_recv_scp_ack(RT_SBRRD_CMD);
				
				if (ack)	break;									
				k++;		
			
		}while(k < 5);
    
		//printf("get adapter ack=%d:0x%x\n",ack,g_recv_scp_msg.data[0]);
		if (ack)	{
				if ((BIT(7)|BIT(5)|BIT(4)) == ((BIT(7)|BIT(5)|BIT(4)) & g_recv_scp_msg.data[0])){			
						snprintf(data+strlen(data),128,"standard scp class-AB,");
				}	
				else if ((BIT(7)|BIT(4)) == ((BIT(7)|BIT(4)) & g_recv_scp_msg.data[0])){
						snprintf(data+strlen(data),128,"standard scp class-B,");
				}
				else if ((BIT(7)|BIT(5)) == ((BIT(7)|BIT(5)) & g_recv_scp_msg.data[0])){
						snprintf(data+strlen(data),128,"standard scp class-A,");
				}
				
				if (1 == !!(BIT(3) & g_recv_scp_msg.data[0]) &&
						0 == !!(BIT(2) & g_recv_scp_msg.data[0])){
						snprintf(data+strlen(data),128,"High voltage class-B,");
						
				}
				else if (0 == !!(BIT(3) & g_recv_scp_msg.data[0]) &&
								 0 == !!(BIT(2) & g_recv_scp_msg.data[0])){
						snprintf(data+strlen(data),128,"Low voltage class-B,");
						
				}
				else if (0 == !!(BIT(3) & g_recv_scp_msg.data[0]) &&
								 1 == !!(BIT(2) & g_recv_scp_msg.data[0])){
						snprintf(data+strlen(data),128,"High voltage class-A,");
						
				}
				else {
						snprintf(data+strlen(data),128,"Low voltage class-A,");						
				}
				
				if (0 == ((BIT(1)|BIT(0)) & g_recv_scp_msg.data[0])){
					  snprintf(data+strlen(data),128,"type-A");						
				}
				else if(1 == ((BIT(1)|BIT(0)) & g_recv_scp_msg.data[0])){
						snprintf(data+strlen(data),128,"type-C");
				}		
		
		}		
		
		if (ack)	{
				printf("%s\n",data);
			  return 0;
		}
				
		k = 0;
		ack = 0;	
		for(i = 0;i < 3;i++){
			sgm_send_scp_cmd(RT_SBRRD_CMD, SCP_ADP_TYPE1,0,NULL);
			
			do{
					delay_1ms(10);
					ack = sgm_recv_scp_ack(RT_SBRRD_CMD);
					
					if (ack)	break;									
					k++;		
				
			}while(k < 5);		
			if (ack)	break;
			
		};
		
		if (!ack){
				printf("GADPT: ERROR\n");
				return 0;
		}
			
		
		if (BIT(7) & g_recv_scp_msg.data[0]){
			snprintf(data+strlen(data),128,"standard scp,");			
		}
		else{
			snprintf(data+strlen(data),128,"Non_standard scp,");				
		}
		
		if (BIT(4) & g_recv_scp_msg.data[0]){
			snprintf(data+strlen(data),128,"class-B,");			
		}
		else {
			snprintf(data+strlen(data),128,"Non class-B,");			
		}
		
		if (1 == ((BIT(1)|BIT(0)) & g_recv_scp_msg.data[0])){
			snprintf(data+strlen(data),128,"Type-C");			
		}
		else{
			snprintf(data+strlen(data),128,"Type-A");
		}
		printf("%s\n",data);
		return 0;	
}

int sgm_get_scp_version_info()
{
	bool ack = 0;	
	uint8_t k = 0;
	uint8_t data[2] = {0};	
	
	sgm_send_scp_cmd(RT_SBRRD_CMD, SCP_COMPILEDVER_H,0,NULL);
	
	do{
		delay_1ms(10);
		ack = sgm_recv_scp_ack(RT_SBRRD_CMD);
		
		if (ack){			
			break;
		}
		
		k++;		
		
	}while(k < 5);
	data[0] = g_recv_scp_msg.data[0];
	
	k = 0;
	ack = 0;
	sgm_send_scp_cmd(RT_SBRRD_CMD, SCP_COMPILEDVER_L,0,NULL);
	
	do{
		delay_1ms(10);
		ack = sgm_recv_scp_ack(RT_SBRRD_CMD);
		
		if (ack){			
			break;
		}
		
		k++;		
		
	}while(k < 5);
	data[1] = g_recv_scp_msg.data[0];		
	printf("version = %d.%d.%d\n",data[0],(data[1]&GENMASK(7,4))>>4,data[1]&GENMASK(3,0));
	return 0;	
}


int sgm_get_scp_adater_info()
{
	bool ack = 0;	
	uint8_t k = 0;
	uint8_t b_adp_type = 0;
	uint8_t data[8] = {0};
	uint16_t vendor_id = 0;	
	uint16_t moudle_id = 0;
	uint16_t serrial_id = 0;
	uint8_t chip_id = 0;
	uint8_t hwver = 0;
	uint16_t fwver = 0;
	uint8_t adp_b_type1 = 0;
	uint8_t factory_id = 0;
	
	sgm_send_scp_cmd(RT_SBRRD_CMD, SCP_B_ADP_TYPE,0,NULL);
	
	do{
		delay_1ms(10);
		ack = sgm_recv_scp_ack(RT_SBRRD_CMD);
		
		if (ack){			
			break;
		}
		
		k++;		
		
	}while(k < 5);
		
	if (ack){
		b_adp_type = g_recv_scp_msg.data[0];		
	};
	if (1 == g_sgm_scp_debug_flag)
			printf("get adapter :k = %d,b_adp_type = 0x%x\n",k,b_adp_type);
  
	k = 0;
	ack = 0;
	sgm_send_scp_cmd(RT_MBRRD_CMD, SCP_VENDER_ID_H,8,NULL);
	
	do{
		delay_1ms(10);
		ack = sgm_recv_scp_ack(RT_MBRRD_CMD);
		
		if (ack){			
			break;
		}
		
		k++;		
		
	}while(k < 5);		
	
	memcpy(data,g_recv_scp_msg.data,8);
	if (1 == g_sgm_scp_debug_flag){
			printf("adapter info[%d,%d,%d,%d,%d,%d,%d,%d]\n",data[0],data[1],
							data[2],data[3],data[4],data[5],data[6],data[7]);
	}
	vendor_id = (data[0]<<8)|data[1];
	moudle_id = (data[2]<<8)|data[3];	
	serrial_id = (data[4]<<8)|data[5];
	chip_id = data[6];
	hwver = data[7];
	
	printf("vendor_id[0x%x]moudle_id[0x%x]serrial_id[0x%x]chip_id[0x%x]hwver[0x%x]\n",
					vendor_id,moudle_id,serrial_id,chip_id,hwver);
	
	k = 0;
	ack = 0;
	sgm_send_scp_cmd(RT_MBRRD_CMD, SCP_FWVER_H,2,NULL);
	
	do{		
		delay_1ms(10);
		ack = sgm_recv_scp_ack(RT_MBRRD_CMD);
		
		if (ack){			
			break;
		}
		
		k++;		
		
	}while(k < 5);
	
	fwver = g_recv_scp_msg.data[0]<<8|g_recv_scp_msg.data[1];
	
	k = 0;
	ack = 0;
	sgm_send_scp_cmd(RT_MBRRD_CMD, SCP_ADP_B_TYPE1-1,4,NULL);
	k = 0;
	do{		
		delay_1ms(10);
		ack = sgm_recv_scp_ack(RT_MBRRD_CMD);
		
		if (ack){			
			break;
		}
		
		k++;		
		
	}while(k < 5);
	
	adp_b_type1 = g_recv_scp_msg.data[1];
	factory_id = g_recv_scp_msg.data[2];
	
	printf("fwver[0x%x]adp_b_type1[0x%x]factory_id[0x%x]\n",
					fwver,adp_b_type1,factory_id);
	return 0;	
}

int mult10(int n)
{
	int sum = 10;
	int m;
	
	if (0 == n) return 1;
	
	for (m=1;m<n;m++) sum *= 10;
	
	return sum;
}

int openroot10(int sum)
{
	int n = 10;
	int m;
	
	if (1 == sum) return 0;
	
	for (m=1;m<4;m++) {
		sum = sum/n;
		if (sum < 1)
			break;
	}
	return m -1;
}

int sgm_get_scp_spec_param()
{
	bool ack = 0;
	uint8_t k = 0;
	uint8_t temp = 0;
	uint8_t data[16] = {0};
	uint32_t max_pwr = 0;
	uint32_t cnt_pwr = 0;
	uint32_t min_vout = 0;
	uint32_t max_vout = 0;
	uint32_t min_iout = 0;
	uint32_t max_iout = 0;
	uint32_t vstep = 0;
	uint32_t istep = 0;
	uint32_t max_verr = 0;
	uint32_t max_ierr = 0;
	uint32_t max_sttime = 0;
	uint32_t max_rsptime = 0;
	
	sgm_send_scp_cmd(RT_MBRRD_CMD, SCP_MAX_PWR,8,NULL);
	
	do{
		delay_1ms(10);
		ack = sgm_recv_scp_ack(RT_MBRRD_CMD);
		if (ack){			
			break;
		}
		
		k++;		
		
	}while(k<5);
	memcpy(data,g_recv_scp_msg.data,8);
	
	k = 0;
	ack = 0;
	sgm_send_scp_cmd(RT_MBRRD_CMD, SCP_MAX_VERR,4,NULL);
	
	do{
		delay_1ms(10);
		ack = sgm_recv_scp_ack(RT_MBRRD_CMD);
		if (ack){			
			break;
		}
		
		k++;		
		
	}while(k<5);
	memcpy(data+8,g_recv_scp_msg.data,4);
	
	temp = !!(data[0] & BIT(7));
	max_pwr = (data[0] & GENMASK(6, 0))* mult10(temp)/10;	//0.1W	
	
	temp = !!(data[1] & BIT(7));
	cnt_pwr = (data[1] & GENMASK(6, 0))* mult10(temp)/10;	//0.1W
	
	temp = (data[2] & GENMASK(7, 6))>>6;
	min_vout = (data[2] & GENMASK(5, 0)) * mult10(temp); //1mV
		
	temp = (data[3] & GENMASK(7, 6))>>6;
	max_vout = (data[3] & GENMASK(5, 0)) * mult10(temp); //1mV
	
	printf("max_pwr[%d],cnt_pwr[%d],min_vout[%d],max_vout[%d]\n",max_pwr,cnt_pwr,min_vout,max_vout);
	
	temp = (data[4] & GENMASK(7, 6))>>6;
	min_iout = (data[4] & GENMASK(5, 0)) * mult10(temp); //1mA
		
	temp = (data[5] & GENMASK(7, 6))>>6;
	max_iout = (data[5] & GENMASK(5, 0)) * mult10(temp); //1mA
	
	temp = !!(data[6] & BIT(7));
	vstep = (data[6] & GENMASK(6, 0))* mult10(temp);	 //1mV	
	
	temp = !!(data[7] & BIT(7));
	istep = (data[7] & GENMASK(6, 0))* mult10(temp);     //1mA
	
	printf("min_iout[%d],max_iout[%d],vstep[%d],istep[%d]\n",min_iout,max_iout,vstep,istep);
	
	temp = !!(data[8] & BIT(7));
	max_verr = (data[8] & GENMASK(6, 0))* mult10(temp);	 //1mV	
	
	temp = !!(data[9] & BIT(7));
	max_ierr = (data[9] & GENMASK(6, 0))* mult10(temp);   //1mA
	
	temp = !!(data[10] & BIT(7));
	max_sttime = (data[10] & GENMASK(6, 0))* mult10(temp);	  //1mS	
	
	temp = !!(data[11] & BIT(7));
	max_rsptime = (data[11] & GENMASK(6, 0))* mult10(temp);   //1mS
	
	printf("max_verr[%d],max_ierr[%d],max_sttime[%d],max_rsptime[%d]\n",max_verr,max_ierr,max_sttime,max_rsptime);
	
	return 0;	
}

int sgm_get_scp_ctrl_info()
{
	bool ack = 0;	
	int k = 0;
	
	sgm_send_scp_cmd(RT_MBRRD_CMD, SCP_CTRL_BYTE0,2,NULL);
	do{
		delay_1ms(10);
		ack = sgm_recv_scp_ack(RT_MBRRD_CMD);
		if (ack){			
			break;
		}
		
		k++;		
		
	}while(k<5);
	
	//*ctrl_0 = g_recv_scp_msg.data[0];
	//*ctrl_1 = g_recv_scp_msg.data[1];
	
	printf("ctrl_0[%d],ctrl_1[%d]\n",g_recv_scp_msg.data[0],g_recv_scp_msg.data[1]);
	return 0;
}

int sgm_get_scp_dev_status()
{
	bool ack = 0;
	uint8_t k = 0;	
	uint8_t data[16] = {0};
	uint8_t status_0 = 0;
	uint8_t status_1 = 0;
	uint8_t status_2 = 0;
	uint8_t ssts = 0;
	uint8_t inside_tmp = 0;
	uint8_t port_tmp = 0;
	uint16_t read_vout = 0;
	uint16_t read_iout = 0;	
	uint16_t dac_vset = 0;
	uint16_t dac_iset = 0;
	//uint32_t max_rsptime = 0;

	sgm_send_scp_cmd(RT_MBRRD_CMD, SCP_STATUS_BYTE0,8,NULL);
	
	do{
		delay_1ms(10);
		ack = sgm_recv_scp_ack(RT_MBRRD_CMD);
		if (ack){			
			break;
		}
		
		k++;		
		
	}while(k<5);
	
	if (ack){
			memcpy(data,g_recv_scp_msg.data,8);
			
			status_0 = data[0];
			status_1 = data[1];
			status_2 = data[2];
			ssts = data[3];
			inside_tmp = data[4];
			port_tmp = data[5];
			read_vout = data[6]<<8|data[7];
			
			printf("status_0[%d]status_1[%d]status_2[%d]ssts[%d]inside_tmp[%d]port_tmp[%d]read_vout[%d]\n",
					status_0,status_1,status_2,ssts,inside_tmp,port_tmp,read_vout);
	}
	
	k = 0;
	ack = 0;
	sgm_send_scp_cmd(RT_MBRRD_CMD, SCP_READ_IOUT_H,6,NULL);
	
	do{
		delay_1ms(10);
		ack = sgm_recv_scp_ack(RT_MBRRD_CMD);
		if (ack){			
			break;
		}
		
		k++;		
		
	}while(k<5);
	if (ack){
			memcpy(data+8,g_recv_scp_msg.data,6);
			
			read_iout = data[8]<<8|data[9];
			dac_vset = data[10]<<8|data[11];
			dac_iset = data[12]<<8|data[13];
			
			printf("read_iout[%d]dac_vset[%d]dac_iset[%d]\n",
					read_iout,dac_vset,dac_iset);
	}
	
	return 0;	
}

int sgm_get_scp_config_info()
{
	bool ack = 0;
	uint8_t k = 0;	
	uint8_t data[16] = {0};
	uint16_t vset_boundary = 0;
	uint16_t iset_boundary = 0;
	uint16_t max_vset_offset = 0;
	uint16_t max_iset_offset = 0;
	uint16_t vset = 0;
	uint16_t iset = 0;
	uint8_t temp;
	
	sgm_send_scp_cmd(RT_MBRRD_CMD, SCP_VSET_BOUNDARY_H,8,NULL);
	
	do{
		delay_1ms(10);
		ack = sgm_recv_scp_ack(RT_MBRRD_CMD);
		if (ack){			
			break;
		}
		
		k++;		
		
	}while(k<5);
	
	if (ack){
			memcpy(data,g_recv_scp_msg.data,8);
			
			vset_boundary = (data[0]<<8)|data[1];			
			iset_boundary = (data[2]<<8)|data[3];	
		
			temp = (data[4] & GENMASK(7, 6))>>6;
			max_vset_offset = (data[4] & GENMASK(5, 0)) * mult10(temp); //1mV

			temp = (data[5] & GENMASK(7, 6))>>6;
			max_iset_offset = (data[5] & GENMASK(5, 0)) * mult10(temp); //1mA
						
			printf("vset_boundary[%d]iset_boundary[%d]max_vset_offset[%d]max_iset_offset[%d]\n",
					vset_boundary,iset_boundary,max_vset_offset,max_iset_offset);
	}

	k = 0;
	ack = 0;
	sgm_send_scp_cmd(RT_MBRRD_CMD, SCP_VSET_H,4,NULL);
	
	do{
		delay_1ms(10);
		ack = sgm_recv_scp_ack(RT_MBRRD_CMD);
		if (ack){			
			break;
		}
		
		k++;		
		
	}while(k<5);
	
	if (ack){
			memcpy(data+8,g_recv_scp_msg.data,4);
			
			vset = (data[8]<<8)|data[9];			
			iset = (data[10]<<8)|data[11];			
			
			printf("vset[%d]iset[%d]\n",vset,iset);
	}
	return 0;
}

int sgm_get_scp_info()
{
	sgm_get_scp_version_info();
	sgm_get_scp_adater_info();
	sgm_get_scp_spec_param();
	sgm_get_scp_ctrl_info();
	sgm_get_scp_dev_status();
	sgm_get_scp_config_info();
	
	//sgm_set_scp_output_mode();
	//sgm_enter_test_mode();
	//sgm_update_bits(0xF3,BIT(7),BIT(7));	
	return 0;
}

int sgm_config_scp_param(uint8_t addr,uint8_t *param,uint8_t len)
{
	bool ack = 0;
	uint8_t k = 0;	
		
	if (len & len < 2){
			sgm_send_scp_cmd(RT_SBRWR_CMD, addr,0,param);
			
			do{
				delay_1ms(10);
				ack = sgm_recv_scp_ack(RT_SBRWR_CMD);
				if (ack){	
					printf("Single addr[0x%x] len=%d set success \n",addr,len);			
					break;
				}
				
				k++;		
				
			}while(k<5);
	}	
	
	if (len > 1){
	
			k = 0;
			ack = 0;
			sgm_send_scp_cmd(RT_MBRWR_CMD, addr,len,param);
			
			do{
				delay_1ms(10);
				ack = sgm_recv_scp_ack(RT_MBRWR_CMD);
				if (ack){	
					printf("Mult addr[0x%x] len=%d set success \n",addr,len);
					break;
				}
				
				k++;		
				
			}while(k<5);
	}
	return 0;
}

int sgm_set_scp_ctrl_info(uint8_t ctrl_0,uint8_t ctrl_1)
{
	bool ack = 0;
	int k =0;
	uint8_t buff[2] = {0};
	
	buff[0] = ctrl_0;
	buff[1] = ctrl_1;
	
	sgm_send_scp_cmd(RT_MBRWR_CMD, SCP_CTRL_BYTE0,2,buff);
	do{
		delay_1ms(10);
		ack = sgm_recv_scp_ack(RT_MBRWR_CMD);
		if (k > 10){			
			return -1;
		}
		k++;		
		
	}while(!ack);
	
	return 0;
}




int sgm_set_scp_volt_boundary(uint16_t mV)
{
	bool ack = 0;
	uint8_t k = 0;
	uint8_t buff[2] = {0};
	
	buff[0] = mV>>8;
	buff[1] = mV&0xFF;
	
	sgm_send_scp_cmd(RT_MBRWR_CMD, SCP_VSET_BOUNDARY_H,2,buff);	
	
	do{
		delay_1ms(10);
		ack = sgm_recv_scp_ack(RT_MBRWR_CMD);
		if (k > 10){			
			return -1;
		}
		k++;		
		
	}while(!ack);
	
	return 0;
}

int sgm_set_scp_max_volt_offset(uint32_t mV)
{
	bool ack = 0;
	uint8_t k = 0;
	uint8_t val = 0;
	
	if (mV <= 0x3F) //bit[5:0]
		val = (uint8_t)mV;
	else if (mV <= 0x3F *10){
		val = BIT(6)|mV;
	}
	else if (mV <= 0x3F *100){
		val = BIT(7)|mV;
	}
	else if (mV <= 0x3F *1000){
		val = BIT(7)|BIT(6)|mV;
	}
	sgm_send_scp_cmd(RT_SBRWR_CMD, SCP_VSET_BOUNDARY_H,0,&val);
	
	
	do{
		delay_1ms(10);
		ack = sgm_recv_scp_ack(RT_SBRWR_CMD);
		if (k > 10){			
			return -1;
		}
		k++;		
		
	}while(!ack);
	
	return 0;
}

int sgm_set_scp_volt_offset(uint16_t mV)
{
	bool ack = 0;
	uint8_t k = 0;
	uint8_t val[2] = {0};

	val[0] = mV >> 8;
	val[1] = mV & 0xFF;
	
	sgm_send_scp_cmd(RT_MBRWR_CMD, SCP_VSET_OFFSET_H,2,val);	
	
	do{
		delay_1ms(10);
		ack = sgm_recv_scp_ack(RT_MBRWR_CMD);
		if (k > 10){			
			return -1;
		}
		k++;		
		
	}while(!ack);
	
	return 0;
}

int sgm_set_scp_volt_offset_step(int8_t mV)
{
	bool ack = 0;
	uint8_t k = 0;
	uint8_t val[1] = {0};
	
	if (mV < 0)
		val[0] = val[0] | BIT(7) | (abs(mV)&0x7F);
	else
		val[0] = mV;	
	
	sgm_send_scp_cmd(RT_SBRWR_CMD, SCP_STEP_VSET_OFFSET,0,val);	
	
	do{
		delay_1ms(10);
		ack = sgm_recv_scp_ack(RT_SBRWR_CMD);
		if (k > 10){			
			return -1;
		}
		k++;		
		
	}while(!ack);
	
	return 0;
}

int sgm_set_scp_curr_boundary(uint8_t *mA)
{
	bool ack = 0;
	uint8_t k = 0;
	uint8_t buff[2] = {0};
	
	buff[0] = *mA>>8;
	buff[1] = *mA&0xFF;
	
	sgm_send_scp_cmd(RT_MBRWR_CMD, SCP_ISET_BOUNDARY_H,2,buff);	
	
	do{
		delay_1ms(10);
		ack = sgm_recv_scp_ack(RT_MBRWR_CMD);
		if (k > 10){			
			return -1;
		}
		k++;		
		
	}while(!ack);
	
	return 0;
}

int sgm_set_scp_max_curr_offset(uint32_t mA)
{
	bool ack = 0;
	uint8_t k = 0;
	uint8_t val = 0;
	
	if (mA <= 0x3F) //bit[5:0]
		val = (uint8_t)mA;
	else if (mA <= 0x3F *10){
		val = BIT(6)|mA;
	}
	else if (mA <= 0x3F *100){
		val = BIT(7)|mA;
	}
	else if (mA <= 0x3F *1000){
		val = BIT(7)|BIT(6)|mA;
	}
	sgm_send_scp_cmd(RT_SBRWR_CMD, SCP_ISET_BOUNDARY_H,0,&val);	
	
	do{
		delay_1ms(10);
		ack = sgm_recv_scp_ack(RT_SBRWR_CMD);
		if (k > 10){			
			return -1;
		}
		k++;		
		
	}while(!ack);
	
	return 0;
}

int sgm_set_scp_curr_offset(uint16_t mA)
{
	bool ack = 0;
	uint8_t k = 0;
	uint8_t val[2] = {0};

	val[0] = mA >> 8;
	val[1] = mA & 0xFF;
	
	sgm_send_scp_cmd(RT_MBRWR_CMD, SCP_ISET_OFFSET_H,2,val);	
	
	do{
		delay_1ms(10);
		ack = sgm_recv_scp_ack(RT_MBRWR_CMD);
		if (k > 10){			
			return -1;
		}
		k++;		
		
	}while(!ack);
	
	return 0;
}

int sgm_set_scp_curr_offset_step(int8_t mA)
{
	bool ack = 0;
	uint8_t k = 0;
	uint8_t val[1] = {0};
	
	if (mA < 0)
		val[0] = val[0] | BIT(7) | (abs(mA)&0x7F);
	else
		val[0] = mA;	
	
	sgm_send_scp_cmd(RT_SBRWR_CMD, SCP_STEP_ISET_OFFSET,0,val);	
	
	do{
		delay_1ms(10);
		ack = sgm_recv_scp_ack(RT_SBRWR_CMD);
		if (k > 10){			
			return -1;
		}
		k++;		
		
	}while(!ack);
	
	return 0;
}

int sgm_set_scp_output_volt(uint32_t mV)
{
	bool ack = 0;
	uint8_t k = 0;
	uint8_t buff[2] = {0};
	
	buff[0] = mV>>8;
	buff[1] = mV&0xFF;
	
	sgm_send_scp_cmd(RT_MBRWR_CMD, SCP_VSET_H,2,buff);	
	
	do{
		delay_1ms(10);
		ack = sgm_recv_scp_ack(RT_MBRWR_CMD);
		if (k > 10){			
			return -1;
		}
		k++;		
		
	}while(!ack);
	
	return 0;
}

int sgm_set_scp_output_curr(uint32_t mA)
{
	bool ack = 0;
	uint8_t k = 0;
	uint8_t buff[2] = {0};
	
	buff[0] = mA>>8;
	buff[1] = mA&0xFF;
	
	sgm_send_scp_cmd(RT_MBRWR_CMD, SCP_ISET_H,2,buff);	
	
	do{
		delay_1ms(10);
		ack = sgm_recv_scp_ack(RT_MBRWR_CMD);
		if (k > 10){			
			return -1;
		}
		k++;		
		
	}while(!ack);
	
	return 0;
}

#if 1
int sgm_get_scp_dev_single_vout(){
	
	uint8_t sread_vout = 0;	
	uint32_t vout = 0;
	bool ack = 0;
	int k =0;
	sgm_send_scp_cmd(RT_SBRRD_CMD, SCP_SREAD_VOUT,0,NULL);
	
	do{
		delay_1ms(10);
		ack = sgm_recv_scp_ack(RT_SBRRD_CMD);
		if (k > 10){			
			return -1;
		}
		k++;		
		
	}while(!ack);
	
	sread_vout = g_recv_scp_msg.data[0];
	vout = (sread_vout + 225)*10 + 3000;	
	
	printf("get single_vout :%d\n",vout);
	
	return 0;
}

int sgm_set_scp_dev_single_vout(uint32_t vout){	
	
	uint8_t val = 0;
	bool ack = 0;
	int k = 0;
	val = (vout -3000)/10;
	
	//fix
	sgm_send_scp_cmd(RT_SBRWR_CMD, SCP_VSSET,0,&val);
	
	do{
		delay_1ms(10);
		ack = sgm_recv_scp_ack(RT_SBRWR_CMD);
		if (k > 10){			
			return -1;
		}
		k++;		
		
	}while(!ack);	
	
	
	printf("Set single_vout :%d\n",vout);
	
	return 0;
}

int sgm_get_scp_dev_single_iout(){	
	
	uint32_t iout = 0;
	bool ack = 0;
	int k = 0;
	sgm_send_scp_cmd(RT_SBRRD_CMD, SCP_SREAD_IOUT,0,NULL);
	
	do{
		delay_1ms(10);
		ack = sgm_recv_scp_ack(RT_SBRRD_CMD);
		if (k > 10){			
			return -1;
		}
		k++;		
		
	}while(!ack);	
	
	iout = g_recv_scp_msg.data[0]*50;  //mA
	
	printf("get single_iout :%d\n",iout);
	
	return 0;
}

int sgm_set_scp_dev_single_iout(uint32_t iout){	
	
	uint8_t val = 0;
	bool ack = 0;
	int k = 0;
	val = iout/50;
	
	//fix
	sgm_send_scp_cmd(RT_SBRWR_CMD, SCP_ISSET,0,&val);
	
	do{
		delay_1ms(10);
		ack = sgm_recv_scp_ack(RT_SBRWR_CMD);
		if (k > 10){			
			return -1;
		}
		k++;		
		
	}while(!ack);	
	
	
	printf("Set single_Iout :%d\n",iout);
	
	return 0;
}
#endif


int sgm_scp_write_operation(uint8_t index,uint32_t *param,uint8_t len)
{ 
		int ack = 0;
		uint8_t data[128] = {0};
		int k = 0;
		
		if (!s_scp_output_mode){		
				ack = sgm_set_scp_output_mode();
				if (ack < 0)
				{
						printf("WCMD: ERROR\n");
						return 0;
				}	
		}				
		
    for(k = 0;k < len - 2; k++)
		{
			data[k] = param[k + 2];
			printf("WCMD: %d \n",param[k + 2]);
		}
		
		if (len == 3 && param[0] == RT_SBRWR_CMD)
				sgm_send_scp_cmd(param[0],param[1],len - 2,data);	
		else if (len > 3 && param[0] == RT_MBRWR_CMD)
				sgm_send_scp_cmd(param[0],param[1],len - 2,data);
		else{
				printf("WCMD: ERROR\n");
				return 0;
		}
		if (1 == g_sgm_scp_debug_flag)				
				printf("scp cmd param[0]=%x,param[1]=%x,param[2]=%x,len=%d\n",param[0],param[1],param[2],len);
		
		do{
				delay_1ms(10);
				ack = sgm_recv_scp_ack(param[0]);
				if (ack){			
					break;
				}
				
				k++;		
			
		}while(k<5);
		
		if (ack)
		{
				printf("WCMD: OK\n");
				return 0;
		}
		else
		{
				printf("WCMD: ERROR\n");
				return 0;
		}		
	
}

int sgm_scp_read_operation(uint8_t index,uint32_t *param,uint8_t len)
{ 
	bool ack = 0;
	uint8_t data[128] = {0};
	int k = 0;
		
	if (len == 2 && param[0] == RT_SBRRD_CMD)
		sgm_send_scp_cmd(param[0],param[1],0,NULL);	
	else if (len == 3 && param[0] == RT_MBRRD_CMD)
		sgm_send_scp_cmd(param[0],param[1],param[2],NULL);
	else{
		printf("RCMD: ERROR\n");
		return 0;
	}
	if (1 == g_sgm_scp_debug_flag)		
			printf("scp cmd param[0]=%x,param[1]=%x,param[2]=%x,len=%d\n",param[0],param[1],param[2],len);
	
	do{
		delay_1ms(10);
		ack = sgm_recv_scp_ack(param[0]);
		if (ack){			
			break;
		}
		
		k++;		
		
	}while(k<5);
	
	if (0 == ack){
			printf("RCMD: ERROR\n");
			return 0;
	}
	memcpy(data,"RCMD:",5);
  if (param[0] == RT_SBRRD_CMD){
		printf("%s[%d]\n",data,g_recv_scp_msg.data[0]);
		return 0;
	}
			
	for (k = 0;k < param[2] && ack == 1;k++){
		snprintf(data+strlen(data),128,"[%d]",g_recv_scp_msg.data[k]);		
	}
	printf("%s\n",data);
	return 0;
	
}

int sgm_scp_debug_info(uint8_t index,uint32_t *param,uint8_t len)
{
	//s_scp_run_state = DEBUG_STATE;
	if (1 == len){
		g_sgm_scp_debug_flag = param[0];
	}
	return 0;
	
}

#if 0
void scp_state_machine()
{
		uint8_t irs_1,irs_2,irs_3;
		uint8_t dev_id;
		
		switch (s_scp_run_state) {
		
		case SEND_STATE:
				break;
		case RECV_STATE:			
			break;
		case DEBUG_STATE:	
			{
				sgm_status_fault();
				sgm_read_byte(SGM41600T_PUMP_SCP_ISR_1,&irs_1);
				sgm_read_byte(SGM41600T_PUMP_SCP_ISR_2,&irs_2);
				sgm_read_byte(SGM41600T_PUMP_DCP_ISR,&irs_3);
				dev_id = sgm_field_read(F_BIT_DEV_ID);
				
				printf("scp_state_machine dev_id:0x%x, irs_1:0x%x,irs_2:0x%x,irs_3:0x%x\n",dev_id,irs_1,irs_2,irs_3);
				sgm_get_adc();							
				sgm_read_byte(SGM41600T_PUMP_DCP_CTL,&irs_1);
				s_scp_run_state = IDLE;
			}			
			break;
		case IDLE:			
			break;
		default:
			break;
	
		}
		return;
}

#endif
