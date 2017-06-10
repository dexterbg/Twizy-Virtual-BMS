# SEVCON Configuration

You do not need to change the SEVCON configuration to use a custom battery if the custom battery delivers voltage and power matching the standard operating range and your SEVCON battery protection has not been disabled.

- The SEVCON G48 series can operate with input voltages from 19.3 to 69.6 V. The Twizy SEVCON is by default configured for a range of 39 to 65 V.
- In standard configuration, the Twizy will draw up to ~330 A or 16 kW from the battery, and recuperate at up to ~70 A or 3.5 kW.

If your battery cannot cope with these limits, you can either use the Virtual BMS to lower the limits or modify the SEVCON configuration.

If you need to set the limits regularly below the standards, consider tuning the Twizy down. Triggering the power limiter will result in a sudden power cutback followed by slowly raising power levels until the limit is reached again. This continues over and over leading to an inconvenient pumping effect.

**Note**: SEVCON write access has been limited by Renault on Twizys (SEVCONs) delivered after June 2016. The write protection only applies to power and driving profile tuning registers, most other parameters can still be changed, including voltage limits.


## Tools

You can configure the SEVCON using…

1. [Official SEVCON tools](http://www.sevcon.com/products/low-voltage-controllers/gen4/)
2. [OVMS](https://github.com/openvehicles/Open-Vehicle-Monitoring-System)
3. [Twizy-Cfg for Arduino](https://github.com/bgdexter/Twizy-Cfg)
4. Generic CANopen editor

The syntax for reading/writing the SEVCON registers slightly varies depending on the software used, but the registers and values are the same.

| Function | Twizy-Cfg | OVMS |
| --- | --- | --- |
| Enter pre-op mode | `p` | `cfg pre` |
| Enter op mode | `o` | `cfg op` |
| Read register | `r <id> <sub>` | `cfg read <id> <sub>` |
| Write register | `w <id> <sub> <val>` | `cfg write <id> <sub> <value>` |

The following command examples use the Twizy-Cfg syntax.

**Note**: negative values currently need to be read and written with value offset 65536 ("complementary"), i.e. -1 = 65535.


## Battery voltage limits

| Register | Description | Default | Scaling | …= |
| --- | --- | --- | --- | --- |
| `2c01 01` | Over voltage start cutback | 992 | 1/16 | 62 V |
| `2c01 02` | Over voltage limit | 1040 | 1/16 | 65 V |
| `2c02 01` | Under voltage start cutback | 656 | 1/16 | 41 V |
| `2c02 02` | Under voltage limit | 624 | 1/16 | 39 V |
| `2c03 00` | Protection delay | 80 | 1/10 | 8 s |

**Example**: to allow voltages down to 35 V, issue…

1. `w 2c02 01 592`
2. `w 2c02 02 560`

**Keep in mind** that the SEVCON derives current levels from allowed power levels (unless configured otherwise, see below). A lower voltage will result in higher currents, take care to lower the power limits as necessary using the `setPowerLimits()` API call when your voltage drops.


## Battery current & power limits

**Note**: these only apply to the driving state, the charger current can be controlled by `setChargeCurrent()` regardless of the SEVCON configuration.

| Register | Description | Default | Scaling | …= |
| --- | --- | --- | --- | --- |
| `2870 01` | Battery current limit source | 4 | see below | BMS |
| `2870 02` | Max discharge current | 500 | 1 | 500 A |
| `2870 03` | Max recharge current | 65536 | -1 | 200 A |
| `2870 04` | Profile 1 level | 0 | 1 | 0% |
| `2870 05` | Profile 2 level | 0 | 1 | 0% |
| `2870 06` | _Max discharge power (derived)_ | i.e. 4608 | 1/256 | 18 kW |
| `2870 06` | _Max recharge power (derived)_ | i.e. 64896 | -1/256 | 2.5 kW |
| `4623 01` | _Calculated recharge current limit (power/voltage)_ | 65493 | -1 | 43 A |
| `4623 02` | _Calculated discharge current limit (power/voltage)_ | 314 | 1 | 314 A |
| `4623 03` | Battery current cutback range | 15 | 1 | 15 A |
| `3813 30` | Battery current recovery speed | 100 | 100/256 | 39.0625 %/s |
| `3813 30` | Battery current cutback speed | 800 | 1 | 312.5 %/s |

**Battery current limit source**:
  - `4`: BMS controls power limits through `setPowerLimits()`, current levels calculated from power levels
  - `3`: Use max currents scaled down by profile level (`setPowerLimits()` disabled)
  - `1`: Use max currents (`setPowerLimits()` disabled)
  - `0`: Limiter off (`setPowerLimits()` disabled)

These registers need pre-operational mode to allow writes.

**Example**: to disable `setPowerLimits()` and set a fixed maximum of 250 A for discharge (driving) and 50 A for recharging (recuperation), issue…

1. `p`
2. `w 2870 01 1`
3. `w 2870 02 250`
4. `w 2870 03 65486`
5. `o`


## SOC based power cutbacks

| Register | Description | Default | Scaling | …= |
| --- | --- | --- | --- | --- |
| `3813 11` | Drive cutback map: SOC 1 | 0 | 1/50 | 0 % SOC |
| `3813 12` | … cutback 1 | 750 | 1/50 | 15 % power |
| `3813 13` | … SOC 2 | 750 | 1/50 | 15 % SOC |
| `3813 14` | … cutback 2 | 5000 | 1/50 | 100 % power |
| `3813 15` | Brake cutback map: SOC 1 | 0 | 1/50 | 0 % SOC |
| `3813 16` | … cutback 1 | 5000 | 1/50 | 100 % power |
| `3813 17` | … SOC 2 | 500 | 1/50 | 10 % SOC |
| `3813 18` | … cutback 2 | 5000 | 1/50 | 100 % power |

The SEVCON applies these linear cutbacks based on SOC additionally to the BMS power limits allowing for a smooth cutback slope. Default is to cutback drive power below 15% SOC, and leave brake power at BMS control.

These registers need pre-operational mode to allow writes.

**Example**: configure full brake power cutback from 90% to 100% SOC:

1. `p`
2. `w 3813 15 4500`
3. `w 3813 17 5000`
4. `w 3813 18 0`
5. `o`


## Other configuration options

Read the SEVCON Gen4 manual for some basic description. The full register set is documented in the SEVCON master dictionary, which is ©SEVCON. It's contained in the SEVCON DVT package.

Most registers of interest for normal tuning have been documented in the [OVMS Twizy SDO list](https://github.com/openvehicles/Open-Vehicle-Monitoring-System/raw/master/docs/Renault-Twizy/Twizy-SDO-List.ods).

**Have fun!**
