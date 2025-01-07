#include "PluginDefinition.h"

#include <stdlib.h>
#include <time.h>
#include <shlwapi.h>
#include "DockingFeature/GoToLineDlg.h"

#include <string>
#include <sstream>
#include <vector>
#include <iostream>
#include <fstream>
#include <chrono>

#include <locale>
#include <codecvt>

#include <winsock.h>

#pragma comment(lib, "ws2_32.lib")


const TCHAR sectionName[] = TEXT("Insert Extesion");
const TCHAR keyName[] = TEXT("doCloseTag");
const TCHAR configFileName[] = TEXT("pluginDemo.ini");

DemoDlg _goToLine;

#ifdef UNICODE 
	#define generic_itoa _itow
#else
	#define generic_itoa itoa
#endif

FuncItem funcItem[nbFunc];

//
// The data of Notepad++ that you can use in your plugin commands
//
NppData nppData;


TCHAR iniFilePath[MAX_PATH];
bool doCloseTag = false;

#define DOCKABLE_DEMO_INDEX 15

//
// Initialize your plugin data here
// It will be called while plugin loading   
void pluginInit(HANDLE hModule)
{
	// Initialize dockable demo dialog
	_goToLine.init((HINSTANCE)hModule, NULL);
}

//
// Here you can do the clean up, save the parameters (if any) for the next session
//
void pluginCleanUp()
{
	::WritePrivateProfileString(sectionName, keyName, doCloseTag?TEXT("1"):TEXT("0"), iniFilePath);
}

//
// Initialization of your plugin commands
// You should fill your plugins commands here
void commandMenuInit()
{
	//
	// Firstly we get the parameters from your plugin config file (if any)
	//

	// get path of plugin configuration
	::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM)iniFilePath);

	// if config path doesn't exist, we create it
	if (PathFileExists(iniFilePath) == FALSE)
	{
		::CreateDirectory(iniFilePath, NULL);
	}

	// make your plugin config file full file path name
	PathAppend(iniFilePath, configFileName);

	// get the parameter value from plugin config
	doCloseTag = (::GetPrivateProfileInt(sectionName, keyName, 0, iniFilePath) != 0);


    //--------------------------------------------//
    //-- STEP 3. CUSTOMIZE YOUR PLUGIN COMMANDS --//
    //--------------------------------------------//
    // with function :
    // setCommand(int index,                      // zero based number to indicate the order of command
    //            TCHAR *commandName,             // the command name that you want to see in plugin menu
    //            PFUNCPLUGINCMD functionPointer, // the symbol of function (function pointer) associated with this command. The body should be defined below. See Step 4.
    //            ShortcutKey *shortcut,          // optional. Define a shortcut to trigger this command
    //            bool check0nInit                // optional. Make this menu item be checked visually
    //            );


	ShortcutKey* itraceKey = new ShortcutKey;
	itraceKey->_isAlt = true;
	itraceKey->_isCtrl = false;
	itraceKey->_isShift = false;
	itraceKey->_key = 0x49; //VK_I
	setCommand(0, TEXT("Connect to iTrace-Core"), connectToCore, itraceKey, false);
}


//
// Here you can do the clean up (especially for the shortcut)
//
void commandMenuCleanUp()
{
	// Don't forget to deallocate your shortcut here
	delete funcItem[0]._pShKey;
}

//----------------------------------------------//
//-- STEP 4. DEFINE YOUR ASSOCIATED FUNCTIONS --//
//----------------------------------------------//

void trimTrailingWhitespace(std::string& str) {
	size_t end = str.find_last_not_of(" \t\n\r\f\v"); // Whitespace characters
	if (end != std::string::npos) {
		str.erase(end + 1);  // Erase from end+1 to the end of the string
	}
	else {
		str.clear();  // If no non-whitespace characters were found, clear the string
	}
}

std::wstring stringToWString(const std::string& str) {
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
	std::wstring wstr(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], size_needed);
	return wstr;
}

std::vector<std::string> split(const std::string& text, char sep) {
	std::vector<std::string> result;
	std::stringstream ss(text);
	std::string item;
	while (std::getline(ss, item, sep)) {
		result.push_back(item);
	}
	return result;
}

void getLineAndColumnFromXY(HWND curScintilla, int x, int y, int& line, int& column) {
	POINT pt = { x,y };
	ScreenToClient(curScintilla, &pt);
	// Step 1: Convert (x, y) coordinates to a document position
	int pos = static_cast<int>(::SendMessage(curScintilla, SCI_POSITIONFROMPOINTCLOSE, pt.x, pt.y));
	
	// Check if the position is valid
	if (pos == -1) {
		line = -1;
		column = -1;
		return; // (x, y) is outside the text area
	}

	// Step 2: Get the line number from the character position
	line = static_cast<int>(::SendMessage(curScintilla, SCI_LINEFROMPOSITION, pos, 0));

	// Step 3: Get the column number by calculating the difference from the line start
	int lineStartPos = static_cast<int>(::SendMessage(curScintilla, SCI_POSITIONFROMLINE, line, 0));
	column = pos - lineStartPos;

	++line;
	++column;
}

