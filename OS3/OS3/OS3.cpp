#include <windows.h>	
#include <process.h>	
#include <iostream>
#include <chrono>
using std::cin;
using std::cout;
using std::endl;
using std::cerr;

const int MARKER_SLEEP_TIME = 5;
const int CELL_INITIAL_VALUE = 0;
int* arr;
int dim;
CRITICAL_SECTION critical_section;
HANDLE arbeiten_nicht_kapitulieren;
HANDLE* kann_nicht_arbeiten;
HANDLE** arbeiten_oder_kapitulieren;


DWORD WINAPI MarkArray(LPVOID iMarkerIndex)
{
	WaitForSingleObject(arbeiten_nicht_kapitulieren, INFINITE);
	
	int marker_index = (int)iMarkerIndex;
	int marked_cells_num = 0;
	int current_arr_index;
	srand(marker_index);
	while (true)
	{
		current_arr_index = rand() % dim;
		EnterCriticalSection(&critical_section);
		if (CELL_INITIAL_VALUE == arr[current_arr_index])
		{
			Sleep(MARKER_SLEEP_TIME);
			arr[current_arr_index] = marker_index + 1;
			marked_cells_num++;
			LeaveCriticalSection(&critical_section);
			Sleep(MARKER_SLEEP_TIME);
		}
		else
		{
			cout << "(C) " << marker_index + 1 << " kann nicht arbeiten. [amount of marked arr cells is " << marked_cells_num << ", cannot mark " << current_arr_index << "th cell of arr.]" << endl;
			LeaveCriticalSection(&critical_section);
			SetEvent(kann_nicht_arbeiten[marker_index]);
			int was_ist_das = WaitForMultipleObjects(2, arbeiten_oder_kapitulieren[marker_index], FALSE, INFINITE);
			if (1 == was_ist_das)
			{
				for (int i = 0; i < dim; i++)
				{
					if (arr[i] == marker_index + 1)
					{
						arr[i] = CELL_INITIAL_VALUE;
					}
				}
				cout << "(C) " << marker_index + 1 << " kapitulieren." << endl;
				break;
			}
			else
			{
				ResetEvent(arbeiten_oder_kapitulieren[marker_index][0]); // чтоб потом могли приказать продолжить работу заново, тут же ручное обновление указали, только насчет доступа не пон, сможет ли он сам перенатроить
			}
		}
	}

	return 0;
}

void free_handle_matrix_rows(HANDLE** matrix, int rows_already_allocated)
{
	for (int i = 0; i < rows_already_allocated; i++) {
		delete[] matrix[i];
	}
}

void free_space(HANDLE** arbeiten_oder_kapitulieren, HANDLE* kann_nicht_arbeiten, bool* markers_stopped, DWORD* IDMarkers, HANDLE* hMarkers, int* arr)
{
	delete[] arbeiten_oder_kapitulieren;
	delete[] kann_nicht_arbeiten;
	delete[] markers_stopped;
	delete[] IDMarkers;
	delete[] hMarkers;
	delete[] arr;
}

void print_array(int* arr, int dim)
{
	for (int i = 0; i < dim; i++)
	{
		cout << arr[i] << " ";
	}
	cout << endl;
}

int get_index_to_stop(int markers_num, bool* markers_stopped)
{
	int index_to_stop;
	cout << "input: <index of marker to stop>" << endl;
	cin >> index_to_stop;
	while (!(index_to_stop > 0 && index_to_stop <= markers_num) || markers_stopped[index_to_stop - 1])
	{
		cout << "Index out of range or marker has already stopped." << endl << "input: <index of marker to stop>" << endl;
		cin >> index_to_stop;
	}
	return index_to_stop - 1; // indexation is from 0 in our data structures and markers' names are from 1
}

