#pragma once

#include "web_crawler.h"

#define REGISTERFEEDCLASS(className) \
class src_##className : public dailyhot::FeedObject<src_##className>, public WebCrawler { \
public: \
	src_##className() {} \
    virtual drogon::HttpRequestPtr CreateRequest(const drogon::HttpClientPtr& client) const override; \
	virtual std::string srcURL() const override; \
	virtual Json::Value ParseData(const drogon::HttpResponsePtr& pResp) const override; \
};

REGISTERFEEDCLASS(36kr)
REGISTERFEEDCLASS(baidu)
REGISTERFEEDCLASS(bestblogs)
REGISTERFEEDCLASS(bilibili)
REGISTERFEEDCLASS(calendar)
REGISTERFEEDCLASS(ckxx)
REGISTERFEEDCLASS(douban)
REGISTERFEEDCLASS(douban_group)
REGISTERFEEDCLASS(douyin)
REGISTERFEEDCLASS(douyin_music)
REGISTERFEEDCLASS(gcores)
REGISTERFEEDCLASS(github)
REGISTERFEEDCLASS(hupu)
REGISTERFEEDCLASS(huxiu)
REGISTERFEEDCLASS(ithome)
REGISTERFEEDCLASS(jianshu)
REGISTERFEEDCLASS(juejin)
REGISTERFEEDCLASS(kuaishou)
REGISTERFEEDCLASS(netease)
REGISTERFEEDCLASS(netease_music)
REGISTERFEEDCLASS(newsqq)
REGISTERFEEDCLASS(qq_music)
REGISTERFEEDCLASS(rustcc)
REGISTERFEEDCLASS(sina)
REGISTERFEEDCLASS(smth)
REGISTERFEEDCLASS(solidot)
REGISTERFEEDCLASS(sspai)
REGISTERFEEDCLASS(thepaper)
REGISTERFEEDCLASS(tieba)
REGISTERFEEDCLASS(toutiao)
REGISTERFEEDCLASS(v2ex)
REGISTERFEEDCLASS(weibo)
REGISTERFEEDCLASS(weread)
REGISTERFEEDCLASS(zhihu)

