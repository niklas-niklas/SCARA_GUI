#include <Arduino.h>
#include <Stepper.h>
#include <Wire.h>
#include <AS5600.h>

#include <math.h>
#include <SpeedyStepper.h>



#include <websocket.h>
// #include <Varibles.h>
    Servo myservo;

    
    AS5600 encoder;


    // encoder.setAddress(0x36);



//Steppers

    SpeedyStepper stepper1;   
    SpeedyStepper stepper2;
    

//Position
    //dimensions
    int len_arm = 140;
    float limit_left_1 = 62.5, limit_left_2 = 49.5;

    //Values

    float goto_1_pos=0, goto_2_pos=0, x_goto,y_goto;
    float angle_goto=0,dis_goto=0,angle_cur=0,dis_cur=0;
    float angle_goto_diff=0, angle_cur_diff=0;

    float cur_angle_1,goto_angle_1,goto_angle_1_test;  
    float cur_angle_2,goto_angle_2,goto_angle_2_test;  

    float cur_x,cur_y;
    //Calculations
    

//Limit switches
    #define limit1 23    //Lower arm
    #define limit2 19    //Upper arm
    #define stepper1_stall 18



//Steppers
    //General
    AS5600 as5600;
    const int stepsPerRevolution = 800;  
    const int gear_ratio = 12;
    int rotation = stepsPerRevolution*gear_ratio;
      
    #define MS1 4
    #define MS2 16
    #define MS3 17

    #define EN1 26
    #define EN2 32



//Boundary xy_positions

float L_stop_1=0.172, R_stop_1=0.828;
float L_stop_2=0.14, R_stop_2=0.845;

// float L_stop_1=0.2, R_stop_1=0.8;
// float L_stop_2=0.2, R_stop_2=0.8;

