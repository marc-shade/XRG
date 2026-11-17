# MacBook and cooling-pad overview

The macOS Temperature panel can show the hottest Mac die reading, measured
internal fan RPM, and the Razer cooling pad's acknowledged fan command together.
Temperature labels follow XRG's Fahrenheit/Celsius preference. The validated
installation uses Fahrenheit.

Install and run the automatic service from
[Mac Cooling Control](https://github.com/marc-shade/mac-cooling-control).
When `~/Library/Caches/padctl/status.json` exists, XRG automatically shows the
cooling overview. Right-click the Temperature panel and toggle **Show Mac and
Cooling Pad Overview** to return to the existing sensor selections.

- **Mac die** is the hottest `PMU tdie` or `MTR Temp Sensor` value. These names
  do not establish a CPU/GPU core map. Duplicate sensor names retain the hottest
  reading instead of whichever service was enumerated last.
- **Mac fan 1/2** are real SMC actual-speed readings, ordered by sensor key.
- **Pad cmd** is an acknowledged command from the controller. It is not measured
  RPM. Disconnected, stopped and stale feeds replace the number with a status.
- **Pad temp** shows `Unavailable`; no pad-temperature reading has been validated.

The overview includes Mac die-temperature history. The collapsed mini view shows
the die value; expand it for all rows. The context menu and tooltip describe pad
status and the difference between commanded and measured RPM.

XRG does not open the pad or write hardware commands. The JSON reader checks
schema, field types, valid RPM range and freshness, caps file size at 64 KiB, and
clears old command values on errors. Mac sensors continue updating if the pad
controller stops. The monitor reads and does not modify internal Mac fan control.

## Build and validation

On the validation machine, the installed Xcode SDK required a deployment target
of at least macOS 12. The following build exited 0 with `BUILD SUCCEEDED`:

```sh
DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer xcodebuild \
  -project XRG.xcodeproj -scheme XRG -configuration Release \
  -derivedDataPath "$HOME/Library/Caches/XRG-cooling-build" \
  MACOSX_DEPLOYMENT_TARGET=12.0 CODE_SIGN_IDENTITY= CODE_SIGNING_REQUIRED=NO build
```

Build output included SDK/deprecation warnings. The app was locally ad-hoc
signed and installed, and its executable hash matched the built artifact.
The app was relaunched and the native view was visually inspected.
One observed render showed 137.9°F, internal fans at 1675/1801 RPM, and a
1600 RPM pad command. These are samples, not performance comparisons.

Compile and run the live integration check with the pad service active:

```sh
xcrun clang -fobjc-arc -framework Cocoa -framework IOKit \
  -I 'Data Miners/APSL' -I 'Data Miners' \
  Tools/verify_cooling.m 'Data Miners/APSL/SMCInterface.m' \
  'Data Miners/APSL/XRGAppleSiliconSensorMiner.m' \
  'Data Miners/XRGCoolingPadStatus.m' \
  -o "$HOME/Library/Caches/verify-xrg-cooling"
"$HOME/Library/Caches/verify-xrg-cooling"
```

This gate exited 0 using real Mac sensors and the live pad status file. It also
rejected six deliberately malformed/stale inputs without modifying the live
file. A separate SIGSTOP/SIGCONT test of the controller verified that XRG showed
`Stale` after 15 seconds, then recovered when publication resumed.

## Gaps / not covered

No pad tachometer, pad-temperature sensor, calibrated noise or cooling-performance
benefit was measured. No XRG fan-control buttons are included. Linux, Intel Macs,
other pads/firmware, sleep/wake and reboot/login were not tested. The build is
locally ad-hoc signed, not notarized. Existing unrelated XRG modules were not
regression-tested beyond observing the application running after relaunch.
