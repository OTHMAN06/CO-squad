#include <ESP8266WiFi.h>              // Wi-Fi library for ESP8266
#include <ESP8266WebServer.h>         // Web server library for ESP8266
#include <DHT.h>                      // Library for DHT sensors (temperature & humidity)
#include <Ultrasonic.h>               // Library for Ultrasonic sensors (distance measurement)
#include <WiFiClientSecure.h>         // Enables secure client connections
#include <ESP_Mail_Client.h>          // Library for sending emails via SMTP

#define DHTPIN D2                     // Pin connected to the DHT sensor
#define DHTTYPE DHT11                 // Define the type of DHT sensor used (DHT11)
DHT dht(DHTPIN, DHTTYPE);             // Create a DHT object

// Wi-Fi credentials
const char* ssid = "Abdalrahman's-phone";  // Wi-Fi SSID
const char* password = "123456789";        // Wi-Fi password

// SMTP server and email details
const char* smtpServer = "smtp.gmail.com";          // SMTP server address
const int smtpPort = 465;                           // SMTP server port (SSL)
const char* emailFrom = "othman.mohamed.1142005@gmail.com";  // Sender email
const char* emailPassword = "gkve qhih vwcm nsvv";  // App password for sender email
const char* emailTo = "grandtheifer5@gmail.com";    // Recipient email

ESP8266WebServer server(80);        // Create a web server on port 80
SMTPSession smtp;                   // Create an SMTP session object

// Structure to hold container data
struct Container {
  int triggerPin;                   // Pin connected to ultrasonic trigger
  int echoPin;                      // Pin connected to ultrasonic echo
  float capacity;                   // Container capacity in cm³
  float height;                     // Container height in cm
  bool isFull;                      // Flag to indicate if the container is full
  bool emailSent;                   // Flag to track if email was sent
};

// Initialize a list of containers with default values
std::vector<Container> containers = {
  {D5, D6, 1000, 20, false, false}  // Container with specific pins and dimensions
};

void setup() {
  Serial.begin(115200);             // Start serial communication at 115200 baud rate
  WiFi.begin(ssid, password);       // Connect to the Wi-Fi network

  // Wait until connected to Wi-Fi
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);                    // Wait 1 second
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("Server IP Address: ");
  Serial.println(WiFi.localIP());   // Print the IP address of the server

  dht.begin();                      // Initialize DHT sensor

  // Define routes for the web server
  server.on("/", handleRoot);       // Home page route
  server.on("/set", handleSetValues); // Route for setting container parameters
  server.on("/add", handleAddContainer); // Route for adding new containers
  server.begin();                   // Start the web server
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();            // Handle incoming client requests

  // Loop through each container and monitor its status
  for (auto &container : containers) {
    Ultrasonic ultrasonic(container.triggerPin, container.echoPin); // Initialize ultrasonic sensor
    long distance = ultrasonic.read();  // Read distance from ultrasonic sensor

    if (distance <= container.height && !container.isFull) {  // If container is full
      container.isFull = true;      // Mark container as full
      container.emailSent = false;  // Reset email sent flag
    }

    // If container is full and distance is very low, send an email
    if (container.isFull && distance <= 10 && !container.emailSent) {
      sendEmail();                  // Send email notification
      container.emailSent = true;   // Mark email as sent
    }

    if (distance > container.height) {  // If container is no longer full
      container.isFull = false;     // Reset full status
    }
  }

  delay(2000);                      // Delay 2 seconds between sensor readings
}

