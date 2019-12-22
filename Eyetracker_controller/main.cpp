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
//�� �� ������ x�� y�� ��ǥ�� ������ ���� �־��
int res[2] = { 0, }; //����� DB select�� ������� ������ ������.
double Xoffset=0;
double Yoffset=0;
double width = 0;
double height = 0;
float offsetData[2][MAX] = {(0,0),};	//����ó�� �� �� ������ ��ǥ�� �ϳ��� �����ص�.
bool exitflag = false; //�� �÷��װ� 1�̵Ǹ� ��� Ŀ�ؼǰ� �����带 �����ϰ� ���α׷� ������.
int i = 0;
/* Function List*/
void MouseMove(int, int);
void eyeTrackerFunction();
void checkDB();	//DB ����� status�� correction�� ��ȯ�ϴ� �Լ�

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


	// ����� ������ ����
	SIZE s;
	ZeroMemory(&s, sizeof(SIZE));
	s.cx = (LONG)::GetSystemMetrics(SM_CXFULLSCREEN);
	s.cy = (LONG)::GetSystemMetrics(SM_CYFULLSCREEN);
	std::cout << "ScreenSize : " << s.cx << "x" << s.cy << std::endl;
	width = s.cx;	//��ũ�� �ʺ�
	height = s.cy;	//��ũ�� ����
	constexpr float offset = 0.0f;

	intlib->CoordinateTransformAddOrUpdateDisplayArea(width, height);
	intlib->CoordinateTransformSetOriginOffset(offset, offset);

	// subscribe to gaze point data; print data to stdout when called
	intlib->SubscribeGazePointData([](IL::GazePointData evt, void* context)//evt > ��Ⱑ �ν��� ��ǥ��. �İ����� �ʿ���(������� ���� ��������)
		{
			//cout << "x: " << evt.x << ", y: " << evt.y << endl;
			if (!correction)
			{
				/*std::cout << "correcting now : ";
				std::cout << width / 2 - evt.x << height / 2 - evt.y << std::endl;*/
				offsetData[0][i] = evt.x;
				offsetData[1][i++] = evt.y;
			}
			else {//After correction ������ �����߸� else�� ������.
				// evt.validity == 0 �̰���, 1�϶� ����
				if (evt.validity)
				{
					blink = false;
					MouseMove(evt.x, evt.y);
				}
				else {
					if (blink == false) {
						std::cout << "BLINK DETECTED" << std::endl;
						//Ŭ�������� dbcheck �ؾ���.						
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
		intlib->WaitAndUpdate();//�ѹ� ȣ�⸶�� ��ǥ�� ������.
	}
	correction = true;

	double oxy[3][3] = { {0,0,0}, };
	for (i = 1; offsetData[0][i]!=NULL; i++) {//������ ���� ��ǥ���� ���� �ϴ� ����
		//���� ������ ������ ������ 3 section�� ����� ��	
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
	while (!exitflag) {//������ ���� ���ı� ������ �������� ���ư�.
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
			cout << mysql_errno(&conn) << " ���� " << mysql_error(&conn);
			return;
		}
		else {
			//cout << "���� ����" << endl; //���� ���� ���� ���
			if (mysql_select_db(&conn, db)) {//������ ���̽� ����
				cout << mysql_errno(&conn) << " ���� " << mysql_error(&conn);
				return;
			}

			int state = 0;
			const char* query = "select * from functbl;";
			//while (!exitflag) {
				//cout << "wwwwwwwwwwwwwwwwwwwww" << endl;
			state = mysql_query(connection, query);
			if (state == 0) {
				sql_result = mysql_store_result(connection); //Result Set�� ����
				//cout << "id\tname\tstatus" << endl;
				while ((sql_row = mysql_fetch_row(sql_result)) != NULL) {
					//cout << sql_row[0] << "\t" << sql_row[1] << "\t" << sql_row[2] << endl; //����� ���
					resdb[0] = atoi(sql_row[2]);//status��
					resdb[1] = atoi(sql_row[3]);//correction��
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
			cout << "�Լ� ȣ��!!!\t" << res[1] << endl;
			eyeTrackerFunction();
			if (exitflag) {
				break;
			}
		}
	}
}
int main()
{	//0 : status(����Ʈ��ŷ ȣ�� ����) , 1: correction(������Ű ���� ���� ����)
	//system("mode con: cols=30 lines=30");
	thread t1(checkDB);
	thread t2(resconn);
	t1.join();
	t2.join();
}
