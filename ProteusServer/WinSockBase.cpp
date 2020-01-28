#include "stdafx.h"
#include "WinSockBase.h"
#include "Utils.h"
#include <exception>
#include <string>
#include <WinSock2.h>
#include <memory>

#pragma comment (lib, "Ws2_32.lib")

std::list<std::string> ParsePacket(CPacketDispather* pPacketDisp, char* pMsg, int nLen)
{
    std::list<std::string> pMsgsList;
    if (pPacketDisp)
    {
        if (!pPacketDisp->OnNewMsg(pMsg, nLen, pMsgsList))
        {
            CThreadSafeCout{} << "Ошибка пакета!\n";
        }
    }
    return pMsgsList;
}

std::mutex CWSBase::ws_mutex;
unsigned int CWSBase::m_objectCounter = 0;
bool CWSBase::m_bWSStartup = false;
#define MAKEWORD1(a, b)      ((WORD)(((BYTE)(((DWORD_PTR)(a)) & 0xff)) | ((WORD)((BYTE)(((DWORD_PTR)(b)) & 0xff))) << 8))


CWSBase::CWSBase(UINT protocol)
    : m_protocol(protocol), m_socket(INVALID_SOCKET)
{
    std::lock_guard<std::mutex> lock{ ws_mutex };
    if (!m_bWSStartup)
    {
        WSADATA wsaData;
        m_bWSStartup = (NO_ERROR == WSAStartup(MAKEWORD1(2, 2), &wsaData));
    }
    m_objectCounter++;
    m_pPacketDisp = CreatePacketDispatcher();
}

CWSBase::~CWSBase()
{
    std::lock_guard<std::mutex> lock{ ws_mutex };
    m_objectCounter--;
    if (!m_objectCounter && m_bWSStartup)
    {
        WSACleanup();
        m_bWSStartup = FALSE;
    }
}

bool CWSBase::InitWSLibrary()
{
    std::lock_guard<std::mutex> lock{ ws_mutex };
    if (!m_bWSStartup)
    {
        WSADATA wsaData;
        m_bWSStartup = (NO_ERROR != WSAStartup(MAKEWORD1(2, 2), &wsaData));
    }
    return m_bWSStartup;
}

std::unique_ptr<CPacketDispather> CWSBase::CreatePacketDispatcher()
{
    return std::make_unique<CPacketDispather>();
}

void CWSBase::ClientThread(SOCKET client, HANDLE hStopEvent)
{
    WSAEVENT hSignalEvent = WSACreateEvent();
    if (WSA_INVALID_EVENT != hSignalEvent)
    {
        if (0 == WSAEventSelect(client, hSignalEvent, FD_READ | FD_CLOSE))
        {
            HANDLE arHandles[2] = { hStopEvent, hSignalEvent };
            WSANETWORKEVENTS	NetworkEvents;
            ZeroMemory(&NetworkEvents, sizeof(NetworkEvents));
            bool bStop = false;
            while (true)
            {
                DWORD wait = WSAWaitForMultipleEvents(2, arHandles, FALSE, WSA_INFINITE, 0);

                switch (wait)
                {
                case WSA_WAIT_EVENT_0:
                    bStop = true;
                    break;

                case WSA_WAIT_EVENT_0 + 1:
                {
                    if (0 == WSAEnumNetworkEvents(client, hSignalEvent, &NetworkEvents))
                    {
                        WSAResetEvent(hSignalEvent);

                        if (NetworkEvents.lNetworkEvents & FD_READ || NetworkEvents.lNetworkEvents & FD_CLOSE)
                        {

                            unsigned long count = 0;
                            if (0 == ioctlsocket(client, FIONREAD, &count))
                            {
                                if (count < 1)
                                {
                                    if (NetworkEvents.lNetworkEvents & FD_CLOSE)
                                    {
                                        bStop = true;
                                    }
                                }
                                else
                                {
                                    try
                                    {
                                        auto ptr = std::make_unique<unsigned char[]>(count);
                                        auto buf = ptr.get();
                                        if (buf)
                                        {
                                            sockaddr_in from;
                                            int len = static_cast<int>(count);
                                            int iResult = Receive(client, (char*)buf, len, &from);
                                            if (iResult > 0)
                                            {
                                                auto msgList = RecieveHandler(m_pPacketDisp.get(), client, (char*)buf, count, &from);

                                                for (const auto& str : msgList)
                                                {
                                                    CThreadSafeCout{} << str << std::endl;
                                                }
                                            }
                                            else if (iResult == 0)
                                            {
                                                bStop = true;
                                            }
                                            else /* ошибка */
                                            {

                                            }
                                        }
                                    }
                                    catch (std::bad_alloc&)
                                    {

                                    }
                                }
                            }
                        }
                    }
                    break;
                }
                }
                if (bStop)
                {
                    break;
                }
            }
        }
        WSACloseEvent(hSignalEvent);
    }
}

