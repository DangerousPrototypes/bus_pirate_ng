
typedef struct _modeConfig
{
	uint8_t numbits;				// number of used bits
	uint8_t wwr;					// write with read
	uint8_t hiz;					// are we in HiZ?
	uint8_t mode;					// which mode we are in?
	uint8_t pullups;				// pullup enabled? (0=off, 1=on)
	uint8_t vpumode;				// vpu mode (0=ext, 1=5v, 2=3v3)
	uint8_t bitorder;				// bitorder (0=msb, 1=lsb)
	uint8_t psu;					// psu (0=off, 1=on)
	uint8_t error;					// error occurred
	uint8_t displaymode;				// display mode (dec, hex, oct, bin)
} _modeConfig;


// define our globals
extern int cmdhead, cmdtail;
extern char cmdbuff[CMDBUFFSIZE];
extern struct _modeConfig modeConfig;
extern const char vpumodes[][4];
extern const char bitorders[][4];
extern const char states[][4];


//declarations
void consumewhitechars(void);
uint32_t getint(void);
void versioninfo(void);
void doUI(void);
void initUI(void);
void showstates(void);
void changemode(void);
void printhelp(void);
void getuserinput(void);

void changedisplaymode(void);

