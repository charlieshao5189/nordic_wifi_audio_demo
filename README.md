# wifi_audio_simple_sample

A simple sample to demo Wi-Fi and UDP/TCP scoket connection for audio through Wi-Fi usage.

# Requirements:

HW: 
- nRF5340 Audio DK x 2
- nRF7002EK x 2
- USB C cable x 2
- Earphones/Headphones with 3.5mm jack

SW: 
- NCS v2.8.0
- Opus v1.5.2
# wifi_audio_simple_sample

A simple sample to demo Wi-Fi and UDP/TCP socket connection for audio through Wi-Fi usage.

# Requirements:

HW: 
- nRF5340 Audio DK x 2
- nRF7002EK x 2
- USB C cable x 2
- Earphones/Headphones with 3.5mm jack

SW: 
- NCS v2.8.0
- Opus v1.5.2

# Repository Setup

```bash
git clone https://github.com/charlieshao5189/nordic_wifi_audio_demo.git
cd wifi_audio/src/audio/opus
git submodule update --init
git checkout v1.5.2
```

# Building
The sample has following building options.

## WiFi Station Mode + WiFi CREDENTIALS SHELL(for SSID+Password Input) + UDP 

Gateway:

```bash
west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_gateway --sysbuild -- -DSHIELD="nrf7002ek"
west build    -b nrf5340_audio_dk/nrf5340/cpuapp -d build_gateway --sysbuild -- -DSHIELD="nrf7002ek"
west flash --erase -d build_gateway
```

Headset:

```bash
west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_headset --sysbuild -- -DSHIELD="nrf7002ek"  -DEXTRA_CONF_FILE="overlay-audio-headset.conf"
west build    -b nrf5340_audio_dk/nrf5340/cpuapp -d build_headset --sysbuild -- -DSHIELD="nrf7002ek"  -DEXTRA_CONF_FILE="overlay-audio-headset.conf"
west flash --erase -d build_headset
```

## WiFi Station Mode + Static SSID & PASSWORD + UDP
Gateway:

```bash
west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_static_gateway --sysbuild -- -DSHIELD="nrf7002ek"  -DEXTRA_CONF_FILE=overlay-wifi-sta-static.conf
west build    -b nrf5340_audio_dk/nrf5340/cpuapp -d build_static_gateway --sysbuild -- -DSHIELD="nrf7002ek"  -DEXTRA_CONF_FILE=overlay-wifi-sta-static.conf
west flash --erase -d build_static_gateway

west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_static_opus_gateway --sysbuild -- -DSHIELD="nrf7002ek"  -DEXTRA_CONF_FILE="overlay-wifi-sta-static.conf;overlay-opus.conf"
west build    -b nrf5340_audio_dk/nrf5340/cpuapp -d build_static_opus_gateway --sysbuild -- -DSHIELD="nrf7002ek"  -DEXTRA_CONF_FILE="overlay-wifi-sta-static.conf;overlay-opus.conf"
west flash --erase -d build_static_opus_gateway
```
Headset:

```bash
west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_static_headset --sysbuild -- -DSHIELD="nrf7002ek"  -DEXTRA_CONF_FILE="overlay-wifi-sta-static.conf;overlay-audio-headset.conf"
west build    -b nrf5340_audio_dk/nrf5340/cpuapp -d build_static_headset --sysbuild -- -DSHIELD="nrf7002ek"  -DEXTRA_CONF_FILE="overlay-wifi-sta-static.conf;overlay-audio-headset.conf"
west flash --erase -d build_static_headset

west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_static_opus_headset --sysbuild -- -DSHIELD="nrf7002ek"  -DEXTRA_CONF_FILE="overlay-wifi-sta-static.conf;overlay-audio-headset.conf;overlay-opus.conf"
west build    -b nrf5340_audio_dk/nrf5340/cpuapp -d build_static_opus_headset --sysbuild -- -DSHIELD="nrf7002ek"  -DEXTRA_CONF_FILE="overlay-wifi-sta-static.conf;overlay-audio-headset.conf;overlay-opus.conf"
west flash --erase -d build_static_opus_headset
```

- Use `-DEXTRA_CONF_FILE=overlay-tcp.conf` to switch from UDP socket to TCP socket.
- Use `-DEXTRA_CONF_FILE=overlay-opus.conf` to turn on Opus codec.

