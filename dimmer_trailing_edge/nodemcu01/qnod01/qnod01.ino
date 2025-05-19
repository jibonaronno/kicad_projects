
/*
 * Timer1 ISR Document
 * https://circuits4you.com/2018/01/02/esp8266-timer-ticker-example/
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>
#include <ESP8266WebServer.h>
#include <Hash.h>
#include <Ticker.h> //Not used here

#define ZERO_CROSSING_INT_PIN 12
#define DELTA 4                //(t_zero_crossing - t_interrupt)/STEP_TIME
#define STEP_TIME 78          //for 128 lvls (in uS) (65 for 50 Hz)

const char* ssid     = "ENERGYPAC";
const char* password = "tulip2022";
WebSocketsServer webSocket = WebSocketsServer(81);

volatile boolean zero_cross = 0;
int zcount = 0;

volatile boolean L1 = 0;
int dmrCounter = 0;
int dmrCounterLimit = 0;

int timer1counter = 0;

Ticker ticker;

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED: {
            IPAddress ip = webSocket.remoteIP(num);
            Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

            // send message to client
            webSocket.sendTXT(num, "Connected");
        }
            break;
        case WStype_TEXT:
            Serial.printf("[%u] get Text: %s\n", num, payload);
            if(payload[0] == '#') {
                // we get RGB data

                // decode rgb data
                uint32_t rgb = (uint32_t) strtol((const char *) &payload[1], NULL, 16);
                //analogWrite(LED_RED,    ((rgb >> 16) & 0xFF));
                //analogWrite(LED_GREEN,  ((rgb >> 8) & 0xFF));
                //analogWrite(LED_BLUE,   ((rgb >> 0) & 0xFF));
            }

            String text = String((char *) &payload[0]);
            
            if(text == "ON_0")
            {
              //digitalWrite(D2, HIGH);
              L1 = 1;
            }
            else if(text == "OFF_0")
            {
              //digitalWrite(D2, LOW);
              L1 = 0;
            }

            if(text.startsWith("a"))
            {
              String aVal=(text.substring(text.indexOf("a")+1,text.length())); 
              dmrCounterLimit = aVal.toInt();
            }
            
            break;
    }
}

void ICACHE_RAM_ATTR dimTimerISR()
{
  if(zero_cross == 1)
  {
    if(L1 == 1)
    {
      if(dmrCounter < dmrCounterLimit)
      {
        dmrCounter++;
      }
      else
      {
        dmrCounter = 0;
        zero_cross = 0;
        digitalWrite(D2, LOW);
      }
    }
    else
    {
      digitalWrite(D2, LOW);
    }
  }
  timer1_write(1000);

  if(timer1counter < 4)
  {
    timer1counter++;
  }
  else
  {
    timer1counter = 0;
    //ESP.wdtFeed();
  }
}

void setup() {
  // put your setup code here, to run once:
  digitalWrite(D2, LOW);
  pinMode(D2, OUTPUT);
  pinMode(ZERO_CROSSING_INT_PIN,INPUT);
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  noInterrupts();
  timer1_attachInterrupt(dimTimerISR);
  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
  timer1_write(1000); //120000 us
  
  attachInterrupt(ZERO_CROSSING_INT_PIN,Zero_Crossing_Int,RISING);
  interrupts();

}

void Zero_Crossing_Int()
{
  zero_cross = 1;
  if(L1 == 1)
  {
    digitalWrite(D2, HIGH);
  }
  
  if(zcount < 500)
  {
    zcount++;
  }
}


void loop() {
  // put your main code here, to run repeatedly:
  webSocket.loop();
  if(zcount >= 500)
  {
    Serial.println("AC LINE DETECTED");
    zcount = 0;
  }
  yield();
}
