all: scenario tempSensor tempControl heatActuator alarmActuator webInterface

scenario: scenario.c
	gcc $< -o $@ -lrt

tempSensor: tempSensor.c
	gcc $< -o $@ -lrt

tempControl: tempControl.c
	gcc $< -o $@ -lrt

heatActuator: heatActuator.c
	gcc $< -o $@ -lrt

alarmActuator: alarmActuator.c
	gcc $< -o $@ -lrt

webInterface: webInterface.c
	gcc $< -o $@ -lrt

clean: 
	rm -rf *~ scenario tempSensor tempControl heatActuator alarmActuator webInterface

