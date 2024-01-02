// Colloqium_operations_from_file.cpp : Defines the entry point for the application.
//

#include "Colloqium_operations_from_file.h"
#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
using std::string;
using std::vector;
using std::cout;
using std::cin;
using std::cerr;
using std::endl;

HANDLE hResultFileMutex;
const string INPUT_FILENAME_PREFIX = "in_";
const string OUTPUT_PATH_POSTFIX = "/out.dat";
string abs_dir_path;
vector<string> input_filenames;
int files_num;
vector<bool> file_is_free;

void free_space(DWORD* IDThreads, HANDLE* hThreads)
{
	delete[] IDThreads;
	delete[] hThreads;
}

void increment_resulting_sum(int increment)
{
	int curr_resulting_sum;

	WaitForSingleObject(hResultFileMutex, INFINITE);

	std::ifstream fin(abs_dir_path + OUTPUT_PATH_POSTFIX);
	if (!fin.is_open())
	{
		fin.close();
		throw std::ifstream::failure("Cannot open file for reading.");
	}
	fin >> curr_resulting_sum;
	curr_resulting_sum += increment;
	fin.close();
	std::ofstream fout(abs_dir_path + OUTPUT_PATH_POSTFIX);
	if (!fout.is_open())
	{
		fin.close();
		throw std::ofstream::failure("Cannot open file for writing.");
	}
	fout << curr_resulting_sum;
	fout.close();

	ReleaseMutex(hResultFileMutex);
}

int get_file_result(string filename)
{
	int operation;
	double first_operand, second_operand, file_result;

	std::ifstream fin(filename);
	if (!fin.is_open())
	{
		fin.close();
		throw std::ifstream::failure("Cannot open file for reading.");
	}
	fin >> operation >> first_operand >> second_operand;

	switch (operation)
	{
	case 1:
		file_result = first_operand + second_operand;
		break;
	case 2:
		file_result = first_operand * second_operand;
		break;
	case 3:
		file_result = first_operand * first_operand + second_operand * second_operand;
		break;
	default:
		fin.close();
		throw std::ifstream::failure("Wrong operation provided.");
	}
	fin.close();
	return file_result;
}

DWORD WINAPI ProcessFreeFiles(LPVOID iStartIndex)
{
	int file_result;
	int curr_index = (int)iStartIndex;
	while (curr_index < files_num)
	{
		if (file_is_free[curr_index])
		{
			file_is_free[curr_index] = false;
			try
			{
				file_result = get_file_result(input_filenames[curr_index]);
				increment_resulting_sum(file_result);
			}
			catch (std::exception e)
			{
				cerr << e.what() << endl;
				return 1;
			}
		}
		curr_index++;
	}
	return 0;
}

int main()
{
	HANDLE* hThreads;
	DWORD* IDThreads;
	unsigned int threads_num, files_per_thread;
	hResultFileMutex = CreateMutex(NULL, TRUE, NULL);
	if (!hResultFileMutex)
	{
		cerr << "Cannot create mutex." << endl;
		return 1;
	}


	cout << "input: <abs dir path>" << endl;
	std::getline(cin, abs_dir_path);
	if (!std::filesystem::is_directory(abs_dir_path))
	{
		cerr << "Invalid abs dir path." << endl;
		return 1;
	}
	cout << "input: <threads num>" << endl;
	cin >> threads_num;
	hThreads = new HANDLE[threads_num];
	IDThreads = new DWORD[threads_num];
	if (!hThreads or !IDThreads)
	{
		cerr << "Cannot allocate space for array." << endl;
		free_space(IDThreads, hThreads);
		return 1;
	}


	for (const auto& dir_item : std::filesystem::directory_iterator(abs_dir_path))
	{
		string curr_filename = dir_item.path().string();
		if (dir_item.is_regular_file() and 0 == curr_filename.find(INPUT_FILENAME_PREFIX))
		{
			input_filenames.push_back(curr_filename);
		}
	}
	if (input_filenames.empty())
	{
		cerr << "No input files with \'" << INPUT_FILENAME_PREFIX << "\' prefix are found in directory." << endl;
		free_space(IDThreads, hThreads);
		return 1;
	}
	files_num = input_filenames.size();
	file_is_free.resize(files_num, true);
	files_per_thread = files_num / threads_num;


	int files_index = 0;
	int threads_index = 0;
	while (files_index < files_num)
	{
		if (!(hThreads[threads_index] = CreateThread(NULL, 0, ProcessFreeFiles, (void*)files_index, 0, &IDThreads[threads_index])))
		{
			cerr << "Cannot create thread." << endl;
			free_space(IDThreads, hThreads);
			return 1;
		}
		files_index += (files_per_thread != 0) ? files_per_thread : 1;
		threads_index += 1;
	}

	for (int i = 0; i < threads_index; i++)
	{
		CloseHandle(hThreads[i]);
	}

	free_space(IDThreads, hThreads);
	return 0;
}
