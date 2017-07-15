/**
 * ==========================================================================
 * Blazej's Twizy Lead Acid "BMS": Configuration
 * ==========================================================================
 */
#ifndef _BlazejBMS_config_h
#define _BlazejBMS_config_h

// Bluetooth baud rate: (i.e. 57600 / 38400 / 19200 / 9600)
#define BT_BAUD 9600

// Maximum charge current to use [A]:
#define MAX_CHARGE_CURRENT 30
// Charge current → power drawn from socket:
// 35 A = 2,2 kW
// 30 A = 2,1 kW
// 25 A = 1,7 kW
// 20 A = 1,4 kW
// 15 A = 1,0 kW
// 10 A = 0,7 kW
//  5 A = 0,4 kW

// Maximum driving & recuperation power limits to use [W]:
#define MAX_DRIVE_POWER 18000
#define MAX_RECUP_POWER 10000

// Drive power cutback [%]:
// (100% at FULL → 100% at <SOC1>% → <LVL2>% at <SOC2>% → 0% at EMPTY)
#define DRV_CUTBACK_SOC1    90
#define DRV_CUTBACK_SOC2    45
#define DRV_CUTBACK_LVL2    50

// Charge power cutback [%]:
// (100% at EMPTY → 100% at <SOC>% → 0% at FULL)
#define CHG_CUTBACK_SOC     85

// Lead acid voltage range for discharging [V]:
#define VMIN_DRV (4 * 11.2)
#define VMAX_DRV (4 * 13.0)

// Lead acid voltage range for charging [V]:
#define VMIN_CHG (4 * 12.0)
#define VMAX_CHG (4 * 14.4)

// Analog input port assignment:
#define PORT_TEMP     A0  // temperature sensor LM35D
#define PORT_C1       A1  // voltage divider 60 V
#define PORT_C2       A2  // voltage divider 45 V
#define PORT_C3       A3  // voltage divider 30 V
#define PORT_C4       A4  // voltage divider 15 V

// Set to 1 to enable input port calibration outputs:
#define INPUT_CALIBRATION     0

// Voltage analog input scaling:
#define SCALE_C1      (60.0 / 3.8202247191 * 5.0 * 1.02652)
#define SCALE_C2      (45.0 / 4.0909090909 * 5.0 * 1.01857)
#define SCALE_C3      (30.0 / 3.7918215613 * 5.0 * 1.02534)
#define SCALE_C4      (15.0 / 3.8059701492 * 5.0 * 1.02344)

// Voltage warning/error thresholds [V]:
#define VOLT_DIFF_WARN        3.0
#define VOLT_DIFF_ERROR       5.0
#define VOLT_DIFF_SHUTDOWN   10.0

// SOC smoothing [samples]:
#define SMOOTH_SOC_DOWN       700  // adaption to lower voltage
#define SMOOTH_SOC_UP         400  // adaption to higher voltage
#define SMOOTH_SOC_UP_CHG      50  //  adaption to higher voltage while charging

// OPTION: Scale voltage to SOC difference to current [A]:
//#define SCALE_CURRENT         150
//#define OFFSET_CURRENT        -70      

// Temperature analog input scaling:
// LM35D: +2 .. +100°, 10 mV / °C => 100 °C = 1.0 V
#define BASE_TEMP       2.0
#define SCALE_TEMP      500.0

// Temperature smoothing [samples]:
#define SMOOTH_TEMP     30

// Temperature warning/error thresholds [°C]:
#define TEMP_WARN       40
#define TEMP_ERROR      45
#define TEMP_SHUTDOWN   50

#endif // _BlazejBMS_config_h
