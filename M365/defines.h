#include <EEPROM.h>
#include <U8g2lib.h>
#include <Wire.h>

#include "M365.h"

U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

//#define CUSTOM_WHELL_SIZE;

#ifdef CUSTOM_WHELL_SIZE
  const int WHELL_SIZE = 100; //10"
#endif

const unsigned int LONG_PRESS = 2000;
const unsigned int HIBERNATE_PRESS = 8000;

const int MIN_BRAKE = 50;
const int MAX_BRAKE = 130;

byte warnBatteryPercent = 5;

bool autoBig = true;
byte bigMode = 0;
bool bigWarn = true;

bool Settings = false;
int menuPos = 0;

volatile int oldBrakeVal = -1;
volatile int oldThrottleVal = -1;


const int MIN_THROTTLE = 50;
const int MAX_THROTTLE = 150;

struct {
  byte battWarnPercent = 15;
} conf;

// -1 = min; 0 = middle; 1 = max
struct {
  int brake = -1;
  int throttle = -1;
  bool brakeChange = false;
  bool throttleChange = false;
  unsigned long kTimer = 0;
} kVal;

struct {
  unsigned int  curh;
  unsigned int  curl;
  unsigned int  vh;
  unsigned int  vl;
  unsigned int  sph;
  unsigned int  spl;
  unsigned int  milh;
  unsigned int  mill;
  unsigned int  aveh;
  unsigned int  avel;
  char          remPercent;
  int           remCapacity; 
  int           voltage; 
  unsigned int  milTotH;
  unsigned char milTotL; 
  unsigned int  tripMin;
  unsigned int  tripSec;
  unsigned char powerMin;
  unsigned char powerSec; 
  unsigned int  mTemp;
  unsigned int  bTemp1;
  unsigned int  bTemp2;
} m365_info;

unsigned long thisMillis = 0;
unsigned long lastMillis = 0;

void (* resetFunc) (void) = 0; 

// -----------------------------------------------------------------------------------------------------------

#define XIAOMI_PORT Serial
#define RX_DISABLE UCSR0B &= ~_BV(RXEN0);
#define RX_ENABLE  UCSR0B |=  _BV(RXEN0);

struct {
  unsigned char prepared; //1 if prepared, 0 after writing
  unsigned char DataLen;  //lenght of data to write
  unsigned char buf[16];  //buffer
  unsigned int  cs;       //cs of data into buffer
  unsigned char _dynQueries[5];
  unsigned char _dynSize = 0;
} _Query;

volatile unsigned char _NewDataFlag = 0; //assign '1' for renew display once
volatile bool _Hibernate = false;   //disable requests. For flashing or other things

enum {CMD_CRUISE_ON, CMD_CRUISE_OFF, CMD_LED_ON, CMD_LED_OFF, CMD_WEAK, CMD_MEDIUM, CMD_STRONG};
struct __attribute__((packed)) CMD {
  unsigned char len;
  unsigned char addr;
  unsigned char rlen;
  unsigned char param;
  int           value;
} _cmd;

const unsigned char _commandsWeWillSend[] = {1, 8, 10, 14}; //insert INDEXES of commands, wich will be send in a circle

        // INDEX                     //0     1     2     3     4     5     6     7     8     9    10    11    12    13    14
const unsigned char _q[] PROGMEM = {0x3B, 0x31, 0x20, 0x1B, 0x10, 0x1A, 0x69, 0x3E, 0xB0, 0x23, 0x3A, 0x7B, 0x7C, 0x7D, 0x40}; //commands
const unsigned char _l[] PROGMEM = {   2,   10,    6,    4,   18,   12,    2,    2,   32,    6,    4,    2,    2,    2,   30}; //expected answer length of command
const unsigned char _f[] PROGMEM = {   1,    1,    1,    1,    1,    2,    2,    2,    2,    2,    2,    2,    2,    2,    1}; //format of packet

//wrappers for known commands
const unsigned char _h0[]    PROGMEM = {0x55, 0xAA};
const unsigned char _h1[]    PROGMEM = {0x03, 0x22, 0x01};
const unsigned char _h2[]    PROGMEM = {0x06, 0x20, 0x61};
const unsigned char _hc[]    PROGMEM = {0x04, 0x20, 0x03}; //head of control commands

struct __attribute__ ((packed)){ //dynamic end of long queries
  unsigned char hz; //unknown
  unsigned char th; //current throttle value
  unsigned char br; //current brake value
} _end20t;

const unsigned char RECV_TIMEOUT =  5;
const unsigned char RECV_BUFLEN  = 64;

struct __attribute__((packed)) ANSWER_HEADER{ //header of receiving answer
  unsigned char len;
  unsigned char addr;
  unsigned char hz;
  unsigned char cmd;
} AnswerHeader;

struct __attribute__ ((packed)) {
  unsigned char state;      //0-stall, 1-drive, 2-eco stall, 3-eco drive
  unsigned char ledBatt;    //battery status 0 - min, 7(or 8...) - max
  unsigned char headLamp;   //0-off, 0x64-on
  unsigned char beepAction;
} S21C00HZ64;

struct __attribute__((packed))A20C00HZ65 {
  unsigned char hz1;
  unsigned char throttle; //throttle
  unsigned char brake;    //brake
  unsigned char hz2;
  unsigned char hz3;
} S20C00HZ65;

struct __attribute__((packed))A25C31 {
  unsigned int  remainCapacity;     //remaining capacity mAh
  unsigned char remainPercent;      //charge in percent
  unsigned char u4;                 //battery status??? (unknown)
  int           current;            //current        /100 = A
  int           voltage;            //batt voltage   /100 = V
  unsigned char temp1;              //-=20
  unsigned char temp2;              //-=20
} S25C31;

struct __attribute__((packed))A25C40 {
  int c1; //cell1 /1000
  int c2; //cell2
  int c3; //etc.
  int c4;
  int c5;
  int c6;
  int c7;
  int c8;
  int c9;
  int c10;
  int c11;
  int c12;
  int c13;
  int c14;
  int c15;
} S25C40;

struct __attribute__((packed))A23C3E {
  int i1;                           //mainframe temp
} S23C3E;

struct __attribute__((packed))A23CB0 {
  //32 bytes;
  unsigned char u1[10];
  int           speed;              // /1000
  unsigned int  averageSpeed;       // /1000
  unsigned long mileageTotal;       // /1000
  unsigned int  mileageCurrent;     // /100
  unsigned int  elapsedPowerOnTime; //time from power on, in seconds
  int           mainframeTemp;      // /10
  unsigned char u2[8];
} S23CB0;

struct __attribute__((packed))A23C23 { //skip
  unsigned char u1;
  unsigned char u2;
  unsigned char u3; //0x30
  unsigned char u4; //0x09
  unsigned int  remainMileage;  // /100 
} S23C23;

struct __attribute__((packed))A23C3A {
  unsigned int powerOnTime;
  unsigned int ridingTime;
} S23C3A;
