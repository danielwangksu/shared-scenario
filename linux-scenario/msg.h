/**************************************************************
*	Header file define message format						  *
*	by Daniel Wang											  *
***************************************************************/

enum type_t {SENSOR_UPDATE, HEATER_COMMAND, ALARM_COMMAND, SETPOINT_UPDATE, HEATER_CONFIRM, ALARM_CONFIRM, PID_UPDATE};

typedef struct
{
	enum type_t type;
	int data;
} Msg;