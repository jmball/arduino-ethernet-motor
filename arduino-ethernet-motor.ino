/* 
  Echo back messages to a client.
*/

#include <SPI.h>
#include <Ethernet.h>
#include <TimeLib.h>

// TODO: add error messages

// MAC address
byte mac[] = {0xA8, 0x61, 0x0A, 0xAE, 0x89, 0x87};

// buffer
char buf_arr[9];

// incoming byte
char m;

// byte index in read buffer
int ri;

// byte index in write buffer
int wi;

// termination character
char TERMCHAR = 0x0A;

// comms timeout in seconds
int timeout = 15;
time_t t0;
int t;

// motor speed
int motor_speed = 35;

// define port
EthernetServer server = EthernetServer(30001);

// null IP address
IPAddress no_client(0, 0, 0, 0);

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  
  // start the Ethernet connection, acquiring an IP from a DHCP server
  Serial.println("Initialize Ethernet with DHCP:");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    } else if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
    }

    // no point in carrying on, so do nothing forevermore:
    while (true) {
      delay(1);
    }
  }

  // print your local IP address:
  Serial.print("My IP address: ");
  Serial.println(Ethernet.localIP());

  // start listening for clients
  server.begin();

  // setup motor on channel A
  pinMode(12, OUTPUT);    // channel A direction pin
  pinMode(9, OUTPUT);     // channel A brake pin
  digitalWrite(12, HIGH); // set direction to forward
  digitalWrite(9, HIGH);  // turn on brake to start with
  analogWrite(3, motor_speed);     // set speed to 25 (max 255)
}

void loop() {
  // maintain an IP lease from the DHCP server
  Ethernet.maintain();

  // if an incoming client connects, there will be bytes available to read:
  EthernetClient client = server.available();

  if (client.remoteIP() != no_client) {
    Serial.print("New client IP address: ");
    Serial.println(client.remoteIP());
  }
  
  // read bytes from the incoming client and write them back
  // to any clients connected to the server: 
  if (client) {
    // read incoming bytes into the buffer
    ri = 0;
    t0 = now();
    t = 0;
    // TODO: Check for invalid string length
    while (t < timeout) {
      m = client.read();
      if (m == TERMCHAR){
        break;  
      }
      buf_arr[ri] = m;
      ri++;
      t = now() - t0;
    }

    // convert buffer to string
    // buf_arr[ri + 1] = '\0';
    String cmd = String(buf_arr);
    
    // print message
    Serial.print("Msg: ");
    Serial.println(cmd);
        
    // write bytes back to all clients
    for (wi = 0; wi <= ri; wi++) {
      server.write(buf_arr[wi]);
    server.write(TERMCHAR);
    }

    // set motor parameters
    // TODO: add status command
    if (cmd == "start") {
      digitalWrite(9, LOW);
    }
    else if (cmd == "stop") {
      digitalWrite(9, HIGH);
    }
    else if (cmd == "fwd") {
      digitalWrite(12, HIGH);
    }
    else if (cmd == "rev") {
      digitalWrite(12, LOW);
    }
    else if (cmd.startsWith("speed")) {
      motor_speed = cmd.substring(5).toInt();
      analogWrite(3, motor_speed);
    }

    // close the client connection
    client.stop();

    // reset buffer
    for (int i = 0; i < 9; i++) {
      buf_arr[i] = '\0';
    }
  }
}
