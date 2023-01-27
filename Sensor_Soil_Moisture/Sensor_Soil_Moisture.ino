#include <LiquidCrystal_I2C.h>
#include <HX711.h>

int alarm = 8;
int led = 9; 
int sensorValue = 0;
float percentValue = 0;
const int Dout = 7;
const int sck = 6;
const float berat = 600;
long skala=0;

HX711 Ukur;
LiquidCrystal_I2C lcd(0x27,16,2);

void setup() {
  Serial.begin(9600);
  pinMode(alarm, OUTPUT);
  pinMode(led, OUTPUT);
  pinMode(A0, INPUT);
  Ukur.begin(Dout, sck);
  Ukur.set_scale(skala);
  lcd.init();
  lcd.backlight();
}

void loop() {
  if(Serial.available()>0){
    String jawaban = Serial.readStringUntil('/n');
    if(jawaban == "kalibrasi"){
      skala = kalibrasi (berat, 10);
      delay(1000);
      Ukur.set_scale(skala);
      //Ukur.tare();
      Serial.print("terkalibrasi\n");
    }else if(jawaban == "ukur"){
      float nilai = Ukur.get_units(5);
      Serial.print("berat:");
      Serial.println(nilai);
    }
  }
  int sensorValue = analogRead(A0);
  Serial.print("\n\nAnalog Value: ");
  Serial.println(sensorValue);
  
  percentValue = map(sensorValue, 899, 410, 0, 100);
  Serial.print("\nPercentValue: ");
  Serial.print(percentValue);
  Serial.println("%");

  if(percentValue<14){ 
    digitalWrite(alarm,HIGH);
    digitalWrite(led, HIGH);
  }
  else{ 
    digitalWrite(alarm,LOW);
    digitalWrite(led, LOW);
  }
  delay(100);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Kadar Air Gabah");
  
  lcd.setCursor(0, 1);
  lcd.backlight();  
  lcd.print("Percent: ");
  lcd.print(percentValue);
  lcd.print("%");
  delay(5000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Berat Gabah");
  
  lcd.setCursor(0, 1);
  lcd.backlight();  
  lcd.print("Berat: ");
  lcd.print(Ukur.get_units(5), 2);
  lcd.print("g");
  delay(5000);

 }

 long kalibrasi(float berat, int jml){
  long hasil=0;
  Ukur.set_scale();
  Ukur.tare();
  Serial.print("masukan beban yang sudah terukur\n");
  Serial.print("masukan ya jika sudah:");
  while(true){
    if(Serial.readStringUntil('\n') == "ya"){
      Ukur.calibrate_scale(berat, jml);
      hasil = Ukur.get_scale();
      Serial.print("sudah terkalibrasi\n");
      Serial.print("skala:");
      Serial.println(Ukur.get_scale());
      Serial.print("tare:");
      Serial.println(Ukur.get_tare());
      break;
    }
  }
  return hasil;
}
