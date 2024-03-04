#include <fstream>
#include <regex>
#include "news_sources.h"
#include <pugixml.hpp>
#include "../common/utilities.h"
#include "BeautifulSoup.hpp"

using namespace dailyhot;
using namespace drogon;

HttpRequestPtr src_36kr::CreateRequest(const drogon::HttpClientPtr& client) const
{
    Json::Value params;
    params["partner_id"] = "wap";
    params["param"]["siteId"] = 1;
    params["param"]["platformId"] = 2;
    params["timestamp"] = trantor::Date::now().microSecondsSinceEpoch();
    auto pReq = HttpRequest::newHttpJsonRequest(params);
    pReq->setMethod(drogon::Post);
    return pReq;
}

std::string src_36kr::srcURL() const
{
    return "https://gateway.36kr.com/api/mis/nav/home/nav/rank/hot";
}

Json::Value src_36kr::ParseData(const HttpResponsePtr& pResp) const
{
    Json::Value finalResp;

    if ((pResp->contentType() != CT_APPLICATION_JSON) || 
        !(pResp->jsonObject()))
    {
        finalResp["code"] = static_cast<int>(k500InternalServerError);
        finalResp["message"] = "返回内容格式错误！";
        return finalResp;
    }

    const Json::Value& root = *(pResp->jsonObject());
    if (root["data"]["hotRankList"].isArray())
    {
        for (auto each : root["data"]["hotRankList"])
        {
            Json::Value item;
            item["id"] = each["itemId"];
            item["title"] = each["templateMaterial"]["widgetTitle"];
            item["pic"] = each["templateMaterial"]["widgetImage"];
            item["owner"] = each["templateMaterial"]["authorName"];
            item["hot"] = each["templateMaterial"]["statRead"];
            item["data"] = each["templateMaterial"];
            item["url"] = "https://www.36kr.com/p/" + each["itemId"].asString();
            item["mobileUrl"] = "https://m.36kr.com/p/" + each["itemId"].asString();
            finalResp["data"].append(item);
        }
    }

    return finalResp;
}

HttpRequestPtr src_baidu::CreateRequest(const drogon::HttpClientPtr& client) const
{
    return HttpRequest::newHttpRequest();
}

std::string src_baidu::srcURL() const
{
    return "https://top.baidu.com/board?tab=realtime";
}

Json::Value src_baidu::ParseData(const HttpResponsePtr& pResp) const
{
    Json::Value finalResp;

    if (pResp->contentType() != CT_TEXT_HTML)
    {
        finalResp["code"] = static_cast<int>(k500InternalServerError);
        finalResp["message"] = "返回内容格式错误！";
        return finalResp;
    }

    Json::Value root;
    const std::string htmlBody = pResp->body().data();
    
    // 正则表达式匹配引起 segment fault
    // std::regex embedReg("/<!--s-data:(.*?)-->/s");
    // std::smatch results;

    // while (std::regex_search(htmlBody, results, embedReg))
    // {
    //     if (results.ready() && results.size() > 1)
    //     {
    //         Json::CharReaderBuilder readFromstr;
    //         std::stringstream ss(results[1].str());
    //         std::string parseError;
    //         if (!Json::parseFromStream(readFromstr, ss, &root, &parseError))
    //         {
    //             finalResp["code"] = static_cast<int>(k500InternalServerError);
    //             finalResp["message"] = "返回HTML中s-data的格式错误！";
    //             return finalResp;
    //         }
    //     }
    // }

    const std::string tag{"<!--s-data:"};
    auto bgn = htmlBody.find(tag);
    if (bgn != std::string::npos)
    {
        bgn += tag.length();
        auto end = htmlBody.find("-->", bgn);
        if (end != std::string::npos && end > bgn)
        {
            Json::CharReaderBuilder readFromstr;
            std::stringstream ss(htmlBody.substr(bgn, end - bgn));
            std::string parseError;
            if (!Json::parseFromStream(readFromstr, ss, &root, &parseError))
            {
                finalResp["code"] = static_cast<int>(k500InternalServerError);
                finalResp["message"] = "返回HTML中s-data的格式错误！";
                return finalResp;
            }
        }
    }

    if (false == root["data"]["cards"].isArray())
    {
        finalResp["code"] = static_cast<int>(k500InternalServerError);
        finalResp["message"] = "解析s-data的内容错误！";
        return finalResp;
    }

    const Json::Value& contents = root["data"]["cards"][0]["content"];
    if (contents.isArray())
    {
        for (auto each : contents)
        {
            Json::Value item;
            item["title"] = each["query"];
            item["desc"] = each["desc"];
            item["pic"] = each["img"];
            item["hot"] = std::stoi(each["hotScore"].asString());
            item["url"] = "https://www.baidu.com/s?wd=" + utils::urlEncodeComponent(each["query"].asString());
            item["mobileUrl"] = item["url"];
            finalResp["data"].append(item);
        }
    }

    return finalResp;
}

HttpRequestPtr src_bilibili::CreateRequest(const drogon::HttpClientPtr& client) const
{
    return HttpRequest::newHttpRequest();
}

std::string src_bilibili::srcURL() const
{
    return "https://api.bilibili.com/x/web-interface/ranking/v2";
}

Json::Value src_bilibili::ParseData(const HttpResponsePtr& pResp) const
{
    Json::Value finalResp;

    if ((pResp->contentType() != CT_APPLICATION_JSON) || 
        !(pResp->jsonObject()))
    {
        finalResp["code"] = static_cast<int>(k500InternalServerError);
        finalResp["message"] = "返回内容格式错误！";
        return finalResp;
    }

    const Json::Value& root = *(pResp->jsonObject());
    if (root["data"]["list"].isArray())
    {
        for (auto video : root["data"]["list"])
        {
            Json::Value item;
            item["id"] = video["bvid"];
            item["title"] = video["title"];
            item["desc"] = video["desc"];
            std::string picUrl = video["pic"].asString();
            std::string prefix = "http:";
            item["pic"] = picUrl.replace(picUrl.find(prefix), prefix.length(), "https:");
            item["owner"] = video["owner"];
            item["data"] = video["stat"];
            item["hot"] = video["stat"]["view"];
            item["url"] = "https://www.bilibili.com/video/" + video["bvid"].asString();
            item["mobileUrl"] = "https://m.bilibili.com/video/" + video["bvid"].asString();
            finalResp["data"].append(item);
        }
    }

    return finalResp;
}

HttpRequestPtr src_calendar::CreateRequest(const drogon::HttpClientPtr& client) const
{
    return HttpRequest::newHttpRequest();
}

std::string src_calendar::srcURL() const
{
    const std::string month = trantor::Date::now().toCustomedFormattedStringLocal("%m");
    std::string url = "https://baike.baidu.com/cms/home/eventsOnHistory/" + month;
    url += ".json";
    return url;
}

