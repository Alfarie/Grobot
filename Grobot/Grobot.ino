#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include<EEPROM.h>

#define CS 48
LiquidCrystal_I2C lcd(0x27, 20, 4);
#define DS1307_I2C_ADDRESS 0x68

#define MAIN 0
#define FAN 1
#define PUMP 2
#define LED 3
#define TIMER 4
#define SET_DATE 5

#define UP 0
#define DOWN 1
#define LEFT 2
#define RIGHT 3

#define SET 9
#define RESET 8

#define RELAY01 7
#define RELAY02 6
#define RELAY03 5

#define FAN_STATE_ADDR 0
#define PUMP_STATE_ADDR 1
#define LED_STATE_ADDR 2

#define FAN_TIMER_SIZE_ADDR 4
#define PUMP_TIMER_SIZE_ADDR 5
#define LED_TIMER_SIZE_ADDR 6

const int On = 129;
const int Off = 128;
const int Program = 130;

int8_t state = MAIN;

byte relay_status[3]  = {Off, Off, Off};

uint8_t fan_timer_list[8][4];
byte fan_timer_size = 0;

uint8_t pump_timer_list[8][4];
byte pump_timer_size = 0;

uint8_t led_timer_list[8][4];
byte led_timer_size = 0;

// lcd data
byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;

long blinkTime = 0;

void Date_Set() {
  int num[6] = { hour , minute , second , dayOfMonth , month, year};
  int index = 0;
  while (true) {
    lcd.setCursor(0, 0); lcd.print("DateTime Setting");
    lcd.setCursor(2, 2); lcd.print(blinkStrDateTime(num, index));
    if ( isRight() ) {
      index++;
      if (index > 5) index = 0;
      delay(200);
    } else if (isLeft() ) {
      index--;
      if (index < 0) index = 5;
      delay(200);
    }
    else if ( isUp() ) {
      if (index == 0) {
        num[index]++;
        if (num[index] > 23) num[index] = 0;
      }
      else if (index == 1 || index == 2) {
        num[index]++;
        if (num[index] > 59) num[index] = 0;
      }
      else if (index == 3) {
        num[index]++;
        if (num[index] > 31) num[index] = 1;
      }
      else if (index == 4) {
        num[index]++;
        if (num[index] > 12) num[index] = 1;
      }
      else if (index == 5) {
        num[index]++;
        if (num[index] == 99) num[index] = 1;
      }
      lcd.setCursor(2, 2); lcd.print(noBlinkStrDateTime(num));
      delay(100);
    }
    else if (isDown()) {
      if (index == 0) {
        num[index]--;
        if (num[index] < 0) num[index] = 23;
      }
      else if (index == 1 || index == 2) {
        num[index]--;
        if (num[index] < 0) num[index] = 59;
      }
      else if (index == 3) {
        num[index]--;
        if (num[index] < 0) num[index] = 31;
      }
      else if (index == 4) {
        num[index]--;
        if (num[index] <= 0) num[index] = 12;
      }
      else if (index == 5) {
        num[index]--;
        if (num[index] < 0) num[index] = 99;
      }
      lcd.setCursor(2, 2); lcd.print(noBlinkStrDateTime(num));
      delay(100);
    }

    if (isSet()) {
      byte numByte[6] = {(byte)num[0] , (byte)num[1] , (byte)num[2] , (byte)num[3] , (byte)num[4] , (byte)num[5] };
      setDateDs1307(numByte[2] , numByte[1] , numByte[0] , 1, numByte[3] , numByte[4] , numByte[5]);
      state = MAIN;
      lcd.clear();
      delay(300);
      break;
    }
    if (isReset()) {
      lcd.clear();
      delay(300);
      break;
    }
  }
}



void SET_DATE_disp() {
  lcd.setCursor(0, 0);
  lcd.print("Date-Time Setting");
  lcd.setCursor(5, 2);
  lcd.print("Date Time");
  if ( isSet()) {
    delay(300);
    lcd.clear();
    Date_Set();

  }
  if ( isReset()) {
    delay(300);
    lcd.clear();
    state = MAIN;
  }
}


void setup() {
  initialize();

}
void loop() {
  // put your main code here, to run repeatedly:


  if (state == MAIN) {
    MAIN_disp();
  }
  else if (state == FAN) {
    FAN_disp();
  }
  else if (state == PUMP) {
    PUMP_disp();
  }
  else if (state == LED) {
    LED_disp();
  }
  else if (state == TIMER) {
    TIMER_disp();
  }
  else if (state == SET_DATE) {
    SET_DATE_disp();
  }

  if (isUp()) {
    delay(300);
    lcd.clear();

    state++;
    state = (state <= SET_DATE) ? state : 0;
  }
  if (isDown()) {
    delay(300);
    lcd.clear();

    state--;
    state = (state >= 0 ) ? state : SET_DATE;
  }
  if (isSet()) {
    delay(300);
  }
  relay_cOntrol();
}



