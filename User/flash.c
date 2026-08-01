//flash.c文件

#include "flash.h"

/*
	FLASH写入操作；
	addr --- 数据写入的地址即位置
	p_buff --- 要写入的数据
	data_len --- 要写入数据的长度
*/
int32_t gd32_flash_write(uint32_t addr,uint16_t *p_buff,uint32_t data_len)    
{
	uint32_t i=0;
	uint32_t new_addr;
	uint16_t data=0; 
	fmc_flag_clear(FMC_FLAG_BANK0_END|FMC_FLAG_BANK0_WPERR|FMC_FLAG_BANK0_PGERR);
    fmc_unlock();
	for(i=0;i<data_len;i++)
	{
		data=*(p_buff+i);
		new_addr = addr+i*sizeof(uint16_t);
		if(FMC_READY != fmc_halfword_program(new_addr,data))
		{
			return -1;//写入失败
		}
        fmc_flag_clear(FMC_FLAG_BANK0_END|FMC_FLAG_BANK0_WPERR|FMC_FLAG_BANK0_PGERR);
	}
	fmc_lock();	
	return 0;
}

/*
	FLASH数据读取；
	addr --- 读取数据的地址即位置
	p_buff --- 读取数据缓存位置
	data_len ---读取数据的长度
*/
void gd32_flash_read(uint32_t addr,uint16_t *p_buff,uint32_t data_len)  
{                           
	uint32_t i=0;
	uint32_t new_addr;
	for(i=0;i<data_len;i++)
	{
		new_addr = addr+i*sizeof(uint16_t);
		p_buff[i] = *(__IO uint16_t *)new_addr;
    }
}

/*
	FLASH块擦除；
	addr --- 要擦除的地址即位置
*/
void gd32_flash_erase_page(uint32_t addr)
{   
    /* unlock the flash program/erase controller */
    fmc_unlock();

    /* clear all pending flags */
    fmc_flag_clear(FMC_FLAG_BANK0_END|FMC_FLAG_BANK0_WPERR|FMC_FLAG_BANK0_PGERR);

    /* erase the flash pages */
	fmc_page_erase(addr);
	fmc_flag_clear(FMC_FLAG_BANK0_END|FMC_FLAG_BANK0_WPERR|FMC_FLAG_BANK0_PGERR);

    /* lock the main FMC after the erase operation */
    fmc_lock();
}

/*
	FLASH擦除；
	擦除整个FLASH。
*/
void gd32_flash_erase_whole(void)
{
    /* unlock the flash program/erase controller */
    fmc_unlock();

    /* clear all pending flags */
    fmc_flag_clear(FMC_FLAG_BANK0_END|FMC_FLAG_BANK0_WPERR|FMC_FLAG_BANK0_PGERR);

    /* erase the flash pages */
	fmc_mass_erase();
	fmc_flag_clear(FMC_FLAG_BANK0_END|FMC_FLAG_BANK0_WPERR|FMC_FLAG_BANK0_PGERR);

    /* lock the main FMC after the erase operation */
    fmc_lock();
	
}



