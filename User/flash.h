//flash.h文件
#ifndef __FLASH_H__
#define __FLASH_H__

#include "gd32f30x.h"


int32_t gd32_flash_write(uint32_t addr,uint16_t *p_buff,uint32_t data_len);
void gd32_flash_read(uint32_t addr,uint16_t *p_buff,uint32_t data_len);
void gd32_flash_erase_page(uint32_t addr);
void gd32_flash_erase_whole(void);
#endif