void relay_cOntrol() {
  for (byte i =  0 ; i < 4 ; i++) {
    if (relay_status[i] == On) {
      if ( i == 0) digitalWrite(RELAY01 , LOW);
      
      if(digitalRead(A6) == HIGH){
        if ( i == 1) digitalWrite(RELAY02 , LOW);
      }
      else{
        if ( i == 1) digitalWrite(RELAY02 , HIGH); 
      }
      
      if ( i == 2) digitalWrite(RELAY03 , LOW);
    }
    else if (relay_status[i] == Off) {
      if ( i == 0) digitalWrite(RELAY01 , HIGH);
      
      if ( i == 1) digitalWrite(RELAY02 , HIGH);
      
      if ( i == 2) digitalWrite(RELAY03 , HIGH);
    }
    else if (relay_status[i] == Program) {
      if(i == 0 ) checkTimerFan();
      if(i == 1) checkTimerPump();
      if(i == 2) checkTimerLed();
    }
  }
}

void initialize() {
  Serial.begin(9600);
  lcd.begin();
  relayInit();
  buttOnInit();
  memoryInit();
}

void memoryInit() {

  int fanstate = EEPROM.read(FAN_STATE_ADDR);
  relay_status[0] = (fanstate <= Program && fanstate >= Off ) ? fanstate : Off;
  int pumpstate = EEPROM.read(PUMP_STATE_ADDR);
  relay_status[1] = (pumpstate <= Program && pumpstate >= Off ) ? pumpstate : Off;
  int ledstate = EEPROM.read(LED_STATE_ADDR);
  relay_status[2] = (ledstate <= Program && ledstate >= Off ) ? ledstate : Off;

  int fansize = EEPROM.read(FAN_TIMER_SIZE_ADDR);
  fan_timer_size = (fansize >= 0 && fansize < 8) ? fansize : 0;

  int pumpsize = EEPROM.read(PUMP_TIMER_SIZE_ADDR);
  pump_timer_size = (pumpsize >= 0 && pumpsize < 8) ? pumpsize : 0;

  int ledsize = EEPROM.read(LED_TIMER_SIZE_ADDR);
  led_timer_size = (ledsize >= 0 && ledsize < 8) ? ledsize : 0;
  int index = 0;
  for (int i = 100 ; i < 100 + (fan_timer_size * 4) ; i += 4) {
    fan_timer_list[index][0] = EEPROM.read(i);
    fan_timer_list[index][1] = EEPROM.read(i + 1);
    fan_timer_list[index][2] = EEPROM.read(i + 2);
    fan_timer_list[index][3] = EEPROM.read(i + 3);
    index++;
  }

  index = 0;
  for (int i = 132 ; i < 132 + (pump_timer_size * 4) ; i += 4) {
    pump_timer_list[index][0] = EEPROM.read(i);
    pump_timer_list[index][1] = EEPROM.read(i + 1);
    pump_timer_list[index][2] = EEPROM.read(i + 2);
    pump_timer_list[index][3] = EEPROM.read(i + 3);
    index++;
  }


  index = 0;
  for (int i = 192 ; i < 192 + (led_timer_size * 4) ; i += 4) {
    led_timer_list[index][0] = EEPROM.read(i);
    led_timer_list[index][1] = EEPROM.read(i + 1);
    led_timer_list[index][2] = EEPROM.read(i + 2);
    led_timer_list[index][3] = EEPROM.read(i + 3);
    index++;
  }


  String status_str = "[STATUS] FAN :" + String(relay_status[0]) + " PUMP:" + String(relay_status[1]) + " LED:" + String(relay_status[2]);
  String size_str = "[SIZE] FAN :" + String(fan_timer_size) + " PUMP:" + String(pump_timer_size) + " LED:" + String(led_timer_size);
  Serial.println(status_str);
  Serial.println(size_str);
}

void buttOnInit() {
  pinMode(SET , INPUT);
  pinMode(RESET , INPUT);
  pinMode(A0 , INPUT);
  pinMode(A1 , INPUT);
  pinMode(A2 , INPUT);
  pinMode(A3 , INPUT);
  pinMode(A4 , INPUT);
  pinMode(A5 , INPUT);

  pinMode(A6 , INPUT);
}
void relayInit() {

  pinMode( RELAY01 , OUTPUT);
  pinMode( RELAY02 , OUTPUT);
  pinMode( RELAY03 , OUTPUT);
  
  digitalWrite(RELAY01 , HIGH);
  digitalWrite(RELAY02 , HIGH);
  digitalWrite(RELAY03 , HIGH);

}



