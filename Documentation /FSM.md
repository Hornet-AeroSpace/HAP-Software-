Title: 
Flight Computer Stage Diagram


Context:

**What will this chart achieve?**
	The intention of this chart is to expose the hardware & software req’s of our 
    system.  


FORMAT:  

1. Stage N, Title 

2. Functions: f
*What is happening during this stage* 

3. Transition Behavior: 	
*Noteworthy Event → Destination Stage*

    ASCENT/BOOST: 
    APOGEE: The highest point of the flight has been reached.
    DESCENT_DROGUE: Apogee deployment charge has fired. The rocket is descending under a small drogue parachute.
    : Main parachute deployment altitude reached and charge has fired (for L2/L3).
    : The rocket is on the ground. The system can now enter a recovery mode (e.g., beeping). 

--------------
Stage 1 “Pad Idle”: The initial state after power-on. Awaiting user input

Functions: 
1. Read accelerometer data 
2. Determine if any movement has happened 

Transition Behavior
If physical switch toggled -> Stage 2.  Switch will be in between Ematch cables 
--------------

Stage 2 “Might_Be_Launching” 

Functions 
1. Preform On-Flight computation (sensorfusion based velocity)
2. Maintain a running total of the previous 30sec of all sensor data 
3. Measure acceleration & barometric pressure 

Maybe - use microphone instead… Decibel level could indicate launch

Transition Behavior
- 1.7G’s > → Stage 3
--------------
 
Stage 3 “ASCENT/BOOST”:  Launch has been detected. The motor is burning.

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

Stage 4 “DESCENT_MAIN” : Motor has burned out, and decending. About to deploy Main parachute 

Functions:
1. Preform On-Flight computation (sensorfusion based velocity)
2. Transmit telemetry data through UART
3. Write telemetry data to sdCard.  
4. Deploy parachute off of pre-defined params. (optional)

Transition Behavior:
- Went Idle → Stage 5.

------------------------
Stage 5: LANDED/RECOVERY. 

------------------------
Success Metrics: 

1. Accuracy of timing inital launch 
2. Samping rate, Ideal versus measured
3. Accuracy of apogee timing 

** Please add more**



