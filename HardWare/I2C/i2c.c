#include <stdbool.h>
#include "i2c.h"
#include <stdio.h>
#include "gd32f30x_i2c.h"
#include "systick.h"
#include "gd32f30x_gpio.h"

extern uint8_t g_sgm_slave_addr;
extern unsigned char CRC8_Calculate(unsigned char *pData, unsigned char Size);


#define HIGH_SPEED_I2C   (3400000)
#define I2C_HOST_CODE    (0x08)

#define SDA_IN()  {GPIO_CTL0(GPIOB)&=~GPIO_MODE_MASK(7);GPIO_CTL0(GPIOB)|=GPIO_MODE_SET(7,0x8);}
#define SDA_OUT() {GPIO_CTL0(GPIOB)&=~GPIO_MODE_MASK(7);GPIO_CTL0(GPIOB)|=GPIO_MODE_SET(7,0x3);}


#define IIC_SCL(n) (n? (GPIO_BOP(GPIOB)= GPIO_PIN_6):(GPIO_BC(GPIOB)=GPIO_PIN_6))
#define IIC_SDA(n) (REG32((GPIOB) + 0x14U - (n << 2)) = GPIO_PIN_7)//(n? (GPIO_BOP(GPIOB)= GPIO_PIN_7) :(GPIO_BC(GPIOB)=GPIO_PIN_7))
#define READ_SDA	(GPIO_ISTAT(GPIOB)& GPIO_PIN_7) //gpio_input_bit_get(GPIOB,GPIO_PIN_7)

static int s_iic_clock_speed = 0;


static inline void delay_147ns()
{    
    __NOP();
    __NOP();
    __NOP();
    __NOP();  
    __NOP();
    __NOP();
    __NOP();
    __NOP(); 
    __NOP();
    __NOP();
    __NOP();
    __NOP();
    __NOP();
}

static inline void delay_80ns()
{	
    __NOP();
    __NOP();
    __NOP(); 
    __NOP();
    __NOP(); 
    __NOP();
}

static inline void delay_4_nop()
{	
    __NOP();
    __NOP();
    __NOP(); 
    __NOP();
}

static inline void delay_ns(int time)
{    
    int count = time;
    while(count--)
    {
        __NOP();
    }
}

static inline void IIC_Start(int delay)
{
	SDA_OUT();
	IIC_SDA(1);	
	IIC_SCL(1);	
    delay_ns(delay);
	IIC_SDA(0);
    delay_ns(delay);	
	IIC_SCL(0);
}

bool IIC_HSWrite_host_code(uint8_t byte)
{
    int t = 100;
	uint8_t BitCnt = 8;
	SDA_OUT();
	IIC_SCL(0);
	while(BitCnt--) 
	{		
        IIC_SDA((byte&0x80) >>7);
		byte<<=1;
        delay_ns(12);
		IIC_SCL(1);
        delay_ns(24);
		IIC_SCL(0);
        delay_ns(12);
	}
    
    //Recv client NACK
    IIC_SDA(0);
    SDA_IN();
	delay_ns(10); 
    IIC_SCL(1);
    while(!READ_SDA)	
    {
		t--;
		if(t==0)
		{
			IIC_SCL(0);   
			return false;
		}		
    }
    
    delay_ns(20);
    IIC_SCL(0);    
    return true;
}

		  
static inline bool IIC_SendByte(uint8_t byte)
{
	//uint8_t BitCnt;
   
	SDA_OUT();
	//IIC_SCL(0);
	//delay_80ns();
    //bit 7
    IIC_SDA((byte&0x80) >>7);     
    delay_80ns();
    IIC_SCL(1);   
    delay_147ns();
    IIC_SCL(0);   
   
    
    //bit 6
    byte<<=1;   
	IIC_SDA((byte&0x80) >>7);     
    delay_80ns();
    IIC_SCL(1);   
    delay_147ns();
    IIC_SCL(0);   
    
    
    //bit 5
    byte<<=1;   
	IIC_SDA((byte&0x80) >>7);     
    delay_80ns();
    IIC_SCL(1);   
    delay_147ns();
    IIC_SCL(0);   
   
    
    //bit 4
    byte<<=1;   
	IIC_SDA((byte&0x80) >>7);     
    delay_80ns();
    IIC_SCL(1);   
    delay_147ns();
    IIC_SCL(0); 
    
    //bit 3
    byte<<=1;   
	IIC_SDA((byte&0x80) >>7);     
    delay_80ns();
    IIC_SCL(1);   
    delay_147ns();
    IIC_SCL(0); 
    
    //bit 2
    byte<<=1;   
	IIC_SDA((byte&0x80) >>7);     
    delay_80ns();
    IIC_SCL(1);   
    delay_147ns();
    IIC_SCL(0); 
    
    //bit 1
    byte<<=1;   
	IIC_SDA((byte&0x80) >>7);     
    delay_80ns();
    IIC_SCL(1);   
    delay_147ns();
    IIC_SCL(0); 
    
    //bit 0
    byte<<=1;   
	IIC_SDA((byte&0x80) >>7);     
    delay_80ns();
    IIC_SCL(1);   
    delay_147ns();
    IIC_SCL(0); 
    
    //IIC_SDA(0);
    SDA_IN();
    __NOP(); 
    __NOP(); 
    __NOP(); 
    IIC_SCL(1);	
    __NOP(); 
    __NOP(); 
    
    if (READ_SDA)
    {
        delay_80ns();
        IIC_SCL(0);   
		return false;
    }
    __NOP(); 
    __NOP(); 
    IIC_SCL(0);   
    return true;
}

