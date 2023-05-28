#include <LiquidCrystal_I2C.h>
#include <HX711.h>
#include <EEPROM.h>

#define YL69 A0
#define COM_YES 2
#define COM_NO -2
#define NO_COM 0
#define ARROW_UP 1
#define ARROW_DOWN  -1
#define NO_ARROW 0

const int Dout = 7;
const int sck = 6;
const int alarm = 8;
const int led = 9; 
const int sensorValue = 0;
const int tombol_cancel = 3;
const int tombol_atas = 5;
const int tombol_bawah = 4;
const int tombol_ok = 2;
const float berat = 700;

long skl=0;
float percentValue = 0.0;
float bawah;
float atas;

HX711 Ukur;
LiquidCrystal_I2C lcd(0x27,16,2);

class Menu{
  private:
    char buff[24];
    LiquidCrystal_I2C* lcd;
    int w;
    int r;
    long* skl;
    int menuIndex = 0;
    int okPin;
    int cancelPin;
    int upPin;
    int downPin;
    volatile int mode = 0; //0=menu, 1=submenu
    int yl69;
    int alarm;
    bool kalibrasi = false;
    float* bawah;
    float* atas;
    int ada = 0;
    
    String menu[5] = {
      "kalibrasi ukuran",
      "pengukuran",
      "kalibrasi YL69",
      "lainya",
      "ya       tidak"
    };

  public:
    Menu(const int pin1, const int pin2, 
      const int pin3, const int pin4, const int yl69, const int alarm, 
      long* skl, LiquidCrystal_I2C* lcd, float* bawah, float* atas){
        this->okPin= pin1;
        this->cancelPin = pin2;
        this->upPin = pin3;
        this->downPin = pin4;
        this->lcd = lcd;
        this->bawah = bawah;
        this->atas = atas;
        this->yl69 = yl69;
        this->alarm = alarm;
    }

    void selamatFunc(){
      lcd->init();
      lcd->backlight();
      long t = millis();
      
      lcd->clear();
      lcd->print("selamat datang");
      delay(2000);
      lcd->setCursor(0,1);
      lcd->print("tekan tombol apa saja untuk memulai");
      while (!ada){
        ada = getArrowStroke(1)? 1 : getCommandStroke(1)? 1 : 0;
      }
      return ;
    }
    
    int kalibrate(int w, int rep, long* skl){
      int kode = 0;
      char buff[16];
      int lanjut = 1;
      long hasil = 0;
      
      //Ukur.tare();
      delay(1000);
      Ukur.set_scale(*skl);
      //Ukur.tare();
      lcd->clear();
      lcd->print("masukan beban!");
      lcd->setCursor(0,1);
      lcd->print(menu[4]);
      while(lanjut){
        int ans = getCommandStroke(1);
        if(ans == 2){
          lanjut = 0;
          kode = 0;
        }else if(ans == -2){
          lanjut = 0;
          kode = 1;
        }
      }
      if(kode == 0){
        lcd->clear();
        lcd->print("mengkalibrasi");
        Ukur.calibrate_scale(w, rep);
        delay(2000);
        hasil = Ukur.get_scale();
        lcd->clear();
        lcd->print("terkalibrasi!");
        delay(2000);
        lcd->clear();
        sprintf(buff, "skala:%d", Ukur.get_scale());
        lcd->print(buff);
        lcd->setCursor(0, 1);
        sprintf(buff, "tare:%d", Ukur.get_tare());
        lcd->print(buff);
        delay(3000);
      }
      return kode;
    }

    void printMenu(){
      lcd->setCursor(0,0);
      lcd->print(menu[menuIndex]);
      lcd->setCursor(0,1);
      lcd->print(menu[4]); 
      return;
    }
    void initCommand(){
      int com = getCommandStroke(1);
      sprintf(buff, "arrow: %d", com);
      //Serial.print(buff);
      int arah = getArrowStroke(1);
      sprintf(buff,",command: %d", arah);
      //Serial.print(buff);
      sprintf(buff, ", menuIndex: %d\n", menuIndex);
      //Serial.print(buff);
      if(!mode){
        if(arah == ARROW_UP){
          menuIndex++;
          menuIndex = menuIndex > 3 ? 0 : menuIndex;
          lcd->clear();
        }else if(arah == ARROW_DOWN){
          menuIndex--;
          menuIndex = menuIndex < 0 ? 3 : menuIndex;
          lcd->clear();
        }
        if(com == COM_YES){
          mode = 1;
        }else if(com == COM_NO){
          mode = 0;
        }
        printMenu();
      }else{
        if(com == COM_YES){
          mode = 1;
        }else if(com == COM_NO){
          mode = 0;
        }
        switch(menuIndex){
          case 0:
            setKalibrasiArg();
            break;
          case 1:
            pengukuranDisplay(Ukur.get_units(5), ukurYL69());
            break;
          case 2:
            kalibrasiYL69(YL69, bawah, atas);
            break;
          default:
            break;
        }
      }
      return;
    }
    int getArrowStroke(int debounce){
      int kode = NO_ARROW;
      long t = millis();

      int up = digitalRead(upPin);
      int down = digitalRead(downPin);
      if(debounce){
        if(!up)
          while((millis() - t) < 200) 
            if(digitalRead(upPin))
              kode = ARROW_UP;
        t = millis();
        if(!down)
          while((millis() - t) < 300)
            if(digitalRead(downPin))
              kode = ARROW_DOWN;
      }else{
        if(!up)
          while((millis() - t) < 100) 
            if(!digitalRead(upPin))
              kode = ARROW_UP;
        t = millis();
        if(!down)
          while((millis() - t) < 100)
            if(!digitalRead(downPin))
              kode = ARROW_DOWN;
      }
      return kode;
    }

