# wifi_audio_simple_sample

A simple sample to demo Wi-Fi and UDP/TCP scoket connection for audio through Wi-Fi usage.

# Requirements:

HW: 
- nRF5340 Audio DK x 2
- nRF7002EK x 2
- USB C cable x 2
- Earphones/Headphones with 3.5mm jack

SW: 
- NCS v2.7.0

# Repository Setup

```
git clone https://github.com/charlieshao5189/nordic_wifi_audio_demo.git
cd wifi_audio/src/audio/opus
git submodule update --init
git checkout v1.5.2
```

# Building
The sample has following building options.

## WiFi Station Mode + Static SSID & PASSWORD + UDP
Gateway:

```
west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_static_gateway --sysbuild -- -DSHIELD=nrf7002ek  -DEXTRA_CONF_FILE=overlay-wifi-sta-static.conf
west build    -b nrf5340_audio_dk/nrf5340/cpuapp -d build_static_gateway --sysbuild -- -DSHIELD=nrf7002ek  -DEXTRA_CONF_FILE=overlay-wifi-sta-static.conf 
west flash --erase -d build_static_gateway
```
Headset:
Need to modify the CONFIG_SOCKET_TARGET_ADDR in `overlay-headset.conf` file to set the correct IP address of the gateway.
```
west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_static_headset --sysbuild -- -DSHIELD=nrf7002ek  -DEXTRA_CONF_FILE="overlay-wifi-sta-static.conf;overlay-audio-headset.conf"
west build    -b nrf5340_audio_dk/nrf5340/cpuapp -d build_static_headset --sysbuild -- -DSHIELD=nrf7002ek  -DEXTRA_CONF_FILE="overlay-wifi-sta-static.conf;overlay-audio-headset.conf"
west flash --erase -d build_static_headset

```

## WiFi Station Mode + WiFi CREDENTIALS SHELL(for SSID+Password Input) + UDP (Under development, need to add CONFIG_SOCKET_TARGET_ADDR to notify headset of gateway IP address)

Gateway:

```
west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_gateway --sysbuild -- -DSHIELD=nrf7002ek
west build    -b nrf5340_audio_dk/nrf5340/cpuapp -d build_gateway --sysbuild -- -DSHIELD=nrf7002ek
west flash --erase -d build_gateway
```

Headset:

```
west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_headset --sysbuild -- -DSHIELD=nrf7002ek  -DEXTRA_CONF_FILE=overlay-audio-headset.conf
west build    -b nrf5340_audio_dk/nrf5340/cpuapp -d build_headset --sysbuild -- -DSHIELD=nrf7002ek  -DEXTRA_CONF_FILE=overlay-audio-headset.conf
west flash --erase -d build_headset
```

WiFi CREDENTIALS SHELL example:

```
uart:~$ wifi_cred
wifi_cred - Wi-Fi Credentials commands
Subcommands:
  add           : Add network to storage.
  auto_connect  : Connect to any stored network.
  delete        : Delete network from storage.
  list          : List stored networks.
uart:~$ wifi_cred add wifi_ssid WPA2-PSK wifi_password
uart:~$ wifi_cred auto_connect
```
The device will remember this set of credential and autoconnect to target router after reset.

Use `-DEXTRA_CONF_FILE=overlay-tcp.conf` to switch from UDP socket to TCP socket.(NOTE: TCP socket is not well tested, may not work.)
Use `-DEXTRA_CONF_FILE=overlay-opus.conf` to turn on Opus codec.(NOTE: Opus codec is still under development, will most likely not work due to nF5340 Audio DK memory limitation.)
