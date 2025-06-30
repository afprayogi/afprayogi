#include <Wire.h>
#include <ESP32Servo.h>

// === PIN SETUP ===
#define IR1_PIN        35
#define IR2_PIN        27
#define MOISTURE_PIN   32
#define LOGAM1_PIN     33
#define LOGAM2_PIN     25

#define S2             22
#define S3             21
#define OUT            23

#define TX2_PIN        17
#define RX2_PIN        16

#define SERVO1_PIN     13
#define SERVO2_PIN     19

Servo servo1;
Servo servo2;

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RX2_PIN, TX2_PIN);

  pinMode(LOGAM1_PIN, INPUT);
  pinMode(LOGAM2_PIN, INPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(OUT, INPUT);

  // Servo init
  servo1.setPeriodHertz(50);
  servo1.attach(SERVO1_PIN, 500, 2400);
  servo2.setPeriodHertz(50);
  servo2.attach(SERVO2_PIN, 500, 2400);
  servo1.write(0);
  servo2.write(90);
}

int readColorFrequency(bool s2State, bool s3State) {
  digitalWrite(S2, s2State);
  digitalWrite(S3, s3State);
  delay(100);
  return pulseIn(OUT, LOW);
}

void loop() {
  int nilaiIR1 = analogRead(IR1_PIN);
  int nilaiIR2 = analogRead(IR2_PIN);
  int nilaiMoisture = analogRead(MOISTURE_PIN);

  bool logam1Terdeteksi = (digitalRead(LOGAM1_PIN) == LOW);
  bool logam2Terdeteksi = (digitalRead(LOGAM2_PIN) == HIGH);
  bool logamTerdeteksi = logam1Terdeteksi || logam2Terdeteksi;

  int redFrequency   = readColorFrequency(LOW, LOW);
  int blueFrequency  = readColorFrequency(LOW, HIGH);
  int greenFrequency = readColorFrequency(HIGH, HIGH);

  String jenis = klasifikasiSampah(logamTerdeteksi, nilaiMoisture, nilaiIR1, nilaiIR2,
                                    redFrequency, greenFrequency, blueFrequency);

  // === Tampilkan data ke Serial Monitor ===
  Serial.println("=== Data Sensor ===");
  Serial.print("IR1: "); Serial.print(nilaiIR1);
  Serial.print(" | IR2: "); Serial.print(nilaiIR2);
  Serial.print(" | Moisture: "); Serial.print(nilaiMoisture);
  Serial.print(" | R: "); Serial.print(redFrequency);
  Serial.print(" | G: "); Serial.print(greenFrequency);
  Serial.print(" | B: "); Serial.print(blueFrequency);
  Serial.print(" | Logam: "); Serial.print(logamTerdeteksi ? "YA" : "TIDAK");
  Serial.print(" | Klasifikasi: "); Serial.println(jenis);
  Serial.println();

  Serial2.println(jenis); // Kirim antar node

 kendaliServo(jenis); // ✅ Aktifkan servo berdasarkan klasifikasi

  delay(500); // Delay agar gerakan selesai sebelum loop ulang
}
String klasifikasiSampah(bool isLogam, int moisture, int ir1, int ir2,
                         int red, int green, int blue) {
  int nilaiIR = ir2;

  if (isLogam) return "Logam";

  // Deteksi jika warna sangat gelap (kemungkinan plastik hitam)
  // Tidak ada sampah: IR sangat tinggi DAN bukan warna gelap (alias background)
  if (nilaiIR > 3000) {
    return "Tidak Ada Sampah";
  }

  // Plastik: IR tinggi atau hitam dengan IR sangat tinggi
  if (nilaiIR >= 800 ) {
    return "Plastik";
  }

  // Organik Basah
  if (moisture < 1500) return "Organik Basah";

  // Organik Kering
  return "Organik Kering";
}

void kendaliServo(String jenis) {
  if (jenis == "Logam") {
    Serial.println("🔁 Servo ke tempat LOGAM");
    servo1.write(0);
    delay(1000);
    servo2.write(55);
  } 
  else if (jenis == "Plastik") {
    Serial.println("🔁 Servo ke tempat PLASTIK");
    servo1.write(0);
    delay(1000);
    servo2.write(125);
  } 
  else if (jenis == "Organik Kering") {
    Serial.println("🔁 Servo ke tempat ORGANIK KERING");
    servo1.write(90);
    delay(1000);
    servo2.write(55);
  } 
  else if (jenis == "Organik Basah") {
    Serial.println("🔁 Servo ke tempat ORGANIK BASAH");
    servo1.write(90);
    delay(1000);
    servo2.write(125);
  } 
  else if ( jenis == "Tidak Ada Sampah") {
    Serial.println("🟡 Tidak Dikenali ➜ Servo posisi NETRAL");
    servo1.write(0);
    delay(1000);
    servo2.write(90);
  }
  else {
    
  }
}



