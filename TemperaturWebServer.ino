#include "esp32-mqtt.h"
#include "Arduino_JSON.h"

String jsonstring = "";
JSONVar myObject;


const int pin_temp = 4;
const int pin_lys = 12;
const int pin_vifte = 14;
const int pin_vanning = 27;

int max_temp = 21;
int min_temp = 19;

int lysStatus = 0;
int varmeStatus = 0;
int vanningStatus = 0;

String header;
 
DHT dht(pin_temp, DHT11);



// Set web server port number to 80
WiFiServer server(80);
unsigned long currentTime = millis();
unsigned long previousTime = 0; 
unsigned long currentLoopTime = millis();
unsigned long previousLoopTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

void setup() {
  
  Serial.begin(115200); 
  pinMode(pin_lys, OUTPUT);
  pinMode(pin_vifte, OUTPUT);
  pinMode(pin_vanning, OUTPUT);
  
  dht.begin();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());

  setupCloudIoT();
  
  server.begin();

}

String lookupString(int val) {
  if(val==1)
    return("P&aring;");
  else
    return("Av");

}

void loop() {
    currentLoopTime = millis();
    if(currentLoopTime > (previousLoopTime+60000)) {
      mqtt->loop();
      delay(10);  // <- fixes some issues with WiFi stability
    
      if (!mqttClient->connected()) {
        connect();
      }
      myObject["device"] = device_id;

      float temp = dht.readTemperature();
      float humidity = dht.readHumidity();
      if(temp>max_temp){
              Serial.println("Skrur av vifte fordi det er over 20 grader");
              varmeStatus = 0;
              digitalWrite(pin_vifte, LOW);
              myObject["sensor"] = "OppvarmingEndring";
              myObject["value"] = 0;
              mqtt->publishTelemetry(JSON.stringify(myObject));
      }
      if(temp<min_temp){
              Serial.println("Skrur p&aring; vifte fordi det er under 18 grader");
              varmeStatus = 1;
              digitalWrite(pin_vifte, HIGH);
              myObject["sensor"] = "OppvarmingEndring";
              myObject["value"] = 1;
              mqtt->publishTelemetry(JSON.stringify(myObject));
      }
      previousLoopTime = currentLoopTime;
      
      myObject["sensor"] = "Temperatur";
      myObject["value"] = temp;
      mqtt->publishTelemetry(JSON.stringify(myObject));

      myObject["sensor"] = "Luftfuktighet";
      myObject["value"] = humidity;
      mqtt->publishTelemetry(JSON.stringify(myObject));

      myObject["sensor"] = "Oppvarming";
      myObject["value"] = varmeStatus;
      mqtt->publishTelemetry(JSON.stringify(myObject));

      myObject["sensor"] = "Vanning";
      myObject["value"] = vanningStatus;
      mqtt->publishTelemetry(JSON.stringify(myObject));

      myObject["sensor"] = "Lys";
      myObject["value"] = lysStatus;
      mqtt->publishTelemetry(JSON.stringify(myObject));

    }

  
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // turns the GPIOs on and off
            if (header.indexOf("GET /lys/on") >= 0) {
              Serial.print("Lys (");
              Serial.print(pin_lys);
              Serial.println(") settes til HIGH");
              lysStatus = 1;
              digitalWrite(pin_lys, HIGH);
            } else if (header.indexOf("GET /lys/off") >= 0) {
              Serial.print("Lys (");
              Serial.print(pin_lys);
              Serial.println(") settes til av");
              lysStatus = 0;
              digitalWrite(pin_lys, LOW);
            }  
            // turns the GPIOs on and off
            if (header.indexOf("GET /varme/on") >= 0) {
              Serial.print("Varmeovn (");
              Serial.print(pin_vifte);
              Serial.println(") settes til HIGH");
              varmeStatus = 1;
              digitalWrite(pin_vifte, HIGH);
            } else if (header.indexOf("GET /varme/off") >= 0) {
              Serial.print("Varmeovn (");
              Serial.print(pin_vifte);
              Serial.println(") settes til av");
              varmeStatus = 0;
              digitalWrite(pin_vifte, LOW);
            }  
            // turns the GPIOs on and off
            if (header.indexOf("GET /vanning/on") >= 0) {
              Serial.print("Vanning (");
              Serial.print(pin_vanning);
              Serial.println(") settes til HIGH");
              vanningStatus = 1;
              digitalWrite(pin_vanning, HIGH);
            } else if (header.indexOf("GET /vanning/off") >= 0) {
              Serial.print("Vanning (");
              Serial.print(pin_vanning);
              Serial.println(") settes til av");
              vanningStatus = 0;
              digitalWrite(pin_vanning, LOW);
            }  

            
            int soil = analogRead(36);
            float h = dht.readHumidity();
            float t = dht.readTemperature();
            float hic = dht.computeHeatIndex(t, h, false);
          
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>Anders' kjellerrom</h1>");
            client.print("<br>Temperatur:");
            client.println(t);
            client.print("<br>Luftfuktighet:");
            client.println(h);
            client.print("<br>Jordfuktighet:");
            client.println(soil);

            // Display current state, and ON(1)/OFF(0) buttons for GPIO 26  
            client.println("<p>Lys status: " + lookupString(lysStatus) + "</p>");
            client.println("<p>Varme status: " + lookupString(varmeStatus) + "</p>");
            client.print("<p>Vanning status: " + lookupString(vanningStatus) + "</p>");

            if (lysStatus==0) {
              client.println("<p><a href=\"/lys/on\"><button class=\"button\">Skru p&aring; lys</button></a></p>");
            } else {
              client.println("<p><a href=\"/lys/off\"><button class=\"button button2\">Skru av lys</button></a></p>");
            } 
            if (varmeStatus==0) {
              client.println("<p><a href=\"/varme/on\"><button class=\"button\">Skru p&aring; vifteovn</button></a></p>");
            } else {
              client.println("<p><a href=\"/varme/off\"><button class=\"button button2\">Skru av vifteovn</button></a></p>");
            } 
            if (vanningStatus==0) {
              client.println("<p><a href=\"/vanning/on\"><button class=\"button\">Skru p&aring; vanning</button></a></p>");
            } else {
              client.println("<p><a href=\"/vanning/off\"><button class=\"button button2\">Skru av vanning</button></a></p>");
            } 
               
            client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }  
}
