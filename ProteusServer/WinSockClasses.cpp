#include "stdafx.h"
#include "WinSockClasses.h"
#include "Utils.h"
#include <exception>
#include <string>
#include <WinSock2.h>
#include <memory>

#pragma comment (lib, "Ws2_32.lib")

/**************************************************************

Класс для TCP - клиента

**************************************************************/
CWinSockTCPClient::CWinSockTCPClient(const std::string& address) : CParent(address, IPPROTO_TCP)
{
}
CWinSockTCPClient::~CWinSockTCPClient()
{
}

bool CWinSockTCPClient::Init(unsigned short port)
{
    if (InitWSLibrary() && (INVALID_SOCKET == m_socket))
    {
        addrinfo* result = nullptr;

        addrinfo hints;

        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        int iResult = getaddrinfo(m_address.c_str(), std::to_string(port).c_str(), &hints, &result);
        if (0 == iResult)
        {
            for (addrinfo* ptr = result; ptr != NULL; ptr = ptr->ai_next)
            {
                m_socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
                if (m_socket == INVALID_SOCKET)
                {
                    return false;
                }

                iResult = connect(m_socket, ptr->ai_addr, (int)ptr->ai_addrlen);
                if (iResult == SOCKET_ERROR)
                {
                    closesocket(m_socket);
                    m_socket = INVALID_SOCKET;
                    continue;
                }
                break;
            }
            freeaddrinfo(result);
            return (INVALID_SOCKET != m_socket);
        }
    }
    return false;
}


/**************************************************************

Класс для UDP - клиента

**************************************************************/
CWinSockUDPClient::CWinSockUDPClient(const std::string& address) : CParent(address, IPPROTO_UDP)
{
    m_bClose.store(false);
}
CWinSockUDPClient::~CWinSockUDPClient()
{
}

bool CWinSockUDPClient::Init(unsigned short port)
{
    if (InitWSLibrary() && (INVALID_SOCKET == m_socket))
    {
        m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (INVALID_SOCKET != m_socket)
        {
            sockaddr_in client_addr;
            int server_len = sizeof(m_serverAddr);
            ZeroMemory(&m_serverAddr, server_len);
            m_serverAddr.sin_family = AF_INET;
            m_serverAddr.sin_port = htons(port);
            inet_pton(AF_INET, m_address.c_str(), &m_serverAddr.sin_addr);

            int client_len = sizeof(client_addr);
            ZeroMemory(&client_addr, client_len);
            client_addr.sin_family = AF_INET;
            client_addr.sin_port = htons(0);
            client_addr.sin_addr.S_un.S_addr = INADDR_ANY;

            return (0 == bind(m_socket, (struct sockaddr*) & client_addr, client_len));
        }
    }
    return false;
}

void CWinSockUDPClient::ClientThread(SOCKET socket, HANDLE)
{
    unsigned char* buf = nullptr;
    const int len = 10000;
    std::unique_ptr<unsigned char[]> ptr = nullptr;
    try
    {
        ptr = std::make_unique<unsigned char[]>(len);
        buf = ptr.get();
    }
    catch (std::bad_alloc&)
    {

    }

    if (!buf)
    {
        return;
    }

    while (true)
    {
        // Событие закрытия
        if (m_bClose.load())
        {
            break;
        }

        sockaddr_in from = { 0 };
        int count = len;
        int iResult = Receive(socket, (char*)buf, count, &from);
        if (iResult > 0)
        {
            auto msgList = RecieveHandler(m_pPacketDisp.get(), socket, (char*)buf, count, &from);

            for (const auto& str : msgList)
            {
                if (m_cbRecieve)
                {
                    m_cbRecieve(str);
                }
                CThreadSafeCout{} << str << std::endl;
            }
        }
    }
}
/***********************************************/

void CWinSockTCPServer::CloseClients()
{
    std::lock_guard<std::mutex> lock{ m_clientsMapLock };
    for (auto& client : m_clients)
    {
        auto& socket = client.first;
        auto& hStop = std::get<0>(client.second);
        auto& thread = std::get<1>(client.second);

        if (thread.joinable())
        {
            if (hStop)
            {
                WSASetEvent(hStop);
            }
            thread.join();
            if (hStop)
            {
                WSACloseEvent(hStop);
            }
        }

        if (socket)
        {
            ::shutdown(socket, SD_SEND);
            ::closesocket(socket);
        }
    }
    m_clients.clear();
}

