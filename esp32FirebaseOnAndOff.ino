#include <NTPClient.h>
#include <Array.h>
#include <WiFi.h>
#include <IOXhop_FirebaseESP32.h>
#include <SPI.h>
#include <MFRC522.h>


//RFID
#define SS_PIN 21 //SS pin
#define RST_PIN 22 //Reset pin

#define RELAY 13 //Relay pin

#define GREENLED 15 //Green led pin (used when permission to enter had been verified)
#define BUZZER 2 //Buzzer pin (used when permission to enter had been rejected and on boot up)
#define REDLED 4 //Red light led (used when permission to enter had been rejected)
#define FIREBASE_HOST //Firebase host URL
#define FIREBASE_AUTH //Firebase authentication key
#define WIFI_SSID //Wifi SSID
#define WIFI_PASSWORD //Wifi password
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

const long utcOffsetInSeconds = 10800;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);


const String masterRfid; //Master card Serial , Used to set new rfid cards
const int maxUsers = 15;
int USERS_COUNT;
String usersRfidArray[2][maxUsers];
int firestatus;
int Counter = 0;
int LastScanned;
String fireS = "";
String firebaseNamePath = "RFID_ARRAY/name/";
String firebaseRfidUidPath = "RFID_ARRAY/rfid_uid/";
String users_Array[3][5];
String rfidUid;


