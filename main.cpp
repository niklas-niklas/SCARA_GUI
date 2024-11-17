#include <Scara_new.h>

void loop2(void);

void task1code(void *pvParameters)
{
  for (;;)
  {
    loop2();
  }
}

int sent = 0;
void setup(){

    Serial.begin(115200);
    Wire.begin();

    

    initWiFi();
    initLittleFS();
    initWebSocket();

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());


  initWebSocket();
  Serial.println("WS_INIT");

  // Route for root / web page
  server.on("/alex", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/main_Alex.html", String(), false);
  });

    server.on("/nik", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/main_Nik.html", String(), false);
  });

  // Start server
  server.begin();
  Serial.println("Server started");

    // myservo.attach(6);
    digitalWrite(EN1,LOW);    //enable stepper 1
    digitalWrite(EN2,LOW);   //enable stepper 2

    pinMode(MS1,OUTPUT);
    pinMode(MS2,OUTPUT);
    pinMode(MS3,OUTPUT);

    digitalWrite(MS1,1);
    digitalWrite(MS2,1);
    digitalWrite(MS3,1);

    stepper1.connectToPins(14,27);
    stepper2.connectToPins(33,25);

    stepper1.setStepsPerRevolution(200*12*16);
    stepper2.setStepsPerRevolution(200*12*16);

    stepper1.setSpeedInRevolutionsPerSecond(speed);
    stepper2.setSpeedInRevolutionsPerSecond(speed);

    stepper1.setAccelerationInRevolutionsPerSecondPerSecond(0.3);
    stepper2.setAccelerationInRevolutionsPerSecondPerSecond(0.3);

    encoder.begin();

    if (encoder.isConnected()) {
		Serial.println("encoder connected");
	}


    if (!myservo.attached()) {
		myservo.setPeriodHertz(50); // standard 50 hz servo
		myservo.attach(13, 1000, 2000); // Attach the servo after it has been detatched
	}

    myservo.write(180);

    pinMode(limit1, INPUT_PULLUP);      //limit switch1
    pinMode(limit2, INPUT_PULLUP);      //limit switch2
    // pinMode(stepper1_stall, INPUT_PULLUP);

        Serial.print("stepper1_loc: " );
        Serial.println(stepper1.getCurrentPositionInRevolutions());
        Serial.print("stepper2_loc: " );
        Serial.println(stepper2.getCurrentPositionInRevolutions());

    myservo.write(0);

    if(!myservo.attached()){Serial.println("not attached");}
    
}
        float old_x = 0;
        float old_y=280;
        float step=10;
        float interpolated_y;
        float interpolated_x;
        float istep;

void loop(){
    
    stepper1.setSpeedInRevolutionsPerSecond(speed);
    stepper2.setSpeedInRevolutionsPerSecond(speed);

    stepper1.processMovement();
    stepper2.processMovement();

    if (sent==50000){

        ang1 = goto_angle_1*360;
        ang2 = goto_angle_2*360;

        String sending = sendCoordinatesToWeb();

        notifyClients(sending);
        sent=0;
        // uint16_t temp = encoder.rawAngle();
        // status = String(temp);
        // Serial.println(encoder.detectMagnet());
        // // Serial.println(encoder.readStatus());
    }
    sent++;

    if(hoome){home();}

    if(preset != 0){presets();}

    if(digitalRead(limit1) == LOW||digitalRead(limit2) == LOW){home();}

    if((add_move_x != 1000)&&(add_move_y !=1000)){

        xy_positions.enqueueMove(add_move_x,add_move_y);
        add_move_x = 1000;
        add_move_y = 1000;

    }

    if(stepper1_stall == 0){
        Serial.print("stepper 1 stalled");
    }

    if((new_g_x != 1000)&&(new_g_y !=1000)){


        if(new_g==5){
            status = "adding in extra points";
            float x_5,y_5,x_dif,y_dif;
            float dis;

            x_5 = old_x;
            y_5 = old_y; 

            x_dif = new_g_x- x_5; 
            y_dif = new_g_y- y_5; 

            dis = sqrt(pow(abs(x_dif),2)+pow(abs(y_dif),2));
            step = round(dis/5);

            for(float i=1;i<=step;i++){
                istep = (i/step);
                interpolated_x = x_5+(x_dif*(istep));
                interpolated_y = y_5+(y_dif*(istep));
                
                xy_positions.enqueueMove(interpolated_x, interpolated_y);
                // i/step;
                
            }

        }
        else if(new_g==2){
            status = "adding move to list";
            xy_positions.enqueueMove(new_g_x,new_g_y);
            
        }
        old_x = new_g_x;
        old_y = new_g_y;
        new_g_x = 1000;
        new_g_y = 1000;
    }

    if(xy_positions.isEmpty()){
        xy_positions.clearMoves();
        }


    if(!ang_positions.isEmpty()&&!moving&&!stop){
        
        moving = true;
        float temp_x, temp_y, temp_z;
        status= "Moving...";

        ang_positions.getNextMove(temp_x, temp_y);
        stepper1.setupMoveInRevolutions(temp_x);
        stepper2.setupMoveInRevolutions(temp_y);
        if(ang_positions.isEmpty()){
            ang_positions.clearMoves();
            status= "Moving Complete";
        }
    }

    if(stop){
        status = "EMERGENCY STOPPED";
    }
    

    while(!xy_positions.isEmpty()){
        coords();
        
    }
    
    

    if(stepper1.motionComplete()&&stepper2.motionComplete()&&moving){
        moving = false;

        goto_angle_2=stepper2.getCurrentPositionInRevolutions();
        goto_angle_1=stepper1.getCurrentPositionInRevolutions();
        cur_x = x_goto;
        cur_y = y_goto;

    }

    xValue = cur_x;
    yValue = cur_y;

}

