/* 
  Control Arduino motor shield with commands over ethernet.
*/

#include <SPI.h>
#include <Ethernet.h>
#include <TimeLib.h>

// MAC address
byte mac[] = {0xA8, 0x61, 0x0A, 0xAE, 0x89, 0x87};

// read buffer
char buf_arr[9];

// incoming byte
char m;

// termination character
char TERMCHAR = 0x0A;

// comms timeout in seconds
int timeout = 15;
time_t t0;

// return message
String ret = String("");
  
// default motor speed (max 255)
int motor_speed = 50;

// motor state variables
String on = String("off");
String dir = String("fwd");

// identity sring
char idn[60] = "arduino-ethernet-motor_a0ae5240-afba-4c40-b3c8-2b1e218a88c9";

// define ethernet port
EthernetServer server = EthernetServer(30001);

// null IP address
IPAddress no_client(0, 0, 0, 0);


void setup() {
  // keep checking for DHCP address until its available
  while (Ethernet.begin(mac) == 0) {
    delay(1);
  }

  // start listening for clients
  server.begin();

  // setup motor on channel A
  pinMode(12, OUTPUT);         // channel A direction pin
  pinMode(9, OUTPUT);          // channel A brake pin
  digitalWrite(12, HIGH);      // set direction to forward
  digitalWrite(9, HIGH);       // turn on brake to start with
  analogWrite(3, motor_speed); // set speed to default
}

void loop() {
  // maintain an IP lease from the DHCP server
  Ethernet.maintain();

  // check for an incoming client connection with bytes available to read
  EthernetClient client = server.available();
  
  // if the client connection has bytes to read, read them and take action 
  if (client) {
    // read incoming bytes into the buffer
    int i = 0;
    t0 = now();
    while (true) {
      // check error conditions on read
      if ((now() - t0) > timeout) {
        ret = "ERROR: read timeout";
        break;
      }
      else if (i > 8) {
        ret = "ERROR: invalid message";
        break;
      }

      // read a byte into the buffer
      m = client.read();
      if (m == TERMCHAR){
        // end of message reached
        break;  
      }
      buf_arr[i] = m;
      i++;
    }

    if (ret == "") {
      // convert buffer to string
      String cmd = String(buf_arr);
      
      // set motor parameters according to command
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
        // read motor speed from cmd and convert to int
        motor_speed = cmd.substring(5).toInt();
        if (motor_speed > 255) {
          ret = "ERROR: invalid motor speed";
        }
        else {
          analogWrite(3, motor_speed);
        }
      }
      else if (cmd == "status") {
        // check brake for on/off state
        if (digitalRead(9) == 1) {
          on = "off";
        }
        else {
          on = "on";
        }
  
        // check direction
        if (digitalRead(12) == 1) {
          dir = "fwd";
        }
        else {
          dir = "rev";
        }
  
        // build return string
        ret = on + "," + dir + "," + motor_speed;
      }
      else if (cmd == "idn") {
        ret = idn;
      }
      else {
        ret = "ERROR: invalid message";
      }
    }

    // create write buffer (server.write needs char array not String) from 
    // return message and send back to all clients
    int wbuf_len = ret.length() + 1; 
    char wbuf[wbuf_len];
    ret.toCharArray(wbuf, wbuf_len);
    server.write(wbuf);
    server.write(TERMCHAR);

    // close the client connection
    client.stop();

    // reset read buffer
    for (int i = 0; i < 9; i++) {
      buf_arr[i] = '\0';
    }

    // reset return msg
    ret = "";
  }
}