Example to build audio gateway/headset with both Opus and TCP socket enabled:
```bash
west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_static_opus_tcp_gateway --sysbuild -- -DSHIELD="nrf7002ek"  -DEXTRA_CONF_FILE="overlay-wifi-sta-static.conf;overlay-opus.conf;overlay-tcp.conf"
west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_static_opus_tcp_headset --sysbuild -- -DSHIELD="nrf7002ek"  -DEXTRA_CONF_FILE="overlay-wifi-sta-static.conf;overlay-audio-headset.conf;overlay-opus.conf;overlay-tcp.conf"
```

# Running Guide
WiFi CREDENTIALS SHELL example:

```bash
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

The headset device works as socket client, need to know socket server address on audio gateway device before build socket connection. The command to set socket server address on headset device is  `socket set_target_addr <ip_address>:<port>`, for example `socket set_target_addr 192.168.50.10:60010`. The server address can be found in the terminal log of gateway device.

After socket connection is established. Make sure your host pc choose nRF5340 USB Audio(audio gateway) as audio output device, then you can press play/pause on headset device to start/stop audio streaming. The VOL+/- buttons can be used to adjust volume.

# Tips:
1. Comment "check_set_compiler_property(APPEND PROPERTY warning_base -Wdouble-promotion)" in zephyr/cmake/compiler/gcc/compiler_flags.cmake can help to disable -Wdouble-promotion warnning in opus library.
2. Command to debug hard fault: /opt/nordic/ncs/toolchains/f8037e9b83/opt/zephyr-sdk/arm-zephyr-eabi/bin/arm-zephyr-eabi-addr2line -e /opt/nordic/ncs/myapps/nordic_wifi_audio_demo/wifi_audio/build_static_headset/wifi_audio/zephyr/zephyr.elf 0x00094eae
# Repository Setup

```
git clone https://github.com/charlieshao5189/nordic_wifi_audio_demo.git
cd wifi_audio/src/audio/opus
git submodule update --init
git checkout v1.5.2
```

# Building
The sample has following building options.

## WiFi Station Mode + WiFi CREDENTIALS SHELL(for SSID+Password Input) + UDP 

Gateway:

```
west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_gateway --sysbuild -- -DSHIELD="nrf7002ek"
west build    -b nrf5340_audio_dk/nrf5340/cpuapp -d build_gateway --sysbuild -- -DSHIELD="nrf7002ek"
west flash --erase -d build_gateway
```

Headset:

```
west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_headset --sysbuild -- -DSHIELD="nrf7002ek"  -DEXTRA_CONF_FILE="overlay-audio-headset.conf"
west build    -b nrf5340_audio_dk/nrf5340/cpuapp -d build_headset --sysbuild -- -DSHIELD="nrf7002ek"  -DEXTRA_CONF_FILE="overlay-audio-headset.conf"
west flash --erase -d build_headset
```

## WiFi Station Mode + Static SSID & PASSWORD + UDP
Gateway:

```
west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_static_gateway --sysbuild -- -DSHIELD="nrf7002ek"  -DEXTRA_CONF_FILE=overlay-wifi-sta-static.conf
west build    -b nrf5340_audio_dk/nrf5340/cpuapp -d build_static_gateway --sysbuild -- -DSHIELD="nrf7002ek"  -DEXTRA_CONF_FILE=overlay-wifi-sta-static.conf
west flash --erase -d build_static_gateway

west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_static_opus_gateway --sysbuild -- -DSHIELD="nrf7002ek"  -DEXTRA_CONF_FILE="overlay-wifi-sta-static.conf;overlay-opus.conf"
west build    -b nrf5340_audio_dk/nrf5340/cpuapp -d build_static_opus_gateway --sysbuild -- -DSHIELD="nrf7002ek"  -DEXTRA_CONF_FILE="overlay-wifi-sta-static.conf;overlay-opus.conf"
west flash --erase -d build_static_opus_gateway
```
Headset:

```
west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_static_headset --sysbuild -- -DSHIELD="nrf7002ek"  -DEXTRA_CONF_FILE="overlay-wifi-sta-static.conf;overlay-audio-headset.conf"
west build    -b nrf5340_audio_dk/nrf5340/cpuapp -d build_static_headset --sysbuild -- -DSHIELD="nrf7002ek"  -DEXTRA_CONF_FILE="overlay-wifi-sta-static.conf;overlay-audio-headset.conf"
west flash --erase -d build_static_headset

