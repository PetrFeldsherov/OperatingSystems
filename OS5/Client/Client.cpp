#include <windows.h>
#include <iostream>
#include <string>
#include "..\Employee\Employee.h"
using std::cout;
using std::cin;
using std::cerr;
using std::endl;
using std::string;

const string PIPE_NAME = "\\\\.\\pipe\\pipe_name";
const string START_ALL_EV_NAME = "start_all_ev";
const int MAX_MESSAGE_SIZE = 10;
const int CONNECT_ATTEMPT_PERIOD = 7000;

char get_correct_command()
{
	char command;
	cout << "input: <\'r\' / \'w\'>" << endl;
	cin >> command;
	while (!('r' == command || 'w' == command))
	{
		cout << "the command provided doesn't correspond to any of the options" << endl << "correct input format: <\'r\' / \'w\'>" << endl;
		cin >> command;
	}
	return command;
}

int main(int argc, char** argv) {
	HANDLE hNamedPipe;
	DWORD nBytesWritten;
	DWORD nBytesRead;
	employee employee;
	char command;
	unsigned int id_to_proceed;
	string message_buffer;

	HANDLE hReadyEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, argv[1]);
	HANDLE hStartAllEvent = OpenEvent(SYNCHRONIZE, FALSE, START_ALL_EV_NAME.c_str());

	if (!hReadyEvent || !hStartAllEvent)
	{
		cout << argv[0] << " " << argv[1] << endl;
		cerr << "Cannot open event." << endl;
		system("pause");
		return GetLastError();
	}
	SetEvent(hReadyEvent);
	WaitForSingleObject(hStartAllEvent, INFINITE);

	while (true)
	{
		hNamedPipe = CreateFile(PIPE_NAME.c_str(), GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if (hNamedPipe != INVALID_HANDLE_VALUE)
		{
			break;
		}
		if (!WaitNamedPipeA(PIPE_NAME.c_str(), CONNECT_ATTEMPT_PERIOD))
		{
			cerr << "Time of waiting for connection is out (" << CONNECT_ATTEMPT_PERIOD / 1000 << "sec)." << endl;
			system("pause");
			return 0;
		}
	}

	while (true)
	{
		command = get_correct_command();
		cout << "input: <argument - id>" << endl;
		cin >> id_to_proceed;
		message_buffer = string(1, command) + string(1, ' ') + std::to_string(id_to_proceed);

		if (!WriteFile(hNamedPipe, message_buffer.c_str(), MAX_MESSAGE_SIZE, &nBytesWritten, NULL))
		{
			cerr << "Cannot send message." << endl;
			system("pause");
			return GetLastError();
		}
		if (!ReadFile(hNamedPipe, &employee, sizeof(employee), &nBytesRead, NULL))
		{
			cerr << "Cannot receive answer from server." << endl;
		}
		else
		{
			if (employee.num < 0)
			{
				cerr << "The employee is either not found or has not been modified." << endl;
				continue;
			}
			else
			{
				cout << "employee: " << employee.num << " " << employee.name << employee.hours;
				if ('w' == command)
				{
					cout << "input: employee's <id> <name> <working hours>" << endl;
					cin >> employee.num >> employee.name >> employee.hours;
					std::cin.ignore(2, '\n'); // or getchar() ?
					if (!WriteFile(hNamedPipe, &employee, sizeof(employee), &nBytesWritten, NULL))
					{
						cerr << "Cannot send the message to server." << endl;
						break;
					}
					cout << "New record is sent to server." << endl;
				}
			}
		}
	}

	return 0;
}