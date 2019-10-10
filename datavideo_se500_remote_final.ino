#include <LEDMatrixDriver.hpp>

#define key_main_A 3
#define key_main_B 4
#define key_main_C 5
#define key_main_D 6

#define BAUDRATE 57600
#define UDELAY 100
#define DBC_DELAY 30

#define LEDMATRIX_SEGMENTS 1
#define LEDMATRIX_WIDTH 8
#define LEDMATRIX_CS_PIN 9

#define ASK_DELAY 10000

LEDMatrixDriver lmd(LEDMATRIX_SEGMENTS, LEDMATRIX_CS_PIN);

byte a[8] = { B00111100,
              B01000010,
              B01000010,
              B01111110,
              B01000010,
              B01000010,
              B01000010,
              B01000010
            };
byte b[8] = { B01111100,
              B01000010,
              B01000010,
              B01111100,
              B01000010,
              B01000010,
              B01000010,
              B01111100
            };
byte c[8] = { B00111100,
              B01000010,
              B01000000,
              B01000000,
              B01000000,
              B01000010,
              B01000010,
              B00111100
            };
byte d[8] = { B01111100,
              B01000010,
              B01000010,
              B01000010,
              B01000010,
              B01000010,
              B01000010,
              B01111100
            };
byte e[8] = { B01111110,
              B01000000,
              B01000000,
              B01111100,
              B01000000,
              B01000000,
              B01000000,
              B01111110
            };
byte f[8] = { B01111110,
              B01000000,
              B01000000,
              B01111100,
              B01000000,
              B01000000,
              B01000000,
              B01000000
            };
byte g[8] = { B00111100,
              B01000010,
              B01000000,
              B00001110,
              B01000010,
              B01000010,
              B01000010,
              B00111100
            };
byte h[8] = { B01000010,
              B01000010,
              B01000010,
              B01111110,
              B01000010,
              B01000010,
              B01000010,
              B01000010
            };
byte x[8] = { B01000010,
              B01000010,
              B00100100,
              B00011000,
              B00100100,
              B01000010,
              B01000010,
              B01000010
            };

typedef struct {
  byte num;
  bool val;
  unsigned long tim;
  byte cod;
} s_button;

bool dbc(s_button *);
void send_bytes(byte*, byte);
void send_key_code(s_button *);
void ask_for_state(void);
void clrbuf(void); //clear input buffer
void parse_serial_input(void);
byte chksum(byte*, byte);
byte get_chksum(byte, byte);
void drawSprite(byte*, int, int, int, int);

s_button b1, b2, b3, b4;
byte buf[256]; //input buffer
unsigned long lastAsk;
bool ask_again;

void setup() {
  ask_again = false;
  Serial.begin(BAUDRATE, SERIAL_8O1);
  Serial.setTimeout(100);

  pinMode(key_main_A, INPUT_PULLUP);
  pinMode(key_main_B, INPUT_PULLUP);
  pinMode(key_main_C, INPUT_PULLUP);
  pinMode(key_main_D, INPUT_PULLUP);

  b1 = {key_main_A, HIGH, millis(), 0x01};
  b2 = {key_main_B, HIGH, millis(), 0x02};
  b3 = {key_main_C, HIGH, millis(), 0x03};
  b4 = {key_main_D, HIGH, millis(), 0x04};

  lastAsk = millis();

  lmd.setEnabled(true);
  lmd.setIntensity(0);
  drawSprite( (byte*)&a, 0, 0, 8, 8);
  lmd.display();
}

void loop() {
  if (dbc(&b1) && b1.val == LOW) {
    send_key_code(&b1);
  }
  if (dbc(&b2) && b2.val == LOW) {
    send_key_code(&b2);
  }
  if (dbc(&b3) && b3.val == LOW) {
    send_key_code(&b3);
  }
  if (dbc(&b4) && b4.val == LOW) {
    send_key_code(&b4);
  }
  if (millis() - lastAsk > ASK_DELAY || ask_again) ask_for_state();
  if (Serial.available() > 0) parse_serial_input();
  Serial.flush();
}

bool dbc(s_button * btn) {
  bool s = digitalRead(btn->num);
  if ((btn->val == s)) {
    btn->tim = millis();
  }
  else if (millis() - btn->tim > UDELAY) {
    btn->val = s;
    return true; // value changed
  }
  return false; // value unchanged
}

