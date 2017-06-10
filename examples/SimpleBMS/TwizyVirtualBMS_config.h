/**
 * ==========================================================================
 * Twizy Virtual BMS: Configuration
 * ==========================================================================
 */

#ifndef _TwizyVirtualBMS_config_h
#define _TwizyVirtualBMS_config_h

// Serial debug output:
// Level 0 = none, only output init & error messages
// Level 1 = log state transitions & CAN statistics
// Level 2 = log CAN frame dumps (10 second interval)
#define TWIZY_DEBUG_LEVEL         1

// Set to 0 to disable CAN transmissions for testing:
#define TWIZY_CAN_SEND            1

// CAN send timing is normally 10 ms (10.000 us).
// You may need to lower this if your Arduino is too slow.
#define TWIZY_CAN_CLOCK_US        10000

// Set your MCP clock frequency here:
#define TWIZY_CAN_MCP_FREQ        MCP_16MHZ

// Set your SPI CS pin number here:
#define TWIZY_CAN_CS_PIN          10

// If you've connected the CAN module's IRQ pin:
//#define TWIZY_CAN_IRQ_PIN       2

// Set your 3MW control pin here:
#define TWIZY_3MW_CONTROL_PIN     3

#endif // _TwizyVirtualBMS_config_h
