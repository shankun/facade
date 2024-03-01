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
REGISTERFEEDCLASS(bilibili)
REGISTERFEEDCLASS(calendar)
REGISTERFEEDCLASS(ckxx)
REGISTERFEEDCLASS(douban)
REGISTERFEEDCLASS(douban_group)
REGISTERFEEDCLASS(douyin)
REGISTERFEEDCLASS(douyin_music)
REGISTERFEEDCLASS(github)
REGISTERFEEDCLASS(ithome)
REGISTERFEEDCLASS(juejin)
REGISTERFEEDCLASS(kuaishou)
REGISTERFEEDCLASS(netease)
REGISTERFEEDCLASS(netease_music_toplist)
REGISTERFEEDCLASS(newsqq)
REGISTERFEEDCLASS(ngabbs)
REGISTERFEEDCLASS(qq_music_toplist)
REGISTERFEEDCLASS(smth)
REGISTERFEEDCLASS(sspai)
REGISTERFEEDCLASS(thepaper)
REGISTERFEEDCLASS(tieba)
REGISTERFEEDCLASS(toutiao)
REGISTERFEEDCLASS(v2ex)
REGISTERFEEDCLASS(weibo)
REGISTERFEEDCLASS(weread)
REGISTERFEEDCLASS(zhihu)

