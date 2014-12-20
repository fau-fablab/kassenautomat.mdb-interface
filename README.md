kassenautomat.mdb-interface
===========================

MDB to USB Interface für Münzwechsler-Ansteuerung


FTDI-Konfiguration
------------------
 * es kommt ein FT232RL zum Einsatz
 * Tool für Linux: https://github.com/mehlis/ft232r_prog.git
 * sonst FT_PROG
 * Linuxzeile: `./ft232r_prog --product 'MDB Interface' --manufacturer 'FAU FabLab' --cbus0 TxLED --cbus1 RxLED --cbus2 Clk12`


Atmel
-----
  * `make fuse` (zur Zeit für external clock via FTDI)
  *  http://www.engbedded.com/fusecalc/
  *  Wenn die Fuses auf externen Takt gesetzt sind, läuft der Chip nur, solange kein USB suspend ist - screen offen halten! (wirklich? nicht jeder hat usb autosuspend an.)
  * `make program`


Sicherung 12V-Schiene
---------------------
  * im worst case sollte eine 5A mtltrg. Sicherung für die 12V-Schiene reichen
 

Aktueller Stand Platine
-----------------------
  * Kommunikation Atmel <-> PC -> MDB geht, MDB -> PC sollte in rev2 auch gehen.
  * RX- und TX-LEDs am FT232RL gehen nicht (TM)
