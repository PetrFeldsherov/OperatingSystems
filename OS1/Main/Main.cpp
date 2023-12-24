#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include "../Employee/Employee.h"
using std::cout;
using std::cin;
using std::endl;
using std::string;
using std::cerr;
using std::ifstream;

const int MAX_FILENAME_SIZE = 255;
const string CREATOR_MODULE_NAME = "Creator.exe";
const string REPORTER_MODULE_NAME = "Reporter.exe";

bool CreateUtilProcess(string commandline_buffer, STARTUPINFO& si, PROCESS_INFORMATION& pi);
void print_content(string filename);

int main()
{
    string input_filename;
    string output_filename;
    string commandline_buffer;
    unsigned int employees_num;
    unsigned int employees_salary;
    STARTUPINFO siCreator;
    STARTUPINFO siReporter;
    PROCESS_INFORMATION piCreator;
    PROCESS_INFORMATION piReporter;

    ZeroMemory(&siCreator, sizeof(STARTUPINFO));
    ZeroMemory(&siReporter, sizeof(STARTUPINFO));
    siCreator.cb = sizeof(STARTUPINFO);
    siReporter.cb = sizeof(STARTUPINFO);


    cout << "input: <input filename>" << endl;
    std::getline(cin, input_filename);
    if (input_filename.empty() || input_filename.size() > MAX_FILENAME_SIZE)
    {
        cerr << "Invalid filename." << endl;
        return 1;
    }
    cout << "input: <employees num>" << endl;
    cin >> employees_num;

    commandline_buffer = CREATOR_MODULE_NAME + " " + input_filename + " " + std::to_string(employees_num);
    if (!CreateUtilProcess(commandline_buffer, siCreator, piCreator))
    {
        cerr << GetLastError() << endl;
        return 1;
    }
    WaitForSingleObject(piCreator.hProcess, INFINITE);
    CloseHandle(piCreator.hThread);
    CloseHandle(piCreator.hProcess);


    cout << "input provided in Creator console:" << endl << "employee id, name, working hours" << endl;
    try
    {
        print_content(input_filename);
    }
    catch (const ifstream::failure& e)
    {
        cerr << e.what() << endl;
        return 1;
    }


    cout << "input: <output filename>" << endl;
    getchar();
    std::getline(cin, output_filename);
    if (output_filename.empty() || output_filename.size() > MAX_FILENAME_SIZE)
    {
        cerr << "Invalid filename." << endl;
        return 1;
    }
    cout << "input: <employees salary>" << endl;
    cin >> employees_salary;

    commandline_buffer = REPORTER_MODULE_NAME + " " + input_filename + " " + output_filename + " " + std::to_string(employees_salary);
    if (!CreateUtilProcess(commandline_buffer, siReporter, piReporter))
    {
        cerr << GetLastError() << endl;
        return 1;
    }
    WaitForSingleObject(piReporter.hProcess, INFINITE);
    CloseHandle(piReporter.hThread);
    CloseHandle(piReporter.hProcess);

    cout << "Report from Reporter:" << endl;
    try
    {
        print_content(output_filename);
    }
    catch (const ifstream::failure& e)
    {
        cerr << e.what() << endl;
        return 1;
    }

    system("pause");
    return 0;
}

bool CreateUtilProcess(string commandline_buffer, STARTUPINFO& si, PROCESS_INFORMATION& pi)
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

void print_content(string filename) {
    string buffer;
    ifstream fin(filename);

    if (!fin.is_open())
    {
        throw ifstream::failure("Cannot open file.");
    }
    while (getline(fin, buffer))
    {
        cout << buffer << endl;
    }
    fin.close();
}