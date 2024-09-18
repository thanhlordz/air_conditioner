#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <DHT.h>
#include <assert.h>
#include <IRrecv.h>
#include <IRac.h>
#include <IRtext.h>
#include <IRutils.h>
#include <panasonicAC.h>
// ==================== start of TUNEABLE PARAMETERS ====================
// An IR detector/demodulator is connected to GPIO pin 14
// e.g. D5 on a NodeMCU board.
// Note: GPIO 16 won't work on the ESP8266 as it does not have interrupts.
// Note: GPIO 14 won't work on the ESP32-C3 as it causes the board to reboot.
#ifdef ARDUINO_ESP32C3_DEV
const uint16_t kRecvPin = 10;  // 14 on a ESP32-C3 causes a boot loop.
#else  // ARDUINO_ESP32C3_DEV
const uint16_t kRecvPin = 14;
#endif  // ARDUINO_ESP32C3_DEV

//WiFi Set-up
const char* ssid = "Am Thanh Co";
const char* password = "123456799";

//MQTT Set-up
const char* mqtt_server = "test.mosquitto.org";

#define DHT_PIN D1
#define DHT_TYPE DHT11

int temp_AC = 18;
int temp_room = 0;
int humi = 0;
std::string protocol{"UNKNOWN"};

/*const uint16_t kIrled = 4; //ESP8266 pin D2
IRPanasonicAc ac(kIrled);*/

// The Serial connection baud rate.
// i.e. Status message will be sent to the PC at this baud rate.
// Try to avoid slow speeds like 9600, as you will miss messages and
// cause other problems. 115200 (or faster) is recommended.
// NOTE: Make sure you set your Serial Monitor to the same speed.
const uint32_t kBaudRate = 115200;

// As this program is a special purpose capture/decoder, let us use a larger
// than normal buffer so we can handle Air Conditioner remote codes.
const uint16_t kCaptureBufferSize = 1024;

// kTimeout is the Nr. of milli-Seconds of no-more-data before we consider a
// message ended.
// This parameter is an interesting trade-off. The longer the timeout, the more
// complex a message it can capture. e.g. Some device protocols will send
// multiple message packets in quick succession, like Air Conditioner remotes.
// Air Coniditioner protocols often have a considerable gap (20-40+ms) between
// packets.
// The downside of a large timeout value is a lot of less complex protocols
// send multiple messages when the remote's button is held down. The gap between
// them is often also around 20+ms. This can result in the raw data be 2-3+
// times larger than needed as it has captured 2-3+ messages in a single
// capture. Setting a low timeout value can resolve this.
// So, choosing the best kTimeout value for your use particular case is
// quite nuanced. Good luck and happy hunting.
// NOTE: Don't exceed kMaxTimeoutMs. Typically 130ms.
#if DECODE_AC
// Some A/C units have gaps in their protocols of ~40ms. e.g. Kelvinator
// A value this large may swallow repeats of some protocols
const uint8_t kTimeout = 50;
#else   // DECODE_AC
// Suits most messages, while not swallowing many repeats.
const uint8_t kTimeout = 15;
#endif  // DECODE_AC
// Alternatives:
// const uint8_t kTimeout = 90;
// Suits messages with big gaps like XMP-1 & some aircon units, but can
// accidentally swallow repeated messages in the rawData[] output.
//
// const uint8_t kTimeout = kMaxTimeoutMs;
// This will set it to our currently allowed maximum.
// Values this high are problematic because it is roughly the typical boundary
// where most messages repeat.
// e.g. It will stop decoding a message and start sending it to serial at
//      precisely the time when the next message is likely to be transmitted,
//      and may miss it.

// Set the smallest sized "UNKNOWN" message packets we actually care about.
// This value helps reduce the false-positive detection rate of IR background
// noise as real messages. The chances of background IR noise getting detected
// as a message increases with the length of the kTimeout value. (See above)
// The downside of setting this message too large is you can miss some valid
// short messages for protocols that this library doesn't yet decode.
//
// Set higher if you get lots of random short UNKNOWN messages when nothing
// should be sending a message.
// Set lower if you are sure your setup is working, but it doesn't see messages
// from your device. (e.g. Other IR remotes work.)
// NOTE: Set this value very high to effectively turn off UNKNOWN detection.
const uint16_t kMinUnknownSize = 12;

// How much percentage lee way do we give to incoming signals in order to match
// it?
// e.g. +/- 25% (default) to an expected value of 500 would mean matching a
//      value between 375 & 625 inclusive.
// Note: Default is 25(%). Going to a value >= 50(%) will cause some protocols
//       to no longer match correctly. In normal situations you probably do not
//       need to adjust this value. Typically that's when the library detects
//       your remote's message some of the time, but not all of the time.
const uint8_t kTolerancePercentage = kTolerance;  // kTolerance is normally 25%

