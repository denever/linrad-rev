

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!                                                         !! 
// !!  Default is FALSE for all debug and manipulate options. !!
// !!                                                         !!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


// ************************************************************
//               ----   File structure  ----
// Note that the user can override these standard directories
// by specifying a full path.
// GIFDIR is where screen dumps are saved
// WAVDIR is where output audio files are saved
// RAWDIR is where input raw data files are saved
#if OSNUM == OS_FLAG_LINUX
#define GIFDIR "/home/linrad_data/"
#define WAVDIR "/home/linrad_data/"
#define RAWDIR "/home/linrad_data/"
#endif

#if OSNUM == OS_FLAG_WINDOWS
#define GIFDIR "C:\\linrad_data\\"
#define WAVDIR "C:\\linrad_data\\"
#define RAWDIR "C:\\linrad_data\\"
#endif
// ************************************************************
//               ----   Configuration   ----
// The coherent graph which also contains the S-meter is rectangular
// and it is set to a multiple of the font width in size.
// The side length will affect the number of digits for the S-meter
// and the screen space used for this window
#define COH_SIDE 7
// ************************************************************
//                ----  Debug tool  ----
// Set DUMPFILE to TRUE to get debug info in the file dmp.
// Use xz("string"); to get traces from code execution
// or make explicit writes to dmp. xz() will call
// fflush() and sync() and thus it contains information
// just before a crash that would otherwise not have been 
// physically written to the disk. 
// If you trace crashes, use fprintf(dmp,"%d.....",variables)
// followed by xz(" "); to physically write to disk.
// (Do not use xz() if you want fast processing)

//#define DUMPFILE TRUE
#define DUMPFILE FALSE
// *************************************************************
//        ---- manipulate calibration data ----
// Extract a calibration file from save files in Linrads .raw format
// and save them as the appropriate dsp_xxx_corr file.
//#define SAVE_RAWFILE_CORR TRUE
#define SAVE_RAWFILE_CORR FALSE
// *************************************************************
//        ---- manipulate calibration data ----
// Extract a calibration file from save files in Linrads .raw format
// and save them as the appropriate dsp_xxx_corr file.
//#define SAVE_RAWFILE_IQCORR TRUE
#define SAVE_RAWFILE_IQCORR FALSE
// *************************************************************
//        ---- manipulate calibration data ----
// Use calibration data from the appropriate dsp_xx_corr file
// and disregard whatever data that is contained in the
// calibration file.
//#define DISREGARD_RAWFILE_CORR TRUE
#define DISREGARD_RAWFILE_CORR FALSE
// **************************************************************
//        ---- manipulate calibration data ----
// Skip the mirror image calibration that is present in a .raw file.
//#define DISREGARD_RAWFILE_IQCORR TRUE
#define DISREGARD_RAWFILE_IQCORR FALSE
// ************************************************************
//                ----  Debug tool  ----
// Linrad can display buffer usage as bars on screen when "T"
// is pressed to provide timing info. This option is however
// rather time-consuming and not often helpful 
//#define BUFBARS TRUE
#define BUFBARS FALSE
