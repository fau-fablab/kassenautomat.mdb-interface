kassenautomat.mdb-interface
===========================

MDB to USB Interface für Münzwechsler-Ansteuerung


FTDI-Konfiguration
------------------
 * es kommt ein FT232RL zum Einsatz
 * Tool für Linux: https://github.com/mehlis/ft232r_prog.git
 * sonst FT_PROG
 * Linuxzeile: `./ft232r_prog --product 'MDB Interface' --manufacturer 'FAU FabLab' --cbus0 TxLED --cbus1 RxLED --cbus2 Clk12`
