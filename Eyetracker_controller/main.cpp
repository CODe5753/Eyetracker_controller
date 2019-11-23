#include <iostream>
#include <interaction_lib/InteractionLib.h>
#include <interaction_lib/misc/InteractionLibPtr.h>
#include <Windows.h>
#define MAX 500
bool blink = false;
bool correction = false;
//이 두 변수는 x와 y의 좌표를 보정한 값을 넣어둠
float Xoffset=0;
float Yoffset=0;
long width = 0;
long height = 0;
float offsetData[2][MAX] = {(0,0),};
int i = 0;

void MouseMove(float x, float y);
void MouseMove(float x, float y) {
	SetCursorPos(x+Xoffset, y+Yoffset);
	Sleep(1);
}
int main()
{
	// create the interaction library
	IL::UniqueInteractionLibPtr intlib(IL::CreateInteractionLib(IL::FieldOfUse::Interactive));


	// 모니터 사이즈 기재
	SIZE s;
	ZeroMemory(&s, sizeof(SIZE));
	s.cx = (LONG)::GetSystemMetrics(SM_CXFULLSCREEN);
	s.cy = (LONG)::GetSystemMetrics(SM_CYFULLSCREEN);
	std::cout << "ScreenSize : "<<s.cx << "x" << s.cy << std::endl;
	width = s.cx;
	height = s.cy;
	constexpr float offset = 0.0f;

	intlib->CoordinateTransformAddOrUpdateDisplayArea(width, height);
	intlib->CoordinateTransformSetOriginOffset(offset, offset);

	// subscribe to gaze point data; print data to stdout when called
	intlib->SubscribeGazePointData([](IL::GazePointData evt, void* context)
		{
			if (!correction)
			{
				std::cout << "correcting now : ";
				std::cout << width / 2 - evt.x << height / 2 - evt.y << std::endl;
				offsetData[0][i]= width / 2 - evt.x;
				offsetData[1][i++] = height / 2 - evt.y;					
			}
			else {//After correction
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
			}
		}, nullptr);

	std::cout << "Starting interaction library update loop.\n";

	constexpr size_t max_cycles = MAX;
	size_t cycle = 0;
	while (cycle++ < max_cycles)
	{
		intlib->WaitAndUpdate();		
	}
	correction = true;
	
	for (i = 1; i < max_cycles; i++) {
		Xoffset += offsetData[0][i];
		Yoffset += offsetData[1][i];
	}
	Xoffset /= max_cycles;
	Yoffset /= max_cycles;

	while (true) {
		intlib->WaitAndUpdate();
	}
}
