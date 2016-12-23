/**************************************************************
*	Header file define message format						  *
*	by Daniel Wang											  *
***************************************************************/

// enum type_t {SENSOR_UPDATE = 1, HEATER_COMMAND = 2, ALARM_COMMAND = 3, SETPOINT_UPDATE = 4, HEATER_CONFIRM = 5, ALARM_CONFIRM = 6, WEB_CONFIRM = 7};

// support functionality in TempSensor
#define SENSOR_CONFIRM = 0; // not used

// support functionality in TempControl
#define CONTROL_CONFIRM = 0;
#define SENSOR_UPDATE = 1;
#define SETPOINT_UPDATE = 2;

// support functionality in HeaterActuator
#define HEATER_CONFIRM = 0; // not used
#define HEATER_COMMAND = 1;

// support functionality in AlarmActuator
#define ALARM_CONFIRM = 0; // not used
#define ALARM_COMMAND = 1;

// support functionality in WebInterface 
#define WEB_CONFIRM = 0;