    int getCommandStroke(int debounce){
      int kode = NO_COM;
      long t = millis();
      int ok = digitalRead(okPin);  //pull up, jika 1 maka tidak terpijit
      int cancel = digitalRead(cancelPin); //pull up, jika 1 maka tidak terpijit

      if(debounce){
        if(!ok)
          while((millis() - t) < 200)
            if(digitalRead(okPin))
              kode = COM_YES;
        if(!cancel)
          while((millis() - t) < 300)
            if(digitalRead(cancelPin))
              kode = COM_NO;
      }else{
        if(!ok) kode = COM_YES;
        if(!cancel) kode = COM_NO;
      }
      return kode;
    }
    void pengukuranDisplay(float percentValue, float ukuran){
      
      lcd->clear();
      lcd->setCursor(0, 0);
      lcd->print("Kadar Air Gabah");
      lcd->setCursor(0, 1); 
      lcd->print("Percent: ");
      lcd->print(percentValue);
      lcd->print("%");
      delay(3000);
      lcd->clear();
      lcd->setCursor(0,0);
      lcd->print("Berat Gabah");
      lcd->setCursor(0, 1);  
      lcd->print("Berat: ");
      lcd->print(ukuran, 2);
      lcd->print("g");
      delay(3000);
      return;
    }

    void kalibrasiYL69(const int pin, float* bawah, float* atas){
      char buff[16];
      char* text = "bawah:%d atas:%d";
      int b = *bawah;
      int a = *atas;
      bool edit = false;
      int editIndex = 0;

      
      lcd->clear();
      lcd->print("masukan batas:");
      lcd->setCursor(0,1);
      lcd->print(sprintf(buff, text, *bawah, *atas));
      while(true){
        int arrow = getArrowStroke(1);
        int command = getCommandStroke(1);
//        Serial.print(sprintf(buff, "arrow: %i", arrow));
//        Serial.println(sprintf(buff,",command: %i", command));
        if(!edit){
          if(arrow == ARROW_UP){
            editIndex++;
            editIndex = editIndex > 1? 0: editIndex;
          }
          else if(arrow == ARROW_DOWN){
            editIndex--;
            editIndex = editIndex < 0? 1: editIndex;
          }
          if(command == COM_YES) edit = true;
          else if(command == COM_NO) edit = false;
        }else{
           if(arrow == ARROW_UP){
            if(editIndex) b++;
            else a++;
          }
          else if(arrow == ARROW_DOWN){
            if(editIndex) b--;
            else a--;
          }
          if(command == COM_YES){
            *bawah = b;
            *atas = a;
            break;
          }
          else if(command == COM_NO){
            edit = false;
            b = *bawah;
            a = *atas;
          }
          lcd->clear();
          lcd->print("masukan batas:");
          lcd->setCursor(1,0);
          lcd->print(sprintf(buff,text,b,a));
        }
      }
      interrupts();
      return;
    }

