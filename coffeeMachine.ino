#include <ESP8266WiFi.h>           // Library for ESP8266 WiFi functionality
#include <ESP8266WebServer.h>      // Library for setting up a web server
#include <DHT.h>                   // Library for DHT sensor
#include <Ultrasonic.h>            // Library for Ultrasonic sensor
#include <WiFiClientSecure.h>      // Library for secure WiFi connection
#include <ESP_Mail_Client.h>       // Library for email functionality

// DHT sensor configuration
#define DHTPIN D2                 // Pin connected to DHT sensor
#define DHTTYPE DHT11             // Type of DHT sensor (DHT11)
DHT dht(DHTPIN, DHTTYPE);         // Initialize DHT sensor

// WiFi credentials
const char* ssid = "";            // WiFi SSID
const char* password = "";        // WiFi password

// SMTP server configuration for email
const char* smtpServer = "smtp.gmail.com";     // SMTP server address
const int smtpPort = 465;                      // SMTP server port
const char* emailFrom = "your_email@gmail.com"; // Sender email
const char* emailPassword = "your_password";   // Sender email password or app password
const char* emailTo = "recipient_email@gmail.com"; // Recipient email

// Web server on port 80
ESP8266WebServer server(80);

// Email session
SMTPSession smtp;

// Structure to define a container with ultrasonic sensor
struct Container {
  int triggerPin;   // Trigger pin for ultrasonic sensor
  int echoPin;      // Echo pin for ultrasonic sensor
  float capacity;   // Maximum capacity of the container in cm³
  float height;     // Maximum height of the container in cm
  bool isFull;      // Flag to indicate if the container is full
};

// Vector to hold multiple container configurations
std::vector<Container> containers = {
  {D5, D6, 1000, 20, false},   // Container 1 configuration
  {D7, D8, 1000, 20, false}    // Container 2 configuration
};

int filledContainersCount = 0; // Counter for the number of filled containers

void setup() {
  Serial.begin(115200);        // Initialize serial communication
  WiFi.begin(ssid, password);  // Connect to WiFi
  
  // Wait for WiFi connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  dht.begin();                 // Initialize DHT sensor

  // Setup HTTP routes
  server.on("/", handleRoot);                 // Root route to display web interface
  server.on("/set", handleSetValues);         // Route to set container parameters
  server.on("/add", handleAddContainer);      // Route to add new container
  server.begin();                             // Start HTTP server
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();        // Handle incoming HTTP requests

  // Iterate over each container and check its status
  for (auto &container : containers) {
    Ultrasonic ultrasonic(container.triggerPin, container.echoPin); // Initialize ultrasonic sensor
    long distance = ultrasonic.read(); // Measure distance to the surface of coffee grounds

    // Check if container is full and send email if necessary
    if (distance <= container.height && !container.isFull) {
      container.isFull = true;  // Mark container as full
      filledContainersCount++;  // Increment filled containers count
      sendEmail(filledContainersCount, distance); // Send alert email
    }
  }

  delay(2000); // Delay for 2 seconds between sensor readings
}

// Handle root route to display web interface for container management
void handleRoot() {
  String page = "<html>\
// HTML content for web page
  <head>\
    <title>Container Settings</title>\
    <style>\
      /* CSS styling for web page */\
      body { font-family: Arial, sans-serif; background-color: #f4f4f4; }\
    </style>\
  </head>\
  <body>\
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
  </body>\
  </html>";
  
  server.send(200, "text/html", page); // Send HTML response
}

// Handle setting values for a specific container
void handleSetValues() {
  int index = server.arg("index").toInt();  // Get container index from request
  if (index < containers.size()) {
    if (server.hasArg("capacity")) {
      containers[index].capacity = server.arg("capacity").toFloat(); // Update capacity
    }
    if (server.hasArg("height")) {
      containers[index].height = server.arg("height").toFloat(); // Update height
    }
  }
  server.send(200, "text/html", "Container updated!<br><a href=\"/\">Back</a>");
}

// Handle adding a new container configuration
void handleAddContainer() {
  Container newContainer;
  newContainer.triggerPin = server.arg("trigger").toInt();
  newContainer.echoPin = server.arg("echo").toInt();
  newContainer.capacity = server.arg("capacity").toFloat();
  newContainer.height = server.arg("height").toFloat();
  newContainer.isFull = false;
  containers.push_back(newContainer); // Add new container to the vector
  
  server.send(200, "text/html", "New container added!<br><a href=\"/\">Back</a>");
}

// Function to send an email notification when a container is full
void sendEmail(int filledCount, long distance) {
  ESP_Mail_Session session;                 // Create an SMTP session
  session.server.host_name = smtpServer;    // Set SMTP server
  session.server.port = smtpPort;           // Set SMTP port
  session.login.email = emailFrom;          // Set sender email
  session.login.password = emailPassword;   // Set sender password
  session.time.ntp_server = "pool.ntp.org"; // Set NTP server for time synchronization
  session.time.gmt_offset = 0;              // Set GMT offset

  SMTP_Message message;  // Create email message
  message.sender.name = "ESP8266 Monitor";
  message.sender.email = emailFrom;
  message.subject = "Container Alert";
  message.addRecipient("Recipient", emailTo); // Add recipient
  message.text.content = "Containers Status:\nTotal filled containers: " 
                          + String(filledCount) + "\nDistance: " + String(distance);

  smtp.debug(1); // Enable debugging
  smtp.callback(smtpCallback); // Attach callback for status
  if (!smtp.connect(&session)) { // Connect and send email
    Serial.println("Failed to connect to SMTP server!");
    return;
  }
  if (!MailClient.sendMail(&smtp, &message)) {
    Serial.println("Failed to send email: " + smtp.errorReason());
  } else {
    Serial.println("Email sent successfully.");
  }
  smtp.closeSession();
}

// SMTP callback for debugging email sending status
void smtpCallback(SMTP_Status status) {
  Serial.println(status.info());
}
