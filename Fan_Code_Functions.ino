void dataSent(const uint8_t* mac_addr, esp_now_send_status_t status)
{
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}







void dataRecieved(const uint8_t* mac, const uint8_t* incomingData, int len)
{
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
  Serial.print("Bytes received: ");
  Serial.println(len);
  incomingTemp = incomingReadings.avgTemp;
  incomingTemp = (incomingTemp * 9/5) + 32;
  displayFlag=1;
  //dispRefresh =0;
  previousTime = millis(); // Update the time of the last data reception
  noSignal=0;
}








void readButtons(void)
{
  int buttonVal1 = digitalRead(ON_OFF);
  if(buttonVal1 != prevButtonVal1){
    if(buttonVal1 == LOW){
      Serial.println("Button 1 Pressed");

      
      tft.fillScreen(ST77XX_BLACK);
      isOn = !isOn;

      if(isOn == false)
      {
        analogWrite(ENA, 0);
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, HIGH);
        tft.setTextWrap(false);
        tft.setTextSize(4);
        tft.setTextColor(ST77XX_MAGENTA, ST77XX_BLACK); 
      
        tft.setCursor(66, 50);
        tft.println("SLEEPING");
        delay(2000);
        tft.fillScreen(ST77XX_BLACK);

        //esp_deep_sleep_start(); // Put ESP32 into deep sleep


      }
      if(isOn == true && noSignal == 0){
        editMode = ACTIVE;
        printSensorReadings(incomingTemp, temp_unit, targetAppTemp);
      }
    }
    delay(50);
  }

  prevButtonVal1 = buttonVal1;

  int buttonVal2 = digitalRead(increase);
  if(buttonVal2 != prevButtonVal2)
  {
    // Serial.println("Button 2 new val");
    if(buttonVal2 == LOW)
    {
      // Serial.println("Button 2 low");
      if(editMode == EDIT_TEMP)
      {
        targetAppTemp += 0.5;
        
        if(targetAppTemp >90)
        {
        targetAppTemp = 90;
        }
        // Serial.println("Button 2 Pressed");
        // Serial.println(targetAppTemp);
      }
      // else if(editMode == EDIT_UNIT)
      // {
      //   temp_unit = TEMP_C_MODE;
      // }
    }
    delay(50);
  }

  prevButtonVal2 = buttonVal2;

  int buttonVal3 = digitalRead(decrease);
  if(buttonVal3 != prevButtonVal3){
    if(buttonVal3 == LOW){
      if(editMode == EDIT_TEMP){
        targetAppTemp -= 0.5;
        if(targetAppTemp <10)
        {
        targetAppTemp =10;
        }
        Serial.println("Button 3 Pressed");
        Serial.println(targetAppTemp);
      // }else if(editMode == EDIT_UNIT){
      //   temp_unit = TEMP_F_MODE;
      }
    }
    delay(50);
  }

  prevButtonVal3 = buttonVal3;

  int buttonVal4 = digitalRead(modeSel);
  if(buttonVal4 != prevButtonVal4){
    if(buttonVal4 == LOW && isOn == true){
       Serial.println("Button 4 Pressed");
      dispRefresh = 0;
      switch(editMode){
        case ACTIVE:{
          editMode = EDIT_TEMP;
          break;
        }
        case EDIT_TEMP:{
          //editMode = EDIT_UNIT;
          editMode = ACTIVE;
          tft.fillScreen(ST77XX_BLACK);
          if(noSignal == 0)
          {
            printSensorReadings(incomingTemp, temp_unit, targetAppTemp);
            fanSpeedControl();

          }

          break;
        }
        // case EDIT_UNIT:{
        //   editMode = ACTIVE;
        //   break;
        //}
      }
    }
    delay(50);
  }
  prevButtonVal4 = buttonVal4;
}

void displayEditMode(float targetTemp, int mode)
{
  tft.setTextWrap(false);
  tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
  tft.setCursor(55, TARGET_DISP_VERT_COORD);
  tft.setTextSize(6);

  tft.cp437(true); // Use correct CP437 character codes
  tft.setCursor(37, TARGET_DISP_VERT_COORD);
  tft.setTextSize(6);

  tft.drawRoundRect(27, TARGET_DISP_VERT_COORD-10, 266, 62, 12, ST77XX_CYAN);
  tft.drawRoundRect(26, TARGET_DISP_VERT_COORD-11, 268, 64, 13, ST77XX_CYAN);
  tft.drawRoundRect(25, TARGET_DISP_VERT_COORD-12, 270, 66, 14, ST77XX_CYAN);
  tft.print(targetTemp);

  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK); //set text to white and background to black
  tft.write(0xF8); // Print the degrees symbol

  switch(mode)
  {
    case TEMP_F_MODE:    
      tft.println("F");   
    break;
    case TEMP_C_MODE:
      tft.println("C");   
    break;
  }
}







