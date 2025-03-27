#include <windows.h>
#include <iostream>
#include <thread>
#include <Psapi.h>
#include <shlwapi.h>
#include <algorithm>
#include <deque>
#include <fstream>
//#define DEBUG
char start[210];
char kill[210];
bool killing=0;
bool emerg_killing=0;
struct EnumData {
	std::wstring targetProcessName;
	HWND hwndResult;
};
char path[200];
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
	EnumData* data = reinterpret_cast<EnumData*>(lParam);
	DWORD processId;
	GetWindowThreadProcessId(hwnd, &processId);
	
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
	if (!hProcess) {
		return TRUE; // 无法打开进程，跳过
	}
	
	WCHAR exePath[MAX_PATH];
	DWORD size = MAX_PATH;
	if (!QueryFullProcessImageNameW(hProcess, 0, exePath, &size)) {
		CloseHandle(hProcess);
		return TRUE;
	}
	CloseHandle(hProcess);
	
	// 提取文件名并转换为小写
	WCHAR* fileName = PathFindFileNameW(exePath);
	std::wstring currentProcessName(fileName);
	std::transform(currentProcessName.begin(), currentProcessName.end(), currentProcessName.begin(), ::towlower);
	
	if (currentProcessName == data->targetProcessName) {
		data->hwndResult = hwnd;
		return FALSE; // 找到匹配，停止枚举
	}
	
	return TRUE; // 继续枚举
}

HWND FindWindowByProcessName(const std::string& processName) {
	// 将输入进程名转换为小写宽字符串
	int wideCharLen = MultiByteToWideChar(CP_ACP, 0, processName.c_str(), -1, nullptr, 0);
	if (wideCharLen == 0) {
		return nullptr;
	}
	std::wstring targetNameW;
	targetNameW.resize(wideCharLen);
	MultiByteToWideChar(CP_ACP, 0, processName.c_str(), -1, &targetNameW[0], wideCharLen);
	targetNameW.pop_back(); // 移除末尾的null字符
	
	// 转换为小写
	std::transform(targetNameW.begin(), targetNameW.end(), targetNameW.begin(), ::towlower);
	
	EnumData data;
	data.targetProcessName = targetNameW;
	data.hwndResult = nullptr;
	
	EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&data));
	
	return data.hwndResult;
}
int wsaforegrnd()
{
	HWND hwnd=FindWindowByProcessName("WsaClient.exe");	
	if(GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW){
		return true;
	}
	return false;
}
double GetCommittedMemoryPercentage()
{
	MEMORYSTATUSEX memStatus;
	memStatus.dwLength = sizeof(memStatus);
	
	if (!GlobalMemoryStatusEx(&memStatus))
	{
		DWORD error = GetLastError();
		std::cerr << "GlobalMemoryStatusEx failed with error code: " << error << std::endl;
		return -1.0;
	}
	
	ULONGLONG totalCommitLimit = memStatus.ullTotalPageFile;       // 总可提交内存
	ULONGLONG committedMemory = totalCommitLimit - memStatus.ullAvailPageFile;  // 已提交内存
	//std::cout<<totalCommitLimit<<std::endl<<committedMemory;
	if (totalCommitLimit == 0)
	{
		std::cerr << "Total commit limit is zero." << std::endl;
		return -1.0;
	}
	
	double percentage = (static_cast<double>(committedMemory) / totalCommitLimit) * 100.0;
	return percentage;
}
void emerg_restart(){
#ifdef DEBUG
	std::cout<<"\nemergency killing";
#endif
	system(kill);
	Sleep (15*1000);
	while(GetCommittedMemoryPercentage()>75){
		Sleep(5*60*1000);
	}
	emerg_killing=0;
#ifdef DEBUG
	std::cout<<"\nstarting";
#endif
	system(start);
}
void restart(){
	const int qsize=3;
	const int interval=30;
	std::deque<bool>q1;
	for(int i=0;i<qsize;++i){
		q1.push_back(1);
	}

	bool running=1;
	while(running==1){
		if(GetCommittedMemoryPercentage()<70){
			return;
		}
		q1.push_back(wsaforegrnd());
#ifdef DEBUG
		std::cout<<"\nwsacondition:"<<q1.back();
#endif
		q1.pop_front();
		running=0;
		for(int i=0;i<qsize;++i){
			running=running||q1[i];
		}
		Sleep (interval*1000);
	}
#ifdef DEBUG
	std::cout<<"\nkilling";
#endif
	system(kill);
	Sleep (5*1000);
	killing=0;
	while(GetCommittedMemoryPercentage()>75){
		Sleep(5*60*1000);
	}
#ifdef DEBUG
	std::cout<<"\nstarting";
#endif
	system(start);
}
int wmain(){
	std::ifstream pathFile("path.txt");
	if (!pathFile) {
		std::cerr << "error reading path" << std::endl;
		return 0;
	}
	pathFile.getline(path,200);
	strcpy(kill,path);
	strcat(kill," /shutdown");
	strcpy(start,"start ");
	strcat(start,path);
#ifdef DEBUG
	std::cout<<path<<std::endl;
	std::cout<<"\n start script:"<<start;
	std::cout<<"\n kill script:"<<kill;
#endif
	while(1){
		double ratio = GetCommittedMemoryPercentage();
#ifdef DEBUG
		std::cout<<"\ncommitted ratio:"<<ratio;
#endif
		if(ratio==-1){
			std::cerr<<"\n failure getting memory info";
			break;
		}
		else if(ratio>95){
			if(!emerg_killing){
				std::thread worker1(emerg_restart);
				emerg_killing=1;
				worker1.detach();
			}
			else{
				Sleep (60000*10);
				continue;
			}
		}
		else if(ratio>75){
			if(!killing){
				std::thread worker2(restart);
				killing=1;
				worker2.detach();
			}
			else{
				Sleep (60000*30);
				continue;
			}
		}
		Sleep (60000*5);
	}
	return 0;
}
