#include "dailyhot_Newsfeed.h"
#include "news_sources.h"
#include <drogon/utils/coroutine.h>
#include "../common/utilities.h"

using namespace dailyhot;

Newsfeed::Newsfeed()
: m_allNewsSrc({{"36kr", {"36氪", "热榜"}},
                {"baidu", {"百度", "热搜榜"}},
                {"bilibili", {"哔哩哔哩", "热门榜"}},
                {"calendar", {"历史上的今天", "历史事件"}},
                {"douban", {"豆瓣", "新片榜"}},
                {"douban_group", {"豆瓣小组", "精选话题"}},
                {"douyin", {"抖音", "热点榜"}},
                {"douyin_music", {"抖音", "热歌榜"}},
                {"github", {"Github", "Trending"}},
                {"ithome", {"IT之家", "热榜"}},
                {"juejin", {"稀土掘金", "热榜"}},
                {"kuaishou", {"快手", "热榜"}},
                {"netease", {"网易新闻", "热点榜"}},
                {"netease_music", {"网易云音乐", "排行榜"}},
                {"newsqq", {"腾讯新闻", "热点榜"}},
                {"qq_music", {"QQ音乐", "排行榜"}},
                {"sina", {"新浪新闻", "最新新闻"}},
                {"smth", {"水木社区", "十大话题"}},
                {"solidot", {"Solidot", "最新资讯"}},
                {"sspai", {"少数派", "热榜"}},
                {"thepaper", {"澎湃新闻", "热榜"}},
                {"tieba", {"百度贴吧", "热议榜"}},
                {"toutiao", {"今日头条", "热榜"}},
                {"v2ex", {"v2ex论坛", "新帖"}},
                {"weibo", {"微博", "热搜榜"}},
                {"weread", {"微信读书", "飙升榜"}},
                {"zhihu", {"知乎", "热榜"}},
                {"ckxx", {"参考消息", "频道"}}
               })
, m_ckxxChannels({"zhongguo", "gj", "guandian", "ruick", 
                  "tiyujk", "kejiyy", "wenhualy", 
                  "cankaomt", "cankaozk", "junshi"
                })
, m_wyCategories({"19723756", "3779629", "2884035", "3778678"})
, m_qqCategories({"62", "26", "27", "4", "52", "67"})
, m_typeRankName({{"19723756", "飙升榜"}, {"3779629", "新歌榜"}, 
                  {"2884035", "原创榜"}, {"3778678", "热歌榜"},
                  {"62", "飙升榜"}, {"26", "热歌榜"}, 
                  {"27", "新歌榜"}, {"4", "流行指数榜"}, 
                  {"52", "腾讯音乐人原创榜"}, {"67", "听歌识曲榜"}})
{
}

