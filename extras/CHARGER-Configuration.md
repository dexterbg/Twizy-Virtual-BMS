# Charger Configuration

You do not need to change the charger if your battery can be charged at the standard voltage and current levels provided by the builtin [IES-Synergy Elips 2000W](http://www.ies-synergy.com/en/chargers/onboard-sealed-chargers/elips-2000w) charger.

- The Elips 2000W can charge with a final voltage of up to 60 V.
- Charge current can be set from 0 - 35 A in 5 A steps.

It may be possible to configure the Elips in the future, at the moment you need to control the charge current and stop the charge process using the `setChargeCurrent()` [API call](../API.md), if your battery cannot cope with the Elips standard charge process.


## Using a custom charger

If you need or want to use a custom charger, take care not to remove the Elips 2000W from the Twizy without further consideration.

The Elips is operating with a specialized firmware and has a crucial role in the Twizy's control protocol.

You can charge a custom battery directly without using the Elips, but for driving and 12V trickle charging the Elips needs to be connected to the battery and CAN bus.

If you're interested in replacing the Elips: creating a "Twizy Virtual Charger" should be possible as well, but has to be done yet.


