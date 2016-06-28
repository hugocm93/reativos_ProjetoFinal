#include <LiquidCrystal.h>

#define N_ROWS 4
#define N_COLUMNS 3
#define DEBOUNCE 20
#define TEMP_BUTTON '*'
#define TIME_BUTTON '#'

#define SENSOR A1
#define RELAY A2

/*Primeiro: intervalo em milisegundos entre as checagens de temperatura.
  Segundo: segundos do timer. Terceiro: Temperatura limiar.*/
#define INTERVAL 10000
#define TIMEOUT  60000
#define THRESHOLD 28

enum Mode {
  Idle,
  Temperature,
  Timer,
  Setup,
  On
};

LiquidCrystal lcd(13, 12, 11, 10, 9, 8);

String lcdText[2] = {{""}, {""}};

unsigned long interval = INTERVAL, timeout = TIMEOUT, threshold = THRESHOLD;

/*Guarda a última vez que foi feita a atualização da temperatura*/
unsigned long lastUpdateTime;

/*Guarda o tempo que o timer começou a contar. timerStamp == 0
  significa que ele não foi inicializado*/
unsigned long timerStamp;

/*Guarda a última leitura do sensor de temperatura*/
int oldSensorValue;

Mode option = Idle;

bool serial = false;

const int rowPins[] = { 7, 2, 3, 5};
const int columnPins[] = { 6, A0, 4};
const char blank = 0;
const char numberpad[N_ROWS][N_COLUMNS] =
{
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

/*Função que converte a temperatura para Celsius*/
float tempInCelsius(int sensorValue);

/*Função que verifica se algo foi recebido no Serial*/
void receiveData();

char pressedKey();

void firstRun();

unsigned int getParameter(char message[]);

unsigned int stringToUInt(char number[], int numberLength);

void clearAllStates();

void set(Mode mode);

int printInLcd(String message, int line);

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);

  pinMode(SENSOR, INPUT);
  pinMode(RELAY, OUTPUT);

  for (int r = 0; r < N_ROWS; r++) {
    pinMode(rowPins[r], INPUT);
    digitalWrite(rowPins[r], HIGH); //enable pull-up
  }

  for (int c = 0; c < N_COLUMNS; c++) {
    pinMode(columnPins[c], OUTPUT);
    digitalWrite(columnPins[c], HIGH);
  }

  set(Temperature);
}

void loop() {
  /*Verifica a temperatura de tempos em tempos*/
  if (millis() >= lastUpdateTime + interval) {
    oldSensorValue = analogRead(SENSOR);
    lastUpdateTime = millis();
  }
  
  receiveData();

  /*Tratamento e comportamento de cada modo*/
  switch (option) {
    case Temperature:
//      lcd.clear();
//      lcd.setCursor(0, 0);
//      lcd.print("Temperature ");
//      lcd.setCursor(0, 1);
//      lcd.print(tempInCelsius(oldSensorValue));
//      lcd.print(" C");

      printInLcd("Temperature ", 0);
      printInLcd(String(tempInCelsius(oldSensorValue)) + " C", 1);

      Serial.print("Temperature ");
      Serial.println(String(tempInCelsius(oldSensorValue)) + " C");

      /*Verifica se a temperatura limiar foi atingida e faz a ação em caso afirmativo*/
      if (tempInCelsius(oldSensorValue) > (float)threshold) {
        digitalWrite(RELAY, HIGH);
      }
      else {
        if(digitalRead(RELAY)){
          break;
        }
        digitalWrite(RELAY, LOW);
      }
      break;

    case Timer:
//      lcd.clear();
//      lcd.setCursor(0, 0);
//      lcd.print("Timer ");
//      lcd.setCursor(0, 1);
//      lcd.print(millis() - timerStamp);

      printInLcd("Timer ",0);
      printInLcd(String((millis() - timerStamp)/1000) + " s", 1);

      Serial.print("Timer ");
      Serial.println(millis() - timerStamp);

      /*Verifica se o timer foi atingido e faz a ação em caso afirmativo*/
      if (millis() - timerStamp > timeout) {
        digitalWrite(RELAY, LOW);
        option = Idle;
      }
      else {
        digitalWrite(RELAY, HIGH);
      }
      break;

    case On:
        digitalWrite(RELAY, HIGH);
      break;
      

    case Idle:
//      lcd.clear();
//      lcd.setCursor(0, 0);
//      lcd.print("Idle");

      printInLcd("Idle ",0);
      printInLcd(" ",1);

      timerStamp = 0;
  }

}

