//
// RBRUDPTelemetryLogger.cpp - Richard Burns Rally - UDP Telemetry data logger app to debug NGP/RBR UDP telemetry data.
//
// Copyright (c) 2025, MIKA-N. www.RallySimFans.hu. All rights reserved. 
// 
// License: Do whatever you want with this application and code. Free to copy, share, modify, re-publish the code in both open and closed source applications,
//          but DO NOT HOLD the author liable for any direct or indirect damages or loss caused by this application.
//

#include <iostream>

#include "UDPListener.h"
#include "rbr.telemetry.data.TelemetryData.h"   // This telemetryData header files was taken from NGP rbr plugin zip package. See NGP plugin docs for more details.


// TODO: Parameterize the listening port number based on RBR NGP-UDP telemetry setting (ie. RBR sends udp data packets to this port).
//       For now let's use a lazy hard-coded listen port and let's except NGP/RBR settings to use this same port number.
#define UDP_LISTENER_PORT 6776


// SAT calculation parameters and structs
#define SAT_LOW_PASS_ALPHA  0.25f       // Filtering factor for G forces to eliminate noise and to get a more stable SAT value. 0.0f = no smoothing, 1.0f = max smoothing (ie. no reaction to changes in G forces)
#define SAT_MAX_STEER_RAD   0.042f      // Ford Fiesta WRC 2019 max steer radians at 100% steering input to either left or right. This value comes from NGP common.lsp properties and calulcation of MaxSteeringLock * SteeringRackRatio.


typedef struct _SATResult
{
    float satValue;
    float smoothedSway;

    _SATResult() : satValue(0.0f), smoothedSway(0.0f) {}
} SATResult;
typedef SATResult* PSATResult;


/// <summary>
/// Trap CTRL+C key press and clean up WSA socket stack
/// </summary>
/// <param name="dwCtrlType"></param>
/// <returns></returns>
static BOOL WINAPI console_ctrl_handler(DWORD dwCtrlType)
{
    ::WSACleanup();
    return TRUE;
}


/// <summary>
/// Get a PHYSICS/xxxx/ subfolder name based on a car slot idx number. NGP/RBR physics subfolder name is needed in SAT calculation
/// routine to lookup for NGP car specific common.lsp physics file and to get the max steering rack angle.
/// </summary>
/// <param name="iCarSlot"></param>
/// <returns></returns>
static const char* GetSlotFolderName(unsigned int carSlotIdx)
{
    static const char* slotFolders[] = { "c_xsara", "h_accent", "mg_zr", "m_lancer", "p_206", "s_i2003", "t_coroll", "s_i2000" };
    if (carSlotIdx <= 7)
        return slotFolders[carSlotIdx];
    else
        return "";
}


/// <summary>
/// The UDP gear integer is like this as "real" gears
///     0=Reverse
///     1=Neutral
///     2=Gear1, 3=Gear2, ..6=Gear5, etc..
/// </summary>
/// <param name="gear"></param>
/// <returns></returns>
static char GetRBRGearAsChar(int gear)
{
    if (gear < 0)
	{       
		return '?'; // Invalid gear number
	}

    switch (gear)
    {
		case 0: return 'R'; // Reverse
        case 1: return 'N'; // Neutral
        default:  return '0' + (gear - 1); // 2=Gear1, 3=Gear2, etc... (for now this hopes RBR never gets cars with more than 9 gears)
    }
}