Task<> Newsfeed::GetHotList(HttpRequestPtr req, 
                            std::function<void (const HttpResponsePtr &)> callback,
                            std::string source, dailyhot::Parameter param) const
{
    Json::Value content;

    if (source == "all")
    {
        content["code"] = k200OK;
        content["message"] = "获取成功";
        content["name"] = "全部接口";
        content["subtitle"] = "除了特殊接口外的全部接口列表";
        content["total"] = m_allNewsSrc.size();
        
        for (const auto& [k, v] : m_allNewsSrc)
        {
            Json::Value item;
            item["name"] = k;
            item["title"] = v.first;
            item["subtitle"] = v.second;
            Json::Value subItem;
            subItem["opts"]["end"] = true;
            subItem["opts"]["name"].isNull();
            subItem["opts"]["sensitive"] = false;
            subItem["opts"]["strict"] = false;
            subItem["opts"]["prefix"] = "";
            subItem["name"].isNull();
            subItem["methods"].append("HEAD");
            subItem["methods"].append("GET");
            subItem["path"] = "/" + k;
            item["stack"].append(subItem);
            subItem["path"] = subItem["path"].asString() + "/new";
            item["stack"].append(subItem);
            content["data"].append(item);
        }
        auto resp = HttpResponse::newHttpJsonResponse(content);
        callback(resp);
        co_return;
    }    

    if (m_allNewsSrc.find(source) == m_allNewsSrc.end())
    {
        auto resp = HttpResponse::newNotFoundResponse();
        callback(resp);
        co_return;
    }
    
    std::string type_str = GetTypeId(source, param.typeVal);
    bool applicable = false;
    nosql::RedisClientPtr redisPtr = app().getFastRedisClient(WebCrawler::s_dbName);
    if (!redisPtr)
    {
        content["code"] = k500InternalServerError;
        content["message"] = "连接Redis失败！";
        auto resp = HttpResponse::newHttpJsonResponse(content);
        callback(resp);
        co_return;
    }
    
    std::string key = type_str.empty() ? source : std::format("{}:{}", source, type_str);
    auto result = co_await redisPtr->execCommandCoro("JSON.GET %s $.updateTime", key.c_str());

    if (!result.isNil())
    {
        std::string lastTime = result.asString();
        size_t len = lastTime.length();
        // ["年-月-日 时:分:秒"]
        const std::string timeStr = lastTime.substr(2, len-4);
        auto date = trantor::Date::fromDbStringLocal(timeStr);
        // 缓存半小时过期
        if (trantor::Date::now() < date.after(3600))
        {
            try
            {
                auto value = co_await redisPtr->execCommandCoro("JSON.GET %s", key.c_str());

                Json::CharReaderBuilder readFromstr;
                std::stringstream ss(value.asString());
                std::string parseError;
                applicable = Json::parseFromStream(readFromstr, ss, &content, &parseError);
            }
            catch(const nosql::RedisException &err)
            {
                LOG_ERROR << "Redis execCommandCoro get failed: " << err.what();
                content["code"] = static_cast<int>(err.code());
                content["message"] = err.what();
                content["name"] = key;
            }
        }
    }

    if (applicable)
        content["from"] = "cache";
    else
        content = co_await FetchNewData(source, type_str);

	auto resp = HttpResponse::newHttpJsonResponse(content);
    callback(resp);
}

Task<> Newsfeed::UpdateHotList(HttpRequestPtr req, 
                               std::function<void (const HttpResponsePtr &)> callback, 
                               std::string source) const
{
    Json::Value content = co_await FetchNewData(source);
    auto resp = HttpResponse::newHttpJsonResponse(content);
    callback(resp);
}

Task<> Newsfeed::Calendar(HttpRequestPtr req, 
                          std::function<void(const HttpResponsePtr &)> callback, 
                          std::string month, std::string day)
{
    Json::Value content;
    bool illegal = false;
    if (month.length() != 2 || day.length() != 2)
    {
        illegal = true;
        content["message"] = "month和day必须是两位整数！";
    }
    else
    {
        const int month_val = std::stoi(month);
        const int day_val = std::stoi(day);
        if (month_val < 1 || month_val > 12 || day_val < 1 || day_val > 31)
        {
            illegal = true;
            content["message"] = "month和day不是正确的日期！";
        }
    }

    if (illegal)
    {
        content["code"] = static_cast<int>(k500InternalServerError);
        auto response = HttpResponse::newHttpJsonResponse(content);
        callback(response);
        co_return;
    }
    
    auto pCrawler = FeedClassMap::getSingleInstance("src_calendar");
    auto pSource = std::dynamic_pointer_cast<src_calendar>(pCrawler);
    if (!pSource)
    {
        content["code"] = static_cast<int>(k500InternalServerError);
        content["message"] = "获取src_calendar失败";
    }
    else
    {
        pSource->m_parameter.clear();
        std::string req_path = pSource->srcURL();
        size_t finalPos = req_path.find_last_of('/');
        std::string url;
        if (finalPos != std::string::npos)
        {
            url = req_path.substr(0, finalPos + 1);
            url += month;
            req_path = url + ".json";
        }
        url = req_path;
        auto client = CreateHttpClient(req_path);
        
        try
        {
            auto req = pSource->CreateRequest(client);
            req->setPath(req_path);

            auto resp = co_await client->sendRequestCoro(req);

            if (resp->getStatusCode() == k200OK)
            {
                pSource->m_parameter = month + day;
                content = pSource->ParseData(resp);
                if (content["code"].isNull())
                {
                    content["code"] = 200;
                    content["message"] = "获取成功";
                    content["from"] = "server";
                    content["total"] = content["data"].size();
                    content["updateTime"] = trantor::Date::now().toDbStringLocal();
                    content["subtitle"] = m_allNewsSrc.at("calendar").second;
                    content["source"] = "calendar";
                }
            }
            else
            {
                content["code"] = static_cast<int>(resp->getStatusCode());
                content["message"] = std::format("从{}请求数据失败！", url);
            }
        }
        catch (const HttpException &e)
        {
            LOG_ERROR << "Unexpected exception: " << e.what();
            content["code"] = static_cast<int>(e.code());
            content["message"] = e.what();
            content["name"] = "calendar";
        }
    }

    auto resp = HttpResponse::newHttpJsonResponse(content);
    callback(resp);
}

