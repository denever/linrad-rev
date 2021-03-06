
// **************  DLL defines  *********************************
// These definitions are taken from "D2XX Programmer's Guide"
// Future Technology Devices International Ltd 2006
// They are for SDR-14 and SDR-IQ

#define FT_OPEN_BY_SERIAL_NUMBER    1
#define FT_OPEN_BY_DESCRIPTION      2

#define FT_OK 0
#define FT_INVALID_HANDLE 1
#define FT_DEVICE_NOT_FOUND 2
#define FT_DEVICE_NOT_OPENED 3
#define FT_IO_ERROR 4
#define FT_INSUFFICIENT_RESOURCES 5
#define FT_INVALID_PARAMETER 6
#define FT_INVALID_BAUD_RATE 7
#define FT_DEVICE_NOT_OPENED_FOR_ERASE 8
#define FT_DEVICE_NOT_OPENED_FOR_WRITE 9
#define FT_FAILED_TO_WRITE_DEVICE 10
#define FT_EEPROM_READ_FAILED 11
#define FT_EEPROM_WRITE_FAILED 12
#define FT_EEPROM_ERASE_FAILED 13
#define	FT_EEPROM_NOT_PRESENT 14
#define	FT_EEPROM_NOT_PROGRAMMED 15
#define	FT_INVALID_ARGS 16
#define FT_NOT_SUPPORTED 17
#define	FT_OTHER_ERROR 18


typedef PVOID	FT_HANDLE;
typedef ULONG	FT_STATUS;

typedef FT_STATUS (WINAPI *sdrOpenEx)(PVOID, DWORD, FT_HANDLE *);
typedef FT_STATUS (WINAPI *sdrRead)(FT_HANDLE, LPVOID, DWORD, LPDWORD);
typedef FT_STATUS (WINAPI *sdrWrite)(FT_HANDLE, LPVOID, DWORD, LPDWORD);
typedef FT_STATUS (WINAPI *sdrClose)(FT_HANDLE);
typedef FT_STATUS (WINAPI *sdrSetTimeouts)(FT_HANDLE, DWORD, DWORD);

// *****************************************************************
// Defines for perseususb.dll for the Perseus hardware.

typedef int   (WINAPI *PtrToGetDLLVersion)(void);
typedef int   (WINAPI *PtrToOpen)(void);
typedef int   (WINAPI *PtrToClose)(void);
typedef char* (WINAPI *PtrToGetDeviceName)(void);
typedef int   (WINAPI *PtrToEepromFunc)(UCHAR *, DWORD, UCHAR);
typedef int   (WINAPI *PtrToFwDownload)(char* fname);
typedef int   (WINAPI *PtrToFpgaConfig)(char* fname, Key48 *pKey);
typedef int   (WINAPI *PtrToSetAttPresel)(unsigned char idSet);
typedef int   (WINAPI *PtrToSio)(void *ctl, int nbytes);
typedef int   (WINAPI *PInputCallback)(char *pBuf, int nothing);
typedef int   (WINAPI *PtrToStartAsyncInput)(int dwBufferSize, 
                           PInputCallback pInputCallback, void *pExtra);
typedef int  (WINAPI *PtrToStopAsyncInput)(void);

