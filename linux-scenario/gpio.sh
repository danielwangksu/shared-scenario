#!/bin/bash
echo "configuring GPIO pins"
for i in {115,15,112,14,44,45}
do
        echo $i > /sys/class/gpio/export
done
echo "setting the GPIO pin direction as out"
for i in {115,15,112,14,44,45}
do
        echo "out" > /sys/class/gpio/gpio$i/direction
done
