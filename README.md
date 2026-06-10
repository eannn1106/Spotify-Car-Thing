# Spotify Car Thing_v1
A designated spotify display that can control playback, fetch track information including song duration, song artist, song album and displays lyrics. This spotify display is specially modified to fit in among non-coders by logging in wifi credentials throughout captive portal.
However for the one-time authentication part from Spotify API have to done it through a serial monitor by running the given code. 

****Note this device requires Spotify Premium and external sound system e.g. phone, laptop, speaker**
<img width="4000" height="3000" alt="Spotify_Car_Thing__v1_(rendered case)" src="https://github.com/user-attachments/assets/88d3d9d5-75f8-41c5-97be-8fcc931b804c" />
# Why I want to build this?
- A spotify car thing manufactured from Spotify costs over 30 USD, and for aesthetics reasons, I think that it is cool to have one sitting right in front of the car dashboard.
- The original spotify car thing doesn't support lyrics display and whenever I heard a song playing in my Spotify, I have the curiosity to seek for the lyrics. But I doesn't want the lyrics to pop out in the corner of my desktop. Instead I wanted a seperate display device for this.
- Spotify discontinued their Car Thing in 2022 which then I think there is a lot of potential with this product it can bring to the car interior industry and music industry.
# Why you should prefer this and not other products?
- Cheap and affordable. Products like this only uses microcontrollers to access the API through internet to get lyrics and fetch track information and display it through a friendly budget display screen.
- Doesn't have to be a "car thing", it can be a display on your table for decorating purposes.
- Flexibility. Instead of using bluetooth, this device requires wifi which is much more wider and higher bandwidth compared to bluetooth.
- Low power required. This device only borrows a little port from your device to operate, and doesn't requires a power plug for this device to work.
# How to setup? (one-time setup)
1. Download the sketch file code into Arduino IDE or other code editor platforms that support sketch files.
2. Go to https://developer.spotify.com/dashboard and click create apps to have access to the API.
3. For the redirect url, enter this url https://spotifyesp32.vercel.app/api/spotify/callback
4. Copy the client ID and client secret in the developer page.<img width="1919" height="916" alt="image" src="https://github.com/user-attachments/assets/6efaa6dc-b6db-48da-88d5-cf05436da376" />
**image from developer.spotify.com
5. Enter the client ID and client secret at the 16th and 17th row.<img width="1227" height="274" alt="image" src="https://github.com/user-attachments/assets/8dd495a4-9623-46a1-99e7-060dfd3cd460" />
6. Plug in the Spotify Car Thing into your laptop or desktop.
7. Compile and upload the code into the esp32 devkit_v1.
8. Open the serial monitor.
9. Connect to the wifi called "SpotifyCarThing" with the password "spotify123".
10. It will bring you to a captive web portal for you to enter your wifi credentials. <img width="1177" height="2560" alt="image" src="https://github.com/user-attachments/assets/e3a1f621-24a0-4bdf-9f69-11e4c120392b" />
11. Enter your wifi credentials in the text box, it can be a home wifi or your personal hotspot from your device. (the wifi have to be in 2.4 GHz Band) **(IOS user have to turn on maximise compatibility)**
12. From the serial monitor, copy the authentication url provided and copy it into your browser to login and authenticate your spotify account. 
13. Play something on your Spotify app!
# How to use it (in the long run)
1. Powering it up
   - Connect this to a 5v power supply through the micro usb slot in the left hand side of the device. A USB port from your laptop/desktop or a standard 5v USB wall charger will do.
2. Play/Pause toggle switch
   - Locate the middle from top to bottom switch to play or pause the current track.
3. Skip track switch
   - Locate the most bottom from top to bottom switch to skip forward the current playing track.
4. Previous track switch
   - Locate the toppest from top to bottom switch to skip backwards the current playing track.
5. Reset WiFi hidden switch
   - Long press on all 3 switch for 3 seconds the reset the WiFi.
   - Repeat the process to enter wifi credentials
# How it works
1. Authentication mode
<img width="831" height="555" alt="image" src="https://github.com/user-attachments/assets/bcbecccc-453e-4ddf-9c17-f7550e56f275" />
 
 **image from developer.spotify.com

