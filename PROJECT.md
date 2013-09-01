# <center>The Satellites</center>

<center>Group 4</center>
<center>Yimeng Wang 20358434 y386wang</center>
<center>Xuan Zhao 20341186 x42zhao</center>

## Overview

This project use up to three trains to simulate satellites that can move in three different orbits. The project has been divid into different level of goals. 

* Primary goal: Controlling the satellites to move around three interfering orbits without collide each other. 
* Secondary goal: Controlling the satellites to switch between the orbits without collision. 

## How to...

To compile the source, 

* execute the `make clean; make all` command. 
* *make* will compile the program into `./train.elf`

To execute, 

* Restart the ARM box
* Load the program from `ARM/y386wang/train.elf`
* Send `go` to start execution

To control the satellite, send one of the following command

* `ob <satellite_id> <orbit_id>` assign orbit to a satellite
* `sw <switch_id> <direction>` assign switch direction
* `q` quit the program

Note: 

* `satellite_id` must be within INTEGER range
* `satellite_id` must be within range `[0-3]`
* `switch_id` is limited in range `[1-18]` and `[153-156]`
* `direction` is limited as either `C` or `S`


## Program Structure

### Train Center

Unlike the Milestone 2, the reservation system in the center has been simplified into a sensor attribution system. Whenever a satellite reserves, the reservation will be queued for each sensor. The up coming sensor update will be attributed according to each sensor's reservation queue. 

Not only reservation, the center is also the information bridge between satellites. Each satellite sends `SatelliteReport` to update their current orbit and current location on the orbit. If the satellite has a parent satellite, the center will forward it with its parent's `SatelliteReport`. In this way, satellites can monitor others location and adjust their speed accordingly. 

### Train Driver

Satellites neither settle down on a fix position, nor reverse their way. They keep moving on a curtain orbit for years. The orbits are designed so the satellites won't collide into each other. 

To simulate the behavior of satellite, our drivers never stop the satellite, nor reverse it. (Except at the beginning, they don't know their orbit yet.) They drive their satellite around the given orbit, and keep a safe distance with others by keep the communication alive. Other satellites maybe on different but interfering orbits.

When a satellite is going to enter an orbit, the driver will adjust its position and speed to adapt the new orbit's condition. 

## Algorithms

### Location Recovery

If a sensor was not attributed to any satellite, it will be saved and wait for the lost satellite's recovery request. Only the recovery request within certain time frame is considered as valid. Upon a valid request, center will resend the sensor update to the requester. The satellite will reconsider its location, based on the new sensor data. 

### Collision Avoidance

The orbits are interfering with each other. Thus, even the satellites running on different orbits, they may collide as well. In order to keep a safe distance between other satellites, the drivers comply to a dynamic hierarchy relationship. 

Except the first satellite on the most outer orbit, each satellite has a parent. Satellite driver send `SatelliteReport` every now and then to report the satellite's current location and receive it's parent's information. Depends on whether it's on the same orbit with its parent, the driver will adjust the speed with its best effort to keep the satellite within a certain distance behind its parent. 

Once the distance between satellites comes to a certain threshold, the system comes into a synchronized state. Only small adjustment will be performed to keep the entire system synced.

### Switching Orbit

By complying to the dynamic hierarchy relationship and smart adjustment on the speed, the satellites will stay a part from each other, as long as they stay on a single orbit. 

To accomplish orbit switching, the driver predicts the cut-in time and adjust the speed prior entering the new orbit. This ensure the new coming train will comply to the existing order within the orbit, and does not cut-in too soon or too late. 

Once the train successfully entered the new orbit, a new hierarchy formed. All the satellites will adjust speed so the entire system move towards synchronized state. 
