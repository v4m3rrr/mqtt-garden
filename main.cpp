#include <Arduino.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "Secrets.h"


#define WIFI_SSID SECRET_SSID
#define WIFI_PASS SECRET_PASS

#define MQTT_SERVER_NAME "192.168.0.123"
#define MQTT_SERVER_PORT 1883

#define MQTT_USER SECRET_USER
#define MQTT_PASS SECRET_USER_PASS

#define MQTT_CLIENT_ID "Esp8266spacer"
#define MQTT_TOPIC_TEMPERATURE "Esp8266spacer/garden/temperature"
#define MQTT_TOPIC_HUMIDITY "Esp8266spacer/garden/humidity"

#define MQTT_TOPIC_REQUEST_TEMP "db/garden/temperature"
#define MQTT_TOPIC_REQUEST_HUM "db/garden/humidity"

#define MQTT_LAST_WILL_TOPIC "Esp8266spacer/status"

#define MQTT_PUBLISH_DELAY 60000


void callback(char*,byte*,unsigned int);
void reconnect();

DHT dht;
#define DHT_PIN 16

WiFiClient esp;
PubSubClient client(MQTT_SERVER_NAME,MQTT_SERVER_PORT,callback,esp);

float dhtTemp;
float dhtHum;

void setup() {
    Serial.begin(9600);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID,WIFI_PASS);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("Connected to: "+String(WIFI_SSID));
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    dht.setup(DHT_PIN);
}

void loop()
{
    while(!client.connected()){
        reconnect();
        delay(5000);
    }   
    
    client.loop();

    unsigned long now = millis();

    static unsigned long lastSampling= now - dht.getMinimumSamplingPeriod()*2 -1;
    if( now-lastSampling > (unsigned int)dht.getMinimumSamplingPeriod()*2){
        lastSampling=now;

        dhtTemp = dht.getTemperature();
        dhtHum = dht.getHumidity();

        Serial.print("Temperature: ");
        Serial.println(dhtTemp);
        Serial.print("Humidity: ");
        Serial.println(dhtHum);

        Serial.println();
    } 

    static unsigned long lastMsg = now - MQTT_PUBLISH_DELAY -1;
    if(now - lastMsg > MQTT_PUBLISH_DELAY){
        lastMsg=now;

        Serial.println("Published message temp: "+String(dhtTemp)+" and hum: "+String(dhtHum));
        Serial.println();
        client.publish(MQTT_TOPIC_TEMPERATURE, String(dhtTemp).c_str(),true);
        client.publish(MQTT_TOPIC_HUMIDITY, String(dhtHum).c_str(),true);
    }
}

void reconnect() {
    Serial.print("Attempting to connect to MQTT Server: ");
    Serial.println(MQTT_SERVER_NAME);

    if(client.connect(MQTT_CLIENT_ID,MQTT_USER,MQTT_PASS,MQTT_LAST_WILL_TOPIC,1,true,"offline")){
        Serial.println("Connected");
        Serial.print("ClientId: ");
        Serial.println(MQTT_CLIENT_ID);

        client.publish(MQTT_LAST_WILL_TOPIC,"online",true);
        client.subscribe(MQTT_TOPIC_REQUEST_TEMP);
        client.subscribe(MQTT_TOPIC_REQUEST_HUM);
    }else{
        Serial.print("Connection failed! Rc= ");
        Serial.println(client.state());
    }
}

void callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.println("] ");
    Serial.print("Payload:");

    for(uint i=0;i<length;i++)
        Serial.print((char)payload[i]);
    Serial.println();

    if(length!=1){
        Serial.println();
        return;
    }
        

    if(strcmp(topic,MQTT_TOPIC_REQUEST_TEMP)==0){
        if(payload[0] != '0'){
            Serial.print("Published message temp: ");
            Serial.println(dhtTemp);
            client.publish(MQTT_TOPIC_TEMPERATURE, String(dhtTemp).c_str(),true);
        }
    }else if(strcmp(topic,MQTT_TOPIC_REQUEST_HUM)==0){
        if(payload[0] != '0'){
            Serial.print("Published message hum: ");
            Serial.println(dhtHum);
            client.publish(MQTT_TOPIC_HUMIDITY, String(dhtHum).c_str(),true);
        }
    }

    Serial.println();
}