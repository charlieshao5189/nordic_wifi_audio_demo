# wifi_audio_simple_sample

A simple sample to demo Wi-Fi and UDP/TCP scoket connection for audio through Wi-Fi usage.

# Requirements:

HW: nRF5340 Audio DK + nRF7002EK
SW: NCS v2.7.0

# RepositorySetup:
´´´
git clone https://github.com/charlieshao5189/nordic_wifi_audio_demo.git
cd wifi_audio/src/audio/opus
git submodule update --init
git checkout v1.5.2
´´´

# Building
The sample has following building options:



WiFi Station Mode + Static SSID & PASSWORD + UDP
Gateway:

```
west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_sta_static_gateway --sysbuild -- -DSHIELD=nrf7002ek  -DEXTRA_CONF_FILE=overlay-wifi-sta-static.conf
west build  -b nrf5340_audio_dk/nrf5340/cpuapp -d build_sta_static_gateway --sysbuild -- -DSHIELD=nrf7002ek  -DEXTRA_CONF_FILE=overlay-wifi-sta-static.conf 
west flash --erase -d build_sta_static_gateway
```
Headset:
```
west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_sta_static_headset --sysbuild -- -DSHIELD=nrf7002ek  -DEXTRA_CONF_FILE="overlay-wifi-sta-static.conf;overlay-audio-headset.conf"
west build -b nrf5340_audio_dk/nrf5340/cpuapp -d build_sta_static_headset --sysbuild -- -DSHIELD=nrf7002ek  -DEXTRA_CONF_FILE="overlay-wifi-sta-static.conf;overlay-audio-headset.conf"
west flash --erase -d build_sta_static_headset

```

WiFi Station Mode + WiFi CREDENTIALS SHELL(for SSID+Password Input) + UDP

```
west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_sta_static_gateway --sysbuild -- -DSHIELD=nrf7002ek   
west build  -b nrf5340_audio_dk/nrf5340/cpuapp -d build_sta_static_gateway --sysbuild -- -DSHIELD=nrf7002ek   
west flash --erase -d build_sta_static_gateway
```
Headset:
```
west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_sta_static_headset --sysbuild -- -DSHIELD=nrf7002ek  -DEXTRA_CONF_FILE=overlay-audio-headset.conf
west build -b nrf5340_audio_dk/nrf5340/cpuapp -d build_sta_static_headset --sysbuild -- -DSHIELD=nrf7002ek  -DEXTRA_CONF_FILE=overlay-audio-headset.conf
west flash --erase -d build_sta_static_headset


Use `-DEXTRA_CONF_FILE=overlay-tcp.conf` to switch from UDP socket to TCP socket.