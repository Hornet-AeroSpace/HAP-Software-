Title: 
Flight Computer Stage Diagram


Context:

**What will this chart achieve?**
	The intention of this chart is to expose the hardware & software req’s of our 
    system.  


FORMAT:  

1. Stage N, Title 

2. Functions: 
*What is happening during this stage* 

3. Transition Behavior: 
		*Noteworthy Event → Destination Stage*

--------------
Stage 1 “Idle”

Functions: 
1. Read accelerometer data 
2. Determine if any movement has happened 

Transition Behavior
- Light Jostle → Stage 2
- If still, Remain 
--------------

Stage 2 “Might be Launching”

Functions 
1. Preform On-Flight computation (sensorfusion based velocity)
2. Maintain a running total of the previous 30sec of all sensor data 
3. Measure acceleration & barometric pressure 

Maybe - use microphone instead… Decibel level could indicate launch

Transition Behavior
- Went Idle → Stage 1
- 1.7G’s > → Stage 3
--------------
 
Stage 3 “Actively Launching”

Functions:
1. Preform On-Flight computation (sensorfusion based velocity)
2. Switch off decision logic if spd
3. Transmit telemetry data through UART
4. Write telemetry data to sdCard.  
5. Deploy parachute off of pre-defined params. 

Transition Behavior:
- Parachute Deployed → Stage 4
- Anything Else, Remain. 


—---------------

Stage 4 “Launch (Second Stage)”

Functions:
1. Preform On-Flight computation (sensorfusion based velocity)
2. Transmit telemetry data through UART
3. Write telemetry data to sdCard.  
4. Deploy parachute off of pre-defined params. (optional)

Transition Behavior:
- Went Idle → Stage 1



Success Metrics: 

1. Accuracy of timing during launch 
2. Samping rate, Ideal versus measured
3. Accuracy of apogee timing 
** Please add more**



