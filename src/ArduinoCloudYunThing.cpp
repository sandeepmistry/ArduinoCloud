#include "ArduinoCloudYunThing.h"

ArduinoCloudYunThing::ArduinoCloudYunThing() {
}

boolean ArduinoCloudYunThing::begin(const char* name, const char* username, const char* id, const char* password) {
    ArduinoCloudThingBase::begin(name, username, id, password);

    if (updateBridge()) {
        process.begin(F("python"));
        process.addParameter(F("-u"));
        process.addParameter(F("/usr/arduino-mqtt/bridge.py"));
        process.runAsynchronously();
        process.setTimeout(10000);

        // wait for script to launch
        process.readStringUntil('\n');
        process.readStringUntil('\n');

        return 1;
    }

    return 0;
}

boolean ArduinoCloudYunThing::updateBridge() {
    Process p;

    int r1 = p.runShellCommand(F("mkdir -p /usr/arduino-mqtt"));
    int r2 = p.runShellCommand(F("wget -N https://raw.githubusercontent.com/256dpi/arduino-mqtt/v1.10.1/yun/mqtt.py --no-check-certificate -P /usr/arduino-mqtt"));
    int r3 = p.runShellCommand(F("wget -N https://raw.githubusercontent.com/256dpi/arduino-mqtt/v1.10.1/yun/bridge.py --no-check-certificate -P /usr/arduino-mqtt"));

    return r1 == 0 && r2 == 0 && r3 == 0;
}

boolean ArduinoCloudYunThing::connect() {
    const char* willPayload = "offline";
    String willTopic;

    willTopic += username;
    willTopic += "/";
    willTopic += name;
    willTopic += "/status";

    process.print(F("w:"));
    process.print(willTopic);
    process.print(F(":"));
    process.print(strlen(willPayload));
    process.print(F(";"));
    process.print(willPayload);
    process.print(F("\n"));

    process.print(F("t:/etc/ssl/certs/Go_Daddy_Class_2_CA.crt;\n"));

    // send connect request
    process.print(F("c:"));
    process.print(SERVER_DOMAIN);
    process.print(F(":"));
    process.print(SERVER_PORT);
    process.print(F(":"));
    process.print(name);
    process.print(F(":"));
    process.print(id);
    process.print(F(":"));
    process.print(password);
    process.print(F(";\n"));

    // wait for answer
    String ret = process.readStringUntil('\n');
    alive = ret.equals(F("a;"));

    if(!alive) {
        process.close();
        return false;
    } else {
        publish(willTopic.c_str(), "online");

        mqttSubscribe();
    }

    return true;
}

void ArduinoCloudYunThing::publish(const char * topic, const char * payload) {
    publish(topic, (char*)payload, strlen(payload));
}

void ArduinoCloudYunThing::publish(const char * topic, char * payload, unsigned int length) {
    // send publish request
    process.print(F("p:"));
    process.print(topic);
    process.print(F(":"));
    process.print(length);
    process.print(F(";"));

    for(unsigned int i=0; i<length; i++) {
        process.write(payload[i]);
    }

    process.print('\n');
}

void ArduinoCloudYunThing::mqttSubscribe() {
    for (int i=0; i<properties_count; i++){
        if (strcmp(properties[i]->permission, "RW") == 0){
            String topic = buildTopicProperty(i);

            process.print(F("s:"));
            process.print(topic);
            process.print(F(";\n"));
        }
    }
}

void ArduinoCloudYunThing::poll() {
    int av = this->process.available();

    if(av > 0) {
        String ret = process.readStringUntil(';');

        if(ret.startsWith("m")) {
            int startTopic = 2;
            int endTopic = ret.indexOf(':', startTopic + 1);
            String topic = ret.substring(startTopic, endTopic);

            int startPayloadLength = endTopic + 1;
            int endPayloadLength = ret.indexOf(':', startPayloadLength + 1);
            int payloadLength = ret.substring(startPayloadLength, endPayloadLength).toInt();

            char buf[payloadLength+1];
            process.readBytes(buf, payloadLength);
            buf[payloadLength] = '\0';

            updatePropertyFromTopic(topic.c_str(), buf);
        } else if(ret.startsWith("e")) {
            alive = false;
            process.close();
        }

        process.readStringUntil('\n');
    }

    if (!alive) {
        connect();
    }
}

boolean ArduinoCloudYunThing::connected() {
    return process.running() && alive;
}
