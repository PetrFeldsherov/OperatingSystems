// OS2.cpp : Defines the entry point for the application.
//

#include <windows.h>
#include <iostream>
using std::cin;
using std::cout;
using std::endl;
using std::cerr;

volatile int* min_num;
volatile int* max_num;
volatile int nums_sum = 0;

DWORD WINAPI MinMax(LPVOID iptrFirstElement)
{
    int* curr_element = static_cast<int*>(iptrFirstElement);
    int dim = *curr_element;
    curr_element++;

    min_num = max_num = curr_element;
    while (std::distance(static_cast<int*>(iptrFirstElement), curr_element) <= dim)
    {
        if (*curr_element > *max_num)
        {
            max_num = curr_element;
        }
        else if (*curr_element < *min_num)
        {
            min_num = curr_element;
        }
        Sleep(7);
        curr_element++;
    }

    return 0;
}

DWORD WINAPI Sum(LPVOID iptrFirstElement) // dim итак известен в main
{
    int* elements = static_cast<int*>(iptrFirstElement);
    int dim = *elements;
    elements++;

    while (dim--)
        nums_sum += elements[dim];

    return 0;
}

int main()
{
    HANDLE hMinMax;
    HANDLE hSum;
    DWORD dwIDMinMax;
    DWORD dwIDSum;
    unsigned int dim;
    int* numbers;


    cout << "input: <array dimension>" << endl;
    cin >> dim;
    if (0 == dim)
    {
        cerr << "The dimension provided is 0, empty array." << endl;
        return 0;
    }
    numbers = new int[dim + 1];
    if (!numbers)
    {
        delete[] numbers;
        cerr << "Cannot alocate space for numbers array." << endl;
        return 1;
    }
    numbers[0] = dim;
    cout << "input: <int array elements...>" << endl;
    for (int i = 1; i <= dim; i++)
    {
        cin >> numbers[i];
    }

    if (!(hMinMax = CreateThread(NULL, 0, MinMax, static_cast<void*>(numbers), 0, &dwIDMinMax)))
    {
        cerr << "Cannot create MinMax thread: " << GetLastError() << endl;
        return 1;
    }
    if (!(hSum = CreateThread(NULL, 0, Sum, static_cast<void*>(numbers), 0, &dwIDSum)))
    {
        cerr << "Cannot create Sum thread: " << GetLastError() << endl;
        return 1;
    }
    WaitForSingleObject(hMinMax, INFINITE);
    CloseHandle(hMinMax);
    WaitForSingleObject(hSum, INFINITE);
    CloseHandle(hSum);

    int average = nums_sum / dim;
    cout << "minimal number is " << *min_num << " and maximal number is " << *max_num << ", average number is " << average << endl;
    *min_num = average;
    *max_num = average;
    for (int i = 1; i <= dim; i++)
    {
        cout << numbers[i] << " ";
    }
    cout << endl;

    delete[] numbers;
    system("pause");
    return 0;
}
