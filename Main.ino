int stageNumber = 1;

void setup() {

}

void loop() {

}


//-------------- Function Definitions --------------

void stageOne() { //Idle
    //read accelerometer data
    //Determine if any movement has happened

    if (0 /* light jostle */) { //if (0) as placeholders; delete when implimented
        stageNumber = 2;
    }
}

void stageTwo() { //Potentially Launching
    //Preform On-Flight computation (sensorfusion based velocity)
    //Maintain a running total of the previous 30sec of all sensor data
    //Measure acceleration & barometric pressure
    
    if (0 /* accelerated at 1.7Gs */) { //Maybe - use microphone instead… Decibel level could indicate launch
        stageNumber = 3;
    } else if (0 /* went idle */) {
        stageNumber = 1;
    }
}

void stageThree() { //Actively Launching
    //Preform On-Flight computation (sensorfusion based velocity)
    //Switch off decision logic if spd
    //Transmit telemetry data through UART
    //Write telemetry data to sdCard

    if (0 /* pre-defined parameters to deploy parachute */) {
        //deploy parachute
        stageNumber = 4;
    }
}

void stageFour() { //Launch (Second Stage)
    //Preform On-Flight computation (sensorfusion based velocity)
    //Transmit telemetry data through UART
    //Write telemetry data to sdCard
    //Deploy parachute off of pre-defined params. (optional)
}