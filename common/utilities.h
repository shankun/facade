#pragma once

#include <string>
#include <iconv.h>
#include <drogon/HttpClient.h>

namespace dailyhot
{

// 字符集转换
class converter
{
public:
    converter(const std::string& out_encode,
              const std::string& in_encode,
              bool ignore_error = false,
              size_t buf_size = 1024);

    ~converter();
    
    void convert(const std::string& input, std::string& output) const;

private:
  void check_convert_error() const;

  iconv_t iconv_;
  bool ignore_error_;
  const size_t buf_size_;
};


// path传入全URL，返回请求path
drogon::HttpClientPtr CreateHttpClient(std::string& path);

std::string toLower(const std::string &in);

std::string replaceAll(const std::string &str, const std::string &oldStr, const std::string &newStr);

// 将Unicode编码（形如：\\u5b55）转为字符串，假设都是ansi字符，不考虑宽字符编码转换
std::string UnEscape(const std::string& escaped_str);

// 获取微信读书的书籍 ID
//  @param {string} bookId - 书籍 ID
//  @returns {string} - 唯一的书籍 ID
std::string GetWeReadID(const std::string& bookId);

// 获取当前可执行文件所在目录，以'/'结尾
std::string GetCurProcPath();
}
