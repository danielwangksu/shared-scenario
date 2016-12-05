all: scenario tempSensor tempControl heatActuator alarmActuator webInterface

scenario: scenario.c
	gcc $< -o $@ 

tempSensor: tempSensor.c
	gcc $< -o $@ 

tempControl: tempControl.c
	gcc $< -o $@ 

heatActuator: heatActuator.c
	gcc $< -o $@ 

alarmActuator: alarmActuator.c
	gcc $< -o $@ 

webInterface: webInterface.c
	gcc $< -o $@ 

clean: 
	rm -rf *~ scenario tempSensor tempControl heatActuator alarmActuator webInterface

