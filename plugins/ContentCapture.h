/**
 *  ContentCapture.h
 *  抓取一言、今日诗词等内容并存储
 */

#pragma once

#include <drogon/plugins/Plugin.h>

using namespace drogon;

namespace trantor
{
  class EventLoop;
}

class ContentCapture : public Plugin<ContentCapture>
{
  public:
    ContentCapture() {}
    /// This method must be called by drogon to initialize and start the plugin.
    /// It must be implemented by the user.
    void initAndStart(const Json::Value &config) override;

    /// This method must be called by drogon to shutdown the plugin.
    /// It must be implemented by the user.
    void shutdown() override;
  
  private:

    // 更新下载一言数据包
    void DownloadHitokoto(const std::string& siteAddress, const std::string& repo);

    // 获取内容并存入redis
    void FetchToStorage(Json::Value srcList);

    uint64_t m_downloadTimerId;
    uint64_t m_workTimerId;
    trantor::EventLoop *m_pEvLoop;
};

