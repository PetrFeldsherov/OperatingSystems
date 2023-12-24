#include <windows.h>
#include <process.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
using std::cin;
using std::cout;
using std::endl;
using std::cerr;
using std::string;
using std::to_string;
using std::ifstream;
using std::ofstream;
using std::vector;

const string PRODUCER_SEM_NAME = "producer_semaphore_name";
const string CONSUMER_SEM_NAME = "consumer_semaphore_name";
const string MES_TRANS_MUT_NAME = "mess_trans_mutex_name";
const string READY_EV_NAME_PREFIX = "ready_";

bool CreateProducerProcess(string commandline_buffer, STARTUPINFO& si, PROCESS_INFORMATION& pi);
void free_allocated_space(PROCESS_INFORMATION* piProducerProcesses, HANDLE* hReadyEvents);
string get_correct_command();
vector<string> get_lines_to_rewrite(ifstream& fin);
void rewrite_lines_left(ofstream& fout, vector<string>& lines);
string extract_message(string message_transfer_filename);

int main()
{
	HANDLE hProducerSemaphore;
	HANDLE hConsumerSemaphore;
	HANDLE hMessageTransferMutex;
	HANDLE* hReadyEvents;
	STARTUPINFO si;
	ZeroMemory(&si, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	PROCESS_INFORMATION* piProducerProcesses;

	string message_transfer_filename;
	unsigned int message_buffer_size, producers_num;
	cout << "input: <message transfer filename>" << endl;
	cin >> message_transfer_filename;
	ofstream fout(message_transfer_filename);
	if (!fout.is_open())
	{
		fout.close();
		cerr << "Cannot open file with provided filename." << endl;
		return 1;
	}
	fout.close();
	cout << "input: <message buffer size> <senders num>" << endl;
	cin >> message_buffer_size >> producers_num;
	if (0 == message_buffer_size || 0 == producers_num)
	{
		cerr << "Message buffer size and producers num mustn't be zero." << endl;
		return 1;
	}
	cout << endl << endl;

	hProducerSemaphore = CreateSemaphore(NULL, message_buffer_size, message_buffer_size, PRODUCER_SEM_NAME.c_str());
	hConsumerSemaphore = CreateSemaphore(NULL, 0, message_buffer_size, CONSUMER_SEM_NAME.c_str());
	hMessageTransferMutex = CreateMutex(NULL, FALSE, MES_TRANS_MUT_NAME.c_str());
	if (!hProducerSemaphore || !hConsumerSemaphore || !hMessageTransferMutex)
	{
		cerr << "Cannot create synchronization object." << endl;
		return GetLastError();
	}
	hReadyEvents = new HANDLE[producers_num];
	piProducerProcesses = new PROCESS_INFORMATION[producers_num];
	if (!hReadyEvents || !piProducerProcesses)
	{
		free_allocated_space(piProducerProcesses, hReadyEvents);
		cerr << "Cannot allocate space for array." << endl;
		return GetLastError();
	}

	string ready_ev_name;
	string commandline_buffer;
	for (int i = 0; i < producers_num; i++)
	{
		ready_ev_name = READY_EV_NAME_PREFIX + to_string(i);
		if (!(hReadyEvents[i] = CreateEvent(NULL, FALSE, FALSE, ready_ev_name.c_str())))
		{
			free_allocated_space(piProducerProcesses, hReadyEvents);
			cerr << "Cannot create event." << endl;
			return GetLastError();
		}

		commandline_buffer = "Sender.exe " + message_transfer_filename + " " + ready_ev_name + " " + PRODUCER_SEM_NAME + " " + CONSUMER_SEM_NAME + " " + MES_TRANS_MUT_NAME;
		if (!CreateProducerProcess(commandline_buffer, si, piProducerProcesses[i]))
		{
			free_allocated_space(piProducerProcesses, hReadyEvents);
			cerr << "Cannot create process." << endl;
			return GetLastError();
		}
	}

	WaitForMultipleObjects(producers_num, &hReadyEvents[0], TRUE, INFINITE);
	string command;
	string message;
	while (true)
	{
		command = get_correct_command();
		if ("exit" == command)
		{
			break;
		}
		//ReleaseSemaphore(hConsumerSemaphore, 1, NULL);
		WaitForSingleObject(hConsumerSemaphore, INFINITE);

		WaitForSingleObject(hMessageTransferMutex, INFINITE);
		try
		{
			message = extract_message(message_transfer_filename);
		}
		catch (std::ios_base::failure e)
		{
			cerr << "Extracting the line needed from file is failed: " << e.what() << endl;
			return 1;
		}
		ReleaseMutex(hMessageTransferMutex);

		ReleaseSemaphore(hProducerSemaphore, 1, NULL);
		cout << "message: " << message << endl;
	}

	CloseHandle(hConsumerSemaphore);
	CloseHandle(hProducerSemaphore);
	CloseHandle(hMessageTransferMutex);
	for (int i = 0; i < producers_num; i++)
	{
		CloseHandle(hReadyEvents[i]);
	}

	system("pause");
	return 0;
}

bool CreateProducerProcess(string commandline_buffer, STARTUPINFO& si, PROCESS_INFORMATION& pi)
{
	return CreateProcess(
		NULL, // no module name
		&commandline_buffer[0], // widestr commandline with module name
		NULL, // default process sequrity atributes
		NULL, // default main thread sequrity atributes
		FALSE, // descriptors won't be inherited
		CREATE_NEW_CONSOLE,
		NULL, // current
		NULL, // current
		&si, // ZeroMemory - WIN OS "decides on its own"
		&pi); // descriptors and IDs of the process and its main thread
}

void free_allocated_space(PROCESS_INFORMATION* piProducerProcesses, HANDLE* hReadyEvents)
{
	delete[] piProducerProcesses;
	delete[] hReadyEvents;
}

string get_correct_command()
{
	string command;
	cout << "input: <\'read\' / \'exit\'>" << endl;
	cin >> command;
	while (!("read" == command || "exit" == command))
	{
		cout << "the command provided doesn't correspond to any of the options" << endl << "correct input format: <\'read\' / \'exit\'>" << endl;
		cin >> command;
	}
	return command;
}

vector<string> get_lines_to_rewrite(ifstream& fin)
{
	vector<string> result;
	string line;
	while (getline(fin, line))
	{
		result.push_back(line);
	}
	return result;
}

void rewrite_lines_left(ofstream& fout, vector<string>& lines)
{
	for (string line : lines)
	{
		fout << line << endl;
	}
}


string extract_message(string message_transfer_filename)
{
	string extracted_message;
	vector<string> buffer_rewrite_left;

	ifstream fin(message_transfer_filename);
	if (!fin.is_open())
	{
		throw ifstream::failure("Cannot open file for reading.");
	}
	if (!getline(fin, extracted_message))
	{
		throw ifstream::failure("No message to extract from file for message transmission.");
	}
	
	buffer_rewrite_left = get_lines_to_rewrite(fin);
	fin.close();
	ofstream fout(message_transfer_filename);
	if (!fout.is_open())
	{
		throw ofstream::failure("Cannot open file for writing.");
	}
	rewrite_lines_left(fout, buffer_rewrite_left);
	fout.close();

	return extracted_message;
}