void TIMER_disp() {
  lcd.setCursor(0, 0); lcd.print("Timer Display");
  int8_t st = 0;
  while (true) {
    if (st == 0) {
      
      lcd.setCursor(0, 0); lcd.print("Timer Display");
      lcd.setCursor(0,2); lcd.print("<<");
      lcd.setCursor(5, 2); lcd.print("Fan Config");
      lcd.setCursor(18,2); lcd.print(">>");
    }
    else if (st == 1) {
      lcd.setCursor(0,2); lcd.print("<<");
      lcd.setCursor(0, 0); lcd.print("Timer Display");
      lcd.setCursor(5, 2); lcd.print("Pump Config");
      lcd.setCursor(18,2); lcd.print(">>");
    }
    else if (st == 2) {
      lcd.setCursor(0,2); lcd.print("<<");
      lcd.setCursor(0, 0); lcd.print("Timer Display");
      lcd.setCursor(5, 2); lcd.print("LED Config");
      lcd.setCursor(18,2); lcd.print(">>");
    }
    if (isLeft()) {
      delay(300);
      lcd.clear();
      st++;
      st = (st > 2) ? 0 : st;
    }
    if (isRight()) {
      delay(300);
      lcd.clear();
      st--;
      st = (st < 0) ? 2 : st;
    }
    if (isSet()) {
      delay(300);
      lcd.clear();
      TIMER_setting(st);
    }
    if (NextPage()) {
      break;
    }
  }
}

void TIMER_setting(int mode) {
  lcd.clear();
  int8_t st = 0;
  while (true) {
    lcd.setCursor(0, 0);
    if (mode == 0) lcd.print("FAN CONFIG");
    else if (mode == 1) lcd.print("PUMP CONFIG");
    else if (mode == 2) lcd.print("LED CONFIG");

    if (st == 0) {
      lcd.setCursor(0,2); lcd.print("<<");
      lcd.setCursor(5, 2); lcd.print("SHOW TIMER");
      lcd.setCursor(18,2); lcd.print(">>");
    }
    else if (st == 1) {
      lcd.setCursor(0,2); lcd.print("<<");
      lcd.setCursor(5, 2); lcd.print("ADD TIMER");
      lcd.setCursor(18,2); lcd.print(">>");
    }
    else if (st == 2) {
      lcd.setCursor(0,2); lcd.print("<<");
      lcd.setCursor(3, 2); lcd.print("DELETE TIMER");
      lcd.setCursor(18,2); lcd.print(">>");
    }

    if (isLeft()) {
      delay(300);
      lcd.clear();
      st++;
      st = (st > 2) ? 0 : st;
    }
    if (isRight()) {
      delay(300);
      lcd.clear();
      st--;
      st = (st < 0) ? 2 : st;
    }
    if (isSet()) {
      delay(300);
      lcd.clear();
      if (st == 1) {
        AddTimer(mode);
      }
      if (st == 0) {
        ShowTimer(mode);
      }
      if (st == 2) {
        DeleteTimer(mode);
      }
    }

    if (isReset()) {
      delay(300);
      lcd.clear();
      break;
    }
  }
}

void DeleteTimer(int mode) {

  lcd.clear();
  delay(500);
  if (mode == 0) {
    fan_timer_size = 0;
    EEPROM.write(FAN_TIMER_SIZE_ADDR , 0);
  }
  else if (mode == 1) {
    pump_timer_size = 0;
    EEPROM.write(PUMP_TIMER_SIZE_ADDR , 0);
  }
  else if (mode == 2) {
    led_timer_size = 0;
    EEPROM.write(LED_TIMER_SIZE_ADDR , 0);
  }
  lcd.setCursor(5, 2); lcd.print("removing...");
  
  delay(500);
  lcd.clear();
}

