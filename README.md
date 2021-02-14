## ThingSpeak Basic Pin Monitor for ESP8266
A minimalist implementation of input state reporting to ThingSpeak using REST.

 - No external dependencies other than `ESP8266WiFi.h`
 - Lean and efficient HTTP POST to REST endpoint
 - Monitors and reports ESP internal VCC for alerting on low voltage conditions
 - Deep sleep between readings for minimum power consumption
 - Configurable shorter sleep interval on error result
 - Easily adapted for multiple inputs

The code contains detailed comments and is pretty self-explanatory.  The HTTP POST function is tested only with ThingSpeak and will likely get you mixed results if used with other REST endpoints.
