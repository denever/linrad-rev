// *********************************************************************
// *  This software uses the Perseus API which is defined through the  *
// *  following four files of the Microtelecom Software Defined Radio  *
// *  Developer Kit (SDRK):                                            *
// *  main.cpp, perseusdll.cpp, perseusdefs.h, perseusdll.h            *
// *  To use the Perseus hardware you need the perseususb.dll file     *
// *  You also need at least one of the following files:               *
// *  perseus125k24v21.sbs                                             *
// *  perseus250k24v21.sbs                                             *
// *  perseus500k24v21.sbs                                             *
// *  perseus1m24v21.sbs                                               *
// *  perseus2m24v21.sbs                                               *
// *  The dll and the sbs files are copyright protected and only       *
// *  available at the official Microtelecom Internet site:            *
// *  http://www.microtelecom.it                                       *
// *********************************************************************
#ifndef PERSEUSDEF_H
#define PERSEUSDEF_H

#define PERSEUS_SAMPLING_CLOCK 80000000.0
#define MAX_PERSEUS_FILTER 11

#define MAX_PERSEUS_RATES 6

// Preselector Settings defines
#define PERSEUS_FLT_1	0		// VLF LF MF Low-pass filter
#define PERSEUS_FLT_2	1		// BPF1
#define PERSEUS_FLT_3	2		// BPF2
#define PERSEUS_FLT_4	3		// ...
#define PERSEUS_FLT_5	4
#define PERSEUS_FLT_6	5
#define PERSEUS_FLT_7	6
#define PERSEUS_FLT_8	7
#define PERSEUS_FLT_9	8
#define PERSEUS_FLT_10	9		// ..
#define PERSEUS_FLT_WB	10		// Preselector By-Pass (Wide-Band Mode)

// Upper cutoff frequency of the preselections filter in Hz
#define PERSEUS_FLT_1_FC	 1700000
#define PERSEUS_FLT_2_FC	 2100000
#define PERSEUS_FLT_3_FC	 3000000
#define PERSEUS_FLT_4_FC	 4200000
#define PERSEUS_FLT_5_FC	 6000000
#define PERSEUS_FLT_6_FC	 8400000
#define PERSEUS_FLT_7_FC	12000000
#define PERSEUS_FLT_8_FC	17000000
#define PERSEUS_FLT_9_FC	24000000
#define PERSEUS_FLT_10_FC	32000000


// Values returned by FirmwareDownload()
#define IHEX_DOWNLOAD_OK              0 //FX2 Firmware successfully downloaded
#define IHEX_DOWNLOAD_FILENOTFOUND    1 //FX2 Firmware file not found
#define IHEX_DOWNLOAD_IOERROR         2 //USB IO Error
#define IHEX_DOWNLOAD_INVALIDHEXREC   3 //Invalid HEX Record
#define IHEX_DOWNLOAD_INVALIDEXTREC   4 //Invalide Extended HEX Record

// Values returned by FpgaConfig()
#define FPGA_CONFIG_OK              0 //FPGA successfully configured
#define FPGA_CONFIG_FILENOTFOUND    1 //FPGA bitstream file not found
#define FPGA_CONFIG_IOERROR         2 //USB IO Error
#define FPGA_CONFIG_FWNOTLOADED     3 //FX2 Firmware not loaded


// Perseus SIO (serial I/O) interface definitions

// Receiver control bit masks (ctl field of SIOCTL data structure)
#define PERSEUS_SIO_FIFOEN    0x01 //Enable inbound data USB endpoint
#define PERSEUS_SIO_DITHER    0x02 //Enable ADC dither generator
#define PERSEUS_SIO_GAINHIGH  0x04 //Enable ADC preamplifier
#define PERSEUS_SIO_RSVD      0x08 //For Microtelecm internal use only
#define PERSEUS_SIO_TESTFIFO  0x10 //For Microtelecm internal use only
#define PERSEUS_SIO_ADCBYPASS 0x20 //For Microtelecm internal use only
#define PERSEUS_SIO_ADCATT60  0x40 //For Microtelecm internal use only
#define PERSEUS_SIO_SCRAMBLE  0x80 //For Microtelecm internal use only

// Perseus receiver USB buffering scheme definitions (never touch)
#define PERSEUS_SAMPLESPERFRAME	170   // Samples in one FX2 USB transaction
#define PERSEUS_COMPONENTSPERSAMPLE 2 // I & Q components
#define PERSEUS_BYTESPERCOMPONENT   3 // 24 bit data
// Derived defines
#define PERSEUS_BYTESPERSAMPLE (PERSEUS_BYTESPERCOMPONENT*PERSEUS_COMPONENTSPERSAMPLE)
#define PERSEUS_BYTESPERFRAME (PERSEUS_SAMPLESPERFRAME*PERSEUS_BYTESPERSAMPLE)

// When calling PerseusStartAsyncInput(...)
// the buffer size must always be a multiple of PERSEUS_BYTESPERFRAME
#define PERSEUS_BYTESPERFRAME (PERSEUS_SAMPLESPERFRAME*PERSEUS_BYTESPERSAMPLE)

// SIO Receiver Control data structure
#pragma pack(1)
typedef struct {
unsigned char ctl; // Receiver control bit mask
int freg;          // Receiver DDC LO frequency register
} SIOCTL;
#pragma pack()

// Microtelecom original FPGA bitstream signature
typedef struct {
unsigned char sz[6];
} Key48;

extern char *perseus_bitstream_names[MAX_PERSEUS_RATES];

extern Key48 perseus125k24v11_signature;
extern Key48 perseus250k24v11_signature;
extern Key48 perseus500k24v11_signature;
extern Key48 perseus1m24v11_signature;
extern Key48 *perseus_bitstream_signatures[MAX_PERSEUS_RATES];
extern SIOCTL sioctl;
extern int perseus_bytesperbuf;

#endif