std::list<std::string> CWinSockTCPServer::RecieveHandler(CPacketDispather* /*pPacketDisp*/, SOCKET client, char* pMsg, int nLen, sockaddr_in* from)
{
    std::list<std::string> msgList;
    auto pClientOpt = m_clients.find(client);

    if (m_clients.end() != pClientOpt)
    {
        auto pPacketDisp = &std::get<2>(pClientOpt->second);
        msgList = CParent::RecieveHandler(pPacketDisp->get(), client, pMsg, nLen, from);
    }
    return msgList;
}

/**************************************************************

Класс для TCP - сервера

**************************************************************/

CWinSockTCPServer::CWinSockTCPServer() : CParent(IPPROTO_TCP)
{
}
CWinSockTCPServer::~CWinSockTCPServer()
{
    if (IsOpen())
    {
        Close();
    }
}

bool CWinSockTCPServer::Open(unsigned short port)
{
    if (!m_thread.native_handle())
    {
        if (Init(port))
        {
            m_port = port;
            try
            {
                m_thread = std::thread(&CWinSockTCPServer::AcceptThread, this);
            }
            catch (std::exception&)
            {
                // Как-то обрабатываем ошибку
                return false;
            }
        }
        return m_thread.native_handle() != nullptr;
    }
    return false;
}

void CWinSockTCPServer::Close()
{
    CloseClients();

    if (!m_bClose.load())
    {
        CWinSockTCPClient client("127.0.0.1");
        m_bClose.store(true);
        if (client.Open(m_port))
        {
            if (m_thread.joinable())
            {
                m_thread.join();
            }
        }
        m_bClose.store(false);
    }
}

bool CWinSockTCPServer::Init(unsigned short port)
{
    if (InitWSLibrary() && (INVALID_SOCKET == m_socket))
    {
        struct addrinfo* result = nullptr;
        struct addrinfo hints;

        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = AI_PASSIVE;

        int iResult = getaddrinfo(nullptr, std::to_string(port).c_str(), &hints, &result);
        if (0 == iResult)
        {
            m_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
            if (INVALID_SOCKET != m_socket)
            {
                iResult = bind(m_socket, result->ai_addr, (int)result->ai_addrlen);
                freeaddrinfo(result);
                if (SOCKET_ERROR != iResult)
                {
                    iResult = listen(m_socket, SOMAXCONN);
                    return SOCKET_ERROR != iResult;
                }
            }
        }

        if (result)
        {
            freeaddrinfo(result);
        }
        if (INVALID_SOCKET != m_socket)
        {
            closesocket(m_socket);
        }
    }
    return false;
}

void CWinSockTCPServer::AcceptThread()
{
    if (INVALID_SOCKET == m_socket)
    {
        return;
    }

    SOCKET AcceptSocket;
    while (true)
    {
        AcceptSocket = accept(m_socket, nullptr, nullptr);
        // Событие закрытия
        if (m_bClose.load())
        {
            break;
        }
        if (INVALID_SOCKET != AcceptSocket)
        {
            std::lock_guard<std::mutex> lock{ m_clientsMapLock };
            try
            {
                HANDLE hStopEvent = WSACreateEvent();
                if (hStopEvent)
                {
                    auto pParser = CreatePacketDispatcher();
                    std::thread thread(&CWinSockServer::ClientThread, this, AcceptSocket, hStopEvent);
                    m_clients.emplace(AcceptSocket, ClientThreadOpt(hStopEvent, std::move(thread), std::move(pParser)));
                }
            }
            catch (std::exception&)
            {

            }
        }
    }
    ::closesocket(m_socket);
}


/**************************************************************

Класс для UDP - сервера

**************************************************************/

CWinSockUDPServer::CWinSockUDPServer() : CParent(IPPROTO_UDP)
{
}
CWinSockUDPServer::~CWinSockUDPServer()
{
    if (IsOpen())
    {
        Close();
    }
}

bool CWinSockUDPServer::Open(unsigned short port)
{
    if (!m_thread.native_handle())
    {
        if (Init(port))
        {
            m_port = port;
            try
            {
                m_thread = std::thread(&CWinSockUDPServer::AcceptThread, this);
            }
            catch (std::exception&)
            {
                // Как-то обрабатываем ошибку
                return false;
            }
        }
        return m_thread.native_handle() != nullptr;
    }
    return false;
}

void CWinSockUDPServer::Close()
{
    if (!m_bClose.load())
    {
        m_bClose.store(true);

        if (m_thread.joinable())
        {
            m_thread.join();
        }
        m_bClose.store(false);
    }
}

