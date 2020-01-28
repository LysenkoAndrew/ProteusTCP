// ProteusClient.cpp: ���������� ����� ����� ��� ����������� ����������.
//

#include "stdafx.h"
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <set>
#include "../ProteusServer/WinSockClasses.h"
#include "../ProteusServer/Utils.h"

std::unique_ptr<WinSockWrapper> client = nullptr;

BOOL WINAPI ConsoleHandler(DWORD dwCtrlType)
{
    if ((CTRL_CLOSE_EVENT == dwCtrlType) || (CTRL_SHUTDOWN_EVENT == dwCtrlType))
    {
        if (client && client->IsOpen())
        {
            client->Close();
            client.release();
        }
        return TRUE;
    }
    return FALSE;
}

int _tmain(int argc, _TCHAR* argv[])
{
    setlocale(LC_ALL, "Russian");
    bool bParamsOk = false;

    // ��� ����������, ����� � ����� �����
    std::tuple<std::string, std::string, unsigned short> params;
    if (7 == argc)
    {
        std::vector<std::string> cmdParams = ParseCmdLine(argc, argv);
        if (cmdParams.size() == 6)
        {
            params = GetConnectOpt(false, cmdParams);
            bParamsOk = (!std::get<0>(params).empty() && !std::get<1>(params).empty() && std::get<2>(params));
        }
    }

    if (bParamsOk)
    {
        const auto& connectType = std::get<0>(params);
        const auto& ipAddress = std::get<1>(params);
        const auto& nPort = std::get<2>(params);

        SetConsoleCtrlHandler(ConsoleHandler, TRUE);
        client = std::make_unique<WinSockWrapper>();
        if (client.get())
        {
            if (client->CreateWinSock(false, connectType, ipAddress))
            {
                if (client->Open(nPort))
                {
                    std::string msg = connectType + "-������ � ������� " + ipAddress + " ������ �� ����� " + std::to_string(nPort);
                    CThreadSafeCout{} << msg << std::endl;
                    while (true)
                    {
                        std::cout << "������� ���� ��������� (��� \"exit\" ��� ������ �� ���������):" << std::endl;
                        std::string str;
                        std::getline(std::cin, str);
                        if (0 == str.compare("exit"))
                        {
                            client->Close();
                            break;
                        }
                        else
                        {
                            if (!client->Send(str))
                            {
                                std::cout << "������ �������� ���������\n";
                                if (!client->IsOpen())
                                {
                                    std::cout << "������ ����������\n";
                                    std::cout << "�������� �������\n";
                                    client->Close();
                                    break;
                                }
                            }
                        }
                    }
                }
                else
                {
                    std::cerr << "������ ��� ��������� � ��������!\n";
                }
            }
        }
    }
    else
    {
        std::cout << "�������� ��������� ���������!\n";
        std::cout << "������� ��������� ������� �������: -t <tcp>(<udp>) -p <����� �����> -a <ip-����� �������>";
        std::cin.get();
    }
    return 0;
}

