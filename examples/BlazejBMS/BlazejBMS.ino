/**
 * ==========================================================================
 * Blazej's Twizy Lead Acid "BMS"
 * ==========================================================================
 * 
 * Hardware setup:
 *  - Arduino Nano + NiRen MCP2515_CAN (16 MHz) + relay + power regulator
 *  - HC-06 Bluetooth module (AltSoftSerial pin 8+9)
 * Sensors:
 *  - 1x LM35D temperature sensor (PORT_TEMP)
 *  - 4x voltage divider at 60/45/30/15 V (PORT_C1…4)
 * 
 * Authors:
 *  - Michael Balzer    <dexter@dexters-web.de>
 *  - Błażej Błaszczyk  <blazej.blaszczyk@pascal-engineering.com>
 * 
 * Libraries used:
 *  - TwizyVirtualBMS   https://github.com/dexterbg/Twizy-Virtual-BMS
 *  - MCP_CAN           https://github.com/coryjfowler/MCP_CAN_lib
 *  - FlexiTimer2       https://github.com/PaulStoffregen/FlexiTimer2
 *  - AltSoftSerial     https://github.com/PaulStoffregen/AltSoftSerial
 * 
 * License:
 *  This is free software under GNU Lesser General Public License (LGPL)
 *  https://www.gnu.org/licenses/lgpl.html
 *  
 */

#define BLAZEJ_BMS_VERSION    "V2.1.0 (2017-07-05)"

#include "TwizyVirtualBMS_config.h"
#include "TwizyVirtualBMS.h"
#include "AltSoftSerial.h"
#include "BlazejBMS_config.h"

TwizyVirtualBMS twizy;

// Bluetooth software serial port:
// Note: AltSoftSerial uses fixed pins!
// i.e. Arduino Nano: RX = pin 8, TX = pin 9
AltSoftSerial bt;


// --------------------------------------------------------------------------
// State variables
//

float temp = 20.0;

float
  vpack = VMAX_DRV;

float
  c1 = VMAX_DRV / 4,
  c2 = VMAX_DRV / 4,
  c3 = VMAX_DRV / 4,
  c4 = VMAX_DRV / 4;

float
  soc = 99.0;

float  curr = 0.0;

int drvpwr = MAX_DRIVE_POWER;
int recpwr = MAX_RECUP_POWER;

int chgcur = MAX_CHARGE_CURRENT;

unsigned long error = TWIZY_OK;


