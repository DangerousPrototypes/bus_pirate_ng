

#include <stdint.h>
#include "buspirateNG.h"
#include <libopencm3/stm32/flash.h>
#include "cdcacm.h"
#include "bpflash.h"

// reads an absolute address in flash
uint32_t readflash(uint32_t addr)
{

	return *(uint32_t*)(addr);

}

// writes to a absolute adress in flash
uint32_t writeflash(uint32_t addr, uint32_t data)
{
	uint32_t flash_status;

	flash_wait_for_last_operation();
	flash_unlock();
	flash_program_word(addr, data);
	flash_wait_for_last_operation();
	flash_status = flash_get_status_flags();
	flash_lock();


	if(flash_status != FLASH_SR_EOP)
		return flash_status;
	else
		return 0;
}

// erases a page in flash memory (1 page=1024bytes)
uint32_t eraseflashpage(uint32_t addr)
{
	uint32_t flash_status;

	flash_wait_for_last_operation();
	flash_unlock();
	flash_erase_page(addr);
	flash_wait_for_last_operation();
	flash_status = flash_get_status_flags();
	flash_lock();


	if(flash_status != FLASH_SR_EOP)
		return flash_status;
	else
		return 0;
}

// show a hexdhump of the reserved space
// TODO: come up with a better name ps=private store
void displayps(void)
{
	int i;
	uint32_t flash1, flash2;

	cdcprintf("\r\n");

	for(i=0; i<FLASH_SIZE; i+=8)
	{
		flash1=readflash(FLASH_START+i);
		flash2=readflash(FLASH_START+i+4);

		cdcprintf("%08X: %08X%08X ", FLASH_START+i, flash1, flash2);
		cdcprintf("%c%c%c%c%c%c%c%c", '.', '.', '.', '.', '.', '.', '.', '.');
		cdcprintf("\r\n");
	}

}

// fill the private space with data
void fillps(void)
{
	int i;

	eraseflashpage(0x08010000-0x800);
	eraseflashpage(0x08010000-0x800);

	for(i=0; i<FLASH_SIZE; i+=4)
		writeflash((0x08010000-0x800)+i, i);
 
}





