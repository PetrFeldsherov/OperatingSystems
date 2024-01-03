#include <process.h>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <conio.h>
#include <time.h>
#include <algorithm>
#include "..\Employee\Employee.h"
using std::string;
using std::cout;
using std::cin;
using std::cerr;
using std::endl;


const int CLIENTS_NUM = 3;
const string PIPE_NAME = "\\\\.\\pipe\\pipe_name";
const string START_ALL_EV_NAME = "start_all_ev";
const string START_EV_NAME_PREFIX = "start_";
const string CLIENT_MODULE_NAME = "Client.exe ";
const int MAX_FILENAME_SIZE = 255;
const int MAX_MESSAGE_SIZE = 10;

HANDLE* hReadyEvents;
CRITICAL_SECTION EmployeesCS;
int employees_num;
employee* employees;
bool* employee_modified;

void free_space(bool* employee_is_modifying, employee* employees, HANDLE* hReadyEvents, PROCESS_INFORMATION* piClientProcesses)
{
    delete[] employee_is_modifying;
    delete[] employees;
    delete[] hReadyEvents;
    delete[] piClientProcesses;
}

void read_employees_console()
{
    cout << "repeating input: {employees' data}" << endl;
    for (int i = 0; i < employees_num; i++) {
        cout << "(" << i + 1 << "): <id> <name> <working hours>" << endl;
        cin >> employees[i].num >> employees[i].name >> employees[i].hours;
    }
}

void sort_employees()
{
    qsort(employees, employees_num, sizeof(employee), employees_comparator);
}

void write_employees_binary(string filename)
{
    std::fstream fout(filename.c_str(), std::ios::binary | std::ios::out);
    if (!fout.is_open())
    {
        fout.close();
        throw std::fstream::failure("Cannot open binary file for writing.");
    }
    fout.write(reinterpret_cast<char*>(employees), sizeof(employee) * employees_num);
    fout.close();
}

bool CreateClientProcess(string commandline_buffer, STARTUPINFO& si, PROCESS_INFORMATION& pi)
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

employee* find_employee(int employee_id)
{
    employee search_key;
    search_key.num = employee_id;
    return reinterpret_cast<employee*>(bsearch(reinterpret_cast<const char*>(&search_key), reinterpret_cast<const char*>(employees),
        employees_num, sizeof(employee), employees_comparator));
}

int get_recieved_id(char* recieved_message)
{
    recieved_message[0] = ' ';
    return atoi(recieved_message);
}

DWORD WINAPI messaging(LPVOID p) {
    HANDLE hNamedPipe = (HANDLE)p;
    DWORD nBytesRead;
    DWORD nBytesWritten;
    char recieved_message[MAX_MESSAGE_SIZE];
    char recieved_command;
    int recieved_id;
    unsigned int employee_index;
    employee* employee_to_send;
    employee* invalid_employee = new employee; // empl with id -1 is invalid, passing it as message means error
    invalid_employee->num = -1;

    while (true)
    {
        if (!ReadFile(hNamedPipe, recieved_message, MAX_MESSAGE_SIZE, &nBytesRead, NULL))
        {
            if (ERROR_BROKEN_PIPE == GetLastError())
            {
                cout << "Client disconnected." << endl;
                break;
            }
            cerr << "Cannot read message." << endl;
            break;
        }

        if (strlen(recieved_message) > 0) {
            recieved_command = recieved_message[0];
            recieved_id = get_recieved_id(recieved_message);
            EnterCriticalSection(&EmployeesCS);
            employee_to_send = find_employee(recieved_id);
            LeaveCriticalSection(&EmployeesCS);

            // sending response
            if (!employee_to_send) {
                employee_to_send = invalid_employee;
            }
            else {
                employee_index = employee_to_send - employees;
                if (employee_modified[employee_index])
                    employee_to_send = invalid_employee;
                else {
                    switch (recieved_command) {
                    case 'w':
                        cout << "requested: modify id " << recieved_id << endl;
                        employee_modified[employee_index] = true;
                        break;
                    case 'r':
                        cout << "requested: read id " << recieved_id << endl;
                        break;
                    }
                }
            }

            if (!WriteFile(hNamedPipe, employee_to_send, sizeof(employee), &nBytesWritten, NULL))
            {
                cerr << "Cannod send the answer to client." << endl;
            }
            else
            {
                cout << "The response is sent to client." << endl;
            }

            //receiving a changed record
            if ('w' == recieved_command && employee_to_send != invalid_employee) {
                if (ReadFile(hNamedPipe, employee_to_send, sizeof(employee), &nBytesRead, NULL)) {
                    cout << "Employee record changed." << endl;
                    employee_modified[employee_to_send - employees] = false;
                    EnterCriticalSection(&EmployeesCS);
                    sort_employees();
                    LeaveCriticalSection(&EmployeesCS);
                }
                else {
                    std::cerr << "Error in reading a message." << endl;
                    break;
                }
            }
        }
    }
    FlushFileBuffers(hNamedPipe);
    DisconnectNamedPipe(hNamedPipe);
    CloseHandle(hNamedPipe);
    delete invalid_employee;
    return 0;
}

