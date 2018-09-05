#include "defines.h"


// Adafruit_GFX BCD numeric font (e.g. for clock displays) by andig (https://gist.github.com/andig/7369820)
static const unsigned char font_bcd[] = {
  B01011111, // 0 0x5f
  B00000101, // 1 0x05
  B01110110, // 2 0x76
  B01110101, // 3 0x75
  B00101101, // 4 0x2d
  B01111001, // 5 0x79
  B01111011, // 6 0x7b
  B01000101, // 7 0x45
  B01111111, // 8 0x7f
  B01111101  // 9 0x7d
};

void barVert(int16_t x, int16_t y, int16_t w, int16_t l) {
  u8g2.drawTriangle(x+1, y+2*w+1, x+w, y+w+1, x+2*w, y+2*w+1);
  u8g2.drawTriangle(x+2, y+2*w+l+1, x+w, y+3*w+l, x+2*w-1, y+2*w+l+1);
  u8g2.drawBox(x+1, y+2*w+1, 2*w-1, l);
}

void barHor(int16_t x, int16_t y, int16_t w, int16_t l) {
  u8g2.drawTriangle(x+2*w+1, y+2*w-1, x+w+2, y+w, x+2*w+1, y+1);
  u8g2.drawTriangle(x+2*w+l+1, y+2*w-1, x+3*w+l, y+w, x+2*w+l+1, y+1);
  u8g2.drawBox(x+2*w+1, y+1, l, 2*w-1);
}

void drawBCD(int16_t x, int16_t y, int8_t num, int16_t w, int16_t l) {
  // @todo clipping
  if (num < 0 || num > 9) return;
  
  int8_t c = font_bcd[num];
  int16_t d = 2*w+l+1;
  
  if (c & 0x01) barVert(x+d, y+d, w, l);
  if (c & 0x02) barVert(x, y+d, w, l);
  if (c & 0x04) barVert(x+d, y, w, l);
  if (c & 0x08) barVert(x, y, w, l);
  
  if (c & 0x10) barHor(x, y+2*d, w, l);
  if (c & 0x20) barHor(x, y+d, w, l);
  if (c & 0x40) barHor(x, y, w, l);
}

void showBatt(int percent, bool blinkIt) {
  u8g2.drawFrame(0, 52, 103, 11);

  if (!blinkIt || (blinkIt && (millis() % 1000 < 500)))
    for (int i = 0; i < float(20) / 100 * percent; i++) u8g2.drawBox(2 + i * 5, 54, 4, 7);

  if (((conf.battWarnPercent == 0) || (percent > conf.battWarnPercent)) || ((conf.battWarnPercent != 0) && (millis() % 500 < 250))) {
    u8g2.setCursor(105, 61);
    if (percent < 100) u8g2.print(' ');
    if (percent < 10) u8g2.print(' ');
    u8g2.print(percent);
    u8g2.print('\%');
  }
}
 