std::list<std::string> CWSBase::RecieveHandler(CPacketDispather* pPacketDisp, SOCKET /*client*/, char* pMsg, int nLen, sockaddr_in* /*from*/)
{
    return ParsePacket(pPacketDisp, pMsg, nLen);
}

bool CWSBase::Send(const std::string& str)
{
    if (str.size() && m_pPacketDisp)
    {
        std::string formatMsg = m_pPacketDisp->MakeMessage(str);
        int nLen = static_cast<int>(formatMsg.length());
        return (Send(m_socket, (char*)formatMsg.c_str(), nLen, &m_serverAddr) > 0);
    }
    return false;
}

int CWSBase::Send(SOCKET client, char* buf, int& len, sockaddr_in* to)
{
    if (len)
    {
        if (IPPROTO_UDP == m_protocol)
        {
            return sendto(client, buf, len, 0, (sockaddr*)to, sizeof(sockaddr_in));
        }
        else if (IPPROTO_TCP == m_protocol)
        {
            return send(client, buf, len, 0);
        }
    }
    return 0;
}

int CWSBase::Receive(SOCKET client, char* buf, int& len, sockaddr_in* from)
{
    if (len)
    {
        if (IPPROTO_UDP == m_protocol)
        {
            int nSize = sizeof(sockaddr_in);
            return recvfrom(client, buf, len, 0, (sockaddr*)from, &nSize);
        }
        else if (IPPROTO_TCP == m_protocol)
        {
            return recv(client, buf, len, 0);
        }
    }
    return 0;
}

/**************************************************************

Базовый класс для клиента

**************************************************************/

CWinSockClient::CWinSockClient(const std::string& address, UINT protocol)
    : CParent(protocol), m_address(address), m_hStopEvent(nullptr), m_cbRecieve(nullptr)
{
}
CWinSockClient::~CWinSockClient()
{
    Close();
}

bool CWinSockClient::Open(unsigned short port)
{
    if (!m_thread.native_handle() && !m_hStopEvent)
    {
        if (Init(port))
        {
            try
            {
                m_hStopEvent = WSACreateEvent();
                if (m_hStopEvent)
                {
                    m_thread = std::thread(&CParent::ClientThread, this, std::ref(m_socket), std::ref(m_hStopEvent));
                }
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

void CWinSockClient::Close()
{
    auto hThread = m_thread.native_handle();
    if (hThread && m_thread.joinable())
    {
        if (m_hStopEvent)
        {
            WSASetEvent(m_hStopEvent);
            m_thread.join();
        }
    }
    if (m_hStopEvent)
    {
        WSACloseEvent(m_hStopEvent);
    }
}

void CWinSockClient::SetCallBackRecieve(ClientRecieveCallBack pFn)
{
    m_cbRecieve = pFn;
}

/**************************************************************

Базовый класс сервера

**************************************************************/

CWinSockServer::CWinSockServer(UINT protocol) : CParent(protocol), m_port(0)
{
    m_bClose.store(false);
}

CWinSockServer::~CWinSockServer()
{
}

std::list<std::string> CWinSockServer::RecieveHandler(CPacketDispather* pPacketDisp, SOCKET client, char* pMsg, int nLen, sockaddr_in* from)
{
    std::list<std::string> pMsgsList = CParent::RecieveHandler(pPacketDisp, client, pMsg, nLen, from);
    for (const auto& msg : pMsgsList)
    {
        if (pPacketDisp)
        {
            auto pMsgFormat = pPacketDisp->MakeMessage(msg);
            auto len = static_cast<int>(pMsgFormat.length());
            int i = Send(client, (char*)pMsgFormat.c_str(), len, from);
            if (i)
            {

            }
        }
        MsgInfo(msg);
    }
    return pMsgsList;
}