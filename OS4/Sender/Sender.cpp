#include <windows.h>
#include <process.h>
#include <iostream>
#include <fstream>
#include <string>
using std::cin;
using std::cout;
using std::endl;
using std::cerr;
using std::string;
using std::ofstream;
using std::ifstream;

string get_correct_command();
string get_correct_message();
void put_message(string message_transfer_filename, string message);

int main(int argc, char* argv[])
{
	string message_transfer_filename = argv[1];
	HANDLE hReadyEvent = OpenEvent(EVENT_MODIFY_STATE, TRUE, argv[2]);
	HANDLE hProducerSemaphore = OpenSemaphore(SEMAPHORE_ALL_ACCESS, TRUE, argv[3]);
	HANDLE hConsumerSemaphore = OpenSemaphore(SEMAPHORE_ALL_ACCESS, TRUE, argv[4]);
	HANDLE hMessageTransferMutex = OpenMutex(MUTEX_ALL_ACCESS, TRUE, argv[5]);
	if (!hProducerSemaphore || !hConsumerSemaphore || !hMessageTransferMutex || !hReadyEvent)
	{
		cerr << "Cannot open synchronization objects and event provided by parent process." << endl;
		return GetLastError();
	}

	SetEvent(hReadyEvent);
	string command;
	string message;
	while (true)
	{
		command = get_correct_command();
		if ("exit" == command)
		{
			break;
		}
		message = get_correct_message();
		WaitForSingleObject(hProducerSemaphore, INFINITE);

		WaitForSingleObject(hMessageTransferMutex, INFINITE);
		try
		{
			put_message(message_transfer_filename, message);
		}
		catch (const ofstream::failure& e)
		{
			cerr << e.what() << endl;
			return 1;
		}
		ReleaseMutex(hMessageTransferMutex);

		ReleaseSemaphore(hConsumerSemaphore, 1, NULL);
	}

	CloseHandle(hReadyEvent);
	CloseHandle(hProducerSemaphore);
	CloseHandle(hConsumerSemaphore);
	CloseHandle(hMessageTransferMutex);

	system("pause");
	return 0;
}

string get_correct_command()
{
	string command;
	cout << "input: <\'write\' / \'exit\'>" << endl;
	cin >> command;
	while (!("write" == command || "exit" == command))
	{
		cout << "the command provided doesn't correspond to any of the options" << endl << "correct input format: <\'write\' / \'exit\'>" << endl;
		cin >> command;
	}
	return command;
}

string get_correct_message()
{
	string message;
	cout << "input: <message, size is up to 20 symbols>" << endl;
	cin >> message;
	while (!(!message.empty() && message.size() <= 20))
	{
		cout << "the message provided doesn't correspond to conditions" << endl << "correct input format: <message, size is up to 20 symbols>" << endl;
		cin >> message;
	}
	return message;
}

void put_message(string message_transfer_filename, string message)
{
	ofstream fout(message_transfer_filename, std::ios::app);
	if (!fout.is_open())
	{
		throw ofstream::failure("Cannot open file for writing.");
	}
	fout << message << endl;
	fout.close();
}