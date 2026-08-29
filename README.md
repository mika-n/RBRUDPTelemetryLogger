# RBRUDPTelemetryLogger

UDP telemetry debug logger app for _Richard Burns Rally_ game and NGP UDP telemetry messages.

You can use this application to verify RBR/NGP is sending out telemetry messages. 

# Configuration and Usage
- At the moment the application listens UDP messages in a pre-fixed 6776 udp port number. You can change this in _RBRUDPTelemetryLogger.cpp_ and _UDP_LISTENER_PORT_ option. Make sure no other apps are using this UDP port number.
- Enable _NGP plugin_ and _UDP Telemetry feature_ in Richard Burns Rally. In www.rallysimfans.hu RBR mod version you can do this by using _RSFLauncher.Telemetry_ and _RSFLauncher.Plugins_ options (set the UDP telemetry IP number as 127.0.0.1 and port number as 6776).
- Close other apps listening the 6776 udp port (for example SimHub) while running this UDP logger application.
- Set RBR temporarily to use windowed screen mode and a smaller resolution in order to see both RBR and this UDP logger application windows at the same time side by side in one monitor (set _RSFLauncher.Screen&Graphics.FullscreenMode=Windowed_ and _Resolution_ for example 1024x768).
- Launch this _RBRUDPTelemetryLogger.exe_ application and leave it running in the background.
- Launch _RBR_ and enter any stage with any car and see if the RBR udp telemetry logger application starts showing incoming udp messages.
- If this UDP logger application shows incoming messages then you know RBR/NGP game settings are correct. All is good.
- If you don't see any incoming messages then please see the Troubleshooting section below.

# Troubleshooting
- RSFLauncher.Plugins.NGP plugin and SFLauncher.Telemetry.UDPTelemetry option are enabled before launching RBR.
- RSFLauncher.Telemetry iP number and UDP port number are correct (use 127.0.0.1 ip number if both the RBR and this UDP logger application are running in the same PC. Set the port number to 6776).
- No other applications hijacking the UDP 6776 port. Close applications like SimHub.
- Run both RBR and this UDP telemetry logger applications without "Run as Administrator" option (usually RBR does not need this).
- Make sure RichardBurnsRally_SSE.exe and Plugins/NGP/bin/x64/NgpTelemetryDaemon.exe applications are allowed to access network resources (Windows Fireawall application). Yes, the latest RBR+NGP version uses an external NgpTelemetryDaemon.exe process to send out UDP telemetry messages and this NgpTelemetryDaemon.exe needs to have access to network services also, not just RichardBurnsRally_sse.exe process.
- Make sure RichardBurnsRally_sse.exe and Plugins/NGP/bin/x64/NgpTelemetryDaemon.exe are not in "blocked" state. Use FileExplorer, right mouse button click and file properties window to verify this. The first file properties tab page should not show "blocked" button. If it does then press the "Unblock" button.

# License
This is "unlicensed" application and released to public domain as is. You can use the application and source code in anyway you want, but DO NOT HOLD the author liable for any direct or indirect damages or loss caused by this application.

www.rallysimfans.hu