// 
static inline uint8_t IIC_RcvByte(bool ack)
{
	uint8_t retc;
	retc=0; 
	SDA_IN();
	delay_80ns();
	 
    //BIT7
    IIC_SCL(0);
    delay_147ns();
    IIC_SCL(1);		           
    retc=retc<<1;
    if(READ_SDA) retc |=1;
    __NOP();
    __NOP();
    
    //BIT6
    IIC_SCL(0);
    delay_147ns();
    IIC_SCL(1);		           
    retc=retc<<1;
    if(READ_SDA) retc |=1;
    delay_4_nop();
    
    //BIT5
    IIC_SCL(0);
    delay_147ns();
    IIC_SCL(1);		           
    retc=retc<<1;
    if(READ_SDA) retc |=1;
	delay_4_nop();
    
    //BIT4
    IIC_SCL(0);
    delay_147ns();
    IIC_SCL(1);		           
    retc=retc<<1;
    if(READ_SDA) retc |=1;
    delay_4_nop();
    
    //BIT3
    IIC_SCL(0);	
    delay_147ns();
    IIC_SCL(1);		           
    retc=retc<<1;
    if(READ_SDA) retc |=1;
    delay_4_nop();
    
    //BIT2
    IIC_SCL(0);	
    delay_147ns();
    IIC_SCL(1);		           
    retc=retc<<1;
    if(READ_SDA) retc |=1;
    delay_4_nop();
    
    //BIT1
    IIC_SCL(0);
    delay_147ns();
    IIC_SCL(1);		           
    retc=retc<<1;
    if(READ_SDA) retc |=1;
    delay_4_nop();
    
    //BIT0
    IIC_SCL(0);
    delay_147ns();
    IIC_SCL(1);		           
    retc=retc<<1;
    if(READ_SDA) retc |=1;
    delay_4_nop();
    
    IIC_SCL(0);
    SDA_OUT();
    //delay_80ns();
    if (ack)
    {
        IIC_SDA(0);
        __NOP();
        __NOP();
        //delay_80ns();
        IIC_SCL(1);
        delay_147ns();
        IIC_SCL(0);
        //delay_80ns();
        __NOP();
        __NOP();
    }
    else
    { 
        IIC_SDA(1);
        __NOP();
        __NOP();
        //delay_80ns();
        IIC_SCL(1);
        delay_147ns();
        IIC_SCL(0);
        //delay_80ns();
        __NOP();
        __NOP();
    
    }
    
	return(retc);
}

uint8_t gpio_to_IIC_Write_One_Byte(uint8_t slave,uint8_t reg,uint8_t data)
{
	bool ack = 0;
		
	IIC_Start(12);
    ack = IIC_HSWrite_host_code(I2C_HOST_CODE);
	if (false == ack)
		return 1;
   
    /*   Start signal timing start*/
    SDA_OUT();
	IIC_SDA(1);	
	IIC_SCL(1);	
    delay_147ns();
	IIC_SDA(0);
    delay_147ns();	
	IIC_SCL(0);
     /*   Start signal timing end*/
	ack = IIC_SendByte(slave);
	if (false == ack)
		return 1;
	ack = IIC_SendByte(reg);// 
	if (false == ack)
		return 1;    	 										  		   
	ack = IIC_SendByte(data);
	if (false == ack)
		return 1;  		    	   
	//IIC_Stop();
    
     /*   Stop signal timing start*/
    SDA_OUT();
	IIC_SCL(0);
	IIC_SDA(0);
    delay_80ns();
	IIC_SCL(1);
    delay_80ns();
	IIC_SDA(1);
    delay_80ns();
     /*   Stop signal timing end*/
		
	return 0;
}

