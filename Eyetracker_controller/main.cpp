#include <iostream>
#include <interaction_lib/InteractionLib.h>
#include <interaction_lib/misc/InteractionLibPtr.h>
#include <Windows.h>
#include <mysql.h>
#include <thread>

#define MAX 1000

using namespace std;
bool blink = false;
bool correction = false;
//이 두 변수는 x와 y의 좌표를 보정한 값을 넣어둠
int res[2] = { 0, }; //여기는 DB select의 결과들을 저장할 공간임.
double Xoffset=0;
double Yoffset=0;
double width = 0;
double height = 0;
float offsetData[2][MAX] = {(0,0),};	//보정처리 할 때 찍히는 좌표를 하나씩 저장해둠.
bool exitflag = false; //이 플래그가 1이되면 모든 커넥션과 쓰레드를 종료하고 프로그램 종료함.
int i = 0;
/* Function List*/
void MouseMove(int, int);
void eyeTrackerFunction();
void checkDB();	//DB 결과의 status와 correction을 반환하는 함수

/*Source*/
void MouseMove(int x, int y) {	
	/*cout << "Xoffset :"<< Xoffset<<", Yoffset :" << Yoffset << endl;
	cout << "x-Xoffset :" << x+Xoffset << ", y+Yoffset :" << y+Yoffset << endl;*/
	SetCursorPos(x+Xoffset, y+Yoffset);
	Sleep(1);
}