/// <summary>
/// Calculate SAT physics value from other RBR telemetry data. This is a very basic and experimental SAT calculation method, 
/// but it should be enough to get a feel of how the SAT value changes in different driving conditions and to test the concept
/// of using RBR data to calculate a SAT value on the fly.
/// 
/// 
/// </summary>
/// <param name="packet"></param>
/// <param name="prevSway"></param>
/// <returns></returns>
static SATResult CalculateSAT(const rbr::telemetry::data::TelemetryData& rbrTeleData, float prevSway)
{
	float steerPos = rbrTeleData.control_.steering_;        // Normalized steering input (-1.0f to 1.0f)
    float swayG = rbrTeleData.car_.accelerations_.sway_;    // Lateral Acceleration
    float speed = rbrTeleData.car_.speed_;                  // Speed kph

    // Use actual spring forces for highly accurate weight transfer drop-off metrics
    float forceFL = rbrTeleData.car_.suspensionLF_.springForce_;
    float forceFR = rbrTeleData.car_.suspensionRF_.springForce_;

    // Do low-pass filtering to filter out high-frequency noise and to level out high spikes
    float smoothedSway = (swayG * SAT_LOW_PASS_ALPHA) + (prevSway * (1.0f - SAT_LOW_PASS_ALPHA));

    // Convert normalized steering input to approximate radians.
    //
    // This example uses a fixed SAT_MAX_STEER_RAD defined value from Ford Fiesta WRC 2019 NGP/RBR car.
    //
    // A real solution should look up the actual NGP/RBR car physics data to calculaate the max steeering track angle in radians.
    // It would work like this:
	// - rbrTeleData.car_.index_ gives the RBR car slot number (0..7). The slot number is a lookup key to Physics/xxxx/common.lsp file
    //   in RBR game folder where the XXXX is an index specific subfolder name (see GetSlotFolderName function above).
    // 
	// - The common.lsp file has a car specific MaxSteeringLock and SteeringRackRatio parameters.
    // - The formula is for SAT_MAX_STEER_RAD is MaxSteeringLock * SteeringRackRatio
	// - For example Ford Fiesta WRC 2019 ngp physics file (the common.lsp file) has MaxSteeringLock 0.750 and SteeringRackRatio 0.056,
    //   so the SAT_MAX_STEER_RAD value would be 0.750 * 0.056 = 0.042
    //
    // NOTE!
    // - The code here should re-read and lookup up-to-date COMMON.LSP parameter values each time the current rbrteleData.totalSteps_ 
    //   value is all of a sudden smaller than the previous value. A smaller value means a RBR stage was reloaded or restarted 
    //   and therefore a car in slot 0..7 could be a different car.
    //
    // As a side note. 
    // The MaxSteeringLock parameter in common.lsp file is center-to-lock angle.
    // this means the full lock-to-lock steering rotation range in degrees for a car would be MaxSteeringLock * 360 * 2.
    // Ford Fiesta WRC 2019 would have lock-to-lock steering wheel rotation range as 0.750 * 360 * 2 = 540 degrees.
    //
    // Other useful NGP common.lsp values and calculations:
    // 1 / InverseMass = a car mass in kg
	// Pii/180 * MaxSteeringLock = max steering angle in degrees (center-to-lock full)

    float steerRad = steerPos * SAT_MAX_STEER_RAD;

    // Baseline torque countering the direction of your steer angle
    float baseTorque = smoothedSway * std::cos(steerRad);

    // Understeer / Front Axle Load drop-off emulation.
    // Under heavy braking or hard cornering, check if front axle load changes drastically
    float totalFrontForce = forceFL + forceFR;

    // Prevent divide-by-zero or weird bugs if car flies into the air and free falling (total force drops to 0)
    float loadFactor = 1.0f;
    if (totalFrontForce > 10.0f) 
    {
        // Simple normalization scaling factor based on active front load context
        loadFactor = max(0.1f, min(1.2f, totalFrontForce / 20000.0f));
    }
    else 
    {
        // Wheel went entirely light, maybe a car is now airborned?
        loadFactor = 0.1f;
    }

    // Low-speed dampening (stops steering wheels shaking violently at a complete standstill). Smooth out at <15km/h speed
    float speedFactor = 1.0f;
    if (speed < 15.0f) 
    {
        speedFactor = max(0.0f, speed / 15.0f);
    }

    // SAT calulcation
    SATResult result;
    result.satValue = baseTorque * loadFactor * speedFactor;
    result.smoothedSway = smoothedSway;
    return result;
}


