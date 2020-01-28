#pragma once
#include "WinSockBase.h"

class CWinSockTCPClient : public CWinSockClient
{
    using CParent = CWinSockClient;

public:
    explicit CWinSockTCPClient(const std::string& address);
    virtual ~CWinSockTCPClient();
protected:
    bool Init(unsigned short port) override;
};

class CWinSockUDPClient : public CWinSockClient
{
    using CParent = CWinSockClient;

public:
    explicit CWinSockUDPClient(const std::string& address);
    virtual ~CWinSockUDPClient();
    void ClientThread(SOCKET, HANDLE) override;
protected:
    bool Init(unsigned short port) override;
    std::atomic_bool m_bClose;
};




class CWinSockTCPServer : public CWinSockServer
{
    using CParent = CWinSockServer;
public:
    explicit CWinSockTCPServer();
    virtual ~CWinSockTCPServer();
    bool Open(unsigned short port) override;
    void Close() override;
protected:
    bool Init(unsigned short port) override;
    void AcceptThread() override;
    void CloseClients();
    virtual std::list<std::string> RecieveHandler(CPacketDispather* pPacketDisp, SOCKET client, char* pMsg, int nLen, sockaddr_in* from) override;
    using ClientThreadOpt = std::tuple<HANDLE, std::thread, std::unique_ptr<CPacketDispather>>;
    using ClientMap = std::map<SOCKET, ClientThreadOpt>;
    ClientMap m_clients;
};

class CWinSockUDPServer : public CWinSockServer
{
    using CParent = CWinSockServer;
public:
    explicit CWinSockUDPServer();
    virtual ~CWinSockUDPServer();
    bool Open(unsigned short port) override;
    void Close() override;
protected:
    bool Init(unsigned short port) override;
    void AcceptThread() override;
    virtual std::list<std::string> RecieveHandler(CPacketDispather* pPacketDisp, SOCKET client, char* pMsg, int nLen, sockaddr_in* from) override;
    using ClientMap = std::map<ULONG, std::unique_ptr<CPacketDispather>>;
    ClientMap m_clients;
};


class WinSockWrapper
{
public:
    WinSockWrapper();
    ~WinSockWrapper();
    WinSockWrapper(const WinSockWrapper&) = delete;
    WinSockWrapper& operator=(WinSockWrapper&) = delete;

    bool CreateWinSock(bool bServer, const std::string& connectType, const std::string& address);
    void DeleteWinSock();
    bool Open(unsigned short port);
    void Close();
    bool IsOpen() const;
    bool Send(const std::string& msg);

protected:
    std::unique_ptr<CWSBase> m_winSockItem;
};