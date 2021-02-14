#include <ESP8266WiFi.h>                                      // esp8266 wifi library

/* hardware paramaters */
#define PIN_LED 2                                             // pin to which status led is connected
#define PIN_INPUT 4                                           // pin to which input data source is connected

/* connection paramaters */
#define MAX_CONN_ATTEMPTS 2                                   // maximum number of api/ntp connection attempts
#define MAX_CONN_TIME 10                                      // maxiumum time in seconds to attempt wifi connection
#define DELAY_ERROR 2000                                      // time in milliseconds between subsequent post attempts on error result
#define HTTP_TIMEOUT 1000                                     // time in milliseconds to wait for http response

/* wifi paramaters */
static const char* WIFI_SSID = "yourssidhere";                // your wireless network name (SSID)
static const char* WIFI_PASS = "yourpasswordhere";            // your Wi-Fi network password

/* individual paramaters (change per device as required) */
#define SENSOR_INDEX 1                                        // sensor index used as field id of temperature
#define STATUS_FIELD 2                                        // field id of sensor status
#define POST_PERIOD 900                                       // total time period between posts
#define POST_ERROR 150                                        // total time period between posts if previous post was unsuccessful
#define LED_STATUS 1                                          // enable onboard led status display

/* api paramaters */
#define API_KEY "API_WRITE_KEY_HERE"
#define API_HOST "api.thingspeak.com"
#define API_BODY "GET /update?api_key=%s&field%u=%u"          // standard body for api, includes key and unsigned int as first Field
#define API_FIELD_FLOAT "&field%i=%.2f"                       // additional api field to append float value

ADC_MODE(ADC_VCC);

/* ip address for static assignment */
IPAddress ip(10, 1, 1, 200);                                  // starting ip address (incremented by sensor index)
IPAddress subnet(255, 255,255, 0);                            // subnet mask
IPAddress gateway(10, 1, 1, 1);                               // default gateway
IPAddress dns(1, 1, 1, 1);                                    // dns server

/* connect to specified wifi ssid */
bool connect_wifi(const char* ssid, const char* psk) {
  Serial.print("\r\nConneting to Wi-Fi");

  WiFi.mode(WIFI_STA);                                        // set wifi to station mode
  WiFi.config(ip, dns, gateway, subnet);                      // configure static IP for faster connection (comment out for dhcp)
  WiFi.begin(ssid, psk);                                      // begin wifi connection

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(50);
    if (micros() / 1000000 > MAX_CONN_TIME) {                 // abort connection attempt if maximum connection time exceeded
      return 0;                                               // return indicating failure
    }
  }

  Serial.println(" ok!");
  return 1;                                                   // return indicating success
}

/* enter deep sleep for pre-defined period */
void go_to_sleep(uint32_t period = POST_PERIOD) {
  uint32_t sleep_period;                                      // duration of deep sleep in milliseconds

  sleep_period = (period * 1000000);
  Serial.printf("\r\nEntering deep sleep for %.2f seconds...\r\n", sleep_period / 1000000.0);
  ESP.deepSleep(sleep_period, WAKE_NO_RFCAL);
}

/* post data to thingspeak via http */
bool post_data(const char* data) {
  uint8_t failure_count = 0;                                  // number of consecutive http failures
  uint8_t response_length;                                    // length of http response
  uint32_t http_response = 0;                                 // response from http request (for thingspeak, this is always numerical)
  uint32_t timeout;                                           // response timeout limit
  WiFiClient client;                                          // wifi client used to create tcp connection

  while (failure_count++ < MAX_CONN_ATTEMPTS) {
    Serial.println("\r\nConnecting to API...");

    if (client.connect(API_HOST, 80)) {                       // connect to thingspeak api
      Serial.println(data);
      client.println(data);                                   // post data to api
      timeout = millis() + HTTP_TIMEOUT;                      // timeout for http response

      while (millis() < timeout) {                            // start http timeout
        response_length = client.available();                 // wait for http response length
        if (response_length > 0) {                            // response is ready
          char response_string[response_length];
          client.readBytes(response_string, response_length); // read http response
          http_response = atoi(response_string);              // convert response to int
          break;                                              // response received, we're done here
        }
      }                                                       // timed out

      client.stop();                                          // stop client; timeout reached or data received at this point

      if (http_response) {                                    // validate response from api
        Serial.printf("Response: %u\r\n", http_response);
        return 1;
      }
    }

    Serial.println("Connection or response error!");
    if (failure_count < MAX_CONN_ATTEMPTS) {
      delay(DELAY_ERROR);                                     // delay between subsequent attempts
    }
  }

  return 0;                                                   // conection failed or timed out waiting for response
}

void setup() {
  bool pin_state;                                             // state of pin to be measured
  char api_command[64];                                       // char array containing full http api command
  float voltage;                                              // device voltage as read from adc

  /* perform setup */
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_INPUT, INPUT_PULLUP);
  digitalWrite(PIN_LED, 1);                                   // turn led off
  Serial.begin(74880);

  /* collect sensor data */
  pin_state = !digitalRead(PIN_INPUT);                        // read inverted pin state
  sprintf(api_command, API_BODY, API_KEY, SENSOR_INDEX, pin_state);

  /* collect voltage data */
  voltage = ESP.getVcc() / 1024.0;
  char status[12];
  sprintf(status, API_FIELD_FLOAT, STATUS_FIELD, voltage);
  strcat(api_command, status);                                // append field to api body

  /* output data to serial */
  Serial.printf("\r\nPin State: %u", pin_state);
  Serial.printf("\r\nVoltage: %.2f\r\n", voltage);

  /* connect to wifi once everything else done */
  connect_wifi(WIFI_SSID, WIFI_PASS);

  /* post data to thingspeak */
  digitalWrite(PIN_LED, !LED_STATUS);                         // turn on led if enabled
  if (post_data(api_command) == 0) {
    go_to_sleep(POST_ERROR);                                  // shorter sleep cycle on error
  }
  digitalWrite(PIN_LED, 1);

  /* enter deep sleep until next reading */
  go_to_sleep();
}

void loop() {}