uint8_t gpio_to_IIC_Write_block(uint8_t slave,uint8_t reg,uint8_t *p_data,uint8_t len)
{
	bool ack = 0;
		
	IIC_Start(12);
    ack = IIC_HSWrite_host_code(I2C_HOST_CODE);
	if (false == ack)
		return 1;
   
    /*   Start signal timing start*/
    SDA_OUT();
	IIC_SDA(1);	
	IIC_SCL(1);	
    delay_147ns();
	IIC_SDA(0);
    delay_147ns();	
	IIC_SCL(0);
     /*   Start signal timing end*/
	ack = IIC_SendByte(slave);
	if (false == ack)
		return 1;
	ack = IIC_SendByte(reg);// 
	if (false == ack)
		return 1; 
    
    while(len--)
    {
        ack = IIC_SendByte(*p_data);        
        if (false == ack)
            return 1; 
        p_data++; 
    }    
     /*   Stop signal timing start*/
    SDA_OUT();
	IIC_SCL(0);
	IIC_SDA(0);
    delay_80ns();
	IIC_SCL(1);
    delay_80ns();
	IIC_SDA(1);
    delay_80ns();
     /*   Stop signal timing end*/
    return 0;
}

uint8_t gpio_to_IIC_Read_One_Byte(uint8_t slave,uint8_t reg,uint8_t* data)
{
	uint8_t temp=0;
	bool ack = 0;
    
    /*   Start signal timing start  */
    SDA_OUT();
	IIC_SDA(1);	
	IIC_SCL(1);	
    delay_147ns();
	IIC_SDA(0);
    delay_147ns();	
	IIC_SCL(0);
     /*   Start signal timing end  */
    
    ack = IIC_HSWrite_host_code(I2C_HOST_CODE);
	if (false == ack)
		return 1;
    /*   Start  signal timing start*/
    SDA_OUT();
	IIC_SDA(1);	
	IIC_SCL(1);	
    delay_147ns();
	IIC_SDA(0);
    delay_147ns();	
	IIC_SCL(0);
     /*   Start timing end*/
	ack = IIC_SendByte(slave);   //
	if (false == ack)
		return 1;
	ack = IIC_SendByte(reg);   //
	if (false == ack)
		return 1;	
    
	/*   restart signal timing start*/
    SDA_OUT();
	IIC_SDA(1);	
	IIC_SCL(1);	
    delay_147ns();
	IIC_SDA(0);
    delay_147ns();	
	IIC_SCL(0);
     /*   restart signal timing end*/
    
	ack = IIC_SendByte(slave+1);           		   
	if (false == ack)
		return 1;
    
	temp=IIC_RcvByte(false);
    
    /*   stop signal timing start*/
    SDA_OUT();
	IIC_SCL(0);
	IIC_SDA(0);
    delay_80ns();
	IIC_SCL(1);
    delay_147ns();
	IIC_SDA(1);
    delay_147ns();
    /*   stop signal timing end*/
	*data = temp;  
	return 0;
}

