
#define SDR14_SAMPLING_CLOCK 66.66666666
#define PERSEUS_SAMPLING_CLOCK 80000000.0

extern double adjusted_sdr_clock;
extern char *sdr14_name_string;
extern char *sdriq_name_string;
extern char *perseus_name_string;


void close_sdr14(void);
void close_perseus(void);
int init_sdr14(void);
int init_perseus(void);

