#include "wled.h"
#include <HardwareSerial.h>

/*
 * Usermods allow you to add own functionality to WLED more easily
 * See: https://github.com/wled-dev/WLED/wiki/Add-own-functionality
 *
 * This is an example for a v2 usermod.
 * v2 usermods are class inheritance based and can (but don't have to) implement more functions, each of them is shown in this example.
 * Multiple v2 usermods can be added to one compilation easily.
 *
 * Creating a usermod:
 * This file serves as an example. If you want to create a usermod, it is recommended to use usermod_v2_empty.h from the usermods folder as a template.
 * Please remember to rename the class and file to a descriptive name.
 * You may also use multiple .h and .cpp files.
 *
 * Using a usermod:
 * 1. Copy the usermod into the sketch folder (same folder as wled00.ino)
 * 2. Register the usermod by adding #include "usermod_filename.h" in the top and registerUsermod(new MyUsermodClass()) in the bottom of usermods_list.cpp
 */

// class name. Use something descriptive and leave the ": public Usermod" part :)
class SerialForwarding : public Usermod
{

private:
    // Private class members. You can declare variables and functions only accessible to your usermod here
    bool enabled = false;
    bool initDone = false;
    unsigned long lastTime = 0;

    // string that are used multiple time (this will save some flash memory)
    static const char _name[];
    static const char _enabled[];

public:
    // non WLED related methods, may be used for data exchange between usermods (non-inline methods should be defined out of class)

    /**
     * Enable/Disable the usermod
     */
    inline void enable(bool enable) { enabled = enable; }

    /**
     * Get usermod enabled/disabled state
     */
    inline bool isEnabled() { return enabled; }

    // in such case add the following to another usermod:
    //  in private vars:
    //   #ifdef USERMOD_EXAMPLE
    //   CockUsermod* UM;
    //   #endif
    //  in setup()
    //   #ifdef USERMOD_EXAMPLE
    //   UM = (CockUsermod*) UsermodManager::lookup(USERMOD_ID_EXAMPLE);
    //   #endif
    //  somewhere in loop() or other member method
    //   #ifdef USERMOD_EXAMPLE
    //   if (UM != nullptr) isExampleEnabled = UM->isEnabled();
    //   if (!isExampleEnabled) UM->enable(true);
    //   #endif

    // methods called by WLED (can be inlined as they are called only once but if you call them explicitly define them out of class)

    /*
     * setup() is called once at boot. WiFi is not yet connected at this point.
     * readFromConfig() is called prior to setup()
     * You can use it to initialize variables, sensors or similar.
     */

#define RX_GPIO 18
#define TX_GPIO 19

    HardwareSerial *serial1;

    void setup() override
    {
        // do your set-up here
        // Serial.println("Hello from my usermod!");
        initDone = true;
        pinMode(2, OUTPUT);
        serial1 = new HardwareSerial(1);
        serial1->begin(115200, SERIAL_8N1, RX_GPIO, TX_GPIO);
    }

    /*
     * connected() is called every time the WiFi is (re)connected
     * Use it to initialize network interfaces
     */
    void connected() override
    {
        // Serial.println("Connected to WiFi!");
    }

    /*
     * loop() is called continuously. Here you can check for events, read sensors, etc.
     *
     * Tips:
     * 1. You can use "if (WLED_CONNECTED)" to check for a successful network connection.
     *    Additionally, "if (WLED_MQTT_CONNECTED)" is available to check for a connection to an MQTT broker.
     *
     * 2. Try to avoid using the delay() function. NEVER use delays longer than 10 milliseconds.
     *    Instead, use a timer check as shown here.
     */

    void loop() override
    {
        // if usermod is disabled or called during strip updating just exit
        // NOTE: on very long strips strip.isUpdating() may always return true so update accordingly
        if (/*!enabled ||*/ strip.isUpdating())
            return;

        // do your magic here
        // if (millis() - lastTime > 1000)
        // {

        //   lastTime = millis();
        // }
        String line;

        if (serial1->available())
        {
            line = serial1->readStringUntil('\n');
            serial1->print("Received from serial1: ");
            serial1->println(line);
            digitalWrite(2, HIGH);
        }
        else
        {
            digitalWrite(2, LOW);
        }
    }

    /*
     * addToJsonInfo() can be used to add custom entries to the /json/info part of the JSON API.
     * Creating a "u" object allows you to add custom key/value pairs to the Info section of the WLED web UI.
     * Below it is shown how this could be used for e.g. a light sensor
     */
    void addToJsonInfo(JsonObject &root) override
    {
        // if "u" object does not exist yet wee need to create it
        JsonObject user = root["u"];
        if (user.isNull())
            user = root.createNestedObject("u");

        // this code adds "u":{"ExampleUsermod":[20," lux"]} to the info object
        // int reading = 20;
        // JsonArray lightArr = user.createNestedArray(FPSTR(_name))); //name
        // lightArr.add(reading); //value
        // lightArr.add(F(" lux")); //unit

        // if you are implementing a sensor usermod, you may publish sensor data
        // JsonObject sensor = root[F("sensor")];
        // if (sensor.isNull()) sensor = root.createNestedObject(F("sensor"));
        // temp = sensor.createNestedArray(F("light"));
        // temp.add(reading);
        // temp.add(F("lux"));
    }

    /*
     * addToJsonState() can be used to add custom entries to the /json/state part of the JSON API (state object).
     * Values in the state object may be modified by connected clients
     */
    void addToJsonState(JsonObject &root) override
    {
        if (!initDone || !enabled)
            return; // prevent crash on boot applyPreset()

        JsonObject usermod = root[FPSTR(_name)];
        if (usermod.isNull())
            usermod = root.createNestedObject(FPSTR(_name));

        // usermod["user0"] = userVar0;
    }

    /*
     * readFromJsonState() can be used to receive data clients send to the /json/state part of the JSON API (state object).
     * Values in the state object may be modified by connected clients
     */
    void readFromJsonState(JsonObject &root) override
    {
        if (!initDone)
            return; // prevent crash on boot applyPreset()

        JsonObject usermod = root[FPSTR(_name)];
        if (!usermod.isNull())
        {
            // expect JSON usermod data in usermod name object: {"ExampleUsermod:{"user0":10}"}
            userVar0 = usermod["user0"] | userVar0; // if "user0" key exists in JSON, update, else keep old value
        }
        // you can as well check WLED state JSON keys
        // if (root["bri"] == 255) Serial.println(F("Don't burn down your garage!"));
    }

    /*
     * handleOverlayDraw() is called just before every show() (LED strip update frame) after effects have set the colors.
     * Use this to blank out some LEDs or set them to a different color regardless of the set effect mode.
     * Commonly used for custom clocks (Cronixie, 7 segment)
     */
    void handleOverlayDraw() override
    {
        // strip.setPixelColor(0, RGBW32(0,0,0,0)) // set the first pixel to black
    }

    /**
     * onStateChanged() is used to detect WLED state change
     * @mode parameter is CALL_MODE_... parameter used for notifications
     */
    void onStateChange(uint8_t mode) override
    {
        // do something if WLED state changed (color, brightness, effect, preset, etc)
    }

    // More methods can be added in the future, this example will then be extended.
    // Your usermod will remain compatible as it does not need to implement all methods from the Usermod base class!
};

// add more strings here to reduce flash memory usage
const char SerialForwarding::_name[] PROGMEM = "SerialForwarding";
const char SerialForwarding::_enabled[] PROGMEM = "enabled";

static SerialForwarding serial_forwarding;
REGISTER_USERMOD(serial_forwarding);
