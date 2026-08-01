#ifndef _SGM41600T_SCP_PROTOCOL_H
#define _SGM41600T_SCP_PROTOCOL_H

#include <stdint.h>

#define MAX_SCP_PROTO_LEN                     (16)

#define SGM41600T_SCP_PROTO_ACK               (0x08)
#define SGM41600T_SCP_PROTO_NACK              (0x03)

#define SGM41600T_SCP_PROTO_ADAPTER           BIT(7)
#define SGM41600T_SCP_ADAPTER_MIN_TYPE        GENMASK(5,4)
#define SGM41600T_SCP_CHG_INTERFACE_TYPE      GENMASK(1,0)

#define SGM41600T_SCP_ADP_B_TYPE1             (0x82)

#define RT_BUFF_MAX_LEN                       (8)


enum SCP_REG_ADDR{
	//GET DEV TYPE INFO CMD
	
	SCP_COMPILEDVER_H = 0x7C,
	SCP_COMPILEDVER_L = 0x7D,
	SCP_ADP_TYPE0 = 0x7E,
	SCP_ADP_TYPE1 = 0x80,
	
	//GET DEV INFO CMD
	SCP_B_ADP_TYPE,
	SCP_VENDER_ID_H,
	SCP_VENDER_ID_L,
	SCP_MODULE_ID_H,
	SCP_MODULE_ID_L,
	SCP_SERRIAL_NO_H,
	SCP_SERRIAL_NO_L,
	SCP_CHIP_ID,
	SCP_HWVER,
	SCP_FWVER_H,
	SCP_FWVER_L,
	SCP_ADP_B_TYPE1 = 0x8D,
	SCP_FACTORY_ID,
	
	//GET Index specification INFO CMD
	SCP_MAX_PWR = 0x90,
	SCP_CNT_PWR,
	SCP_MIN_VOUT,
	SCP_MAX_VOUT,
	SCP_MIN_IOUT,
	SCP_MAX_IOUT,
	SCP_VSTEP,
	SCP_ISTEP,
	SCP_MAX_VERR,
	SCP_MAX_IERR,
	SCP_MAX_STTIME,
	SCP_MAX_RSPTIME,
	
	//CTRL INFO CMD
	SCP_CTRL_BYTE0 = 0xA0,
	SCP_CTRL_BYTE1,
	
	//GET DEV STATUS INFO CMD
	SCP_STATUS_BYTE0,
	SCP_STATUS_BYTE1,
	SCP_STATUS_BYTE2,
	SCP_SSTS,
	SCP_INSIDE_TMP,
	SCP_PORT_TMP,
	SCP_READ_VOUT_H,
	SCP_READ_VOUT_L,
	SCP_READ_IOUT_H,
	SCP_READ_IOUT_L,	
	SCP_DAC_VSET_H,
	SCP_DAC_VSET_L,
	SCP_DAC_ISET_H,
	SCP_DAC_ISET_L,
	SCP_VSET_BOUNDARY_H,
	SCP_VSET_BOUNDARY_L,
	SCP_ISET_BOUNDARY_H,
	SCP_ISET_BOUNDARY_L,
	SCP_MAX_VSET_OFFSET,
	SCP_MAX_ISET_OFFSET,
	SCP_VSET_H = 0xB8,
	SCP_VSET_L,
	SCP_ISET_H,
	SCP_ISET_L,
	SCP_VSET_OFFSET_H,
	SCP_VSET_OFFSET_L,
	SCP_ISET_OFFSET_H,
	SCP_ISET_OFFSET_L,
#if 0
	SCP_VROFFSET = 0xC6,
	SCP_VSOFFSET,
#endif
	SCP_SREAD_VOUT,
	SCP_SREAD_IOUT,
	SCP_VSSET,
	SCP_ISSET,
	SCP_STEP_VSET_OFFSET,
	SCP_STEP_ISET_OFFSET,
	SCP_MSG_MAX
};

enum SCP_CMD_T{	
	RT_SBRWR_CMD = 0x0B,	
	RT_SBRRD_CMD ,
	RT_MBRWR_CMD = 0x1B,
	RT_MBRRD_CMD,
	RT_CMD_MAX	
};

struct scp_send_t{
	unsigned char sbrwr;
	unsigned char reg;
	unsigned char len;
	unsigned char data[MAX_SCP_PROTO_LEN];
};

struct scp_recv_t{
	unsigned char ack;	
	unsigned char data[MAX_SCP_PROTO_LEN];
};

enum SCP_STATE_T{	
	IDLE = 0x0,	
	SEND_STATE ,
	RECV_STATE,
	DEBUG_STATE 	
};

void scp_state_machine(void);
int sgm_en_scp_protocol(unsigned char index,uint32_t *param,unsigned char len);
int sgm_get_scp_dev_type(void);
int sgm_scp_debug_info(unsigned char index,uint32_t *param,unsigned char len);
int sgm_get_scp_info(void);
int sgm_scp_write_operation(unsigned char index,uint32_t *param,unsigned char len);
int sgm_scp_read_operation(unsigned char index,uint32_t *param,unsigned char len);
#endif