void home(){
    Serial.println("Homing steppers");
    status= "Homing Steppers";
    moving = true;

    stepper2.moveToHomeInRevolutions(-1, 0.3, 1, limit2);

    stepper2.moveRelativeInRevolutions(130.5/360);

    stepper1.moveToHomeInRevolutions(-1, 0.2, 1, limit1);

    stepper1.moveRelativeInRevolutions(116.0/360);

    stepper1.setCurrentPositionInRevolutions(0.5);

    stepper2.setCurrentPositionInRevolutions(0.5);

    goto_angle_1=stepper1.getCurrentPositionInRevolutions();
    goto_angle_2=stepper2.getCurrentPositionInRevolutions();

    cur_x = 0;
    cur_y = 280;

    Serial.println("finished homing");
    status= "Homing Complete";
    hoome=false;
    moving = false;
}

void XY_AngDis(float x,float y){
    
    x_goto = x, y_goto = y;
    dis_goto = sqrt(pow(abs(x_goto),2)+pow(abs(y_goto),2));//0,0 to point
    angle_goto = (atan(abs(x_goto)/abs(y_goto))/(2*PI));//angle to 0
    quad_check();//what quadrant
    angle_goto_diff = (acos((dis_goto/2)/len_arm)/(2*PI));
    flip();//what way does the arm pose

    if(valid()){
        // Serial.println("sending inter");
        stepper1.setSpeedInRevolutionsPerSecond(speed);
        stepper2.setSpeedInRevolutionsPerSecond(speed);
        goto_angle_1=goto_angle_1_test;
        goto_angle_2=goto_angle_2_test;
        ang_positions.enqueueMove(goto_angle_1,goto_angle_2);

    }
    else{
        // Serial.println("sending inter");
        status= "Invalid move entered";
    }
}


bool valid(){
    if((goto_angle_1_test>L_stop_1)&&(goto_angle_1_test<R_stop_1)&&(goto_angle_2_test>L_stop_2)&&(goto_angle_2_test<R_stop_2)){
        return true;
    }
    return false;
}



void moveServo(int i){

    myservo.write(i);

    if(i==0){toggleState ="Up";}
    else{toggleState = "Down";}


}

void quad_check(){
        if((x_goto>0)&(y_goto>0)){angle_goto=0.5+angle_goto;}//++ top right
        if((x_goto>0)&(y_goto<0)){angle_goto=1-angle_goto;}//+- bottom right
        if((x_goto<0)&(y_goto<0)){angle_goto=0+angle_goto;}//-- bottom left
        if((x_goto<0)&(y_goto>0)){angle_goto=0.5-angle_goto;}//-+ top left
        if((x_goto==0)&(y_goto>0)){angle_goto=0.5;}//x-axis pos
        if((x_goto==0)&(y_goto<0)){angle_goto=0;}//x-axis neg
        if((x_goto>0)&(y_goto==0)){angle_goto=0.75;}//y-axis pos
        if((x_goto<0)&(y_goto==0)){angle_goto=0.25;}//y-axis neg
}

