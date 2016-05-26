#include "ArduinoCloudYunThing.h"

ArduinoCloudYunThing::ArduinoCloudYunThing() {
}

boolean ArduinoCloudYunThing::begin(const char* name, const char* username, const char* id, const char* password) {
    ArduinoCloudThingBase::begin(name, username, id, password);

    return updateBridge();
}

boolean ArduinoCloudYunThing::updateBridge() {
    Process p;

    int r1 = p.runShellCommand("mkdir -p /usr/arduino-mqtt");
    int r2 = p.runShellCommand("wget -N https://raw.githubusercontent.com/256dpi/arduino-mqtt/v1.9.6/yun/mqtt.py --no-check-certificate -P /usr/arduino-mqtt");
    int r3 = p.runShellCommand("wget -N https://raw.githubusercontent.com/256dpi/arduino-mqtt/v1.9.6/yun/bridge.py --no-check-certificate -P /usr/arduino-mqtt");

    return r1 == 0 && r2 == 0 && r3 == 0;
}

boolean ArduinoCloudYunThing::connect() {
    process.begin("python");
    process.addParameter("-u");
    process.addParameter("/usr/arduino-mqtt/bridge.py");
    process.runAsynchronously();
    process.setTimeout(10000);

    // wait for script to launch
    process.readStringUntil('\n');

    // process.print("w:");
    // process.print(willTopic);
    // process.print(':');
    // process.print(strlen(willPayload));
    // process.print(';');
    // process.print(willPayload);
    // process.print('\n');

    // send connect request
    process.print("c:");
    process.print(SERVER_DOMAIN);
    process.print(':');
    process.print(SERVER_PORT);
    process.print(':');
    process.print(name);
    process.print(':');
    process.print(id);
    process.print(':');
    process.print(password);
    process.print(";\n");

    // wait for answer
    String ret = process.readStringUntil('\n');
    alive = ret.equals("a;");

    if(!alive) {
    process.close();
    return false;
    }

    return true;
}

void ArduinoCloudYunThing::publish(const char * topic, const char * payload) {
  publish(topic, (char*)payload, strlen(payload));
}

void ArduinoCloudYunThing::publish(const char * topic, char * payload, unsigned int length) {
  // send publish request
  process.print("p:");
  process.print(topic);
  process.print(':');
  process.print(length);
  process.print(';');

  for(unsigned int i=0; i<length; i++) {
    process.write(payload[i]);
  }

  process.print('\n');
}

void ArduinoCloudYunThing::subscribe(const char * topic) {
    for (int i=0; i<properties_count; i++){
        if (strcmp(properties[i]->permission, "RW") == 0){
            String topic = buildTopicProperty(i);

            process.print("s:");
            process.print(topic);
            process.print(";\n");
        }
    }
  process.print("s:");
  process.print(topic);
  process.print(";\n");
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

    //   messageReceived(topic, String(buf), buf, payloadLength);
    } else if(ret.startsWith("e")) {
      alive = false;
      process.close();
    }

    process.readStringUntil('\n');
  }
}

boolean ArduinoCloudYunThing::connected() {
  return process.running() && alive;
}
