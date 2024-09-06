# wifi_audio_simple_sample

A simple sample to demo Wi-Fi and UDP/TCP scoket connection for audio through Wi-Fi usage.

# Requirements:

HW: nRF5340 Audio DK + nRF7002EK
SW: NCS v2.7.0

The sample has following building options:

WiFi Station Mode + WiFi CREDENTIALS SHELL(for SSID+Password Input) + UDP

```
west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_sta --sysbuild -- -DSHIELD=nrf7002ek
```

WiFi Station Mode + Static SSID & PASSWORD + UDP

```
west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_sta_static --sysbuild -- -DSHIELD=nrf7002ek -DEXTRA_CONF_FILE=overlay-sta-static.conf
```

WiFi SoftAP Mode + UDP

```
west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_softap --sysbuild -- -DSHIELD=nrf7002ek -DEXTRA_CONF_FILE=overlay-softap.conf 
```

Use `-DEXTRA_CONF_FILE=overlay-tcp.conf` to switch from UDP socket to TCP socket.
