//  ContentCapture.cc

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

//    trantor::EventLoop loop;
    auto evLoop = app().getLoop();
    if (!evLoop)
        return;

    const Json::Value& list = config["sources"];
    // 定时任务
    m_workTimerId = evLoop->runEvery(6s, [this, list]() {
        FetchToStorage(list);
    });
}

void ContentCapture::shutdown() 
{
    app().getLoop()->invalidateTimer(m_workTimerId);
}

void ContentCapture::FetchToStorage(Json::Value srcList)
{
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
