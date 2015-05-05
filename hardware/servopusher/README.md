servopusher
-----------

The coin-accepting mechanism sometime jams. In this case a lever on top of it has to be pushed.

For this a small system is used which pushes the lever with a servo after the user pushes a switch on the outside of the cash terminal.

After triggering the switch, the microcontroller deactivates the interrupts and drives the servo from position 0 into position 1.
Upon arrival the system starts the return to position 0. Position 0 does not push the lever while position 1 does.

The outside momentary switch is a vandalism safe one with an LED ring. The system could flash the LED ring as a "DONE"-indicator. 
