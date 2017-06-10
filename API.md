# API & Usage

Hint: begin with the "Template" example to get a standard usage template including all callbacks.


## Library inclusion

  1. Create a TwizyVirtualBMS object (`twizy` is a good name for this)
  2. Call `twizy.begin()` in your setup function
  3. Call `twizy.looper()` in your main loop:
        ```c++
        #include "TwizyVirtualBMS_config.h"
        #include "TwizyVirtualBMS.h"
        
        TwizyVirtualBMS twizy;
        
        void setup() {
          Serial.begin(115200);
          twizy.begin();
        }
        
        void loop() {
          twizy.looper();
        }
        ```


## Twizy control functions

Note: all control functions validate their parameters. If you pass any value out of bounds, nothing will be changed, an error message will be generated on the serial port and the function will return `false`.

  - `bool setChargeCurrent(int amps)` -- Set battery charge current level
    - amps: 0 .. 35 A current level at battery
    - will stop charge if set to 0 while charging
    - Note: this has a 5 A resolution rounded downwards
    - Note: 35 A will not be reached with current charger generation (max ~32 A)

      | Current level | Power drawn from socket |
      | ------------- | ----------------------- |
      | 35 A          | ~ 2.2 kW                |
      | 30 A          | ~ 2.1 kW                |
      | 25 A          | ~ 1.7 kW                |
      | 20 A          | ~ 1.4 kW                |
      | 15 A          | ~ 1.0 kW                |
      | 10 A          | ~ 0.7 kW                |
      |  5 A          | ~ 0.4 kW                |

  - `bool setCurrent(float amps)` -- Set momentary battery pack current level
    - amps: -500 .. +500 (A, positive = charge, negative = discharge)
  
  - `bool setSOC(float soc)` -- Set state of charge
    - soc: 0.00 .. 100.00 (%)
    - Note: the charger will not start charging at SOC=100%
  
  - `bool setPowerLimits(unsigned int drive, unsigned int recup)` -- Set SEVCON power limits
    - drive: 0 .. 30000 (W)
    - recup: 0 .. 30000 (W)
    - Note: both limits have a resolution of 500 W and will be rounded downwards
    - Note: these limits do not apply if the SEVCON has been configured to ignore them (as done in some tuning configurations)
  
  - `bool setSOH(int soh)` -- Set state of health
    - soh: 0 .. 100 (%)
  
  - `bool setCellVoltage(int cell, float volt)` -- Set battery cell voltage
    - cell: 1 .. 14 (the original Twizy battery has 14 cells)
    - volt: 1.0 .. 5.0
    - Note: this does no implicit update on the overall pack voltage
  
  - `bool setVoltage(float volt, bool deriveCells)` -- Set battery pack voltage
    - volt: 19.3 … 69.6 (SEVCON G48 series voltage range)
    - deriveCells: true = set all cell voltages to volt/14
  
  - `bool setModuleTemperature(int module, int temp)` -- Set battery module temperature
    - module: 1 .. 7 (the original Twizy battery is organized in 7 modules)
    - temp: -40 .. 100 (°C)
    - Note: this does no implicit update on the overall pack temperature
  
  - `bool setTemperature(int tempMin, int tempMax, bool deriveModules)` -- Set battery pack temperature
    - tempMin: -40 .. 100 (°C)
    - tempMax: -40 .. 100 (°C)
    - deriveModules: true = set all module temperatures to avg(min,max)
  
  - `bool setError(unsigned long error)` -- Set error/warning indicators
    - error: 0x000000 .. 0xFFFFFF (0 = no error) or use a bitwise ORed combination of…

      | Code              | Description                           |
      | ----------------- | ------------------------------------- |
      | `TWIZY_OK`        | Clear all indicators                  |
      | `TWIZY_SERV`      | Set SERV indicator                    |
      | `TWIZY_SERV_12V`  | Set SERV + 12V battery indicator      |
      | `TWIZY_SERV_BATT` | Set SERV + 12V main battery indicator |
      | `TWIZY_SERV_TEMP` | Set SERV + temperature indicator      |
      | `TWIZY_SERV_STOP` | Set SERV + STOP indicator + beep      |