uint8_t gpio_to_IIC_Read_block(uint8_t slave,uint8_t reg,uint8_t* p_data,uint8_t len)
{
	bool ack = 0;
    
    /*   Start signal timing start  */
    SDA_OUT();
	IIC_SDA(1);	
	IIC_SCL(1);	
    delay_147ns();
	IIC_SDA(0);
    delay_147ns();	
	IIC_SCL(0);
    
     /*   Start signal timing end  */
    ack = IIC_HSWrite_host_code(I2C_HOST_CODE);
	if (false == ack)
		return 1;
    /*   Start  signal timing start*/
    SDA_OUT();
	IIC_SDA(1);	
	IIC_SCL(1);	
    delay_147ns();
	IIC_SDA(0);
    delay_147ns();	
	IIC_SCL(0);
     /*   Start timing end*/
	ack = IIC_SendByte(slave);   //
	if (false == ack)
		return 1;
	ack = IIC_SendByte(reg);   //
	if (false == ack)
		return 1;	
    
	/*   restart signal timing start*/
    SDA_OUT();
	IIC_SDA(1);	
	IIC_SCL(1);	
    delay_147ns();
	IIC_SDA(0);
    delay_147ns();	
	IIC_SCL(0);
     /*   restart signal timing end*/
    
	ack = IIC_SendByte(slave+1);           		   
	if (false == ack)
		return 1;
    while(--len)
    {
        *p_data =IIC_RcvByte(true);
        p_data++;        
    }
	*p_data =IIC_RcvByte(false);
     p_data++;	   
	//IIC_Stop(); 
    /*   stop signal timing start*/
    SDA_OUT();
	IIC_SCL(0);
	IIC_SDA(0);
    delay_80ns();
	IIC_SCL(1);
    delay_147ns();
	IIC_SDA(1);
    delay_147ns();
    /*   stop signal timing end*/
	
	return 0;
}


/*!
    \brief      cofigure the I2C0 and I2C1 interfaces
    \param[in]  none
    \param[out] none
    \retval     none
*/
void i2c_config(uint32_t clkspeed)
{
    /* enable I2C clock */
    rcu_periph_clock_enable(RCU_I2C0);
    /* configure I2C clock */
    if (clkspeed <= 100000)
        i2c_clock_config(I2C0,clkspeed,I2C_DTCY_2);
    else 
        i2c_clock_config(I2C0,clkspeed,I2C_DTCY_16_9);
    /* configure I2C address */
    i2c_mode_addr_config(I2C0,I2C_I2CMODE_ENABLE,I2C_ADDFORMAT_7BITS,g_sgm_slave_addr);
	
		/* enable I2C0 */
    i2c_enable(I2C0);
	
    /* enable acknowledge */
    i2c_ack_config(I2C0,I2C_ACK_ENABLE);	
}

void i2c_init(uint32_t clkspeed)
{
	
    /* enable GPIOB clock */
    rcu_periph_clock_enable(RCU_GPIOB);
    /* GPIO config */
    s_iic_clock_speed = clkspeed;
    if (clkspeed == HIGH_SPEED_I2C)
    {
        gpio_init(GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_6 | GPIO_PIN_7);   
        gpio_bit_set(GPIOB, GPIO_PIN_6);
        gpio_bit_set(GPIOB, GPIO_PIN_7);
    }
    else
    {
        i2c_deinit(I2C0);
        gpio_init(GPIOB, GPIO_MODE_AF_OD, GPIO_OSPEED_50MHZ, GPIO_PIN_6 | GPIO_PIN_7);
         /* I2C config */
        i2c_config(clkspeed);
    }
}

