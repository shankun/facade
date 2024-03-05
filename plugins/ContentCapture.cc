//  ContentCapture.cc

#include <fstream>
#include <chrono>
#include "ContentCapture.h"
#include <drogon/HttpAppFramework.h>
#include "../common/utilities.h"

using namespace std::literals;
using namespace drogon;

void ContentCapture::initAndStart(const Json::Value &config)
{
    if (false == config.isMember("sources") ||
        false == config["sources"].isArray() ||
        config["sources"].empty())
    {
        LOG_ERROR << "'sources' not found in plugin config.";
        return;
    }

    m_pEvLoop = trantor::EventLoop::getEventLoopOfCurrentThread();
    if (!m_pEvLoop)
        return;

    // 每次运行服务时更新一言数据包
    const std::string url = config["hitokoto_url"].asString();
    const std::string dirPath = app().getCustomConfig()["hitokoto_storage"].asString();
    std::filesystem::remove_all(dirPath);
    std::filesystem::create_directories(dirPath);

    m_downloadTimerId = m_pEvLoop->runEvery(2s, [this, url, dirPath]() {
        DownloadHitokoto(url, dirPath);
    });

    const Json::Value& list = config["sources"];
    // 定时任务
    m_workTimerId = m_pEvLoop->runEvery(30min, [this, list]() {
        FetchToStorage(list);
    });
}

void ContentCapture::shutdown() 
{
    m_pEvLoop->invalidateTimer(m_workTimerId);
}

void ContentCapture::DownloadHitokoto(const std::string& siteAddress, const std::string& repo)
{
    std::string savePath = repo;
    std::string file_name;
    for (char c = 'a'; c <= 'l'; ++c)
    {
        if (!std::filesystem::exists(savePath + c + ".json"))
        {
            file_name = c;
            file_name += ".json";
            break;
        }
    }

    if (file_name.empty())
    {
        m_pEvLoop->invalidateTimer(m_downloadTimerId);
        return;
    }
    
    savePath += file_name;
    std::string reqPath = siteAddress + file_name;
    
    auto pos = reqPath.find("://");
    auto pathPos = reqPath.find('/', pos + 3);
    std::string server_name;
    std::string req_path;
    if (pathPos == std::string::npos)
    {
        server_name = reqPath;
        req_path = "/";
    }
    else
    {
        server_name = reqPath.substr(0, pathPos);
        req_path = reqPath.substr(pathPos);
    }

    reqPath = req_path;
    auto pClient = HttpClient::newHttpClient(server_name, m_pEvLoop);
    auto req = HttpRequest::newHttpRequest();
    req->setMethod(drogon::Get);
    pClient->setUserAgent("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/122.0.0.0 Safari/537.36 Edg/114.0.1823.67");
    req->addHeader("Accept-Encoding", "gzip, deflate, br, zstd");
    req->setPath(reqPath);

    pClient->sendRequest(req,
            [req, savePath](ReqResult result,
                            const HttpResponsePtr &resp) {

                if (result == ReqResult::Ok && 
                    resp->getStatusCode() == k200OK) {
                        std::ofstream outfile(savePath, std::ios::out | std::ios::trunc);
                        outfile.write(resp->body().data(), resp->getBody().length());
                    }
            });
}

void ContentCapture::FetchToStorage(Json::Value srcList)
{
return;
    orm::DbClientPtr postgre = app().getDbClient("postgre");
    if (!postgre)
        return;

    for (auto each : srcList)
    {
        std::string req_path = each["url"].asString();
        auto client = dailyhot::CreateHttpClient(req_path);
        auto req = HttpRequest::newHttpRequest();
        req->setPath(req_path);

        if (each.isMember("headers") &&
            each["headers"].isArray())
        {
            const Json::Value& hdrs = each["headers"];
            for (auto itr = hdrs.begin(); itr != hdrs.end(); ++itr)
            {
                req->addHeader(itr->asString(), std::next(itr)->asString());
                itr++;
            }
        }
    }
}
