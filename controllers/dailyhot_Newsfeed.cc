#include <fstream>
#include <random>
#include "dailyhot_Newsfeed.h"
#include "news_sources.h"
#include <drogon/utils/coroutine.h>
#include "../common/utilities.h"

using namespace dailyhot;

Newsfeed::Newsfeed()
{
    if (WebCrawler::s_allNewsSrc.empty())
    {
        const Json::Value& srcList = app().getCustomConfig()["feed_src"];
        if (srcList.isArray() && !srcList.empty())
        {
            std::for_each(srcList.begin(), srcList.end(), [this](const Json::Value &src) {
                if (src.isObject() && src.isMember("class"))
                {
                    const std::string key = src["class"].asString();
                    WebCrawler::s_allNewsSrc[key] = src;
                }
            });
        }
    }
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
        content["total"] = WebCrawler::s_allNewsSrc.size();
        
        for (const auto& [k, v] : WebCrawler::s_allNewsSrc)
        {
            Json::Value item;
            item["name"] = k;
            item["title"] = v["name"].asString();
            item["subtitle"] = v["title"].asString();
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
    else if (source == "hitokoto")
    {
        callback(PickHitokoto());
        co_return;
    }

    if (WebCrawler::s_allNewsSrc.find(source) == WebCrawler::s_allNewsSrc.end())
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
        pSource->SetParameter(month);
        std::string url = pSource->srcURL();
        auto client = CreateHttpClient(url);
        
        try
        {
            auto req = pSource->CreateRequest(client);
            req->setPath(url);

            auto resp = co_await client->sendRequestCoro(req);

            if (resp->getStatusCode() == k200OK)
            {
                pSource->SetParameter(month + day);
                content = pSource->ParseData(resp);
                if (content["code"].isNull())
                {
                    content["code"] = 200;
                    content["message"] = "获取成功";
                    content["from"] = "server";
                    content["total"] = content["data"].size();
                    content["updateTime"] = trantor::Date::now().toDbStringLocal();
                    content["subtitle"] = WebCrawler::s_allNewsSrc.at("calendar")["title"].asString();
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

    news_src->SetParameter(type);
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
                if (WebCrawler::s_allNewsSrc.at(src).isMember("categories"))
                {
                    const Json::Value& categories = WebCrawler::s_allNewsSrc.at(src)["categories"];
                    if (categories.isArray() && !categories.empty() && 
                        categories.front().isObject() && !type.empty())
                    {
                        for (const auto& item : categories)
                        {
                            if (item.isMember(type))
                            {
                                val_str = item[type].asString();
                                break;
                            }
                        }
                    }
                }
                
                if (val_str.empty() && WebCrawler::s_allNewsSrc.at(src).isMember("title"))
                    val_str = WebCrawler::s_allNewsSrc.at(src)["title"].asString();
                
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
        LOG_ERROR << "获取原始信息出错: " << e.what();
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

    // BestBlogs需要传入指定榜单类别
    // type: 1=软件编程；2=人工智能

    if (WebCrawler::s_allNewsSrc.find(src) == WebCrawler::s_allNewsSrc.end() || 
        !WebCrawler::s_allNewsSrc.at(src).isMember("categories"))
    {
        return type_str;
    }

    const Json::Value& categories = WebCrawler::s_allNewsSrc.at(src)["categories"];
    if (!categories.isArray() || type_val < 1 || type_val > static_cast<int>(categories.size()))
        return type_str;

    if (categories[type_val - 1].isObject())
    {
        const std::vector<std::string>& keys = categories[type_val - 1].getMemberNames();
        if (!keys.empty())
            type_str = keys.front();
    }
    else if (categories[type_val - 1].isString())
    {
        type_str = categories[type_val - 1].asString();
    }
    
    return type_str;
}

HttpResponsePtr Newsfeed::PickHitokoto() const
{
    const std::string path = app().getCustomConfig()["hitokoto_storage"].asString();
    int min = static_cast<int>('a');
    int max = static_cast<int>('l');
    std::random_device seed; // 硬件生成随机数种子
	std::ranlux48 engine(seed()); // 利用种子生成随机数引擎
    std::uniform_int_distribution<> distrib(min, max); // 设置随机数范围，并为均匀分布
    char random = static_cast<char>(distrib(engine));
    Json::Value content;
    std::string file = path + random + ".json";
    std::ifstream fReader(file, std::ios::binary);
    if (!fReader.is_open())
    {
        content["code"] = static_cast<int>(k500InternalServerError);
        content["message"] = "打开文件失败！";
        return HttpResponse::newHttpJsonResponse(content);
    }

    Json::Reader reader;
    Json::Value value;
    if (!reader.parse(fReader, value) || value.empty() || value.isArray() == false)
    {
        content["code"] = static_cast<int>(k500InternalServerError);
        content["message"] = "解析文件失败！";
    }
    else
    {
        std::uniform_int_distribution<> distribut(0, value.size() - 1);
        content = value[distribut(engine)];
    }

    return HttpResponse::newHttpJsonResponse(content);
}