int32_t i2c_buffer_read(uint8_t device_address,uint8_t read_address, uint8_t* p_buffer, uint16_t number_of_byte, uint16_t timeout)
{
    uint32_t delay_time;
    uint32_t i2c_delay_time = timeout * 48;
		
	uint8_t error_code = 0;
    /* wait until I2C bus is idle */
    delay_time = i2c_delay_time;
    while(i2c_flag_get(I2C0, I2C_FLAG_I2CBSY)) {
		if((delay_time--) == 0){
			error_code = 1;
			goto err;
		}	
	}
		 
    if(2 == number_of_byte){
        i2c_ackpos_config(I2C0,I2C_ACKPOS_NEXT);
    }

    /* send a start condition to I2C bus */
    i2c_start_on_bus(I2C0);

    /* wait until SBSEND bit is set */
    delay_time = i2c_delay_time;
    while(SET!=i2c_flag_get(I2C0, I2C_FLAG_SBSEND)){
		if((delay_time--) == 0){
			error_code = 2;
			goto err;
		}	
	}
    /* send slave address to I2C bus */
    i2c_master_addressing(I2C0, device_address, I2C_TRANSMITTER);

    /* wait until ADDSEND bit is set */
    delay_time = i2c_delay_time;
    while(SET!=i2c_flag_get(I2C0, I2C_FLAG_ADDSEND)) {
		if((delay_time--) == 0){
			error_code = 3;
			goto err;
		}
	}

    /* clear the ADDSEND bit */
    i2c_flag_clear(I2C0,I2C_FLAG_ADDSEND);

    /* wait until the transmit data buffer is empty */
    delay_time = i2c_delay_time;
    while(SET!=i2c_flag_get(I2C0, I2C_FLAG_TBE)) {
		if((delay_time--) == 0){
			error_code = 4;
			goto err;
		}
	}

    /* enable I2C0*/
    i2c_enable(I2C0);

    /* send the EEPROM's internal address to write to */
    i2c_data_transmit(I2C0, read_address);

    /* wait until BTC bit is set */
    delay_time = i2c_delay_time;
    while(SET!=i2c_flag_get(I2C0, I2C_FLAG_BTC)) {
		if((delay_time--) == 0){
			error_code = 5;
			goto err;
		}
	}
    /* send a start condition to I2C bus */
    i2c_start_on_bus(I2C0);

    /* wait until SBSEND bit is set */
    delay_time = i2c_delay_time;
    while(SET!=i2c_flag_get(I2C0, I2C_FLAG_SBSEND)) {
		if((delay_time--) == 0){
			error_code = 6;
			goto err;
		}
	}

    /* send slave address to I2C bus */
    i2c_master_addressing(I2C0, device_address, I2C_RECEIVER);

    if(number_of_byte < 3){
        /* disable acknowledge */
        i2c_ack_config(I2C0,I2C_ACK_DISABLE);
    }

    /* wait until ADDSEND bit is set */
    delay_time = i2c_delay_time;
    while(SET!=i2c_flag_get(I2C0, I2C_FLAG_ADDSEND)) {
		if((delay_time--) == 0){
			error_code = 7;
			goto err;
		}
	}

    /* clear the ADDSEND bit */
    i2c_flag_clear(I2C0,I2C_FLAG_ADDSEND);

    if(1 == number_of_byte){
        /* send a stop condition to I2C bus */
        i2c_stop_on_bus(I2C0);
    }

    /* while there is data to be read */
    delay_time = i2c_delay_time;

    while((--delay_time) && number_of_byte) {
        if(3 == number_of_byte){
            /* wait until BTC bit is set */
            while(!i2c_flag_get(I2C0, I2C_FLAG_BTC)) {
				if((delay_time--) == 0){
					error_code = 8;
					goto err;
				}
			}

            /* disable acknowledge */
            i2c_ack_config(I2C0,I2C_ACK_DISABLE);
        }
        if(2 == number_of_byte){
            /* wait until BTC bit is set */
            while(!i2c_flag_get(I2C0, I2C_FLAG_BTC)) {
				if((delay_time--) == 0){
					error_code = 9;
					goto err;
				}
			}
            /* send a stop condition to I2C bus */
            i2c_stop_on_bus(I2C0);
        }

        /* wait until the RBNE bit is set and clear it */
        if(i2c_flag_get(I2C0, I2C_FLAG_RBNE)){
            /* read a byte from the EEPROM */
            *p_buffer = i2c_data_receive(I2C0);

            /* point to the next location where the byte read will be saved */
            p_buffer++;

            /* decrement the read bytes counter */
            number_of_byte--;
        }
    }

    /* wait until the stop condition is finished */
    delay_time = i2c_delay_time;
    while((I2C_CTL0(I2C0)&0x0200)) {
		if((delay_time--) == 0){
			error_code = 10;
			goto err;
		}
	}	
    /* enable acknowledge */
    i2c_ack_config(I2C0,I2C_ACK_ENABLE);
    i2c_ackpos_config(I2C0,I2C_ACKPOS_CURRENT);
    return 0;
err:
	/* send a stop condition to I2C bus */
    i2c_stop_on_bus(I2C0);
	while(I2C_CTL0(I2C0)&0x0200)
    {	if((delay_time--) == 0){	error_code = 11;}	}
	i2c_init(s_iic_clock_speed);
	return error_code;
}
//used for sgm25720 test
int i2c_buffer_write_25720(uint8_t device_address,uint8_t reg, uint8_t* p_buffer, uint8_t number_of_byte,uint16_t timeout, bool is_stop)
{
    uint32_t delay_time;
    uint32_t i2c_delay_time = timeout * 48;
    uint8_t error_code;
	
	//printf("addr = %#x, reg = %#x, buff = %#x.\n", device_address, reg, p_buffer[0]);
    /* wait until I2C bus is idle */
    delay_time = i2c_delay_time;
//    while(i2c_flag_get(I2C0, I2C_FLAG_I2CBSY))
//    {	if((delay_time--) == 0){	error_code = 1; goto err;	}	}
    
    /* send a start condition to I2C bus */
    i2c_start_on_bus(I2C0);
    
    /* wait until SBSEND bit is set */
    delay_time = i2c_delay_time;
    while(!i2c_flag_get(I2C0, I2C_FLAG_SBSEND))
    {	if((delay_time--) == 0){	error_code = 2; goto err;	}	}
    
    /* send slave address to I2C bus */
    i2c_master_addressing(I2C0, device_address, I2C_TRANSMITTER);
    
    /* wait until ADDSEND bit is set */
    delay_time = i2c_delay_time;
    while(!i2c_flag_get(I2C0, I2C_FLAG_ADDSEND))
    {	if((delay_time--) == 0){	error_code = 3; goto err;	}	}
    
    /* clear the ADDSEND bit */
    i2c_flag_clear(I2C0,I2C_FLAG_ADDSEND);
    
    /* wait until the transmit data buffer is empty */
    delay_time = i2c_delay_time;
    while( SET != i2c_flag_get(I2C0, I2C_FLAG_TBE))
    {	if((delay_time--) == 0){	error_code = 4; goto err;	}	}
    
    /* send the EEPROM's internal address to write to : only one byte address */
    i2c_data_transmit(I2C0, reg);
    
    /* wait until BTC bit is set */
    delay_time = i2c_delay_time;
    while(!i2c_flag_get(I2C0, I2C_FLAG_BTC))
    {	if((delay_time--) == 0){	error_code = 5; goto err;	}	}
    
    /* while there is data to be written */
    while(number_of_byte--){  
        i2c_data_transmit(I2C0, *p_buffer);

        /* point to the next byte to be written */
        p_buffer++; 

        /* wait until BTC bit is set */
        delay_time = i2c_delay_time;
        while(!i2c_flag_get(I2C0, I2C_FLAG_BTC))
        {	if((delay_time--) == 0){	error_code = 6; goto err;	}	}
    }
    /* send a stop condition to I2C bus */
	if (is_stop) {
		i2c_stop_on_bus(I2C0);
		/* wait until the stop condition is finished */
		delay_time = i2c_delay_time;
		while(I2C_CTL0(I2C0)&0x0200)
		{	if((delay_time--) == 0){	error_code = 7; goto err;	}	}
	}
	return 0;
err:
	i2c_stop_on_bus(I2C0);
	while(I2C_CTL0(I2C0)&0x0200)
    {	if((delay_time--) == 0){	error_code = 8;}	}
	
	i2c_init(s_iic_clock_speed);
	return error_code;
}