void eyeTrackerFunction() {
	// create the interaction library
	IL::UniqueInteractionLibPtr intlib(IL::CreateInteractionLib(IL::FieldOfUse::Interactive));


	// 모니터 사이즈 기재
	SIZE s;
	ZeroMemory(&s, sizeof(SIZE));
	s.cx = (LONG)::GetSystemMetrics(SM_CXFULLSCREEN);
	s.cy = (LONG)::GetSystemMetrics(SM_CYFULLSCREEN);
	std::cout << "ScreenSize : " << s.cx << "x" << s.cy << std::endl;
	width = s.cx;	//스크린 너비
	height = s.cy;	//스크린 높이
	constexpr float offset = 0.0f;

	intlib->CoordinateTransformAddOrUpdateDisplayArea(width, height);
	intlib->CoordinateTransformSetOriginOffset(offset, offset);

	// subscribe to gaze point data; print data to stdout when called
	intlib->SubscribeGazePointData([](IL::GazePointData evt, void* context)//evt > 기기가 인식한 좌표값. 후가공이 필요함(사람마다 오차 잡아줘야함)
		{
			//cout << "x: " << evt.x << ", y: " << evt.y << endl;
			if (!correction)
			{
				/*std::cout << "correcting now : ";
				std::cout << width / 2 - evt.x << height / 2 - evt.y << std::endl;*/
				offsetData[0][i] = evt.x;
				offsetData[1][i++] = evt.y;
			}
			else {//After correction 보정이 끝나야만 else로 빠진다.
				// evt.validity == 0 미감지, 1일때 감지
				if (evt.validity)
				{
					blink = false;
					MouseMove(evt.x, evt.y);
				}
				else {
					if (blink == false) {
						std::cout << "BLINK DETECTED" << std::endl;
						//클릭했을때 dbcheck 해야함.						
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
	int cycle = 0;
	while (/*cycle++ < max_cycles*/ res[1] != 1)
	{
		intlib->WaitAndUpdate();//한번 호출마다 좌표값 측정함.
	}
	correction = true;

	double oxy[3][3] = { {0,0,0}, };
	for (i = 1; offsetData[0][i]!=NULL; i++) {//수집한 보정 좌표들을 연산 하는 구간
		//보정 구간을 세개로 나눠서 3 section의 평균을 냄	
		if (offsetData[0][i] < width *1/3) { 
			oxy[0][0] += offsetData[0][i];	
			oxy[0][1] += offsetData[1][i]; 
			oxy[0][2]++;
		}
		else if (offsetData[0][i] < width*2/3 ) {
			oxy[1][0] += offsetData[0][i];
			oxy[1][1] += offsetData[1][i];
			oxy[1][2]++;
		}
		else if (offsetData[0][i] < width*3/3) {
			oxy[2][0] += offsetData[0][i];
			oxy[2][1] += offsetData[1][i];
			oxy[2][2]++;
		}
		cout << "offsetData[0][" << i << "] : " << offsetData[0][i] << endl;
	}
	for (int j = 0; j < 3; j++) {
		for (int k = 0; k < 3; k++) {
			cout << "oxy[" << j << "][" << k << "] : " << oxy[j][k] << endl;
		}
	}

	Xoffset = ((width*1/3*1/2 - oxy[0][0] / oxy[0][2]) +(width * 2 / 3 *3/ 4 - oxy[1][0] / oxy[1][2]) + (width*3/3*5/6 - oxy[2][0] / oxy[2][2])) / 3;
	//Yoffset = (oxy[0][1] / oxy[0][2] + oxy[1][1] / oxy[1][2] + oxy[2][1] / oxy[2][2]) / 3;
	Yoffset = ((height * 1 / 3 * 1 / 2 - oxy[0][1] / oxy[0][2]) + (height * 2 / 3 * 3 / 4 - oxy[1][1] / oxy[1][2]) + (height * 3 / 3 * 5 / 6 - oxy[2][1] / oxy[2][2])) / 3;
	cout << "Xoffset : " << Xoffset << ",Yoffset : " << Yoffset << endl;
	//system("pause");
	while (!exitflag) {//보정이 끝난 이후기 때문에 무한으로 돌아감.
		if (res[0] == 0) {
			exitflag = true;
			break;
		}
		intlib->WaitAndUpdate();
	}
}
void checkDB() {
	while (!exitflag) {
		//cout << "checkDB" << endl;
		int resdb[2];
		const char* host = "localhost";
		const char* user = "root";
		const char* pw = "951222";
		const char* db = "diosk";
		MYSQL* connection = NULL;
		MYSQL conn;
		MYSQL_RES* sql_result;
		MYSQL_ROW sql_row;

		if (mysql_init(&conn) == NULL)
			cout << "mysql_init() error!" << endl;

		connection = mysql_real_connect(&conn, host, user, pw, db, 3306, (const char*)NULL, 0);
		if (connection == NULL) {
			cout << mysql_errno(&conn) << " 에러 " << mysql_error(&conn);
			return;
		}
		else {
			//cout << "연결 성공" << endl; //연결 성공 여부 출력
			if (mysql_select_db(&conn, db)) {//데이터 베이스 선택
				cout << mysql_errno(&conn) << " 에러 " << mysql_error(&conn);
				return;
			}

			int state = 0;
			const char* query = "select * from functbl;";
			//while (!exitflag) {
				//cout << "wwwwwwwwwwwwwwwwwwwww" << endl;
			state = mysql_query(connection, query);
			if (state == 0) {
				sql_result = mysql_store_result(connection); //Result Set에 저장
				//cout << "id\tname\tstatus" << endl;
				while ((sql_row = mysql_fetch_row(sql_result)) != NULL) {
					//cout << sql_row[0] << "\t" << sql_row[1] << "\t" << sql_row[2] << endl; //결과값 출력
					resdb[0] = atoi(sql_row[2]);//status값
					resdb[1] = atoi(sql_row[3]);//correction값
					res[0] = resdb[0];
					res[1] = resdb[1];
					//mysql_close(connection);
					//return;
				}
				//mysql_free_result(sql_result);
			}
			//}
			mysql_close(connection);
		}
	}
}
void resconn() {
	while (!exitflag) {
		if (res[0] == 1) {
			cout << "함수 호출!!!\t" << res[1] << endl;
			eyeTrackerFunction();
			if (exitflag) {
				break;
			}
		}
	}
}
int main()
{	//0 : status(아이트래킹 호출 여부) , 1: correction(오토핫키 보정 종료 여부)
	//system("mode con: cols=30 lines=30");
	thread t1(checkDB);
	thread t2(resconn);
	t1.join();
	t2.join();
}
