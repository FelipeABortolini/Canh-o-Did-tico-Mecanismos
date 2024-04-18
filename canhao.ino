#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <math.h>
#include <Stepper.h>
#include <Servo.h>
#include <stdlib.h> // Para malloc e free

#define STEPS1 65 // Número de passos por revolução do motor de passo
#define STEPS2 43 // Número de passos por revolução do motor de passo

// Definição dos pinos do motor de passo
Stepper myStepper1(STEPS1, 10, 11);
Stepper myStepper2(STEPS2, 12, 13);

Servo myServo;

int inclinacaoCount = 0;
int outBoxCount = 0;

struct Step {
    int motor; // Identificador do motor (por exemplo, 1 para myStepper1, 2 para myStepper2)
    int steps; // Quantidade de passos
};

// Definindo uma estrutura de dados para armazenar os movimentos
struct StepList {
    struct Step *steps; // Array de movimentos
    int count; // Número total de movimentos na lista
};

// Função para inicializar a lista de movimentos
struct StepList* initializeStepList() {
    struct StepList *list = (struct StepList*)malloc(sizeof(struct StepList));
    list->steps = NULL;
    list->count = 0;
    return list;
}

struct StepList *movements = initializeStepList();

const byte ROWS = 4; // Quatro linhas
const byte COLS = 4; // Quatro colunas

char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', '.'}
};

byte rowPins[ROWS] = {9, 8, 7, 6}; // Conectar a linha de saída do teclado ao pinos digitais do Arduino
byte colPins[COLS] = {5, 4, 3, 2}; // Conectar a coluna de entrada do teclado ao pinos digitais do Arduino

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

// DEFINIÇÕES
#define endereco  0x27 // Endereços comuns: 0x27, 0x3F
#define colunas   16
#define linhas    2

//LiquidCrystal lcd(12, 11, 10, 5, 4, 3, 2);
// INSTANCIANDO OBJETOS
LiquidCrystal_I2C lcd(endereco, colunas, linhas);

byte graus[8] = {   // Define o padrão do caractere de graus
  B00111,
  B00101,
  B00111,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
};

// #define GRAVIDADE 9.81
// #define MASSA 0.0028 // kg
// #define MASSA 0.002 // kg
// #define K 300 // N/m
// #define DEF_MOLA 0.065 // m

// float velocidade_inicial = sqrt((K*(pow(DEF_MOLA, 2)))/MASSA);

float distancia_alvo = 0.0;
float altura_alvo = 0.0;
float distancia_anterior = 0.0;
float altura_anterior = 0.0;
bool digitandoDistancia = false;
bool digitandoAltura = false;
bool entradaDistancia = false; // Flag para indicar se está digitando a distância
bool entradaAltura = false; // Flag para indicar se está digitando a altura
char distancia_input[16] = ""; // String para armazenar a entrada da distância
char altura_input[16] = ""; // String para armazenar a entrada da altura

// float calcula_angulo_necessario(float velocidade_inicial, float distancia_alvo, float altura_alvo) {
float calcula_angulo_necessario(float distancia_alvo, float altura_alvo) {
  float delta_y = altura_alvo / 100.0; // Convertendo altura de cm para metros
  float delta_x = distancia_alvo;
  // float v0 = velocidade_inicial;

  // Equação para calcular o ângulo
  // float theta = atan2(pow(v0, 2) + sqrt(pow(v0, 4) - GRAVIDADE * (GRAVIDADE * pow(delta_x, 2) + 2 * delta_y * pow(v0, 2))), GRAVIDADE * delta_x);
  float theta = 90 - delta_x * 15 + delta_y / 0.0333;

  // return theta * 180 / PI;
  return theta;
}

// Função para adicionar um movimento à lista
void addStep(struct StepList *list, int motor, int steps, int milissec, bool inclinacao) {
    list->count++;
    list->steps = (struct Step*)realloc(list->steps, list->count * sizeof(struct Step));
    list->steps[list->count - 1].motor = motor;
    list->steps[list->count - 1].steps = steps;
    if(motor == 1){
      myStepper1.step(steps);
    } else {
      myStepper2.step(steps);      
    }
    delay(milissec);
    if(inclinacao){
      inclinacaoCount++;
    } else {
      outBoxCount++;
    }
}

