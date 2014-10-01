kassenautomat.mdb-interface
===========================

MDB to USB Interface für Münzwechsler-Ansteuerung


FTDI-Konfiguration
------------------
 * es kommt ein FT232RL zum Einsatz
 * Tool für Linux: https://github.com/mehlis/ft232r_prog.git
 * sonst FT_PROG
 * Linuxzeile: `./ft232r_prog --product 'MDB Interface' --manufacturer 'FAU FabLab' --cbus0 TxLED --cbus1 RxLED --cbus2 Clk12`


Programmieren
-------------
  * Wenn der Chip in Auslieferungszustand ist, sollte B8 oder B9 bei avrdude eingestellt werden
  * Bei konfigurierter Taktquelle kann auch mehr Geschwindigkeit eingesetzt werden
  
Sicherung 12V-Schiene
---------------------
  * im worst case sollte eine 5A mtltrg. Sicherung für die 12V-Schiene reichen

Aktueller Stand Platine
-----------------------
  * Serial-Loopback-Test erfolgreich - auf der seriellen Konsole kommt was raus, wenn man RX und TX verbindet
  * RX- und TX-LEDs am FT232RL gehen nicht (TM)
  * UART am AVR mag auch nich nicht (scheint noch falsch konfiguriert zu sein, mit ungelöschtem Chip stört er die RX/TX des FT232RL sodass nichts mehr durchkommt)