void displayFSM() {
  u8g2.firstPage();
  do {
  struct {
    unsigned int curh;
    unsigned int curl;
    unsigned int vh;
    unsigned int vl;
    unsigned int sph;
    unsigned int spl;
    unsigned int milh;
    unsigned int mill;
    unsigned int Min;
    unsigned int Sec;
    unsigned int temp;
  }m365;

  int brakeVal = -1;
  int throttleVal = -1;

  int tmp_0, tmp_1;

  m365.sph = abs(S23CB0.speed) / 1000;         //speed
  m365.spl = abs(S23CB0.speed) % 1000 / 100;
  m365.curh = abs(S25C31.current) / 100;       //current
  m365.curl = abs(S25C31.current) % 100;
  m365.vh = abs(S25C31.voltage) / 100;         //voltage
  m365.vl = abs(S25C31.voltage) % 100;

  u8g2.clearDisplay();

  if ((S25C31.current == 0) && (S25C31.remainPercent == 0)) {
    u8g2.setFont(u8g2_font_ncenB14_tr);
    u8g2.setCursor(0, 0);
    u8g2.println("BUS not");  
    u8g2.println("connected!");  
    u8g2.println("No data to");  
    u8g2.println("display!"); 
    u8g2.display();
    return; 
  }
  
  if ((m365.sph > 1) && Settings) Settings = false;

  if ((S23CB0.speed <= 100) || Settings) {
    if (S20C00HZ65.brake > 130)
    brakeVal = 1;
      else
      if (S20C00HZ65.brake < 50)
        brakeVal = -1;
        else
        brakeVal = 0;
    if (S20C00HZ65.throttle > 150)
      throttleVal = 1;
      else
      if (S20C00HZ65.throttle < 50)
        throttleVal = -1;
        else
        throttleVal = 0;

    if (((brakeVal == 1) && (throttleVal == 1) && !Settings) && ((oldBrakeVal != 1) || (oldThrottleVal != 1))) { // brake max + throttle max = menu on
      menuPos = 0;
      Settings = true;
    }

    if (Settings) {
      if ((throttleVal == 1) && (oldThrottleVal != 1) && (brakeVal == -1) && (oldBrakeVal == -1))                // brake min + throttle max = change menu value
      switch (menuPos) {
        case 0:
          autoBig = !autoBig;
          break;
        case 1:
          switch (bigMode) {
            case 0:
              bigMode = 1;
              break;
            default:
              bigMode = 0;
          }
          break;
        case 2:
          switch (warnBatteryPercent) {
            case 0:
              warnBatteryPercent = 5;
              break;
            case 5:
              warnBatteryPercent = 10;
              break;
            case 10:
              warnBatteryPercent = 15;
              break;
            default:
              warnBatteryPercent = 0;
          }
          break;
        case 3:
          bigWarn = !bigWarn;
          break;
        case 4:
          Settings = false;
          EEPROM.put(1, autoBig);
          EEPROM.put(2, warnBatteryPercent);
          EEPROM.put(3, bigMode);
          EEPROM.put(4, bigWarn);
          break;
      } else
      if ((brakeVal == 1) && (oldBrakeVal != 1) && (throttleVal == -1) && (oldThrottleVal == -1))                // brake max + throttle min = change menu position
        if (menuPos < 4)
          menuPos++;
          else
          menuPos = 0;

      u8g2.setFont(u8g2_font_profont10_tr); 
      u8g2.setCursor(0, 0);

      if (menuPos == 0)
        u8g2.print(">");
        else
        u8g2.print(" ");

      u8g2.print("Big speedometer: ");
      if (autoBig)
        u8g2.print("YES");
        else
        u8g2.print("NO");

      u8g2.setCursor(0, 8);

      if (menuPos == 1)
        u8g2.print(">");
        else
        u8g2.print(" ");

      u8g2.print("Big speedo. mode: ");
      switch (bigMode) {
        case 1:
          u8g2.print("2");
          break;
        default:
          u8g2.print("1");
      }

      u8g2.setCursor(0, 16);

      if (menuPos == 2)
        u8g2.print(">");
        else
        u8g2.print(" ");

      u8g2.print("Battery warning: ");
      switch (warnBatteryPercent) {
        case 5:
          u8g2.print(" 5%");
          break;
        case 10:
          u8g2.print("10%");
          break;
        case 15:
          u8g2.print("15%");
          break;
        default:
          u8g2.print("OFF");
          break;
      }

      u8g2.setCursor(0, 24);

      if (menuPos == 3)
        u8g2.print(">");
        else
        u8g2.print(" ");

      u8g2.print("Big batt. warn.: ");
      if (bigWarn)
        u8g2.print("YES");
        else
        u8g2.print("NO");
        
      //u8g2.drawFastHLine(0, 36, 128, WHITE);

      u8g2.setCursor(0, 40);
      
      if (menuPos == 4)
        u8g2.print(">");
        else
        u8g2.print(" ");

      u8g2.print("Save and exit");
/*
      u8g2.drawFastHLine(1, 40, 3, WHITE);
      u8g2.drawRect(0, 41, 5, 6, WHITE);
      u8g2.drawPixel(2, 41, BLACK);
      u8g2.fillRect(1, 43, 3, 4, WHITE);
*/
      u8g2.setCursor(0, 54);
      u8g2.print("Batt: ");
      u8g2.print(m365.vh);
      u8g2.print('.');
      if (m365.vl < 10) u8g2.print('0');
      u8g2.print(m365.vl);
      u8g2.print("V, ");
      u8g2.print(S25C31.remainCapacity);
      u8g2.print("mAh");

      u8g2.display();

      oldBrakeVal = brakeVal;
      oldThrottleVal = throttleVal;
 
      return;
    } else
    if ((throttleVal == 1) && (oldThrottleVal != 1) && (brakeVal == -1) && (oldBrakeVal == -1)) {
      //u8g2.drawRect(0, 0, 127, 63, 0);
      
      tmp_0 = S23CB0.mileageTotal / 1000;
      tmp_1 = (S23CB0.mileageTotal % 1000) / 10;

      u8g2.setFont(u8g2_font_profont10_tr); 
      u8g2.setCursor(0, 17);
      u8g2.println("Total distance:");
      u8g2.setFont(u8g2_font_ncenB14_tr);
      u8g2.setCursor(10, 29);
      if (tmp_0 < 1000) u8g2.print(' ');
      if (tmp_0 < 100) u8g2.print(' ');
      if (tmp_0 < 10) u8g2.print(' ');
      u8g2.print(tmp_0);
      u8g2.print('.');
      if (tmp_1 < 10) u8g2.print('0');
      u8g2.print(tmp_1);
      u8g2.setFont(u8g2_font_profont10_tr); 
      u8g2.print("km");

      u8g2.display();

      return;
    }

    oldBrakeVal = brakeVal;
    oldThrottleVal = throttleVal;
  }

  if (bigWarn && (((warnBatteryPercent != 0) && (S25C31.remainPercent <= warnBatteryPercent)) && (millis() % 1000 < 500)))
    //u8g2.drawBitmap(0, 0, battery, 128, 64, WHITE); 
    if ((m365.sph > 1) && (autoBig)) {      
      /*switch (bigMode) {
        case 1:
          tmp_0 = m365.curh / 10;
          tmp_1 = m365.curh % 10;
          if (tmp_0 > 0) drawBCD(0, 0, tmp_0, 4, 11);
          drawBCD(36, 0, tmp_1, 4, 11);
          u8g2.fillRect(64, 42, 7, 7, 1);
          tmp_0 = m365.curl / 10;
          tmp_1 = m365.curl % 10;
          drawBCD(84, 0, m365_info.spl, 4, 11);
          u8g2.setCursor(108, 1);
          u8g2.setFont(u8g2_font_ncenB14_tr);
          u8g2.print(tmp_1);
          if ((S25C31.current >= 0) || ((S25C31.current < 0) && (millis() % 1000 < 500))) {
            u8g2.setCursor(108, 34);
            u8g2.print("A");
          }
          break;*/
          tmp_0 = m365_info.sph / 10;
          tmp_1 = m365_info.sph % 10;
          if (tmp_0 > 0) drawBCD(0, 0, tmp_0, 4, 11);
          drawBCD(36, 0, tmp_1, 4, 11);
          u8g2.drawBox(71, 41, 7, 7);
          drawBCD(84, 0, m365_info.spl, 4, 11);
          u8g2.setCursor(117, 8);
          u8g2.print(F("km"));
          u8g2.setCursor(117, 20);
          u8g2.print(F("/h"));
          
          }
      else {
      m365.milh = S23CB0.mileageCurrent / 100;   //mileage
      m365.mill = S23CB0.mileageCurrent % 100;
      m365.Min = S23C3A.ridingTime / 60;         //riding time
      m365.Sec = S23C3A.ridingTime % 60;
      m365.temp = S23CB0.mainframeTemp / 10;     //temperature

      u8g2.setFont(u8g2_font_ncenB14_tr);
      u8g2.setCursor(0, 0);

      if (m365.sph < 10) u8g2.print(' ');
      u8g2.print(m365.sph);
      u8g2.print('.');
      u8g2.print(m365.spl);
      u8g2.setFont(u8g2_font_profont10_tr); 
      u8g2.print("k/h");

      u8g2.setFont(u8g2_font_ncenB14_tr);
      u8g2.setCursor(94, 0);

      if (m365.temp < 10) u8g2.print(' ');
      u8g2.print(m365.temp);
      u8g2.setFont(u8g2_font_profont10_tr); 
      u8g2.drawCircle(119, 1, 1, 1);
      u8g2.setCursor(122, 0);
      u8g2.print("C");

      u8g2.setFont(u8g2_font_ncenB14_tr);
      u8g2.setCursor(0, 16);

      if (m365.milh < 10) u8g2.print(' ');
      u8g2.print(m365.milh);
      u8g2.print('.');
      if (m365.mill < 10) u8g2.print('0');
      u8g2.print(m365.mill);
      u8g2.setFont(u8g2_font_profont10_tr); 
      u8g2.print("km");

      u8g2.setFont(u8g2_font_ncenB14_tr);
      u8g2.setCursor(0, 32);

      if (m365.Min < 10) u8g2.print('0');
      u8g2.print(m365.Min);
      u8g2.print(':');
      if (m365.Sec < 10) u8g2.print('0');
      u8g2.print(m365.Sec);

      u8g2.setFont(u8g2_font_ncenB14_tr);
      u8g2.setCursor(62, 32);

      if (m365.curh < 10) u8g2.print(' ');
      u8g2.print(m365.curh);
      u8g2.print('.');
      if (m365.curl < 10) u8g2.print('0');
      u8g2.print(m365.curl);
      u8g2.setFont(u8g2_font_profont10_tr); 
      u8g2.print("A");

      showBatt(S25C31.remainPercent, S25C31.current < 0);
    }
  if (millis() % 1000 < 500) {
      u8g2.setCursor(0, 0);
      u8g2.print('*');
    }
  } while (u8g2.nextPage());
}

