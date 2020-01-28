#pragma once
#include <Windows.h>
#include <iostream>
#include <string>
#include <regex>
#include <algorithm>
#include <functional>
#include <numeric>
#include <vector>
#include <mutex>
#include <sstream>
#include <list>

// ѕотокобезопасный cout
class CThreadSafeCout : public std::stringstream {
public:
	~CThreadSafeCout();
	static std::mutex cout_mutex;
};

void MsgInfo(const std::string& msg);

// ѕодсчет суммы цифр в числе
inline unsigned numberSum(unsigned num)
{
    unsigned sum = 0;
	while (num)
	{
		sum += num % 10;
		num /= 10;
	}
	return sum;
}

unsigned StrToLength(const char* buf, unsigned len);
std::string LengthToStr(unsigned uLen, unsigned bytes);

std::string narrow(const wchar_t* str, int len);

std::vector<std::string> ParseCmdLine(int argc, _TCHAR* argv[]);

bool GetCoddingLength(unsigned& uLen);

// ѕолучить тип соединени€, адрес и номер порта
std::tuple<std::string, std::string, unsigned short> GetConnectOpt(bool bServer, const std::vector<std::string>& args);

//  ласс, отвечающий за кодирование и раскодирование заголовка пакета
/*
—труктура заголовка:
	0 байт		- 0x6F	маркер начала заголовка
	1-3 байта	- закодированна€ длина пакета
		биты 24 - 8	- длина значимого сообщени€
		биты 7 - 0	- сумма чисел длины значимого сообщени€
	далее значимое сообщение
*/
class CPacketDispather
{
public:
	CPacketDispather();
	virtual ~CPacketDispather();
	CPacketDispather(const CPacketDispather&) = delete;
	CPacketDispather& operator=(CPacketDispather&) = delete;
	virtual bool OnNewMsg(char* buf, unsigned len, std::list<std::string>& pMsgs);
	virtual std::string MakeMessage(const std::string& str);
protected:
	bool m_bHeader;
	unsigned m_uLength;
	std::string m_msg;
	unsigned m_uMsgIndex;
	void clear();
};