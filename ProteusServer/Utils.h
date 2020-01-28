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

// ���������������� cout
class CThreadSafeCout : public std::stringstream {
public:
	~CThreadSafeCout();
	static std::mutex cout_mutex;
};

void MsgInfo(const std::string& msg);

// ������� ����� ���� � �����
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

// �������� ��� ����������, ����� � ����� �����
std::tuple<std::string, std::string, unsigned short> GetConnectOpt(bool bServer, const std::vector<std::string>& args);

// �����, ���������� �� ����������� � �������������� ��������� ������
/*
��������� ���������:
	0 ����		- 0x6F	������ ������ ���������
	1-3 �����	- �������������� ����� ������
		���� 24 - 8	- ����� ��������� ���������
		���� 7 - 0	- ����� ����� ����� ��������� ���������
	����� �������� ���������
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