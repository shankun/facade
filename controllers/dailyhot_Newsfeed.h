#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

namespace dailyhot
{
struct Parameter{
    int typeVal;
};

class Newsfeed : public HttpController<Newsfeed>
{
  public:
    Newsfeed();

    METHOD_LIST_BEGIN
    // use METHOD_ADD to add your custom processing function here;
    // METHOD_ADD(Newsfeed::get, "/{2}/{1}", Get); // path is /dailyhot/Newsfeed/{arg2}/{arg1}
    // METHOD_ADD(Newsfeed::your_method_name, "/{1}/{2}/list", Get); // path is /dailyhot/Newsfeed/{arg1}/{arg2}/list
    ADD_METHOD_TO(Newsfeed::Calendar, "/feed/calendar/date?month={month}&day={day}", Get);
    ADD_METHOD_TO(Newsfeed::UpdateHotList, "/feed/{src}/new", Get);
    ADD_METHOD_TO(Newsfeed::GetHotList, "/feed/{src}", Get);

    METHOD_LIST_END
    // 为了避免频繁请求官方数据，默认对数据做了缓存处理，默认为 30 分钟
    // 后接 ?type={type}指定类别获取不同的排行榜
    Task<> GetHotList(HttpRequestPtr req, 
                      std::function<void (const HttpResponsePtr &)> callback, 
                      std::string source, dailyhot::Parameter param) const;
    
    // 直接从服务端拉取最新数据，不会从本地缓存中读取
    Task<> UpdateHotList(HttpRequestPtr req, 
                         std::function<void (const HttpResponsePtr &)> callback, 
                         std::string source) const;

    // 特殊接口：历史上的某天 /calendar/date?month=01&day=01
    // 月份和日期必须是两位数
    Task<> Calendar(HttpRequestPtr req, 
                    std::function<void (const HttpResponsePtr &)> callback, 
                    std::string month, std::string day);

  private:
    // 通过网络请求获取数据并解析
    Task<Json::Value> FetchNewData(const std::string& src, 
            const std::string& type = std::string()) const;

    std::string GetTypeId(const std::string& src, int type_val) const;

    // 从本地数据中随机读取一条一言
    HttpResponsePtr PickHitokoto() const;

    // 所有新闻来源
    const std::map<std::string, std::pair<std::string, std::string> > m_allNewsSrc;

    const std::vector<std::string> m_ckxxChannels; // 参考消息频道
    const std::vector<std::string> m_wyCategories; // 网易云音乐榜单
    const std::vector<std::string> m_qqCategories; // QQ音乐榜单
    const std::map<std::string, std::string> m_typeRankName; // 音乐榜单名
    const std::vector<std::string> m_bestBlogsCategories; // BestBlogs栏目
};
}

namespace drogon
{
// 获取请求路径后面的所有参数
template <>
inline dailyhot::Parameter fromRequest(const HttpRequest &req)
{
    const std::string val_str = req.getParameter("type");
    dailyhot::Parameter input;
    if (val_str.empty() == false)
    {
        input.typeVal = std::stoi(val_str);
    }
    return input;
}
}