void send_bytes(uint8_t * bytes, uint8_t len)
{
  Serial.flush(); // wait for ongoing transmision to end (e.g. from Serial.println())
  byte i = 0;
  while (i < len)
  {
    Serial.write(bytes[i]);
    Serial.flush(); // wait for end of byte transmision.
    //delayMicroseconds(UDELAY); // delay 100us between bytes
    i++;
  }
  Serial.flush();
}

void send_key_code(s_button * btn)
{
  byte bytes[9];
  bytes[0] = 0xF0; //header
  bytes[1] = 0x32; //id
  bytes[2] = 0x09; //length
  bytes[3] = 0x05; //control command
  bytes[4] = 0x01; //key command
  bytes[5] = btn->cod;
  byte checksum = chksum(bytes, 9);
  bytes[6] = checksum % 16 + 0x30;
  bytes[7] = checksum / 16 + 0x30;
  bytes[8] = 0xFF;
  send_bytes(bytes, 9);
}
void ask_for_state(void)
{
  byte bytes[8];
  bytes[0] = 0xF0; //header
  bytes[1] = 0x32; //id
  bytes[2] = 0x08; //length
  bytes[3] = 0x05; //control command
  bytes[4] = 0x00; //ask status
  byte checksum = chksum(bytes, 8);
  bytes[5] = checksum % 16 + 0x30;
  bytes[6] = checksum / 16 + 0x30;
  bytes[7] = 0xFF;
  send_bytes(bytes, 8);
  ask_again = false;
  lastAsk = millis();
}
void clrbuf(void) {
  for (int i = 0; i < 256; i++) {
    buf[i] = '\0';
  }
}
//\h(fc 32 11 05 00 01 10 00 07 41 42 01 00 20 30 30 ff)
void parse_serial_input() {
  clrbuf(); //clear buffer for sanity
  ask_again = false;
  int n = Serial.readBytes(buf, 255); //read until 0xFF, or max 255 bytes
  byte* start = buf;
  if (start[0] == 0xFC && start[1] == 0x32) {
    byte len = start[2];
    if (chksum(start, len) == get_chksum(start, len)) {
      if (len == 0x11 && start[3] == 0x05) {
        byte LED = start[9] % 16;
        if (LED == 1) {
          drawSprite( (byte*)&a, 0, 0, 8, 8);
          lmd.display();
        }
        else if (LED == 2) {
          drawSprite( (byte*)&b, 0, 0, 8, 8);
          lmd.display();
        }
        else if (LED == 4) {
          drawSprite( (byte*)&c, 0, 0, 8, 8);
          lmd.display();
        }
        else if (LED == 8) {
          drawSprite( (byte*)&d, 0, 0, 8, 8);
          lmd.display();
        }
        else { // wrong led value
          //drawSprite( (byte*)&e, 0, 0, 8, 8);
          //lmd.display();
          ask_again = true;
        }
      } else { // wrong answer
        //drawSprite( (byte*)&f, 0, 0, 8, 8);
        //lmd.display();
        ask_again = true;
      }
    } else { // wrong checksum
      //drawSprite( (byte*)&g, 0, 0, 8, 8);
      //lmd.display();
      ask_again = true;
    }
  } else { // wrong frame
    //drawSprite( (byte*)&h, 0, 0, 8, 8);
    //lmd.display();
    ask_again = true;
  }
}
byte chksum(byte * bytes, byte len) {
  byte checksum = 0;
  for (int i = 0; i < len - 3; i++) {
    checksum += bytes[i];
  }
  return checksum;
}

byte get_chksum(byte * bytes, byte len) {
  byte L = bytes[len - 3];
  byte H = bytes[len - 2];
  return (H % 16 * 16) + (L % 16);
}

void drawSprite( byte * sprite, int x, int y, int width, int height )
{
  byte mask = B10000000;
  for ( int iy = 0; iy < height; iy++ )
  {
    for ( int ix = 0; ix < width; ix++ )
    {
      lmd.setPixel(x + ix, y + iy, (bool)(sprite[iy] & mask ));
      mask = mask >> 1;
    }
    mask = B10000000;
  }
}
