#include "stdafx.h"
#include "Utils.h"
#include <set>

std::mutex CThreadSafeCout::cout_mutex;

CThreadSafeCout::~CThreadSafeCout()
{
    std::lock_guard<std::mutex> lock{ cout_mutex };
    std::cout << rdbuf();
    std::cout.flush();
}

void MsgInfo(const std::string& msg)
{
    if (msg.empty())
    {
        return;
    }
    std::regex re("[0-9]");
    std::sregex_token_iterator iter(msg.begin(), msg.end(), re);
    std::sregex_token_iterator end;
    std::vector<unsigned char> v;
    v.reserve(std::distance(iter, end));

    for (; iter != end; ++iter)
    {
        v.push_back(static_cast<unsigned char>(std::stoi(iter->str().c_str())));
    }
    if (v.empty())
    {
        CThreadSafeCout{} << "В сообщении нет чисел!\n";
        return;
    }
    // в порядке убывания
    std::sort(v.begin(), v.end(), std::greater<char>());
    CThreadSafeCout{} << "Сортировка: ";
    std::copy(std::begin(v), std::end(v), std::ostream_iterator<int>{CThreadSafeCout{}});
    CThreadSafeCout{} << std::endl;

    // сумма
    auto sum = std::accumulate(v.begin(), v.end(), static_cast<unsigned long>(0));
    CThreadSafeCout{} << "Сумма: " << sum << std::endl;

    // максимальное и минимальное значения
    // т.к. массив отсортирован - можно просто взять первое и последнее значение
    // auto max = v[0];
    // auto min = v[v.size() - 1];
    auto minmax = std::minmax_element(v.begin(), v.end());
    CThreadSafeCout{} << "Минимум: " << static_cast<int>(*minmax.first) << "\tМаксимум: " << static_cast<int>(*minmax.second) << std::endl;
}

std::string narrow(const wchar_t* str, int len)
{
    if (!str || len <= 0)
    {
        return{};
    }

    int nBytes = ::WideCharToMultiByte(CP_ACP, NULL, str, len, NULL, 0, NULL, NULL);
    if (nBytes == 1) return{};

    std::vector<char> buf(nBytes);
    buf.resize(nBytes);
    ::WideCharToMultiByte(CP_ACP, 0, str, len, &buf[0], nBytes, NULL, NULL);
    return std::string(buf.begin(), buf.end());
}

std::vector<std::string> ParseCmdLine(int argc, _TCHAR* argv[])
{
    std::vector<std::string> cmdArgs;

    for (int i = 1; i < argc; i++)
    {
        std::string sArg;
#ifndef UNICODE
        sArg = argv[i];
#else
        sArg = narrow(argv[i], wcslen(argv[i]));
#endif
        if (!sArg.empty())
            cmdArgs.push_back(sArg);
    }
    return cmdArgs;
}

std::tuple<std::string, std::string, unsigned short> GetConnectOpt(bool bServer, const std::vector<std::string>& args)
{
    auto opt = std::make_tuple<std::string, std::string, unsigned short>({}, {}, 0);
    const std::set<std::string> serverTypes = { "tcp", "udp" };

    // Тип		
    auto pParam = std::find(args.begin(), args.end(), "-t");
    if ((args.end() != pParam) && (args.end() != (pParam + 1)))
    {
        auto pType = serverTypes.find(*(pParam + 1));
        if (serverTypes.end() != pType)
        {
            std::get<0>(opt) = *pType;
        }
    }

    // Адрес
    if (!bServer)
    {
        pParam = std::find(args.begin(), args.end(), "-a");
        if ((args.end() != pParam) && (args.end() != (pParam + 1)))
        {
            std::get<1>(opt) = *(pParam + 1);
        }
    }
    // Порт			
    pParam = std::find(args.begin(), args.end(), "-p");
    if ((args.end() != pParam) && (args.end() != (pParam + 1)))
    {
        std::get<2>(opt) = static_cast<unsigned short>(atoi((pParam + 1)->c_str()));
    }
    return opt;
}

unsigned StrToLength(const char* buf, unsigned len)
{
    unsigned uLen = 0;
    if (buf && len)
    {
        for (unsigned i = 0; i < len; i++)
        {
            auto ee = (unsigned)((unsigned char) * (buf + i));
            uLen |= ee << (i * 8);
        }
    }

    return uLen;
}

std::string LengthToStr(unsigned uLen, unsigned bytes)
{
    std::string str;
    if (bytes)
    {
        for (unsigned i = 0; i < bytes; i++)
        {
            str.push_back(static_cast<char>((uLen >> (i * 8)) & 0xFF));
        }
    }
    return str;
}