void flip(){

    //Left
    goto_angle_1_test=angle_goto-angle_goto_diff;
    goto_angle_2_test=0.5+angle_goto_diff*2;

    if(!valid()){
        //Right
        goto_angle_1_test=angle_goto+angle_goto_diff;
        goto_angle_2_test=0.5-angle_goto_diff*2;
    }

    if(!valid()){
        goto_angle_1_test=cur_angle_1;
        goto_angle_2_test=cur_angle_2;
        status= "Invalid move entered";
    }
}

void coords(){

    float temp_x, temp_y, temp_z;
    xy_positions.getNextMove(temp_x, temp_y, temp_z);

    XY_AngDis(temp_x, temp_y);

}

void presets(){
    if(preset == 1){
        preset1(); 
        preset=0;
        Serial.println("yes");
    }
    if(preset == 2){
        preset2(); 
        preset=0;
        Serial.println("yes");
    }

    if(preset == 5){
        ang_positions.clearMoves(); 
        preset=0;
    }
    if(preset == 6){
        status= "Stopping";
        stop = !stop;
        
        preset = 0;
    }
}

void preset1(){
    xy_positions.enqueueMove(0,230,1);
    moveServo(0);
    xy_positions.enqueueMove(0,210);
    xy_positions.enqueueMove(20,190);
    xy_positions.enqueueMove(20,170);
    xy_positions.enqueueMove(0,150);
    xy_positions.enqueueMove(-20,170);
    xy_positions.enqueueMove(-20,190);
    xy_positions.enqueueMove(-10,200);
    xy_positions.enqueueMove(0,170,1);
    xy_positions.enqueueMove(0,190);
    xy_positions.enqueueMove(-20,210);
    xy_positions.enqueueMove(-20,230,1);
    xy_positions.enqueueMove(0,250);
    xy_positions.enqueueMove(20,230);
    xy_positions.enqueueMove(20,210);
    xy_positions.enqueueMove(10,200,1);
    moveServo(180);
    xy_positions.enqueueMove(0,280);
    Serial.println("preset 1");
}

