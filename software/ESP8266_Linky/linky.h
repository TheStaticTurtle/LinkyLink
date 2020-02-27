#ifndef Linky_h
#define Linky_h

#include "Arduino.h"

#define LINKY_BUFFER_TELEINFO_SIZE  100
#define LINKY_START_FRAME           0x02
#define LINKY_END_FRAME             0x03
#define LINKY_START_LINE            0x0A
#define LINKY_END_LINE              0x0D
#define LINKY_END_DATA              0x04

#define LINKY_COM_RATE				1200

struct LinkyData {
	char   ADCO   [13];      //Adresse du compteur
	char   OPTARIF[ 5];      //Option tarifaire choisie
	int    ISOUSC;           //Intensité souscrite
	int    BASE;             //Index option Base
	long   HCHC;             //Index option Heures Creuses - Heures Creuses
	long   HCHP;             //Index option Heures Creuses - Heures Pleines
	long   EJPHN;            //Index option EJP - Heures Normales
	long   EJPHPM;           //Index option EJP - Heures de Pointe Mobile
	long   BBRHCJB;          //Index option Tempo - Heures Creuses Jours Bleus
	long   BBRHPJB;          //Index option Tempo - Heures Pleines Jours Bleus
	long   BBRHCJW;          //Index option Tempo - Heures Creuses Jours Blancs
	long   BBRHPJW;          //Index option Tempo - Heures Pleines Jours Blancs
	long   BBRHCJR;          //Index option Tempo - Heures Pleines Jours Rouges
	long   BBRHPJR;          //Index option Tempo - Heures Pleines Jours Rouges
	int    PEJP;             //Préavis Début EJP (30 min)
	char   PTEC   [ 5];      //Période Tarifaire en cours
	char   DEMAIN [ 5];      //Couleur du lendemain
	int    IINST;            //Intensité Instantanée
	int    IINST1;           //Intensité Instantanée Phase n°1 (Triphaser seulement)
	int    IINST2;           //Intensité Instantanée Phase n°2 (Triphaser seulement)
	int    IINST3;           //Intensité Instantanée Phase n°3 (Triphaser seulement)
	int    ADPS;             //Avertissement de DépassementDe Puissance Souscrite
	int    ADIR1;            //Avertissement de DépassementDe Puissance Souscrite Phase n°1 (Triphaser seulement)
	int    ADIR2;            //Avertissement de DépassementDe Puissance Souscrite Phase n°2 (Triphaser seulement)
	int    ADIR3;            //Avertissement de DépassementDe Puissance Souscrite Phase n°3 (Triphaser seulement)
	int    IMAX;             //Intensité maximale appelée
	int    IMAX1;            //Intensité maximale appelée Phase n°1 (Triphaser seulement)
	int    IMAX2;            //Intensité maximale appelée Phase n°2 (Triphaser seulement)
	int    IMAX3;            //Intensité maximale appelée Phase n°3 (Triphaser seulement)
	long   PMAX;             //Puissance maximale triphasée atteinte
	long   PAPP;             //Puissance apparente / Puissance apparente triphasée soutirée
	char   HHPHC;            //Horaire Heures Pleines Heures Creuses
	char   MOTDETAT[ 7];     //Mot d'état du compteur
	char   PPOT    [ 3];     //Présence des potentiels (Triphaser seulement) ("0X", X = coupures de phase phase n => bit n = 1)
}; 

class SoftwareSerial;

class Linky
{

	public:
		Linky(int RXPin,int TXPin,int DATAENpin);
		void begin();
		void end();
		bool updateAsync();
		bool updateAsync(int blinkPin, bool invert);
		void waitEndMessage();
		time_t lastFullFrame();
		LinkyData grab();
	private:
		void updateStruct(int len) ;
		void processRXChar(char c);
		bool validateInput(String input, bool hasLetters, bool hasNumbers, bool hasSpecial);
		bool isValidNumber(String str);
		int getCommandValue_int(String CMD, int CMDlenght, int CMDResultLenght, String line, int lineLenght, int defaultIfError);
		long getCommandValue_long(String CMD, int CMDlenght, int CMDResultLenght, String line, int lineLenght, long defaultIfError) ;
		bool getCommandValue_str(String CMD, int CMDlenght, int CMDResultLenght, String line, int lineLenght, char* value, bool hasLetters, bool hasNumbers, bool hasSpecial);
		int 			_pin_RX;
		int 			_pin_TX;
		int				_DATAENpin;
		SoftwareSerial* _serport;
		bool			_serialEnabled = false;
		int 			_bufferIterator;
		char 			_buffer[LINKY_BUFFER_TELEINFO_SIZE];
		LinkyData 		_data;
		time_t 			_lastFrame;
};

#endif 
