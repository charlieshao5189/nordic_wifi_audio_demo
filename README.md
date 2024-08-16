# nordic_wifi_audio_demo
A demo for Wi-Fi Audio based on nRF5340 Audio DK + nRF7002 EK

# Hardware
Recorder: nRF5340 Audio DK + nRF7002 EK

Player: nRF5340 Audio DK + nRF7002 EK / UDP+Opus client on PC and Mobilephone


# Bulding 
VS Code Extension:

![recorder build configuraiton](/doc_resources/build_configuraiton_recorder.png)

Command line:
```
west build -p -b nrf5340_audio_dk/nrf5340/cpuapp --sysbuild -- -DSHIELD=nrf7002ek
```

# To-do-Tasks
1. Audio drivers (Line in/Microphone ADC and Headphone DAC) 
2. Data streaming via UDP/Wi-Fi on Station mode 
3. Port latest opus-codec library

# Useful references

- [nrf52_audio_opus_sgtl5000](https://github.com/ubicore/nrf52_audio_opus_sgtl5000) 
- [NCS nRF5340 Audio applications](https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/applications/nrf5340_audio/index.html) 
- [Wi-Fi BLE Camera Demo](https://github.com/NordicPlayground/nrf70-wifi-ble-image-transfer-demo)
- [Opus Codec](https://opus-codec.org/)
