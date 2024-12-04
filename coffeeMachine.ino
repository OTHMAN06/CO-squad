#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DHT.h>
#include <Ultrasonic.h>
#include <WiFiClientSecure.h>
#include <ESP_Mail_Client.h>

#define DHTPIN D2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "";
const char* password = "";

const char* smtpServer = "smtp.gmail.com";
const int smtpPort = 465;
const char* emailFrom = "abdalrahman.hossam.othman@gmail.com";
const char* emailPassword = "starkothman5";  // App password recommended
const char* emailTo = "grandtheifer5@gmail.com";

ESP8266WebServer server(80);
SMTPSession smtp;

struct Container {
  int triggerPin;
  int echoPin;
  float capacity;
  float height;
  bool isFull;
};

std::vector<Container> containers = {
  {D5, D6, 1000, 20, false},
  {D7, D8, 1000, 20, false}
};

int filledContainersCount = 0;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  dht.begin();

  server.on("/", handleRoot);
  server.on("/set", handleSetValues);
  server.on("/add", handleAddContainer);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();

  for (auto &container : containers) {
    Ultrasonic ultrasonic(container.triggerPin, container.echoPin);
    long distance = ultrasonic.read();
    if (distance <= container.height && !container.isFull) {
      container.isFull = true;
      filledContainersCount++;
      sendEmail(filledContainersCount, distance);
    }
  }

  delay(2000);
}

void handleRoot() {
  String page = "<html>\
  <head>\
    <title>Container Settings</title>\
    <style>\
      body {\
        font-family: Arial, sans-serif;\
        background-color: #f4f4f4;\
        margin: 0;\
        padding: 0;\
        display: flex;\
        justify-content: center;\
        align-items: center;\
        height: 100vh;\
      }\
      .container {\
        background: #ffffff;\
        padding: 20px;\
        border-radius: 8px;\
        box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);\
        width: 400px;\
      }\
      h1, h2 {\
        text-align: center;\
        color: #333;\
      }\
      form {\
        display: flex;\
        flex-direction: column;\
      }\
      input[type=\"number\"], input[type=\"submit\"] {\
        padding: 10px;\
        margin: 10px 0;\
        border: 1px solid #ccc;\
        border-radius: 4px;\
        font-size: 16px;\
      }\
      input[type=\"submit\"] {\
        background-color: #007BFF;\
        color: white;\
        cursor: pointer;\
        border: none;\
        transition: background 0.3s;\
      }\
      input[type=\"submit\"]:hover {\
        background-color: #0056b3;\
      }\
      a {\
        display: block;\
        text-align: center;\
        margin-top: 10px;\
        color: #007BFF;\
        text-decoration: none;\
      }\
      a:hover {\
        text-decoration: underline;\
      }\
    </style>\
  </head>\
  <body>\
    <div class='container'>\
      <h1>Set Container Parameters</h1>\
      <form action='/set' method='get'>\
        Container Index: <input type='number' name='index'><br>\
        Capacity (cm³): <input type='number' name='capacity'><br>\
        Height (cm): <input type='number' name='height'><br>\
        <input type='submit' value='Set'>\
      </form>\
      <h2>Add New Container</h2>\
      <form action='/add' method='get'>\
        Trigger Pin: <input type='number' name='trigger'><br>\
        Echo Pin: <input type='number' name='echo'><br>\
        Capacity (cm³): <input type='number' name='capacity'><br>\
        Height (cm): <input type='number' name='height'><br>\
        <input type='submit' value='Add'>\
      </form>\
      <a href='/'>Back</a>\
    </div>\
  </body>\
  </html>";
  
  server.send(200, "text/html", page);
}

void handleSetValues() {
  int index = server.arg("index").toInt();
  if (index < containers.size()) {
    if (server.hasArg("capacity")) {
      containers[index].capacity = server.arg("capacity").toFloat();
    }
    if (server.hasArg("height")) {
      containers[index].height = server.arg("height").toFloat();
    }
  }
  server.send(200, "text/html", "Container updated!<br><a href=\"/\">Back</a>");
}

void handleAddContainer() {
  Container newContainer;
  newContainer.triggerPin = server.arg("trigger").toInt();
  newContainer.echoPin = server.arg("echo").toInt();
  newContainer.capacity = server.arg("capacity").toFloat();
  newContainer.height = server.arg("height").toFloat();
  newContainer.isFull = false;
  containers.push_back(newContainer);
  
  server.send(200, "text/html", "New container added!<br><a href=\"/\">Back</a>");
}

void sendEmail(int filledCount, long distance) {
  // Configure the SMTP session
  ESP_Mail_Session session;
  session.server.host_name = smtpServer;
  session.server.port = smtpPort;
  session.login.email = emailFrom;
  session.login.password = emailPassword;
  session.time.ntp_server = "pool.ntp.org";
  session.time.gmt_offset = 0;

  // Create the email message
  SMTP_Message message;
  message.sender.name = "ESP8266 Monitor";
  message.sender.email = emailFrom;
  message.subject = "Container Alert";
  message.addRecipient("Recipient", emailTo);
  message.text.content = "Containers Status:\nTotal filled containers: " + String(filledCount) +
                         "\nDistance to full: " + String(distance) + " cm";

  // Attach the callback for debugging
  smtp.debug(1);
  smtp.callback(smtpCallback);

  // Connect using session settings
  if (!smtp.connect(&session)) {
    Serial.println("Failed to connect to SMTP server!");
    return;
  }

  // Send the email
  if (!MailClient.sendMail(&smtp, &message)) {
    Serial.println("Failed to send email: " + smtp.errorReason());
  } else {
    Serial.println("Email sent successfully.");
  }

  smtp.closeSession();
}

void smtpCallback(SMTP_Status status) {
  Serial.println(status.info());
}