    void lainyaMenu(){
      ;
    }
    void setKalibrasiArg(){
      char buff[16];
      char* txt = "berat: %ig";
      char* txt2 = "rep: %i";
      char* txt3 = "ambil dari ROM?";
      char* txt4 = "kalibrasi?";
      int num=0;
      int w;
      int rep;
      int ans;
      int mod=0;
      int lanjut = 1;

      lcd->clear();
      lcd->print("ambil dari ROM?");
      lcd->setCursor(0,1);
      lcd->print(menu[4]);
      while(lanjut){
        ans = getCommandStroke(1);
        if(ans == COM_NO) lanjut=0;
        else if(ans == COM_YES){
          mod = 1; //eeprom
          lanjut = 0;
        }
      }
      if(mod == 1)
        lanjut = 0;
      else
        lanjut = 1;
      if(lanjut){
        lcd->clear();
        lcd->print(txt4);
        lcd->setCursor(0,1);
        lcd->print(menu[4]);
        while(lanjut){
          ans = getCommandStroke(1);
          if(ans == COM_NO){
            mod = 0; //keluar
            lanjut = 0;
           }else if(ans == COM_YES){
            mod = 2; //kalibrasi
            lanjut=0;
          }
        }
      }
      if(mod == 1){
        unsigned long temp;
        
        lcd->clear();
        lcd->print("ambil dari ROM");
        EEPROM.get(1, temp);
        delay(2000);
        lcd->clear();
        lcd->print("terbaca:");
        lcd->print(temp);
        delay(2000);
        if (temp == 0){
          // kalau gak ada suruh kalibrasi
          lcd->clear();
          lcd->print("belum dikalibrasi");
          delay(2000);
          lcd->clear();
          lcd->print("silahkan kalibrasi");
          lcd->setCursor(0, 1);
          lcd->print(menu[4]);
        }else{
          // kalau ada langsung pakai untuk kalibrasi
          lcd->clear();
          lcd->print("ambil data");
          Ukur.set_scale(*skl);
          delay(2000);
          lcd->clear();
          lcd->print("terkalibrasi");
          delay(2000);
        }
      }else if(mod == 2){
        lanjut=1;
        lcd->clear();
        lcd->print("masukan berat");
        lcd->setCursor(0,1);
        lcd->print("berat:");
        lcd->print(sprintf(buff, txt, num));
        while(lanjut){
          ans = getArrowStroke(0);
          if(ans == ARROW_UP){
            lcd->clear();
            num++;
            sprintf(buff, txt, num);
            lcd->print(buff);
          }else if(ans == ARROW_DOWN){
            num--;
            sprintf(buff, txt, num);
            lcd->clear();
            lcd->print(buff);
          }
          ans = getCommandStroke(1);
          if(ans==COM_YES){
            w = num;
            num = 0;
            lanjut=0;
          }else if(ans == COM_NO){
            lanjut = 0;
          }
        }
        lanjut=1;
        lcd->clear();
        lcd->print("masukan rep");
        lcd->setCursor(0,1);
        lcd->print("rep:");
        lcd->print(sprintf(buff, txt2, num));
        while(lanjut){
          ans = getArrowStroke(0);
          if(ans == ARROW_UP){
            lcd->clear();
            num++;
            sprintf(buff, txt2, num);
            lcd->print(buff);
          }else if(ans == ARROW_DOWN){
            num--;
            lcd->clear();
            sprintf(buff, txt2, num);
            lcd->print(buff);
          }
          ans = getCommandStroke(1);
          if(ans==COM_YES){
            rep = num;
            num=0;
            lanjut=0;
          }else if(ans == COM_NO){
            lanjut=0;
          }
        }
      }
      if(w && rep){
        kalibrasi = true;
        this->w = w;
        this->r = r;
        kalibrate(this->w, this->r, this->skl);
      }else
        kalibrasi = false;
      this->mode = 0;
      return ;
    }
    float ukurYL69(){
      int sensorValue = analogRead(this->yl69);
      float percentValue = map(sensorValue, 1023, 500 , 0, 100);
      if(percentValue < 14){ 
        ringAlarm(false);
      }else{ 
        ringAlarm(true);
      }
      return percentValue;
    }
    void ringAlarm(bool alarm){
      if(alarm){
        digitalWrite(alarm, HIGH);
        digitalWrite(led, HIGH);
      }else{
        digitalWrite(alarm, LOW);
        digitalWrite(led, LOW);
      }
      return;
    }
    void setMode(int value){
      mode = value;
      return;
    }
}utama(tombol_ok, tombol_cancel, tombol_atas, tombol_bawah, YL69, alarm, &skl, &lcd, &atas, &bawah);
void setup() {
  Serial.begin(9600);
  pinMode(alarm, OUTPUT);
  pinMode(led, OUTPUT);
  pinMode(YL69, INPUT);
  pinMode(tombol_cancel, INPUT_PULLUP);
  pinMode(tombol_atas, INPUT_PULLUP);
  pinMode(tombol_bawah, INPUT_PULLUP);
  pinMode(tombol_ok, INPUT_PULLUP);
  Ukur.begin(Dout, sck);
  Ukur.set_scale(skl);
  utama.selamatFunc();
}
void loop() {
//  attachInterrupt(digitalPinToInterrupt(2), setMode, CHANGE);
//  attachInterrupt(digitalPinToInterrupt(3), setMode, CHANGE);
  utama.initCommand();
}
void setMode(){
  utama.setMode(0);
  return;
}
