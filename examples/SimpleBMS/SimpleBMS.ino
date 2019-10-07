/**
 * ==========================================================================
 * Twizy Virtual BMS Example: SimpleBMS
 * ==========================================================================
 *
 * - Derive SOC and power control from pack voltage measured using a
 *   simple voltage divider connected to an analog port.
 * - Measure pack temperature using a simple temperature sensor (LM35D)
 *   and issue temperature and STOP warnings accordingly.
 * 
 * Author: Michael Balzer <dexter@dexters-web.de>
 * 
 * Libraries used:
 *  - TwizyVirtualBMS: https://github.com/dexterbg/Twizy-Virtual-BMS
 *  - MCP_CAN: https://github.com/coryjfowler/MCP_CAN_lib
 *  - TimerOne: https://github.com/PaulStoffregen/TimerOne
 * 
 * Licenses:
 *  This is free software under GNU Lesser General Public License (LGPL)
 *  https://www.gnu.org/licenses/lgpl.html
 * 
 */

#include "TwizyVirtualBMS_config.h"
#include "TwizyVirtualBMS.h"

TwizyVirtualBMS twizy;


// -----------------------------------------------------
// Configuration
// 

// Maximum driving & recuperation power limits to use [W]:
#define MAX_DRIVE_POWER       18000
#define MAX_RECUP_POWER       8000

// Maximum charge current to use [A]:
#define MAX_CHARGE_CURRENT    35

// Voltage range 0…100% for driving:
#define VMIN_DRV              42.0
#define VMAX_DRV              52.0

// Voltage range 0…100% for charging:
#define VMIN_CHG              42.0
#define VMAX_CHG              57.6

// Voltage analog input:
// i.e. voltage divider scaling 60V → 4.5V
// (you may need to add some tolerance correction factor)
#define PORT_VOLT             A0
#define SCALE_VOLT            (60.0 / 4.5 * 5.0)

// SOC smoothing samples:
#define SMOOTH_SOC_DOWN       30  // slow adaption to lower voltage
#define SMOOTH_SOC_UP         5   // fast adaption to higher voltage

// Temperature analog input:
// i.e. LM35D: 2…100 °C → 0…1.0 V  (10 mV/°C)
#define PORT_TEMP             A1
#define BASE_TEMP             2.0
#define SCALE_TEMP            (100.0 / 1.0 * 5.0)

// Temperature smoothing samples:
#define SMOOTH_TEMP           10


// -----------------------------------------------------
// Status
// 

float temp = 20.0;
float soc = 90.0;     // Note: needs to be below 100 to be able to start charging


// --------------------------------------------------------------------------
// Callback: handle state transition for BMS
//  - called by twizyEnterState() after Twizy handling
// Note: avoid complex operations, this needs to be fast.
// 
void bmsEnterState(TwizyState currentState, TwizyState newState) {
  
  // The charger will not start charging at SOC=100%, so lower a too
  // high SOC on wakeup to enable topping-off charges:
  if (currentState == Init && newState == Ready) {
    if (soc > 99) {
      soc -= 1;
      twizy.setSOC(soc);
      twizy.setChargeCurrent(5);
      Serial.print(F("bmsEnterState: soc lowered to "));
      Serial.println(soc, 2);
    }
  }
  
}