void AddTimer(byte mode) {
  int8_t timer[4] = { 0 , 0 , 0 , 0};
  int8_t index = 0;
  int8_t timer_list[8][4];
  int8_t timer_size = 0;
  byte i , j;

  if (mode == 0) {
    timer_size = fan_timer_size;
    for ( i = 0; i < timer_size ; i++) {
      for ( j = 0; j < 4; j++) {
        timer_list[i][j] = fan_timer_list[i][j];
      }
    }
  }

  if (mode == 1) {
    timer_size = pump_timer_size;
    for ( i = 0; i < timer_size ; i++) {
      for ( j = 0; j < 4; j++) {
        timer_list[i][j] = pump_timer_list[i][j];
      }
    }
  }

  if (mode == 2) {
    timer_size = led_timer_size;
    for ( i = 0; i < timer_size ; i++) {
      for ( j = 0; j < 4; j++) {
        timer_list[i][j] = led_timer_list[i][j];
      }
    }
  }

  if(timer_size > 0){
    timer[0] = timer_list[timer_size-1][2];
    timer[1] = timer_list[timer_size-1][3] +1;
    index=2;
  }

  lcd.setCursor(0, 0);
  lcd.print("Add Timer");
  if (mode == 0) {
    lcd.print(" FAN");
  }
  if (mode == 1) {
    lcd.print(" PUMP");
  }
  if (mode == 2) {
    lcd.print(" LED");
  }
  while (true) {
    lcd.setCursor(0, 0);
    lcd.print("Add Timer");
    lcd.setCursor(2, 2);
    lcd.print(blinkStrTimer(timer , index));
    if (isRight()) {
      delay(300);
      index++;
      index = (index > 3) ? 0 : index;
    }
    else if (isLeft()) {
      delay(300);
      index--;
      index = (index < 0) ? 3 : index;
    }
    else if (isUp()) {
      delay(100);
      if (index == 0 || index == 2) {
        timer[index]++;
        timer[index] = (timer[index] > 23) ? 0 : timer[index];
      }
      else if (index == 1 || index == 3) {
        timer[index]++;
        timer[index] = (timer[index] > 59) ? 0 : timer[index];
      }
      lcd.setCursor(2, 2);
      lcd.print(noBlinkStrTimer(timer ));
    }
    else if (isDown()) {
      delay(100);
      if (index == 0 || index == 2) {
        timer[index]--;
        timer[index] = (timer[index] < 0) ? 23 : timer[index];
      }
      else if (index == 1 || index == 3) {
        timer[index]--;
        timer[index] = (timer[index] < 0) ? 59 : timer[index];
      }
      lcd.setCursor(2, 2);
      lcd.print(noBlinkStrTimer(timer ));
    }

    else if (isSet()) {
      delay(300);
      if (CheckTimerAndSave(timer , mode)) {
        for (i = 0 ; i < fan_timer_size ; i++) {
          for (j = 0 ; j < 4 ; j++) {
            Serial.print(" " + String(fan_timer_list[i][j]));
          }
          Serial.println();
        }
        Serial.println();
        for (i = 0 ; i < pump_timer_size ; i++) {
          for (j = 0 ; j < 4 ; j++) {
            Serial.print(" " + String(pump_timer_list[i][j]));
          }
          Serial.println();
        }
        Serial.println();
        for (i = 0 ; i < led_timer_size ; i++) {
          for (j = 0 ; j < 4 ; j++) {
            Serial.print(" " + String(led_timer_list[i][j]));
          }
          Serial.println();
        }
        Serial.println();
        lcd.clear();
        lcd.setCursor(5, 2); lcd.print("Add Success");
        delay(1000);
        break;
      }
      else {
        Serial.println("wrong timer format");
        lcd.clear();
        lcd.setCursor(5, 2); lcd.print("Wrong Format");
        delay(1000);
        lcd.clear();
      }
    }

    if (isReset()) {
      delay(300);
      lcd.clear();
      break;
    }
  }
  lcd.clear();
}



void MAIN_disp() {
  getDateDs1307();
  displayTime();

  lcd.setCursor(0, 1); lcd.print("FAN: " + getStatusString(relay_status[0]));
  if(digitalRead(A6) == HIGH){
    lcd.setCursor(0, 2); lcd.print("PUMP: " + getStatusString(relay_status[1]));
  }
  else{
    lcd.setCursor(0, 2); lcd.print("PUMP: LOW      ");
  }
    
  lcd.setCursor(0, 3); lcd.print("LED: " + getStatusString(relay_status[2]));
}

void FAN_disp() {
  int st = relay_status[0];
  while (true) {
    lcd.setCursor(0, 0);
    lcd.print("Fan Display");
    if (st == On) {
      lcd.setCursor(0,2); lcd.print("<<");
      lcd.setCursor(8, 2);
      lcd.print(getStatusString(On));
      lcd.setCursor(18,2); lcd.print(">>");
      
    }
    else if (st == Off ) {
      lcd.setCursor(0,2); lcd.print("<<");
      lcd.setCursor(8, 2);
      lcd.print(getStatusString(Off));
      lcd.setCursor(18,2); lcd.print(">>");
    }
    else if (st == Program  ) {
      lcd.setCursor(0,2); lcd.print("<<");
      lcd.setCursor(6, 2);
      lcd.print(getStatusString(Program));
      lcd.setCursor(18,2); lcd.print(">>");
    }
    if (isLeft()) {
      delay(300);
      st++;
      st = (st <= Program) ? st : Off;
      lcd.clear();
    }
    if (isRight()) {
      delay(300);
      st--;
      st = (st >= Off) ? st : Program;
      lcd.clear();
    }
    if (isSet()) {
      delay(300);
      lcd.clear();
      relay_status[0] = st;
      EEPROM.write(FAN_STATE_ADDR , st);
      state = MAIN;
      break;
    }

    if (NextPage()) {
      break;
    }
  }

}

