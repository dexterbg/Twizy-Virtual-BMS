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
  
  - `bool setCurrentQA(long quarterAmps)` -- Set momentary battery pack current level
    - quarterAmps: -2000 .. +2000 (1/4 A, positive = charge, negative = discharge)
    - This is the native BMS current resolution, so no float/division necessary
  
  - `bool setSOC(float soc)` -- Set state of charge
    - soc: 0.00 .. 100.00 (%)
    - Note: the charger will not start charging at SOC=100%
  
  - `bool setPowerLimits(unsigned int drive, unsigned int recup)` -- Set SEVCON power limits
    - drive: 0 .. 30000 (W)
    - recup: 0 .. 30000 (W)
    - Note: both limits have a resolution of 500 W and will be rounded downwards
    - Note: these limits do not apply if the [SEVCON has been configured](extras/SEVCON-Configuration.md) to ignore them
  
  - `bool setSOH(int soh)` -- Set state of health
    - soh: 0 .. 100 (%)
  
  - `bool setCellVoltage(int cell, float volt)` -- Set battery cell voltage
    - cell: 1 .. 16 (the original Twizy battery has 14 cells)
    - volt: 1.0 .. 5.0
    - Note: this does no implicit update on the overall pack voltage
    - Note: cell voltages #15 & #16 will be stored in frame 0x700 (custom protocol extension)
  
  - `bool setVoltage(float volt, bool deriveCells)` -- Set battery pack voltage
    - volt: 19.3 .. 69.6 (SEVCON G48 series voltage range)
    - deriveCells: true = set cell voltages #1-#14 to volt/14
  
  - `bool setModuleTemperature(int module, int temp)` -- Set battery module temperature
    - module: 1 .. 8 (the original Twizy battery is organized in 7 modules)
    - temp: -40 .. 100 (°C)
    - Note: this does no implicit update on the overall pack temperature
    - Note: module temperature #8 will be stored in frame 0x554 byte 7, which is unused on the original pack layout
  
  - `bool setTemperature(int tempMin, int tempMax, bool deriveModules)` -- Set battery pack temperature
    - tempMin: -40 .. 100 (°C)
    - tempMax: -40 .. 100 (°C)
    - deriveModules: true = set module temperatures #1-#7 to avg(min,max)
  
  - `bool setError(unsigned long error)` -- Set display error/warning indicators
    - error: 0x000000 .. 0xFFFFFF (0 = no error) or use a bitwise ORed combination of…

      | Code              | Description                           |
      | ----------------- | ------------------------------------- |
      | `TWIZY_OK`        | Clear all indicators                  |
      | `TWIZY_SERV`      | Set SERV indicator                    |
      | `TWIZY_SERV_12V`  | Set SERV + 12V battery indicator      |
      | `TWIZY_SERV_BATT` | Set SERV + 12V main battery indicator |
      | `TWIZY_SERV_TEMP` | Set SERV + temperature indicator      |
      | `TWIZY_SERV_STOP` | Set SERV + STOP indicator + beep      |


## Info access

  - `int getChargerTemperature()` -- Get internal charger temperature
    - result: -40 .. 100 (°C, resolution 1 °C)
    - Hint: use this to protect the charger against over temperature by reducing the charge current

  - `float getDCConverterCurrent()` -- Get DC/DC converter current level
    - result: 0 .. 51 (A, resolution 1/5 A)

  - `bool isPluggedIn()` -- Get power grid connection status
    - result: true = 230V detected

  - `bool isSwitchedOn()` -- Get key switch status
    - result: true = key turned


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
    - clockCnt: cyclic 10 ms interval counter range 0 … 2999 (reset to 0 on `Off`/`Init`)
    - use this to add custom CAN sends or do periodic checks

  - `void attachProcessCanMsg(TwizyProcessCanMsgCallback fn)`
    - fn: `void fn(unsigned long rxId, byte rxLen, byte *rxBuf)`
    - called on all received (filtered) CAN messages
    - see `setCanFilter()` for setup of additional custom CAN ID filters


## State machine

The VirtualBMS will do state transitions automatically based on CAN input received from the Twizy.

__TwizyStates:__
  - `Off`
  - `Init`
  - `Error`
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

Use the `Error` state to signal **severe problems** to the Twizy and cause an emergency shutdown. `Error` turns off CAN sends and drops the 3MW (ECU_OK) signal. SEVCON and charger will switch off all battery power immediately. To resolve the `Error` state from driving, the user needs to do a power cycle. When used during a charge, the charger will send the BMS into `Off` state, so the charge can simply be restarted by replugging the charger.

