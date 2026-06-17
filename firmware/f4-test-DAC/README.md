# STM32 ADC → DAC vs PWM Analog Output Comparison

## Overview

This project demonstrates reading an analog voltage from a potentiometer using the STM32 ADC and reproducing that signal using two different methods:

- Internal DAC output
- PWM output filtered through an RC low-pass filter

The goal is to compare the quality, range, and behavior of both analog reconstruction methods.

---

## Hardware Setup

### Input
- Potentiometer
- Connected to:
  - ADC1 Channel 5 (PA5)

### Outputs
1. DAC Output
   - STM32 internal DAC channel (specify channel if needed)

2. PWM Output
   - Timer: TIM1
   - Channel: 4
   - Pin: PE14

---

## RC Filter (PWM to Analog Conversion)

The PWM signal is converted into an analog voltage using a low-pass RC filter:

- Resistor: 10 kΩ  
- Capacitor: 220 nF  

Cutoff frequency:
\[
f_c = \frac{1}{2\pi RC} \approx 72 \text{ Hz}
\]

This smooths the PWM signal into a stable analog voltage.

---

## Timer Configuration (PWM)

- Timer clock: 16 MHz
- Auto-reload register (ARR): 255
- PWM mode: edge-aligned

This provides:
- 8-bit PWM resolution
- Smooth duty cycle control suitable for analog reconstruction

---

## ADC Configuration

- ADC: ADC1
- Channel: 5
- Input pin: PA5
- Reads potentiometer voltage and scales it for both outputs

---

## Key Results

### DAC Output
- Stable analog output
- Does not reach perfect 0 V or VDD rails
- Small offset at extremes due to internal DAC buffer behavior

### PWM + RC Filter Output
- Smooth analog-like voltage after filtering
- Achieves rail-to-rail output (0 V to VDD)
- Better usable range than DAC in this setup
- Small ripple depends on PWM frequency and filter design

---

## Conclusion

- The STM32 DAC provides simple and low-noise analog output but has limited output range near the rails.
- PWM with RC filtering provides a cost-effective DAC alternative with:
  - Full rail-to-rail output
  - Good linearity
  - Slight ripple depending on filter quality

In this project, the PWM + RC method performed better in terms of output range and practical usability.

---

## Possible Improvements

- Increase PWM frequency to reduce ripple further
- Use a multi-stage or active filter for smoother output
- Disable DAC output buffer to improve rail performance
- Use DMA for continuous ADC sampling
- Compare linearity error between ADC input and both outputs