void PUMP_disp() {
  int st = relay_status[1];
  while (true) {
    lcd.setCursor(0, 0);
    lcd.print("Pump Display");
    if (st == On) {
      lcd.setCursor(0,2); lcd.print("<<");
      lcd.setCursor(8, 2);
      lcd.print(getStatusString(On));
      lcd.setCursor(18,2); lcd.print(">>");
      
    }
    else if (st == Off ) {
      lcd.setCursor(0,2); lcd.print("<<");
      lcd.setCursor(8, 2);
      lcd.print(getStatusString(Off));
      lcd.setCursor(18,2); lcd.print(">>");
    }
    else if (st == Program  ) {
      lcd.setCursor(0,2); lcd.print("<<");
      lcd.setCursor(6, 2);
      lcd.print(getStatusString(Program));
      lcd.setCursor(18,2); lcd.print(">>");
    }
    if (isLeft()) {
      delay(300);
      st++;
      st = (st <= Program) ? st : Off;
      lcd.clear();
    }
    if (isRight()) {
      delay(300);
      st--;
      st = (st >= Off) ? st : Program;
      lcd.clear();
    }
    if (isSet()) {
      delay(300);
      lcd.clear();
      relay_status[1] = st;
      EEPROM.write(PUMP_STATE_ADDR , st);
      state = MAIN;
      break;
    }
    if (NextPage()) {
      break;
    }
  }
}

void LED_disp() {
  int st = relay_status[2];
  while (true) {
    lcd.setCursor(0, 0);
    lcd.print("LED Display");
    if (st == On) {
      lcd.setCursor(0,2); lcd.print("<<");
      lcd.setCursor(8, 2);
      lcd.print(getStatusString(On));
      lcd.setCursor(18,2); lcd.print(">>");
      
    }
    else if (st == Off ) {
      lcd.setCursor(0,2); lcd.print("<<");
      lcd.setCursor(8, 2);
      lcd.print(getStatusString(Off));
      lcd.setCursor(18,2); lcd.print(">>");
    }
    else if (st == Program  ) {
      lcd.setCursor(0,2); lcd.print("<<");
      lcd.setCursor(6, 2);
      lcd.print(getStatusString(Program));
      lcd.setCursor(18,2); lcd.print(">>");
    }
    if (isLeft()) {
      delay(300);
      st++;
      st = (st <= Program) ? st : Off;
      lcd.clear();
    }
    if (isRight()) {
      delay(300);
      st--;
      st = (st >= Off) ? st : Program;
      lcd.clear();
    }
    if (isSet()) {
      delay(300);
      lcd.clear();
      relay_status[2] = st;
      EEPROM.write(LED_STATE_ADDR , st);
      state = MAIN;
      break;
    }
    if (NextPage()) {
      break;
    }
  }
}


void displayTime() {
  lcd.setCursor(0, 0);
  lcd.print(hour, DEC); lcd.print(":");
  if (minute < 10) lcd.print("0");
  lcd.print(minute, DEC); lcd.print(":");
  if (second < 10) lcd.print("0");
  lcd.print(second, DEC); lcd.print(" ");
  lcd.print(dayOfMonth, DEC); lcd.print("/"); lcd.print(month, DEC); lcd.print("/"); lcd.print(year, DEC);
}




byte decToBcd(byte val) {
  return ( (val / 10 * 16) + (val % 10) );
}
byte bcdToDec(byte val) {
  return ( (val / 16 * 10) + (val % 16) );
}

void spite_logo() {
  lcd.setCursor(0, 0); lcd.print("iNtel-Agro ");
  lcd.setCursor(0, 2); lcd.print(" Smart cOntrol V1.0");
}

bool isLeft() {
  return (digitalRead(A2) == LOW) ? true : false;
}
bool isRight() {
  return (digitalRead(A3) == LOW) ? true : false;
}
bool isUp() {
  return (digitalRead(A0) == LOW) ? true : false;
}
bool isDown() {
  return (digitalRead(A1) == LOW) ? true : false;
}
bool isSet() {
  return (digitalRead(A4) == LOW) ? true : false;
}
bool isReset() {
  return (digitalRead(A5) == LOW) ? true : false;
}

String getStatusString(int relay_status) {
  if ( relay_status == On) return "On";
  else if (relay_status == Off) return "Off";
  else if (relay_status == Program) return "Program";
}