int main()
{
	unsigned int markers_num;
	HANDLE* hMarkers;
	DWORD* IDMarkers;
	bool* markers_stopped;
	cout << "input: <array dimension>" << endl;
	cin >> dim;
	cout << "input: <markers num>" << endl;
	cin >> markers_num;
	cout << endl << endl;

	arr = new int[dim];
	for (int i = 0; i < dim; i++)
	{
		arr[i] = CELL_INITIAL_VALUE;
	}
	hMarkers = new HANDLE[markers_num];
	IDMarkers = new DWORD[markers_num];
	markers_stopped = new bool[markers_num];

	kann_nicht_arbeiten = new HANDLE[markers_num];
	arbeiten_oder_kapitulieren = new HANDLE*[markers_num];

	if (!arr || !hMarkers || !IDMarkers || !markers_stopped || !kann_nicht_arbeiten || !arbeiten_oder_kapitulieren)
	{
		cerr << "Cannot allocate space for array." << endl;
		free_space(arbeiten_oder_kapitulieren, kann_nicht_arbeiten, markers_stopped, IDMarkers, hMarkers, arr);
		return 1;
	}


	arbeiten_nicht_kapitulieren = CreateEvent(NULL, TRUE, FALSE, NULL); // correct, threads start event
	if (!arbeiten_nicht_kapitulieren)
	{
		cerr << "Cannot create event: " << GetLastError() << endl;
		free_space(arbeiten_oder_kapitulieren, kann_nicht_arbeiten, markers_stopped, IDMarkers, hMarkers, arr);
		return 1;
	}

	for (int i = 0; i < markers_num; i++)
	{
		arbeiten_oder_kapitulieren[i] = new HANDLE[2];
		if (!arbeiten_oder_kapitulieren[i])
		{
			cerr << "Cannot allocate space for array." << endl;
			free_handle_matrix_rows(arbeiten_oder_kapitulieren, i + 1);
			free_space(arbeiten_oder_kapitulieren, kann_nicht_arbeiten, markers_stopped, IDMarkers, hMarkers, arr);
			return 1;
		}
		kann_nicht_arbeiten[i] = CreateEvent(NULL, TRUE, FALSE, NULL); // threads event null true false null correct
		arbeiten_oder_kapitulieren[i][0] = CreateEvent(NULL, FALSE, FALSE, NULL); // auto reset of event
		arbeiten_oder_kapitulieren[i][1] = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (!kann_nicht_arbeiten[i] || !arbeiten_oder_kapitulieren[i][0] || !arbeiten_oder_kapitulieren[i][1])
		{
			cerr << "Cannot create event: " << GetLastError() << endl;
			free_handle_matrix_rows(arbeiten_oder_kapitulieren, i + 1);
			free_space(arbeiten_oder_kapitulieren, kann_nicht_arbeiten, markers_stopped, IDMarkers, hMarkers, arr);
			return 1;
		}


		markers_stopped[i] = false;
		if (!(hMarkers[i] = CreateThread(NULL, 0, MarkArray, (void*)(i), 0, &IDMarkers[i])))
		{
			cerr << "Cannot create thread: " << GetLastError() << endl;
			free_handle_matrix_rows(arbeiten_oder_kapitulieren, i + 1);
			free_space(arbeiten_oder_kapitulieren, kann_nicht_arbeiten, markers_stopped, IDMarkers, hMarkers, arr);
			return 1;
		}
	}


	InitializeCriticalSection(&critical_section);
	SetEvent(arbeiten_nicht_kapitulieren);

	int index_to_stop;
	int working_threads_left = markers_num;
	while (working_threads_left--)
	{
		WaitForMultipleObjects(markers_num, &kann_nicht_arbeiten[0], TRUE, INFINITE);
		cout << "these german markers cannot work further, current state of array:" << endl;
		print_array(arr, dim);

		index_to_stop = get_index_to_stop(markers_num, markers_stopped);
		markers_stopped[index_to_stop] = true;
		SetEvent(arbeiten_oder_kapitulieren[index_to_stop][1]);
		WaitForSingleObject(hMarkers[index_to_stop], INFINITE);
		cout << "this german marker with index " << index_to_stop + 1 << " is stopped and others \"weiter arbeiten\" (if any left, idk), current state of array:" << endl;
		print_array(arr, dim);
		cout << endl << endl;

		for (int i = 0; i < markers_num; i++)
		{
			if (markers_stopped[i])
			{
				continue;
			}

			ResetEvent(kann_nicht_arbeiten[i]); // чтоб потом могли пожаловаться опять фрицы
			SetEvent(arbeiten_oder_kapitulieren[i][0]); // приказано продолжить работу всем остальным, kann_nicht_arbeiten работает как по маслу, а вот когда осуществляется вызов продолжить работу, то надо обратно переустанавливать
		}
	}
	cout << "oh, lord, what's the matter with our array" << endl;

	DeleteCriticalSection(&critical_section);

	return 0;
}