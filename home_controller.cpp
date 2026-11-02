#include <SPI.h>
#include <Ethernet.h>
#include <avr/wdt.h>

byte mac[] = { 0x02, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Static network settings: pick an address OUTSIDE your router's DHCP pool.
IPAddress ip(192, 168, 0, 90);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dnsServer(192, 168, 0, 1);

const uint8_t PIN_NORTH = 6;
const uint8_t PIN_SOUTH = 7;

// Flip these if your relay module is active-LOW.
const uint8_t RELAY_ON  = HIGH;
const uint8_t RELAY_OFF = LOW;

const unsigned long PULSE_MS          = 1000;
const unsigned long COOLDOWN_MS       = 3000;
const unsigned long CLIENT_TIMEOUT_MS = 2000;

EthernetServer server(80);

bool linkWasDown = false;

// ---- Non-blocking relay pulse ----
int8_t activePin = -1;          // pin currently pulsing, -1 = none
unsigned long pulseStart = 0;
bool cooldown = false;
unsigned long cooldownStart = 0;

bool startPulse(uint8_t pin)
{
    if (activePin != -1 || cooldown) return false;  // busy
    activePin = pin;
    pulseStart = millis();
    digitalWrite(pin, RELAY_ON);
    return true;
}

void updatePulse()
{
    unsigned long now = millis();
    if (activePin != -1 && now - pulseStart >= PULSE_MS)
    {
        digitalWrite(activePin, RELAY_OFF);
        activePin = -1;
        cooldown = true;
        cooldownStart = now;
    }
    if (cooldown && now - cooldownStart >= COOLDOWN_MS)
    {
        cooldown = false;
    }
}

// ---- Network ----
void startNetwork()
{
    Ethernet.begin(mac, ip, dnsServer, gateway, subnet);
    server.begin();
}

void sendResponse(EthernetClient& client, const char* status, const char* body)
{
    client.print("HTTP/1.1 ");
    client.println(status);
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.println(body);
}

void routeRequest(EthernetClient& client, const char* requestLine)
{
    char path[32] = "";

    // extract the text between the first and second space
    if (strncmp(requestLine, "GET ", 4) == 0)
    {
        const char* start = requestLine + 4;
        const char* end = strchr(start, ' ');
        if (end)
        {
            size_t n = end - start;
            if (n < sizeof(path))
            {
                memcpy(path, start, n);
                path[n] = '\0';
            }
        }
    }

    // strip query string if present (/a?x=1 -> /a)
    char* q = strchr(path, '?');
    if (q) *q = '\0';

    Serial.print("Path: ");
    Serial.println(path);

    if (strcmp(path, "/a") == 0)
    {
        if (startPulse(PIN_NORTH)) sendResponse(client, "200 OK", "A done");
        else                       sendResponse(client, "503 Service Unavailable", "Busy");
    }
    else if (strcmp(path, "/b") == 0)
    {
        if (startPulse(PIN_SOUTH)) sendResponse(client, "200 OK", "B done");
        else                       sendResponse(client, "503 Service Unavailable", "Busy");
    }
    else
    {
        sendResponse(client, "404 Not Found", "Not found");
    }
}

void handleClient(EthernetClient& client)
{
    char requestLine[64];
    uint8_t len = 0;
    bool firstLineDone = false;
    bool currentLineIsBlank = true;
    unsigned long startedAt = millis();

    while (client.connected() && millis() - startedAt < CLIENT_TIMEOUT_MS)
    {
        if (!client.available()) continue;

        char c = client.read();

        // capture the first line only, e.g. "GET /a HTTP/1.1"
        if (!firstLineDone)
        {
            if (c == '\n')
            {
                requestLine[len] = '\0';
                firstLineDone = true;
            }
            else if (c != '\r' && len < sizeof(requestLine) - 1)
            {
                requestLine[len++] = c;
            }
        }

        // blank line = end of headers, so now we can respond
        if (c == '\n' && currentLineIsBlank)
        {
            routeRequest(client, requestLine);
            break;
        }

        if (c == '\n')       currentLineIsBlank = true;
        else if (c != '\r')  currentLineIsBlank = false;
    }

    delay(1);
    client.stop();
}

void setup()
{
    wdt_disable();

    // Safe relay state first, before anything else can take time.
    digitalWrite(PIN_NORTH, RELAY_OFF);
    digitalWrite(PIN_SOUTH, RELAY_OFF);
    pinMode(PIN_NORTH, OUTPUT);
    pinMode(PIN_SOUTH, OUTPUT);

    Serial.begin(9600);
    delay(1000);  // give the W5500 time to power up
    Serial.println("Arduino - Home Control");

    startNetwork();

    if (Ethernet.hardwareStatus() == EthernetNoHardware)
    {
        Serial.println("Ethernet hardware not found, rebooting to retry...");
        wdt_enable(WDTO_2S);
        while (true);  // watchdog resets the board and we try again
    }

    Serial.print("Web Server IP Address: ");
    Serial.println(Ethernet.localIP());

    wdt_enable(WDTO_8S);
}

void loop()
{
    wdt_reset();
    updatePulse();

    // Re-initialize the W5500 when the cable/router comes back.
    if (Ethernet.linkStatus() == LinkOFF)
    {
        linkWasDown = true;
    }
    else if (linkWasDown)
    {
        linkWasDown = false;
        Serial.println("Link restored, re-initializing");
        startNetwork();
    }

    EthernetClient client = server.available();
    if (client)
    {
        handleClient(client);
    }
}