Json::Value src_calendar::ParseData(const HttpResponsePtr& pResp) const
{
    Json::Value finalResp;
    // 返回格式为text/json
    if ((pResp->contentType() != CT_CUSTOM) || 
        !(pResp->jsonObject()))
    {
        finalResp["code"] = static_cast<int>(k500InternalServerError);
        finalResp["message"] = "calendar 返回内容格式错误！";
        return finalResp;
    }

    const Json::Value& root = *(pResp->jsonObject());
    std::string month;
    for (auto itr = root.begin(); itr != root.end() ; itr++)
        month = itr.key().asString();

    const std::string date = m_parameter.empty() ?
     trantor::Date::now().toCustomedFormattedStringLocal("%m%d") : m_parameter;

    if (root[month][date].isArray())
    {
        std::regex href("<[^>]+>");
        for (auto history : root[month][date])
        {
            Json::Value item;
            item["year"] = history["year"];
            item["id"] = history["bvid"];
            // 删除文本中的超链接标记
            std::string text =
                std::regex_replace(history["title"].asString(), href, std::string(""));

            std::string year = history["year"].asString();
            item["title"] = year + "年 " + text;
            text = std::regex_replace(history["desc"].asString(), href, std::string(""));
            item["desc"] = text;
            item["pic"] = history.isMember("pic_share") ? history["pic_share"] : history["pic_index"];
            item["avatar"] = history["pic_calendar"];
            item["type"] = history["type"];
            item["url"] = history["link"];
            item["mobileUrl"] = history["link"];
            finalResp["data"].append(item);
        }
    }

    return finalResp;
}

HttpRequestPtr src_ckxx::CreateRequest(const drogon::HttpClientPtr& client) const
{
    return HttpRequest::newHttpRequest();
}

std::string src_ckxx::srcURL() const
{
    std::string url{"https://china.cankaoxiaoxi.com/json/channel/"};
    url += m_parameter;
    return url + "/list.json";
}

Json::Value src_ckxx::ParseData(const HttpResponsePtr& pResp) const
{
    Json::Value finalResp;

    if ((pResp->contentType() != CT_APPLICATION_JSON) || 
        !(pResp->jsonObject()))
    {
        finalResp["code"] = static_cast<int>(k500InternalServerError);
        finalResp["message"] = "ckxx 返回内容格式错误！";
        return finalResp;
    }

    const Json::Value& root = *(pResp->jsonObject());
    if (root["list"].isArray())
    {
        for (auto eachNew : root["list"])
        {
            Json::Value item;
            item["id"] = eachNew["id"];
            item["title"] = eachNew["data"]["title"];
            item["pic"] = eachNew["data"]["mCoverImg"];
            item["hot"] = eachNew["data"]["visitCount"];
            item["url"] = eachNew["data"]["url"];
            item["mobileUrl"] = eachNew["data"]["url"];
            finalResp["data"].append(item);
        }
    }

    return finalResp;
}

HttpRequestPtr src_douban::CreateRequest(const drogon::HttpClientPtr& client) const
{
    client->setUserAgent("Mozilla/5.0 (iPhone; CPU iPhone OS 15_0 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/15.0 Mobile/15E148 Safari/604.1");
    return HttpRequest::newHttpRequest();
}

std::string src_douban::srcURL() const
{
    return "https://movie.douban.com/chart";
}

Json::Value src_douban::ParseData(const HttpResponsePtr& pResp) const
{
    Json::Value finalResp;
    // 返回格式为text/html
    if ((pResp->contentType() != CT_TEXT_HTML) || pResp->body().empty())
    {
        finalResp["code"] = static_cast<int>(k500InternalServerError);
        finalResp["message"] = "豆瓣新片榜 返回内容格式错误！";
        return finalResp;
    }

    std::string strVal;
    std::string resp_str{pResp->body()};
    std::replace(resp_str.begin(), resp_str.end(), '\n',' ');
    const std::string prefix{"movie.douban.com"};
    const std::string mobileAddr{"m.douban.com/movie"};
    const std::regex pattern(R"(\d+)");  // 匹配一个或多个数字
    const std::string src_head{"src=\""};

    try
    {
        BeautifulSoup parser(resp_str);
        bool found = false;
        const TagNode article = parser.find("div", {{"class", "article"}}, found);
        if (!found)
        {
            finalResp["code"] = static_cast<int>(k500InternalServerError);
            finalResp["message"] = "豆瓣新片榜 返回内容解析错误！";
            return finalResp;
        }

        const auto& items = 
                parser.find_all(article, "tr", {{"class", "item"}});

        for(const TagNode& item : items)
        {
            const TagNode& link = parser.find(item, "a", {{"class", "nbg"}}, found);
            if (!found)
                continue;

            Json::Value eachMovie;
            strVal = link["href"];
            eachMovie["url"] = strVal;
            strVal.replace(strVal.find(prefix), prefix.length(), mobileAddr);
            eachMovie["mobileUrl"] = strVal;

            // 貌似不支持 <img .... /> 的格式
            const TagNode& image = parser.find(item, "img", found);
            if (found) {
                eachMovie["pic"] = image["src"];
            } else {
                strVal = parser.getNodeData(link.getChildAtIndex(1));
                // <img src="......" width= />
                size_t from_pos = strVal.find(src_head);
                if (from_pos != strVal.npos)
                {
                    from_pos += src_head.length();
                    size_t end_pos = strVal.find("\"", from_pos);
                    if (end_pos != strVal.npos && end_pos > from_pos)
                        eachMovie["pic"] = strVal.substr(from_pos, end_pos - from_pos);
                }
            }

            const TagNode& movie = parser.find(item, "div", {{"class", "pl2"}}, found);
            if (!found)
                continue;

            const TagNode& rating = parser.find(movie, "span", {{"class", "rating_nums"}}, found);
            if (found)
                eachMovie["score"] = parser.getNodeText(rating);

            const TagNode& comment_num = parser.find(movie, "span", {{"class", "pl"}}, found);
            if (found)
            {
                std::smatch results;
                const std::string comment = parser.getNodeText(comment_num);

                if (std::regex_search(comment, results, pattern))
                {
                    const std::string num_str = results[0];
                    eachMovie["comments"] = std::stoi(num_str);
                }
            }

            const TagNode& title_a = parser.find(movie, "a", found);
            if (found)
            {
                std::string title = std::format("[★{}] ", eachMovie["score"].asCString());
                strVal = parser.getNodeText(title_a);
                strVal = std::regex_replace(strVal, std::regex(R"(\s+)"), " ");
                title += strVal;
                const TagNode& title_span = parser.find(movie, "span", found);
                if (found)
                    title += " " + parser.getNodeText(title_span);

                eachMovie["title"] = title;
            }

            const TagNode& description = parser.find(movie, "p", found);
            if (found)
                eachMovie["desc"] = parser.getNodeText(description);
                
            finalResp["data"].append(eachMovie);
        }
    }
    catch(const std::exception& e)
    {
        finalResp["code"] = static_cast<int>(k500InternalServerError);
        finalResp["message"] = e.what();
        return finalResp;
    }
    
    return finalResp;
}

HttpRequestPtr src_douban_group::CreateRequest(const drogon::HttpClientPtr& client) const
{
    auto pReq = HttpRequest::newHttpRequest();
    pReq->addHeader("accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7");
    pReq->addHeader("accept-language", "zh-CN,zh;q=0.9,en;q=0.8");
    pReq->addHeader("cache-control", "max-age=0");
    pReq->addHeader("sec-ch-ua", "\"Not_A Brand\";v=\"8\", \"Chromium\";v=\"120\", \"Google Chrome\";v=\"120\"");
    pReq->addHeader("sec-ch-ua-mobile", "?0");
    pReq->addHeader("sec-ch-ua-platform", "\"Windows\"");
    pReq->addHeader("sec-fetch-dest", "document");
    pReq->addHeader("sec-fetch-mode", "navigate");
    pReq->addHeader("sec-fetch-site", "none");
    pReq->addHeader("sec-fetch-user", "?1");
    pReq->addHeader("upgrade-insecure-requests", "1");
    return pReq;
}

