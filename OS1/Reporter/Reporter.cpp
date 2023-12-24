#include <iostream>
#include <string>
#include <fstream>
#include "../Employee/Employee.h"
using std::string;
using std::ifstream;
using std::ofstream;
using std::cerr;
using std::endl;

const int MAX_FILENAME_SIZE = 255;

int main(int argc, char* argv[])
{
	string input_filename(argv[1]);
	string output_filename(argv[2]);
	int employees_salary(std::stoi(argv[3]));
	employee current_employee;
	ifstream fin(input_filename);
	ofstream fout(output_filename);

	if (!fin.is_open() || !fout.is_open())
	{
		cerr << "cannot open file\n";
	}
	fout << "Report for \'" << input_filename << "\'.\nemployee id, name, working hours\n";
	while (fin >> current_employee.num >> current_employee.name >> current_employee.hours)
	{
		fout << current_employee.num << " " << current_employee.name << " " << current_employee.hours << " " << current_employee.hours * employees_salary << endl;
	}

	fout.close();
	fin.close();
	system("pause");
	return 0;
}