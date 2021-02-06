/*****************
  Tally light ESP32 for Blackmagic ATEM switcher

  Version 2.0

  A wireless (WiFi) tally light for Blackmagic Design
  ATEM video switchers, based on the M5StickC ESP32 development
  board and the Arduino IDE.

  Edited by Chris Connor 02 02 2021

  For more information, see:
  https://oneguyoneblog.com/2020/06/13/tally-light-esp32-for-blackmagic-atem-switcher/

  Based on the work of Kasper Skårhøj:
  https://github.com/kasperskaarhoj/SKAARHOJ-Open-Engineering

******************/

#include <M5StickCPlus.h>
#include <WiFi.h>
#include <SkaarhojPgmspace.h>
#include <ATEMbase.h>
#include <ATEMstd.h>

//IPAddress clientIp(192, 168, 1, 92);          // IP address of the ESP32 - I dont think this does anything...it was getting a DHCP address
IPAddress switcherIp(192, 168, 1, 91);       // IP address of the ATEM switcher
ATEMstd AtemSwitcher;

// http://www.barth-dev.de/online/rgb565-color-picker/
#define GRAY      0x0020 //   8  8  8
#define GREEN     0x0200 //   0 64  0
#define RED       0xF800 // 255  0  0
#define WHITE     0xFFFF
#define MAROON    0xC000 //
#define HC_YELLOW 0xFF28

// WiFi Setup Details
//const char* ssid = "BT-WQAHQR";
//const char* password =  "GEk6VcVFVt3vMA";
const char* ssid = "FlaxmillWifi";
const char* password =  "NQDQRBMVMQ";

// LED PIN FOR PHYSICAL LED
int ledPin = 10;

int PreviewTallyPrevious = 1;
int ProgramTallyPrevious = 1;
int CamPrevious = 1;
int currentCam = 1;

// MAX CAMS AN ATEM MINI PRO
const int maxCameras = 4;

void setup() {

  Serial.println("Bootting up tally light..");

  Serial.begin(115200);

  // Start the Ethernet, Serial (debugging) and UDP:
  WiFi.begin(ssid, password);

  Serial.println("Bootting up tally light..");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println("Connected to the WiFi network");


  // initialize the M5StickC object
  M5.begin();

  LoadingScreen();

  pinMode(ledPin, OUTPUT);  // LED: 1 is on Program (Tally)
  digitalWrite(ledPin, HIGH); // off

  // Initialize a connection to the switcher:
  AtemSwitcher.begin(switcherIp);
  AtemSwitcher.serialOutput(0x80);
  AtemSwitcher.connect();
}

void loop() {

  M5.update();


  // Check for packets, respond to them etc. Keeping the connection alive!
  AtemSwitcher.runLoop();

  int ProgramTally = AtemSwitcher.getProgramTally(currentCam);
  int PreviewTally = AtemSwitcher.getPreviewTally(currentCam);
  //  int ProgramTally = 3;
  //  int PreviewTally = 4;



  // IF THE STATE OF THE ATEM HAS CHANGE, OR THE CAMERA NUMBER HAS CHANGED - UPDATE THE SCREEN
  if ((ProgramTallyPrevious != ProgramTally) || (PreviewTallyPrevious != PreviewTally) || (currentCam != CamPrevious)) {

    // TEMP BLOCK FOR DEBUG
    Serial.println("===The state changed===");
    Serial.println("Current cam is " + String(currentCam) + " Previous cam was " + String(CamPrevious));
    Serial.println("---");
    Serial.println("Previous Programme:" + String(ProgramTally));
    Serial.println("Program:" + String(ProgramTallyPrevious));
    Serial.println("---");
    Serial.println("Previous Preview:" + String(PreviewTally));
    Serial.println("Preview:" + String(PreviewTallyPrevious));
    Serial.println("---");

    if ((ProgramTally && !PreviewTally) || (ProgramTally && PreviewTally) ) { // only program, or program AND preview
      drawLabel(RED, BLACK, LOW, currentCam, "LIVE");
    } else if (PreviewTally && !ProgramTally) { // only preview
      drawLabel(GREEN, BLACK, HIGH, currentCam, "PREVIEW");
    } else if (!PreviewTally || !ProgramTally) { // neither
      drawLabel(BLACK, WHITE, HIGH, currentCam, "INACTIVE");
    }

    //TEMP REMOVE BELOW - ONLY FOR DEBUG
    //    if (currentCam == 2) {
    //      drawLabel(GREEN, BLACK, HIGH, currentCam, "PREVIEW");
    //    } else if (currentCam == 3) {
    //      drawLabel(RED, BLACK, LOW, currentCam, "LIVE");
    //    } else {
    //      drawLabel(WHITE, GRAY, HIGH, currentCam, "INACTIVE");
    //    }

    // DO WE NOW CHANGE CAM STATE HERE?
    CamPrevious = currentCam;

  }

  // CHECK IF BUTTON WAS PRESSED - CHANGE CAMERA
  if (M5.BtnA.wasPressed()) {

    // LOG THE STATE
    logState(ProgramTally, PreviewTally);
    Serial.println("Current cam is" + String(currentCam) + " Previous cam was " + String(CamPrevious));

    // INCREMENT OR RETURN TO ONE
    if (currentCam != maxCameras) {
      CamPrevious = currentCam;
      currentCam++;
    } else if (currentCam == maxCameras) {
      CamPrevious = 4;
      currentCam = 1;
    }

    Serial.println("Current cam is now" + String(currentCam) + " Previous cam was " + String(CamPrevious));

  }


  // UPDATE THE ATEM VALUES
  ProgramTallyPrevious = ProgramTally;
  PreviewTallyPrevious = PreviewTally;

}

void logState(int ProgramTally, int PreviewTally) {

  Serial.println("ProgramTally from Atem: " + String(ProgramTally));
  Serial.println("PreviewTally from Atem: " + String(PreviewTally));


}

// PURELY FOR TESTING
double getRandomBin()
{
  return (double)rand() / (double)RAND_MAX ;
}

void LoadingScreen() {

  digitalWrite(ledPin, LOW);
  M5.Lcd.fillScreen(MAROON);
  M5.Lcd.setTextColor(HC_YELLOW, MAROON);

  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(16, 25);
  M5.Lcd.printf("STREAM\n TEAM\n\n\n LOADING...");
  delay(5000);
  digitalWrite(ledPin, HIGH);
}

void drawLabel(unsigned long int screenColor, unsigned long int labelColor, bool ledValue, int camNumber, char statusString[]) {

  // TOGGLE THE LED ON AND OFF
  digitalWrite(ledPin, ledValue);

  M5.Lcd.fillScreen(screenColor);
  M5.Lcd.setTextColor(labelColor, screenColor);

  // CAMERA TXT
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(16, 25);
  M5.Lcd.printf("CAMERA");

  // CAMERA NUMBER
  M5.Lcd.setTextSize(8);
  M5.Lcd.setCursor(50, 110);
  M5.Lcd.printf("%d", (camNumber));

  // ADD A WEE RECORDING CIRCLE
  if (screenColor == RED) {
    M5.Lcd.fillCircle(115, 115, 10, 0xA800);
  }

  // STATUS STRING AT THE BOTTOM
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(16, 200);
  M5.Lcd.printf(statusString);

  // TEMP BELOW FOR DEBUG
  //  if (camNumber == 2) {
  //    M5.Lcd.printf("PREVIEW");
  //
  //  } else if (camNumber == 3) {
  //    M5.Lcd.printf("LIVE");
  //
  //  } else {
  //    M5.Lcd.printf("INACTIVE");
  //
  //  }

}
