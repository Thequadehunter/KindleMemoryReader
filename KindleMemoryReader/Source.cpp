#include <iostream>
#include<Windows.h>
#include <TlHelp32.h>
#include <psapi.h>
#include <fstream>
#include <string>
#include <stdlib.h> 
#include <tchar.h>

using namespace std;

//forward declarations
int GetWindowString(HWND hwnd, string &s);
BOOL CALLBACK FindWindowBySubstr(HWND hwnd, LPARAM substring);
HWND getKindleHWND();
void BringKindleToFront();
string GetKindleWindowTitle(HWND window);
string GetKindleBookTitle(string s);
DWORD_PTR GetModuleBaseAddress(DWORD processID);
DWORD readPointerChain(HANDLE handle, DWORD baseAddr, int pLevel, DWORD offsets[]);
//------------------------------------------------

int GetWindowString(HWND hwnd, string &s)
{
	char buffer[65536];

	int txtlen = GetWindowTextLength(hwnd) + 1; //idk why this works like this it just does
	GetWindowText(hwnd, buffer, txtlen); //read window text into char buffer

	s = buffer;
	return txtlen;
}

BOOL CALLBACK FindWindowBySubstr(HWND hwnd, LPARAM substring)
{
	const DWORD TITLE_SIZE = 1024;
	TCHAR windowTitle[TITLE_SIZE];

	if (GetWindowText(hwnd, windowTitle, TITLE_SIZE))
	{
		_tprintf(TEXT("%s\n"), windowTitle);
		// Uncomment to print all windows being enumerated 
		if (_tcsstr(windowTitle, LPCTSTR(substring)) != NULL)
		{
			// We found the window! Stop enumerating. 
			SwitchToThisWindow(hwnd, true); //bring it to front so we can get title of window
			return false;
		}
	}
	return true; // Need to continue enumerating windows 
}

void BringKindleToFront()
{
	const TCHAR substring[] = TEXT("Kindle for PC"); //TODO - make this a general use function you lazy ass
	EnumWindows(FindWindowBySubstr, (LPARAM)substring); // opens kindle to forefront upon finding it
}

HWND getKindleHWND()
{
	HWND window = GetForegroundWindow();
	return window;
}

string GetKindleWindowTitle(HWND window)
{
	string s;
	int len = GetWindowString(window, s); //read window name into s
	return s;
}

string GetKindleBookTitle(string s)
{
	unsigned found = s.find_last_of("-");
	string title = s.substr(found + 1);
	title.erase(0, 1); // erase extra space in first position
	return title;
}

//gets the base address of module by processID 
DWORD_PTR GetModuleBaseAddress(DWORD processID)
{
	DWORD_PTR   baseAddress = 0;
	HANDLE      processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
	HMODULE     *moduleArray;
	LPBYTE      moduleArrayBytes;
	DWORD       bytesRequired;

	if (processHandle)
	{
		if (EnumProcessModules(processHandle, NULL, 0, &bytesRequired)) //go through list of handles
		{
			if (bytesRequired)
			{
				moduleArrayBytes = (LPBYTE)LocalAlloc(LPTR, bytesRequired);

				if (moduleArrayBytes)
				{
					unsigned int moduleCount;

					moduleCount = bytesRequired / sizeof(HMODULE);
					moduleArray = (HMODULE *)moduleArrayBytes;

					if (EnumProcessModules(processHandle, moduleArray, bytesRequired, &bytesRequired))
					{
						baseAddress = (DWORD_PTR)moduleArray[0];
					}

					LocalFree(moduleArrayBytes);
				}
			}
		}

		CloseHandle(processHandle);
	}

	return baseAddress;
}