void printSensorReadings(float temperature, int mode, float targetTemp) //-------------------------------------------------------------------------------------
{ 
  tft.setTextWrap(false);
  tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK); 
  
  //DISPLAY RECORDED TEMPERATURE
  //tft.fillRoundRect(0, 0, 320, 100, 0, ST77XX_BLACK);

  if(temperature < 10 && temperature > 0) // if text is three digits, make smaller.
  {
    if(size1Flag == 0){
      tft.fillRoundRect(0, 0, 320, 100, 0, ST77XX_BLACK);
    }
    tft.setCursor(55, TEMP_DISP_VERT_COORD);
    tft.setTextSize(6);

    tft.drawRoundRect(45, TEMP_DISP_VERT_COORD-10, 266 -36, 62, 12, ST77XX_MAGENTA);
    tft.drawRoundRect(44, TEMP_DISP_VERT_COORD-11, 268 -36, 64, 13, ST77XX_MAGENTA);
    tft.drawRoundRect(43, TEMP_DISP_VERT_COORD-12, 270 -36, 66, 14, ST77XX_MAGENTA);
    size1Flag = 1;
    size2Flag = 0;
    size3Flag = 0;
  }
  if(temperature < 100 && temperature >= 10) // if text is three digits, make smaller. //7 characters
  {
    if(size2Flag == 0){
      tft.fillRoundRect(0, 0, 320, 100, 0, ST77XX_BLACK);
    }
    tft.setCursor(37, TEMP_DISP_VERT_COORD);
    tft.setTextSize(6);

    tft.drawRoundRect(27, TEMP_DISP_VERT_COORD-10, 266, 62, 12, ST77XX_MAGENTA);
    tft.drawRoundRect(26, TEMP_DISP_VERT_COORD-11, 268, 64, 13, ST77XX_MAGENTA);
    tft.drawRoundRect(25, TEMP_DISP_VERT_COORD-12, 270, 66, 14, ST77XX_MAGENTA);
    size1Flag = 0;
    size2Flag = 1;
    size3Flag = 0;
  }
  if(temperature < 1000 && temperature >= 100) // if text is three digits, make smaller.
  {
    if(size3Flag == 0){
      tft.fillRoundRect(0, 0, 320, 100, 0, ST77XX_BLACK);
    }
    tft.setCursor(19, TEMP_DISP_VERT_COORD);
    tft.setTextSize(6);

    tft.drawRoundRect(9, TEMP_DISP_VERT_COORD-10, 266+36, 62, 12, ST77XX_MAGENTA);
    tft.drawRoundRect(8, TEMP_DISP_VERT_COORD-11, 268+36, 64, 13, ST77XX_MAGENTA);
    tft.drawRoundRect(7, TEMP_DISP_VERT_COORD-12, 270+36, 66, 14, ST77XX_MAGENTA);
    size1Flag = 0;
    size2Flag = 0;
    size3Flag = 1;
  }
  tft.setTextColor(ST77XX_MAGENTA, ST77XX_BLACK); //set text to white and background to black
  tft.cp437(true); // Use correct CP437 character codes
  tft.print(temperature);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK); //set text to white and background to black
  tft.write(0xF8); // Print the degrees symbol

  switch(mode)
  {
    case TEMP_F_MODE:    
      tft.println("F");   
    break;
    case TEMP_C_MODE:
      tft.println("C");   
    break;
  }


  //DISPLAY TARGET TEMPERATURE 
  tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK); //set text to white and background to black
  tft.setCursor(78, TEMP_DISP_VERT_COORD+80);
  tft.setTextSize(4);
  tft.drawRoundRect(72, TEMP_DISP_VERT_COORD+73, 176, 42, 7, ST77XX_CYAN);
  tft.drawRoundRect(71, TEMP_DISP_VERT_COORD+72, 178, 44, 8, ST77XX_CYAN);
  tft.cp437(true); // Use correct CP437 character codes
  tft.print(targetTemp);

  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK); //set text to white and background to black
  tft.write(0xF8); // Print the degrees symbol

  switch(mode)
  {
    case TEMP_F_MODE:    
      tft.println("F");   
    break;
    case TEMP_C_MODE:
      tft.println("C");   
    break;
  }

  //FAN SPEED BAR
  tft.drawRoundRect(59, 180, 204, 20, 7, ST77XX_WHITE);
  tft.fillRoundRect(61, 182, 200, 16, 5, ST77XX_BLACK); //make the width adribute a variable for the speed.
  tft.fillRoundRect(61, 182, fanSpeedPercent*2, 16, 5, ST77XX_CYAN); //make the width adribute a variable for the speed.

}







void fanSpeedControl(void)
  {
    fanSpeed = (incomingTemp-targetAppTemp)*(255/(ABS_MAX_TEMP-targetAppTemp));
    
    if(fanSpeed > 255)
    {
        fanSpeed = 255;
    }
    if(fanSpeed < 70) //find threshold where fan turns off
    {
        fanSpeed = 0;
    }
    Serial.println(fanSpeed);
    fanSpeedPercent = (fanSpeed * 100)/(255);
    analogWrite(ENA, fanSpeed);
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
  }