std::string src_douban_group::srcURL() const
{
    return "https://www.douban.com/group/explore";
}

Json::Value src_douban_group::ParseData(const HttpResponsePtr& pResp) const
{
    Json::Value finalResp;
    // 返回格式为text/html
    if ((pResp->contentType() != CT_TEXT_HTML) || pResp->body().empty())
    {
        finalResp["code"] = static_cast<int>(k500InternalServerError);
        finalResp["message"] = "豆瓣小组话题榜 返回内容格式错误！";
        return finalResp;
    }

    std::string strVal;
    std::string resp_str{pResp->body()};
    std::replace(resp_str.begin(), resp_str.end(), '\n',' ');
    const std::string src_head{"src=\""};

    try
    {
        BeautifulSoup parser(resp_str);
        const auto& items = parser.find_all("div", {{"class", "channel-item"}});
        if (items.empty())
        {
            finalResp["code"] = static_cast<int>(k500InternalServerError);
            finalResp["message"] = "豆瓣小组话题榜 返回内容解析错误！";
            return finalResp;
        }

        bool found = false;
        for(const TagNode& item : items)
        {
            const TagNode& title_a = parser.find(item, "a", found);
            if (!found)
                continue;

            Json::Value eachTopic;
            eachTopic["title"] = parser.getNodeText(title_a);
            eachTopic["url"] = title_a["href"];
            eachTopic["mobileUrl"] = title_a["href"];

            const TagNode& likes = parser.find(item, "div", {{"class", "likes"}}, found);
            if (found)
                eachTopic["hot"] = std::stoi(parser.getNodeText(likes));

            const TagNode& image = parser.find(item, "img", found);
            if (found) {
                strVal = image["src"];
                if (false == strVal.empty())
                {
                    strVal.replace(strVal.find(".webp"), 5, ".jpg");
                    eachTopic["pic"] = strVal;
                }
            }

            const TagNode& description = parser.find(item, "div", {{"class", "block"}}, found);
            if (found)
            {
                strVal = parser.getNodeText(description);
                std::replace(strVal.begin(), strVal.end(), '\r', ' ');
                eachTopic["desc"] = strVal;
            }

            const TagNode& from = parser.find(item, "div", {{"class", "source"}}, found);
            if (found)
            {
                const TagNode& src_a = parser.find(from, "a", found);
                if (found)
                    eachTopic["source"] = parser.getNodeText(src_a);
            }
                
            finalResp["data"].append(eachTopic);
        }
    }
    catch(const std::exception& e)
    {
        finalResp["code"] = static_cast<int>(k500InternalServerError);
        finalResp["message"] = e.what();
        return finalResp;
    }

    return finalResp;
}

HttpRequestPtr src_douyin::CreateRequest(const drogon::HttpClientPtr& client) const
{
    client->setUserAgent("okhttp3");
    return HttpRequest::newHttpRequest();
}

std::string src_douyin::srcURL() const
{
    return "https://aweme.snssdk.com/aweme/v1/hot/search/list/?device_platform=android&version_name=13.2.0&version_code=130200&aid=1128";
}

Json::Value src_douyin::ParseData(const HttpResponsePtr& pResp) const
{
    Json::Value finalResp;

    if ((pResp->contentType() != CT_APPLICATION_JSON) || 
        !(pResp->jsonObject()))
    {
        finalResp["code"] = static_cast<int>(k500InternalServerError);
        finalResp["message"] = "抖音 返回内容格式错误！";
        return finalResp;
    }

    std::string val_str;
    const Json::Value& root = *(pResp->jsonObject());
    if (root["data"]["word_list"].isArray())
    {
        for (auto each : root["data"]["word_list"])
        {
            Json::Value item;
            item["title"] = each["word"];
            item["pic"] = each["word_cover"]["url_list"][0];
            item["hot"] = each["hot_value"];
            val_str = "https://www.douyin.com/hot/";
            val_str += utils::urlEncodeComponent(each["sentence_id"].asString());
            item["url"] = val_str;
            item["mobileUrl"] = val_str;
            finalResp["data"].append(item);
        }
    }

    return finalResp;
}

HttpRequestPtr src_douyin_music::CreateRequest(const drogon::HttpClientPtr& client) const
{
    client->setUserAgent("okhttp3");
    return HttpRequest::newHttpRequest();
}

std::string src_douyin_music::srcURL() const
{
    return "https://aweme.snssdk.com/aweme/v1/chart/music/list/?device_platform=android&version_name=13.2.0&version_code=130200&aid=1128&chart_id=6853972723954146568&count=100";
}

Json::Value src_douyin_music::ParseData(const HttpResponsePtr& pResp) const
{
    Json::Value finalResp;

    if ((pResp->contentType() != CT_APPLICATION_JSON) || 
        !(pResp->jsonObject()))
    {
        finalResp["code"] = static_cast<int>(k500InternalServerError);
        finalResp["message"] = "抖音热歌榜 返回内容格式错误！";
        return finalResp;
    }

    std::string val_str;
    const Json::Value& root = *(pResp->jsonObject());
    if (root["music_list"].isArray())
    {
        for (auto song : root["music_list"])
        {
            Json::Value item;
            item["id"] = song["music_info"]["id"];
            item["title"] = song["music_info"]["title"];
            item["album"] = song["music_info"]["album"];
            item["artist"] = song["music_info"]["author"];
            item["lyric"] = song["music_info"]["lyric_url"];
            item["pic"] = song["music_info"]["cover_large"]["url_list"][0];
            item["hot"] = song["heat"];
            item["url"] = song["music_info"]["play_url"]["url_list"][0];
            item["mobileUrl"] = item["url"];
            finalResp["data"].append(item);
        }
    }

    return finalResp;
}

HttpRequestPtr src_github::CreateRequest(const drogon::HttpClientPtr& client) const
{
    return HttpRequest::newHttpRequest();
}

std::string src_github::srcURL() const
{
    return "https://gh.shankun.xyz/https://github.com/trending";
}