bool NextPage() {
  if (isUp()) {
    delay(300);
    lcd.clear();
    state++;
    state = (state < SET_DATE) ? state : 0;
    return true;
  }
  if (isDown()) {
    delay(300);
    lcd.clear();
    state--;
    state = (state >= 0 ) ? state : 3;
    return true;
  }
  if (isReset()) {
    delay(300);
    lcd.clear();
    Serial.println("RESET");
    state = MAIN;
    return true;
  }
  return false;
}
void getDateDs1307()
{
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(decToBcd(0));
  Wire.endTransmission();
  Wire.requestFrom(DS1307_I2C_ADDRESS, 7);
  second = bcdToDec(Wire.read() & 0x7f);
  minute = bcdToDec(Wire.read());
  hour = bcdToDec(Wire.read() & 0x3f); // Need to change this if 12 hour am/pm
  dayOfWeek = bcdToDec(Wire.read());
  dayOfMonth = bcdToDec(Wire.read());
  month = bcdToDec(Wire.read());
  year = bcdToDec(Wire.read());
}

String blinkStrTimer(byte timer[], byte i) {
  byte index = 0;
  long currentTime = millis();
  long df = currentTime - blinkTime;
  byte timer_set[4] = {timer[0] , timer[1], timer[2], timer[3] };
  String strNum[4];
  for (int i = 0 ; i < 4 ; i++) {
    if (timer_set[i] < 10)
      strNum[i] = "0" + String(timer_set[i]);
    else
      strNum[i] = String(timer_set[i]);
  }
  if      ( i == 0 ) index = 0;
  else if ( i == 1 ) index = 3;
  else if ( i == 2 ) index = 8;
  else if ( i == 3 ) index = 11;
  String hole = strNum[0] + ":" + strNum[1] + " - " + strNum[2] + ":" + strNum[3];
  if (df < 300) {
    hole[index] = '_';
    hole[index + 1] = '_';
    return hole;
  }
  else if (df > 300 && df < 600) {
    return hole;
  }
  else {
    blinkTime = currentTime;
    hole[index] = '_';
    hole[index + 1] = '_';
    return hole;
  }
}

String noBlinkStrTimer(byte timer[]) {
  byte timer_set[4] = {timer[0] , timer[1], timer[2], timer[3] };
  String strNum[4];
  for (int i = 0 ; i < 4 ; i++) {
    if (timer_set[i] < 10)
      strNum[i] = "0" + String(timer_set[i]);
    else
      strNum[i] = String(timer_set[i]);
  }
  return strNum[0] + ":" + strNum[1] + " - " + strNum[2] + ":" + strNum[3];
}

bool CheckTimerAndSave(byte *timer, byte mode) {

  if (mode == 0) {
    int startTimeSum = timer[0] * 100 + timer[1];
    int stopTimeSum = timer[2] * 100 + timer[3];
    if (startTimeSum >= stopTimeSum ) {
      return false;
    }

    for (int i = 0 ; i < fan_timer_size ; i++) {
      int timeSum =  fan_timer_list[i][2] * 100 + fan_timer_list[i][3];
      if (startTimeSum <= timeSum) {
        return false;
      }
    }



    fan_timer_list[fan_timer_size][0] = timer[0];
    fan_timer_list[fan_timer_size][1] = timer[1];
    fan_timer_list[fan_timer_size][2] = timer[2];
    fan_timer_list[fan_timer_size][3] = timer[3];

    int start_addr = 100 + ((fan_timer_size) * 4);
    EEPROM.write(start_addr , fan_timer_list[fan_timer_size][0]);
    EEPROM.write(start_addr + 1 , fan_timer_list[fan_timer_size][1]);
    EEPROM.write(start_addr + 2 , fan_timer_list[fan_timer_size][2]);
    EEPROM.write(start_addr + 3 , fan_timer_list[fan_timer_size][3]);


    fan_timer_size++;
    EEPROM.write(FAN_TIMER_SIZE_ADDR , fan_timer_size);

    return true;
  }

  else if (mode == 1) {
    int startTimeSum = timer[0] * 100 + timer[1];
    int stopTimeSum = timer[2] * 100 + timer[3];
    if (startTimeSum >= stopTimeSum ) {
      return false;
    }

    for (int i = 0 ; i < pump_timer_size ; i++) {
      int timeSum =  pump_timer_list[i][2] * 100 + pump_timer_list[i][3];
      if (startTimeSum <= timeSum) {
        return false;
      }
    }

    pump_timer_list[pump_timer_size][0] = timer[0];
    pump_timer_list[pump_timer_size][1] = timer[1];
    pump_timer_list[pump_timer_size][2] = timer[2];
    pump_timer_list[pump_timer_size][3] = timer[3];


    int start_addr = 132 + ((pump_timer_size) * 4);
    EEPROM.write(start_addr , pump_timer_list[pump_timer_size][0]);
    EEPROM.write(start_addr + 1 , pump_timer_list[pump_timer_size][1]);
    EEPROM.write(start_addr + 2 , pump_timer_list[pump_timer_size][2]);
    EEPROM.write(start_addr + 3 , pump_timer_list[pump_timer_size][3]);


    pump_timer_size++;
    EEPROM.write(PUMP_TIMER_SIZE_ADDR , pump_timer_size);

    return true;
  }
  else if (mode == 2) {
    int startTimeSum = timer[0] * 100 + timer[1];
    int stopTimeSum = timer[2] * 100 + timer[3];
    if (startTimeSum >= stopTimeSum ) {
      return false;
    }

    for (int i = 0 ; i < led_timer_size ; i++) {
      int timeSum =  led_timer_list[i][2] * 100 + led_timer_list[i][3];
      if (startTimeSum <= timeSum) {
        return false;
      }
    }

    led_timer_list[led_timer_size][0] = timer[0];
    led_timer_list[led_timer_size][1] = timer[1];
    led_timer_list[led_timer_size][2] = timer[2];
    led_timer_list[led_timer_size][3] = timer[3];


    int start_addr = 192 + ((led_timer_size) * 4);
    EEPROM.write(start_addr , led_timer_list[led_timer_size][0]);
    EEPROM.write(start_addr + 1 , led_timer_list[led_timer_size][1]);
    EEPROM.write(start_addr + 2 , led_timer_list[led_timer_size][2]);
    EEPROM.write(start_addr + 3 , led_timer_list[led_timer_size][3]);


    led_timer_size++;
    EEPROM.write(LED_TIMER_SIZE_ADDR , led_timer_size);

    return true;
  }
}