// --------------------------------------------------------------------------
// Callback: timer ticker
//  - called every 10 ms by twizyTicker() after twizy handling
//  - clockCnt cyclic range: 0 .. 2999 = 30 seconds (reset to 0 on Off/Init)
// Note: avoid complex operations, this needs to be fast.
// 
void bmsTicker(unsigned int clockCnt) {
  
  if (!twizy.inState(Off) && (clockCnt % 100 == 0)) {
    // per second:
    Serial.println(F("\nbmsTicker:"));
    
    float vpack, newsoc, newtemp;
    
    // read pack voltage:
    vpack = analogRead(PORT_VOLT) / 1024.0 * SCALE_VOLT;
    
    Serial.print(F("- vpack=")); Serial.println(vpack, 2);
    twizy.setVoltage(vpack, true);
    
    // derive SOC change from voltage:
    if (twizy.inState(Charging)) {
      newsoc = (vpack - VMIN_CHG) / (VMAX_CHG - VMIN_CHG) * 100.0;
    }
    else {
      newsoc = (vpack - VMIN_DRV) / (VMAX_DRV - VMIN_DRV) * 100.0;
    }
    
    // smooth SOC:
    if (newsoc < soc) {
      // slow adaption to lower voltages:
      soc = (soc * (SMOOTH_SOC_DOWN-1) + newsoc) / SMOOTH_SOC_DOWN;
    }
    else {
      // fast adaption to higher voltages:
      soc = (soc * (SMOOTH_SOC_UP-1) + newsoc) / SMOOTH_SOC_UP;
    }
    
    // sanitize...
    soc = constrain(soc, 0.0, 100.0);
    
    Serial.print(F("- soc=")); Serial.println(soc, 2);
    twizy.setSOC(soc);
    
    // derive power limits & charge current from SOC:
    if (soc >= 90.0) {
      // high SOC: scale down recuperation & charge power
      int recpwr = (100-soc) / 10.0 * MAX_RECUP_POWER;
      Serial.print(F("- recpwr=")); Serial.println(recpwr);
      twizy.setPowerLimits(MAX_DRIVE_POWER, recpwr);
      // charge automatically stops below 5.0 A, so keep min 5.0 until 100%:
      float chgcurr = (round(soc*100) == 10000) ? 0.0 : 5.0 + (100-soc) / 10.0 * (MAX_CHARGE_CURRENT-5.0);
      Serial.print(F("- chgcurr=")); Serial.println(chgcurr);
      twizy.setChargeCurrent(chgcurr);
    }
    else if (soc <= 20.0) {
      // low SOC: scale down drive power
      int drvpwr = soc / 20.0 * MAX_DRIVE_POWER;
      Serial.print(F("- drvpwr=")); Serial.println(drvpwr);
      twizy.setPowerLimits(drvpwr, MAX_RECUP_POWER);
      twizy.setChargeCurrent(MAX_CHARGE_CURRENT);
    }
    else {
      // normal SOC: allow max power & current
      twizy.setPowerLimits(MAX_DRIVE_POWER, MAX_RECUP_POWER);
      twizy.setChargeCurrent(MAX_CHARGE_CURRENT);
    }
    
    // read battery temperature:
    newtemp = BASE_TEMP + analogRead(PORT_TEMP) / 1024.0 * SCALE_TEMP;
    
    // smooth:
    temp = (temp * (SMOOTH_TEMP-1) + newtemp) / SMOOTH_TEMP;
    
    Serial.print(F("- temp=")); Serial.println(temp);
    twizy.setTemperature(temp, temp, true);
    
    // set error status if battery too hot:
    if (temp > 50) {
      twizy.setError(TWIZY_SERV_TEMP|TWIZY_SERV_STOP);
    }
    else if (temp > 40) {
      twizy.setError(TWIZY_SERV_TEMP);
    }
    else {
      twizy.setError(TWIZY_OK);
    }
    
  }
  
}


// -----------------------------------------------------
// SETUP
// 

void setup() {
  
  Serial.begin(1000000);
  
  twizy.begin();
  twizy.attachTicker(bmsTicker);
  twizy.attachEnterState(bmsEnterState);
  
  twizy.setPowerLimits(MAX_DRIVE_POWER, MAX_RECUP_POWER);
  twizy.setChargeCurrent(MAX_CHARGE_CURRENT);
  twizy.setSOH(100);
  twizy.setSOC(soc);
  twizy.setTemperature(temp, temp, true);
  twizy.setVoltage(50.0, true);
  twizy.setCurrent(0.0);
  twizy.setError(TWIZY_OK);
}


// -----------------------------------------------------
// MAIN LOOP
// 

void loop() {
  twizy.looper();
}