void open_pipes() {
    HANDLE hNamedPipe;
    HANDLE* hThreads = new HANDLE[CLIENTS_NUM];
    if (!hThreads)
    {
        throw std::exception("Cannot allocate space for array.");
    }
    for (int i = 0; i < CLIENTS_NUM; i++) {
        hNamedPipe = CreateNamedPipe(PIPE_NAME.c_str(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, 0, 0, INFINITE, NULL);
        if (INVALID_HANDLE_VALUE == hNamedPipe) {
            throw std::exception("Cannot create named pipe.");
        }
        if (!ConnectNamedPipe(hNamedPipe, NULL)) {
            cout << "No client has been connected to the pipe." << endl;
            break;
        }
        if (!(hThreads[i] = CreateThread(NULL, 0, messaging, static_cast<LPVOID>(hNamedPipe), 0, NULL)))
        {
            throw std::exception("Cannot create thread.");
        }
    }
    cout << "Clients connected to pipe." << endl;
    WaitForMultipleObjects(CLIENTS_NUM, hThreads, TRUE, INFINITE);
    cout << "All clients are disconnected." << endl;
    delete[] hThreads;
}

int main() {
    STARTUPINFO si;
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    PROCESS_INFORMATION* piClientProcesses;
    HANDLE hStartAllEvent;
    string filename, commandline_buffer, ready_ev_name;

    cout << "input: <filename>" << endl;
    cin >> filename;
    if (!(filename.size() > 0 && filename.size() < MAX_FILENAME_SIZE))
    {
        cerr << "Invalid filename length." << endl;
        return 1;
    }
    cout << "input: <employees num>" << endl;
    cin >> employees_num;


    piClientProcesses = new PROCESS_INFORMATION[CLIENTS_NUM];
    hReadyEvents = new HANDLE[CLIENTS_NUM];
    employees = new employee[employees_num];
    employee_modified = new bool[employees_num];
    if (!piClientProcesses || !hReadyEvents || !employees || !employee_modified)
    {
        cerr << "Cannot allocate space for array." << endl;
        free_space(employee_modified, employees, hReadyEvents, piClientProcesses);
        return 1;
    }
    for (int i = 0; i < employees_num; i++)
    {
        employee_modified[i] = false;
    }


    read_employees_console();
    sort_employees();
    try
    {
        write_employees_binary(filename);
    }
    catch (std::fstream::failure e)
    {
        cerr << e.what() << endl;
        free_space(employee_modified, employees, hReadyEvents, piClientProcesses);
        return 1;
    }


    hStartAllEvent = CreateEvent(NULL, TRUE, FALSE, START_ALL_EV_NAME.c_str());
    if (!hStartAllEvent)
    {
        cerr << "Cannot create event." << endl;
        free_space(employee_modified, employees, hReadyEvents, piClientProcesses);
        return GetLastError();
    }
    for (int i = 0; i < CLIENTS_NUM; i++)
    {
        ready_ev_name = START_EV_NAME_PREFIX + std::to_string(i);
        if (!(hReadyEvents[i] = CreateEvent(NULL, FALSE, FALSE, ready_ev_name.c_str())))
        {
            cerr << "Cannot create event." << endl;
            free_space(employee_modified, employees, hReadyEvents, piClientProcesses);
            return GetLastError();
        }

        commandline_buffer = CLIENT_MODULE_NAME + ready_ev_name;
        if (!CreateClientProcess(commandline_buffer, si, piClientProcesses[i]))
        {
            cerr << "Cannot create process." << endl;
            free_space(employee_modified, employees, hReadyEvents, piClientProcesses);
            return GetLastError();
        }
    }
    WaitForMultipleObjects(CLIENTS_NUM, hReadyEvents, TRUE, INFINITE);
    SetEvent(hStartAllEvent);
    try
    {
        open_pipes();
    }
    catch (std::exception e)
    {
        cerr << e.what() << endl;
        free_space(employee_modified, employees, hReadyEvents, piClientProcesses);
        return 1;
    }

    cout << "\tID, Name, Working hours" << endl;
    for (int i = 0; i < employees_num; i++)
        cout << " #" << i + 1 << " " << employees[i].num << employees[i].name << employees[i].hours << endl;
    free_space(employee_modified, employees, hReadyEvents, piClientProcesses);
    system("pause");
    return 0;
}
