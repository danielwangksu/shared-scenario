# shared-scenario

This repository contains the draft of the temperature control scenario on modified MINIX3.

The current scenario only works on the modified MINIX3 on [DHS MINIX3 Repository](https://github.com/arguslab/dhs-minix3-kernel) commit **_18de0cb_** (before the test version of ACM is included)

To tun the scenario, first compiler each of those files on Beaglebone black with coopresonding names using CC. e.g.

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