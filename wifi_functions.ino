extern String kerberos = "canepa";

//WIFI global constants and variables
extern const int wifiControlPin = 2;
extern String wifis = "";        // holds list of wifisc
extern String get_response ="";  // generic string to hold responses
extern bool wifi_good = false;   // connected to AP
extern String ssid = "testnet";
extern String password = "password";

void setup_wifi(){
  wifi.begin();
  wifi.connectWifi(ssid, password);
}


String load_combo() {
  String domain = "iesc-s2.mit.edu";
  int port = 80;
  String path = "/student_code/" + kerberos + "/dev1/sb1.py";
  String resp = "";
  if (wifi.isConnected()) {
    Serial.print("Sending request at t=");
    Serial.println(millis());
    wifi.sendRequest(GET, domain, port, path , "", true);
    unsigned long t = millis();
    while (!wifi.hasResponse() && millis() - t < 5000); //wait for response
    if (wifi.hasResponse()) {
      resp = wifi.getResponse();
      Serial.print("Got response at t=");
      Serial.println(millis());
      //Serial.println(resp);
    } else {
      Serial.println("No timely response");
    }
  }
  return resp;
}

void send_combo(String passcode) {
  if (wifi.isConnected()){
    Serial.print("Sending request at t=");
    Serial.println(millis());
    String domain = "iesc-s2.mit.edu";
    int port = 80;
    String path = "/student_code/" + kerberos + "/dev1/sb1.py";
    String postdata = "combo=" + passcode;
    wifi.sendRequest(POST, domain, port, path, postdata, false);
    unsigned long t = millis();
    while (!wifi.hasResponse() && millis() - t < 5000); //wait for response
    if (wifi.hasResponse()) {
      String resp = wifi.getResponse();
      Serial.print("Got response at t=");
      Serial.println(millis());
      //Serial.println(resp);
    } else {
      Serial.println("No timely response");
    }
  }
}