int i2c_buffer_write(uint8_t device_address,uint8_t reg, uint8_t* p_buffer, uint8_t number_of_byte,uint16_t timeout, bool is_stop)
{
    uint32_t delay_time;
    uint32_t i2c_delay_time = timeout * 48;
    uint8_t error_code;
    /* wait until I2C bus is idle */
    delay_time = i2c_delay_time;
    while(i2c_flag_get(I2C0, I2C_FLAG_I2CBSY))
    {	if((delay_time--) == 0){	error_code = 1; goto err;	}	}
    
    /* send a start condition to I2C bus */
    i2c_start_on_bus(I2C0);
    
    /* wait until SBSEND bit is set */
    delay_time = i2c_delay_time;
    while(!i2c_flag_get(I2C0, I2C_FLAG_SBSEND))
    {	if((delay_time--) == 0){	error_code = 2; goto err;	}	}
    
    /* send slave address to I2C bus */
    i2c_master_addressing(I2C0, device_address, I2C_TRANSMITTER);
    
    /* wait until ADDSEND bit is set */
    delay_time = i2c_delay_time;
    while(!i2c_flag_get(I2C0, I2C_FLAG_ADDSEND))
    {	if((delay_time--) == 0){	error_code = 3; goto err;	}	}
    
    /* clear the ADDSEND bit */
    i2c_flag_clear(I2C0,I2C_FLAG_ADDSEND);
    
    /* wait until the transmit data buffer is empty */
    delay_time = i2c_delay_time;
    while( SET != i2c_flag_get(I2C0, I2C_FLAG_TBE))
    {	if((delay_time--) == 0){	error_code = 4; goto err;	}	}
    
    /* send the EEPROM's internal address to write to : only one byte address */
    i2c_data_transmit(I2C0, reg);
    
    /* wait until BTC bit is set */
    delay_time = i2c_delay_time;
    while(!i2c_flag_get(I2C0, I2C_FLAG_BTC))
    {	if((delay_time--) == 0){	error_code = 5; goto err;	}	}
    
    /* while there is data to be written */
    while(number_of_byte--){  
        i2c_data_transmit(I2C0, *p_buffer);

        /* point to the next byte to be written */
        p_buffer++; 

        /* wait until BTC bit is set */
        delay_time = i2c_delay_time;
        while(!i2c_flag_get(I2C0, I2C_FLAG_BTC))
        {	if((delay_time--) == 0){	error_code = 6; goto err;	}	}
    }
    /* send a stop condition to I2C bus */
	if (is_stop)
		i2c_stop_on_bus(I2C0);
    
    /* wait until the stop condition is finished */
    delay_time = i2c_delay_time;
    while(I2C_CTL0(I2C0)&0x0200)
    {	if((delay_time--) == 0){	error_code = 7; goto err;	}	}
		
	return 0;
err:
	i2c_stop_on_bus(I2C0);
	while(I2C_CTL0(I2C0)&0x0200)
    {	if((delay_time--) == 0){	error_code = 8;}	}
	i2c_init(s_iic_clock_speed);
	return error_code;
}

