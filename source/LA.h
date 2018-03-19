
void LA_start(void);
uint32_t LA_send(uint32_t d);
uint32_t LA_read(void);
void LA_macro(uint32_t macro);
void LA_setup(void);
void LA_setup_exc(void);
void LA_cleanup(void);
void LA_pins(void);
void LA_settings(void);


#define LATRIGGERMENU	"Trigger\r\n 1. Rising\r\n 2. Falling\r\n 3. Both\r\n 4. No trigger*\r\n> "
#define LAPERIODMENU 	"period> "
#define LASAMPLEMENU	"samples> "

#define CMDREAD		0x03
#define CMDWRITE	0x02
#define CMDQUADMODE	0x38
#define CMDSEQLMODE
#define CMDRESETSPI	0xFF
#define CMDWRITERREG	0x05