void calculate() {
  switch(AnswerHeader.cmd) {
    case 0x31: // 1
      m365_info.curh = abs(S25C31.current) / 100;     // current
      m365_info.curl = abs(S25C31.current) % 100;
      m365_info.remCapacity = S25C31.remainCapacity;
      m365_info.remPercent = S25C31.remainPercent;
      m365_info.bTemp1 = S25C31.temp1 - 20;
      m365_info.bTemp2 = S25C31.temp2 - 20;
      m365_info.voltage = S25C31.voltage;
      //m365_info.volth = S25C31.voltage / 100;
      //m365_info.voltl = S25C31.voltage % 100;
      break;
    case 0xB0: // 8 speed average mileage poweron time
      int _speed;
      _speed = abs(S23CB0.speed);
      #ifdef CUSTOM_WHELL_SIZE
        _speed = _speed * WHELL_SIZE / 85;
      #endif
      m365_info.sph = _speed / 1000;                  // speed
      m365_info.spl = _speed % 1000 / 100;
      m365_info.milh = S23CB0.mileageCurrent / 100;   // mileage
      m365_info.mill = S23CB0.mileageCurrent % 100;
      m365_info.milTotH = S23CB0.mileageTotal / 1000; // mileage total
      m365_info.milTotL = S23CB0.mileageTotal % 10;
      m365_info.aveh = S23CB0.averageSpeed / 1000;
      m365_info.avel = S23CB0.averageSpeed % 1000;
      m365_info.mTemp = S23CB0.mainframeTemp / 10;
      break;
    case 0x3A: //10
      m365_info.tripMin  = S23C3A.ridingTime  / 60;   // riding time
      m365_info.tripSec  = S23C3A.ridingTime  % 60;
      m365_info.powerMin = S23C3A.powerOnTime / 60;   // power on time
      m365_info.powerSec = S23C3A.powerOnTime % 60;
      break;
    case 0x40: //cellsa
      break;
    default:
      break;
  }
}  

