
#include "MainHelpers.h"
#include <vector>
#include <string>
#include <stdio.h>
#include "EncrypterCLI/EncrypterCLI.h"

#define NT_SUCCESS(Status)          (((NTSTATUS)(Status)) >= 0)

#define STATUS_UNSUCCESSFUL         ((NTSTATUS)0xC0000001L)


//const BYTE rgbPlaintext;
//{ 'P', 'A', 'S', 'S', 'W', 'O', 'R', 'D' };


static const BYTE rgbIV[] =
{
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};



using std::string;
using std::vector;

// Declare Window Handers
HWND a, b;
HWND upldBtn;

void AddControls(HWND hwnd)
{
	HWND hwnda;
		
	//Creates a label
	hwnda = CreateWindowW(L"Static", L"Confidential Computing with Machine Learning using Intel SGX.", WS_VISIBLE | WS_CHILD | SS_CENTER, 60, 100, 300, 309, hwnd, (HMENU)TITLE, NULL, NULL);
	HFONT hFont = CreateFont(30, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
	SendMessage(hwnda, WM_SETFONT, WPARAM(hFont), TRUE);
	
	/*CreateWindowW(L"Edit", L"...", WS_VISIBLE | WS_CHILD, 200, 152, 100, 50, hwnd, NULL, NULL, NULL);
	CreateWindowW(L"Button", L"Click", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 200, 190, 100, 30, hwnd, NULL, NULL, NULL);*/
}

void drawRect(HWND hwnd, HDC hdc)
{
	RECT parentRect;
	long width, height;
	if (GetWindowRect(hwnd, &parentRect))
	{
		width = parentRect.right - parentRect.left;
		height = parentRect.bottom - parentRect.top;
	}

	//White BG_ This helped in debugging the window resize render error
	RECT rect = { 0, 0, width,height };
	HBRUSH brush = CreateSolidBrush(RGB(255,255,255));
	FillRect(hdc, &rect, brush);
	DeleteObject(brush);

	rect = { 0, 0, (width/2)-50,height };
	brush = CreateSolidBrush(RGB(50, 151, 151));
	FillRect(hdc, &rect, brush);
	DeleteObject(brush);


}

void addLoginContols(HWND hwnd)
{
	a = CreateWindowW(L"Static", L"Username : ", WS_VISIBLE | WS_CHILD | SS_CENTER, 572, 234, 70, 20, hwnd, NULL, NULL, NULL);
	CreateWindowW(L"Static", L"Password : ", WS_VISIBLE | WS_CHILD | SS_CENTER, 572, 308, 70, 20, hwnd, NULL, NULL, NULL);
	b = CreateWindowW(L"Edit", L"...", WS_VISIBLE | WS_CHILD | WS_BORDER, 718, 225, 200, 40, hwnd, NULL, NULL, NULL);
	CreateWindowW(L"Edit", L"...", WS_VISIBLE | WS_CHILD | WS_BORDER, 718, 300, 200, 40, hwnd, NULL, NULL, NULL);
	CreateWindowW(L"Button", L"Next", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 751, 381, 133, 36, hwnd, (HMENU)LOGIN_NEXT, NULL, NULL);
	ShowWindow(a, 1);

}

void removeLoginContols(HWND hwnd)
{	
	SetWindowText(b,TEXT("SHIT"));
	ShowWindow(a, 0);
}

void loadMainApp(HWND hwnd)
{
	AddControls(hwnd);
	addLoginContols(hwnd);
}





//-------------------------------------------------------------------------------------------------------
//Client Dashboard Helper functions

void addFileUploadControls(HWND hwnd)
{
	upldBtn = CreateWindowW(L"Button", L"Upload", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 751, 381, 133, 36, hwnd, (HMENU)OPEN_FILE_BUTTON, NULL, NULL);


}

void uploadFileProc(HWND hwnd)
{
	// This function take a file path and copies it to the Data directory

	OPENFILENAME ofn;

	//The file name selected will be stored here
	char file_name[100];

	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED |
		COINIT_DISABLE_OLE1DDE);
	if (SUCCEEDED(hr))
	{
		IFileOpenDialog *pFileOpen;

		// Create the FileOpenDialog object.
		hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
			IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

		if (SUCCEEDED(hr))
		{
			// Show the Open dialog box.
			hr = pFileOpen->Show(NULL);

			// Get the file name from the dialog box.
			if (SUCCEEDED(hr))
			{
				IShellItem *pItem;
				hr = pFileOpen->GetResult(&pItem);
				if (SUCCEEDED(hr))
				{
					PWSTR pszFilePath;
					hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

					// Display the file name to the user.
					if (SUCCEEDED(hr))
					{
						// concatenate the string with the Data directory path
						wchar_t saveTo[100] = L"Data/";
						wcscat_s(saveTo,100, L"text.txt");
												
						
						if (encrypter(pszFilePath, TEXT("crypt/CT.txt"), TEXT("crypt/keyOut.txt")))
							MessageBox(NULL, pszFilePath, L"File Enrypter", MB_OK);
						else
							MessageBox(NULL, L"Encryption Error", L"File Enrypter", MB_OK);


						//	//Call ReadFile with the buffer initialized above.
						//Copy the file to the Data Directory
						/*if(copyFileToDest(pszFilePath, (LPCWSTR)saveTo))
							MessageBox(NULL, pszFilePath, L"File Copy", MB_OK);
						else 
							MessageBox(NULL, L"File Open Error", L"File Copy", MB_OK);*/

						CoTaskMemFree(pszFilePath);
					}
					pItem->Release();
				}
			}
			pFileOpen->Release();
		}
		CoUninitialize();
	}
}

BOOL copyFileToDest(PWSTR pszFilePath, LPCWSTR saveTo) {
	if (CopyFile(pszFilePath, saveTo, TRUE))
		return 1;
	else
	{
		return 0;
	}

}

void listFilesDir(HWND hwnd, TCHAR folder)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	BYTE names[] = {"0"};
	int y = 108;

	//_tprintf(TEXT("Target file is %s\n"), argv[1]);
	hFind = FindFirstFile(L"Data/*.txt", &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		//printf("FindFirstFile failed (%d)\n", GetLastError());
		MessageBox(NULL, L"Error", L"File Copy", MB_OK);

		return;
	}
	else
	{

		while (FindNextFile(hFind, &FindFileData))
		{
			//MessageBox(NULL, FindFileData.cFileName, L"File Copy", MB_OK);
			CreateWindowW(L"Static", FindFileData.cFileName, WS_VISIBLE | WS_CHILD | SS_CENTER, 72, y, 200, 20, hwnd, NULL, NULL, NULL);
			y = y + 25;

		}

			FindClose(hFind);
		//MessageBox(NULL, (LPCWSTR)names[0], L"File Copy", MB_OK);

	}

}