/// <summary>
/// Main entry point. Let's open a UDP listener and wait for incoming UDP data packets.
/// </summary>
/// <returns></returns>
int main()
{
    int retValue = 0;
    WSADATA wsaData {};

    std::cout << "Richard Burns Rally - UDP Telemetry data logger." << std::endl;
    std::cout << std::endl;
    std::cout << "Launch RBR and enable NGP plugin and UDP telemetry feature." << std::endl;
    std::cout << "In RallySimFans RBR version see RSFLauncher.Telemetry option page" << std::endl;
    std::cout << "or enable UDP telemetry using in-game Plugins/NGP options." << std::endl;
    std::cout << std::endl;
    std::cout << std::endl;

    if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "ERROR. WSAStartup call failed. Cannot do anything." << std::endl;
        std::cout << "Press ENTER to quit this application..." << std::endl;
        std::cin.ignore();
        return 1;
    }

    ::SetConsoleCtrlHandler(console_ctrl_handler, TRUE);

    try
    {        
        CUDPListener listener(UDP_LISTENER_PORT);

        if (listener.Initialize() == false)
        {
            std::cerr << "ERROR. UDP listener failed. Cannot do anything. Make sure the UDP port number is not already reserved." << std::endl;
            std::cout << "Press ENTER to quit this application..." << std::endl;
            std::cin.ignore();
            retValue = 1;
        }
        else
        {
            DWORD dwCurrentTick = 0;
            DWORD dwLastInfoMsgTick = 0;
            DWORD dwLastUDPMsgTick = 0;

            int errorCount = 0;
            size_t bytesReceived = 0;

            rbr::telemetry::data::TelemetryData rbrTeleData{};
            SATResult currentSATResult;
			float prevSwayG = 0.0f;

            std::cout << "Waiting for UDP data packets from RBR..." << std::endl;;


            //
            // Read loop to process incoming NGP/RBR UDP telemetry data packets.
            // User can quit the loop with CTLR+C keypress.
            //
            while (true)
            {
#pragma warning(push)
#pragma warning(disable: 28159)
                // Yes, we know this tick wraps around if a PC runs more then 49 days without a reboot. We don't care about it in this app.
                dwCurrentTick = ::GetTickCount();
#pragma warning(pop)

                if ((dwCurrentTick - dwLastInfoMsgTick) > 30000)
                {
                    // Remind about how to quit this app every 30 seconds
                    dwLastInfoMsgTick = dwCurrentTick;
                    std::cout << std::endl << "Press CTRL+C key combination to quit this application." << std::endl << std::endl;
                }

                if (listener.IsValidSocket() == false)
                {
					// Invalid socket. The app is probably closing. Let's quit the read loop and let the app close itself gracefully
                    break;
                }

                bytesReceived = listener.ReceiveDataPacket((BYTE*)&rbrTeleData, sizeof(rbrTeleData));

                if (bytesReceived < sizeof(rbrTeleData))
                {
                    errorCount++;
                    if (errorCount >= 50)
                    {
                        std::cerr << "ERROR. Too many errors. Failed to receive valid UDP data packets from RBR. Check if RBR is running and NGP plugin and UDP telemetry features are enabled." << std::endl;
                        std::cout << "Press ENTER to quit this application..." << std::endl;
                        std::cin.ignore();
                        retValue = 1;
                        break;
                    }
                }
                else
                {
                    currentSATResult = CalculateSAT(rbrTeleData, prevSwayG);
                    prevSwayG = currentSATResult.smoothedSway;

                    if ((dwCurrentTick - dwLastUDPMsgTick) > 2000)
                    {
                        // Dump NGP/RBR telemetry details to a console as a string every 2 seconds (let's not overflood the console)
                        dwLastUDPMsgTick = dwCurrentTick;

                        //std::cout.precision(2);
                        std::cout << rbrTeleData.totalSteps_ << ":"
                            << " Stage=" << rbrTeleData.stage_.index_       // RBR stage ID. See rbr/maps/tracks.ini for a name of a stage. Except for all BTB stages this is always 41 (ie. not the real BTB map id).
                            << " CarSlot=" << rbrTeleData.car_.index_       // 0..7 car slot. See rbr/cars/cars.ini file for a name of a car in the slot idx
                            << " RaceTime=" << rbrTeleData.stage_.raceTime_
                            << " Speed=" << (rbrTeleData.car_.speed_ < 5.0f ? 0.0f : rbrTeleData.car_.speed_) // Speed in kph or mph depending on RBR settings. Show 0 if < 5 to avoid decimal noise of a stalled engine
                            << " Gear=" << GetRBRGearAsChar(rbrTeleData.control_.gear_)
                            << " RPM=" << (rbrTeleData.car_.engine_.rpm_ < 100.0f ? 0.0f : rbrTeleData.car_.engine_.rpm_)  // Show 0 if < 100 to avoid decimal noise of a stalled engine
                            << " Throttle=" << (rbrTeleData.control_.throttle_ * 100.0f) << "%"
                            << " Brake=" << (rbrTeleData.control_.brake_ * 100.0f) << "%"
                            << " SAT=" << currentSATResult.satValue
                            << " SwayG=" << currentSATResult.smoothedSway
                            << std::endl;
                    }
                }
            }
        }

        listener.Close();
    }
	catch (...)
	{
        retValue = 1;
	}

    ::WSACleanup();
    return retValue;
}
