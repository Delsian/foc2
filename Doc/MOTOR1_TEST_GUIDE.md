# Motor 1 Test Configuration Guide

## Configuration

The system is now configured for **Motor 1 only** testing with encoder feedback:

- **Motor 1**: TIM3 PWM (channels 2, 3, 4)
- **Encoder 1**: MT6701 on I2C1, address 0x06
- **Current sensing**: ADC channels 2, 3 (Motor 1)
- **Motor 0**: Disabled (encoder init commented out)

## Quick Start Test Sequence

### 1. Power On and Check Encoder
```
[System starts]
=== Motor 1 Test Configuration ===
Motor 1 + Encoder 1 on I2C1 enabled
Motor 0 disabled for this test

I2C1 scan results:
  Found device at 0x06  <-- Encoder should be detected
```

If you don't see the encoder, check I2C1 wiring.

### 2. Calibrate Encoder (First Time)
```
> c

=== Motor 1 Encoder Calibration ===
Aligning Motor 1 to electrical zero...
Waiting for rotor alignment (1 second)...
Motor 1 calibrated offset: 123.4 degrees
SUCCESS: Calibration saved!
========================
```

The motor will briefly apply power to align, then release.

### 3. Enable Closed-Loop Control
```
> e

Motor 1 encoder ENABLED (closed-loop control)
Note: Use 'c' to calibrate offset for optimal performance
```

### 4. Test Open-Loop First (Baseline)
```
> p                    # Enter velocity mode
Velocity mode enabled on Motor 1 (amplitude: 5%)

> ++++                 # Increase to 40 RPM
Target velocity: 40 RPM

> >>                   # Increase amplitude to 15%
Amplitude: 15%

> i                    # Check status
=== Motor 1 Control Info ===
Mode: Velocity
Amplitude: 15%
Encoder: DISABLED (open-loop)
Target RPM: 40
Current RPM: 40
Current: 850 mA (limit: 2 A)
Encoder (raw): 234.5 deg
========================
```

**Note the current draw** - this is your open-loop baseline (~850mA in example).

### 5. Enable Encoder and Compare
```
> e                    # Enable encoder feedback

Motor 1 encoder ENABLED (closed-loop control)

> i                    # Check status again
=== Motor 1 Control Info ===
Mode: Velocity
Amplitude: 15%
Encoder: ENABLED (closed-loop)
Target RPM: 40
Current RPM: 41

--- Encoder Data (Closed-Loop) ---
Mechanical angle: 234.5°
Electrical angle: 145.2°
Measured velocity: 41 RPM

Current: 620 mA (limit: 2 A)  <-- Should be 25-35% lower!
========================
```

**Power savings**: 850mA → 620mA = **27% reduction** ✅

### 6. Test Different Speeds
```
> ++++++++             # Increase to 120 RPM
> i                    # Check current

> ++++++++++++++++     # Increase to 260 RPM
> i                    # Check current

> ----------------     # Back to 100 RPM
> i                    # Check current
```

### 7. Toggle to See Difference
```
> e                    # Disable encoder
Motor 1 encoder feedback DISABLED (open-loop control)
> i                    # Note current draw

> e                    # Enable encoder again
Motor 1 encoder ENABLED (closed-loop control)
> i                    # Note lower current
```

## Expected Results

| Condition | Current Draw | Efficiency | Smoothness |
|-----------|-------------|------------|------------|
| Open-loop (no encoder) | 100% (baseline) | 70-80% | Moderate vibration |
| Closed-loop (with encoder) | **65-75%** | **85-95%** | **Very smooth** |

## Commands Reference

### Motor Control
- `+` / `-` : Increase/decrease speed by 10 RPM
- `>` / `<` : Increase/decrease power by 5%
- `p` : Toggle position/velocity mode

### Encoder Control
- `e` : Toggle encoder feedback (open ↔ closed loop)
- `c` : Calibrate encoder offset
- `i` : Print detailed status

## Troubleshooting

### "ERROR: Motor 1 encoder not detected!"
**Check:**
1. Encoder connected to I2C1
2. I2C address is 0x06 (check with I2C scan at startup)
3. Encoder has power (3.3V or 5V)
4. Magnet positioned over encoder IC

### Motor vibrates with encoder enabled
**Solution**: Recalibrate with `c` command

### Current draw not improving
**Check:**
1. Encoder calibration (`c`) was done
2. Encoder reading successfully (check `i` for encoder data)
3. Pole pairs setting correct (default: 7)

### Wrong rotation direction
**Solution**: Modify in code to invert direction:
```c
foc_encoder_config(motor[1], &encoder_motor1, 7, offset1, true);  // Last param = true
```

## Testing SVPWM Benefits

The system uses SVPWM by default. Benefits are already included in the measurements.

**Total optimization stack:**
1. SVPWM modulation: ~15% voltage utilization improvement
2. Encoder feedback: ~15-20% commutation efficiency
3. **Combined**: 25-35% total power reduction

## Monitoring Motor Current

Watch the current values from the `i` command:
```
Current: 620 mA (limit: 2 A)
```

If current approaches limit (2A):
- Decrease amplitude with `<`
- Reduce target RPM with `-`
- Check for mechanical binding

## Data Logging

To log performance data, capture serial output:
```bash
# On host computer
screen /dev/ttyACM0 115200 -L   # -L enables logging
# or
minicom -D /dev/ttyACM0 -b 115200 -C motor_test.log
```

Then run tests and use `i` command periodically to capture data points.

## Success Criteria

✅ **Test Passed** if you see:
1. Encoder detected on I2C1 at startup
2. Calibration completes successfully
3. Encoder data appears in `i` command when enabled
4. **Current draw reduces by 20-35%** when encoder enabled
5. Motor runs smoothly with less vibration

## Example Test Session

```
> i
Encoder: NOT DETECTED      # Before calibration

> c
Motor 1 calibrated offset: 156.7 degrees
SUCCESS!

> e
Motor 1 encoder ENABLED

> p
Velocity mode enabled

> +++
Target velocity: 30 RPM

> >>>
Amplitude: 20%

> i
Current: 780 mA            # Open-loop baseline
Encoder (raw): 156.7 deg

> e
Motor 1 encoder ENABLED

> i
Current: 560 mA            # 28% improvement!
Measured velocity: 31 RPM  # Real feedback
Electrical angle: 234.5°   # Precise commutation
```

## Next Steps

Once Motor 1 testing is complete and working well:
1. Document the power savings achieved
2. If needed, re-enable Motor 0 in the code
3. Configure both motors with their respective encoders
4. Test dual-motor operation

For now, focus on validating the closed-loop performance with Motor 1!
