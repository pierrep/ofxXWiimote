#include "ofxXWiimote.h"

#include "ofMain.h"

ofEvent<ofxXWiimote::WiimoteKeyEvent> ofxXWiimote::wiimoteKeyEvent = ofEvent<ofxXWiimote::WiimoteKeyEvent>();

ofxXWiimote::ofxXWiimote()
{
    active = false;
    iface = NULL;
}

ofxXWiimote::~ofxXWiimote()
{
    xwii_iface_unref(iface);
}


//--------------------------------------------------------------
int ofxXWiimote::listDevices()
{
    /* Enumerate Wiimotes */
    struct xwii_monitor *mon;
    char *ent;
    int num = 0;

    mon = xwii_monitor_new(false, false);
    if (!mon)
    {
        ofLogError("Cannot create Wiimote monitor");
        return -1;
    }

    if (geteuid() != 0)
    {
        ofLog(OF_LOG_WARNING,"Warning: Please run as root! (sysfs+evdev access needed)");
        return -1;
    }

    while ((ent = xwii_monitor_poll(mon)))
    {
        ofLog(OF_LOG_NOTICE,"** Found device #%d: %s", ++num, ent);
        free(ent);
    }

    xwii_monitor_unref(mon);

    return num;
}

bool ofxXWiimote::reconnectDevice(int device_id)
{
    if(active) {
        active = false;
        if(iface != NULL) {
            ofLog() << "removing iface ref";
            xwii_iface_unref(iface);
        }
        ofLogNotice() << "stop thread";
        stopThread();
        ofLogNotice() << "wait for thread...";
        waitForThread(true,2000);
        bool val = setup(device_id);
        return val;
    }
    else {
        ofLogNotice() << "reconnect - not active";
        if(iface != NULL) {
            xwii_iface_unref(iface);
        }
        bool val = setup(device_id);
        return val;
    }
    return false;

}

//--------------------------------------------------------------
bool ofxXWiimote::setup(int device_id)
{
    ofLogNotice() << "Setting up Wiimote" << endl;

    /* Enumerate Wiimotes */
    struct xwii_monitor *mon;
    char *ent;
    int num = 0;

    ofLogNotice() << "create new monitor" << endl;
    mon = xwii_monitor_new(true, false);
    if (!mon)
    {
        ofLogError("Cannot create Wiimote monitor");
        return false;
    }

    if (geteuid() != 0)
    {
        ofLog(OF_LOG_WARNING,"Warning: Please run as root! (sysfs+evdev access needed)");
        return false;
    }

    while ((ent = xwii_monitor_poll(mon)))
    {
        num++;
        ofLog(OF_LOG_NOTICE,"** Found device #%d: %s\n", num, ent);
        if(device_id == num)
        {
            ofLog(OF_LOG_NOTICE,"** Found device #%d: %s\n", num, ent);
            ofLog(OF_LOG_NOTICE,"** Opening device #%d: %s\n", num, ent);
            active = openWiimote(ent);
            if(!active) ofLogError() << "Failed to open Wiimote" << endl;
            m_id = device_id;
        }
        free(ent);
    }
    ofLogNotice() << "unref monitor";
    xwii_monitor_unref(mon);

    if(!active) return false;
    else {
            ofLogNotice() << "Starting thread" << endl;
            startThread();
    }

    return true;
}

//--------------------------------------------------------------
bool ofxXWiimote::openWiimote(char *syspath)
{

    int ret = 0;
    ret = xwii_iface_new(&iface, syspath);

    queryDeviceType();
    queryExtension();

    ret = xwii_iface_open(iface, xwii_iface_available(iface) | XWII_IFACE_WRITABLE);
    if (ret)
    {
        ofLog(OF_LOG_ERROR,"Cannot open interface: %d", ret);
        return false;
    }

    memset(fds, 0, sizeof(fds));
    fds[0].fd = 0;
    fds[0].events = POLLIN;
    fds[1].fd = xwii_iface_get_fd(iface);
    fds[1].events = POLLIN;
    fds_num = 2;

    ret = xwii_iface_watch(iface, true);
    if (ret)
    {
        ofLog(OF_LOG_ERROR,"Error: Cannot initialize hotplug watch descriptor");
        return false;
    }

    return true;
}

//--------------------------------------------------------------
/* device watch events */
void ofxXWiimote::handle_watch()
{
    static unsigned int num;
    int ret;

    ofLog(OF_LOG_NOTICE,"Watch Event #%u", ++num);

    ret = xwii_iface_open(iface, xwii_iface_available(iface) | XWII_IFACE_WRITABLE);
    if (ret)
    {
        ofLog(OF_LOG_ERROR,"Cannot open interface: %d", ret);
        return;
    }

    memset(fds, 0, sizeof(fds));
    fds[0].fd = 0;
    fds[0].events = POLLIN;
    fds[1].fd = xwii_iface_get_fd(iface);
    fds[1].events = POLLIN;
    fds_num = 2;

    ret = xwii_iface_watch(iface, true);
    if (ret)
    {
        ofLog(OF_LOG_ERROR,"Error: Cannot initialize hotplug watch descriptor");
        return;
    }
}

void ofxXWiimote::getAccel(ofVec3f& _accel)
{
    lock();
    _accel = accel;
    unlock();
}