Task<Json::Value> Newsfeed::FetchNewData(const std::string &src, const std::string& type) const
{
    auto pCrawler = FeedClassMap::getSingleInstance("src_" + src);
    Json::Value respData;

    if (!pCrawler)
    {
        respData["code"] = static_cast<int>(k500InternalServerError);
        respData["message"] = "获取失败";
        co_return respData;
    }

    auto news_src = std::dynamic_pointer_cast<WebCrawler>(pCrawler);
    if (!news_src)
    {
        respData["code"] = static_cast<int>(k500InternalServerError);
        respData["message"] = "获取WebCrawler失败";
        co_return respData;
    }

    news_src->m_parameter = type;
    std::string req_path = news_src->srcURL();
    std::string key = src;
    std::string val_str;
LOG_INFO << "URL: " << req_path;
    auto client = CreateHttpClient(req_path);
    
    try
    {
        auto req = news_src->CreateRequest(client);
        req->setPath(req_path);

        auto resp = co_await client->sendRequestCoro(req);

        if (resp->getStatusCode() == k200OK)
        {
            respData = news_src->ParseData(resp);
            if (respData["code"].isNull())
            {
                respData["code"] = 200;
                respData["message"] = "获取成功";
                respData["from"] = "server";
                respData["total"] = respData["data"].size();
                respData["updateTime"] = trantor::Date::now().toDbStringLocal();
                if (m_typeRankName.find(type) != m_typeRankName.end())
                    val_str = m_typeRankName.at(type);
                else
                    val_str = m_allNewsSrc.at(src).second;
                
                respData["subtitle"] = val_str;
                respData["source"] = src;
                key = type.empty() ? src : std::format("{}:{}", src, type);
                news_src->Save(key, respData);
            }
        }
        else
        {
            respData["code"] = static_cast<int>(resp->getStatusCode());
            respData["message"] = std::format("获取{}失败", req_path);
        }
    }
    catch (const HttpException &e)
    {
        LOG_ERROR << "Unexpected exception: " << e.what();
        respData["code"] = static_cast<int>(e.code());
        respData["message"] = e.what();
        respData["name"] = key;
    }

    co_return respData;
}

std::string Newsfeed::GetTypeId(const std::string& src, int type_val) const
{
    std::string type_str;
    // 参考消息新闻频道
    // type: 1=中国(zhongguo); 2=国际(gj); 3=观点(guandian); 4=锐参考(ruick); 
    //       5=体育健康(tiyujk); 6=科技应用(kejiyy); 7=文化旅游(wenhualy); 
    //       8=参考漫谈(cankaomt); 9=参考智库(cankaozk); 10=军事(junshi)

    // 网易云音乐排行榜需要传入指定榜单类别
    // type: 1=飙升榜; 2=新歌榜; 3=原创榜; 4=热歌榜

    // QQ音乐排行榜需要传入指定榜单类别
    // type: 1=飙升榜; 2=热歌榜; 3=新歌榜; 4=流行指数榜; 
    //       5=腾讯音乐人原创榜; 6=听歌识曲榜

    if (src == "ckxx")
        type_str = m_ckxxChannels[type_val - 1];
    else if (src == "netease_music")
        type_str = m_wyCategories[type_val - 1];
    else if (src == "qq_music")
        type_str = m_qqCategories[type_val - 1];
    
    return type_str;
}
