
#include <Servo.h>

Servo myservo;  // create servo object to control a servo
               // a maximum of eight servo objects can be created

int pos_wait = 0; // where should the servo start?
int pos_press = 180; // till where should the servo turn? minus for other direction
int wait = 2000; // how long should the servo wait in the end positions
int button = 7;  // The button is connected to PIN 7
int led = 5; // The LED in the button is connected to PIN 5

// connections:
// SERVO: yellow to PIN 9, other two are plus and GND
// button: black: GND
//         orange: GND
//         yellow: button to PIN7
//         red: LED to PIN 5


void setup()
{
  myservo.attach(9);  // attaches the servo on pin 9 to the servo object
  pinMode(button, INPUT);
  digitalWrite (button, HIGH);
  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);
}

void loop()
{
  myservo.write(pos_wait); 
  if (digitalRead(button) == LOW) {
    digitalWrite(led, HIGH);
    myservo.write(pos_press);
    delay(wait);
    myservo.write(pos_wait);
    delay(wait);
    digitalWrite(led,LOW);
  }
}

