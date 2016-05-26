#ifndef ArduinoCloudYun_h
#define ArduinoCloudYun_h

#include <ArduinoCloudThingBase.h>
#include <Bridge.h>
#include <Process.h>
#include <FileIO.h>

class ArduinoCloudYunThing : public ArduinoCloudThingBase {
public:
    ArduinoCloudYunThing();
    boolean begin(const char* name, const char* username, const char* id, const char* password);
    void poll();

protected:
    virtual void publish(const char * topic, const char * payload);

private:
    boolean connect();
    void publish(const char * topic, char * payload, unsigned int length);
    void subscribe(const char * topic);
    boolean connected();

    Process process;
    boolean alive = false;
    boolean updateBridge();
};
#endif