bool CWinSockUDPServer::Init(unsigned short port)
{
    if (InitWSLibrary() && (INVALID_SOCKET == m_socket))
    {
        m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (m_socket != INVALID_SOCKET)
        {
            sockaddr_in RecvAddr;
            RecvAddr.sin_family = AF_INET;
            RecvAddr.sin_port = htons(port);
            RecvAddr.sin_addr.s_addr = htonl(INADDR_ANY);

            const int iResult = bind(m_socket, (SOCKADDR*)& RecvAddr, sizeof(RecvAddr));
            return iResult == 0;
        }
        if (INVALID_SOCKET != m_socket)
        {
            closesocket(m_socket);
        }
    }
    return false;
}

void CWinSockUDPServer::AcceptThread()
{
    if (INVALID_SOCKET == m_socket)
    {
        return;
    }

    unsigned char* buf = nullptr;
    const int len = 10000;
    std::unique_ptr<unsigned char[]> ptr = nullptr;
    try
    {
        ptr = std::make_unique<unsigned char[]>(len);
        buf = ptr.get();
    }
    catch (std::bad_alloc&)
    {

    }

    if (!buf)
    {
        return;
    }

    while (true)
    {
        // Событие закрытия
        if (m_bClose.load())
        {
            break;
        }
        sockaddr_in from = { 0 };
        int count = len;
        int iResult = Receive(m_socket, (char*)buf, count, &from);
        if (iResult > 0)
        {
            auto msgList = RecieveHandler(m_pPacketDisp.get(), m_socket, (char*)buf, count, &from);
            for (const auto& str : msgList)
            {
                CThreadSafeCout{} << str << std::endl;
            }
        }
    }
    ::closesocket(m_socket);
}

std::list<std::string> CWinSockUDPServer::RecieveHandler(CPacketDispather* /*pPacketDisp*/, SOCKET server, char* pMsg, int nLen, sockaddr_in* from)
{
    std::list<std::string> msgList;
    ULONG addr = from ? from->sin_addr.S_un.S_addr : 0;
    if (addr)
    {

        auto pClientOpt = m_clients.find(addr);
        // Новый клиент
        if (m_clients.end() == pClientOpt)
        {
            auto packetDisp = CreatePacketDispatcher();
            if (packetDisp)
            {
                auto res = m_clients.emplace(addr, std::move(packetDisp));
                if (res.second)
                {
                    pClientOpt = res.first;
                }
            }
        }

        if (m_clients.end() != pClientOpt)
        {
            auto pPacketDisp = pClientOpt->second ? pClientOpt->second.get() : nullptr;
            if (pPacketDisp)
            {
                msgList = CWinSockServer::RecieveHandler(pPacketDisp, server, pMsg, nLen, from);
            }
        }
    }
    return msgList;
}

/**************************************************************

Класс-обертка для

**************************************************************/
WinSockWrapper::WinSockWrapper() : m_winSockItem(nullptr)
{
}

WinSockWrapper::~WinSockWrapper()
{
    DeleteWinSock();
}

bool WinSockWrapper::CreateWinSock(bool bServer, const std::string& connectType, const std::string& address)
{
    if (connectType.compare("tcp") == 0)
    {
        if (bServer)
            m_winSockItem = std::make_unique<CWinSockTCPServer>();
        else
            m_winSockItem = std::make_unique<CWinSockTCPClient>(address);
    }
    if (connectType.compare("udp") == 0)
    {
        if (bServer)
            m_winSockItem = std::make_unique<CWinSockUDPServer>();
        else
            m_winSockItem = std::make_unique<CWinSockUDPClient>(address);
    }
    return m_winSockItem ? m_winSockItem.get() : false;
}

void WinSockWrapper::DeleteWinSock()
{
    if (m_winSockItem)
    {
        m_winSockItem.release();
    }
}

bool WinSockWrapper::Open(unsigned short port)
{
    return m_winSockItem ? m_winSockItem->Open(port) : false;
}

void WinSockWrapper::Close()
{
    if (m_winSockItem)
    {
        m_winSockItem->Close();
    }
}

bool WinSockWrapper::IsOpen() const
{
    return m_winSockItem ? m_winSockItem->IsOpen() : false;
}

bool WinSockWrapper::Send(const std::string& msg)
{
    return m_winSockItem ? m_winSockItem->Send(msg) : false;
}

