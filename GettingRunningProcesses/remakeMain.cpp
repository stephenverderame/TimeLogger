#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib, "Version.lib")
#include <Windows.h>
#include <stdio.h>
#include <Psapi.h>
#include <memory>
#include <sstream>
#include <iomanip>
#include <wchar.h>
#include <unordered_map>
#include <fstream>
struct processInfo {
	unsigned int total, timer, active;
	processInfo() : total(0), timer(0), active(0) {};
	processInfo(unsigned int t, unsigned int ti, unsigned int a) : total(t), timer(ti), active(a) {};
};
const wchar_t * totalTimeKey = L"Total Runtime";
#define CWM_RESET_TIMER 0x700
#define CWM_UPDATE 0x800
#define CWM_REFRESHED 0x900
BOOL CALLBACK enumWindowsProc(HWND hwnd, LPARAM lParam) {
	DWORD pid;
	GetWindowThreadProcessId(hwnd, &pid);
	DWORD style = GetWindowLongPtr(hwnd, GWL_STYLE);
	void ** args = reinterpret_cast<void**>(lParam);
	std::vector<size_t> * v = reinterpret_cast<std::vector<size_t>*>(args[0]);
	HWND fgWnd = reinterpret_cast<HWND>(args[1]);
	auto processes = *reinterpret_cast<std::unordered_map<std::wstring, processInfo>*>(args[2]);
	clock_t lastCheck = *reinterpret_cast<clock_t*>(args[3]);
	if (style & WS_VISIBLE) {
		HANDLE handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
		WCHAR path[MAX_PATH];
		GetModuleFileNameExW((HMODULE)handle, 0, path, MAX_PATH);
		wchar_t * file = wcsrchr(path, L'\\') + 1;
		std::wstring wstr = path;
		std::hash<std::wstring> h;
		size_t hash = h(wstr);
		if (std::find(v->cbegin(), v->cend(), hash) == v->cend()) {
			if (processes.find(wstr) != processes.end()) {
				unsigned int addTime = (clock() - lastCheck) / CLOCKS_PER_SEC / 60;
				processes[wstr].total += addTime;
				processes[wstr].timer += addTime;
				if (fgWnd == hwnd) processes[wstr].active += addTime;
			}
			else
				processes.emplace(wstr, processInfo{ 1, 1, 1 });
			v->push_back(hash);
		}

		CloseHandle(handle);
	}
	return TRUE;
}
void saveData(unsigned int totalTime, const std::unordered_map<std::wstring, processInfo> & processes) {
//	printf("Save\n");
	std::wofstream out;
	out.open("data.txt");
	out << totalTimeKey << ':' << totalTime << ";0-0\n";
	for (auto it : processes) {
		out << it.first << ':' << it.second.total << ';' << it.second.timer << '-' << it.second.active << '\n';
	}
	out.close();
}
void loadData(unsigned int & totalTime, std::unordered_map<std::wstring, processInfo> & processes) {
	struct stat buffer;
	if (stat("data.txt", &buffer) == 0) {
		std::wifstream in("data.txt");
		std::wstring str;
		std::getline(in, str);
		totalTime = std::stoi(str.substr(str.find_last_of(L':') + 1, str.find_last_of(L';')));
		while (std::getline(in, str)) {
			std::wstring process = str.substr(0, str.find_last_of(L':'));
			unsigned int time = std::stoi(str.substr(str.find_last_of(L':') + 1, str.find_last_of(L';')));
			unsigned int rTime = std::stoi(str.substr(str.find_last_of(L';') + 1, str.find_last_of(L'-')));
			unsigned int aTime = std::stoi(str.substr(str.find_last_of(L'-') + 1));
			processes[process] = { time, rTime, aTime };
		}
	}
}
LRESULT CALLBACK eventHandler(HWND wnd, UINT msg, WPARAM w, LPARAM l) {
	void ** data = reinterpret_cast<void**>(GetWindowLongPtr(wnd, GWL_USERDATA));
	if (data != nullptr) {
		auto totalTime = *reinterpret_cast<unsigned int *>(data[0]);
		auto processes = reinterpret_cast<std::unordered_map<std::wstring, processInfo>*>(data[1]);
		if (msg == WM_POWERBROADCAST || msg == WM_QUERYENDSESSION) {
			saveData(totalTime, *processes);
			return TRUE;
		}
		else if (msg == CWM_RESET_TIMER) {
			char * process = reinterpret_cast<char*>(w);
			//		printf("Resetting %s\n", process);
			std::wstringstream ss;
			ss << process;
			for (auto it = processes->begin(); it != processes->end(); ++it) {
				if (it->first.find(ss.str()) != std::string::npos) {
					//				wprintf(L"Clearing: %s\n", it->first.c_str());
					processes->at(it->first).timer = 0;
					break;
				}
			}
			VirtualFreeEx(GetModuleHandle(NULL), process, l, MEM_RELEASE);
			return TRUE;
		}
		else if (msg == CWM_UPDATE) {
//			printf("update!");
			saveData(totalTime, *processes);
			HWND wnd = FindWindow(NULL, "Time Logger");
			if (wnd) {
				PostMessage(wnd, CWM_REFRESHED, 0, 0);
			}
//			else printf("Can't send update!\n");
			return TRUE;
		}
	}
//	else printf("Data is null!\n");
	return DefWindowProc(wnd, msg, w, l);
}
int __stdcall WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
//int main() {
	WNDCLASSEX wind;
	ZeroMemory(&wind, sizeof(wind));
	wind.cbSize = sizeof(WNDCLASSEX);
	wind.lpszClassName = "eventHandlerWindow";
	wind.lpfnWndProc = eventHandler;
	RegisterClassEx(&wind);
	HWND wnd = CreateWindow(wind.lpszClassName, "handler", WS_OVERLAPPED, 200, 200, 0, 0, NULL, NULL, NULL, NULL);
	const UINT timerId = 8302;
	clock_t lastCheck = clock();
	unsigned int totalTime = 0;
	std::unordered_map<std::wstring, processInfo> processes;
	void * wndArgs[2] = { &totalTime, &processes };
	if(SetWindowLongPtr(wnd, GWL_USERDATA, reinterpret_cast<LONG_PTR>(wndArgs)) == 0) printf("Failed to set wnd data %d\n", GetLastError());
	loadData(totalTime, processes);
	while (true) {
		SetTimer(wnd, timerId, 60000, NULL);
		WaitMessage();
		MSG msg;
		while (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE)) {
			TranslateMessage(&msg);
			if (msg.message == WM_TIMER || msg.message == WM_QUERYENDSESSION || msg.message == WM_POWERBROADCAST || msg.message == CWM_UPDATE) {
				HWND fg = GetForegroundWindow();
				std::vector<size_t> v;
				void * args[4] = { &v, fg, &processes, &totalTime };
				EnumWindows(enumWindowsProc, reinterpret_cast<LPARAM>(args));
				totalTime += (clock() - lastCheck) / CLOCKS_PER_SEC / 60;
				lastCheck = clock();
//				printf("Updating");
			}
			DispatchMessage(&msg);
		}
//		printf("Loop\n");
	}
	DestroyWindow(wnd);

}