// Função para adicionar um movimento à lista
void popStep(struct StepList *list, int milissec, bool inclinacao, double angulo, bool retornandoInclinacao) {
  if (retornandoInclinacao && angulo > 57.5 && angulo <= 62.5 ) {
    myStepper1.step(-STEPS1);
    myStepper2.step(-list->steps[list->count - 1].steps);
    myStepper1.step(STEPS1);
  } else if (retornandoInclinacao && angulo > 62.5 && angulo <= 67.5 ) {
    myStepper1.step(-STEPS1);
    myStepper2.step(-list->steps[list->count - 1].steps);
    myStepper1.step(STEPS1);
  } else if (retornandoInclinacao && angulo > 67.5) {
    myStepper1.step(-STEPS1);
    myStepper2.step(-list->steps[list->count - 1].steps*0.5);
    myStepper1.step(STEPS1);
    myStepper2.step(-list->steps[list->count - 1].steps*0.5);
  } else {
    if(list->steps[list->count - 1].motor == 1){
      myStepper1.step(-list->steps[list->count - 1].steps);
    } else {
      myStepper2.step(-list->steps[list->count - 1].steps);      
    }
  }
    delay(milissec);

    list->count--;
    list->steps = (struct Step*)realloc(list->steps, list->count * sizeof(struct Step));
    if(inclinacao){
      inclinacaoCount--;
    } else {
      outBoxCount--;
    }
}

void goOutBox(int milissec){
  addStep(movements, 1, STEPS1*0.6, milissec, false);
  addStep(movements, 2, STEPS2, milissec, false);
  addStep(movements, 1, STEPS1*0.7, milissec, false);
  addStep(movements, 2, STEPS2, milissec, false);
  addStep(movements, 1, STEPS1*0.7, milissec, false);
  addStep(movements, 2, STEPS2*0.8, milissec, false);
}

void returnToBox(int milissec, double angulo){
  while(outBoxCount > 0){
    popStep(movements, milissec, false, angulo, false);
  }
  myStepper2.step(-STEPS2*0.4);
}

void inclina(double angulo, int milissec){

  if (angulo <= 2.5 && angulo <= 7.5) {
  } else if (angulo > 7.5 && angulo <= 12.5) {
    addStep(movements, 2,-STEPS2*0.4, milissec, true);
  } else if (angulo > 12.5 && angulo <= 17.5) {
    addStep(movements, 2,-STEPS2*0.6, milissec, true);
  } else if (angulo > 17.5 && angulo <= 22.5) {
    addStep(movements, 2,-STEPS2*0.8, milissec, true);
  } else if (angulo > 22.5 && angulo <= 27.5) {
    addStep(movements, 2,-STEPS2, milissec, true);
  } else if (angulo > 27.5 && angulo <= 32.5) {
    addStep(movements, 2,-STEPS2*1.2, milissec, true);
  } else if (angulo > 32.5 && angulo <= 37.5) {
    addStep(movements, 2,-STEPS2*1.4, milissec, true);
  } else if (angulo > 37.5 && angulo <= 42.5) {
    addStep(movements, 2,-STEPS2*1.6, milissec, true);
  } else if (angulo > 42.5 && angulo <= 47.5) {
    addStep(movements, 2,-STEPS2*1.8, milissec, true);
  } else if (angulo > 47.5 && angulo <= 52.5) {
    addStep(movements, 2,-STEPS2*2, milissec, true);
  } else if (angulo > 52.5 && angulo <= 57.5) {
    addStep(movements, 2,-STEPS2*2.2, milissec, true);
  } else if (angulo > 57.5 && angulo <= 62.5) {
    addStep(movements, 2,-STEPS2*2.4, milissec, true);
  } else if (angulo > 62.5 && angulo <= 67.5) {
    addStep(movements, 2,-STEPS2*2.6, milissec, true);
  } else if (angulo > 67.5) {
    addStep(movements, 2,-STEPS2*2.8, milissec, true);
  }
}

void retornaInclinacao(int milissec, double angulo){
  while(inclinacaoCount > 0){
    popStep(movements, milissec, true, angulo, true);
  }
}

