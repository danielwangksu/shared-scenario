# shared-scenario

This repository contains the draft of the temperature control scenario on Linux and modified MINIX3.

There are current three versions:

* Scenario for Linux
* Scenario for MINIX3 (messy protopype for first version of modified MINIX3)
* Scenario for MINIX3 (cleaner version with new syscalls)
	
	
## Linux Version

The linux verion is under the **/linux-scenario** folder, can be easily compiled using the **Makefile**.

## MINIX3 Messy Version

The messy verison is moved into **/messy-minix-scenario** folder.

The current messy scenario only works on the first modified MINIX3 kernel host in the repository [arguslab/dhs-minix3-kernel](https://github.com/arguslab/dhs-minix3-kernel) with the MINIX3 OS commit **_18de0cb_** (before the test version of ACM is included)

To run the scenario, first compiler each of those files on Beaglebone black with coopresonding names using CC. e.g.

```bash
cc scenario.c -o scenario

cc alarmActuator.c -o alarmActuator

cc heatActuator.c -o heatActuator

cc tempSensor.c -o tempSensor

cc tempControl.c -o tempControl

cc webInterface.c -o webInterface
```

And then run the program using:

```
./scenario
```

When Beaglebone black is connected on the same network, the web interface can be visited on port 10000

```
http://[IP]:10000/index.html
```

Sorry that the code is messy, a much cleaner version is available but only work with the newly added experimental system calls, which will be merged into the DHS MINIX3 Repository, after ACM is properly implemented.

## MINIX3 Clean Version

The clean version is stored in **/minix-scenario-with-newsyscalls** folder.


This vesion of scenario only works on the modified MINIX3 host in the stable DHS MINIX3 repository [arguslab/dhs-minix3](https://github.com/arguslab/dhs-minix3).

To run the scenario, first compiler each of those files on Beaglebone black with coopresonding names using CC. e.g.

```bash
cc scenario.c -o scenario

cc alarmActuator.c -o alarmActuator

cc heatActuator.c -o heatActuator

cc tempSensor.c -o tempSensor

cc tempControl.c -o tempControl

cc webInterface.c -o webInterface
```

And then run the program using:

```
./scenario
```

When Beaglebone black is connected on the same network, the web interface can be visited on port 10000

```
http://[IP]:10000/index.html
```
