#pragma comment(lib, "Version.lib")
#include <window.h>
#include <Psapi.h>
#include <BasicList.h>
#include <fstream>
#include <wchar.h>
#include <memory>
#include <iomanip>
#include <sstream>
#include <guiMain.h>
#include <Page.h>
#include <string>
#include <unordered_map>
#define CWM_RESET_TIMER 0x700
#define CWM_UPDATE 0x800
#define CWM_REFRESHED 0x900
struct lang {
	WORD language;
	WORD codePage;
};
unsigned int TOTAL_TIME = 0;
std::unordered_map<std::string, unsigned int> relTimes;
std::unordered_map<std::string, unsigned int> activeTimes;
std::string selectedItem;
void addItem(BasicList * list, std::wstring path, unsigned int time, unsigned int rTime, unsigned int aTime) {
	DWORD size = GetFileVersionInfoSizeW(path.c_str(), NULL);
	auto buffer = std::make_unique<char[]>(size);
	GetFileVersionInfoW(path.c_str(), NULL, size, buffer.get());

	lang * fileLang = nullptr;
	unsigned int s;
	VerQueryValueW(buffer.get(), L"\\VarFileInfo\\Translation", reinterpret_cast<void**>(&fileLang), &s);
	if (!fileLang) {
		path = path.substr(path.find_last_of('\\') + 1);
		int size = WideCharToMultiByte(CP_UTF8, 0, (wchar_t*)path.c_str(), path.size(), NULL, 0, NULL, NULL);
		auto name = std::make_unique<char[]>(size);
		WideCharToMultiByte(CP_UTF8, 0, (wchar_t*)path.c_str(), path.size(), name.get(), size, NULL, NULL);
		auto cell = new ListCell(new Text(26, 0, size * 10, name.get()), nullptr);
		cell->userData = reinterpret_cast<void*>(time);
		list->addCell(cell);
		relTimes[name.get()] = rTime;
		activeTimes[name.get()] = aTime;
		return;
	}
	std::wstringstream query;
	query << "\\StringFileInfo\\" << std::setw(4) << std::setfill(L'0') << std::hex << fileLang->language << std::setw(4) << std::setfill(L'0') << std::hex << fileLang->codePage << "\\" << "FileDescription";
	void * fileDesc = nullptr;
	unsigned int fileDescLen = 0;
	VerQueryValueW(buffer.get(), query.str().c_str(), &fileDesc, &fileDescLen);

	SHFILEINFOW info;
	SHGetFileInfoW(path.c_str(), FILE_ATTRIBUTE_NORMAL, &info, sizeof(info), SHGFI_ICON | SHGFI_USEFILEATTRIBUTES | SHGFI_SMALLICON);
	ICONINFO iconInfo;
	GetIconInfo(info.hIcon, &iconInfo);
	BITMAP iconData;
	GetObject(iconInfo.hbmColor, sizeof(iconData), &iconData);
	Image * img = new Image(iconInfo.hbmColor);
	if (fileDesc) {
		int size = WideCharToMultiByte(CP_UTF8, 0, (wchar_t*)fileDesc, fileDescLen, NULL, 0, NULL, NULL);
		auto name = std::make_unique<char[]>(size);
		WideCharToMultiByte(CP_UTF8, 0, (wchar_t*)fileDesc, fileDescLen, name.get(), size, NULL, NULL);
		auto cell = new ListCell(new Text(iconData.bmWidth + 10, 0, size * 10, name.get()), img);
		cell->userData = reinterpret_cast<void*>(time);
		list->addCell(cell);
		relTimes[name.get()] = rTime;
		activeTimes[name.get()] = aTime;
	}
	else {
		std::wstring exe = path.substr(path.find_last_of(L'\\') + 1);
		int size = WideCharToMultiByte(CP_UTF8, 0, exe.c_str(), exe.size(), NULL, 0, NULL, NULL);
		auto name = std::make_unique<char[]>(size);
		WideCharToMultiByte(CP_UTF8, 0, exe.c_str(), exe.size(), name.get(), size, NULL, NULL);
		auto cell = new ListCell(new Text(iconData.bmWidth + 10, 0, size * 10, name.get()), img);
		cell->userData = reinterpret_cast<void*>(time);
		list->addCell(cell);
		relTimes[name.get()] = rTime;
		activeTimes[name.get()] = aTime;
	}
}
void onSearch(void * data) {
	void ** d = (void**)data;
	BasicList * list = reinterpret_cast<BasicList*>(d[0]);
	int * lastSearch = reinterpret_cast<int*>(d[1]);
	gui::TextField * text = reinterpret_cast<gui::TextField*>(d[2]);
	std::string query = text->getText();
	for (int i = 0; i < query.size(); ++i)
		query[i] = tolower(query[i]);
	auto cells = list->getCells();
	for (int i = 0; i < cells->size(); ++i) {
		int index = (i + *lastSearch) % cells->size();
		std::string name = cells->at(index)->txt->msg;
		for (int j = 0; j < name.size(); ++j)
			name[j] = tolower(name[j]);
		if (name.find(query) != std::string::npos) {
			list->scrollTo(index);
			list->selectCell(index);
			*lastSearch = index + 1;
			return;
		}
	}
	MessageBox(NULL, "Search Query Not Found", "Time Logger", MB_OK | MB_ICONEXCLAMATION);
}
void selectCallback(int index, BasicList * list, void * data) {
	void ** d = (void**)data;
	gui::Label * label = (gui::Label*)d[0];
	gui::Label * name = (gui::Label*)d[1];
	gui::Label * percent = (gui::Label*)d[2];
	gui::Progressbar * progress = (gui::Progressbar*)d[3];
	gui::Label * relative = (gui::Label*)d[4];
	gui::Label * active = (gui::Label*)d[5];
	ListCell * cell = list->getCells()->at(index);
	unsigned int time = reinterpret_cast<unsigned int>(cell->userData);
	std::stringstream ss;
	ss << "Total Time: " << time / 60 << ":" << std::setw(2) << std::setfill('0') << time % 60;
	label->setText(const_cast<char*>(ss.str().c_str()));
	name->setText(const_cast<char*>(cell->txt->msg.c_str()));
	ss.str("");
	ss << "Relative Time: " << relTimes[cell->txt->msg] / 60 << ":" << std::setw(2) << std::setfill('0') << relTimes[cell->txt->msg] % 60;
	relative->setText(const_cast<char*>(ss.str().c_str()));
	ss.str("");
	ss << std::fixed << std::setprecision(1) << (double)time / TOTAL_TIME * 100.0 << '%';
	progress->setRange(0, TOTAL_TIME);
	progress->setPos(time);
	percent->setText(const_cast<char*>(ss.str().c_str()));
	ss.str("");
	ss << "Active Time: " << activeTimes[cell->txt->msg] / 60 << ":" << std::setw(2) << std::setfill('0') << activeTimes[cell->txt->msg] % 60;
	ss << " (" << std::fixed << std::setprecision(1) << (double)activeTimes[cell->txt->msg] / TOTAL_TIME * 100.0 << "%)";
	active->setText(const_cast<char*>(ss.str().c_str()));
	selectedItem = cell->txt->msg;

}
void loadData(BasicList * display) {
	struct stat fs;
	if (stat("data.txt", &fs) == 0) {
		printf("Loading data\n");
		std::wifstream in("data.txt");
		std::wstring str;
		while (std::getline(in, str)) {
			std::wstring path = str.substr(0, str.find_last_of(L':'));
			std::wstring totalTime = str.substr(str.find_last_of(L':') + 1, str.find_last_of(L';'));
			std::wstring relTime = str.substr(str.find_last_of(L';') + 1, str.find_first_of(L'-'));
			std::wstring actTime = str.substr(str.find_last_of(L'-') + 1);
			unsigned int time = std::stoi(totalTime);
			unsigned int rTime = std::stoi(relTime);
			unsigned int aTime = std::stoi(actTime);
			addItem(display, path, time, rTime, aTime);
			if (path == L"Total Runtime") {
				TOTAL_TIME = time;
			}
		}
	}
	else MessageBox(NULL, "No data file found", "Time Logger", MB_ICONERROR | MB_OK);
}
void update() {
	HWND wind = FindWindow("eventHandlerWindow", "handler");
	if (!wind) printf("Cannot send!\n");
	PostMessage(wind, CWM_UPDATE, 0, 0);
}
void sendMSG() {
	HWND wind = FindWindow("eventHandlerWindow", "handler");
	if (wind) {
		DWORD pid;
		GetWindowThreadProcessId(wind, &pid);			
		HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
		void * addr = VirtualAllocEx(process, 0, selectedItem.size() + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		WriteProcessMemory(process, addr, selectedItem.c_str(), selectedItem.size() + 1, 0);
		PostMessage(wind, CWM_RESET_TIMER, (WPARAM)addr, selectedItem.size() + 1);
		CloseHandle(process);
	}
}
int main() {
	Window wind("Time Logger", CS_DBLCLKS);
	gui::GUI::bindWindow(wind);
	wind.use();
	wind.createWindow(0, 0, 800, 800, WS_CLIPCHILDREN);	
	wind.show(true);
	wind.addEventListener(new EventListener([](EventParams p) {}, WM_ERASEBKGND)); //prevent default erase behavior --> canvas is controlling drawing
	BasicList * list = new BasicList(30, 100, 700, 500);
	list->setBorder(RGB(0, 0, 0), 2, PS_SOLID);
	list->setSpacing(30);
	wind.getCanvas()->addDrawable(list);
	wind.getCanvas()->enableDoubleBuffering(true);
	std::wstring str;
	gui::Label * info = new gui::Label("data", 20, 700, 200, 20, "", WS_CLIPCHILDREN, wind);
	gui::Label * id = new gui::Label("app", 20, 630, 600, 20, "", WS_CLIPCHILDREN, wind);
	gui::Label * relInfo = new gui::Label("rel", 20, 720, 200, 20, "", WS_CLIPCHILDREN, wind);
	gui::Label * activeInfo = new gui::Label("active", 550, 650, 300, 20, "", WS_CLIPCHILDREN, wind);
	gui::Progressbar * percent = new gui::Progressbar("percentTotal", 600, 700, 100, 20, 0, 100, WS_CLIPCHILDREN, wind);
	gui::Label * percentLabel = new gui::Label("percentLabel", 600, 730, 100, 20, "", WS_CLIPCHILDREN, wind);
	gui::TextField * entry = new gui::TextField("searchBox", 20, 20, 300, 20, WS_CLIPCHILDREN, wind);
	gui::Button * btn = new gui::Button("searchBtn", 340, 20, 70, 20, "Search", nullptr, BS_DEFPUSHBUTTON | WS_CLIPCHILDREN, wind);
	gui::Button * reset = new gui::Button("rBtn", 20, 670, 120, 30, "Reset Relative", sendMSG, WS_CLIPCHILDREN, wind);
	gui::Button * refresh = new gui::Button("refreshBtn", 20, 60, 120, 20, "Refresh", update, WS_CLIPCHILDREN, wind);
	gui::Page * p1 = new gui::Page("p1", { entry, btn, info, id, percent, percentLabel, relInfo, reset, refresh, activeInfo });
	p1->setLayout(new AbsoluteLayout());
	gui::MainPage * mainPage = new gui::MainPage({ p1 });
	mainPage->navigateTo("p1");
	int persistData = 0;
	void * classes[3] = { list, &persistData, entry };
	btn->setClick2(onSearch, classes);
	void * labels[6] = { info, id, percentLabel, percent, relInfo, activeInfo };
	list->selectCallback(selectCallback, (void*)labels);
	loadData(list);
	wind.addEventListener(new EventListener([list, &wind](EventParams p) {
		list->clearCells();
		loadData(list);
		wind.invalidateDisplay(false);
	}, CWM_REFRESHED));
	while (true) {
		MSG msg;
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			if (msg.message == WM_QUIT) return 0;
			DispatchMessage(&msg);
			mainPage->handleMessage(&msg);
		}
	}
	delete list;
	delete mainPage;
}