Json::Value src_github::ParseData(const HttpResponsePtr& pResp) const
{
    Json::Value finalResp;
    // 返回格式为text/html
    if ((pResp->contentType() != CT_TEXT_HTML) || pResp->body().empty())
    {
        finalResp["code"] = static_cast<int>(k500InternalServerError);
        finalResp["message"] = "github 返回内容格式错误！";
        return finalResp;
    }

    const std::string prefix{"https://github.com/"};
    // 阿里云ECS无法访问github.com，需要通过cloudflare代理。
    // 但代理脚本会将html中所有链接加上前缀，因此应该删除前缀。
    const std::string proxyPrefix{"https://gh.shankun.xyz/https%3A%2F%2Fgithub.com%2F"};
    std::string strVal;
//    std::regex white_spaces(R"(/(^\s*)|(\s*$)/g)");

    std::string resp_str{pResp->body()};
    std::replace(resp_str.begin(), resp_str.end(), '\n',' ');

    try
    {
        BeautifulSoup parser(resp_str);
        auto articles = parser.find_all("article", {{"class", "Box-row"}});

        for(const TagNode& article : articles)
        {
            bool found = false;
            const TagNode& articleH2 = parser.find(article, "h2", found);
            if (!found)
                continue;
            
            const TagNode& a_tag = parser.find(articleH2, "a", found);
            if (!found)
                continue;
            
            strVal = a_tag["href"];
            Json::Value eachProj;
            if (strVal.starts_with(proxyPrefix))
                strVal = strVal.substr(proxyPrefix.length());

            eachProj["title"] = strVal;
            eachProj["url"] = prefix + strVal;
            strVal.clear();
            const TagNode& article_p = parser.find(article, "p", found);
            if (found)
                strVal = parser.getNodeText(article_p);

            eachProj["desc"] = strVal;
            strVal.clear();
            const Node& div_f6 = found ? article_p.getNextSibling() : articleH2.getNextSibling();
            const TagNode& lang = parser.find(div_f6, "span", 
                    {{"itemprop", "programmingLanguage"}}, found);

            if (found)
                strVal = parser.getNodeText(lang);

            eachProj["language"] = strVal;
            strVal.clear();
            const TagNode& a_stars = parser.find(div_f6, "a", found);
            if (found)
                strVal = parser.getNodeText(a_stars);

            eachProj["stars"] = strVal;
            strVal.clear();
            const Node& a_forks = a_stars.getNextSibling();
            eachProj["forks"] = parser.getNodeText(a_forks);

            const Node& last_span = a_forks.getNextSibling().getNextSibling();
            strVal = parser.getNodeText(last_span);
            size_t spacePos = strVal.find(' ');
            if (spacePos != strVal.npos)
                strVal = strVal.substr(0, spacePos);

            eachProj["starstoday"] = strVal;
            eachProj["mobileUrl"] = eachProj["url"];
            finalResp["data"].append(eachProj);
        }
    }
    catch(const std::exception& e)
    {
        finalResp["code"] = static_cast<int>(k500InternalServerError);
        finalResp["message"] = e.what();
        return finalResp;
    }
    
    return finalResp;
}

HttpRequestPtr src_ithome::CreateRequest(const drogon::HttpClientPtr& client) const
{
    client->setUserAgent("Mozilla/5.0 (iPhone; CPU iPhone OS 15_0 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/15.0 Mobile/15E148 Safari/604.1");
    return HttpRequest::newHttpRequest();
}

std::string src_ithome::srcURL() const
{
    return "https://m.ithome.com/rankm";
}

Json::Value src_ithome::ParseData(const HttpResponsePtr& pResp) const
{
    Json::Value finalResp;
    // 返回格式为text/html
    if ((pResp->contentType() != CT_TEXT_HTML) || pResp->body().empty())
    {
        finalResp["code"] = static_cast<int>(k500InternalServerError);
        finalResp["message"] = "ithome 返回内容格式错误！";
        return finalResp;
    }

    std::string strVal;
    std::string resp_str{pResp->body()};
    std::replace(resp_str.begin(), resp_str.end(), '\n',' ');
    const std::regex idPattern{R"([html|live]\/(\d+)\.htm)"};
    try
    {
        BeautifulSoup parser(resp_str);
        auto rank_items = parser.find_all("div", {{"class", "rank-name"}});

        for(const TagNode& rank_item : rank_items)
        {
            const std::string rank_type = parser.getNodeText(rank_item);
            const Node& rank_box = rank_item.getNextSibling();
            const auto& articles = 
                parser.find_all(rank_box, "div", {{"class", "placeholder one-img-plc"}});
            
            if (articles.empty())
                continue;
            
            for (const TagNode& article : articles)
            {
                bool found = false;
                const TagNode& title = parser.find(article, "p", {{"class", "plc-title"}}, found);
                if (!found)
                    continue;

                Json::Value eachPost;
                const TagNode& link = parser.find(article, "a", found);
                if (found)
                {
                    strVal = link["href"];
                    eachPost["mobileUrl"] = strVal;
                    std::smatch results;
                    if (std::regex_search(strVal, results, idPattern))
                    {
                        strVal = results[1];
                        eachPost["url"] = 
                            std::format("https://www.ithome.com/0/{}/{}.htm", strVal.substr(0, 3), strVal.substr(3));
                    }
                }

                eachPost["title"] = parser.getNodeText(title);

                const TagNode& image = parser.find(article, "img", found);
                if (found)
                    eachPost["img"] = image["data-original"];

                const TagNode& time = parser.find(article, "span", {{"class", "post-time"}}, found);
                if (found)
                    eachPost["time"] = parser.getNodeText(time);

                eachPost["type"] = rank_type;

                const TagNode& reviews = parser.find(article, "span", {{"class", "review-num"}}, found);
                if (found)
                    eachPost["hot"] = std::stoi(parser.getNodeText(reviews));
                
                finalResp["data"].append(eachPost);
            }
        }
    }
    catch(const std::exception& e)
    {
        finalResp["code"] = static_cast<int>(k500InternalServerError);
        finalResp["message"] = e.what();
        return finalResp;
    }
    
    return finalResp;
}

HttpRequestPtr src_juejin::CreateRequest(const drogon::HttpClientPtr& client) const
{
    return HttpRequest::newHttpRequest();
}

std::string src_juejin::srcURL() const
{
    return "https://api.juejin.cn/content_api/v1/content/article_rank?category_id=1&type=hot";
}

Json::Value src_juejin::ParseData(const HttpResponsePtr& pResp) const
{
    Json::Value finalResp;

    if ((pResp->contentType() != CT_APPLICATION_JSON) || 
        !(pResp->jsonObject()))
    {
        finalResp["code"] = static_cast<int>(k500InternalServerError);
        finalResp["message"] = "稀土掘金 返回内容格式错误！";
        return finalResp;
    }

    std::string val_str;
    const Json::Value& root = *(pResp->jsonObject());
    if (root["data"].isArray())
    {
        for (auto each : root["data"])
        {
            Json::Value item;
            item["id"] = each["content"]["content_id"];
            item["title"] = each["content"]["title"];
            item["hot"] = each["content_counter"]["hot_rank"];
            val_str = "https://juejin.cn/post/" + item["id"].asString();
            item["url"] = val_str;
            item["mobileUrl"] = val_str;
            finalResp["data"].append(item);
        }
    }

    return finalResp;
}

HttpRequestPtr src_kuaishou::CreateRequest(const drogon::HttpClientPtr& client) const
{
    client->setUserAgent("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/114.0.0.0 Safari/537.36 Edg/114.0.1823.67");
    HttpRequestPtr pCaller = HttpRequest::newHttpRequest();
    pCaller->setParameter("isHome" , "1");
    return pCaller;
}

std::string src_kuaishou::srcURL() const
{
    return "https://www.kuaishou.com";
}

