#pragma once
#include "xwiimote.h"
#include <poll.h>
#include "ofThread.h"
#include "ofEvents.h"

class ofxXWiimote : public ofThread
{
public:
    static int listDevices();

    struct WiimoteKeyEvent
    {
        unsigned int key;
        bool pressed;
    };

    static ofEvent<WiimoteKeyEvent> wiimoteKeyEvent;

    ofxXWiimote();
    virtual ~ofxXWiimote();
    bool setup(int i = 1);
    void threadedFunction();

    bool openWiimote(char *syspath);
    void queryDeviceType();
    void queryExtension();
    void rumble(bool on);
    void getAccel(ofVec3f& accel);
    void getNormalisedAccel(ofVec3f& _accel);
    bool reconnectDevice(int device_id);

    bool active;

protected:

    void handle_watch();
    void handle_keys(const struct xwii_event *event);

    struct xwii_iface *iface;
    struct xwii_event event;
    ofVec3f accel;

private:
    int fds_num;
    struct pollfd fds[2];
    int m_id;

};
