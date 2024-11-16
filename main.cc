#include <drogon/drogon.h>
#include "controllers/news_sources.h"

using namespace drogon;

int main()
{
    // 通过CSP模板生成上传文件的HTML页面
    app().registerHandler(
        "/upload",
        [](const HttpRequestPtr &,
           std::function<void(const HttpResponsePtr &)> &&callback) {
            auto resp = HttpResponse::newHttpViewResponse("FileUpload");
            callback(resp);
        });

    app().registerHandler(
        "/upload_endpoint",
        [](const HttpRequestPtr &req,
           std::function<void(const HttpResponsePtr &)> &&callback) {
            MultiPartParser fileUpload;
            if (fileUpload.parse(req) != 0 || fileUpload.getFiles().size() != 1)
            {
                auto resp = HttpResponse::newHttpResponse();
                resp->setBody("Must only be one file");
                resp->setStatusCode(k403Forbidden);
                callback(resp);
                return;
            }

            auto &file = fileUpload.getFiles()[0];
            auto md5 = file.getMd5();
            auto resp = HttpResponse::newHttpResponse();
            resp->setBody(
                "The server has calculated the file's MD5 hash to be " + md5);
            file.save();
            LOG_INFO << "The uploaded file has been saved to the ./uploads "
                        "directory";
            callback(resp);
        },
        {Post});

     app().registerPostHandlingAdvice([](const drogon::HttpRequestPtr &req,
                                        const drogon::HttpResponsePtr &resp) {
        if (req->path().find("/feed/") == 0)
        {
            const std::string src = "src_" + req->path().substr(6);
            if (dailyhot::FeedClassMap::containClass(src)) {
                auto crawler = dailyhot::FeedClassMap::getSingleInstance(src);
                if (crawler)
                {
                    auto news_src = std::dynamic_pointer_cast<WebCrawler>(crawler);
                    if (news_src)
                        news_src->UpdateCache();
                }
            }
        }
    });
    
    LOG_INFO << "Server running on 127.0.0.1:443";
    
//    app().addListener("0.0.0.0",443) //Set HTTP listener address and port
//       .setClientMaxBodySize(20 * 2000 * 2000);

    app().loadConfigFile("./facade_config.json")  //Load config file
    .run();
    //Run HTTP framework,the method will block in the internal event loop
    return 0;
}