// Legacy (No longer supported!)
//
// Change to `true` if you miss/need the old "Raw Timing[]" display.
#define LEGACY_TIMING_INFO false
// ==================== end of TUNEABLE PARAMETERS ====================

// Use turn on the save buffer feature for more complete capture coverage.

// This section of code runs only once at start-up.

IRrecv irrecv(kRecvPin, kCaptureBufferSize, kTimeout, true);
decode_results results;  // Somewhere to store the results
WiFiClient espClient;
void callback(char* topic, byte* payload, unsigned int length);
PubSubClient client(espClient);

DHT dht(DHT_PIN, DHT_TYPE);
void connectWiFi(){
    Serial.print("Connecting to WiFi ");
    WiFi.begin(ssid, password);
    while(WiFi.status() != WL_CONNECTED){
        delay(200);
        Serial.print(".");
    }
    Serial.println(" Succesful");
}


//State of the AC Panasonic
/*class statePanasonic {
public:
    state(){
    }
    void setTemp(uint8_t tempAC){
        temp = tempAC;
    }

    void setFan(std::string fanAC){
        if(fanAC == "Low"){
            fan = kPanasonicAcFanLow;
        }
        else if(fanAC == "Medium"){
            fan = kPanasonicAcFanMed;
        }
        else if(fanAC == "High"){
            fan = kPanasonicAcFanHigh;
        }
        else{
            Serial.println("Invalid value for Fan");
        }
    }

    void setPower(std::string powerAC){
        if(powerAC == "on"){
            power = 1;
        }
        else if(powerAC == "off"){
            power = 0;
        }
        else{
            Serial.println("Invalid value for Power");
        }
    }

    void setMode(std::string modeAC){
        if(modeAC == "Auto"){
            mode = kPanasonicAcAuto;
        }
        else if(modeAC == "Cool"){
            mode = kPanasonicAcCool;
        }
        else if(modeAC == "Dry"){
            mode = kPanasonicAcDry;
        }
        else if(modeAC == "Heat"){
            mode = kPanasonicAcHeat;
        }
        else{
            Serial.println("Invalid value for Fan");
        }
    }

    std::string fanToString(){
        switch(fan){
            case kPanasonicAcFanLow:
                return "Low";
            case kPanasonicAcFanMed:
                return "Medium";
            case kPanasonicAcFanHigh:
                return "High";
            default:
                return "Medium";
        }
    } 

    std::string modeToString(){
        switch(mode){
            case kPanasonicAcAuto:
                return "Auto";
            case kPanasonicAcCool:
                return "Cool";
            case kPanasonicAcHeat:
                return "Heat";
            case kPanasonicAcDry:
                return "Dry";
            default: 
                return "Auto";
        }
    }

    std::string powerToString(){
        if(power){
            return "On";
        }
        else{
            return "Off";
        }
    }

    uint8_t temp = 25, fan = 1, mode = 3;
    bool power = 0;

    void controlAC(bool powerAC, uint8_t tempAC, uint8_t modeAC, uint8_t fanAC){
        if(powerAC){
            ac.on();
        }
        else{
            ac.off();
        }
        switch (modeAC){
            case kPanasonicAcAuto:
                ac.setMode(kPanasonicAcAuto);
                break;
            case kPanasonicAcHeat:
                ac.setMode(kPanasonicAcHeat);
                break;
            case kPanasonicAcCool:
                ac.setMode(kPanasonicAcCool);
                break;
            case kPanasonicAcDry:
                ac.setMode(kPanasonicAcDry);
                break;
        }
        switch (fanAC){
            case kPanasonicAcFanLow:
                ac.setFan(kPanasonicAcFanLow);
                break;
            case kPanasonicAcFanMin:
                ac.setFan(kPanasonicAcFanMin);
                break;
            case kPanasonicAcFanHigh:
                ac.setFan(kPanasonicAcFanHigh);
                break;
            case kPanasonicAcFanAuto:
                ac.setFan(kPanasonicAcFanAuto);
                break;
        }
        ac.setTemp(tempAC);
        ac.send();
        Serial.println("Sending A/C done");
        printState();
        client.loop();
        delay(2000);
    }
};
statePanasonic acState;
*/
void printState() {
    Serial.println("AC State:");
    Serial.printf(" %s\n", ac.toString().c_str());
    Serial.println();
}

//Package data of AC state
void packAC(){
    StaticJsonDocument<200> container;
    char msg[80]; 
    container["power"] = acState.powerToString();
    container["temperature"] = acState.temp;
    container["mode"] = acState.modeToString();
    container["fan_speed"] = acState.fanToString();
    serializeJson(container, msg);
    client.publish("home/ac/upload_state", msg);
    delay(100);
    Serial.println("Message Sent!");
}

