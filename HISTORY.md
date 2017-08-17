# History

## Version 1.2.1 (2017-08-17)

- New API calls stateName(), stateName(state)
- No out of bounds error messages in TWIZY_DEBUG_LEVEL 0


## Version 1.2.0 (2017-07-29)

- Ticker increments clockCnt on exit (first hook call with clockCnt=0)
- Protocol startup phase closer to original:
  - id155[0] set to 0xFF on Init
  - Transition from Init to Ready after 100 ms
  - 3MW timing changed to 200 ms
  - id155[3] update coupled to 3MW switch
- Added API functions to query charger infos:
  - int getChargerTemperature();
  - float getDCConverterCurrent();
  - bool isPluggedIn();
  - bool isSwitchedOn();


## Version: 1.1.0 (2017-06-20)

- Added support for Timer2 & Timer3 (see config header)
- Added `setCurrentQA()` API call (native=fast 1/4 A resolution)


## Version: 1.0.0 (2017-06-17)

- Conversion to Arduino library
- Added CAN RX callback `ProcessCanMsg`
- Added `setError()` and error codes
- Using ROM strings to save RAM
- Configurable debug output level
- Added example Template
- Added example SimpleBMS
- Added `setCanFilter()`
- Added API documentation
- Added hardware documentation
- Added SEVCON configuration info
- Added charger configuration info
- Protocol documentation update on error codes
- Added `Error` state for emergency shutdowns
- Ticker callback also called in state `Off`
- Added donation info & donors file
- Added parts images
- Added inState() test functions
- Added Arduino components to parts list
- Added overview text for Blazejs prototype


## Version: 0.2 (2017-06-06)

- 3MW pulse cycle
- Separate states for trickle charging
- CAN TX retries
- Added bmsTicker() callback


## Version: 0.1 (2017-06-04)

- Initial release