const unsigned len_max = 65537;
const unsigned bits_in_max = 17;
const unsigned move_in_max = 24 - bits_in_max;
const unsigned lengthMask = 0x7F;
const char packetMarker = 111;
const unsigned lenLength = 3;
const unsigned specLen = lenLength + 1;

bool GetCoddingLength(unsigned& uLen)
{
    unsigned uUncodingLen = uLen >> move_in_max;
    const unsigned lenNumbersSum = uLen & lengthMask;
    uLen = uUncodingLen;
    return numberSum(uUncodingLen) == lenNumbersSum;
}

CPacketDispather::CPacketDispather() : m_bHeader(false), m_uLength(0), m_uMsgIndex(0)
{
}

CPacketDispather::~CPacketDispather()
{
}

void CPacketDispather::clear()
{
    m_bHeader = false;
    m_uLength = 0;
    m_uMsgIndex = 0;
    m_msg.clear();
}

bool CPacketDispather::OnNewMsg(char* buf, unsigned len, std::list<std::string>& pMsgs)
{
    if (!buf || !len)
    {
        return (m_bHeader && m_uLength) ? true : false;
    }

    // Ищем маркер заголовка
    if (!m_bHeader)
    {
        // Длина еще не найдена
        if (!m_uLength)
        {
            // Нет и части длины
            if (!m_uMsgIndex)
            {
                // Поиск маркера
                for (unsigned i = 0; i < len; i++)
                {
                    // Маркер
                    if (packetMarker == *(buf + i))
                    {
                        m_bHeader = true;
                        return OnNewMsg(buf + i + 1, len - i - 1, pMsgs);
                    }
                }
            }
        }
    }
    else /* Найден маркер заголовка */
    {
        // Длина еще не найдена
        if (!m_uLength)
        {
            const auto msgLen = m_msg.size();		// столько байт длины сейчас
            const unsigned ostBytes = lenLength - msgLen;	// столько еще нужно
            if (len >= ostBytes)
            {
                for (unsigned i = 0; i < ostBytes; i++, m_uMsgIndex++)
                {
                    m_msg.push_back(*(buf + i));
                }

                unsigned uLen = StrToLength(m_msg.c_str(), lenLength);
                if (GetCoddingLength(uLen))
                {
                    m_uLength = uLen;
                    m_msg.clear();
                }
                else /* это не длина */
                {
                    clear();
                }
                return OnNewMsg(buf + lenLength, len - lenLength, pMsgs);

            }
            else /* сохраняем что есть */
            {
                for (unsigned i = 0; i < len; i++, m_uMsgIndex++)
                {
                    m_msg.push_back(*(buf + i));
                }
            }
        }
        else /* длина найдена */
        {
            const auto msgLen = m_msg.size();
            // Столько осталось байт
            const unsigned ostBytes = m_uLength - msgLen;
            // В этом пакете недостаточно
            if (ostBytes > len)
            {
                for (unsigned i = 0; i < len; i++, m_uMsgIndex++)
                {
                    m_msg.push_back(*(buf + i));
                }
            }
            else /* достаточно */
            {
                for (unsigned i = 0; i < ostBytes; i++, m_uMsgIndex++)
                {
                    m_msg.push_back(*(buf + i));
                }
                pMsgs.push_back(std::move(m_msg));
                clear();

                /* в пакете больше байт чем нужно */
                if (len > ostBytes)
                {
                    const unsigned ostLen = len - ostBytes;
                    return OnNewMsg(buf + ostBytes, ostLen, pMsgs);
                }
            }
        }
    }
    return true;
}

std::string CPacketDispather::MakeMessage(const std::string& str)
{
    std::string msg;
    const auto msgLen = str.length();
    if (!str.empty() && (msgLen <= len_max))
    {
        try
        {
            msg.resize(str.length() + specLen);
            msg.at(0) = (char)packetMarker;
            int newLen = msgLen << move_in_max;
            newLen |= numberSum(static_cast<unsigned>(msgLen));
            const std::string sLen = LengthToStr(newLen, lenLength);
            if (sLen.size() == lenLength)
            {
                for (int i = 0; i < lenLength; i++)
                {
                    msg.at(i + 1) = sLen[i];
                }
            }
            std::copy(str.begin(), str.end(), msg.begin() + specLen);
        }
        catch (std::bad_alloc&)
        {
        }
    }
    return msg;
}