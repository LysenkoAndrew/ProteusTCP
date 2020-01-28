#pragma once
#include <Ws2tcpip.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <map>
#include <atomic>
#include <memory>
#include "Utils.h"

std::list<std::string> ParsePacket(CPacketDispather* pPacketDisp, char* pMsg, int nLen);


class CWSBase
{
public:
    CWSBase(UINT protocol);
    virtual ~CWSBase();
    CWSBase(const CWSBase&) = delete;
    CWSBase& operator=(CWSBase&) = delete;

    virtual bool Open(unsigned short port) = 0;
    virtual void Close() = 0;
    bool IsOpen() { return m_thread.native_handle() != nullptr; }
    virtual void ClientThread(SOCKET, HANDLE);
    virtual bool Send(const std::string& /*str*/);
protected:
    virtual bool Init(unsigned short port) = 0;
    virtual std::list<std::string> RecieveHandler(CPacketDispather* pPacketDisp, SOCKET client, char* pMsg, int nLen, sockaddr_in* from);
    virtual std::unique_ptr<CPacketDispather> CreatePacketDispatcher();
    virtual int Receive(SOCKET client, char* buf, int& len, sockaddr_in* from);
    virtual int Send(SOCKET client, char* buf, int& len, sockaddr_in* from);
    bool InitWSLibrary();
    static std::mutex ws_mutex;
    static unsigned int m_objectCounter;
    static bool m_bWSStartup;

    SOCKET m_socket;
    std::thread m_thread;
    std::unique_ptr<CPacketDispather> m_pPacketDisp;
    sockaddr_in m_serverAddr;
    UINT m_protocol;
};
using ClientRecieveCallBack = void(*)(const std::string& str);
class CWinSockClient : public CWSBase
{
    using CParent = CWSBase;
public:
    CWinSockClient(const std::string& address, UINT protocol);
    virtual ~CWinSockClient();
    bool Open(unsigned short port) override;
    void Close() override;
    void SetCallBackRecieve(ClientRecieveCallBack pFn);
protected:
    virtual bool Init(unsigned short port) override = 0;
    HANDLE m_hStopEvent;
    std::string m_address;
    ClientRecieveCallBack m_cbRecieve;
};

class CWinSockServer : public CWSBase
{
    using CParent = CWSBase;
public:
    CWinSockServer(UINT protocol);
    virtual ~CWinSockServer();
protected:
    virtual std::list<std::string> RecieveHandler(CPacketDispather* pPacketDisp, SOCKET client, char* pMsg, int nLen, sockaddr_in* from) override;
    virtual void AcceptThread() = 0;
    std::mutex m_clientsMapLock;
    unsigned short m_port;
    std::atomic_bool m_bClose;
};