void keyVal() {
  volatile static int oldBrakeVal = kVal.brake;
  volatile static int oldThrottleVal = kVal.throttle;
  
  if (S20C00HZ65.brake > MAX_BRAKE)
    kVal.brake = 1;
    else
    if (S20C00HZ65.brake < MIN_BRAKE)
      kVal.brake = -1;
      else
      kVal.brake = 0;
  if (S20C00HZ65.throttle > MAX_THROTTLE)
    kVal.throttle = 1;
    else
    if (S20C00HZ65.throttle < MIN_THROTTLE)
      kVal.throttle = -1;
      else
      kVal.throttle = 0;

  kVal.brakeChange = oldBrakeVal != kVal.brake;
  kVal.throttleChange = oldThrottleVal != kVal.throttle;

  if (kVal.brakeChange || kVal.throttleChange || ((kVal.throttle != 1) && (kVal.brake != 1))) kVal.kTimer = millis();
}

// -----------------------------------------------------------------------------------------------------------


void processPacket(unsigned char * data, unsigned char len) {
  unsigned char RawDataLen;
  RawDataLen = len - sizeof(AnswerHeader) - 2;//(crc)

  switch (AnswerHeader.addr) { //store data into each other structure
    case 0x20: //0x20
      switch (AnswerHeader.cmd) {
        case 0x00:
          switch (AnswerHeader.hz) {
            case 0x64: //BLE ask controller
              break;
            case 0x65:
              if (_Query.prepared == 1 && !_Hibernate) writeQuery();

              memcpy((void*)& S20C00HZ65, (void*)data, RawDataLen);

              break;
            default: //no suitable hz
              break;
          }
          break;
        case 0x1A:
          break;
        case 0x69:
          break;
        case 0x3E:
          break;
        case 0xB0:
          break;
        case 0x23:
          break;
        case 0x3A:
          break;
        case 0x7C:
          break;
        default: //no suitable cmd
          break;
      }
      break;
    case 0x21:
      switch (AnswerHeader.cmd) {
        case 0x00:
        switch(AnswerHeader.hz) {
          case 0x64: //answer to BLE
            memcpy((void*)& S21C00HZ64, (void*)data, RawDataLen);
            break;
          }
          break;
      default:
        break;
      }
      break;
    case 0x22:
      switch (AnswerHeader.cmd) {
        case 0x3B:
          break;
        case 0x31:
          break;
        case 0x20:
          break;
        case 0x1B:
          break;
        case 0x10:
          break;
        default:
          break;
      }
      break;
    case 0x23:
      switch (AnswerHeader.cmd) {
        case 0x17:
          break;
        case 0x1A:
          break;
        case 0x69:
          break;
        case 0x3E: //mainframe temperature
          if (RawDataLen == sizeof(A23C3E)) memcpy((void*)& S23C3E, (void*)data, RawDataLen);
          break;
        case 0xB0: //speed, average speed, mileage total, mileage current, power on time, mainframe temp
          if (RawDataLen == sizeof(A23CB0)) memcpy((void*)& S23CB0, (void*)data, RawDataLen);
          break;
        case 0x23: //remain mileage
          if (RawDataLen == sizeof(A23C23)) memcpy((void*)& S23C23, (void*)data, RawDataLen);
          break;
        case 0x3A: //power on time, riding time
          if (RawDataLen == sizeof(A23C3A)) memcpy((void*)& S23C3A, (void*)data, RawDataLen);
          break;
        case 0x7C:
          break;
        case 0x7B:
          break;
        case 0x7D:
          break;
        default:
          break;
      }
      break;
    case 0x25:
      switch (AnswerHeader.cmd) {
        case 0x40: //cells info
          if(RawDataLen == sizeof(A25C40)) memcpy((void*)& S25C40, (void*)data, RawDataLen);
          break;
        case 0x3B:
          break;
        case 0x31: //capacity, remain persent, current, voltage
          if (RawDataLen == sizeof(A25C31)) memcpy((void*)& S25C31, (void*)data, RawDataLen);
          break;
        case 0x20:
          break;
        case 0x1B:
          break;
        case 0x10:
          break;
        default:
          break;
        }
        break;
      default:
        break;
  }

  for (unsigned char i = 0; i < sizeof(_commandsWeWillSend); i++)
    if (AnswerHeader.cmd == _q[_commandsWeWillSend[i]]) {
      _NewDataFlag = 1;
      break;
    }
}

