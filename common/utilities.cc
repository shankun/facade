#include <vector>
#include <regex>
#include <stdexcept>
#include <format>
#include "utilities.h"
#include <trantor/utils/Logger.h>
#include <drogon/utils/Utilities.h>

using namespace dailyhot;

converter::converter(const std::string& out_encode,
                     const std::string& in_encode,
                     bool ignore_error,
                     size_t buf_size)
: ignore_error_(ignore_error), buf_size_(buf_size)
{
    if (buf_size == 0)
        throw std::runtime_error("buffer size must be greater than zero");

    iconv_t conv = ::iconv_open(out_encode.c_str(), in_encode.c_str());
    if (conv == (iconv_t)-1)
    {
        if (errno == EINVAL)
            throw std::runtime_error(
                "not supported from " + in_encode + " to " + out_encode);
        else
            throw std::runtime_error("unknown error");
    }
    iconv_ = conv;
}

converter::~converter()
{
    iconv_close(iconv_);
}

void converter::convert(const std::string& input, 
                        std::string& output) const
{
    // copy the string to a buffer as iconv function requires
    //  a non-const char pointer.
    std::vector<char> in_buf(input.begin(), input.end());
    char* src_ptr = &in_buf[0];
    size_t src_size = input.size();

    std::vector<char> buf(buf_size_);
    std::string dst;
    while (0 < src_size)
    {
      char* dst_ptr = &buf[0];
      size_t dst_size = buf.size();
      size_t res = ::iconv(iconv_, &src_ptr, &src_size, &dst_ptr, &dst_size);
      if (res == (size_t)-1)
      {
        if (errno == E2BIG)
        {
          // ignore this error
        }
        else if (ignore_error_)
        {
          // skip character
          ++src_ptr;
          --src_size;
        } else
        {
          check_convert_error();
        }
      }
      dst.append(&buf[0], buf.size() - dst_size);
    }
    dst.swap(output);
}

void converter::check_convert_error() const
{
    switch (errno)
    {
      case EILSEQ:
      case EINVAL:
        throw std::runtime_error("invalid multibyte chars");
      default:
        throw std::runtime_error("unknown error");
    }
}

std::string dailyhot::toLower(const std::string &in)
{
    std::string out = in;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return tolower(c);
    });
    return out;
}

std::string dailyhot::replaceAll(const std::string &str, const std::string &oldStr, const std::string &newStr)
{
    std::string finalStr(str);
    size_t start_pos = 0;
    while ((start_pos = finalStr.find(oldStr, start_pos)) != std::string::npos) {
        finalStr.replace(start_pos, oldStr.length(), newStr);
        start_pos += newStr.length();
    }
    return finalStr;
}

std::string dailyhot::UnEscape(const std::string &escaped_str)
{
    std::string origin = escaped_str;
    std::regex pattern(R"(\\u([\d\w]{4}))", std::regex::icase);
    for (std::smatch sm; std::regex_search(origin, sm, pattern);)
    {
        std::string final(1, static_cast<char>(std::stoi(sm.str(), 0, 16)));
        origin.replace(origin.find(sm.str()), sm.str().length(), final);
    }
 
    return origin;
}

std::string dailyhot::GetWeReadID(const std::string& bookId)
{
    std::string resultId;

    try
    {
        // 使用 MD5 哈希算法创建哈希对象
        const std::string hash_str = toLower(drogon::utils::getMd5(bookId));
        // 取哈希结果的前三个字符作为初始值
        resultId = hash_str.substr(0, 3);

        // 判断书籍 ID 的类型并进行转换
        std::vector<std::string> fa;
        std::regex onlyNum{R"(^\d*$)"};

        if (std::regex_match(bookId, onlyNum))
        {
          // 如果书籍 ID 只包含数字，则将其拆分成长度为 9 的子字符串，并转换为十六进制表示
            for (int i = 0; i < bookId.length(); i += 9)
            {
                const std::string& chunk = bookId.substr(i, i + 9);
                fa.emplace_back(std::format("{:x}", std::stoi(chunk)));
            }
            resultId += "3"; // 将类型添加到初始值中
        }
        else
        {
            // 如果书籍 ID 包含其他字符，则将每个字符的 Unicode 编码转换为十六进制表示
            fa.resize(1);
            for (char c : bookId)
                fa.front() += std::format("{:x}", static_cast<int>(c));

           resultId += "4";
        }

        // 将数字2和哈希结果的后两个字符添加到初始值中
        resultId += "2" + hash_str.substr(hash_str.length() - 2);

        // 处理转换后的子字符串数组
        for (int j = 0; j < fa.size(); ++j)
        {
            const std::string subLen = std::format("{:x}", (int)fa[j].length());
            // 如果长度只有一位数，则在前面添加0
            const std::string subLenPadded = (subLen.length() == 1 ? "0" + subLen : subLen);

           // 将长度和子字符串添加到初始值中
           resultId += subLenPadded + fa[j];

           // 如果不是最后一个子字符串，则添加分隔符 'g'
           if (j < fa.size() - 1)
              resultId += "g";
        }

        // 如果初始值长度不足 20，从哈希结果中取足够的字符补齐
        if (resultId.length() < 20)
           resultId += hash_str.substr(0, 20 - resultId.length());

        // 使用 MD5 哈希算法创建新的哈希对象
        const std::string finalHash = drogon::utils::getMd5(resultId);
        // 取最终哈希结果的前三个字符并添加到初始值的末尾
        resultId += finalHash.substr(0, 3);
    }
    catch(const std::exception& e)
    {
        resultId.clear();
        const std::string err_info{"处理微信读书 ID 时出现错误："};
        LOG_ERROR << err_info + e.what();
    }
    
    return resultId;
}

drogon::HttpClientPtr dailyhot::CreateHttpClient(std::string& path)
{
    auto pos = path.find("://");
    auto pathPos = path.find('/', pos + 3);
    std::string server_name;
    std::string req_path;
    if (pathPos == std::string::npos)
    {
        server_name = path;
        req_path = "/";
    }
    else
    {
        server_name = path.substr(0, pathPos);
        req_path = path.substr(pathPos);
    }

    path = req_path;
    return drogon::HttpClient::newHttpClient(server_name);
}

std::string dailyhot::GetCurProcPath()
{
    std::string location;
    char exePath[PATH_MAX];
    size_t len = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
    if (len != -1) {
        exePath[len] = '\0'; // 添加字符串结束符
        location = exePath;
        size_t lastSlash = location.rfind('/');
        if (lastSlash != std::string::npos)
            location = location.substr(0, lastSlash + 1);
    }

    return location;
}