**Note**: to indicate **non-critical** problems, do not enter the `Error` state but instead only use `setError()`.

See [protocol documentation](extras/Protocol.ods) for details.

__API:__
  - `TwizyState state()` -- Query current state
  - `FLASHSTRING *stateName([state])` -- Query name of current state / specific state
  - `bool inState(TwizyState state1 [, … state5])` -- Test for 1-5 states
  - `void enterState(TwizyState newState)` -- Force a state change
    - Call `enterState(StopCharge)` to stop a running charge process. Note that `setChargeCurrent(0)` will do so as well.
    - Call `enterState(Error)` to cause an emergency shutdown. Before doing this you should use `setError()` and (if possible) give the user some time to react when in state `Driving`.

__Callbacks:__
  - Before entering states `Ready`, `Driving`, `Charging` and `Trickle` from any other state, the user callback `CheckState()` will be called.
    Exception is entering `Ready` from `Error`, this needs to be done manually anyway (don't expect SEVCON or charger to follow though).
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


## Extended info frame

Beginning with version 1.3.0, the VirtualBMS supports sending an extended BMS status information on the CAN bus. The frame layout has been designed by Pascal Ripp and Michael Balzer to create a standard base for CAN bus tools like the Twizplay and the OVMS.

The extended info frame is sent at CAN ID 0x700 once per second in all states except `Off`. The frame transports these fields:

  - Byte 0: BMS specific state #1 (main state, i.e. twizy.state())
  - Byte 1: highest 3 bits = BMS type ID (see below), remaining 5 bits = BMS specific error code (see below)
  - Bytes 2-4: cell voltages #15 & #16 (encoded in 12 bits like #1-#14)
  - Bytes 5-6: balancing status (bits 15…0 = cells 16…1, 1 = balancing active)
  - Byte 7: BMS specific state #2 (auxiliary state or data)

The VirtualBMS will not send frame 0x700 unless you set the BMS type. It will also not insert any data into the fields by itself, to fill in data you need to use the API calls as shown below. This way you can use the VirtualBMS library to implement other BMS types as well.


### BMS types

The BMS type is meant for CAN tools to be able to identify the BMS and decode the BMS specific states and error info.

There are currently 7 possible BMS types, defined in `enum TwizyBmsType`:

  - 0 = `bmsType_VirtualBMS` (states and error codes as documented here)
  - 1 = `bmsType_EdriverBMS` (see Pascal's documentation for details)
  - 2…6 = reserved
  - 7 = `bmsType_undefined` (disables frame 0x700)

Types #2…#6 can be assigned to future BMS types.

Please contact us if you want to allocate an ID. Keep in mind: any new BMS type needs support in all CAN tools, so try to reuse one of the already defined BMS types as long as possible.


### BMS error codes (for `bmsType_VirtualBMS`)

Our basic standard proposition covers these error codes (defined in `enum TwizyBmsError`):

  - 0 = `bmsError_None`
  - 1 = `bmsError_EEPROM`
  - 2 = `bmsError_SensorFailure`
  - 3 = `bmsError_VoltageHigh`
  - 4 = `bmsError_VoltageLow`
  - 5 = `bmsError_VoltageDiff`
  - 6 = `bmsError_TemperatureHigh`
  - 7 = `bmsError_TemperatureLow`
  - 8 = `bmsError_TemperatureDiff`
  - 9 = `bmsError_ChargerTemperatureHigh`

Custom error codes can be added beginning at 128. Please contact us if you'd like to add standard error codes.


### API calls

  - `bool setInfoBmsType(byte bmsType)` -- Set informational BMS type
    - bmsType: 0 .. 7 (see above / enum TwizyBmsType)
    - Note: type 7 (bmsType_undefined = default value) deactivates frame 0x700

  - `bool setInfoState1(byte state)` -- Set informational state 1 (main state)
    - state: 0x00 .. 0xFF (specific by BMS type)

  - `bool setInfoState2(byte state)` -- Set informational state 2 (aux state)
    - state: 0x00 .. 0xFF (specific by BMS type)

  - `bool setInfoError(byte errorCode)` -- Set informational error code
    - errorCode: 0x00 .. 0x1F (specific by BMS type)

  - `bool setInfoBalancing(unsigned int flags)` -- Set informational balancing status
    - flags: 16 bits = 16 cells (#16 = MSB, #1 = LSB), 1 = balancing