void setup() {
  lcd.init(); // INICIA A COMUNICAÇÃO COM O DISPLAY
  lcd.backlight(); // LIGA A ILUMINAÇÃO DO DISPLAY
  lcd.createChar(0, graus); // Define o caractere de graus
  lcd.clear(); // LIMPA O DISPLAY
  lcd.print("Dist.(m):");
  lcd.setCursor(0, 1);
  lcd.print("Alt.(cm):");
  myStepper1.setSpeed(90); // ajusta a velocidade inicial para 90 passos
  myStepper2.setSpeed(90); // ajusta a velocidade inicial para 90 passos
  myServo.attach(0);
  myServo.write(120);
}

void loop() {
  char key = keypad.getKey();

  if (key != NO_KEY) {
    if (key == '#') {
      if (entradaDistancia && entradaAltura) { // Verifica se ambas a distância e altura foram digitadas
        digitandoDistancia = false;
        digitandoAltura = false;
        entradaDistancia = false;
        entradaAltura = false;
        lcd.clear();
        lcd.print("Calculando...");
        delay(2000); // Simulando um cálculo rápido

        // float angulo = calcula_angulo_necessario(velocidade_inicial, atof(distancia_input), atof(altura_input)); // Convertendo a string para float
        float angulo = calcula_angulo_necessario(atof(distancia_input), atof(altura_input)); // Convertendo a string para float
        lcd.clear();
        lcd.print("Dist:");
        lcd.print(distancia_input);
        lcd.print("m");
        lcd.setCursor(0, 1);
        lcd.print("Alt:");
        lcd.print(altura_input);
        lcd.print("cm");
        delay(2000);

        lcd.clear();
        lcd.print("Angulo: ");
        lcd.print(angulo);
        lcd.write(byte(0)); // Escreve o caractere de graus
        lcd.setCursor(0, 1);
        lcd.print("Pos. Mecanismo..");

        goOutBox(1000); // SAI DA CAIXA
        // ====================== TO DO - ANGULOS ========================================================================
        delay(3000);

        inclina(angulo, 100);
        delay(3000);

        lcd.clear();
        lcd.print("Disparando...");
        delay(2000);
        myServo.write(240);
        delay(500);
        myServo.write(120);
        delay(2000);

        lcd.clear();
        lcd.print("Concluido!");
        lcd.setCursor(0, 1);
        lcd.print("Ret. Mecanismo..");

        retornaInclinacao(100, angulo);
        delay(3000);
        returnToBox(250, angulo);
        delay(1000);

        lcd.clear();
        lcd.print("Dist.(m):");
        lcd.setCursor(0, 1);
        lcd.print("Alt.(cm):");
        strcpy(distancia_input, ""); // Limpa a string da distância
        strcpy(altura_input, ""); // Limpa a string da altura
      }
    }
    else if (key == 'A') {
      digitandoDistancia = true;
      digitandoAltura = false;
      entradaDistancia = true; // Indica que começou a entrada da distância
      lcd.clear();
      lcd.print("Dist.(m):");
    }
    else if (key == 'B') {
      digitandoDistancia = false;
      digitandoAltura = true;
      entradaAltura = true; // Indica que começou a entrada da altura
      lcd.clear();
      lcd.print("Alt.(cm):");
    }
    else if (key == 'C') { // clear all
      lcd.clear();
      lcd.print("Resetando...");
      delay(2000);
      lcd.clear();
      lcd.print("Dist.(m):");
      lcd.setCursor(0, 1);
      lcd.print("Alt.(cm):");
      strcpy(distancia_input, ""); // Limpa a string da distância
      strcpy(altura_input, ""); // Limpa a string da altura
    }
    else if (digitandoDistancia) {
      if (key == '*') {
        // Verificar se há algo para apagar
        if (strlen(distancia_input) > 0) {
          distancia_input[strlen(distancia_input) - 1] = '\0'; // Remove o último caractere da string
          lcd.clear();
          lcd.print("Dist.(m):");
          lcd.print(distancia_input);
        }
      }
      else {
        // Adicionar o caractere à string da distância
        strcat(distancia_input, &key);
        lcd.print(key);
      }
    }
    else if (digitandoAltura) {
      if (key == '*') {
        // Verificar se há algo para apagar
        if (strlen(altura_input) > 0) {
          altura_input[strlen(altura_input) - 1] = '\0'; // Remove o último caractere da string
          lcd.clear();
          lcd.print("Alt.(cm):");
          lcd.print(altura_input);
        }
      }
      else {
        // Adicionar o caractere à string da altura
        strcat(altura_input, &key);
        lcd.print(key);
      }
    }
  }
}