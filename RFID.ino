#include <SPI.h>
#include <MFRC522.h>

#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>

constexpr uint8_t RST_PIN = D2;     // Configurable, see typical pin layout above
constexpr uint8_t SS_PIN = D4;     // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance
char c;

#define FIREBASE_HOST "iot-domotic.firebaseio.com"
#define FIREBASE_AUTH "xKfX5zoUrQ5lxa4n5oDFlJORUW8axXZKFttpvjrT"
#define WIFI_SSID "vw-10613"
#define WIFI_PASSWORD "ZTE1RTHH4Q04180"

void setup() {
  Serial.begin(9600);        // Initialize serial communications with the PC
  SPI.begin();               // Init SPI bus
  mfrc522.PCD_Init();        // Init MFRC522 card

  // connect to wifi.
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());
  
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  
}

void loop()
{
  Serial.println("*****************************************");
  Serial.println("*\t1. Registrar Nuevo Usuario\t*");
  Serial.println("*\t2. Registrar Monto de Compra\t*");
  Serial.println("*****************************************");
  while(!Serial.available()){}
  c=Serial.read();
  switch(c)
  {
    case '1':
    Serial.println("Coloque la targeta del usuario sobre el lector de targeta y no lo quite de ahi");
    Escribir();
    break;
    case '2':
    Serial.println("Introduzca el monto total de las compras que realizo el usuario:");
    while(!Serial.available()){}
    float Monto = Serial.parseFloat();
    Serial.print(Monto);
    Serial.println("  Bs");
    Serial.println("Coloque la targeta del cliente");
    Leer(Monto);
    break;
  }
}
void Escribir()
{
  // Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery from the factory.
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  // Look for new cards
  while ( ! mfrc522.PICC_IsNewCardPresent()) {
    //return;
  }

  // Select one of the cards
  while ( ! mfrc522.PICC_ReadCardSerial()) {
    //return;
  }

  ////Serial.print(F("Card UID:"));    //Dump UID
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    ////Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    ////Serial.print(mfrc522.uid.uidByte[i], HEX);
  }
  ////Serial.print(F(" PICC type: "));   // Dump PICC type
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  ////Serial.println(mfrc522.PICC_GetTypeName(piccType));

  byte buffer[34];
  byte block;
  MFRC522::StatusCode status;
  byte len;

  Serial.setTimeout(40000L) ;     // wait until 40 seconds for input from serial
  // Ask personal data: Family name
  Serial.println(F("Entra NickName terminando con #"));
  len = Serial.readBytesUntil('#', (char *) buffer, 30) ; // read family name from serial
  for (byte i = len; i < 30; i++) buffer[i] = ' ';     // pad with spaces

  block = 1;
  //Serial.println(F("Authenticating using key A..."));
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    //Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  else Serial.println(F("___USUARIO REGISTRADO EXITOSAMENTE____\n "));

  // Write block
  status = mfrc522.MIFARE_Write(block, buffer, 16);
  if (status != MFRC522::STATUS_OK) {
    //Serial.print(F("MIFARE_Write() failed: "));
    //Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  else Serial.println(F("\n\t**Fin del proceso**\n"));
  Serial.println(" ");
  mfrc522.PICC_HaltA(); // Halt PICC
  mfrc522.PCD_StopCrypto1();  // Stop encryption on PCD
}
void Leer(float Monto)
{
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  //some variables we need
  byte block;
  byte len;
  MFRC522::StatusCode status;

  //-------------------------------------------

  // Look for new cards
  while ( ! mfrc522.PICC_IsNewCardPresent()) {
    //return;
  }

  // Select one of the cards
  while ( ! mfrc522.PICC_ReadCardSerial()) {
    //return;
  }
  Serial.println(F("\n\t**Card Detected:**"));

  //-------------------------------------------

  //mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid)); //dump some details about the card

  //mfrc522.PICC_DumpToSerial(&(mfrc522.uid));      //uncomment this to see all blocks in hex
  len = 18;
  //---------------------------------------- GET LAST NAME

  byte buffer2[18];
  block = 1;

  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 1, &key, &(mfrc522.uid)); //line 834
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Authentication failed: "));
    //Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  status = mfrc522.MIFARE_Read(block, buffer2, &len);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Reading failed: "));
    //Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  //PRINT LAST NAME
  String codigo = "";
  for (uint8_t i = 0; i < 16; i++) {
    //Serial.write(buffer2[i] );
    char tt=buffer2[i];
    codigo = codigo + String(tt);
  }
  float adicionar = Firebase.getFloat(codigo);
  Serial.print("Usuario:____________________________________");
  Serial.println(codigo);
  Serial.print("Puntos Acumulados:__________________________");
  Serial.println(adicionar);
  Serial.print("Puntos Que se sumaran con esta compra:______");
  
  
  //Serial.println("Introduzca el monto total que el cliente compro en Bolivianos:");
  //while(!Serial.available()){}
  float variable = Monto;
  
  variable = variable * 0.05;
  Serial.println(variable);
  adicionar = adicionar + variable;
  // set string value
  
  Serial.print("Puntos Acumulados Ahora:____________________");
  Serial.println(adicionar);
  Firebase.setFloat(codigo, adicionar);
  //----------------------------------------

  Serial.println(F("\n\t**Fin del proceso**\n"));

  delay(3500); //change value if you want to read cards faster
  Serial.flush();
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}