void ShowTimer(uint8_t mode ) {
  byte timer_list[8][4];
  byte timer_size = 0;
  int currentPage = 0;
  byte i, j;
  if (mode == 0) {
    timer_size = fan_timer_size;
  }
  else if (mode == 1) {
    timer_size = pump_timer_size;
  }
  else if (mode == 2) {
    timer_size = led_timer_size;
  }
  while (true) {
    int l = 0;
    if (isReset() ) {
      lcd.clear();
      delay(300);
      break;
    }
    if (timer_size == 0) {
      lcd.setCursor(5, 2); lcd.print("NO DATA");

      continue;
    }
    for (int i = currentPage * 3 ; i < currentPage * 3 + 3 && i < timer_size ; i++) {
      byte *timer = timer_list[i];
      if (mode == 0) {
        lcd.setCursor(2, l); lcd.print(String(i + 1) + "."); lcd.print(noBlinkStrTimer(fan_timer_list[i]));
        //        Serial.println( String(fan_timer_list[i][0]) + ":" + String(fan_timer_list[i][1])
        //                        + "-" + String( fan_timer_list[i][2]) + ":" + String(fan_timer_list[i][3]) );
      }
      else if (mode == 1) {
        lcd.setCursor(2, l); lcd.print(String(i + 1) + "."); lcd.print(noBlinkStrTimer(pump_timer_list[i]));

      }
      else if (mode == 2) {
        lcd.setCursor(2, l); lcd.print(String(i + 1) + "."); lcd.print(noBlinkStrTimer(led_timer_list[i]));
      }
      l++;
    }

    if (currentPage == 0) {
      if (timer_size > 3) {
        lcd.setCursor(15, 3); lcd.print("next>");
      }
    }
    else if (currentPage > 0 ) {
      if (currentPage * 3 + 3 >= timer_size ) {
        lcd.setCursor(15, 3); lcd.print("next>");
        lcd.setCursor(0, 3); lcd.print("<back");
      }
    }

    if (isRight()) {
      lcd.clear();
      delay(300);

      currentPage++;
      if (currentPage * 3 > timer_size) {
        currentPage = 0;
      }
    }
    else if (isLeft()) {
      lcd.clear();
      delay(300);

      currentPage--;
      if (currentPage < 0 ) currentPage = 0;
    }
  }
}

void checkTimerFan() {
  int currentMin = hour * 60 + minute;
  bool flag = false;
  int index = 0;
  for (int i = 0 ; i < fan_timer_size ; i++) {
    int startTimerMin = fan_timer_list[i][0] * 60 + fan_timer_list[i][1];
    int stopTimerMin =  fan_timer_list[i][2] * 60 + fan_timer_list[i][3];
    if (currentMin >= startTimerMin && currentMin < stopTimerMin) {
      flag = true;
      break;
    }
    index++;
  }
  if (flag) {
    //index--;
    digitalWrite(RELAY01 , LOW);
//    Serial.println("FAN : " + String(fan_timer_list[index][0]) + ":" + String(fan_timer_list[index][1]) + "-" +
//                   String(fan_timer_list[index][2] ) + ":" + String(fan_timer_list[index][3]));
  }
  else {
    digitalWrite(RELAY01 , HIGH);
  }
}

