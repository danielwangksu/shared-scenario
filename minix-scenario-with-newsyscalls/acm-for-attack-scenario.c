#include "ac_realm.h"
// if you want to bypass the access control, change ENABLE_ACM 0 0
#define ENABLE_ACM	0

typedef struct {
  int receiver_acid;
  int nr_operations;
  unsigned short *operation_access_right;
} ac_receiver_entry;

typedef struct {
  int nr_receivers;
  ac_receiver_entry *ac_receivers;
} ac_sender_entry;

#define FIRST_ACID  100
#define NR_AC_PROCESSES 5

#define LAST_ACID (FIRST_ACID + NR_AC_PROCESSES - 1)

#define TEMPSEN_ACID FIRST_ACID
#define TEMPCOT_ACID (FIRST_ACID + 1)
#define HEATACT_ACID (FIRST_ACID + 2) 
#define ALARACT_ACID (FIRST_ACID + 3) 
#define WEBINTF_ACID (FIRST_ACID + 3) 

// #define NR_SERVER_FUNCTIONS 5
// #define function0 0
// #define function1 1
// #define function2 2
// #define function3 3
// #define function4 4


// they should be replaced with bitchunk_t type variables.
unsigned short v1 = 1; // only f0 is allowed (for server return)
unsigned short v2 = 2; // only f1 is allowed (for server return)
unsigned short v4 = 4; // only f2 is allowed (for server return)


ac_receiver_entry tempSensor_entry[1] = {{TEMPCOT_ACID, 1, &v2}};

ac_receiver_entry tempControl_entry[3] =  {
  {HEATACT_ACID, 1, &v2},
  {ALARACT_ACID, 1, &v2},
  {WEBINTF_ACID, 1, &v1},
};

ac_receiver_entry heatActuator_entry[1] = {{TEMPCOT_ACID, 1, &v1}};
ac_receiver_entry alarmActuator_entry[1] = {{TEMPCOT_ACID, 1, &v1}};
ac_receiver_entry webInterface_entry[1] = {{TEMPCOT_ACID, 1, &v4}};

ac_sender_entry access_control_matrix[NR_AC_PROCESSES] = {
  {1, tempSensor_entry},
  {3, tempControl_entry},
  {1, heatActuator_entry},
  {1, alarmActuator_entry},
  {1, webInterface_entry}
};