void packEnvi(){
    
    StaticJsonDocument<200> container;
    char msg[80]; 
    container["real_temperature"] = temp_room;
    container["humidity"] = humi;
    serializeJson(container, msg);
    client.publish("home/ac/room_envi", msg);
    delay(100);
}

void packProtocol(){
    StaticJsonDocument<200> container;
    char msg[30]; 
    container["protocol"] = protocol;
    serializeJson(container, msg);
    client.publish("home/ac/response_protocol", msg);
    delay(100);
    Serial.println("Protocol Sent!");
}
std::string protocolString(const decode_type_t protocol){
    switch(protocol){
        case decode_type_t::PANASONIC_AC:
            return "Panasonic";
        default:
            return "UNKNOWN";
    }
}

void reconnect() {
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        String clientId = "ESP8266Client-";
        clientId += String(random(0xffff), HEX);
        if (client.connect(clientId.c_str())) {
        Serial.println("Connected");
        } else {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        delay(5000);
        }
    }
    client.subscribe("home/ac/request_protocol");
    client.subscribe("home/ac/update");
}

//MQTT subcrisbe will receive the temperature value of outer room 
void callback(char* topic, byte* payload, unsigned int length);

void setup(){
    #if defined(ESP8266)
        Serial.begin(kBaudRate, SERIAL_8N1, SERIAL_TX_ONLY);
    #else  // ESP8266
        Serial.begin(kBaudRate, SERIAL_8N1);
    #endif  // ESP8266
    while (!Serial)  // Wait for the serial connection to be establised.
        delay(50);
    // Perform a low level sanity checks that the compiler performs bit field
    // packing as we expect and Endianness is as we expect.
        assert(irutils::lowLevelSanityCheck() == 0);

    Serial.printf("\n" D_STR_IRRECVDUMP_STARTUP "\n", kRecvPin);
    #if DECODE_HASH
    // Ignore messages with less than minimum on or off pulses.
        irrecv.setUnknownThreshold(kMinUnknownSize);
    #endif  // DECODE_HASH
        irrecv.setTolerance(kTolerancePercentage);  // Override the default tolerance.
        irrecv.enableIRIn();  // Start the receiver
    connectWiFi();
    delay(100);
    ac.begin();
    delay(100);

    Serial.println("Default State of the remote.");
    printState();
    Serial.println("Setting initial state for A/C.");
    ac.off();
    ac.setFan(acState.fan);
    ac.setMode(acState.mode);
    ac.setTemp(acState.temp);
    printState();
    dht.begin();
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
    reconnect();
    ac.send();
    packAC(); 
}

void callback(char* topic, byte* payload, unsigned int length){
    Serial.print("Message arrived on topic: ");
    Serial.println(topic);
    if (strcmp((char*) topic, "home/ac/update") == 0){
        String message = "";
        char* charPointer = (char*) payload;
        message = charPointer;
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, message);
        if (error) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
        }
        acState.setPower(doc["power"]);
        acState.temp = doc["temperature"];
        acState.setMode(doc["mode"]);
        acState.setFan(doc["fan_speed"]);
        delay(200);
        Serial.print("Power: ");
        Serial.println(acState.power);
        Serial.print("Temperature: ");
        Serial.println(acState.temp);
        Serial.print("Mode: ");
        Serial.println(acState.mode);
        Serial.print("Fan: ");
        Serial.println(acState.fan);
        acState.controlAC(acState.power, acState.temp, acState.mode, acState.fan);
        printState();
    }
    if (strcmp((char*) topic, "home/ac/request_protocol") == 0){
        while (!irrecv.decode(&results)){
            delay(200);
        }
        protocol = protocolString(results.decode_type);
        // Display the basic output of what we found.
        Serial.print(resultToHumanReadableBasic(&results));
        // Display any extra A/C info if we have it.
        String description = IRAcUtils::resultAcToString(&results);
        if (description.length()) Serial.println(D_STR_MESGDESC ": " + description);
        Serial.println("Vendor Identified"); 
        packProtocol();
    }
}

void loop(){
    delay(1000);
    if (!client.connected()){
        reconnect();
    }
    /*if (irrecv.decode(&results)){
        protocol = protocolString(results.decode_type);
        // Display the basic output of what we found.
        Serial.print(resultToHumanReadableBasic(&results));
        // Display any extra A/C info if we have it.
        String description = IRAcUtils::resultAcToString(&results);
        if (description.length()) Serial.println(D_STR_MESGDESC ": " + description);
        Serial.println("Vendor Identified");
    }*/
    temp_room = (int) dht.readTemperature();  //Read Temperature
    humi = (int) dht.readHumidity();
    packEnvi();
    client.loop();
}

