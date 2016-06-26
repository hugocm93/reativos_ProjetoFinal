/*Led branco que está representando o relay*/
#define LED 13

/*Led RGB que sinaliza em qual modo está o programa:
  modo temperatura, modo timer ou modo idle.*/
#define RGB_LED_R 12
#define RGB_LED_G 11
#define RGB_LED_B 10

/*Sensor que lê a temperatura*/
#define SENSOR A0

/*Botão que coloca o programa em modo temperatura, timer ou idle.*/
#define BUTTON1 2

/*Primeiro: intervalo em milisegundos entre as checagens de temperatura.
  Segundo: segundos do timer. Terceiro: Temperatura limiar.*/
#define INTERVAL 3000
#define TIMEOUT  10000
#define THRESHOLD 27.0

unsigned long interval = INTERVAL, timeout = TIMEOUT;
float threshold = THRESHOLD;

/*Guarda a última vez que foi feita a atualização da temperatura*/
unsigned long lastUpdateTime;

/*Guarda o tempo que o timer começou a contar. timerStamp == 0
  significa que ele não foi inicializado*/
unsigned long timerStamp;

/*Guarda a última leitura do sensor de temperatura*/
int oldSensorValue;

/*Opção do modo Temperatura ou modo Timer. option == -1 significa que
  ele não foi inicializado*/
int option;

/*Último estado do botão*/
bool lastButtonState;

/*Modo corrente do programa. Dois modos possíveis*/
bool currentMode;

/*Limpa todos os estados do programa*/
void clearAllStates();

/*Acende o led RGB de acordo com a cor desejada("red", "green" ou "blue")*/
void rgb(char* color);

/*Função que é chamada quando o botão tem o seu estado modificado*/
void buttonChanged(bool state);

/*Função que converte a temperatura para Celsius*/
float tempInCelsius(int sensorValue);

/*Função que verifica se algo foi recebido no Serial*/
void receiveData();

void setup() {
  Serial.begin(9600);

  /*Define cada porta*/
  pinMode(LED, OUTPUT);
  pinMode(RGB_LED_R, OUTPUT);
  pinMode(RGB_LED_G, OUTPUT);
  pinMode(RGB_LED_B, OUTPUT);
  pinMode(SENSOR, INPUT);
  pinMode(BUTTON1, INPUT);

  clearAllStates();
}

void loop() {
  /*Verifica a temperatura de tempos em tempos*/
  if (millis() >= lastUpdateTime + interval) {
    oldSensorValue = analogRead(SENSOR);
    lastUpdateTime = millis();
  }

  /*Verifica se o botão mudou de estado.*/
  bool buttonRead = digitalRead(BUTTON1);
  if (buttonRead != lastButtonState) {
    lastButtonState = buttonRead;
    buttonChanged(buttonRead);
  }

  /*Recebe dados vindos da comunicação serial*/
  receiveData();

  /*Tratamento e comportamento de cada modo*/
  switch (option) {
    case 1:
      rgb("green");
      Serial.print("Temperature ");
      Serial.println(tempInCelsius(oldSensorValue));

      /*Verifica se a temperatura limiar foi atingida e faz a ação em caso afirmativo*/
      if (tempInCelsius(oldSensorValue) > threshold) {
        digitalWrite(LED, HIGH);
      }
      else {
        digitalWrite(LED, LOW);
      }
      break;

    case 2:
      rgb("blue");
      Serial.print("Timer ");
      Serial.println(millis() - timerStamp);

      /*Verifica se o timer foi atingido e faz a ação em caso afirmativo*/
      if (millis() - timerStamp > timeout) {
        digitalWrite(LED, LOW);
        option = -1;
      }
      else {
        digitalWrite(LED, HIGH);
      }
      break;

    default:
      rgb("red");
      timerStamp = 0;
  }
}

void rgb(char* color) {
  if (!strcmp(color, "green")) {
    digitalWrite(RGB_LED_R, LOW);
    digitalWrite(RGB_LED_G, HIGH);
    digitalWrite(RGB_LED_B, LOW);
  }
  else if (!strcmp(color, "blue")) {
    digitalWrite(RGB_LED_R, LOW);
    digitalWrite(RGB_LED_G, LOW);
    digitalWrite(RGB_LED_B, HIGH);
  }
  else if (!strcmp(color, "red")) {
    digitalWrite(RGB_LED_R, HIGH);
    digitalWrite(RGB_LED_G, LOW);
    digitalWrite(RGB_LED_B, LOW);
  }
}

void buttonChanged(bool state) {
  if (state) {
    if (currentMode) {
      option = 1;
      timerStamp = 0;
    }
    else {
      option = 2;
      if (timerStamp == 0) {
        timerStamp = millis();
      }
    }
    currentMode = !currentMode;
  }
}

float tempInCelsius(int sensorValue) {
  float mV = (sensorValue) * (5000 / 1024.0);
  return (mV) / 10.;
}

void clearAllStates() {
  oldSensorValue = analogRead(SENSOR);
  lastUpdateTime = millis();
  option = -1;
  timerStamp = 0;
  lastButtonState = LOW;
  currentMode = LOW;
  digitalWrite(LED, LOW);
}

void receiveData() {
  if (Serial.available()) {

    String inputString = "";

    /*Lê do serial se disponível e constroi a String*/
    while (Serial.available()) {
      char inChar = (char)Serial.read();
      inputString += inChar;
    }

    /*Limpa o buffer*/
    while (Serial.available()) {
      Serial.read() ;
    }

    /*Avalia o que foi lido e faz as ações correspondentes*/
    if (inputString == "1") {
      buttonChanged(HIGH);
    }
    else if (inputString == "0") {
      clearAllStates();
    }

    /*Ainda não funciona*/
    else if (inputString == "setup" || inputString == "SETUP") {
      clearAllStates();
      bool flag = true;
      while (flag) {
        String inputString[3] = {"","",""};
        int i = 0;
        if (Serial.available()) {
          while (Serial.available() && i < 3) {
            char inChar = (char)Serial.read();
             if(inChar == ' '){
              i++;
              continue;
            }
            if(inChar == ';'){
              i=4; //Exit loop
              continue;
            }
            inputString[i] += inChar;   
          } 
        }
        if(inputString[0] != "" && inputString[1] != "" && inputString[2] != ""){
              Serial.println(inputString[0]);
              Serial.println(inputString[1]);
              Serial.println(inputString[2]);
              flag = false;
              break;
        }
      }
    }
  }
}