void preset2(){
    // From (50, 150) to (50, 250)
    
    xy_positions.enqueueMove(50, 150);
    moveServo(0);
    xy_positions.enqueueMove(50, 152);
    xy_positions.enqueueMove(50, 154);
    xy_positions.enqueueMove(50, 156);
    xy_positions.enqueueMove(50, 158);
    xy_positions.enqueueMove(50, 160);
    xy_positions.enqueueMove(50, 162);
    xy_positions.enqueueMove(50, 164);
    xy_positions.enqueueMove(50, 166);
    xy_positions.enqueueMove(50, 168);
    xy_positions.enqueueMove(50, 170);
    xy_positions.enqueueMove(50, 172);
    xy_positions.enqueueMove(50, 174);
    xy_positions.enqueueMove(50, 176);
    xy_positions.enqueueMove(50, 178);
    xy_positions.enqueueMove(50, 180);
    xy_positions.enqueueMove(50, 182);
    xy_positions.enqueueMove(50, 184);
    xy_positions.enqueueMove(50, 186);
    xy_positions.enqueueMove(50, 188);
    xy_positions.enqueueMove(50, 190);
    xy_positions.enqueueMove(50, 192);
    xy_positions.enqueueMove(50, 194);
    xy_positions.enqueueMove(50, 196);
    xy_positions.enqueueMove(50, 198);
    xy_positions.enqueueMove(50, 200);
    xy_positions.enqueueMove(50, 202);
    xy_positions.enqueueMove(50, 204);
    xy_positions.enqueueMove(50, 206);
    xy_positions.enqueueMove(50, 208);
    xy_positions.enqueueMove(50, 210);
    xy_positions.enqueueMove(50, 212);
    xy_positions.enqueueMove(50, 214);
    xy_positions.enqueueMove(50, 216);
    xy_positions.enqueueMove(50, 218);
    xy_positions.enqueueMove(50, 220);
    xy_positions.enqueueMove(50, 222);
    xy_positions.enqueueMove(50, 224);
    xy_positions.enqueueMove(50, 226);
    xy_positions.enqueueMove(50, 228);
    xy_positions.enqueueMove(50, 230);
    xy_positions.enqueueMove(50, 232);
    xy_positions.enqueueMove(50, 234);
    xy_positions.enqueueMove(50, 236);
    xy_positions.enqueueMove(50, 238);
    xy_positions.enqueueMove(50, 240);
    xy_positions.enqueueMove(50, 242);
    xy_positions.enqueueMove(50, 244);
    xy_positions.enqueueMove(50, 246);
    xy_positions.enqueueMove(50, 248);
    xy_positions.enqueueMove(50, 250);

    // From (50, 250) to (-50, 250)
    xy_positions.enqueueMove(50, 250);
    xy_positions.enqueueMove(48, 250);
    xy_positions.enqueueMove(46, 250);
    xy_positions.enqueueMove(44, 250);
    xy_positions.enqueueMove(42, 250);
    xy_positions.enqueueMove(40, 250);
    xy_positions.enqueueMove(38, 250);
    xy_positions.enqueueMove(36, 250);
    xy_positions.enqueueMove(34, 250);
    xy_positions.enqueueMove(32, 250);
    xy_positions.enqueueMove(30, 250);
    xy_positions.enqueueMove(28, 250);
    xy_positions.enqueueMove(26, 250);
    xy_positions.enqueueMove(24, 250);
    xy_positions.enqueueMove(22, 250);
    xy_positions.enqueueMove(20, 250);
    xy_positions.enqueueMove(18, 250);
    xy_positions.enqueueMove(16, 250);
    xy_positions.enqueueMove(14, 250);
    xy_positions.enqueueMove(12, 250);
    xy_positions.enqueueMove(10, 250);
    xy_positions.enqueueMove(8, 250);
    xy_positions.enqueueMove(6, 250);
    xy_positions.enqueueMove(4, 250);
    xy_positions.enqueueMove(2, 250);
    xy_positions.enqueueMove(0, 250);
    xy_positions.enqueueMove(-2, 250);
    xy_positions.enqueueMove(-4, 250);
    xy_positions.enqueueMove(-6, 250);
    xy_positions.enqueueMove(-8, 250);
    xy_positions.enqueueMove(-10, 250);
    xy_positions.enqueueMove(-12, 250);
    xy_positions.enqueueMove(-14, 250);
    xy_positions.enqueueMove(-16, 250);
    xy_positions.enqueueMove(-18, 250);
    xy_positions.enqueueMove(-20, 250);
    xy_positions.enqueueMove(-22, 250);
    xy_positions.enqueueMove(-24, 250);
    xy_positions.enqueueMove(-26, 250);
    xy_positions.enqueueMove(-28, 250);
    xy_positions.enqueueMove(-30, 250);
    xy_positions.enqueueMove(-32, 250);
    xy_positions.enqueueMove(-34, 250);
    xy_positions.enqueueMove(-36, 250);
    xy_positions.enqueueMove(-38, 250);
    xy_positions.enqueueMove(-40, 250);
    xy_positions.enqueueMove(-42, 250);
    xy_positions.enqueueMove(-44, 250);
    xy_positions.enqueueMove(-46, 250);
    xy_positions.enqueueMove(-48, 250);
    xy_positions.enqueueMove(-50, 250);

    // // From (-50, 250) to (-50, 150)
    xy_positions.enqueueMove(-50, 250);
    xy_positions.enqueueMove(-50, 248);
    xy_positions.enqueueMove(-50, 246);
    xy_positions.enqueueMove(-50, 244);
    xy_positions.enqueueMove(-50, 242);
    xy_positions.enqueueMove(-50, 240);
    xy_positions.enqueueMove(-50, 238);
    xy_positions.enqueueMove(-50, 236);
    xy_positions.enqueueMove(-50, 234);
    xy_positions.enqueueMove(-50, 232);
    xy_positions.enqueueMove(-50, 230);
    xy_positions.enqueueMove(-50, 228);
    xy_positions.enqueueMove(-50, 226);
    xy_positions.enqueueMove(-50, 224);
    xy_positions.enqueueMove(-50, 222);
    xy_positions.enqueueMove(-50, 220);
    xy_positions.enqueueMove(-50, 218);
    xy_positions.enqueueMove(-50, 216);
    xy_positions.enqueueMove(-50, 214);
    xy_positions.enqueueMove(-50, 212);
    xy_positions.enqueueMove(-50, 210);
    xy_positions.enqueueMove(-50, 208);
    xy_positions.enqueueMove(-50, 206);
    xy_positions.enqueueMove(-50, 204);
    xy_positions.enqueueMove(-50, 202);
    xy_positions.enqueueMove(-50, 200);
    xy_positions.enqueueMove(-50, 198);
    xy_positions.enqueueMove(-50, 196);
    xy_positions.enqueueMove(-50, 194);
    xy_positions.enqueueMove(-50, 192);
    xy_positions.enqueueMove(-50, 190);
    xy_positions.enqueueMove(-50, 188);
    xy_positions.enqueueMove(-50, 186);
    xy_positions.enqueueMove(-50, 184);
    xy_positions.enqueueMove(-50, 182);
    xy_positions.enqueueMove(-50, 180);
    xy_positions.enqueueMove(-50, 178);
    xy_positions.enqueueMove(-50, 176);
    xy_positions.enqueueMove(-50, 174);
    xy_positions.enqueueMove(-50, 172);
    xy_positions.enqueueMove(-50, 170);
    xy_positions.enqueueMove(-50, 168);
    xy_positions.enqueueMove(-50, 166);
    xy_positions.enqueueMove(-50, 164);
    xy_positions.enqueueMove(-50, 162);
    xy_positions.enqueueMove(-50, 160);
    xy_positions.enqueueMove(-50, 158);
    xy_positions.enqueueMove(-50, 156);
    xy_positions.enqueueMove(-50, 154);
    xy_positions.enqueueMove(-50, 152);
    xy_positions.enqueueMove(-50, 150);

    // From (-50, 150) to (50, 150)
    xy_positions.enqueueMove(-50, 150);
    xy_positions.enqueueMove(-48, 150);
    xy_positions.enqueueMove(-46, 150);
    xy_positions.enqueueMove(-44, 150);
    xy_positions.enqueueMove(-42, 150);
    xy_positions.enqueueMove(-40, 150);
    xy_positions.enqueueMove(-38, 150);
    xy_positions.enqueueMove(-36, 150);
    xy_positions.enqueueMove(-34, 150);
    xy_positions.enqueueMove(-32, 150);
    xy_positions.enqueueMove(-30, 150);
    xy_positions.enqueueMove(-28, 150);
    xy_positions.enqueueMove(-26, 150);
    xy_positions.enqueueMove(-24, 150);
    xy_positions.enqueueMove(-22, 150);
    xy_positions.enqueueMove(-20, 150);
    xy_positions.enqueueMove(-18, 150);
    xy_positions.enqueueMove(-16, 150);
    xy_positions.enqueueMove(-14, 150);
    xy_positions.enqueueMove(-12, 150);
    xy_positions.enqueueMove(-10, 150);
    xy_positions.enqueueMove(-8, 150);
    xy_positions.enqueueMove(-6, 150);
    xy_positions.enqueueMove(-4, 150);
    xy_positions.enqueueMove(-2, 150);
    xy_positions.enqueueMove(0, 150);
    xy_positions.enqueueMove(2, 150);
    xy_positions.enqueueMove(4, 150);
    xy_positions.enqueueMove(6, 150);
    xy_positions.enqueueMove(8, 150);
    xy_positions.enqueueMove(10, 150);
    xy_positions.enqueueMove(12, 150);
    xy_positions.enqueueMove(14, 150);
    xy_positions.enqueueMove(16, 150);
    xy_positions.enqueueMove(18, 150);
    xy_positions.enqueueMove(20, 150);
    xy_positions.enqueueMove(22, 150);
    xy_positions.enqueueMove(24, 150);
    xy_positions.enqueueMove(26, 150);
    xy_positions.enqueueMove(28, 150);
    xy_positions.enqueueMove(30, 150);
    xy_positions.enqueueMove(32, 150);
    xy_positions.enqueueMove(34, 150);
    xy_positions.enqueueMove(36, 150);
    xy_positions.enqueueMove(38, 150);
    xy_positions.enqueueMove(40, 150);
    xy_positions.enqueueMove(42, 150);
    xy_positions.enqueueMove(44, 150);
    xy_positions.enqueueMove(46, 150);
    xy_positions.enqueueMove(48, 150);
    xy_positions.enqueueMove(50, 150);
    moveServo(180);
    xy_positions.enqueueMove(0,280);
    Serial.println("preset 2");
}

// void preset3(){}

// void preset4(){}