// Function to handle the root page of the web server
void handleRoot() {
  String page = "<html>\
  <head>\
    <title>Container Monitor</title>\
    <style>\
      body { font-family: Arial, sans-serif; background-color: #f4f4f4; padding: 20px; }\
      h1 { color: #333; text-align: center; }\
      .container { max-width: 900px; margin: 0 auto; }\
      table { width: 100%; border-collapse: collapse; margin-top: 20px; }\
      th, td { border: 1px solid #ddd; padding: 8px; text-align: center; }\
      th { background-color: #f2f2f2; }\
      tr:nth-child(even) { background-color: #f9f9f9; }\
      tr:hover { background-color: #f1f1f1; }\
      .form { margin-top: 20px; }\
      input[type='number'], input[type='submit'] { padding: 10px; margin: 5px 0; width: 100%; }\
      input[type='submit'] { background-color: #4CAF50; color: white; border: none; cursor: pointer; }\
      input[type='submit']:hover { background-color: #45a049; }\
    </style>\
  </head>\
  <body>\
    <div class='container'>\
      <h1>Container Monitoring System</h1>\
      <table>\
        <tr>\
          <th>Container</th>\
          <th>Distance (cm)</th>\
          <th>Temperature (°C)</th>\
          <th>Humidity (%)</th>\
          <th>Status</th>\
        </tr>";

  int containerIndex = 1;  // Initialize container index
  for (auto &container : containers) {
    Ultrasonic ultrasonic(container.triggerPin, container.echoPin); // Create ultrasonic sensor object
    long distance = ultrasonic.read();   // Get distance reading
    float temperature = dht.readTemperature(); // Get temperature reading
    float humidity = dht.readHumidity();       // Get humidity reading
    String status = container.isFull ? "Full" : "Not Full";  // Determine container status

    // Add container data to the table
    page += "<tr>\
      <td>" + String(containerIndex++) + "</td>\
      <td>" + String(distance) + "</td>\
      <td>" + String(temperature) + "</td>\
      <td>" + String(humidity) + "</td>\
      <td>" + status + "</td>\
    </tr>";
  }

  // Add forms for updating and adding containers
  page += "</table>\
      <div class='form'>\
        <h2>Set Container Parameters</h2>\
        <form action='/set' method='get'>\
          <label>Container Index:</label>\
          <input type='number' name='index'>\
          <label>Capacity (cm³):</label>\
          <input type='number' name='capacity'>\
          <label>Height (cm):</label>\
          <input type='number' name='height'>\
          <input type='submit' value='Set'>\
        </form>\
        <h2>Add New Container</h2>\
        <form action='/add' method='get'>\
          <label>Trigger Pin:</label>\
          <input type='number' name='trigger'>\
          <label>Echo Pin:</label>\
          <input type='number' name='echo'>\
          <label>Capacity (cm³):</label>\
          <input type='number' name='capacity'>\
          <label>Height (cm):</label>\
          <input type='number' name='height'>\
          <input type='submit' value='Add'>\
        </form>\
      </div>\
    </div>\
  </body>\
  </html>";

  server.send(200, "text/html", page);  // Send the page to the client
}

// Handles updating container parameters
void handleSetValues() {
  int index = server.arg("index").toInt(); // Get container index from the form
  if (index > 0 && index <= containers.size()) {
    index--; // Convert to 0-based index
    if (server.hasArg("capacity")) {
      containers[index].capacity = server.arg("capacity").toFloat(); // Update capacity
    }
    if (server.hasArg("height")) {
      containers[index].height = server.arg("height").toFloat(); // Update height
    }
  }
  server.send(200, "text/html", "Container updated!<br><a href=\"/\">Back</a>"); // Confirmation
}

// Handles adding a new container
void handleAddContainer() {
  Container newContainer;
  newContainer.triggerPin = server.arg("trigger").toInt();  // Get trigger pin
  newContainer.echoPin = server.arg("echo").toInt();        // Get echo pin
  newContainer.capacity = server.arg("capacity").toFloat(); // Get capacity
  newContainer.height = server.arg("height").toFloat();     // Get height
  newContainer.isFull = false;      // Initialize full status
  newContainer.emailSent = false;   // Initialize email status
  containers.push_back(newContainer); // Add new container to the list

  server.send(200, "text/html", "New container added!<br><a href=\"/\">Back</a>"); // Confirmation
}

// Function to send email notifications
void sendEmail() {
  ESP_Mail_Session session;                   // Initialize mail session
  session.server.host_name = smtpServer;      // SMTP server address
  session.server.port = smtpPort;             // SMTP server port
  session.login.email = emailFrom;            // Sender email
  session.login.password = emailPassword;     // Sender password

  SMTP_Message message;                       // Create a message object
  message.sender.name = "ESP8266 Monitor";    // Sender name
  message.sender.email = emailFrom;           // Sender email
  message.subject = "Container Alert";        // Email subject
  message.addRecipient("Recipient", emailTo); // Add recipient
  message.text.content = "One of your containers has emptied!"; // Email body

  smtp.debug(1); // Enable debugging
  smtp.callback(smtpCallback); // Set callback for SMTP status
  if (!smtp.connect(&session)) { // Connect to the SMTP server
    Serial.println("Failed to connect to SMTP server!");
    return;
  }
  if (!MailClient.sendMail(&smtp, &message)) { // Attempt to send email
    Serial.println("Failed to send email: " + smtp.errorReason());
  } else {
    Serial.println("Email sent successfully.");
  }
  smtp.closeSession(); // Close SMTP session
}

// Callback function for SMTP status updates
void smtpCallback(SMTP_Status status) {
  Serial.println(status.info()); // Print status information
}
