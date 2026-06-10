//libraries needed
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SpotifyEsp32.h>
#include <SPIFFS.h>
#include <WebServer.h>
#include <Preferences.h>
#include <DNSServer.h>
#include <TFT_eSPI.h>

//credentials for spotify API
const char* SSID = "";
const char* PASSWORD = "";
const char* CLIENT_ID = "6ba7dd6f2f054dce97e27e81f39d124b";   //Enter your client ID 
const char* CLIENT_SECRET = "efb5a06d8f7748338bc8cc77807647b0";  //Enter your client secret

//button pins
#define BTN_PLAY_PAUSE 25
#define BTN_NEXT 27
#define BTN_PREV 12

//declare variables
String lastTrack = "";
String lastArtist = "";
String lastLyrics = "";
bool isPlaying = false;
unsigned long lastSpotifyCheck = 0;
unsigned long lastPositionMs = 0;
unsigned long lastPositionTime = 0;
unsigned long lastDurationMs = 0;
bool trackIsPlaying = false;
bool authComplete = false;
String spotifyAuthUrl = "";
bool wifiSaved = false;
bool wifiConnected = false;
String newWifiSSID = "";
String newWifiPass = "";
String savedRefreshToken = "";


//spotify controller
Spotify* sp_ptr = nullptr;
#define sp (*sp_ptr)

//Setup web server and DNS for captive portal
WebServer server(80);
DNSServer dnsServer;
Preferences prefs;
TFT_eSPI tft = TFT_eSPI();
//setup mode flag
bool isSetupMode = false;

//SET UP SCREEN DIMENSIONS
#define SCREEN_W 480
#define SCREEN_H 320

//SET COLORS
#define SPOTIFY_GREEN 0x25ca
#define DARK_GREY 0x2945
#define LIGHT_GREY 0xb596

//LAYOUT POSITIONS
#define HEADER_H 40
#define TRACK_Y 48
#define ARTIST_Y 90
#define ALBUM_Y 118
#define PROGRESS_Y 152
#define PROGRESS_H 16
#define PROGRESS_X 20
#define PROGRESS_W 440
#define LYRIC_Y 192
#define NEXT_LYRIC_Y 240

//DISPLAY FUNCTION
void initDisplay(){
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  Serial.println("[TFT] ✅ Display initialized");
}

//Draw text centered horizontally
void drawCenteredText(String text, int y, uint16_t color, uint8_t size){
  tft.setTextSize(size);
  tft.setTextColor(color, TFT_BLACK);
  int textW = tft.textWidth(text);
  int x = (SCREEN_W - textW) / 2;
  if (x < 5) x = 5;
  tft.setCursor(x, y);
  tft.print(text);
}

//Clear line area
void clearLine(int y, int h){
  tft.fillRect(0, y, SCREEN_W, h, TFT_BLACK);
}

String truncateText(String text, uint8_t size, int maxWidth){
  tft.setTextSize(size);
  if(tft.textWidth(text) <= maxWidth){
    return text;
  }
  while(text.length() > 1 && tft.textWidth(text + "...") > maxWidth){
    text = text.substring(0, text.length() - 1);
  }
  return text + "...";
}

//Screen for booting
void showBootScreen(){
  tft.fillScreen(TFT_BLACK);

  tft.fillCircle(SCREEN_W/2, 100, 50, SPOTIFY_GREEN);
  tft.setTextColor(TFT_BLACK, SPOTIFY_GREEN);
  tft.setCursor(SCREEN_W/2 - 20, 85);
  tft.print("♪");

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(3);
  drawCenteredText("Spotify Car Thing", 175, TFT_WHITE, 3);

  tft.setTextSize(1);
  drawCenteredText("Starting up...", 215, LIGHT_GREY, 1);
}

//wifi setup screen
void showWifiSetupScreen(){
  tft.fillScreen(TFT_BLACK);
  
  tft.fillRect(0, 0, SCREEN_W, HEADER_H, SPOTIFY_GREEN);
  tft.setTextColor(TFT_BLACK, SPOTIFY_GREEN);
  tft.setTextSize(2);
  drawCenteredText("WiFi Setup", 12, TFT_BLACK, 2);

  tft.setTextSize(3);
  tft.setTextColor(SPOTIFY_GREEN, TFT_BLACK);
  drawCenteredText("(())", 60, SPOTIFY_GREEN, 3);

  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  drawCenteredText("Connect to WiFi:", 110, TFT_WHITE, 2);

  tft.setTextSize(2);
  tft.setTextColor(SPOTIFY_GREEN, TFT_BLACK);
  drawCenteredText("SpotifyCarThing", 140, SPOTIFY_GREEN, 2);

  tft.setTextSize(1);
  tft.setTextColor(LIGHT_GREY, TFT_BLACK);
  drawCenteredText("Password: spotify123", 170, LIGHT_GREY, 1);

  tft.drawFastHLine(20, 190, SCREEN_W - 40, DARK_GREY);

  tft.setTextSize(1);
  tft.setTextColor(LIGHT_GREY, TFT_BLACK);
  drawCenteredText("Then open browser and enter", 200, LIGHT_GREY, 1);
  drawCenteredText("your home WiFi or personal hotspot credentials", 215, LIGHT_GREY, 1);
}

