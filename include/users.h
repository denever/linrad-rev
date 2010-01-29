
// Give each hardware unit its own unique number.
// With few units, make these numbers powers of two to make
// address decoding unnecessary.
#define RX10700 16
#define RX70 32
#define RX144 8
#define RXHFA 2
#define RXHFA_GAIN 1
#define TX10700 4
#define TX70 64
// These two tx units may not be used simultaneously
// with this definition:
#define TX144 128
#define TXHFA 128


// These parameters define the frequency control window.
#define FREQ_MHZ_DECIMALS 3
#define FREQ_MHZ_DIGITS 3
#define FREQ_MHZ_ROUNDCORR 0.0005
#define FG_HSIZ ((FREQ_MHZ_DECIMALS+FREQ_MHZ_DIGITS+6)*text_width)
#define FG_VSIZ (2*text_height+5)

// *******************************************
//              The WSE converters
// *******************************************
//
// Radio hardware control is through the parallel port.
// Each hardware unit is controlled by serial data that is clocked
// into a shift register.
// The data in the shift register is transferred to a latch after
// the complete word has been transferred.
// The 8 bits of the output (data) port are used to select
// a hardware unit.
// If all 8 bits are zero, no unit is selected.
// In a small system with maximum 8 units, the 8 data
// pins can be used directly to select one unit each.
// By decoding the 8 bits one can select up to 255 units.
// The number of wires would become impractical and some
// other communication is recommended.
// These are the data pins on the 25 pin d-sub:
//   2  =  bit0
//   3  =  bit1
//   4  =  bit2
//   5  =  bit3
//   6  =  bit4
//   7  =  bit5
//   8  =  bit6
//   9  =  bit7
//
// The control port is used to clock serial data into the
// selected unit. 
// "Strobe" = pin 1 is clock.
// "Select input" = pin 17 is the serial data.

#define BIT0 1
#define BIT1 2
#define BIT2 4
#define BIT3 8
#define BIT4 16
#define BIT5 32
#define BIT6 64
#define BIT7 128


#define HWARE_CLOCK BIT0
#define HWARE_DATA BIT3
#define HWARE_RXTX BIT1
#define HWARE_MORSE_KEY BIT4

