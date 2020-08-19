# nibeanalytics

Project for data collection, analytics and optimization for Nibe Fighter heat pumps. 

Initial ideas with the project are:

  1. Get graphs and visualization for your heat pump without paying +1K Euros for an RCU-11 unit from Nibe.
  2. Optimize the heat pump oeration by the use of machine learning. This as the Nibe heat curve functionality is static and  basic in comparison with a trained machine learning scenario. Goal is decrease power consumption by 5% with same comfort. 
  
Can be used as a replacement for the Nibe RCU-10 or RCU-11 commercial modules.  

The project should fairly work with the listed pumps below (list is based on pumps compatible with RCU-11/10):

- Nibe Fighter 360p (v) 
- Nibe Fighter 113x (v)
- Nibe Fighter 1150 (v)
- Nibe Fighter 123x
- Nibe Fighter 1250
- Nibe Fighter 1320
- Nibe Fighter 1330

(v = verified - Please add your model to the list of verified heat pumps)


## Current functionality: 

  - Data collection over RS485 serial bus
  - Posting of data to HTTP REST service over Wifi using ESPN8266
  - Currently configured for logstash HTTP Input module (https://www.elastic.co/guide/en/logstash/current/plugins-inputs-http.html)


## Roadmap:

  - Elasticsearch Dashboard for visualization (in progress).
  - Usage of AI / Machine learning for heat pump optimization and  management.
  - Figure out and implement the Nibe Command structure so heat pump can be managed prgrammatically.
   

## Requirements:

 - Arduino ATmega2560 (or any Arduino compatible board with support for two or more programmable serial ports).
 - ESP2866 Wifi shield for Arduino
 - WifiEsp Software library for ESP8266 - https://github.com/bportaluri/WiFiEsp
 - RS485 shield for Arduino
 
 - A compatibel Nibe Fighter heat pump running Nibe Software version 2.0 or above (required for RCU-10/11 functionality)
 - A webservice collecting the Nibe data messages (logstash, hadoop, nodejs etc, etc)
 
## Arduino Nibe Logger

 ** WARNING ! Before connecting the logger, the Nibe heat pump and the Arduino logger MUST have a common ground connected. Otherwise the Nibe CPU PCB is in danger (expensive).  

 ![Arduino Nibe Logger](images/00000-arduinoNibeLogger.jpg)

#### Connection scheme Nibe RJ45 -> TP Cable -> RS485 -> Arduino 

 The connection to the heat pump is done using a standard RJ-45 TP Ethernet cable (for pinning se table below). Connect the cable to any of the Nibe Serial Bus end-points. For Nibe 1130 thee are two serial bus end-points at the Display Unit (DPU) and at the main PCB (pictures below).  
 
| Nibe RJ45 | RS485 | Arduino |
|-----------|-------|---------|
|   1  |   A   |         |
|   2  |   B   |         |
|  7,8 |       |   GND   |
|      |   DI  |   18    |
|      |   DE  |    9    |
|      |   RE  |    8    |
|      |   RO  |   19    |
|      |   GND |   GND   |
|      |   VCC |   5V    |


 
 ![Arduino RS485](images/00000-RS485.jpg)

 ![Nibe DPU RJ-45 connector](images/00000-NibeDPU.jpg)
 
 ![Nibe PCB RJ-45 connector](images/00000-NibePCB.jpg)
 




 