Json::Value src_kuaishou::ParseData(const HttpResponsePtr& pResp) const
{
    Json::Value finalResp;

    if (pResp->contentType() != CT_TEXT_HTML)
    {
        finalResp["code"] = static_cast<int>(k500InternalServerError);
        finalResp["message"] = "快手 返回内容格式错误！";
        return finalResp;
    }

    Json::Value root;
    const std::string htmlBody = pResp->body().data();
    const std::string dataStart{"window.__APOLLO_STATE__={"};
    auto bgn = htmlBody.find(dataStart);
    if (bgn != std::string::npos)
    {
        bgn += dataStart.length() - 1;
        auto end = htmlBody.find(";(function()", bgn);
        if (end != std::string::npos && end > bgn)
        {
            Json::CharReaderBuilder readFromstr;
            std::stringstream ss(htmlBody.substr(bgn, end - bgn));
            std::string parseError;
            if (!Json::parseFromStream(readFromstr, ss, &root, &parseError))
            {
                finalResp["code"] = static_cast<int>(k500InternalServerError);
                finalResp["message"] = "返回HTML中json块的格式错误！";
                return finalResp;
            }
        }
    }

    const std::string rank{"$ROOT_QUERY.visionHotRank({\"page\":\"home\"})"};
    if (false == root["defaultClient"][rank]["items"].isArray())
    {
        finalResp["code"] = static_cast<int>(k500InternalServerError);
        finalResp["message"] = "解析items的内容错误！";
        return finalResp;
    }

    std::string val_str;
    const std::regex idPattern{"clientCacheKey=([A-Za-z0-9]+)"};
    const Json::Value& contents = root["defaultClient"][rank]["items"];
    if (contents.isArray())
    {
        for (auto each : contents)
        {
            Json::Value item;
            const std::string id_str{each["id"].asString()};
            const auto &candidate = root["defaultClient"][id_str];
            item["title"] = candidate["name"];

            const std::string img_src{candidate["poster"].asString()};
            std::smatch results;
            
            if (std::regex_search(img_src, results, idPattern))
                val_str = results[1];

            item["pic"] = dailyhot::UnEscape(img_src);
            item["hot"] = candidate["hotValue"];
            item["url"] = "https://www.kuaishou.com/short-video/" + val_str;
            item["mobileUrl"] = item["url"];
            finalResp["data"].append(item);
        }
    }

    return finalResp;
}

HttpRequestPtr src_netease::CreateRequest(const drogon::HttpClientPtr& client) const
{
    return HttpRequest::newHttpRequest();
}

std::string src_netease::srcURL() const
{
    return "https://m.163.com/fe/api/hot/news/flow";
}

Json::Value src_netease::ParseData(const HttpResponsePtr& pResp) const
{
    Json::Value finalResp;

    if ((pResp->contentType() != CT_TEXT_PLAIN) || pResp->body().empty())
    {
        finalResp["code"] = static_cast<int>(k500InternalServerError);
        finalResp["message"] = "网易新闻 返回内容格式错误！";
        return finalResp;
    }

    Json::Value root;
    Json::CharReaderBuilder readFromstr;
    std::stringstream ss(pResp->body().data());
    std::string parseError;
    if (!Json::parseFromStream(readFromstr, ss, &root, &parseError))
    {
        finalResp["code"] = static_cast<int>(k500InternalServerError);
        finalResp["message"] = "返回HTML中s-data的格式错误！";
        return finalResp;
    }

    std::string val_str;
    if (root["data"]["list"].isArray())
    {
        for (auto each : root["data"]["list"])
        {
            Json::Value item;
            item["id"] = each["skipID"];
            item["title"] = each["title"];
            item["desc"] = each["_keyword"];
            item["pic"] = each["imgsrc"];
            item["owner"] = each["source"];
            val_str = "https://www.163.com/dy/article/" + item["id"].asString() + ".html";
            item["url"] = val_str;
            item["mobileUrl"] = val_str;
            finalResp["data"].append(item);
        }
    }

    return finalResp;
}

HttpRequestPtr src_netease_music::CreateRequest(const drogon::HttpClientPtr& client) const
{
    client->setUserAgent("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/114.0.0.0 Safari/537.36 Edg/114.0.1823.67");
    return HttpRequest::newHttpRequest();
}

std::string src_netease_music::srcURL() const
{
    std::string url{"https://music.163.com/discover/toplist?id="};
    return url + m_parameter;
}

Json::Value src_netease_music::ParseData(const HttpResponsePtr& pResp) const
{
    Json::Value finalResp;
    // 返回格式为text/html
    if ((pResp->contentType() != CT_TEXT_HTML) || pResp->body().empty())
    {
        finalResp["code"] = static_cast<int>(k500InternalServerError);
        finalResp["message"] = "网易云音乐返回内容格式错误！";
        return finalResp;
    }

    const std::string prefix{"<textarea id=\"song-list-pre-data\" style=\"display:none;\">"};
    std::string htmlBody{pResp->body()};
    Json::Value root;
    auto bgn = htmlBody.find(prefix);
    if (bgn != std::string::npos)
    {
        bgn += prefix.length();
        auto end = htmlBody.find("</textarea>", bgn);
        if (end != std::string::npos && end > bgn)
        {
            Json::CharReaderBuilder readFromstr;
            std::stringstream ss(htmlBody.substr(bgn, end - bgn));
            std::string parseError;
            if (!Json::parseFromStream(readFromstr, ss, &root, &parseError))
            {
                finalResp["code"] = static_cast<int>(k500InternalServerError);
                finalResp["message"] = "返回HTML中json块的格式错误！";
                return finalResp;
            }
        }
    }

    if (false == root.isArray())
    {
        finalResp["code"] = static_cast<int>(k500InternalServerError);
        finalResp["message"] = "解析json的内容错误，不是array！";
        return finalResp;
    }

    std::string val_str;
    for (auto each : root)
    {
        Json::Value song;
        const std::string id_str{each["id"].asString()};
        song["id"] = id_str;
        std::string singers;
        if (each["artists"].isArray())
        {
            for (auto itr = each["artists"].begin(); itr != each["artists"].end(); ++itr)
            {
                if (itr != each["artists"].begin())
                    singers += " & ";
                
                singers += (*itr)["name"].asString();
            }
        }

        val_str = each["name"].asString();
        
        if (singers.empty() == false)
            val_str += " - " + singers;

        song["title"] = val_str;
        song["pic"] = each["album"]["picUrl"];
            
        song["url"] = "https://music.163.com/song?id=" + id_str;
        song["mobileUrl"] = song["url"];
        finalResp["data"].append(song);
    }

    return finalResp;
}

HttpRequestPtr src_newsqq::CreateRequest(const drogon::HttpClientPtr& client) const
{
    return HttpRequest::newHttpRequest();
}

std::string src_newsqq::srcURL() const
{
    return "https://r.inews.qq.com/gw/event/hot_ranking_list?page_size=50";
}