void showWifiConnectingScreen(String ssid){
  tft.fillScreen(TFT_BLACK);
  
  tft.fillRect(0, 0, SCREEN_W, HEADER_H, DARK_GREY);
  tft.setTextColor(TFT_WHITE, DARK_GREY);
  tft.setTextSize(2);
  drawCenteredText("Connecting to:", 100, LIGHT_GREY, 2);

  tft.setTextSize(2);
  tft.setTextColor(LIGHT_GREY, TFT_BLACK);
  drawCenteredText("Connecting to:", 100, LIGHT_GREY, 2);

  tft.setTextSize(2);
  tft.setTextColor(SPOTIFY_GREEN, TFT_BLACK);
  drawCenteredText(ssid, 130, SPOTIFY_GREEN, 2);

  tft.setTextSize(1);
  tft.setTextColor(LIGHT_GREY, TFT_BLACK);
  drawCenteredText("Please wait...", 175, LIGHT_GREY, 1);
}

void showWifiConnectedScreen(String ip){
  tft.fillScreen(TFT_BLACK);
  
  tft.fillRect(0, 0, SCREEN_W, HEADER_H, SPOTIFY_GREEN);
  tft.setTextColor(TFT_BLACK, SPOTIFY_GREEN);
  tft.setTextSize(2);
  drawCenteredText("WiFi Connected!", 12, TFT_BLACK, 2);

  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  drawCenteredText("IP Address:", 90, TFT_WHITE, 2);

  tft.setTextSize(2);
  tft.setTextColor(SPOTIFY_GREEN, TFT_BLACK);
  drawCenteredText(ip, 120, SPOTIFY_GREEN, 2);
}

void showWifiFailedScreen(){
  tft.fillScreen(TFT_BLACK);

  tft.fillRect(0, 0, SCREEN_W, HEADER_H, 0xf800);
  tft.setTextColor(TFT_WHITE, 0xf800);
  tft.setTextSize(2);
  drawCenteredText("WiFi Failed!", 12, TFT_WHITE, 2);

  tft.setTextSize(1);
  tft.setTextColor(LIGHT_GREY, TFT_BLACK);
  drawCenteredText("Check your WiFi name", 140, LIGHT_GREY, 1);
  drawCenteredText("and password then try again", 155, LIGHT_GREY, 1);
}

void showSpotifyAuthScreen(){
  tft.fillScreen(TFT_BLACK);
  
  tft.fillRect(0, 0, SCREEN_W, HEADER_H, SPOTIFY_GREEN);
  tft.setTextColor(TFT_BLACK, SPOTIFY_GREEN);
  tft.setTextSize(2);
  drawCenteredText("Spotify Login", 12, TFT_BLACK, 2);

  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  drawCenteredText("Open Serial monitor in Arduino IDE", 70, TFT_WHITE, 2);
  drawCenteredText("and copy the URL", 100, TFT_WHITE, 2);
  drawCenteredText("to your browser", 130, TFT_WHITE, 2);

  tft.drawFastHLine(20, 160, SCREEN_W - 40, DARK_GREY);

  tft.setTextSize(1);
  tft.setTextColor(LIGHT_GREY, TFT_BLACK);
  drawCenteredText("Login to Spotify once and works forever", 175, LIGHT_GREY, 1);
  drawCenteredText("Waiting for login...", 215, SPOTIFY_GREEN, 1);
}
void showResetCountdown(int seconds){
  if(seconds == 3){
    tft.fillScreen(TFT_BLACK);
    tft.fillRect(0, 0, SCREEN_W, HEADER_H, 0xf800);
    tft.setTextColor(TFT_WHITE, 0xf800);
    tft.setTextSize(2);
    drawCenteredText("Factory Reset!", 12, TFT_WHITE, 2);

    tft.setTextSize(1);
    tft.setTextColor(LIGHT_GREY, TFT_BLACK);
    drawCenteredText("Release buttons to cancel", 60, LIGHT_GREY, 1);
  }

  //update the countdown no.
  clearLine(100, 80);
  tft.setTextSize(6);
  tft.setTextColor(0xf800, TFT_BLACK);
  drawCenteredText(String(seconds), 110, 0xf800, 6);

  tft.setTextSize(1);
  tft.setTextColor(LIGHT_GREY, TFT_BLACK);
  drawCenteredText("seconds...", 175, LIGHT_GREY, 1);
}

void showResetCancelled(){
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  drawCenteredText("Reset Cancelled", 140, TFT_WHITE, 2);
  delay(1000);
}

String displayedTrack = "";
String displayedArtist = "";
String displayedAlbum = "";
String displayedLyric = "";
String displayedNextLyric = "";

void drawNowPlayingLayout(){
  tft.fillScreen(TFT_BLACK);

  tft.fillRect(0, 0, SCREEN_W, HEADER_H, DARK_GREY);
  tft.setTextColor(SPOTIFY_GREEN, DARK_GREY);
  tft.setTextSize(2);
  tft.setCursor(10, 12);
  tft.print("NOW PLAYING");

  tft.drawRoundRect(PROGRESS_X, PROGRESS_Y, PROGRESS_W, PROGRESS_H, 4, 0X4A69);

  tft.drawFastHLine(20, LYRIC_Y - 12, SCREEN_W - 40, DARK_GREY);
}

void updateTrackInfo(String track, String artist, String album){
  if(track == displayedTrack && artist == displayedArtist && album == displayedAlbum){
    return;
  }
  displayedTrack = track;
  displayedArtist = artist;
  displayedAlbum = album;

  clearLine(TRACK_Y, 95);

  //track name
  tft.setTextSize(2);
  uint8_t trackSize = (tft.textWidth(track) > SCREEN_W - 20) ? 1 : 2;
  drawCenteredText(track, TRACK_Y, TFT_WHITE, trackSize);

  //artist name
  tft.setTextSize(2);
  uint8_t artistSize = (tft.textWidth(artist) > SCREEN_W - 20) ? 1 : 2;
  drawCenteredText(artist, ARTIST_Y, SPOTIFY_GREEN, artistSize);

  //album name
  drawCenteredText(album, ALBUM_Y, LIGHT_GREY, 1);
}

