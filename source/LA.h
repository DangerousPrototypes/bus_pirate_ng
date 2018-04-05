
void LA_start(void);
uint32_t LA_send(uint32_t d);
uint32_t LA_read(void);
void LA_macro(uint32_t macro);
void LA_setup(void);
void LA_setup_exc(void);
void LA_cleanup(void);
void LA_pins(void);
void LA_settings(void);

//ians test functions
void logicAnalyzerSetup(void);
void logicAnalyzerCaptureStop(void);
void logicAnalyzerCaptureStart(void);
void logicAnalyzerDumpSamples(uint32_t numSamples);

#define LATRIGGERMENU	"Trigger\r\n 1. Rising\r\n 2. Falling\r\n 3. Both\r\n 4. No trigger*\r\n> "
#define LAPERIODMENU 	"period> "
#define LASAMPLEMENU	"samples> "

#define CMDREAD		0x03
#define CMDWRITE	0x02
#define CMDQUADMODE	0x38
#define CMDSEQLMODE	0x40
#define CMDRESETSPI	0xFF
#define CMDWRITERREG	0x05

#define BP_LA_SAMPLES_PER_CHANNEL (((BP_LA_SRAM_SIZE)*8)/4) //8 bits per byte, 4 channels per SRAM
#define BP_LA_OVERFLOW_COUNT (BP_LA_SAMPLES_PER_CHANNEL/(0xFFFF+1)) //divide by 16bit timer count

#define BP_LA_COUNTER_PRELOAD (0xFFFF-(0)) //preload the sample counter, can be used to avoid sampling overrun

#define BP_LA_LATCH_SETUP() gpio_set_mode(BP_LA_LATCH_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_LA_LATCH_PIN)
#define BP_LA_LATCH_OPEN() gpio_clear(BP_LA_LATCH_PORT, BP_LA_LATCH_PIN)
#define BP_LA_LATCH_CLOSE() gpio_set(BP_LA_LATCH_PORT, BP_LA_LATCH_PIN)

#define BP_LA_SRAM_CS_SETUP() gpio_set_mode(BP_LA_SRAM_CS_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_LA_SRAM_CS_PIN)
#define BP_LA_SRAM_DESELECT() gpio_set(BP_LA_SRAM_CS_PORT, BP_LA_SRAM_CS_PIN)
#define BP_LA_SRAM_SELECT() gpio_clear(BP_LA_SRAM_CS_PORT, BP_LA_SRAM_CS_PIN)

#define BP_LA_SRAM_CLOCK_SETUP() gpio_set_mode(BP_LA_SRAM_CLK_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_LA_SRAM_CLK_PIN);
#define BP_LA_SRAM_CLOCK_LOW() gpio_clear(BP_LA_SRAM_CLK_PORT, BP_LA_SRAM_CLK_PIN)
#define BP_LA_SRAM_CLOCK_HIGH() gpio_set(BP_LA_SRAM_CLK_PORT, BP_LA_SRAM_CLK_PIN)

#define BP_SRAM1_MOSI_PORT BP_LA_CHAN3_PORT
#define BP_SRAM1_MOSI_PIN BP_LA_CHAN3_PIN
#define BP_SRAM1_MISO_PORT BP_LA_CHAN2_PORT
#define BP_SRAM1_MISO_PIN BP_LA_CHAN2_PIN

#define BP_SRAM1_SIO0_PORT BP_LA_CHAN3_PORT
#define BP_SRAM1_SIO0_PIN BP_LA_CHAN3_PIN
#define BP_SRAM1_SIO1_PORT BP_LA_CHAN2_PORT
#define BP_SRAM1_SIO1_PIN BP_LA_CHAN2_PIN
#define BP_SRAM1_SIO2_PORT BP_LA_CHAN1_PORT
#define BP_SRAM1_SIO2_PIN BP_LA_CHAN1_PIN
#define BP_SRAM1_SIO3_PORT BP_LA_CHAN4_PORT
#define BP_SRAM1_SIO3_PIN BP_LA_CHAN4_PIN

#define BP_SRAM2_MOSI_PORT BP_LA_CHAN5_PORT
#define BP_SRAM2_MOSI_PIN BP_LA_CHAN5_PIN
#define BP_SRAM2_MISO_PORT BP_LA_CHAN6_PORT
#define BP_SRAM2_MISO_PIN BP_LA_CHAN6_PIN

#define BP_SRAM2_SIO0_PORT BP_LA_CHAN5_PORT
#define BP_SRAM2_SIO0_PIN BP_LA_CHAN5_PIN
#define BP_SRAM2_SIO1_PORT BP_LA_CHAN6_PORT
#define BP_SRAM2_SIO1_PIN BP_LA_CHAN6_PIN
#define BP_SRAM2_SIO2_PORT BP_LA_CHAN7_PORT
#define BP_SRAM2_SIO2_PIN BP_LA_CHAN7_PIN
#define BP_SRAM2_SIO3_PORT BP_LA_CHAN8_PORT
#define BP_SRAM2_SIO3_PIN BP_LA_CHAN8_PIN

