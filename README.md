# SimpleWiFiManager

Simplyfied version of the WiFiManager-library (v0.14) by tzapu: https://github.com/tzapu/WiFiManager/

The changes to the original version are:
 - Removed the optional custom parameters
 - Made the library non-blocking


#### This works with the ESP8266 Arduino platform with a recent stable release(2.0.0 or newer) [ESP8266 core for Arduino](https://github.com/esp8266/Arduino)

## Contents
 - [How it works](#how-it-works)
 - [Quick start](#quick-start)
 - [Documentation](#documentation)
 - [Changelog](#changelog)
 - [Contributors](#contributions-and-thanks)


## How It Works
- when your ESP starts up, it sets it up in Station mode and tries to connect to a previously saved Access Point
- if this is unsuccessful (or no previous network saved) it moves the ESP into Access Point mode and spins up a DNS and WebServer (default ip 192.168.4.1)
- using any wifi enabled device with a browser (computer, phone, tablet) connect to the newly created Access Point
- because of the Captive Portal and the DNS server you will either get a 'Join to network' type of popup or get any domain you try to access redirected to the configuration portal
- choose one of the access points scanned, enter password, click save
- ESP will try to connect. If successful, it relinquishes control back to your app. If not, reconnect to AP and reconfigure.

## Quick Start

### Installing
You can install this version by through a checking out the latest changes or a release from github.

####  Checkout from github
- Checkout library to your Arduino libraries folder (Replace {PATH_TO_YOUR_DOCUMENTS} to it's actual value).

```
git clone git@github.com:Murkas94/SimpleWiFiManager.git {PATH_TO_YOUR_DOCUMENTS}/Arduino/libraries/SimpleWifiManger
```

#### Checkout from github as git-submodule:
- Checkout library into a sub-folder of your arduino-project. Needs to be in the folder ```./src/*```, so for example ```./src/SimpleWifiManager```.

```
git submodule add git@github.com:Murkas94/SimpleWiFiManager.git src/SimpleWiFiManager
```

### Using
Please look at the [example](./examples/AutoConnect/AutoConnect.ino).

## Documentation

##### Custom Access Point IP Configuration
This will set your captive portal to a specific IP should you need/want such a feature. Add the following snippet before `autoConnect()`
```cpp
//set custom ip for portal
wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
```

##### Custom Station (client) Static IP Configuration
This will make use the specified IP configuration instead of using DHCP in station mode.
```cpp
wifiManager.setSTAStaticIPConfig(IPAddress(192,168,0,99), IPAddress(192,168,0,1), IPAddress(255,255,255,0));
```
There are a couple of examples in the examples folder that show you how to set a static IP and even how to configure it through the web configuration portal.

#### Custom HTML, CSS, Javascript
There are various ways in which you can inject custom HTML, CSS or Javascript into the configuration portal.
The options are:
- inject custom head element
You can use this to any html bit to the head of the configuration portal. If you add a `<style>` element, bare in mind it overwrites the included css, not replaces.
```cpp
wifiManager.setCustomHeadElement("<style>html{filter: invert(100%); -webkit-filter: invert(100%);}</style>");
```

#### Filter Networks
You can filter networks based on signal quality and show/hide duplicate networks.

- If you would like to filter low signal quality networks you can tell WiFiManager to not show networks below an arbitrary quality %;
```cpp
wifiManager.setMinimumSignalQuality(10);
```
will not show networks under 10% signal quality. If you omit the parameter it defaults to 8%;

- You can also remove or show duplicate networks (default is remove).
Use this function to show (or hide) all networks.
```cpp
wifiManager.setRemoveDuplicateAPs(false);
```

#### Debug
Debug is enabled by default on Serial. To disable add before autoConnect
```cpp
wifiManager.setDebugOutput(false);
```

### Changelog

##### v1.0
First release of the simplyfied library.

### Contributions and thanks
Of course, all the thanks goes to [Alex D - tzapu](https://github.com/tzapu/), who is the original creator of the library. I've used the library for a long time by myself, but needed a simpler and non-blocking version of it.

Thanks also go to all the persons from the community, which helped him to create this library:

__THANK YOU__

[Shawn A](https://github.com/tablatronix)

[Maximiliano Duarte](https://github.com/domonetic)

[alltheblinkythings](https://github.com/alltheblinkythings)

[Niklas Wall](https://github.com/niklaswall)

[Jakub Piasecki](https://github.com/zaporylie)

[Peter Allan](https://github.com/alwynallan)

[John Little](https://github.com/j0hnlittle)

[markaswift](https://github.com/markaswift)

[franklinvv](https://github.com/franklinvv)

[Alberto Ricci Bitti](https://github.com/riccibitti)

[SebiPanther](https://github.com/SebiPanther)

[jonathanendersby](https://github.com/jonathanendersby)

[walthercarsten](https://github.com/walthercarsten)