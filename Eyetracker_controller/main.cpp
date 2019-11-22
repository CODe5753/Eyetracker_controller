
#include <iostream>
#include <interaction_lib/InteractionLib.h>
#include <interaction_lib/misc/InteractionLibPtr.h>
#include <Windows.h>

bool blink = false;
int Xoffset;
int Yoffset;

void MouseMove(float x, float y);
void MouseMove(float x, float y) {
	SetCursorPos(x, y);
	Sleep(1);
}
int main()
{
	// create the interaction library
	IL::UniqueInteractionLibPtr intlib(IL::CreateInteractionLib(IL::FieldOfUse::Interactive));


	// 모니터 사이즈 기재

	constexpr float width = 1920.0f;
	constexpr float height = 1080.0f;
	constexpr float offset = 0.0f;

	intlib->CoordinateTransformAddOrUpdateDisplayArea(width, height);
	intlib->CoordinateTransformSetOriginOffset(offset, offset);

	// subscribe to gaze point data; print data to stdout when called
	intlib->SubscribeGazePointData([](IL::GazePointData evt, void* context)
		{
			// evt.validity == 0 미감지, 1일때 감지
			if (evt.validity)
			{
				blink = false;
				MouseMove(evt.x, evt.y);
			}
			else {
				if (blink == false) {
					std::cout << "BLINK DETECTED" << std::endl;
				}
				blink = true;
			}
			/*std::cout
				<< "x: " << evt.x
				<< ", y: " << evt.y
				<< ", validity: " << (evt.validity == IL::Validity::Valid ? "valid" : "invalid")
				<< ", timestamp: " << evt.timestamp_us << " us"
				<< "\n";*/
		}, nullptr);

	std::cout << "Starting interaction library update loop.\n";

	// setup and maintain device connection, wait for device data between events and 
	// update interaction library to trigger all callbacks, stop after 200 cycles
	constexpr size_t max_cycles = 1000;
	size_t cycle = 0;
	while (cycle++ < max_cycles)
	{
		intlib->WaitAndUpdate();
	}
}
