/**
 * ==========================================================================
 * Twizy Virtual BMS Example: Template
 * ==========================================================================
 * 
 * - You can register some callbacks with the Virtual BMS, this
 *   template contains prototypes for all of them as well as the
 *   basic initialization and integration.
 * 
 * Note: if you don't need a callback, you don't need to define and attach it.
 * 
 * Author: Michael Balzer <dexter@dexters-web.de>
 * 
 * Twizy CAN object dictionary:
 * https://docs.google.com/spreadsheets/d/1gOrG9rnGR9YuMGakAbl4s97a6irHF6UNFV1TS5Ll7MY/edit#gid=0
 * (Maintainer: Michael Balzer <dexter@dexters-web.de>)
 * 
 * Twizy BMS CAN & hardware protocol decoding and reengineering has been done 
 * by a joint effort of (in reverse alphabetical order):
 *  - Lutz Schäfer <aquillo@t-online.de>
 *  - Pascal Ripp <pascal@ripp.li>
 *  - Bernd Eickhoff <b.eickhoff@gmx.de>
 *  - Michael Balzer <dexter@dexters-web.de>
 * 
 * Libraries used:
 *  - MCP_CAN: https://github.com/coryjfowler/MCP_CAN_lib
 *  - TimerOne: https://github.com/PaulStoffregen/TimerOne
 * 
 * Licenses:
 *  This is free software and information under the following licenses:
 *  - Source code: GNU Lesser General Public License (LGPL)
 *    https://www.gnu.org/licenses/lgpl.html
 *  - Documentation: GNU Free Documentation License (FDL)
 *    https://www.gnu.org/licenses/fdl.html
 * 
 */

#include "TwizyVirtualBMS_config.h"
#include "TwizyVirtualBMS.h"

TwizyVirtualBMS twizy;


// --------------------------------------------------------------------------
// Callback: handle state transition for BMS
//  - called by twizyEnterState() after Twizy handling
// Note: avoid complex operations, this needs to be fast.
// 
void bmsEnterState(TwizyState currentState, TwizyState newState) {
  
  // 
  // Add your code here
  // 
  
}


// --------------------------------------------------------------------------
// Callback: check if BMS allows state transition
//  - return true when BMS is ready for newState
//  - called by twizyTicker() every 10 ms before sending frames
//  - called for newState: Ready, Charging, Driving
// Note: avoid complex operations, this needs to be fast.
// 
bool bmsCheckState(TwizyState currentState, TwizyState newState) {

  // 
  // Add your code here
  // 
  
  return true;
}


// --------------------------------------------------------------------------
// Callback: process received CAN message
//  - called on reception of a CAN frame that passed the filters
//    (i.e. normally only IDs 0x423, 0x597 & 0x599)
// Note: avoid complex operations, this needs to be fast.
// 
void bmsProcessCanMsg(unsigned long rxId, byte rxLen, byte *rxBuf) {
  
  // 
  // Add your code here
  // 
  
}


// --------------------------------------------------------------------------
// Callback: timer ticker
//  - called every 10 ms by twizyTicker() after twizy handling
//  - not called when Twizy is in state Off
//  - clockCnt cyclic range: 0 .. 2999 = 30 seconds
// Note: avoid complex operations, this needs to be fast.
// 
void bmsTicker(unsigned int clockCnt) {
  
  // 
  // Add your code here
  // 
  
  if (clockCnt % 100 == 0) {
    // per second…
  }

}


// -----------------------------------------------------
// SETUP
// 

void setup() {
  
  Serial.begin(115200);
  
  // Init TwizyVirtualBMS:
  twizy.begin();
  
  // Attach callbacks:
  twizy.attachEnterState(bmsEnterState);
  twizy.attachCheckState(bmsCheckState);
  twizy.attachProcessCanMsg(bmsProcessCanMsg);
  twizy.attachTicker(bmsTicker);
  
  // Init data:
  twizy.setPowerLimits(18000, 8000);
  twizy.setChargeCurrent(35);
  twizy.setSOH(100);
  twizy.setSOC(90);
  twizy.setTemperature(20, 20, true);
  twizy.setVoltage(50.0, true);
  twizy.setCurrent(0.0);
  twizy.setError(TWIZY_OK);

  // 
  // Add your code here
  // 
  
}


// -----------------------------------------------------
// MAIN LOOP
// 

void loop() {
  
  // TwizyVirtualBMS event handler:
  twizy.looper();
  
  // 
  // Add your code here
  // 
  
}
