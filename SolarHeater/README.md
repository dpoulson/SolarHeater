# Arduino code (NodeMCU) for Solar Heater

It will read in 3 values,

TEMP_INT = Internal temperature of building
TEMP_EXT = External temperature
TEMP_TOP = Temperature at top of solar heater

It will have a single value set (stored in EEPROM, configurable via MQTT)

TEMP_SET = Set point

It will use a PID loop of TEMP_INT, TEMP_SET, and RELAY) to control a relay output and try to get
the TEMP_INT to as close as possible to the TEMP_SET by turning RELAY on and off.

Due to the NodeMCU only having a single ADC, the temperature sensors are multiplexed via GPIO pins