void updateProgressBar(unsigned long positionMs, unsigned long durationMs){
  if(durationMs == 0){
    return;
  }
  float progress = (float)positionMs / (float)durationMs;
  if(progress > 1.0){
    progress = 1.0;
  }

  int filled = (int)(progress * (PROGRESS_W - 4));

  if(filled > 0){
    tft.fillRoundRect(PROGRESS_X + 2, PROGRESS_Y + 2, filled, PROGRESS_H - 4, 3, SPOTIFY_GREEN);
  }

  if(filled < PROGRESS_W - 4){
    tft.fillRect(PROGRESS_X + 2 + filled, PROGRESS_Y + 2, PROGRESS_W - 4 - filled, PROGRESS_H - 4, TFT_BLACK);
  }

  int posMin = (positionMs / 1000) / 60;
  int posSec = (positionMs / 1000) % 60;
  int durMin = (durationMs / 1000) / 60;
  int durSec = (durationMs / 1000) % 60;

  char timeStr[20];
  sprintf(timeStr, "%d:%02d / %d:%02d", posMin, posSec, durMin, durSec);

  tft.fillRect(PROGRESS_X, PROGRESS_Y + PROGRESS_H + 3, PROGRESS_W, 14, TFT_BLACK);

  tft.setTextSize(1);
  drawCenteredText(String(timeStr), PROGRESS_Y + PROGRESS_H + 5, LIGHT_GREY, 1);
}

void updateLyrics(String currentLine, String nextLine){
  if(currentLine == displayedLyric && nextLine == displayedNextLyric){
    return;
  }
  displayedLyric = currentLine;
  displayedNextLyric = nextLine;

  clearLine(LYRIC_Y - 5, SCREEN_H - LYRIC_Y + 5);

  if(!currentLine.isEmpty()){
    tft.setTextSize(2);
    uint8_t lyricSize = (tft.textWidth(currentLine) > SCREEN_W - 20) ? 1 : 2;
    drawCenteredText(currentLine, LYRIC_Y, TFT_WHITE, lyricSize);
  }
  
  if(!nextLine.isEmpty()){
    tft.setTextSize(2);
    uint8_t nextSize = (tft.textWidth(nextLine) > SCREEN_W - 20) ? 1 : 2;
    drawCenteredText(nextLine, NEXT_LYRIC_Y, LIGHT_GREY, nextSize);
  }
}

void updatePlayPauseIcon(bool playing){
  static const unsigned char PROGMEM image_music_pause_bits[] = {0xf9,0xf0,0x89,0x10,0x89,0x10,0x89,0x10,0x89,0x10,0x89,0x10,0x89,0x10,0x89,0x10,0x89,0x10,0x89,0x10,0x89,0x10,0x89,0x10,0x89,0x10,0x89,0x10,0xf9,0xf0,0x00,0x00};
  static const unsigned char PROGMEM image_music_play_bits[] = {0xc0,0x00,0xe0,0x00,0x98,0x00,0x86,0x00,0x81,0x80,0x80,0x60,0x80,0x18,0x80,0x06,0x80,0x18,0x80,0x60,0x81,0x80,0x86,0x00,0x98,0x00,0xe0,0x00,0xc0,0x00,0x00,0x00};
  tft.fillRect(SCREEN_W - 35, 8, 25, 25, DARK_GREY);
  tft.setTextSize(2);
  if(playing){
    tft.setTextColor(SPOTIFY_GREEN, DARK_GREY);
    tft.drawBitmap(SCREEN_W - 32, 12, image_music_play_bits, 15, 16, 0xffff);
  }
  else{
    tft.setTextColor(LIGHT_GREY, DARK_GREY);
    tft.drawBitmap(SCREEN_W - 32, 12, image_music_pause_bits, 12, 16, 0xffff);
  }
}

void showNothingPlaying(){
  if(displayedTrack == "NOTHING_PLAYING"){
    return;
  }
  displayedTrack = "NOTHING_PLAYING";

  tft.fillScreen(TFT_BLACK);
  tft.fillRect(0, 0, SCREEN_W, HEADER_H, DARK_GREY);
  tft.setTextColor(LIGHT_GREY, DARK_GREY);
  tft.setTextSize(2);
  drawCenteredText("Spotify Car Thing", 12, LIGHT_GREY, 2);

  tft.setTextSize(3);
  tft.setTextColor(DARK_GREY, TFT_BLACK);
  drawCenteredText("♪", 120, DARK_GREY, 3);

  tft.setTextSize(2);
  tft.setTextColor(LIGHT_GREY, TFT_BLACK);
  drawCenteredText("Nothing is playing", 180, LIGHT_GREY, 2);

  tft.setTextSize(1);
  tft.setTextColor(DARK_GREY, TFT_BLACK);
  drawCenteredText("Open Spotify and play something!", 220, DARK_GREY, 1);
}

void clearWifiCredentials(){
  prefs.begin("wifi", false);
  prefs.clear();
  prefs.end();
  Serial.println("[PREFS] ✅ WiFi credentials cleared!");
}

//clear saved token
void clearSavedToken(){
  if(SPIFFS.exists("/spotify_refresh.txt")){
    SPIFFS.remove("/spotify_refresh.txt");
    Serial.println("[CLEAR] ✅ Token cleared!");
  }
}

//factory reset - clear both wifi and spotify token
void factoryReset(){
  //Clear WiFi (Prefences)
  clearWifiCredentials();
  //Clear Spotify token (SPIFFS)
  Serial.println("[RESET] ✅ Factory reset complete!");
  tft.setTextSize(2);
  tft.fillScreen(TFT_BLACK);
  drawCenteredText("Resetting...", 140, 0xf800, 2);
}