static DWORD WINAPI connectSocket(void*) {
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	// Get the current scintilla
	int which = -1;
	::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
	if (which == -1)
		return FALSE;
	HWND curScintilla = (which == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;


	SOCKET ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ConnectSocket == INVALID_SOCKET) {
		std::wstring other_message = L"Could not start Websocket Service";
		::SendMessage(nppData._nppHandle, NPPM_SETSTATUSBAR, 0, (LPARAM)other_message.c_str());
		WSACleanup();
		return FALSE;
	}
	sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_port = htons(8008);
	address.sin_addr.s_addr = inet_addr("127.0.0.1");

	

	if (connect(ConnectSocket, (sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
		std::wstring failed_message = L"Could not connect to iTrace-Core - is it running?";
		::SendMessage(nppData._nppHandle, NPPM_SETSTATUSBAR, 0, (LPARAM)failed_message.c_str());
		closesocket(ConnectSocket);
		WSACleanup();
		return FALSE;
	}

	std::ofstream file;

	std::wstring connected_message = L"Connected to iTrace-Core";
	::SendMessage(nppData._nppHandle, NPPM_SETSTATUSBAR, 0, (LPARAM)connected_message.c_str());

	while (true) {
		char recvData[512];
		int bytesReceived = recv(ConnectSocket, recvData, sizeof(recvData),0);
		if (bytesReceived > 0) {
			recvData[bytesReceived] = '\0';

			std::string message(recvData);
			trimTrailingWhitespace(message);
			std::vector<std::string> tokens = split(message,',');

			if(tokens[0] == "session_start") {
				auto now = std::chrono::system_clock::now(); 

				std::string output = tokens[3] + "\\itrace_npp-" + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()) + ".xml";

				file = std::ofstream(output);

				if (!file.is_open()) {
					std::wstring bad = stringToWString(std::string("Could not open file: " + output));
					::SendMessage(nppData._nppHandle, NPPM_SETSTATUSBAR, 0, (LPARAM)bad.c_str());
				}

				file << "<?xml version=\"1.0\"?>" << std::endl;
				file << "<itrace_plugin session_id=\"" << tokens[1] << "\">" << std::endl;
				file << "    <environment screen_width=\"" << GetSystemMetrics(SM_CXSCREEN) << "\" screen_height=\"" << GetSystemMetrics(SM_CYSCREEN) << "\" plugin_type=\"NOTEPAD++\"/>" << std::endl;
				file << "    <gazes>" << std::endl;

			}
			else if (tokens[0] == "session_end") {
				file << "    <gazes/>" << std::endl;
				file << "<itrace_plugin/>" << std::endl;
				file.close();
				std::wstring end_message = L"Session has Ended";
				::SendMessage(nppData._nppHandle, NPPM_SETSTATUSBAR, 0, (LPARAM)end_message.c_str());
			}
			else if (tokens[0] == "gaze") {

				int line = -1;
				int col = -1;

				getLineAndColumnFromXY(curScintilla, std::stoi(tokens[2]), std::stoi(tokens[3]), line, col);
				auto now = std::chrono::system_clock::now();
				long long time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

				char filename[MAX_PATH];
				::SendMessage(nppData._nppHandle, NPPM_GETFILENAME, 0, (LPARAM)filename);

				std::string ext = split(std::string(filename), '.').back();

				char path[MAX_PATH];
				::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, 0, (LPARAM)path);

				int lineHeight = static_cast<int>(::SendMessage(curScintilla, SCI_TEXTHEIGHT, 0, 0));
				int fontSize = static_cast<int>(::SendMessage(curScintilla, SCI_STYLEGETSIZE, 0, 0));

				file << "        <response event_id=\"" << tokens[1] << "\"" <<
									  "plugin_time=\"" << time << "\"" <<
									  "x=\"" << tokens[2] << "\"" <<
									  "y=\"" << tokens[3] << "\"" <<
									  "gaze_target=\"" << filename << "\"" <<
									  "gaze_target_type=\"" << ext << "\"" <<
								  	  "source_file_path=\"" << path << "\"" <<
								  	  "source_file_line=\"" << line << "\"" <<
									  "source_file_col=\"" << col << "\"" <<
									  "editor_line_height=\"" << lineHeight << "\"" <<
									  "editor_font_height=\"" << fontSize << "\"" <<
									  "editor_line_base_x=\"\"" <<
									  "editor_line_base_y=\"\"" << 
									  "/>" << std::endl;
				std::string current = tokens[2] + "," + tokens[3] + "->" + std::to_string(line) + "," + std::to_string(col);
				std::wstring y = stringToWString(current);
				::SendMessage(nppData._nppHandle, NPPM_SETSTATUSBAR, 0, (LPARAM)y.c_str());
			}


		}

	}

	return TRUE;
}

void connectToCore() {
	HANDLE hThread = ::CreateThread(NULL, 0, connectSocket, 0, 0, NULL);
	::CloseHandle(hThread);
}


//
// This function help you to initialize your plugin commands
//
bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk, bool check0nInit) 
{
    if (index >= nbFunc)
        return false;

    if (!pFunc)
        return false;

    lstrcpy(funcItem[index]._itemName, cmdName);
    funcItem[index]._pFunc = pFunc;
    funcItem[index]._init2Check = check0nInit;
    funcItem[index]._pShKey = sk;

    return true;
}