void checkTimerPump() {
  int currentMin = hour * 60 + minute;
  int index = 0;
  bool flag = false;
  for (int i = 0 ; i < pump_timer_size ; i++) {
    int startTimerMin = pump_timer_list[i][0] * 60 + pump_timer_list[i][1];
    int stopTimerMin =  pump_timer_list[i][2] * 60 + pump_timer_list[i][3];
    if (currentMin >= startTimerMin && currentMin < stopTimerMin) {
      flag = true;
      break;
    }
    index++;
  }
  if (flag) {
    //index--;
    digitalWrite(RELAY02 , LOW);

//    Serial.println("Pump : " + String(pump_timer_list[index][0]) + ":" + String(pump_timer_list[index][1]) + "-" +
//                   String(pump_timer_list[index][2] ) + ":" + String(pump_timer_list[index][3]));
  }
  else {
    digitalWrite(RELAY02 , HIGH);
  }
}

void checkTimerLed() {
  int currentMin = hour * 60 + minute;
  bool flag = false;
  int index = 0;
  for (int i = 0 ; i < led_timer_size ; i++) {
    int startTimerMin = led_timer_list[i][0] * 60 + led_timer_list[i][1];
    int stopTimerMin =  led_timer_list[i][2] * 60 + led_timer_list[i][3];
    if (currentMin >= startTimerMin && currentMin < stopTimerMin) {
      flag = true;
      break;
    }
    index++;
  }
  if (flag) {
    //index--;
    digitalWrite(RELAY03 , LOW);
//    Serial.println("LED : " + String(led_timer_list[index][0]) + ":" + String(led_timer_list[index][1]) + "-" +
//                   String(led_timer_list[index][2] ) + ":" + String(led_timer_list[index][3]) );
  }
  else {
    digitalWrite(RELAY03 , HIGH);
  }
}


String blinkStrDateTime(int num[] , int i) {
  int index = 0;
  long currentTime = millis();
  long df = currentTime - blinkTime;
  String strNum[6] = { String(num[0]) , String(num[1]) , String(num[2]) , String(num[3]) , String(num[4]) , String(num[5]) };

  for (int i = 0 ; i < 6; i++)
    if (num[i] < 10)
      strNum[i] = "0" + String(num[i]);


  if (i == 0) index = 0;
  else if (i == 1) index = 3;
  else if (i == 2) index = 6;
  else if (i == 3) index = 9;
  else if (i == 4) index = 12;
  else if (i == 5) index = 15;

  if (df < 300) {
    String hole = strNum[0] + ":" + strNum[1] + ":" + strNum[2];
    hole += " " + strNum[3] + "/" + strNum[4] + "/" + strNum[5];
    hole[index] = '_';
    hole[index + 1] = '_';
    return hole;
  }
  else if (df > 300 && df < 600) {
    String str = strNum[0] + ":" + strNum[1] + ":" + strNum[2];
    str += " " + strNum[3] + "/" + strNum[4] + "/" + strNum[5];
    return str;
  }
  else {
    blinkTime = currentTime;
    String hole = strNum[0] + ":" + strNum[1] + ":" + strNum[2];
    hole += " " + strNum[3] + "/" + strNum[4] + "/" + strNum[5];
    hole[index] = '_';
    hole[index + 1] = '_';

    return hole;
  }
}
String noBlinkStrDateTime(int num[]) {
  String strNum[6] = { String(num[0]) , String(num[1]) , String(num[2]) , String(num[3]) , String(num[4]) , String(num[5]) };
  for (int i = 0 ; i < 6; i++)
    if (num[i] < 10)
      strNum[i] = "0" + String(num[i]);
  String str = strNum[0] + ":" + strNum[1] + ":" + strNum[2];
  str += " " + strNum[3] + "/" + strNum[4] + "/" + strNum[5];
  return str;
}

void setDateDs1307(byte s , byte m , byte h , byte dow , byte dom , byte mo , byte y)
{
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(decToBcd(0));
  Wire.write(decToBcd(s)); // 0 to bit 7 starts the clock
  Wire.write(decToBcd(m));
  Wire.write(decToBcd(h)); // If you want 12 hour am/pm you need to set
  // bit 6 (also need to change readDateDs1307)
  Wire.write(decToBcd(dow));
  Wire.write(decToBcd(dom));
  Wire.write(decToBcd(mo));
  Wire.write(decToBcd(y));
  Wire.endTransmission();
}