west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_static_opus_headset --sysbuild -- -DSHIELD="nrf7002ek"  -DEXTRA_CONF_FILE="overlay-wifi-sta-static.conf;overlay-audio-headset.conf;overlay-opus.conf"
west build    -b nrf5340_audio_dk/nrf5340/cpuapp -d build_static_opus_headset --sysbuild -- -DSHIELD="nrf7002ek"  -DEXTRA_CONF_FILE="overlay-wifi-sta-static.conf;overlay-audio-headset.conf;overlay-opus.conf"
west flash --erase -d build_static_opus_headset
```

- Use `-DEXTRA_CONF_FILE=overlay-tcp.conf` to switch from UDP socket to TCP socket.
- Use `-DEXTRA_CONF_FILE=overlay-opus.conf` to turn on Opus codec.

Example to build auido gateway/headset with both Opus and TCP socket enabled:
```
west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_static_opus_tcp_gateway --sysbuild -- -DSHIELD="nrf7002ek"  -DEXTRA_CONF_FILE="overlay-wifi-sta-static.conf;overlay-opus.conf;overlay-tcp.conf"
west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_static_opus_tcp_headset --sysbuild -- -DSHIELD="nrf7002ek"  -DEXTRA_CONF_FILE="overlay-wifi-sta-static.conf;overlay-audio-headset.conf;overlay-opus.conf;overlay-tcp.conf"
```

# Running Guide
WiFi CREDENTIALS SHELL example:

```
uart:~$ wifi_cred
wifi_cred - Wi-Fi Credentials commands
Subcommands:
  add           : Add network to storage.
                  <-s --ssid "<SSID>">: SSID.
                  [-c --channel]: Channel that needs to be scanned for
                  connection. 0:any channel.
                  [-b, --band] 0: any band (2:2.4GHz, 5:5GHz, 6:6GHz]
                  [-p, --passphrase]: Passphrase (valid only for secure SSIDs)
                  [-k, --key-mgmt]: Key Management type (valid only for secure
                  SSIDs)
                  0:None, 1:WPA2-PSK, 2:WPA2-PSK-256, 3:SAE-HNP, 4:SAE-H2E,
                  5:SAE-AUTO, 6:WAPI, 7:EAP-TLS, 8:WEP, 9: WPA-PSK, 10:
                  WPA-Auto-Personal, 11: DPP
                  [-w, --ieee-80211w]: MFP (optional: needs security type to be
                  specified)
                  : 0:Disable, 1:Optional, 2:Required.
                  [-m, --bssid]: MAC address of the AP (BSSID).
                  [-t, --timeout]: Timeout for the connection attempt (in
                  seconds).
                  [-a, --identity]: Identity for enterprise mode.
                  [-K, --key-passwd]: Private key passwd for enterprise mode.
                  [-h, --help]: Print out the help for the connect command.

  auto_connect  : Connect to any stored network.
  delete        : Delete network from storage.
  list          : List stored networks.
uart:~$ wifi_cred add -s wifi_ssid -p wifi_password -k 1
uart:~$ wifi_cred auto_connect
```
The device will remember this set of credential and autoconnect to target router after reset.

The headset device works as socket client, need to know socket server address on audio gateway device before build socket connection. The command to set socket server address on headset device is  `socket set_target_addr <ip_address>:<port>`, for example `socket set_target_addr 192.168.50.10:60010`. The server address can be found in the terminal log of gateway device.

After socket connection is established. Make sure your host pc choose nRF5340 USB Audio(audio gateway) as audio output device, then you can press play/pause on headset device to start/stop audio streaming. The VOL+/- buttons can be used to adjust volume.


# Tips:
1. Comment "check_set_compiler_property(APPEND PROPERTY warning_base -Wdouble-promotion)" in zephyr/cmake/compiler/gcc/compiler_flags.cmake can help to disable -Wdouble-promotion warnning in opus library.
2. Command to debug hard fault:
        - /opt/nordic/ncs/toolchains/f8037e9b83/opt/zephyr-sdk/arm-zephyr-eabi/bin/arm-zephyr-eabi-addr2line -e /opt/nordic/ncs/myapps/nordic_wifi_audio_demo/wifi_audio/build_static_headset/wifi_audio/zephyr/zephyr.elf 0x00094eae
        - C:\nordic\toolchains\2d382dcd92\opt\zephyr-sdk\arm-zephyr-eabi\bin\arm-zephyr-eabi-addr2line.exe -e C:\nordic\myApps\nordic_wifi_audio_demo\wifi_audio\build_static_gateway\wifi_audio\zephyr\zephyr.elf 0x0002fdfb
