

// for stm32f103 medium
#define FLASH_START ((uint32_t)(0x08010000-0x800))	// userflash 2 last pages
#define FLASH_END ((uint32_t)0x08010000)		// 
#define FLASH_PAGE_SIZE 0x400
#define FLASH_PAGE_MASK ((uint32_t)0xFFFFFC00)
#define FLASH_SIZE 0x800

uint32_t readflash(uint32_t addr);
uint32_t eraseflashpage(uint32_t addr);
uint32_t writeflash(uint32_t addr, uint32_t data);
void displayps(void);
void fillps(void);