Json::Value src_newsqq::ParseData(const HttpResponsePtr& pResp) const
{
    Json::Value finalResp;

    if ((pResp->contentType() != CT_APPLICATION_JSON) || 
        !(pResp->jsonObject()))
    {
        finalResp["code"] = static_cast<int>(k500InternalServerError);
        finalResp["message"] = "腾讯新闻 返回内容格式错误！";
        return finalResp;
    }

    const Json::Value& root = *(pResp->jsonObject());
    if (root["idlist"].isArray() && root["idlist"][0]["newslist"].isArray())
    {
        const Json::Value& list = root["idlist"][0]["newslist"];
        for (auto itr = list.begin(); itr != list.end(); ++itr)
        {
            if (itr == list.begin())
                continue;

            Json::Value item;
            item["id"] = (*itr)["id"];
            item["title"] = (*itr)["title"];
            item["desc"] = (*itr)["abstract"];
            item["descSm"] = (*itr)["nlpAbstract"];
            item["hot"] = (*itr)["readCount"];
            item["pic"] = (*itr)["miniProShareImage"];
            std::string val_str{"https://new.qq.com/rain/a/"};
            item["url"] = val_str + (*itr)["id"].asString();
            item["mobileUrl"] = "https://view.inews.qq.com/a/" + (*itr)["id"].asString();
            finalResp["data"].append(item);
        }
    }

    return finalResp;
}

HttpRequestPtr src_qq_music::CreateRequest(const drogon::HttpClientPtr& client) const
{
    client->setUserAgent("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/114.0.0.0 Safari/537.36 Edg/114.0.1823.67");
    return HttpRequest::newHttpRequest();
}

std::string src_qq_music::srcURL() const
{
    std::string url{"https://c.y.qq.com/v8/fcg-bin/fcg_v8_toplist_cp.fcg?topid="};
    url += m_parameter + "&platform=yqq.json&jsonpCallback=MusicJsonCallbacktoplist";
    return url;
}

Json::Value src_qq_music::ParseData(const HttpResponsePtr& pResp) const
{
    Json::Value finalResp;
    // 返回格式为text/html
    if ((pResp->contentType() != CT_TEXT_HTML) || pResp->body().empty())
    {
        finalResp["code"] = static_cast<int>(k500InternalServerError);
        finalResp["message"] = "QQ音乐返回内容格式错误！";
        return finalResp;
    }

    const std::string_view prefix{"MusicJsonCallbacktoplist("};
    std::size_t bgn = pResp->body().find(prefix);
    std::string htmlBody{pResp->body()};
    Json::Value root;

    if (bgn != std::string::npos && htmlBody.ends_with(')'))
    {
        bgn += prefix.length();
        size_t useful_len = htmlBody.length() - bgn - 1;

        Json::CharReaderBuilder readFromstr;
        std::string resp_str{htmlBody.substr(bgn, useful_len)};
        std::replace(resp_str.begin(), resp_str.end(), '\n', ' ');
        std::stringstream ss(resp_str);
        std::string parseError;
      
        if (!Json::parseFromStream(readFromstr, ss, &root, &parseError))
        {
            finalResp["code"] = static_cast<int>(k500InternalServerError);
            finalResp["message"] = "返回HTML中json块的格式错误！";
            return finalResp;
        }
    }

    if (false == root["songlist"].isArray())
    {
        finalResp["code"] = static_cast<int>(k500InternalServerError);
        finalResp["message"] = "解析json的内容错误，不是array！";
        return finalResp;
    }

    std::string val_str;
    for (auto each : root["songlist"])
    {
        Json::Value song;
        const std::string id_str{each["data"]["songmid"].asString()};
        song["id"] = id_str;
 
        std::string singers;
        if (each["data"]["singer"].isArray())
        {
            for (auto itr = each["data"]["singer"].begin(); itr != each["data"]["singer"].end(); ++itr)
            {
                if (itr != each["data"]["singer"].begin())
                    singers += " & ";
                
                singers += (*itr)["name"].asString();
            }
        }

        val_str = each["data"]["songname"].asString();
        
        if (singers.empty() == false)
            val_str += " - " + singers;

        song["title"] = val_str;
        song["hot"] = each["Franking_value"].asString();
        song["url"] = "https://y.qq.com/n/ryqq/songDetail/" + id_str;
        song["mobileUrl"] = song["url"];
        finalResp["data"].append(song);
    }

    return finalResp;
}

HttpRequestPtr src_smth::CreateRequest(const drogon::HttpClientPtr& client) const
{
    return HttpRequest::newHttpRequest();
}

std::string src_smth::srcURL() const
{
    // 直接从阿里云访问报错：您的ksbj-IP因为访问过于频繁，已被系统封禁
    // 因此使用代理转发
    return "https://gh.shankun.xyz/https://www.newsmth.net/nForum/rss/topten";
}

Json::Value src_smth::ParseData(const HttpResponsePtr& pResp) const
{
    Json::Value finalResp;
    // 返回格式为text/xml
    if ((pResp->contentType() != CT_TEXT_XML) || pResp->body().empty())
    {
        finalResp["code"] = static_cast<int>(k500InternalServerError);
        finalResp["message"] = "smth 返回内容格式错误！";
        return finalResp;
    }

    converter conv("UTF-8", "GB2312", true);
    const std::string input{pResp->body().data()};
    std::string xmlString;
    conv.convert(input, xmlString);

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_string(xmlString.c_str());
    std::string strVal;
    std::regex href("<[^>]+>");

    if (result)
    {
        pugi::xpath_node_set items = doc.select_nodes("/rss/channel/item");
        for (pugi::xpath_node_set::const_iterator itr = items.begin(); 
            itr != items.end(); ++itr)
        {
            Json::Value eachPost;
            eachPost["title"] = itr->node().child_value("title");

            strVal = itr->node().child_value("description");
            // 删除文本中的超链接标记
            std::string text =
                std::regex_replace(strVal, href, std::string(" "));

            eachPost["desc"] = text;
            eachPost["time"] = itr->node().child_value("pubDate");
            strVal = itr->node().child_value("link");
            // replace "http:" with "https:"
            std::string::size_type pos = 4;
            strVal.insert(pos, 1, 's');
            eachPost["url"] = strVal;
            eachPost["mobileUrl"] = eachPost["url"];
            finalResp["data"].append(eachPost);
        }
    }
    else
    {
        finalResp["code"] = static_cast<int>(result.status);
        finalResp["message"] = result.description();
    }

    return finalResp;
}

HttpRequestPtr src_sspai::CreateRequest(const drogon::HttpClientPtr& client) const
{
    return HttpRequest::newHttpRequest();
}

std::string src_sspai::srcURL() const
{
    return "https://sspai.com/api/v1/article/tag/page/get?limit=40&tag=热门文章";
}

Json::Value src_sspai::ParseData(const HttpResponsePtr& pResp) const
{
    Json::Value finalResp;

    if ((pResp->contentType() != CT_APPLICATION_JSON) || 
        !(pResp->jsonObject()))
    {
        finalResp["code"] = static_cast<int>(k500InternalServerError);
        finalResp["message"] = "少数派 返回内容格式错误！";
        return finalResp;
    }

    const Json::Value& root = *(pResp->jsonObject());
    if (root["data"].isArray())
    {
        for (auto each : root["data"])
        {
            Json::Value item;
            item["id"] = each["id"];
            item["title"] = each["title"];
            item["desc"] = each["summary"];
            std::string val_str{"https://cdn.sspai.com/"};
            item["pic"] = val_str + each["banner"].asString();
            item["owner"] = each["author"];
            item["hot"] = each["like_count"];
            val_str = "https://sspai.com/post/" + each["id"].asString();
            item["url"] = val_str;
            val_str = "https://sspai.com/post/" + each["itemId"].asString();
            item["mobileUrl"] = val_str;
            finalResp["data"].append(item);
        }
    }

    return finalResp;
}