void checkResetButton(){
  if (digitalRead(BTN_PLAY_PAUSE) == LOW && digitalRead(BTN_NEXT) == LOW && digitalRead(BTN_PREV) == LOW){
    Serial.println("[RESET] Hold all buttons for 3 seconds to restart 🔄️");
    for(int i = 3; i > 0; i--){
      Serial.printf("Factory resetting in %d seconds...\n", i);
      showResetCountdown(i);
      delay(1000);

      if(digitalRead(BTN_PLAY_PAUSE) == HIGH || digitalRead(BTN_NEXT) == HIGH || digitalRead(BTN_PREV) == HIGH){
        Serial.println("[RESET] ❌ CANCELLED RESET");
        showResetCancelled();
        displayedTrack = "";
        displayedAlbum = "";
        displayedAlbum = "";
        displayedLyric = "";
        drawNowPlayingLayout();
        return;
      }
    }
    factoryReset();
    delay(200);
    ESP.restart();
  }
}

const char* WIFI_HTML = R"=====(
  <!DOCTYPE html>
  <html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta charset="UTF-8">
    <title>Spotify Car Thing Setup</title>
    <style>
      * {box-sizing: border-box; margin: 0; padding: 0;}
      body{
        font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
        background: #191414;
        color: white;
        min-height: 100vh;
        display: flex;
        align-items: center;
        justify-content: center;
        padding:20px;
      }
      .card{
        background: #282828;
        border-radius: 16px;
        padding: 32px 24px;
        width: 100%;
        max-width: 400px;
      }
      .logo{
        text-align: center;
        font-size: 48px;
        margin-bottom: 8px;
      }
      h1{
        text-align: center;
        color: #1DB954;
        font-size: 22px;
        margin-bottom: 4px;
      }
      .subtitle{
        text-align: center;
        color: #b3b3b3;
        font-size: 14px;
        margin-bottom: 28px;
      }
      .section-title{
        color: #1DB954;
        font-size: 13px;
        font-weight: 600;
        text-transform: uppercase;
        letter-spacing: 1px;
        margin-bottom: 12px;
      }
      input{
        width: 100%;
        padding: 14px 16px;
        margin-bottom: 12px;
        border: 2px solid #3E3E3E;
        border-radius: 8px;
        background: #3E3E3E;
        color: white;
        font-size: 16px;
        outline: none;
        transition: border-color 0.2s;
      }
      input:focus {border-color: #1D8954;}
      input::placeholder {color: #727272;}
      button{
        width: 100%;
        padding: 16px;
        background: #1DB954;
        color: white;
        border: none;
        font-size: 16px;
        font-weight: 600;
        cursor: pointer;
        margin-top: 8px;
        transition: background 0.2s;
      }
      button:hover {background: #1ed760;}
      button:active {background: #169c46;}
      .status{
        text-align: center;
        margin-top: 16px;
        padding: 12px;
        border-radius: 50px;
        font-size: 16px;
        font-weight: 600;
        cursor: pointer;
        margin-top: 8px;
        transition: background 0.2s;
      }
      button:hover {background: #1ed760;}
      button:active {background: #169c46;}
      .status{
        text-align: center;
        margin-top: 16px;
        padding: 12px;
        border-radius: 8px;
        font-size: 14px;
        display: none;
      }
      .status.success {background: #1a3a1a; color: #1DB954; display: block;}
      .status.error {background: #3a1a1a; color: #ff4444; display: block;}
      .status.loading {background: #2a2a2a; color: #b3b3b3; display: block;}
      .divider{
        border: none;
        border-top: 1px solid #3E3E3E;
        margin: 24px 0;
      }
      .step{
        display: flex;
        allign-items: center;
        gap: 12px;
        margin-bottom: 12px;
        font-size: 14px;
        color: #b3b3b3;
      }
      .step_num{
        background: #1DB954;
        color: white;
        width: 24px;
        height: 24px;
        border-radius: 50%;
        display: flex;
        align-items: center;
        justify-content: center;
        font-size: 12px;
        font-weight: bold;
        flex-shrink: 0;
      }
    </style>
  </head>
  <body>
    <div class = "card">
      <div class = "logo">🎵</div>
      <h1>Spotify Car Thing</h1>
      <p class = "subtitle">First time setup</p>

      <div class = "step">
        <div class = "step-num">1</div>
        <span>Enter your WiFi details below</span>
      </div>
      <div class = "step">
        <div class = "step-num">2</div>
        <span>Device will connect and show Spotify login</span>
      </div>
      <div class = "step">
        <div class = "step-num">3</div>
        <span>Login once, works forever after!</span>
      </div>

      <hr class = "divider">

      <p class = "section-title">WiFi Details</p>
      <input type = "text" id = "ssid" placeholder = "WiFi Name" autocomplete = "off" autocorrect = "off" autocapitalize = "off" />
      <input type = "password" id = "pass" placeholder = "WiFi Password" />
      <button onclick = "saveWifi()">Connect & Continue -></button>

      <div id = "status" class = "status"></div>
    </div>

    <script>
      function showStatus(msg, type){
        var s = document.getElementById('status');
        s.className = 'status' + type;
        s.innerHTML = msg;
      }

      function saveWifi(){
        var ssid = document.getElementById('ssid').value.trim();
        var pass = document.getElementById('pass').value;

        if(ssid == ''){
          showStatus('❌ Please enter your WiFi name!', 'error');
          return;
        }

        showStatus('⌛ Saving and connecting... please wait', 'loading');

        fetch('/save?ssid=' + encodeURIComponent(ssid) + '&pass=' + encodeURIComponent(pass))
          .then(r => r.text())
          .then(data => {
            showStatus('✅ success' + data, 'success');
          })
          .catch(e => {
            showStatus('✅ Saved! Device is restarting...', 'success');
          });
      }
    </script>
  </body>
  </html>
)=====";

//Storage functions
void saveWifiCredentials(String ssid, String password){
  prefs.begin("wifi", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", password);
  prefs.end();
  Serial.println("[PREFS] ✅ WiFi saved: " + ssid);
  Serial.println("[PREFS] ✅ WiFi password: " + password);
}

bool loadWifiCredentials(String &ssid, String &password){
  prefs.begin("wifi", true);
  ssid = prefs.getString("ssid", "");
  password = prefs.getString("pass", "");
  prefs.end();

  if (ssid.length() > 0){
    Serial.println("[PREFS] ✅ WiFi loaded: " + ssid);
    return true;
  }
  Serial.println("[PREFS] ❌ No WiFi saved");
  return false;
}

//*** TOKEN MANAGEMENT FUNCTIONS ***

//save refresh token to SPIFFS
bool saveRefreshToken(String refresh_token){
  File file = SPIFFS.open("/spotify_refresh.txt", "w");
  if(!file){
    Serial.println("[SAVE]❌ Failed to save token");
    return false;
  }

  file.print(refresh_token);
  file.close();

  Serial.println("[SAVE] ✅ Refresh token saved!");
  return true;
}

//load refresh token from SPIFFS
String loadRefreshToken() {
  if(!SPIFFS.exists("/spotify_refresh.txt")){
    Serial.println("[LOAD]❓No saved token found");
    return"";
  }

  File file = SPIFFS.open("/spotify_refresh.txt", "r");
  if(!file){
    Serial.println("[LOAD] ❌ Failed to open token file");
    return "";
  }

  String token = file.readStringUntil('\n');
  token.trim();
  file.close();

  if(token.length() > 10){
    Serial.println("[LOAD] ✅ Refresh token loaded!");
    Serial.println("[LOAD] ❗Refresh token is: " + token);
    return token;
  }

  return "";
}
void handleRoot(){
  server.send(200, "text/html", WIFI_HTML);
}

void handleSave(){
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");

  if (ssid.length() == 0){
    server.send(400, "text/plain", "WiFi name cannot be empty! ");
    return;
  }

  newWifiSSID = ssid;
  newWifiPass = pass;
  saveWifiCredentials(newWifiSSID, newWifiPass);
  wifiSaved = true;
}
void handleNotFound(){
  server.sendHeader("Location", "http://192.168.4.1", true);
  server.send(302, "text/plain", "");
}

void startSetupHotspot(){
  isSetupMode = true;
  Serial.println("[SETUP] FIRST TIME SETUP MODE");

  WiFi.mode(WIFI_AP);
  delay(100);
  WiFi.softAP("SpotifyCarThing", "spotify123");
  IPAddress apIP = WiFi.softAPIP();
  Serial.printf("[SETUP] Hotspot IP: %s\n", apIP.toString().c_str());
  Serial.println("[SETUP] SSID: SpotifyCarThing");
  Serial.println("[SETUP] Password: spotify123");

  dnsServer.start(53, "*", apIP);

  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.onNotFound(handleNotFound);
  server.begin();

  Serial.println("[SETUP] Instructions: ");
  Serial.println("[SETUP] 1. Connect to SpotifyCarThing");
  Serial.println("[SETUP] 2. Password: spotify123");
  Serial.println("[SETUP] 3. Browser opens setup page");
  Serial.println("[SETUP] 4. Enter your home WiFi details");
}

bool connectToSavedWifi(){
  String ssid, password;
  
  if(!loadWifiCredentials(ssid, password)){
    return false;
  }

  Serial.println("[WiFi] Connecting to: " + ssid);
  Serial.println("[WiFi] Password: " + password);

  WiFi.disconnect(true);
  delay(500);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(ssid.c_str(), password.c_str());

  int attempts = 0;
  while(WiFi.status() != WL_CONNECTED){
    for (int j = 0; j < 10; j++){
      server.handleClient();
      dnsServer.processNextRequest();
      delay(50);
    }
    Serial.print(".");
    attempts++;

    if(attempts > 40){
      Serial.println();
      Serial.println("[WiFi] ❌ Connection failed!");
      Serial.printf("[WiFi] Status: %d\n", WiFi.status());
      return false;
      ESP.restart();
    }
  }

  Serial.println();
  Serial.printf("[WiFi] ✅ Connected! IP: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("[WiFi] Signal: %d dBm\n", WiFi.RSSI());
  wifiConnected = true;
  return true;
}

//lrclib timestamp
int parsets (String line){
  int openBracket = line.indexOf('[');
  int closeBracket = line.indexOf(']');

  if (openBracket == -1 || closeBracket == -1){
    return -1;   //skip if empty line
  }

  String ts = line.substring(openBracket + 1, closeBracket);   //only extracts the number inside the bracket

  int colonPos = ts.indexOf(':');
  int dotPos = ts.indexOf('.');

  if (colonPos == -1 || dotPos == -1){
    return -1;  //skip if empty line
  }

  int min = ts.substring(0, colonPos).toInt();
  int sec = ts.substring(colonPos + 1, dotPos).toInt();
  int hun = ts.substring(dotPos + 1).toInt();
  int totalMs = (min * 60 * 1000) + (sec * 1000) + (hun * 10); //convert everything to ms
  return totalMs;
}

//extract lyrics
String extractLyric(String line){
  int closeBracket = line.indexOf(']');
  if (closeBracket == -1){
    return line;
  }

  String text = line.substring(closeBracket + 1);
  text.trim();
  return text;
}

//URL encoder - encodes the url endpoint so that it is able to abstract json response 
String urlEncode(String str){
  String encoded = "";
  for (int i = 0; i < str.length(); i++){ 
    char c = str[i];
    if (isAlphaNumeric(c) || c == '-' || c == '_' || c == '.' || c == '~' || c == '(' || c == ')'){
      encoded += c;
    }
    else if (c == ' '){
      encoded += '+';
    }
    else if (c == '\''){
      encoded += "%27";
    }
    else{
      encoded += c;
    }
  }
  return encoded;
} 
//fetch lyrics from lrclib
String fetchLyrics (String track, String artist, String album, int durationSec){
  Serial.println("\n[LYRICS] Searching for lyrics...");
  Serial.println("[LYRICS] Track: " + track);
  Serial.println("[LYRICS] Artist: " + artist);
  Serial.println("[LYRICS] Album: " + album);
  Serial.printf("[LYRICS] Duration: %d sec\n", durationSec);

  if (WiFi.status() != WL_CONNECTED){
    Serial.println("[LYRICS] ERROR: WiFi not connected!");
    return "";
  }
  HTTPClient http;
  String url = "https://lrclib.net/api/get";
  url += "?track_name=" + urlEncode(track);
  url += "&artist_name=" + urlEncode(artist);
  url += "&album_name=" + urlEncode(album);
  url += "&duration=" + String(durationSec);
  Serial.println("[LYRICS] Fetching: " + url);
  http.begin(url);
  http.setTimeout(20000); //if lrclib does'nt respond in 10s, then give up
  int httpCode = http.GET(); //200 = success , 404 = not found , 500 = server error, 400 = wrong publish token
  Serial.println("[LYRICS] HTTP code: " + String(httpCode));

  if (httpCode != 200){
    http.end();
    Serial.println("[LYRICS] Request failed!");
    return "";
  }

  String payload = http.getString();
  http.end();
  Serial.println("[LYRICS] Got response, parsing...");
  // parsing the json
  DynamicJsonDocument doc(16384);
  DeserializationError error = deserializeJson(doc, payload);
  if (error){
    Serial.println("[LYRICS] Json parse error: " + String(error.c_str()));
    return "";
  }
  if (doc.size() == 0){
    Serial.println("[LYRICS]No results found");
    return "";
  }
  String syncedLyrics = doc["syncedLyrics"];
  if (!syncedLyrics.isEmpty()){
    Serial.println ("[LYRICS] Found synced lyrics!✓");
    return syncedLyrics;
  }
  
  //do plain lyrics if no synced version
  String plainLyrics = doc["plainLyrics"];
  if (!plainLyrics.isEmpty()){
    Serial.println ("[LYRICS] Found plain lyrics!✓");
    return plainLyrics;
  }

  Serial.println("[LYRICS] No lyrics available for this song");
  return ""; //return nothing
}
//display synced lyrics
void displaySyncedLyrics(String lyrics, int positionMs){
  if (lyrics.isEmpty()){
    return;
  }

  String currentLine = "";
  String nextLine = "";
  int lineStart = 0;
  boolean foundNext = false;

  for (int i = 0; i <= (int)lyrics.length(); i++){
    if (lyrics[i] == '\n' || i == (int)lyrics.length()){
      //when i counter reaches newline or same with the length of the lyrics
      String line = lyrics.substring(lineStart, i);
      lineStart = i + 1;  //move on to the next line

      if (line.isEmpty()) continue;
      //skip empty lines, prob melody

      int lineTime = parsets(line);
      
      if (lineTime == -1) continue;
      //skip lines with no timestamp

      if(lineTime <= positionMs){
        currentLine = extractLyric(line);
        foundNext = false;
      }
      else if (!foundNext){
        nextLine = extractLyric(line);
        foundNext = true;
        break;
      }
    }
  }

  if (!currentLine.isEmpty()){
    Serial.println("♪ " + currentLine); 
  }

  if (!nextLine.isEmpty()){
    Serial.println(" " + nextLine);
  }
  updateLyrics(currentLine, nextLine);
}

//buttons designation
void buttons(){
  if (digitalRead(BTN_PLAY_PAUSE) == LOW){
    if(sp.is_playing()){
      //currently playing -> paused
      response res = sp.pause_playback();
      if(res.status_code == 204 || res.status_code == 200){
        Serial.println("[BTN] ⏸️ Paused");
      }
    }
    else {
      //currently paused -> playing
      response res;
      res = sp.start_a_users_playback();
      if(res.status_code == 204 || res.status_code == 200){
        Serial.println("[BTN] ▶️ Playing");
      }
    }
    delay(10);
  }

  if (digitalRead(BTN_NEXT) == LOW){
    response res = sp.skip_to_next();
    if(res.status_code == 204 || res.status_code == 200){
      Serial.println("[BTN] ⏭️ Next Track");
      lastTrack = "";
      lastArtist = "";
      lastLyrics = "";
    }
    delay(10);
  }

  if (digitalRead(BTN_PREV) == LOW){
    response res = sp.skip_to_previous();
    if(res.status_code == 204 || res.status_code == 200){
      Serial.println("[BTN] ⏮️ Previous Track");
      lastTrack = "";
      lastArtist = "";
      lastLyrics = "";
    }
    delay(10);
  }
}


//setup
void setup(){
  Serial.begin(115200);
  delay(2000);

  initDisplay();
  showBootScreen();
  delay(1000);

  Serial.println("\n\n");
  Serial.println("     _______..______     ______   .___________. __   ___________    ____     _______   __       _______..______    __          ___   ____    ____ ");
  Serial.println("    /       ||   _  \   /  __  \  |           ||  | |   ____\   \  /   /    |       \ |  |     /       ||   _  \  |  |        /   \  \   \  /   / ");
  Serial.println("   |   (----`|  |_)  | |  |  |  | `---|  |----`|  | |  |__   \   \/   /     |  .--.  ||  |    |   (----`|  |_)  | |  |       /  ^  \  \   \/   /  ");
  Serial.println("    \   \    |   ___/  |  |  |  |     |  |     |  | |   __|   \_    _/      |  |  |  ||  |     \   \    |   ___/  |  |      /  /_\  \  \_    _/   ");
  Serial.println(".----)   |   |  |      |  `--'  |     |  |     |  | |  |        |  |        |  '--'  ||  | .----)   |   |  |      |  `----./  _____  \   |  |     ");
  Serial.println("|_______/    | _|       \______/      |__|     |__| |__|        |__|        |_______/ |__| |_______/    | _|      |_______/__/     \__\  |__|     ");
  Serial.println("                                                                                                                                                  ");
  Serial.println("");

  pinMode(BTN_PLAY_PAUSE, INPUT_PULLUP);
  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_PREV, INPUT_PULLUP);
  Serial.println("[SETUP] ✓ Buttons initialized");
  Serial.printf("       Play/Pause: GPIO %d\n", BTN_PLAY_PAUSE);
  Serial.printf("       Next:       GPIO %d\n", BTN_NEXT);
  Serial.printf("       Previous:   GPIO %d\n", BTN_PREV);

  //Initializa SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("[SETUP] ❌ SPIFFS initialization failed!");
  }
  else {
    Serial.println("[SETUP] ✓ SPIFFS initialized");
  }
  //wifi setup
  Serial.println("");
  Serial.println("Checking saved WiFi...");

  if(!connectToSavedWifi()){
    Serial.println("[SETUP] Starting setup hotspot...");
    startSetupHotspot();
    showWifiSetupScreen();

    while(true){
      checkResetButton();
      dnsServer.processNextRequest();
      server.handleClient();

      //got saved wifi but not connected to wifi
      if(wifiSaved && !wifiConnected){
        Serial.println("[SETUP] Attempting WiFi connection...");
        Serial.println("[SETUP] SSID: " + newWifiSSID);
        showWifiConnectingScreen(newWifiSSID);

        WiFi.mode(WIFI_AP_STA);
        delay(200);
        //restart softAP after changing the mode
        WiFi.softAP("SpotifyCarThing", "spotify123");
        delay(200);
        WiFi.setSleep(false);
        WiFi.begin(newWifiSSID.c_str(), newWifiPass.c_str());

        int attempts = 0;
        while(WiFi.status() != WL_CONNECTED && attempts < 40){
          for (int i = 0; i < 10; i++){
            server.handleClient();
            dnsServer.processNextRequest();
            delay(50);
          }
          Serial.print(".");
          attempts ++;
        }

        Serial.println("");

        String ip = WiFi.localIP().toString();
        //using this when wifi connected using from captive wifi page
        if(WiFi.status() == WL_CONNECTED && ip != "0.0.0.0"){
          wifiConnected = true;
          Serial.printf("[WiFi] ✅ Connected! IP: %s\n", ip.c_str());
          showWifiConnectedScreen(ip);
          WiFi.softAP("SpotifyCarThing", "spotify123");
          delay(100);

           // loading saved refresh token
           savedRefreshToken = loadRefreshToken();

          if (savedRefreshToken.length() < 10){
            sp_ptr = new Spotify(CLIENT_ID, CLIENT_SECRET);
            sp.set_scopes("user-read-playback-state user-modify-playback-state user-read-currently-playing");
            sp.begin();

            delay(100);
            spotifyAuthUrl = sp.get_auth_url();
            Serial.println("[AUTH] Auth URL ready: ");
            Serial.println(spotifyAuthUrl);
            showSpotifyAuthScreen();
          
            while(!sp_ptr->is_auth()){
              checkResetButton();
              server.handleClient();
              dnsServer.processNextRequest();
              sp_ptr->handle_client();
              delay(10);
            }
            
            Serial.println("[AUTH] ✅ Authentication successful!");
            
            user_tokens tokens = sp_ptr->get_user_tokens();
            String newToken = String(tokens.refresh_token);
            
            if(newToken.length() > 0){
              saveRefreshToken(newToken);
              savedRefreshToken = newToken;
              Serial.println("[AUTH] ✅ Token saved!");
            }
              
          server.stop();
          Serial.println("[AUTH] Restarting...");
          delay(1000);
          ESP.restart();
          }
          else{
            ESP.restart();
          }
        }
        else{
          Serial.println("[WIFI] ❌ Failed to connect!");
          showWifiFailedScreen();
          delay(1000);
          wifiSaved = false;
          showWifiSetupScreen();
          clearWifiCredentials();
        }
      }
      delay(10);
    }
  }

  //loading refresh token from flash storge
  savedRefreshToken = loadRefreshToken();

  if (savedRefreshToken.length() > 0){
    //found saved token
    Serial.println("[SETUP] ✅ Found saved refresh token!");
    Serial.println("[SETUP] ➡️ Skipping login - using saved credentials");

    //recreate spotify object with saved refresh token
    //ive changed this
    sp_ptr = new Spotify(CLIENT_ID, CLIENT_SECRET, savedRefreshToken.c_str());
    sp.set_scopes("user-read-playback-state user-modify-playback-state user-read-currently-playing");
    sp.begin();
    sp.handle_client();
    delay(1000);
    Serial.printf("[SETUP] Spotify account authenticated: %s\n", sp.is_auth() ? "Yes" : "No");
    delay(2000);
  }

  //not running this
  else{
    Serial.println("[AUTH] No Spotify token found");
    Serial.println("[AUTH] Starting authentication...");

    sp_ptr = new Spotify(CLIENT_ID, CLIENT_SECRET);
    sp.set_scopes("user-read-playback-state user-modify-playback-state user-read-currently-playing");
    sp.begin();
          
    spotifyAuthUrl = sp.get_auth_url();
    Serial.println("[AUTH] Auth URL ready: " + spotifyAuthUrl);
    showSpotifyAuthScreen();
          
    while(!sp.is_auth()){
      checkResetButton();
      sp.handle_client();
    }
    Serial.println("[AUTH] ✅ Authentication successful!");
            
    user_tokens tokens = sp.get_user_tokens();
    String newToken = String(tokens.refresh_token);
            
    if(newToken.length() > 0){
      saveRefreshToken(newToken);
      savedRefreshToken = newToken;
      Serial.println("[AUTH] ✅ Token saved!");
    }
  }
  
  if (savedRefreshToken.length() > 10){
    Serial.println("");
    Serial.println("[SETUP] ✓ All systems ready!");
    Serial.println("[SETUP] Now play something on Spotify...");
    Serial.println("─────────────────────────────────");
    drawNowPlayingLayout();
    checkResetButton();
  }
}

//loop
void loop(){
  //Check for RESET BUTTON (HOLD ALL BUTTONS FOR 3 sec)
  checkResetButton();
  buttons();
  
  if(savedRefreshToken.length() > 10){
    //check spotify every 1 second
    unsigned long now = millis();
    unsigned long livePosition = 0;
    if (trackIsPlaying && lastPositionTime > 0){
      livePosition = lastPositionMs + (now - lastPositionTime);
    }
    static unsigned long lastLyricDisplay = 0;
    if (millis() - lastLyricDisplay >= 1000){
      lastLyricDisplay = millis();

      if(trackIsPlaying && !lastLyrics.isEmpty() && livePosition > 0){
        displaySyncedLyrics(lastLyrics, livePosition);
        updateProgressBar(livePosition, lastDurationMs);
      }
    }

    if(now - lastSpotifyCheck >= 1000){
      lastSpotifyCheck = now;
      checkResetButton();
      //Get full playback state to extract progress
      JsonDocument filter;
      filter["progress_ms"] = true;  
      filter["item"]["name"] = true;
      filter["item"]["album"]["name"] = true;
      filter["item"]["artists"][0]["name"] = true;
      filter["item"]["duration_ms"] = true;
      filter["is_playing"] = true;

      response playbackState = sp.get_currently_playing_track(filter);

      if(playbackState.status_code == 200){
        int currentPosition = playbackState.reply["progress_ms"];        //extract current position duration of track
        int trackLengthMs = playbackState.reply["item"]["duration_ms"]; //extract length duration of track
        String currentTrack = playbackState.reply["item"]["name"];
        String currentAlbum = playbackState.reply["item"]["album"]["name"];
        String currentArtist = playbackState.reply["item"]["artists"][0]["name"];
        bool currentlyPlaying = playbackState.reply["is_playing"];

        //update playing state
        isPlaying = currentlyPlaying;
        //update local position tracking
        lastDurationMs = trackLengthMs;
        lastPositionMs = currentPosition;
        lastPositionTime = millis();
        trackIsPlaying = currentlyPlaying;
        updateProgressBar(currentPosition, trackLengthMs);
        //calculate position in seconds and minutes
        int posMin = (currentPosition / 1000) / 60;
        int posSec = (currentPosition / 1000) % 60;
        int trackMin = (trackLengthMs / 1000) / 60;
        int trackSec = (trackLengthMs / 1000) % 60;
      
        if (!currentTrack.isEmpty()){
          updateTrackInfo(currentTrack, currentArtist, currentAlbum);
          updatePlayPauseIcon(currentlyPlaying);
          Serial.println("───────────────────────────────────────────────────");
          Serial.println("🎵 NOW PLAYING");
          Serial.println(" 📻  " + currentTrack);
          Serial.println(" 🧑‍🎤 Artist: " + currentArtist);
          Serial.println(" 📖 Album: " + currentAlbum);
          Serial.printf(" ⏱️ Position: %d:%02d / %d:%02d \n", posMin, posSec, trackMin, trackSec);
          Serial.printf(" ⏯️ Play/Pause: GPIO %d\n", BTN_PLAY_PAUSE);
          Serial.printf(" ⏭️ Next:       GPIO %d\n", BTN_NEXT);
          Serial.printf(" ⏮️ Previous:   GPIO %d\n", BTN_PREV);
          Serial.println("───────────────────────────────────────────────────");

          //change another track
          if (currentTrack != lastTrack && !currentTrack.isEmpty()){
            lastTrack = currentTrack;
            lastArtist = currentArtist;
            //fetch lyrics with track name, artist, album and duration
            int durationSec = trackLengthMs / 1000;
            lastLyrics = fetchLyrics(currentTrack, currentArtist, currentAlbum, durationSec);
            if(lastLyrics.isEmpty()){
              Serial.println("[LYRICS] No lyrics found for this song");
            }
            else{
              Serial.println("[LYRICS] ✅ Lyrics loaded!");
            }
          }
        }  
      } 
      else if(playbackState.status_code == 204){
        trackIsPlaying = false;
        isPlaying = false;
        Serial.println("[SPOTIFY] Nothing is playing");
        showNothingPlaying();
      }
      else{
        //request failed
        Serial.printf("[SPOTIFY] ❌ Failed to get playback state: %d\n", playbackState.status_code);
        Serial.printf("[WiFi] Signal: %d dBm\n", WiFi.RSSI());
      }
    }
  }
}
