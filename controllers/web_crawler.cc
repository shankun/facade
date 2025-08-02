#include <trantor/utils/Logger.h>
#include <drogon/nosql/RedisClient.h>
#include <drogon/HttpAppFramework.h>
#include "web_crawler.h"

using namespace dailyhot;

std::string WebCrawler::s_dbName = "dailyhotcache";

namespace intrinsic
{
static std::unordered_map<std::string, std::shared_ptr<FeedObjBase>>
    &getObjsMap()
{
    static std::unordered_map<std::string, std::shared_ptr<FeedObjBase>>
        singleInstanceMap;

    return singleInstanceMap;
}

static std::mutex &getMapMutex()
{
    static std::mutex mtx;
    return mtx;
}
}

void FeedClassMap::registerClass(const std::string &className, 
                                 const FeedAllocFunc &func, 
                                 const FeedSharedAllocFunc &sharedFunc)
{
    LOG_TRACE << "Register class:" << className;
    getMap().insert(
        std::make_pair(className, std::make_pair(func, sharedFunc)));
}

FeedObjBase *FeedClassMap::newObject(const std::string &className)
{
    auto iter = getMap().find(className);
    if (iter != getMap().end())
    {
        return iter->second.first();
    }
    else
        return nullptr;
}

std::shared_ptr<FeedObjBase> FeedClassMap::newSharedObject(
    const std::string &className)
{
    auto iter = getMap().find(className);

    if (iter != getMap().end())
    {
        if (iter->second.second)
            return iter->second.second();
        else
            return std::shared_ptr<FeedObjBase>(iter->second.first());
    }
    else
        return nullptr;
}

const std::shared_ptr<FeedObjBase> &FeedClassMap::getSingleInstance(
    const std::string &className)
{
    auto &mtx = intrinsic::getMapMutex();
    auto &singleInstanceMap = intrinsic::getObjsMap();
    {
        std::lock_guard<std::mutex> lock(mtx);
        auto iter = singleInstanceMap.find(className);
        if (iter != singleInstanceMap.end())
            return iter->second;
    }
    auto newObj = newSharedObject(className);
    {
        std::lock_guard<std::mutex> lock(mtx);
        auto ret = singleInstanceMap.insert(
            std::make_pair(className, std::move(newObj)));
        return ret.first->second;
    }
}

void FeedClassMap::setSingleInstance(const std::shared_ptr<FeedObjBase> &ins)
{
    auto &mtx = intrinsic::getMapMutex();
    auto &singleInstanceMap = intrinsic::getObjsMap();
    std::lock_guard<std::mutex> lock(mtx);
    singleInstanceMap[ins->className()] = ins;
}

std::vector<std::string> FeedClassMap::getAllClassName()
{
    std::vector<std::string> ret;
    for (auto const &iter : getMap())
    {
        ret.push_back(iter.first);
    }
    return ret;
}

bool FeedClassMap::containClass(const std::string &className)
{
    return (getMap().find(className) != getMap().end());
}

std::unordered_map<std::string, std::pair<FeedAllocFunc, FeedSharedAllocFunc>>
    &FeedClassMap::getMap()
{
    static std::unordered_map<std::string,
                              std::pair<FeedAllocFunc, FeedSharedAllocFunc>>
        map;

    return map;
}

std::map<std::string, Json::Value> WebCrawler::s_allNewsSrc;

void WebCrawler::Save(const std::string& key_str, Json::Value jsonData)
{
    std::lock_guard<std::mutex> lock(m_newData.mtx);
    m_newData.key = key_str;
    m_newData.data = jsonData;
}

using namespace drogon;

void WebCrawler::UpdateCache()
{
    std::lock_guard<std::mutex> lock(m_newData.mtx);
    if (m_newData.data["from"].asString() == "cache")
        return;

 LOG_INFO << "UpdateCache: " << m_newData.key;
    Json::StreamWriterBuilder wbuilder;
    wbuilder["indentation"] = "\t";
    std::string document = Json::writeString(wbuilder, m_newData.data);
    nosql::RedisClientPtr redisClient = app().getFastRedisClient(s_dbName);
    if (!redisClient)
        return;
    
    try
    {
        redisClient->execCommandAsync(
            [](const drogon::nosql::RedisResult &r) {},
            [](const std::exception &err) {
                LOG_ERROR << "redis set json failed!!! " << err.what();
            },
            "JSON.SET %s $ %s", m_newData.key.c_str(), document.c_str());
    }
    catch(const nosql::RedisException &err)
    {
        LOG_ERROR << "Redis failed to set " 
            << m_newData.key << " : " << err.what();
    }
}

std::string WebCrawler::srcURL() const
{
    std::string key = className();
    if (key.find("src_") == 0)
        key = key.substr(4); // remove "src_" prefix

    std::string url;
    if (s_allNewsSrc.find(key) != s_allNewsSrc.end() &&
        s_allNewsSrc.at(key).isMember("src_url"))
    {
        url = s_allNewsSrc.at(key)["src_url"].asString();
    }
 
    return url;
}

std::string WebCrawler::ExtractContent(const std::string& decoratedStr, 
        const std::string& pre, const std::string& suf) const
{
    std::string result = decoratedStr;
    if (!pre.empty() && decoratedStr.find(pre) == 0)
        result = decoratedStr.substr(pre.length());
    
    std::string::size_type end = result.rfind(suf);
    if (!suf.empty() && end != std::string::npos)
        result = result.substr(0, end);

    return result;
}

