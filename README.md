# OpenNode
**By Martins Ierags (c) 2020 [OpenMiniHub](http//www.openminihub.com)**

## Description
The OpenNode is light and simple library for Arduino that handles wireless communication protocol between controller and nodes with attached sensors/actuators.<br/>
Communication between nodes are serialized using [MySensors.org Serial API v2.0](https://www.mysensors.org/download/serial_api_20) and is compatible with 20+ [controllers](https://www.mysensors.org/controller). Wireless part is implemented using RFM69 library by <a href="https://github.com/LowPowerLab">Felix Rusu</a> (LowPowerLab).

## Features
- easy to use API: similar to MySensors
- compatible with MySensors controllers
- more lightweight and easy readable code than MySensors
- simple report interval configuration
- each node sensor can have different report interval during sleep time
- calculated sleep time between report interval for power saving
- node and contact(attached sensor) presenting messages
- message signing with time expiring (including retry)
- 59 (61) bytes max message length (support AES hardware encryption)
- works with RFM69 library by Felix Rusu without modifications inluding ATC & OTA
- easy node including in existing network (by button pressing)
- initially node can be EmptyNode included in network and then you can push firmware you want to use by OTA

## License
- GPL 3.0, please see the [License](https://github.com/openminihub/OpenNode/blob/master/LICENSE) file
- Node comunication protocol was taken from [MySensors.org](https://github.com/mysensors/MySensors)
- OpenNode initially was developed based on [Branly IoT](https://github.com/kanflo/branly-iot) source by Johan Kanflo
- Some ideas/code and inspiration comes from [LowPowerLab](http://LowPowerLab.com/contact) by Felix Rusu