HttpRequestPtr src_solidot::CreateRequest(const drogon::HttpClientPtr& client) const
{
    return HttpRequest::newHttpRequest();
}

std::string src_solidot::srcURL() const
{
    return "https://www.solidot.org/index.rss";
}

Json::Value src_solidot::ParseData(const HttpResponsePtr& pResp) const
{
    Json::Value finalResp;
    // 返回格式为text/xml
    if ((pResp->contentType() != CT_APPLICATION_XML) || pResp->body().empty())
    {
        finalResp["code"] = static_cast<int>(k500InternalServerError);
        finalResp["message"] = "solidot 返回内容格式错误！";
        return finalResp;
    }

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_string(pResp->body().data());
    std::string strVal;
    std::regex href("<[^>]+>");

    if (result)
    {
        const std::string newLine = "\n";
        pugi::xpath_node_set items = doc.select_nodes("/rss/channel/item");
        for (pugi::xpath_node_set::const_iterator itr = items.begin(); 
            itr != items.end(); ++itr)
        {
            Json::Value eachPost;
            eachPost["title"] = itr->node().child_value("title");

            strVal = itr->node().child_value("description");
            // 删除文本中的超链接标记
            std::string text =
                std::regex_replace(strVal, href, std::string(" "));

            size_t pos = text.find(newLine);
            while (pos != text.npos)
            {
                text.replace(pos, newLine.length(), "");
                pos = text.find(newLine);
            }
            
            text.erase(0, text.find_first_not_of(' ')); // trim
            text.erase(text.find_last_not_of(' ') + 1);
            eachPost["desc"] = text;
            eachPost["time"] = itr->node().child_value("pubDate");
            strVal = itr->node().child_value("link");
            eachPost["url"] = strVal;
            eachPost["mobileUrl"] = eachPost["url"];
            finalResp["data"].append(eachPost);
        }
    }
    else
    {
        finalResp["code"] = static_cast<int>(result.status);
        finalResp["message"] = result.description();
    }

    return finalResp;
}

HttpRequestPtr src_thepaper::CreateRequest(const drogon::HttpClientPtr& client) const
{
    return HttpRequest::newHttpRequest();
}

std::string src_thepaper::srcURL() const
{
    return "https://cache.thepaper.cn/contentapi/wwwIndex/rightSidebar";
}

Json::Value src_thepaper::ParseData(const HttpResponsePtr& pResp) const
{
    Json::Value finalResp;

    if ((pResp->contentType() != CT_APPLICATION_JSON) || 
        !(pResp->jsonObject()))
    {
        finalResp["code"] = static_cast<int>(k500InternalServerError);
        finalResp["message"] = "澎湃 返回内容格式错误！";
        return finalResp;
    }

    const Json::Value& root = *(pResp->jsonObject());
    if (root["data"]["hotNews"].isArray())
    {
        for (auto each : root["data"]["hotNews"])
        {
            Json::Value item;
            item["id"] = each["contId"];
            item["title"] = each["name"];
            item["pic"] = each["pic"];
            item["hot"] = each["praiseTimes"];
            item["time"] = each["pubTime"];
            std::string url_str{"https://www.thepaper.cn/newsDetail_forward_"};
            url_str += each["contId"].asString();
            item["url"] = url_str;
            url_str = "https://m.thepaper.cn/newsDetail_forward_";
            url_str += each["contId"].asString();
            item["mobileUrl"] = url_str;
            finalResp["data"].append(item);
        }
    }

    return finalResp;
}

HttpRequestPtr src_tieba::CreateRequest(const drogon::HttpClientPtr& client) const
{
    return HttpRequest::newHttpRequest();
}

std::string src_tieba::srcURL() const
{
    return "https://tieba.baidu.com/hottopic/browse/topicList";
}

Json::Value src_tieba::ParseData(const HttpResponsePtr& pResp) const
{
    Json::Value finalResp;

    if ((pResp->contentType() != CT_APPLICATION_JSON) || 
        !(pResp->jsonObject()))
    {
        finalResp["code"] = static_cast<int>(k500InternalServerError);
        finalResp["message"] = "贴吧 返回内容格式错误！";
        return finalResp;
    }

    const Json::Value& root = *(pResp->jsonObject());
    if (root["data"]["bang_topic"]["topic_list"].isArray())
    {
        for (auto each : root["data"]["bang_topic"]["topic_list"])
        {
            Json::Value item;
            item["id"] = each["topic_id"];
            item["title"] = each["topic_name"];
            item["desc"] = each["topic_desc"];
            item["pic"] = each["topic_pic"];
            item["hot"] = each["discuss_num"];
            item["url"] = each["topic_url"].asString();
            item["mobileUrl"] = each["topic_url"].asString();
            finalResp["data"].append(item);
        }
    }

    return finalResp;
}

HttpRequestPtr src_toutiao::CreateRequest(const drogon::HttpClientPtr& client) const
{
    return HttpRequest::newHttpRequest();
}

std::string src_toutiao::srcURL() const
{
    return "https://www.toutiao.com/hot-event/hot-board/?origin=toutiao_pc";
}

Json::Value src_toutiao::ParseData(const HttpResponsePtr& pResp) const
{
    Json::Value finalResp;

    if ((pResp->contentType() != CT_APPLICATION_JSON) || 
        !(pResp->jsonObject()))
    {
        finalResp["code"] = static_cast<int>(k500InternalServerError);
        finalResp["message"] = "返回内容格式错误！";
        return finalResp;
    }

    const Json::Value& root = *(pResp->jsonObject());
    if (root["data"].isArray())
    {
        for (auto each : root["data"])
        {
            Json::Value item;
            item["id"] = each["ClusterId"];
            item["title"] = each["Title"];
            item["pic"] = each["Image"]["url"];
            item["hot"] = each["HotValue"];
            std::string val_str{"https://www.toutiao.com/trending/"};
            val_str += each["ClusterIdStr"].asString();
            item["url"] = val_str;
            val_str = "https://api.toutiaoapi.com/feoffline/amos_land/new/html/main/index.html?topic_id=";
            val_str += each["ClusterIdStr"].asString();
            item["mobileUrl"] = val_str;
            finalResp["data"].append(item);
        }
    }

    return finalResp;
}

HttpRequestPtr src_v2ex::CreateRequest(const drogon::HttpClientPtr& client) const
{
    client->setUserAgent("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/114.0.0.0 Safari/537.36 Edg/114.0.1823.67");
    return HttpRequest::newHttpRequest();
}

std::string src_v2ex::srcURL() const
{
    return "https://gh.shankun.xyz/https://machbbs.com/v2ex/?tab=hot";
}