float tempInCelsius(int sensorValue) {
  float mV = (sensorValue) * (5000 / 1024.0);
  return (mV) / 10.;
}

void receiveData() {
  char key;

  if (Serial.available()) {
    serial = true;
    String inputString = "";

    /*Lê do serial se disponível e constroi a String*/
    while (Serial.available()) {
      key = (char)Serial.read();
      if (key == '0' || key == '1' || key == TIME_BUTTON || key == TEMP_BUTTON) {
        break;
      }
    }
    while (Serial.available()) {
      Serial.read() ;
    }
  }
  else {
    serial = false;
    key = pressedKey();
  }

  switch (key) {
    case TIME_BUTTON:
      set(Timer);
      break;

    case TEMP_BUTTON:
      set(Temperature);
      break;

    case '0':
      set(Idle);
      break;

    case '1':
      set(Setup);
      break;

    case '2':
      set(On);
      break;
  }
}

char pressedKey() {
  char key = blank;
  boolean pressed = false;

  for (int c = 0; c < N_COLUMNS && !pressed; c++) {
    digitalWrite(columnPins[c], LOW);
    for (int r = 0; r < N_ROWS; r++) {
      if (digitalRead(rowPins[r]) == LOW) {
        delay(DEBOUNCE); //delay to get rid of noises
      }
      if (digitalRead(rowPins[r]) == LOW) {
        while (digitalRead(rowPins[r]) != HIGH);
        key = numberpad[r][c];
        pressed = true;
        break;
      }
    }
    digitalWrite(columnPins[c], HIGH);
  }
  return key;
}

void firstRun() {
  lcd.clear();
  threshold = getParameter("Temp. in C:");
  timeout =  getParameter("Timer in ms:");
  interval = getParameter("Freq. in ms:");
}

unsigned int getParameter(char message[]) {
  lcd.setCursor(0, 0);
  lcd.print(message);
  Serial.println(message);
  if (!serial) {
    char key = pressedKey();
    int numberLength = 0;
    char number[16];
    do {
      if (key != blank && key != TIME_BUTTON) {
        number[numberLength] = key;
        lcd.setCursor(numberLength++, 1);
        lcd.print(key);
      }
      else if (key == TIME_BUTTON && numberLength > 0) {
        number[--numberLength] = ' ';
        lcd.setCursor(numberLength, 1);
        lcd.print(' ');
      }
      key = pressedKey();
    } while (key != TEMP_BUTTON && numberLength < 16);

    lcd.clear();
    return stringToUInt(number, numberLength);
  }
  else {
    int i = 0;
    char inputString[16];
    while (Serial.available()) {
      Serial.read() ;
    }
    while (!Serial.available());
    if (Serial.available()) {
      int temp = Serial.parseInt();
      Serial.println(temp);
      return (unsigned int)temp;
    }
  }
}


unsigned int stringToUInt(char number[], int numberLength) {
  unsigned int digit = 0;
  numberLength--;

  for (int i = numberLength; i >= 0; i--) {
    digit += (number[i] - 48) * round(pow(10., numberLength - i));
  }
  return digit;
}

void clearAllStates() {
  oldSensorValue = analogRead(SENSOR);
  lastUpdateTime = millis();
  option = Idle;
  timerStamp = 0;
  digitalWrite(RELAY, LOW);
}


void set(Mode mode) {
  clearAllStates();

  switch (mode) {
    case Temperature:
      option = Temperature;
      break;

    case Timer:
      option = Timer;
      if (timerStamp == 0) {
        timerStamp = millis();
      }
      break;

    case Idle:
      option = Idle;
      break;

    case Setup:
      firstRun();
      break;

    case On:
      option = On;
      break;
  }
}

int printInLcd(String message, int line){
  if (message == lcdText[line]) {
    return 0;
  }
  lcdText[line] = message;
  lcd.setCursor(0, line);
  lcd.print("                ");
  lcd.setCursor(0, line);
  lcd.print(lcdText[line]);

  delay(500);
  return 1;
}





