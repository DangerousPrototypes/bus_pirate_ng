


typedef struct _testpin
{
	uint32_t portout;
	uint32_t portin;
	uint16_t pinout;
	uint16_t pinin;

} _testpin;


void selftest(void);
int checkpin(uint32_t portout, uint16_t pinout, uint32_t portin, uint16_t pinin, int mode);
int checksram(uint8_t fillbyte);


#define MODE_PP	0
#define MODE_OD 1