Json::Value src_v2ex::ParseData(const HttpResponsePtr& pResp) const
{
    Json::Value finalResp;
    // 返回格式为text/html
    if ((pResp->contentType() != CT_TEXT_HTML) || pResp->body().empty())
    {
        finalResp["code"] = static_cast<int>(k500InternalServerError);
        finalResp["message"] = "v2ex 返回内容格式错误！";
        return finalResp;
    }

    std::string strVal;
    std::string resp_str{pResp->body()};
    std::replace(resp_str.begin(), resp_str.end(), '\n',' ');

    try
    {
        BeautifulSoup parser(resp_str);
        auto subjects = parser.find_all("div", {{"class", "media-body"}});

        for(const TagNode& subject : subjects)
        {
            Json::Value eachSubject;
            bool found = false;
            const TagNode& link = parser.find(subject, "a", found);
            if (!found)
                continue;

            eachSubject["title"] = parser.getNodeText(link);
            eachSubject["url"] = link["href"];

            eachSubject["mobileUrl"] = link["href"];
            const TagNode& userName = parser.find(subject, "span", found);
            if (found)
            {
                eachSubject["member"] = parser.getNodeText(userName);
                eachSubject["time"] = parser.getNodeText(userName.getNextSibling());
            }

            const TagNode& commentNum = parser.find(link.getNextSibling(), "span", 
                                        {{"class", "ml-2"}}, found);
            if (found)
                eachSubject["comments"] = parser.getNodeText(commentNum);
            
            finalResp["data"].append(eachSubject);
        }
    }
    catch(const std::exception& e)
    {
        finalResp["code"] = static_cast<int>(k500InternalServerError);
        finalResp["message"] = e.what();
        return finalResp;
    }
    
    return finalResp;
}

HttpRequestPtr src_weibo::CreateRequest(const drogon::HttpClientPtr& client) const
{
    return HttpRequest::newHttpRequest();
}

std::string src_weibo::srcURL() const
{
    return "https://weibo.com/ajax/side/hotSearch";
}

Json::Value src_weibo::ParseData(const HttpResponsePtr& pResp) const
{
    Json::Value finalResp;

    if ((pResp->contentType() != CT_APPLICATION_JSON) || 
        !(pResp->jsonObject()))
    {
        finalResp["code"] = static_cast<int>(k500InternalServerError);
        finalResp["message"] = "返回内容格式错误！";
        return finalResp;
    }

    const Json::Value& root = *(pResp->jsonObject());
    if (root["data"]["realtime"].isArray())
    {
        for (auto each : root["data"]["realtime"])
        {
            Json::Value item;
            item["title"] = each["word"];
            item["desc"] = each.isMember("word_scheme") ? each["word_scheme"] : each["word"];
            item["hot"] = each["raw_hot"];
            std::string val_str{"https://s.weibo.com/weibo?q="};
            val_str += utils::urlEncodeComponent(item["desc"].asString());
            val_str += "&t=31&band_rank=1&Refer=top";
            item["url"] = val_str;
            item["mobileUrl"] = val_str;
            finalResp["data"].append(item);
        }
    }

    return finalResp;
}

HttpRequestPtr src_weread::CreateRequest(const drogon::HttpClientPtr& client) const
{
    client->setUserAgent("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/114.0.0.0 Safari/537.36 Edg/114.0.1823.67");
    return HttpRequest::newHttpRequest();
}

std::string src_weread::srcURL() const
{
    return "https://weread.qq.com/web/bookListInCategory/rising?rank=1";
}

Json::Value src_weread::ParseData(const HttpResponsePtr& pResp) const
{
    Json::Value finalResp;

    if ((pResp->contentType() != CT_APPLICATION_JSON) || 
        !(pResp->jsonObject()))
    {
        finalResp["code"] = static_cast<int>(k500InternalServerError);
        finalResp["message"] = "weread 返回内容格式错误！";
        return finalResp;
    }

    std::string val_str;
    const Json::Value& root = *(pResp->jsonObject());
    if (root["books"].isArray())
    {
        for (auto each_book : root["books"])
        {
            Json::Value item;
            item["id"] = each_book["bookInfo"]["bookId"];
            item["title"] = each_book["bookInfo"]["title"];
            item["desc"] = each_book["bookInfo"]["intro"];
            val_str = each_book["bookInfo"]["cover"].asString();
            val_str.replace(val_str.find("s_"), 2, "t9_");
            item["pic"] = val_str;
            item["hot"] = each_book["readingCount"];
            item["author"] = each_book["bookInfo"]["author"];
            val_str = "https://weread.qq.com/web/bookDetail/";
            val_str += GetWeReadID(item["id"].asString());
            item["url"] = val_str;
            item["mobileUrl"] = item["url"];
            finalResp["data"].append(item);
        }
    }

    return finalResp;
}

HttpRequestPtr src_zhihu::CreateRequest(const drogon::HttpClientPtr& client) const
{
    client->setUserAgent("Mozilla/5.0 (iPhone; CPU iPhone OS 15_0 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/15.0 Mobile/15E148 Safari/604.1");
    return HttpRequest::newHttpRequest();
}

std::string src_zhihu::srcURL() const
{
    return "https://www.zhihu.com/hot";
}

Json::Value src_zhihu::ParseData(const HttpResponsePtr& pResp) const
{
    Json::Value finalResp;

    if (pResp->contentType() != CT_TEXT_HTML)
    {
        finalResp["code"] = static_cast<int>(k500InternalServerError);
        finalResp["message"] = "知乎 返回内容格式错误！";
        return finalResp;
    }

    Json::Value root;
    const std::string htmlBody = pResp->body().data();
    const std::string tagStart{"<script id=\"js-initialData\" type=\"text/json\">"};
    auto bgn = htmlBody.find(tagStart);
    if (bgn != std::string::npos)
    {
        bgn += tagStart.length();
        auto end = htmlBody.find("</script>", bgn);
        if (end != std::string::npos && end > bgn)
        {
            Json::CharReaderBuilder readFromstr;
            std::stringstream ss(htmlBody.substr(bgn, end - bgn));
            std::string parseError;
            if (!Json::parseFromStream(readFromstr, ss, &root, &parseError))
            {
                finalResp["code"] = static_cast<int>(k500InternalServerError);
                finalResp["message"] = "返回HTML中json块的格式错误！";
                return finalResp;
            }
        }
    }

    if (false == root["initialState"]["topstory"]["hotList"].isArray())
    {
        finalResp["code"] = static_cast<int>(k500InternalServerError);
        finalResp["message"] = "解析items的内容错误！";
        return finalResp;
    }

    std::string val_str;
    const std::regex notNumber{R"([^\d.])"};
    const Json::Value& contents = root["initialState"]["topstory"]["hotList"];
    if (contents.isArray())
    {
        for (auto each : contents)
        {
            Json::Value item;
            const std::string id_str{each["id"].asString()};
            item["title"] = each["target"]["titleArea"]["text"];
            item["desc"] = each["target"]["excerptArea"]["text"];
            val_str = each["target"]["imageArea"]["url"].asString();
            val_str = dailyhot::UnEscape(val_str);
            item["pic"] = val_str;

            val_str = each["target"]["metricsArea"]["text"].asString();
            std::string text =
                std::regex_replace(val_str, notNumber, std::string(" "));

            text.erase(std::remove(text.begin(), text.end(), ' '), text.end());
            if (false == text.empty())
                item["hot"] = std::stof(text) * 10000;
                
            item["url"] = each["target"]["link"]["url"];
            item["mobileUrl"] = item["url"];
            finalResp["data"].append(item);
        }
    }

    return finalResp;
}
