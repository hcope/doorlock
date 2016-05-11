// Use ESP8266 to repeatedly send GET command to server and parse 
// response.
//
// Written by Joe Steinmeyer, Joel Voldman
// 2015, 2016
// Uses some helper functions inspired by ESP8266 library 
// itead/ITEADLIB_Arduino_ESP8266

//Most functions in here are from L02A for 6.S08.
//Some have been removed (including loop/setup) or edited by hcope and canepa

#include <ESP8266.h>

#define wifiSerial Serial1
#define serialYes true

//WIFI global constants and variables
const int wifiControlPin = 2;
String wifis = "";                // holds list of wifisc
String MAC = "";                  // MAC address
String get_response ="";          // generic string to hold responses
bool wifi_good = false;           // connected to AP
String SSID = "testnet";     // SSID and password
String password = "password";


void wifiEnable(bool yn){
  digitalWrite(wifiControlPin,LOW);
  if (yn){
    delay(250);
    digitalWrite(wifiControlPin,HIGH);
    delay(1000);
  } 
}

void emptyRx() { // Empty ESP8266 buffer
    while(wifiSerial.available() > 0) {
        wifiSerial.read();
    }
}

bool check() {
  emptyRx();
  wifiSerial.println("AT");
  if (serialYes) Serial.println("checking..");
  boolean ok = false;
  if (wifiSerial.find("OK")) {
      if (serialYes) Serial.println("ESP8266 present");
      ok = true;
  }
  return ok;
}

void printWifiResponse(){
  while (wifiSerial.available()>0){
    char cByte = wifiSerial.read();
    Serial.write(cByte);
  }
}

void resetWifi() {
  // set station mode
    wifiSerial.println("AT+CWMODE=3");
    delay(1000);//give some breathing room
    wifiSerial.println("AT+RST"); //reset required to take effect
    delay(2000);
    if (wifiSerial.find("ready")){
      if (serialYes) Serial.println("ESP8266 restarted and ready");
    }
    printWifiResponse();
}

bool connectWifi(String ssid, String password) {
    emptyRx();
    String cmd = "AT+CWJAP=\"" + ssid + "\",\"" + password + "\"";
    wifiSerial.println(cmd);
    unsigned long start = millis();
    String response="";
    while (millis() - start <9000){ //probably can rewrite this thing if needed.
      if (wifiSerial.available()){
        char c = wifiSerial.read();
        if (serialYes) Serial.print(c);
        response = String(response+c);
      }
      if (response.indexOf("OK") != -1){
        break;
      }
    }
    if(response.indexOf("OK") !=-1) {
        wifiSerial.println("AT+CIFSR");
        String resp2 = "";
        start = millis();
        while(millis()-start < 6000){
          if(wifiSerial.available()){
            char c = wifiSerial.read();
            resp2 = String(resp2+c);
          }
        }
        if (serialYes){
          Serial.println("Device IP Info: ");
          Serial.println(resp2);
          Serial.println("Connected!");
          return true;
        }
    }
    else {
        if (serialYes) Serial.println("Cannot connect to wifi");
        return false;
    }
}

String readString(String target1, String target2, String target3, uint32_t timeout)
{
    String data;
    char a;
    unsigned long start = millis();
    while (millis() - start < timeout) {
        while(wifiSerial.available() > 0) {
            a = wifiSerial.read();
      if(a == '\0') continue;
            data += a;
        }
        if (data.indexOf(target1) != -1) {
            break;
        } else if (data.indexOf(target2) != -1) {
            break;
        } else if (data.indexOf(target3) != -1) {
            break;
        }
    }
    return data;
}

// Set up TCP connection
bool startComm(String domain, int port) {

  String data;
  String start_comm = "AT+CIPSTART=\"TCP\",\"" + domain + "\"," + String(port);     
  wifiSerial.println(start_comm);
  if (serialYes) Serial.println(start_comm);

  data = readString("OK", "ERROR", "ALREADY CONNECT", 10000);
    if (data.indexOf("OK") != -1 || data.indexOf("ALREADY CONNECT") != -1) {
        return true;
    }
  return false;
}

bool setMux(int m) {
  
  String data;
  wifiSerial.print("AT+CIPMUX=");
  wifiSerial.println(m);
  data = readString("OK","ERROR","XX",4000);
  
  if (data.indexOf("OK") != -1) {
        return true;
  }
  return false;
}

// Read data from the wifi and test it for any of three target strings
bool readTest(String target1, String target2, String target3, uint32_t timeout) {

    String data_tmp;
    data_tmp = readString(target1, target2, target3, timeout);
    if (data_tmp.indexOf(target1) != -1) {
        return true;
    } else if (data_tmp.indexOf(target2) != -1) {
        return true;
    } else if (data_tmp.indexOf(target3) != -1) {
        return true;
    } else {
    return false;
    }
}



bool sendComm(String buffer, int len) {
    wifiSerial.print("AT+CIPSEND=");    //send length command
    wifiSerial.println(len);
    if (readTest(">", "XX", "XX", 5000)) {    //if we get '>', send rest
        emptyRx();
        for (uint32_t i = 0; i < len; i++) {
            wifiSerial.write(buffer[i]);
        }
        return readTest("SEND OK", "XX", "XX", 10000);
    }
    return false;
}

String httpComm(String domain, int port, String path, String comm) {  
  String response;  
  emptyRx();          // empty buffer
  if (setMux(0)) {    // set mux
    emptyRx();
    
    if (startComm(domain, port)) {  // set up tcp
      emptyRx();  
      
      if (sendComm(comm, comm.length())) {  //send command
        response = receiveData(5000);       //receive response
      } else {
        Serial.println("Send failed");
      } 
    } else {
      Serial.println("Unable to start connection");
    }
  } else {
    Serial.println("MUX command failed");
  }
  wifiSerial.println("AT+CIPCLOSE");        //close tcp connection
  return response;
}

\\\\\\\

void initializeWifi() {
  if (serialYes) Serial.begin(9600); //number meaningless on teensy (1MBit/s)
  //wifi serial setup:
  wifiSerial.begin(115200);
  wifiSerial.setTimeout(4000);
  pinMode(wifiControlPin,OUTPUT);
  wifiEnable(true);
  if (check()){      //if ESP8266 is present
    Serial.println("wifi present");
    resetWifi();    //reset
    MAC = getMAC(1500);
    emptyRx();
  }
}

void requester(String PostData){ //sends POST Request to dev1 py file
{
  if (wifi_good != true){   //if we're not connected to a network
    if (check()){       //if ESP8266 present
      listWifis();        
      if (serialYes) Serial.println(wifis);

      bool good = connectWifi(SSID,password); //connect to network

      if (good){
        wifi_good = true;   // connected to a network
      }else{
        wifi_good=false;
      }
    }
  }
    
  String returnData = "";
  String steps = "";
  if (wifi_good){   //if we're connected to a network
  
    // web server parameters
    String domain = "iesc-s2.mit.edu";
    int port = 80;
    String path = "student_code/canepa/dev1/sb1.py";

    // send a GET command, will return weather info
    String send_comm = 
    "POST "+ path + " HTTP/1.1\r\n" +
    "Host: " + domain + "\r\n\r\n" +
    "Content-Type: application/x-www-form-urlencoded; charset=UTF-8") + "Content-Length:" + 
    (passcode.length()+12) + "\n" + PostData; 
    get_response = httpComm(domain, port, path, send_comm);
    if (serialYes) Serial.println(get_response);
    delay(200);      // delay 0.2 secs
    }  
}
