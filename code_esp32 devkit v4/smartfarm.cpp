#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <DHT.h>

// WiFi & Firebase Config
#define WIFI_SSID "POCO F3"
#define WIFI_PASSWORD "bayardulumas"

#define API_KEY "AIzaSyCgU9rslKJw-XTekSOm9iR0LTLSp2UBapQ"
#define DATABASE_URL "https://smartfarm-eae67-default-rtdb.asia-southeast1.firebasedatabase.app"

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Pins
const int sensorPin = 35;  // Analog kelembaban tanah
const int relayPin = 26;   // Relay pompa

#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

String mode = "otomatis";  // Default mode
String kontrolManual = "OFF";

void nyalakanPompa(bool nyala) {
  digitalWrite(relayPin, nyala ? LOW : HIGH);
}

void setup() {
  Serial.begin(115200);

  pinMode(sensorPin, INPUT);
  pinMode(relayPin, OUTPUT);
  nyalakanPompa(false);

  dht.begin();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Menghubungkan WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nTerhubung ke WiFi!");

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  auth.user.email = "fikronajiah123@gmail.com";
  auth.user.password = "farfromtoxic";

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  int nilaiSensor = analogRead(sensorPin);
  int kelembaban = map(nilaiSensor, 4095, 0, 0, 100);

  float suhu = dht.readTemperature();

  Serial.printf("ADC: %d, Kelembaban: %d%%, Suhu: %.2f°C\n", nilaiSensor, kelembaban, suhu);

  // Kirim sensor data
  Firebase.RTDB.setInt(&fbdo, "/kelembaban", kelembaban);
  if (!isnan(suhu)) {
    Firebase.RTDB.setFloat(&fbdo, "/suhu", suhu);
  }

  // Baca mode dari Firebase
  if (Firebase.RTDB.getString(&fbdo, "/mode")) {
    mode = fbdo.stringData();
  } else {
    Serial.println("❌ Gagal baca mode: " + fbdo.errorReason());
  }

  if (mode == "manual") {
    // Jika manual, baca kontrol ON/OFF
    if (Firebase.RTDB.getString(&fbdo, "/kontrol")) {
      kontrolManual = fbdo.stringData();
    } else {
      Serial.println("❌ Gagal baca kontrol: " + fbdo.errorReason());
    }

    if (kontrolManual == "ON") {
      nyalakanPompa(true);
      Firebase.RTDB.setString(&fbdo, "/status_penyiraman", "Aktif (Manual)");
      Serial.println("Pompa NYALA (Manual)");
    } else {
      nyalakanPompa(false);
      Firebase.RTDB.setString(&fbdo, "/status_penyiraman", "Nonaktif");
      Serial.println("Pompa MATI (Manual)");
    }
  } else {
    // Mode otomatis, berdasarkan kelembaban
    bool otomatisOn = (kelembaban < 30);
    if (otomatisOn) {
      nyalakanPompa(true);
      Firebase.RTDB.setString(&fbdo, "/status_penyiraman", "Aktif (Otomatis)");
      Serial.println("Pompa NYALA (Otomatis)");
    } else {
      nyalakanPompa(false);
      Firebase.RTDB.setString(&fbdo, "/status_penyiraman", "Nonaktif");
      Serial.println("Pompa MATI (Otomatis)");
    }

    // Reset kontrol manual agar tidak konflik
    Firebase.RTDB.setString(&fbdo, "/kontrol", "OFF");
  }

  delay(5000);
}