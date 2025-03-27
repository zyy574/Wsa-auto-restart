# 自动重启WSA Wsa-auto-restart

自动重启WSA以防过度提交内存卡死系统 auto restart wsa to avoid the impact of memory leak

## 编译相关 compile

如果你使用MSVC以外的编译器，加参数-lshlwapi -municode -Wl,--subsystem=windows -Wl,--entry=mainCRTStartup，使用MSVC则代码中添加

    #pragma comment(lib, "Shlwapi.lib")
    #pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"" )

for MSVC users add

    #pragma comment(lib, "Shlwapi.lib")
    #pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"" )

otherwise add -lshlwapi -municode to compile command.

## 使用相关 use

在exe同目录path.txt中填写wsaclient路径

fill the path of wsaclient in path.txt