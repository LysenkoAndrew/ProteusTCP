// ProteusServer.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
#include "WinSockBase.h"
#include "WinSockClasses.h"
#include "Utils.h"
#include <iostream>
#include <memory>
#include <set>

std::unique_ptr<WinSockWrapper> server = nullptr;

void closeServer()
{
    if (server && server->IsOpen())
    {
        server->Close();
        server.release();
    }
}


BOOL WINAPI ConsoleHandler(DWORD dwCtrlType)
{
    if ((CTRL_CLOSE_EVENT == dwCtrlType) || (CTRL_SHUTDOWN_EVENT == dwCtrlType))
    {
        closeServer();
        return TRUE;
    }
    return FALSE;
}


int _tmain(int argc, _TCHAR* argv[])
{
    setlocale(LC_ALL, "Russian");
    bool bParamsOk = false;

    // Тип соединения, адрес и номер порта
    std::tuple<std::string, std::string, unsigned short> params;
    if (5 == argc)
    {
        std::vector<std::string> cmdParams = ParseCmdLine(argc, argv);
        if (cmdParams.size() == 4)
        {
            params = GetConnectOpt(true, cmdParams);
            bParamsOk = (!std::get<0>(params).empty() && std::get<2>(params));
        }
    }

    if (bParamsOk)
    {
        const auto& connectType = std::get<0>(params);
        const auto& ipAddress = std::get<1>(params);
        const auto& nPort = std::get<2>(params);

        SetConsoleCtrlHandler(ConsoleHandler, TRUE);
        server = std::make_unique<WinSockWrapper>();
        if (server.get())
        {
            if (server->CreateWinSock(true, connectType, ipAddress))
            {
                if (server->Open(nPort))
                {
                    std::string msg = connectType + "-сервер с адресом " + ipAddress + " открыт на порту " + std::to_string(nPort);
                    CThreadSafeCout{} << msg << std::endl;
                    while (true)
                    {
                        CThreadSafeCout{} << "Введите \"exit\" для выхода из программы" << std::endl;
                        std::string str;
                        std::getline(std::cin, str);
                        if (0 == str.compare("exit"))
                        {
                            closeServer();
                            return 0;
                        }
                    }
                }
                else
                {
                    std::cout << "Не удалось открыть сервер.\n";
                }
            }
        }
    }
    else
    {
        std::cout << "Неверные параметры программы!\n";
        std::cout << "Введите параметры окрытия сервера: -t <tcp>(<udp>) -p <номер порта>";
        std::cin.get();
    }
    return 0;
}

