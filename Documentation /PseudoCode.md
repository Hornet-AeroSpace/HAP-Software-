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
- If physical switch toggled -> Stage 2.  Switch will be in between Ematch cables
--------------

Stage 2 “Might_Be_Launching” 

Functions 
1. Preform On-Flight computation (sensorfusion based velocity)
2. Begin writing to sd file from BoF (beginning of file) 
3. Measure acceleration & barometric pressure 

Maybe - use microphone instead… Decibel level could indicate launch

Transition Behavior
- 2.5G’s >  → Stage 3
- 5 seconds pass -> stage 2
--------------
 
Stage 3 “ASCENT/BOOST”: Launch has been detected. The motor is burning.

Functions:
1. Cut off parachute ejection logic. 
2. Transmit telemetry data through UART.
3. Write telemetry data to sdCard.  


Transition Behavior:
- 4.5 seconds pass -> Stage 4 
—---------------
 
Stage 4 “Coast”: Motor has burned out, apogee is imminent

Functions:
1. Preform On-Flight computation (sensorfusion based velocity)
3. Transmit telemetry data through UART
4. Write telemetry data to sdCard.  
5. Deploy parachute off of pre-defined params. 

Transition Behavior:
- Parachute Deployed → Stage 5
- Anything Else, Remain. 
—---------------

Stage 5 “DESCENT_MAIN” : Apogee has been reached. About to deploy Main parachute 

Functions:
1. Preform On-Flight computation (sensorfusion based velocity)
2. Transmit telemetry data through UART
3. Write telemetry data to sdCard.  
4. Deploy parachute off of pre-defined params. (optional)

Transition Behavior:
- Went Idle → Stage 6.

------------------------
Stage 6: LANDED/RECOVERY. 

------------------------
Success Metrics: 

1. Apogee Deployment Accuracy: Did the drogue parachute deploy within ± 10 
meters of the true apogee recorded by the backup flight computer? ​

2. Launch Detection Latency: What was the time difference between motor ignition 
(from stopwatch) and the software's transition to Stage 3?​

3. Data Integrity: Was the entire flight successfully logged to the SD card with no 
missing data points or file corruption?​

4. Sample Rate Consistency: Did the measured sample rate in-flight remain close 
to the target 1k –1.5k Hz in Stage 3/4 ?​

5. State Transition Reliability: Does the logged data confirm that all state 
transitions occurred correctly and for the right reasons?​

6. Backup Timer Functionality: (For dedicated test) Did the backup timer 
correctly deploy the charge if apogee detection is disabled?​
​

** Please add more**