// --------------------------------------------------------------------------
// Callback: handle state transition for BMS
//  - called by twizyEnterState() after Twizy handling
// Note: avoid complex operations, this needs to be fast.
// 
void bmsEnterState(TwizyState currentState, TwizyState newState) {
  
  // lower SOC at switch-on to prevent immediate charge stop:
  if (currentState == Init && newState == Ready) {
    if (soc > 99) {
      soc -= 1;
      twizy.setSOC(soc);
      twizy.setChargeCurrent(5);
      Serial.print(F("bmsEnterState: soc lowered to "));
      Serial.println(soc, 1);
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
  
  // full second?
  if (clockCnt % 100 != 0) {
    return;
  }
  
  if (twizy.state() != Off) {
    
    Serial.println(F("\nbmsTicker:"));
    
    error = TWIZY_OK;
    
    
    // ----------------------------------------------------------------------
    // bmsTicker: Read stacked cell voltages
    //
    
    c1 = analogRead(PORT_C1) / 1024.0 * SCALE_C1;   // 60 V
    c2 = analogRead(PORT_C2) / 1024.0 * SCALE_C2;   // 45 V
    c3 = analogRead(PORT_C3) / 1024.0 * SCALE_C3;   // 30 V
    c4 = analogRead(PORT_C4) / 1024.0 * SCALE_C4;   // 15 V

    #if INPUT_CALIBRATION >= 1
      // raw output for calibration:
      Serial.print(F("< raw c1 = ")); Serial.println(c1, 2);
      Serial.print(F("<     c2 = ")); Serial.println(c2, 2);
      Serial.print(F("<     c3 = ")); Serial.println(c3, 2);
      Serial.print(F("<     c4 = ")); Serial.println(c4, 2);
    #endif
    
    // derive single cell voltages from stacked voltages:
    vpack = c1;
    c1 -= c2;
    c2 -= c3;
    c3 -= c4;
    

    // ----------------------------------------------------------------------
    // bmsTicker: Derive SOC from voltage
    // 
    // - newsoc = SOC in operation mode voltage range
    // - soc = smoothed operation mode SOC
    //    (used to derive drive & recup power & charge current)
    //
    
    float newsoc;
    
    // voltage range depends on operation mode:
    if (twizy.state() == Charging) {
      newsoc = (vpack - VMIN_CHG) / (VMAX_CHG - VMIN_CHG) * 100.0;
    }
    else {
      newsoc = (vpack - VMIN_DRV) / (VMAX_DRV - VMIN_DRV) * 100.0;
    }
    
    // smooth...
    if (newsoc < soc) {
      // slow adaption to lower voltages:
      soc = ((soc * (SMOOTH_SOC_DOWN-1)) + newsoc) / SMOOTH_SOC_DOWN;
    }
    else {
      if (twizy.state() == Charging) {
        // fast adaption while charging:
        soc = ((soc * (SMOOTH_SOC_UP_CHG-1)) + newsoc) / SMOOTH_SOC_UP_CHG;
      }
      else {
        // slow adaption while driving:
        soc = ((soc * (SMOOTH_SOC_UP-1)) + newsoc) / SMOOTH_SOC_UP;
      }
    }
    
    // sanitize...
    soc = constrain(soc, 0.0, 100.0);


    // ----------------------------------------------------------------------
    // bmsTicker: Derive power limits & charge current from SOC
    //

    // scale down drive power for low SOC:
    //  100% at FULL → 100% at <SOC1> → <LVL2> at <SOC2> → 0% at EMPTY
    #define SOC2_DRIVE_POWER ((DRV_CUTBACK_LVL2 / 100.0f) * MAX_DRIVE_POWER)
    if (soc <= DRV_CUTBACK_SOC2) {
      float factor = soc / DRV_CUTBACK_SOC2;
      drvpwr = factor * SOC2_DRIVE_POWER;
    }
    else if (soc <= DRV_CUTBACK_SOC1) {
      float factor = ((soc - DRV_CUTBACK_SOC2) / (DRV_CUTBACK_SOC1 - DRV_CUTBACK_SOC2));
      drvpwr = SOC2_DRIVE_POWER + (factor * (MAX_DRIVE_POWER - SOC2_DRIVE_POWER));
    }
    else {
      drvpwr = MAX_DRIVE_POWER;
    }
    
    // scale down recuperation power & charge current for high SOC:
    //  0% at FULL → 100% at <CHG_CUTBACK_SOC> → 100% at EMPTY
    if (soc > 99.99) {
      // stop charge & reduce recuperation at 100% SOC:
      recpwr = 500; // TODO: should be 0 when driving, but affects D/R change
      chgcur = 0;
    }
    else if (soc >= CHG_CUTBACK_SOC) {
      // keep min 1000W / 5A below 100% SOC:
      float factor = ((100 - soc) / (100 - CHG_CUTBACK_SOC));
      recpwr = 1000 + (factor * (MAX_RECUP_POWER - 1000));
      chgcur = 5 + (factor * (MAX_CHARGE_CURRENT - 5));
    }
    else {
      recpwr = MAX_RECUP_POWER;
      chgcur = MAX_CHARGE_CURRENT;
    }
    
    
    // ------------------------------------------------------------
    // bmsTicker: Check cell voltage difference (min - max)
    //

    float cmin = min(c1, min(c2, min(c3, c4)));
    float cmax = max(c1, max(c2, max(c3, c4)));
    float cdif = cmax - cmin;

    #if INPUT_CALIBRATION >= 1
      Serial.print(F(">   cmin = ")); Serial.println(cmin, 2);
      Serial.print(F(">   cmax = ")); Serial.println(cmax, 2);
      Serial.print(F(">   cdif = ")); Serial.println(cdif, 2);
    #endif

    if (cdif >= VOLT_DIFF_SHUTDOWN) {
      // cell difference is critical: emergency shutdown
      Serial.println(F("!!! VOLT_SHUTDOWN"));
      bt.println(F("!!! VOLT_SHUTDOWN"));
      error |= TWIZY_SERV_BATT | TWIZY_SERV_STOP;
      twizy.enterState(Error);
    }
    else if (cdif >= VOLT_DIFF_ERROR) {
      // cell difference is high: set STOP signal, reduce drive power, stop recuperation & charge:
      Serial.println(F("!!! VOLT_ERROR"));
      bt.println(F("!!! VOLT_ERROR"));
      error |= TWIZY_SERV_BATT | TWIZY_SERV_STOP;
      drvpwr /= 4;
      recpwr /= 4;
      chgcur = 0;
    }
    else if (cdif >= VOLT_DIFF_WARN) {
      // cell difference detected: reduce power & charge levels:
      Serial.println(F("!!! VOLT_WARN"));
      bt.println(F("!!! VOLT_WARN"));
      error |= TWIZY_SERV_BATT;
      drvpwr /= 2;
      recpwr /= 2;
      chgcur = min(chgcur, 5);
    }
    
    
    // ----------------------------------------------------------------------
    // bmsTicker: Read & check battery temperature
    //
    
    float newtemp;

    // dual read _seems_ to yield better results (LM35D issue?)
    newtemp = analogRead(PORT_TEMP);
    newtemp = BASE_TEMP + analogRead(PORT_TEMP) / 1024.0 * SCALE_TEMP;

    // raw output for calibration:
    #if INPUT_CALIBRATION >= 1
      Serial.print(F("<   temp = ")); Serial.println(newtemp, 2);
    #endif
    
    // smooth...
    temp = (temp * (SMOOTH_TEMP-1) + newtemp) / SMOOTH_TEMP;

    if (temp > TEMP_SHUTDOWN) {
      // battery is burning: emergency shutdown
      Serial.println(F("!!! TEMP_SHUTDOWN"));
      bt.println(F("!!! TEMP_SHUTDOWN"));
      error |= TWIZY_SERV_TEMP | TWIZY_SERV_STOP;
      twizy.enterState(Error);
    }
    else if (temp > TEMP_ERROR) {
      // battery very hot: set STOP signal, stop recuperation, stop charge:
      Serial.println(F("!!! TEMP_ERROR"));
      bt.println(F("!!! TEMP_ERROR"));
      error |= TWIZY_SERV_TEMP | TWIZY_SERV_STOP;
      drvpwr /= 4;
      recpwr = 0;
      chgcur = 0;
    }
    else if (temp > TEMP_WARN) {
      // battery hot, show warning, reduce recuperation, reduce charge current:
      Serial.println(F("!!! TEMP_WARN"));
      bt.println(F("!!! TEMP_WARN"));
      error |= TWIZY_SERV_TEMP;
      drvpwr /= 2;
      recpwr /= 2;
      chgcur = min(chgcur, 5);
    }


    // ----------------------------------------------------------------------
    // bmsTicker: Estimate current level
    //

    if (twizy.state() == Charging) {
      curr = chgcur;
    }
    else {
      // OPTION: derive current estimation from voltage difference:
      //curr = ((newsoc - soc) / 100) * SCALE_CURRENT + OFFSET_CURRENT;
      //curr = constrain(curr, -500.0, 500.0);
      curr = 0.0;
    }


    // ----------------------------------------------------------------------
    // bmsTicker: Update Twizy state
    //

    #if INPUT_CALIBRATION >= 1
      Serial.println();
    #endif
    
    twizy.setVoltage(vpack, true);
    twizy.setCurrent(curr);
    twizy.setTemperature(temp, temp, true);
    twizy.setSOC(soc);
    twizy.setPowerLimits(drvpwr, recpwr);
    twizy.setChargeCurrent(chgcur);
    twizy.setError(error);
    
    
    // ----------------------------------------------------------------------
    // bmsTicker: Output state to serial port
    //

    Serial.println();
    Serial.print(F("- volt   = ")); Serial.println(vpack, 1);
    Serial.print(F("- ...c1  = ")); Serial.println(c1, 1);
    Serial.print(F("- ...c2  = ")); Serial.println(c2, 1);
    Serial.print(F("- ...c3  = ")); Serial.println(c3, 1);
    Serial.print(F("- ...c4  = ")); Serial.println(c4, 1);
    Serial.print(F("- temp   = ")); Serial.println(temp, 1);
    Serial.print(F("- curr   = ")); Serial.println(curr, 1);
    Serial.println();
    Serial.print(F("- soc%   = ")); Serial.println(soc, 1);
    Serial.print(F("- drvpwr = ")); Serial.println(drvpwr);
    Serial.print(F("- recpwr = ")); Serial.println(recpwr);
    Serial.print(F("- chgcur = ")); Serial.println(chgcur);
    

    // ----------------------------------------------------------------------
    // bmsTicker: Output state to bluetooth port
    //
    
    bt.print(FS(twizyStateName[twizy.state()]));
    bt.print(F(" -- "));
    bt.print(temp, 1);    bt.print(F(" °C -- "));
    bt.print(soc, 1);     bt.print(F(" %SOC -- "));
    bt.println();
    if (error != TWIZY_OK) {
      bt.print(F("ERROR: "));
      bt.println(error & 0x0fff, HEX);
    }
    bt.print(vpack, 1);   bt.print(F(" V -- "));
    bt.print(curr, 1);    bt.print(F(" A -- "));
    bt.print(cdif, 1);    bt.print(F(" CD"));
    bt.println();
    bt.print(c1, 1);      bt.print(F(" C1 -- "));
    bt.print(c2, 1);      bt.print(F(" C2 -- "));
    bt.print(c3, 1);      bt.print(F(" C3 -- "));
    bt.print(c4, 1);      bt.print(F(" C4 -- "));
    bt.println();
    bt.println();  
    

  } // if (twizy.state() != Off)
  
} // bmsTicker()



// -----------------------------------------------------
// SETUP
// 

void setup() {
  
  Serial.begin(115200);
  Serial.println(F("Blazej-BMS " BLAZEJ_BMS_VERSION));
  
  bt.begin(BT_BAUD);
  
  twizy.begin();
  twizy.attachTicker(bmsTicker);
  twizy.attachEnterState(bmsEnterState);
  
  // Init:
  twizy.setPowerLimits(drvpwr, recpwr);
  twizy.setChargeCurrent(chgcur);
  twizy.setSOC(soc);
  twizy.setTemperature(temp, temp, true);
  twizy.setVoltage(vpack, true);
  twizy.setError(error);
  
  twizy.setSOH(100);
  twizy.setCurrent(0.0);

  #if TWIZY_CAN_SEND == 0
    // Dry run:
    twizy.enterState(Ready);
  #endif
}


// -----------------------------------------------------
// MAIN LOOP
// 

void loop() {
  twizy.looper();
}