- Refresh token is the permanent credential while access token expires every hour but Spotify ESP32 library handles renewal automatically.
- Spotify ESP32 library will generate an authentication url with random state tokens that cant be duplicated and print it in the serial monitor.
- After authentication, spotify redirects to vercel.app callback which also handled by spotify ESP32 library.
- Callback exchanges authentication code for refresh token and access token.
- ESP32 receives these tokens by using this sp.handle_client() function and refresh token will be saved by using SPIFFS library in the flash memory.
2. Controlling playback flow
- By setting the scopes from https://developer.spotify.com/documentation/web-api/concepts/scopes, it allows us to make a few requests to spotify api and obtain information, for this specific project I've set the scopes to user-read-playback-state, user-modify-playback-state and user-read-currently-playing.
- Refresh token is loaded and access token is constantly be renewed by Spotify ESP32 library on first API call.
- API will be called for every 1 second to obtain the real position of the specific track position.
- Obtained data from the web api will be in Json document form and will be parsed into each variable.
3. Obtaining lyrics from lrclib API
- ESP32 will make HTTP requests to obtain the lyrics in Json document form containing synced lyrics and unsynced lyrics.
4. Synced lyrics
- Lrclib API is a free open source lyrics API, and obtained synced lyrics comes with the format of milisecond timestamp where convertions will be handled locally by ESP 32.
- ESP32 will create it's own timestamp to match with the synced lyrics' timestamp whenever the current track started playing.
5. Wifi login
- ESP32 will first check if there is any saved networks in the flash memory, if no it will execute captive portal.
- ESP32 will create a Wifi hotspot "SpotifyCarThing" by switching mode to Access point mode.
- DNSServer intercepts all DNS queries and redirect to the IP address of your ESP32.
- Browser automatically opens the setup page for user to type in their wifi credentials.
- After user types their wifi credentials, it will be saved into the flash memory of ESP32 using the Prefences library.
- ESP32 switches to station mode, and the captive portal will be closed.
- Right now, ESP32 will already have the saved wifi credentials in their flash memory, ESP32 will restart and check then connects to the wifi in the flash memory.
6. TFT display management
- Rewriting the entire screen every second will cause constant visible flickering. Instead, each displayed element in NowPlayingInfo is being tracked by the variables locally.
- If the displayed elements doesnt match the current element, it will be updated.
- Text sizing is dynamic as size 2 is used by defauly and will automatically drop the size to 1 if the text is wider than the screen width which is 480px.

# Parts needed
[Bill of Materials](BOM.csv)

# How to assemble?
1. Prepare all the parts stated in the BOM
2. Solder the keyboard switches onto the pcb according to the silkscreen layer.
3. We'll start with attatching the ESP32 into the PCB via the connector sockets.
4. Secure the PCB into the 4 tower screws with 4x 4mm m3 self-tapped screws.
5. Insert and attach the 3.5'' rpi display throgh the hole of the case and into the pcb's connector header pins.
6. Attach the keycaps to the keyboard switch for easier control of buttons.

# Libraries installed
1. Spotify Esp32 - https://github.com/FinianLandes/SpotifyEsp32.git
2. Arduino Json - https://github.com/bblanchon/ArduinoJson.git
3. TFT_eSPI - https://github.com/Bodmer/TFT_eSPI.git

# Schematics
<img width="1525" height="877" alt="image" src="https://github.com/user-attachments/assets/eff37c1b-94e0-4b02-8c2c-3a58f313a164" />

# PCB
<img width="1574" height="831" alt="image" src="https://github.com/user-attachments/assets/2179bdd5-56b1-4249-9ae3-ca6933ca5e32" />
3D view of the PCB
<img width="1082" height="678" alt="image" src="https://github.com/user-attachments/assets/3ebd187c-baae-4ad9-88ef-633375043da2" />
<img width="1032" height="645" alt="image" src="https://github.com/user-attachments/assets/3b8edd2f-04bf-4020-be49-252218ae8e59" />

To make the device more compact, I decided to add the ESP32 below the PCB.
# Case
The case is being held by 4 cantilever snaps at the four corners of the case.
<img width="1125" height="679" alt="image" src="https://github.com/user-attachments/assets/05019cee-6130-42db-b542-c6c89a17285b" />
<img width="1068" height="580" alt="image" src="https://github.com/user-attachments/assets/382c4896-2f38-4874-b40b-9287b0c62510" />

The PCB is being screwed into these 4 screw towers using 4x M3 screws
<img width="1241" height="739" alt="image" src="https://github.com/user-attachments/assets/b2ac7ffb-c3dd-46a8-8230-39c27836538c" />

I got the inspiration from a retro tv specifically this type.
<img width="740" height="740" alt="image" src="https://github.com/user-attachments/assets/70463ebc-d613-453f-b489-271918c81cb5" /> 

The case looks boxy and have slight rounded edges on the side but, for the bottom of the case, I won't make it curved as it can't be tumbling down from bumpy roads. The controls are on the right hand side and the screen is on the left hand side of the case.

# Zine
<img width="1398" height="2000" alt="Zine" src="https://github.com/user-attachments/assets/27b23204-77aa-46b0-b7cf-076a5725356b" />
