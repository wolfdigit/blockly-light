#include <LWiFi.h>
#include <LWiFiClient.h>

//#define SSID "lssh-ebook-ap"
#define SSID "wolphone"
#define PASSWD "decaddbeef"
//#define HOST "172.16.0.7"
#define HOST "wolfoj.wolfdigit.ga"
#define CHUNK (20)
#define MAXCMD (1024)
const int zpin=7;
const int rpin=2;
const int gpin=4;
const int bpin=5;

LWiFiClient client;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("Hello?");
  LWiFi.begin();
  pinMode(rpin, OUTPUT);
  pinMode(gpin, OUTPUT);
  pinMode(bpin, OUTPUT);
  pinMode(zpin, OUTPUT);
  digitalWrite(zpin, HIGH);
}

typedef struct {
  int r, g, b;
  unsigned long t;
} State;
State prog[MAXCMD] = {(State){255>>2, 0, 0, 1000}, (State){0, 255>>2, 0, 1000}, (State){0, 0, 255>>2, 1000}};
int n=3;

int prevId=0;
void parseMsg(char buff[], int len) {
  while (strncmp(buff, "\r\n\r\n", 4)!=0) buff++;
  buff += 4;
  //Serial.println(buff);
  buff[CHUNK-1] = '\0';
  int newId = atoi(buff);
  //Serial.println(String("msgId=")+String(newId));
  if (newId==prevId) return;
  prevId = newId;

  n = 0;
  for (int i=CHUNK; i<len; i+=CHUNK) {
    char *line = buff+i;
    line[3] = '\0';
    prog[n].r = atoi(line)>>2;
    //Serial.println(String("line=")+String(line));
    line[7] = '\0';
    prog[n].g = atoi(line+4)>>2;
    line[11] = '\0';
    prog[n].b = atoi(line+8)>>2;
    line[CHUNK-1] = '\0';
    prog[n].t = atoi(line+12);
    //Serial.println(String("t-line=")+String(line+12));
    //Serial.println(String("t-line=")+String(line+12));
    if (prog[n].t==0) break;
    ++n;
  }
}

unsigned char buff[MAXCMD*CHUNK];
void updateIn() {
  Serial.println("updateIn");
  digitalWrite(rpin, LOW);
  digitalWrite(gpin, HIGH);
  digitalWrite(bpin, HIGH);
  // put your main code here, to run repeatedly:
  if (LWiFi.status()==LWIFI_STATUS_DISCONNECTED) {
    Serial.println("connect wifi");
#ifndef PASSWD
    LWiFi.connect(SSID);
#else
    LWiFi.connectWPA(SSID, PASSWD);
#endif
  }
  Serial.println("wifi OK");
  client.connect(HOST, 80);
  client.println("GET /lite2.php HTTP/1.1"); 
  client.println("Host: " HOST);
  client.println("Connection: close");
  client.println();

  delay(100);

  int len;
  len = client.read(buff, sizeof(buff));
  buff[len] = '\0';
  Serial.println(String("len=")+String(len));
  //Serial.println((char*)buff);
  parseMsg((char*)buff, len);
  Serial.println(String("n=")+String(n));
}

unsigned long cnt=0;
unsigned long idx=0;
int r, g, b;
void calc() {
  if (n==0) {
    r=0; g=0; b=0;
    return;
  }
  double rate = (double)cnt/(double)prog[(idx)%n].t;
  //Serial.println(rate);
  r = prog[(idx)%n].r*(1.0-rate) + prog[(idx+1)%n].r*rate;
  g = prog[(idx)%n].g*(1.0-rate) + prog[(idx+1)%n].g*rate;
  b = prog[(idx)%n].b*(1.0-rate) + prog[(idx+1)%n].b*rate;
  //Serial.println(String(r)+String("-")+String(g)+String("-")+String(b));
  cnt++;
  if (cnt>=prog[(idx)%n].t) {
    idx++;
    cnt = 0;
    //Serial.println(String("idx=")+String(idx)+String(", nprog=")+String(idx%n));
  }
  //analogWrite(rpin, r);
  //analogWrite(gpin, g);
  //analogWrite(bpin, b);
}

unsigned long tick=0;
void loop() {
  unsigned long smicro = micros();
  unsigned long colorTick = tick&0x3f;
  //Serial.println(smicro);
  if ((tick&0x00003f)==0) {
    if ((tick&0x0fffff)==0 || n==0) updateIn();
    calc();
  }

  if (colorTick==0) {
    digitalWrite(rpin, HIGH);
    digitalWrite(gpin, HIGH);
    digitalWrite(bpin, HIGH);
  }
  if (colorTick==r) digitalWrite(rpin, LOW);
  if (colorTick==g) digitalWrite(gpin, LOW);
  if (colorTick==b) digitalWrite(bpin, LOW);

  tick++;
  unsigned long dmicro = micros()-smicro;
  //Serial.println(dmicro);
  if (dmicro<16) delayMicroseconds(16-dmicro);
  //delayMicroseconds(100000-(micros()-smicro));
}