void setup() {

  pinMode(GREENLED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(REDLED, OUTPUT);
  Serial.begin(9600);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.println("Connected.");
  turnAllOnAndOff(); //Notifying the user that the wifi connection has been successfully established
  timeClient.begin(); //In order to set the logs we use timeclient to contain the current time




  Serial.println(WiFi.localIP());
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  USERS_COUNT = Firebase.getInt("RFID_ARRAY/USERS_COUNT"); //Number of users
  String firebaseNamePath = "RFID_ARRAY/name/";
  String firebaseRfidUidPath = "RFID_ARRAY/rfid_uid/";
  for (int i = 0; i <= USERS_COUNT; i++)
  {
    String iStringed = String(i);
    String getIsAllowedPath = "RFID_ARRAY/" + iStringed + "/isAllowed";
    String getNamePath = "RFID_ARRAY/" + iStringed + "/name";
    String getRfidUidPath = "RFID_ARRAY/" + iStringed + "/rfid_uid";
    String isAllowed = Firebase.getString(getIsAllowedPath);
    String getName = Firebase.getString(getNamePath);
    String getRfidUid = Firebase.getString(getRfidUidPath);
    users_Array[0][i] = getName;
    users_Array[1][i] = getRfidUid;
    users_Array [2][i] = isAllowed;

  };
  Serial.println(users_Array[0][0]);
  Serial.println(users_Array[1][0]);
  Serial.println(users_Array[0][1]);
  Serial.println(users_Array[1][1]);

  delay(5000);
  SPI.begin();          // Initiate  SPI bus
  mfrc522.PCD_Init();   // Initiate MFRC522
}

void loop()
{



  if (mfrc522.PICC_IsNewCardPresent())
  {
    Serial.print("scanned");
    if (mfrc522.PICC_ReadCardSerial())
    {
      Serial.println("UID tag :");
      String content = "";
      byte letter;
      for (byte i = 0; i < mfrc522.uid.size; i++)
      {
        Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" :  " ");
        Serial.print(mfrc522.uid.uidByte[i], HEX);
        content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
        content.concat(String(mfrc522.uid.uidByte[i], HEX));
      }

      content.toUpperCase();
      String scannedUID = content.substring(1);
      if (scannedUID == masterRfid)
      {
        Serial.println("MASTER CARD");

        String newUser = Firebase.getString("NEW_RFID");
        if (newUser != "N")
        {
          digitalWrite(GREENLED, HIGH);
          delay(2000);
          turnAllOn();
          rfidUid = returnScannedUid();
          String PATH = "RFID_ARRAY/" + String(USERS_COUNT) + "/";
          String setRfidUidPath = PATH + "/rfid_uid";
          String setNamePath = PATH + "/name";
          String setIsAllowedPath = PATH + "/isAllowed";
          Firebase.setString(setRfidUidPath, rfidUid);
          Firebase.setString(setNamePath, newUser);
          Firebase.setString(setIsAllowedPath, "true");
          Firebase.setString("NEW_RFID", "N");
          USERS_COUNT++;
          Firebase.setInt("RFID_ARRAY/USERS_COUNT", USERS_COUNT);
          turnAllOff();
          ESP.restart();


        }
        else
        {
          openDoor();
        }

      }
      else
      {

        Serial.println("else/notmaster");
        if (isInTheArray(USERS_COUNT, users_Array, scannedUID))
        {
          Serial.println("In the system");
          openDoor();
          setLog();
        }
        else
        {
          digitalWrite(REDLED, HIGH);
          delay(2000);
          digitalWrite(REDLED, LOW);
        }
      }
    }

  }



  firestatus = Firebase.getInt("LED_STATUS");
  if ((firestatus) == 1)
  {
    Serial.println("Led Turned ON");
    openDoor();
    Firebase.setInt("LED_STATUS", 0);
  }

  else {
    Counter++;
    delay(100);
    Serial.println("Command Error! Please send 0/1");
    Serial.println(Counter);
  }

}

boolean isInTheArray(int arrayLength, String uid_list[][5], String rfidUid)
{
  String UID = rfidUid;
  for (int i = 0; i < arrayLength; i++)
  {
    Serial.println(String(i) + "    arrayLegnth ==" + String(arrayLength));
    Serial.println(uid_list[1][i]);
    Serial.println("UID CURRENT" + UID);
    if (UID == uid_list[1][i]  && uid_list[2][i] == "true")
    {
      LastScanned = i;
      return true;
    }
  }
  return false;
}
void openDoor()
{
  digitalWrite(RELAY, HIGH);
  digitalWrite(GREENLED, HIGH);
  delay(3000);
  digitalWrite(RELAY, LOW);
  digitalWrite(GREENLED, LOW);

}
void turnAllOn()
{
  digitalWrite(REDLED, HIGH);
  digitalWrite(BUZZER, HIGH);
  digitalWrite(GREENLED, HIGH);
}

void turnAllOff()
{
  digitalWrite(REDLED, LOW);
  digitalWrite(BUZZER, LOW);
  digitalWrite(GREENLED, LOW);
}

void turnAllOnAndOff()
{
  turnAllOn();
  sleep(3);
  turnAllOff();
}
void setLog()
{
  timeClient.update();
  unsigned long epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime ((time_t *)&epochTime);
  int monthDay = ptm->tm_mday;
  int currentMonth = ptm->tm_mon + 1;
  String Time = String(monthDay)  + "." + String(currentMonth) + "," + String(daysOfTheWeek[timeClient.getDay()]) + "," + String(timeClient.getHours()) + ":" + String(timeClient.getMinutes()) + ":" + String(timeClient.getSeconds());
  String scannedPath = "RFID_ARRAY/" + String(LastScanned) + "/" + rfidUid + "/Logs";
  Firebase.pushString(scannedPath, Time);
}


String returnScannedUid()
{
  if (mfrc522.PICC_IsNewCardPresent())
  {
    Serial.print("scanned");
    if (mfrc522.PICC_ReadCardSerial())
    {
      Serial.print("UID tag :");
      String content = "";
      byte letter;
      for (byte i = 0; i < mfrc522.uid.size; i++)
      {
        Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" :  " ");
        Serial.print(mfrc522.uid.uidByte[i], HEX);
        content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
        content.concat(String(mfrc522.uid.uidByte[i], HEX));
      }

      content.toUpperCase();

      return content.substring(1);

    }
  }
}
