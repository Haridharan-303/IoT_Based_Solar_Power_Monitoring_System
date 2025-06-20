#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT11.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C address 0x27 (from DIYables LCD), 16 column and 2 rows

const int voltageSensor = 35; // Analog input pin for voltage sensor
const int currentSensor = 36; // Analog input pin for current sensor
const int LDR_Sensor = 34; // Analog input pin for light sensor
DHT11 dht11(2); // DHT11 sensor connected to Digital I/O Pin 2

float vOUT = 0.0;
float vIN = 0.0;
float current;
float R1 = 30000.0;
float R2 = 7500.0;
float R3 = 6800.0;
float R4 = 12000.0;

String apiKey = "Thinspeak API KEY";    // Enter your apiKey
const char* ssid = "WIFI Name";  // Enter your WiFi Network's SSID
const char* pass = "WIFI Password";  // Enter your WiFi Network's Password
const char* server = "api.thingspeak.com";

// Telegram BOT Token (Get from Botfather)
#define BOT_TOKEN "Enter your Bot token"
#define CHAT_ID "Enter your char id"

WiFiClientSecure secured_client;

UniversalTelegramBot bot(BOT_TOKEN, secured_client);

WiFiClient client;

const unsigned long BOT_MTBS = 500; // mean time between scan messages
unsigned long bot_lasttime; // last time messages' scan has been done
int lightValue;
int temperature;

void setup() {
  Serial.begin(9600); // Initialize serial communication for debugging


  // Initialize LCD
  lcd.init();
  lcd.backlight(); // Turn on the backlight

  // Display "Solar Power Monitor" on LCD
  lcd.setCursor(2, 0);
  lcd.print("Solar Power");
  lcd.setCursor(4, 1);
  lcd.print("Monitor");
  delay(2000); // Display message for 2 seconds
  lcd.clear(); // Clear previous display
  lcd.setCursor(6, 0);
  lcd.print("Wifi");
  lcd.setCursor(1, 1);
  lcd.print("Connecting....");

  delay(500);
  Serial.print("Connecting to SSID :");
  Serial.print(ssid);
  WiFi.begin(ssid, pass);

  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
 
  while (WiFi.status() != WL_CONNECTED) 
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.println("WiFi connected");
}

void loop() {
  
  // Read voltage from solar panel & Print
  int value = analogRead(voltageSensor);
  vOUT = (value * 5.0) / 1165.0; // Input voltage to the sensor
  vIN = vOUT * (R2 / (R1 + R2)); // Output voltage to the microcontroller
  Serial.print("Solar Panel Voltage: ");
  Serial.print(vOUT);
  Serial.println(" V");


  // Read current from solar panel & Print
  int adc = analogRead(currentSensor);
  float adc_voltage = adc * (3.3 / 4096.0);
  float voltage = (adc_voltage * (R3 + R4) / R4);
  current = (voltage - 2.5) / 0.185;
  if (current < 0.1) {
    current = 0;
  }
  Serial.print("Current : ");
  Serial.print(current);
  Serial.println(" A");
  

  // Read light intensity & Print
  lightValue = analogRead(LDR_Sensor);
  Serial.print("Light Intensity: ");
  Serial.println(lightValue);


  // Attempt to read temperature value from DHT11 sensor
  temperature = dht11.readTemperature();
  // Check the result of the reading
  if (temperature != DHT11::ERROR_CHECKSUM && temperature != DHT11::ERROR_TIMEOUT) {
    // If there's no error, print the temperature value
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");
  } else {
    // If there's an error, print the appropriate error message
    Serial.println(DHT11::getErrorString(temperature));
  }

    //Telegram bot message section
    if (millis() - bot_lasttime > BOT_MTBS)
    {
      int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      while (numNewMessages)
      {
        Serial.println("got response");
        handleNewMessages(numNewMessages);
        numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      }
      bot_lasttime = millis();
    }

    // Print readings to LCD
  printLCD();

  // Send data to ThingSpeak
  sendDataToThingSpeak();

  delay(100); // Delay for stability
}

void printLCD() {
  lcd.clear(); // Clear previous display

  // Display voltage
  lcd.setCursor(3, 0);
  lcd.print("Voltage:");
  lcd.print(vOUT, 1); // Print with one decimal place
  lcd.print("V");

  // Display current
  lcd.setCursor(3, 1);
  lcd.print("Current:");
  lcd.print(current, 1); // Print with one decimal place
  lcd.print("A");
  delay(5000);

  

  // Display temperature
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temperature:");
  lcd.print(temperature, 1); // Print with one decimal place
  lcd.print("C");

  // Display light intensity
  lcd.setCursor(3, 1);
  lcd.print("Light:");
  lcd.print(lightValue);

  delay(500); // Delay to allow for faster display updates
}

void sendDataToThingSpeak() {
  if (client.connect(server, 80)) {
    String postStr = apiKey;
    postStr += "&field1=";
    postStr += String(vOUT);
    postStr += "&field2=";
    postStr += String(current);
    postStr += "&field3=";
    postStr += String(temperature);
    postStr += "&field4=";
    postStr += String(lightValue);
    postStr += "\r\n\r\n\r\n";

    client.print("POST /update HTTP/1.1\n");
    delay(100);
    client.print("Host: api.thingspeak.com\n");
    delay(100);
    client.print("Connection: close\n");
    delay(100);
    client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
    delay(100);
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    delay(100);
    client.print("Content-Length: ");
    delay(100);
    client.print(postStr.length());
    delay(100);
    client.print("\n\n");
    delay(100);
    client.print(postStr);
    delay(100);
  }
  client.stop();
  delay(500);
  Serial.println("Data sent to ThingSpeak....");
  Serial.println("");
}

void handleNewMessages(int numNewMessages)
{
  Serial.print("handleNewMessages ");
  Serial.println(numNewMessages);
  
  for (int i = 0; i < numNewMessages; i++)
  {
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID )
    {
      bot.sendMessage(chat_id, "Unauthorized user", "");
    }
    else
    {
      String text = bot.messages[i].text;
      String from_name = bot.messages[i].from_name;
      if (from_name == "")
        from_name = "Hari";
      if (text == "/status")
      {  
          String msg = "Voltage: "+ String(vOUT) + " V\n";
          msg += "current: "+ String(current) + "A\n";
          msg += "Temperature: "+ String(temperature) + "°C\n";
          msg += "Light Intensity: "+ String(lightValue) + "\n"; 
          bot.sendMessage(chat_id,msg, ""); 
      }
      if (text == "/start")
      {
        String welcome = "Hello, " + from_name + ".\n";
        welcome += "Welcome to Solar Power Monitor!\n";
        welcome += "/status : To check the current Status of the solar power. \n";
        welcome += "To monitor the Volatge, current, Temperature and Light intensity \n";
        bot.sendMessage(chat_id,welcome, "");
      }
    }
  }
}