int sgm_i2c_read_byte(uint8_t reg, uint8_t *val)
{
	uint8_t tmp = 0;
	int ret = 0;
	
    if (s_iic_clock_speed == HIGH_SPEED_I2C) {
        ret = gpio_to_IIC_Read_One_Byte(g_sgm_slave_addr, reg, &tmp);
    } else {
        ret = i2c_buffer_read(g_sgm_slave_addr, reg, &tmp, 1, 1000);
    }
	if (ret) {
		//printf("Failed: reg=%02X, ret=%d\n", reg, ret);
		return ret;
	}
	*val = tmp;
	return 0;
}

int sgm_i2c_write_byte(uint8_t reg, uint8_t val)
{
    int ret;	
    if (s_iic_clock_speed == HIGH_SPEED_I2C) {
        ret = gpio_to_IIC_Write_One_Byte(g_sgm_slave_addr, reg, val);
    } else {
        ret = i2c_buffer_write(g_sgm_slave_addr, reg, &val, 1,1000, true);
    }
	return ret;	
}

int sgm_i2c_read_block_data(uint8_t reg, uint8_t *data,uint8_t len)
{	
	int ret = 0;
	if (s_iic_clock_speed == HIGH_SPEED_I2C) {
        ret = gpio_to_IIC_Read_block(g_sgm_slave_addr, reg, data, len);
    } else {
        ret = i2c_buffer_read(g_sgm_slave_addr, reg, data, len, 1000);
    }

	if (ret) {
		//printf("Failed: reg=%02X, ret=%d\n", reg, ret);
		return ret;
	}
	
	return 0;
}

int sgm_i2c_write_block_data(uint8_t reg, uint8_t *data,uint8_t len)
{	
	int ret = 0;
    if (s_iic_clock_speed == HIGH_SPEED_I2C) {
        ret = gpio_to_IIC_Write_block(g_sgm_slave_addr, reg, data, len);
    } else {
        ret = i2c_buffer_write(g_sgm_slave_addr, reg, data, len, 1000, true);
    }

	if (ret) {
		//printf("Failed: reg=%02X, ret=%d\n", reg, ret);
		return ret;
	}
	
	return 0;
}

int sgm_update_bits(uint8_t reg,uint8_t mask, uint8_t val)
{
	uint8_t tmp;
	int ret;

	//mutex_lock(&sgm->i2c_rw_lock);
    ret = sgm_i2c_read_byte(reg, &tmp);	
	if (ret) {
		//printf("Failed: reg=%02X, ret=%d\n", reg, ret);
		goto out;
	}

	tmp &= ~mask;
	tmp |= val & mask;
	
    ret = sgm_i2c_write_byte(reg, tmp);		
	//if (ret)
		//printf("Failed: tmp=%02X, ret=%d\n", tmp, ret);

out:
	//mutex_unlock(&sgm->i2c_rw_lock);
	return ret;
}