void prepareNextQuery() {
  static unsigned char index = 0;

  _Query._dynQueries[0] = 1;
  _Query._dynQueries[1] = 8;
  _Query._dynQueries[2] = 10;
  _Query._dynQueries[3] = 14;
  _Query._dynSize = 4;

  if (preloadQueryFromTable(_Query._dynQueries[index]) == 0) _Query.prepared = 1;

  index++;

  if (index >= _Query._dynSize) index = 0;
}

unsigned char preloadQueryFromTable(unsigned char index) {
  unsigned char * ptrBuf;
  unsigned char * pp; //pointer preamble
  unsigned char * ph; //pointer header
  unsigned char * pe; //pointer end

  unsigned char cmdFormat;
  unsigned char hLen; //header length
  unsigned char eLen; //ender length

  if (index >= sizeof(_q)) return 1; //unknown index

  if (_Query.prepared != 0) return 2; //if query not send yet

  cmdFormat = pgm_read_byte_near(_f + index);

  pp = (unsigned char*)&_h0;
  ph = NULL;
  pe = NULL;

  switch(cmdFormat) {
    case 1: //h1 only
      ph = (unsigned char*)&_h1;
      hLen = sizeof(_h1);
      pe = NULL;
      break;
    case 2: //h2 + end20
      ph = (unsigned char*)&_h2;
      hLen = sizeof(_h2);

      //copies last known throttle & brake values
      _end20t.hz = 0x02;
      _end20t.th = S20C00HZ65.throttle;
      _end20t.br = S20C00HZ65.brake;
      pe = (unsigned char*)&_end20t;
      eLen = sizeof(_end20t);
      break;
  }

  ptrBuf = (unsigned char*)&_Query.buf;

  memcpy_P((void*)ptrBuf, (void*)pp, sizeof(_h0));  //copy preamble
  ptrBuf += sizeof(_h0);

  memcpy_P((void*)ptrBuf, (void*)ph, hLen);         //copy header
  ptrBuf += hLen;

  memcpy_P((void*)ptrBuf, (void*)(_q + index), 1);  //copy query
  ptrBuf++;

  memcpy_P((void*)ptrBuf, (void*)(_l + index), 1);  //copy expected answer length
  ptrBuf++;

  if (pe != NULL) {
    memcpy((void*)ptrBuf, (void*)pe, eLen);         //if needed - copy ender
    ptrBuf+= hLen;
  }

  //unsigned char
  _Query.DataLen = ptrBuf - (unsigned char*)&_Query.buf[2]; //calculate length of data in buf, w\o preamble and cs
  _Query.cs = calcCs((unsigned char*)&_Query.buf[2], _Query.DataLen);    //calculate cs of buffer

  return 0;
}

