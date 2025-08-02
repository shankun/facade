#pragma once

#include "web_crawler.h"

#define REGISTERFEEDCLASSU(className) \
class src_##className : public dailyhot::FeedObject<src_##className>, public WebCrawler { \
public: \
	src_##className() {} \
    virtual drogon::HttpRequestPtr CreateRequest(const drogon::HttpClientPtr& client) const override; \
	virtual std::string srcURL() const override; \
	virtual Json::Value ParseData(const drogon::HttpResponsePtr& pResp) const override; \
};

#define REGISTERFEEDCLASS(className) \
class src_##className : public dailyhot::FeedObject<src_##className>, public WebCrawler { \
public: \
	src_##className() {} \
    virtual drogon::HttpRequestPtr CreateRequest(const drogon::HttpClientPtr& client) const override; \
	virtual Json::Value ParseData(const drogon::HttpResponsePtr& pResp) const override; \
};

REGISTERFEEDCLASS(36kr)
REGISTERFEEDCLASS(baidu)
REGISTERFEEDCLASSU(bestblogs)
REGISTERFEEDCLASS(bilibili)
REGISTERFEEDCLASSU(calendar)
REGISTERFEEDCLASSU(ckxx)
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
REGISTERFEEDCLASSU(netease_music)
REGISTERFEEDCLASS(newsqq)
REGISTERFEEDCLASSU(qq_music)
REGISTERFEEDCLASS(rustcc)
REGISTERFEEDCLASSU(sina)
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

