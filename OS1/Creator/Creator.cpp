#include <iostream>
#include <fstream>
#include <string>
#include "../Employee/Employee.h"
using std::string;
using std::ofstream;
using std::cerr;
using std::endl;
using std::cin;
using std::cout;

const int MAX_FILENAME_SIZE = 255;

int main(int argc, char* argv[])
{
	string input_filename(argv[1]);
	int employees_num(std::stoi(argv[2]));
	employee current_employee;
	ofstream fout(input_filename);

	if (!fout.is_open())
	{
		cerr << "Cannot open file." << endl;
		return 1;
	}
	cout << "\temployee id, name, working hours" << endl;
	for (int i = 1; i <= employees_num; i++)
	{
		cout << "  #" << i << ": ";
		cin >> current_employee.num >> current_employee.name >> current_employee.hours;
		fout << current_employee.num << " " << current_employee.name << " " << current_employee.hours << endl;
	}

	fout.close();
	system("pause");
	return 0;
}