DWORD readPointerChain(HANDLE handle, DWORD baseAddr, int pLevel, DWORD offsets[])
{
	DWORD address = baseAddr;
	DWORD pTemp; //MUST READ VALUES INTO THIS

	for (int i = 0; i < pLevel; i++)
	{
		if (i == 0)
		{
			ReadProcessMemory(handle, (LPCVOID)address, &pTemp, sizeof(address), NULL); //initial read to store into ptemp
			cout << "initial read: " << hex << address << endl;
		}
		address = (DWORD)pTemp + (DWORD)offsets[i]; //go to next offset
		ReadProcessMemory(handle, (LPCVOID)address, &pTemp, sizeof(address), NULL); //read address with current offset to get next pointer
		cout << "next: " << hex << address << endl;
	}

	cout << "----------------------------------------------" << endl;

	return address;
}



int main() {

	HWND hwnd = FindWindow(NULL, "Kindle");
	int readTest = 0;
	int totalLocations = 0;
	ofstream f;

	if (hwnd == NULL) //if window not open
	{
		cout << "Can't find Window" << endl;
		Sleep(3000);
		exit(-1);
	}
	else
	{
		DWORD procID;
		GetWindowThreadProcessId(hwnd, &procID);
		DWORD baseAddr = GetModuleBaseAddress(procID); //get base address of selected window
		HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procID);

		if (procID == NULL)
		{
			cout << "Cannot obtain process id" << endl;
			Sleep(3000);
			exit(-1);
		}
		else
		{
			BringKindleToFront(); //bring kindle to front so we can get title of window
			HWND kindleHWND = getKindleHWND(); //get the hwnd of front window


			string windowTitle = GetKindleWindowTitle(kindleHWND); //window title
			string bookTitle = GetKindleBookTitle(windowTitle); // book title from window title

			cout << "Window Title: " << windowTitle << endl;
			cout << "Book Title: " << bookTitle << endl;


			DWORD baseOffset = 0x031BE1DC;
			DWORD baseOffsetTotalPages = 0x0321E3B0;
			DWORD offsets[5] = { 0x1C, 0xC, 0xCC, 0x158, 0x18 };
			DWORD totalPagesOffsets[5] = { 0x34, 0x120, 0x10, 0x4, 0x30 };
			DWORD baseAddrTotal = (DWORD)baseAddr + (DWORD)baseOffsetTotalPages;
			DWORD addr = readPointerChain(handle, baseAddr, 5, offsets);
			DWORD addrTotal = readPointerChain(handle, baseAddrTotal, 5, totalPagesOffsets);

			for (;;) //infinite loop
			{
				baseAddr = GetModuleBaseAddress(procID); //get base address of kindle - MUST DO THIS EVERY TIME
				cout << "base address: " << hex << (DWORD)baseAddr << endl;
				baseAddr = (DWORD)baseAddr + (DWORD)baseOffset;

				DWORD baseAddrBottom = GetModuleBaseAddress(procID); //reset base address

				baseAddrTotal = (DWORD)baseAddrBottom + (DWORD)baseOffsetTotalPages;
				DWORD addr = readPointerChain(handle, baseAddr, 5, offsets);
				DWORD addrTotal = readPointerChain(handle, baseAddrTotal, 5, totalPagesOffsets);

				kindleHWND = getKindleHWND(); // get current window again

				if (GetKindleWindowTitle(kindleHWND).find("Kindle for PC") != string::npos) //if kindle window is being used
				{
					windowTitle = GetKindleWindowTitle(kindleHWND); //window title
					bookTitle = GetKindleBookTitle(windowTitle); // book title
				}


				cout << "final address: " << hex << addr << endl;
				cout << "base address total: " << hex << addrTotal << endl;
				cout << "Window Title: " << windowTitle << endl;
				cout << "Book Title: " << bookTitle << endl;

				ReadProcessMemory(handle, (void*)addr, &readTest, sizeof(readTest), 0);
				ReadProcessMemory(handle, (void*)addrTotal, &totalLocations, sizeof(totalLocations), 0);

				cout << dec << readTest << " out of " << totalLocations << endl;

				f.open("pagenumber.txt");
				f << readTest;
				f << ":";
				f << totalLocations;
				f << ":";
				f << bookTitle;
				f.close();
				Sleep(5000);
			}
		}
	}


	return 0;
}