void prepareCommand(unsigned char cmd) {
  unsigned char * ptrBuf;

  _cmd.len  =    4;
  _cmd.addr = 0x20;
  _cmd.rlen = 0x03;

  switch(cmd) {
    case CMD_CRUISE_ON:   //0x7C, 0x01, 0x00
      _cmd.param = 0x7C;
      _cmd.value =    1;
      break;
    case CMD_CRUISE_OFF:  //0x7C, 0x00, 0x00
      _cmd.param = 0x7C;
      _cmd.value =    0;
      break;
    case CMD_LED_ON:      //0x7D, 0x02, 0x00
      _cmd.param = 0x7D;
      _cmd.value =    2;
      break;
    case CMD_LED_OFF:     //0x7D, 0x00, 0x00
      _cmd.param = 0x7D;
      _cmd.value =    0;
      break;
    case CMD_WEAK:        //0x7B, 0x00, 0x00
      _cmd.param = 0x7B;
      _cmd.value =    0;
      break;
    case CMD_MEDIUM:      //0x7B, 0x01, 0x00
      _cmd.param = 0x7B;
      _cmd.value =    1;
      break;
    case CMD_STRONG:      //0x7B, 0x02, 0x00
      _cmd.param = 0x7B;
      _cmd.value =    2;
      break;
    default:
      return; //undefined command - do nothing
      break;
  }
  ptrBuf = (unsigned char*)&_Query.buf;

  memcpy_P((void*)ptrBuf, (void*)_h0, sizeof(_h0));  //copy preamble
  ptrBuf += sizeof(_h0);

  memcpy((void*)ptrBuf, (void*)&_cmd, sizeof(_cmd)); //copy command body
  ptrBuf += sizeof(_cmd);

  //unsigned char
  _Query.DataLen = ptrBuf - (unsigned char*)&_Query.buf[2];               //calculate length of data in buf, w\o preamble and cs
  _Query.cs = calcCs((unsigned char*)&_Query.buf[2], _Query.DataLen);     //calculate cs of buffer

  _Query.prepared = 1;
}

void writeQuery() {
  RX_DISABLE;
  XIAOMI_PORT.write((unsigned char*)&_Query.buf, _Query.DataLen + 2);     //DataLen + length of preamble
  XIAOMI_PORT.write((unsigned char*)&_Query.cs, 2);
  RX_ENABLE;
  _Query.prepared = 0;
}

unsigned int calcCs(unsigned char * data, unsigned char len) {
  unsigned int cs = 0xFFFF;
  for (int i = len; i > 0; i--) cs -= *data++;

  return cs;
}