int sgm_update_16bits(uint8_t reg,uint16_t mask, uint16_t val)
{
	uint8_t tmp[2] = {0};
	uint16_t data = 0;
	int ret;

	//mutex_lock(&sgm->i2c_rw_lock);
	ret = sgm_i2c_read_block_data(reg, tmp, 2);
	if (ret) {
		//printf("Failed: reg=%02X, ret=%d\n", reg, ret);
		goto out;
	}
	data = tmp[0] | tmp[1] << 8 ;
	data &= ~mask;
	data |= val & mask;

	tmp[0] = data & 0xFF;
	tmp[1] = data >> 8 & 0xFF;
    ret = sgm_i2c_write_block_data(reg, tmp, 2);		
	//if (ret)
		//printf("Failed: tmp=%02X, ret=%d\n", tmp, ret);

out:
	//mutex_unlock(&sgm->i2c_rw_lock);
	return ret;
}

int sgm_update_bits_crc(uint8_t reg,uint8_t mask, uint8_t *val)
{
	uint8_t tmp;
	int ret;
	uint8_t crc_buff[4];
	uint8_t write_buff[2];
	
	//mutex_lock(&sgm->i2c_rw_lock);
	ret = sgm_i2c_read_byte(reg, &tmp);
	if (ret) {
		//printf("Failed: reg=%02X, ret=%d\n", reg, ret);
		goto out;
	}

	tmp &= ~mask;
	tmp |= val[0] & mask;

	crc_buff[0] = g_sgm_slave_addr;
	crc_buff[1] = reg;
	crc_buff[2] = tmp;
	crc_buff[3] = 0;
	write_buff[1] = CRC8_Calculate(crc_buff,4);
	write_buff[0] = tmp;
    
    ret = sgm_i2c_write_block_data(reg, write_buff, 2);	
	//if (ret)
		//printf("Failed: tmp=%02X, ret=%d\n", tmp, ret);

out:
	//mutex_unlock(&sgm->i2c_rw_lock);
	return ret;
}

//��ʼ��IIC
void swire_init(void)
{					     
	rcu_periph_clock_enable(RCU_GPIOB);	/* ??GPIOB?? */
		
	gpio_init(GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_5);
	/* ??IIC_SCL??????? */ 
	gpio_init(GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_6);
		/* ??IIC_SDA??????? */ 
	gpio_init(GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_7);
		
	gpio_bit_set(GPIOB, GPIO_PIN_5);
	gpio_bit_set(GPIOB, GPIO_PIN_6);
	gpio_bit_set(GPIOB, GPIO_PIN_7);
}

void set_vo1_vo2_output_volt(uint8_t pulse_count)
{		
	uint8_t i = pulse_count;
	gpio_bit_set(GPIOB, GPIO_PIN_4);
	//350uS+3.2mS+2.8mS+2.6mS
	
	delay_1ms(9);
	while (i--){
		gpio_bit_reset(GPIOB, GPIO_PIN_4);
		//delay_1us(10);        
		gpio_bit_set(GPIOB, GPIO_PIN_4);       
		//delay_1us(10);
	}
	delay_1us(55);
}

void set_vo3_output_volt(uint8_t pulse_count)
{		
	uint8_t i = pulse_count;
	gpio_bit_set(GPIOB, GPIO_PIN_5);
	//350uS+1.8mS
	delay_1us(2150);
	
	while (i--){
		gpio_bit_reset(GPIOB, GPIO_PIN_5);
		delay_1us(10);
		gpio_bit_set(GPIOB, GPIO_PIN_5);
		delay_1us(10);
	}
	delay_1us(55);	
}


void set_vo4_output_volt(uint8_t pulse_count)
{		
	uint8_t i = pulse_count;
	gpio_bit_set(GPIOB, GPIO_PIN_5);
	//350uS
	delay_1us(350);
		
	while (i--){
		gpio_bit_reset(GPIOB, GPIO_PIN_5);
		delay_1us(10);
		gpio_bit_set(GPIOB, GPIO_PIN_5);
		delay_1us(10);
	}
}

void set_vo4_output_volt2(uint8_t pulse_count)
{		
	uint8_t i = pulse_count;
		
	//gpio_bit_set(GPIOB, GPIO_PIN_5);
	//350uS
	delay_1us(1550);
	gpio_bit_set(GPIOB, GPIO_PIN_5);
	delay_1us(350);
	
	while (i--){
		gpio_bit_reset(GPIOB, GPIO_PIN_5);
		delay_1us(10);
		gpio_bit_set(GPIOB, GPIO_PIN_5);
		delay_1us(10);
	}
	delay_1us(55);		
}