//--------------------------------------------------------------
void ofxXWiimote::handle_keys(const struct xwii_event *event)
{
    unsigned int code = event->v.key.code;
    bool pressed = event->v.key.state;

    WiimoteKeyEvent e;
    e.key = code;
    e.pressed = pressed;
    ofNotifyEvent(ofxXWiimote::wiimoteKeyEvent, e, this);

//	if (code == XWII_KEY_LEFT) {
//		mvprintw(4, 7, "%s", str);
//	}
//	} else if (code == XWII_KEY_RIGHT) {
//		mvprintw(4, 11, "%s", str);
//	} else if (code == XWII_KEY_UP) {
//		mvprintw(2, 9, "%s", str);
//	} else if (code == XWII_KEY_DOWN) {
//		mvprintw(6, 9, "%s", str);
//	} else if (code == XWII_KEY_A) {
//		if (pressed)
//			str = "A";
//		mvprintw(10, 5, "%s", str);
//	} else if (code == XWII_KEY_B) {
//		if (pressed)
//			str = "B";
//		mvprintw(10, 13, "%s", str);
//	} else if (code == XWII_KEY_HOME) {
//		if (pressed)
//			str = "HOME+";
//		else
//			str = "     ";
//		mvprintw(13, 7, "%s", str);
//	} else if (code == XWII_KEY_MINUS) {
//		if (pressed)
//			str = "-";
//		mvprintw(13, 3, "%s", str);
//	} else if (code == XWII_KEY_PLUS) {
//		if (pressed)
//			str = "+";
//		mvprintw(13, 15, "%s", str);
//	} else if (code == XWII_KEY_ONE) {
//		if (pressed)
//			str = "1";
//		mvprintw(20, 9, "%s", str);
//	} else if (code == XWII_KEY_TWO) {
//		if (pressed)
//			str = "2";
//		mvprintw(21, 9, "%s", str);
//	}
}

//--------------------------------------------------------------
void ofxXWiimote::getNormalisedAccel(ofVec3f& _accel)
{
    double val;

    lock();
    _accel = accel;
    unlock();

    val = _accel.x;
    val /= 512;
    if (val >= 0)
        val = 10 * pow(val, 0.25);
    _accel.x = val;

    val = _accel.y;
    val /= 512;
    if (val >= 0)
        val = 10 * pow(val, 0.25);
    _accel.y = val;

    val = _accel.z;
    val /= 512;
    if (val >= 0)
        val = 10 * pow(val, 0.25);
    _accel.z = val;
}

//--------------------------------------------------------------
void ofxXWiimote::threadedFunction()
{
    if(!active) return;

    while(isThreadRunning())
    {

        int ret = 0;

        //ret = poll(fds, fds_num, -1);
        ret = poll(fds, fds_num, 200);
        if (ret < 0)
        {
            if (errno != EINTR)
            {
                ret = -errno;
                ofLog(OF_LOG_ERROR,"Error: Cannot poll fds: %d", ret);
                return;
            }
        }

        ret = xwii_iface_dispatch(iface, &event, sizeof(event));
        if (ret)
        {
            if (ret != -EAGAIN)
            {
                ofLog(OF_LOG_ERROR,"Error: Read failed with err:%d",ret);
                return;
            }
        }
        else
        {
            switch (event.type)
            {
            case XWII_EVENT_GONE:
                ofLog(OF_LOG_NOTICE,"Info: Device gone");
                fds[1].fd = -1;
                fds[1].events = 0;
                fds_num = 1;
                break;
            case XWII_EVENT_WATCH:
                handle_watch();
                break;
            case XWII_EVENT_KEY:
                handle_keys(&event);
                break;
            case XWII_EVENT_ACCEL:
                ofLogVerbose() << "Acceleration x: " << event.v.abs[0].x << " Acceleration y: " << event.v.abs[0].y << " Acceleration z: " << event.v.abs[0].z;
                accel.set(event.v.abs[0].x,event.v.abs[0].y,event.v.abs[0].z);
                break;
            case XWII_EVENT_IR:

                break;
            case XWII_EVENT_MOTION_PLUS:
                //cout << "motion plus event" << endl;
                break;
            case XWII_EVENT_NUNCHUK_KEY:
            case XWII_EVENT_NUNCHUK_MOVE:

                break;
            case XWII_EVENT_CLASSIC_CONTROLLER_KEY:
            case XWII_EVENT_CLASSIC_CONTROLLER_MOVE:
                cout << "classic controller event" << endl;
                break;
            case XWII_EVENT_BALANCE_BOARD:

                break;
            case XWII_EVENT_PRO_CONTROLLER_KEY:
            case XWII_EVENT_PRO_CONTROLLER_MOVE:
                cout << "pro controller event" << endl;
                break;
            case XWII_EVENT_GUITAR_KEY:
            case XWII_EVENT_GUITAR_MOVE:

                break;
            case XWII_EVENT_DRUMS_KEY:
            case XWII_EVENT_DRUMS_MOVE:

                break;
            }
        }

    } // thread running

}

//--------------------------------------------------------------
void ofxXWiimote::queryDeviceType()
{
    int ret;
    char *name;

    ret = xwii_iface_get_devtype(iface, &name);
    if (ret)
    {
        ofLogError("Error: Cannot read device type");
    }
    else
    {
        ofLog(OF_LOG_NOTICE, "Device found: %s", name);
        free(name);
    }
}

//--------------------------------------------------------------
void ofxXWiimote::queryExtension()
{
    int ret;
    char *name;

    ret = xwii_iface_get_extension(iface, &name);
    if (ret)
    {
        ofLogError("Cannot read extension type");
    }
    else
    {
        ofLog(OF_LOG_NOTICE,"Extension: %s", name);
        free(name);
    }

    if (xwii_iface_available(iface) & XWII_IFACE_MOTION_PLUS)
        ofLog(OF_LOG_NOTICE,"Extension: Motion Plus");

}

//--------------------------------------------------------------
void ofxXWiimote::rumble(bool on)
{
    xwii_iface_rumble(iface, on);
}