## User callback registration

To hook into the control flow you may register custom functions to be called by the VirtualBMS.

All callbacks are optional. See "Template" example for code templates and example attachment.

  - `void attachEnterState(TwizyEnterStateCallback fn)`
    - fn: `void fn(TwizyState currentState, TwizyState newState)`
    - called on all state transitions after framework handling
    - use this to apply custom settings on state changes

  - `void attachCheckState(TwizyCheckStateCallback fn)`
    - fn: `bool fn(TwizyState currentState, TwizyState newState)`
    - called before transitions from `Init`, `Start…` and `Stop…` into `Ready`, `Driving`, `Charging` and `Trickle`
    - if the callback returns false, the transition will be retried on the next 10 ms ticker run
    - use this to check if the battery/BMS is ready for the state transition

  - `void attachTicker(TwizyTickerCallback fn)`
    - fn: `void fn(unsigned int clockCnt)`
    - called by the 10 ms ticker *after* framework handling, i.e. after sending the Twizy CAN frames
    - clockCnt: cyclic 10 ms interval counter range 0 … 2999
    - use this to add custom CAN sends or do periodic checks, keep in mind this is not called when in state `Off`

  - `void attachProcessCanMsg(TwizyProcessCanMsgCallback fn)`
    - fn: `void fn(unsigned long rxId, byte rxLen, byte *rxBuf)`
    - called on all received (filtered) CAN messages
    - see `setCanFilter()` for setup of additional custom CAN ID filters


## State machine

The VirtualBMS will do state transitions automatically based on CAN input received from the Twizy.

__TwizyStates:__
  - `Off`
  - `Init`
  - `Ready`
  - `StartDrive`
  - `Driving`
  - `StopDrive`
  - `StartCharge`
  - `Charging`
  - `StopCharge`
  - `StartTrickle`
  - `Trickle`
  - `StopTrickle`

Normal startup procedure involves the wakeup phase `Init` followed by `Ready`. Depending on the mode of operation requested, this will be followed by one of the `Start…` states with operation mode established resulting in the according `…ing` state.

The shutdown procedure begins with a transition from the `…ing` state to the according `Stop…` state, followed by `Ready` and finally `Off`.

See [protocol documentation](extras/Protocol.ods) for details.

__API:__
  - `TwizyState state()` -- Query current state
  - `void enterState(TwizyState newState)` -- Force a state change
    - **Note**: this is normally not necessary as the VirtualBMS does all state transitions automatically. Valid exceptions are:
    - You may call `enterState(StopCharge)` to stop a running charge process. Note that `setChargeCurrent(0)` will do so as well.
    - You may call `enterState(Off)` as an emergency measurement to cause the Twizy to stop. You should use `setError()` before and give the user enough time to react, as this may be dangerous depending on the driving situation.

__Callbacks:__
  - Before entering states `Ready`, `Driving`, `Charging` and `Trickle` from any other state, the user callback `CheckState()` will be called.
  - After any state transition, the user callback `EnterState` will be called.


## CAN interface access

To hook into the CAN receiver, use `attachProcessCanMsg()` (see above).

Standard filters will pass IDs `0x423`, `0x597` and `0x599`. Three free CAN filters can be used. The mask is fixed to match the whole ID, so you can filter at most three additional IDs at a time.

  - `void setCanFilter(byte filterNum, unsigned int canId)` -- Set a free ID filter
    - filterNum: 1 … 3
    - canId: 11 bit CAN ID i.e. `0x196`

  - `bool sendMsg(INT32U id, INT8U len, INT8U *buf)` -- Send a CAN message
    - Note: will do three retries if TX buffers are full.
    - Returns true if message has been sent, false on error.
    - Retry and error counts are logged every 10 seconds if debug logging is enabled.


## Debug utils

  - `void dumpId(FLASHSTRING *name, int len, byte *buf)` -- Dump a byte buffer in hex numbers
    - name: must be a PROGMEM string, i.e. `dumpId(F("id123"), ...)`
  
  - `void debugInfo()` -- Dump VirtualBMS status, include frame buffers if debug level >= 2
    - this is automatically called every 10 seconds if debug level >= 1


