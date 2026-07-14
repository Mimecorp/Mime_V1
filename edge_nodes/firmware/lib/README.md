# Patched MPU9250 Library

Because the standard `hideakitai/MPU9250` library fails to initialize when an AK8963 magnetometer is missing (leaving the MPU6500 sensor in sleep mode), you must use your manually patched version of the library.

**Instructions:**
1. Copy the `MPU9250` folder containing your patched library code (usually found in `.pio/libdeps/esp32dev/MPU9250`).
2. Paste that folder directly into this `lib/` directory so the structure looks like this:
   `lib/MPU9250/MPU9250.h`
3. PlatformIO will now automatically compile your patched version for this project on any machine, without needing to download it from the internet.