void dataFSM() {
  static unsigned char   step = 0, _step = 0, entry = 1;
  static unsigned long   beginMillis;
  static unsigned char   Buf[RECV_BUFLEN];
  static unsigned char * _bufPtr;
  _bufPtr = (unsigned char*)&Buf;

  switch (step) {
    case 0:                                                             //search header sequence
      while (XIAOMI_PORT.available() >= 2)
        if (XIAOMI_PORT.read() == 0x55 && XIAOMI_PORT.peek() == 0xAA) {
          XIAOMI_PORT.read();                                           //discard second part of header
          step = 1;
          break;
        }
      break;
    case 1: //preamble complete, receive body
      static unsigned char   readCounter;
      static unsigned int    _cs;
      static unsigned char * bufPtr;
      static unsigned char * asPtr;
      unsigned char bt;
      if (entry) {      //init variables
        memset((void*)&AnswerHeader, 0, sizeof(AnswerHeader));
        bufPtr = _bufPtr;
        readCounter = 0;
        beginMillis = millis();
        asPtr = (unsigned char *)&AnswerHeader;   //pointer onto header structure
        _cs = 0xFFFF;
      }
      if (readCounter >= RECV_BUFLEN) {               //overrun
        step = 2;
        break;
      }
      if (millis() - beginMillis >= RECV_TIMEOUT) {   //timeout
        step = 2;
        break;
      }

      while (XIAOMI_PORT.available()) {               //read available bytes from port-buffer
        bt = XIAOMI_PORT.read();
        readCounter++;
        if (readCounter <= sizeof(AnswerHeader)) {    //separate header into header-structure
          *asPtr++ = bt;
          _cs -= bt;
        }
        if (readCounter > sizeof(AnswerHeader)) {     //now begin read data
          *bufPtr++ = bt;
          if(readCounter < (AnswerHeader.len + 3)) _cs -= bt;
        }
        beginMillis = millis();                       //reset timeout
      }

      if (AnswerHeader.len == (readCounter - 4)) {    //if len in header == current received len
        unsigned int   cs;
        unsigned int * ipcs;
        ipcs = (unsigned int*)(bufPtr-2);
        cs = *ipcs;
        if(cs != _cs) {   //check cs
          step = 2;
          break;
        }
        //here cs is ok, header in AnswerHeader, data in _bufPtr
        processPacket(_bufPtr, readCounter);

        step = 2;
        break;
      }
      break; //case 1:
    case 2:  //end of receiving data
      step = 0;
      break;
  }

  if (_step != step) {
    _step = step;
    entry = 1;
  } else entry = 0;
}


void setup() {
  XIAOMI_PORT.begin(115200);

  byte cfgID = EEPROM.read(0);
  if (cfgID = 128) {

  } else {

  }

  u8g2.begin();
  u8g2.setFont(u8g2_font_t0_12b_mf);
  u8g2.setDrawColor(1);
  u8g2.setColorIndex(1);

  do {
    u8g2.drawXBMP(0, 0, m365_logo_width, m365_logo_height, m365_logo_bits);
  } while (u8g2.nextPage());

  unsigned long wait = millis() + 2000;
  while ((wait > millis()) || ((wait - 1000 > millis()) && (S25C31.current != 0) && (S25C31.voltage != 0) && (S25C31.remainPercent != 0))) {
    dataFSM();
    if (_Query.prepared == 0) prepareNextQuery();
    if (_NewDataFlag == 1) {
      _NewDataFlag = 0;
      calculate();
    }
  }

  if ((S25C31.current == 0) && (S25C31.remainPercent == 0) && (S25C31.voltage == 0)) {
    u8g2.firstPage();
    do {
      u8g2.setCursor(0, 8);
      u8g2.println(F("BUS not connected!"));
      u8g2.setCursor(0, 20);
      u8g2.println(F("No data to display!"));
    } while (u8g2.nextPage());
  }
}

void loop() {
  thisMillis = millis();
  if (lastMillis == 0) lastMillis = thisMillis;

  dataFSM();
  keyVal();

  if (_Query.prepared == 0) prepareNextQuery();

  if (_NewDataFlag == 1) {
    _NewDataFlag = 0;
    calculate();
    displayFSM();
    lastMillis = thisMillis;
  } /*else
  if (thisMillis - lastMillis > 200) {
    //displayFSM();
    //lastMillis = thisMillis;
    if ((S25C31.current != 0) && (S25C31.remainPercent != 0) && (S25C31.voltage != 0)) resetFunc();
  }*/
}
