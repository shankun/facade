#pragma once

#include <json/json.h>
#include <trantor/utils/Logger.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpClient.h>
#include <functional>
#ifndef _MSC_VER
#include <cxxabi.h>
#endif

namespace dailyhot
{
class FeedObjBase;
using FeedAllocFunc = std::function<FeedObjBase *()>;
using FeedSharedAllocFunc = std::function<std::shared_ptr<FeedObjBase>()>;

/**
 * @brief A map class which can create FeedObjects from names.
 */
class FeedClassMap
{
  public:
    /**
     * @brief Register a class into the map
     *
     * @param className The name of the class
     * @param func The function which can create a new instance of the class.
     */
    static void registerClass(const std::string &className,
                              const FeedAllocFunc &func,
                              const FeedSharedAllocFunc &sharedFunc = nullptr);

    /**
     * @brief Create a new instance of the class named by className
     *
     * @param className The name of the class
     * @return FeedObjBase* The pointer to the newly created instance.
     */
    static FeedObjBase* newObject(const std::string &className);

    /**
     * @brief Get the shared_ptr instance of the class named by className
     */
    static std::shared_ptr<FeedObjBase> newSharedObject(
        const std::string &className);

    /**
     * @brief Get the singleton object of the class named by className
     *
     * @param className The name of the class
     * @return const std::shared_ptr<FeedObjBase>& The smart pointer to the
     * instance.
     */
    static const std::shared_ptr<FeedObjBase> &getSingleInstance(
        const std::string &className);

    /**
     * @brief Get the singleton T type object
     *
     * @tparam T The type of the class
     * @return std::shared_ptr<T> The smart pointer to the instance.
     * @note The T must be a subclass of the FeedObjBase class.
     */
    template <typename T>
    static std::shared_ptr<T> getSingleInstance()
    {
        static_assert(std::is_base_of<FeedObjBase, T>::value,
                      "T must be a sub-class of FeedObjBase");
        static auto const singleton =
            std::dynamic_pointer_cast<T>(getSingleInstance(T::classTypeName()));
        assert(singleton);
        return singleton;
    }

    /**
     * @brief Set a singleton object into the map.
     *
     * @param ins The smart pointer to the instance.
     */
    static void setSingleInstance(const std::shared_ptr<FeedObjBase> &ins);

    /**
     * @brief Get all names of classes registered in the map.
     *
     * @return std::vector<std::string> the vector of class names.
     */
    static std::vector<std::string> getAllClassName();

    static bool containClass(const std::string& className);

    /**
     * @brief demangle the type name which is returned by typeid(T).name().
     *
     * @param mangled_name The type name which is returned by typeid(T).name().
     * @return std::string The human readable type name.
     */
    static std::string demangle(const char *mangled_name)
    {
#ifndef _MSC_VER
        std::size_t len = 0;
        int status = 0;
        std::unique_ptr<char, decltype(&std::free)> ptr(
            __cxxabiv1::__cxa_demangle(mangled_name, nullptr, &len, &status),
            &std::free);
        if (status == 0)
        {
            return std::string(ptr.get());
        }
        LOG_ERROR << "Demangle error!";
        return "";
#else
        auto pos = strstr(mangled_name, " ");
        if (pos == nullptr)
            return std::string{mangled_name};
        else
            return std::string{pos + 1};
#endif
    }

  protected:
    static std::unordered_map<std::string,
                              std::pair<FeedAllocFunc, FeedSharedAllocFunc>>
        &getMap();
};

/**
 * @brief The base class for all NewsFeed reflection classes.
 *
 */
class FeedObjBase
{
  public:
    /**
     * @brief Get the class name
     *
     * @return const std::string& the class name
     */
    virtual const std::string &className() const
    {
        static const std::string name{"FeedObjBase"};
        return name;
    }

    /**
     * @brief Return true if the class name is 'class_name'
     */
    virtual bool isClass(const std::string &class_name) const
    {
        return (className() == class_name);
    }

    virtual ~FeedObjBase()
    {
    }
};

template <typename T>
struct isAutoCreationClass
{
    template <class C>
    static constexpr auto check(C *)
        -> std::enable_if_t<std::is_same_v<decltype(C::isAutoCreation), bool>,
                            bool>
    {
        return C::isAutoCreation;
    }

    template <typename>
    static constexpr bool check(...)
    {
        return false;
    }

    static constexpr bool value = check<T>(nullptr);
};

/**
 * a class template to
 * implement the reflection function of creating the class object by class name
 */
template <typename T>
class FeedObject : public virtual FeedObjBase
{
  public:
    const std::string &className() const override
    {
        return alloc_.className();
    }

    static const std::string &classTypeName()
    {
        return alloc_.className();
    }

    bool isClass(const std::string &class_name) const override
    {
        return (className() == class_name);
    }

  protected:
    // protect constructor to make this class only inheritable
    FeedObject() = default;
    ~FeedObject() override = default;

  private:
    class FeedAllocator
    {
      public:
        FeedAllocator()
        {
            registerClass<T>();
        }

        const std::string &className() const
        {
            static std::string className =
                FeedClassMap::demangle(typeid(T).name());
            return className;
        }

        template <typename D>
        void registerClass()
        {
            if constexpr (std::is_default_constructible<D>::value)
            {
                FeedClassMap::registerClass(
                    className(),
                    []() -> FeedObjBase * { return new T; },
                    []() -> std::shared_ptr<FeedObjBase> {
                        return std::make_shared<T>();
                    });
            }
            else if constexpr (isAutoCreationClass<D>::value)
            {
                static_assert(std::is_default_constructible<D>::value,
                              "Class is not default constructable!");
            }
        }
    };

    // use static val to register allocator function for class T;
    static FeedAllocator alloc_;
};

template <typename T>
typename FeedObject<T>::FeedAllocator FeedObject<T>::alloc_;

}

class WebCrawler : public virtual dailyhot::FeedObjBase
{
  public:
    void Save(const std::string& key_str, Json::Value jsonData);
    
    void UpdateCache();

    // Extract the content between pre and suf from decoratedStr
    std::string ExtractContent(const std::string& decoratedStr, 
        const std::string& pre, const std::string& suf) const;

    virtual drogon::HttpRequestPtr CreateRequest(const drogon::HttpClientPtr& client) const = 0;
	
	virtual Json::Value ParseData(const drogon::HttpResponsePtr& pResp) const = 0;

    virtual void SetParameter(const std::string& subtype) = 0;

	virtual std::string srcURL() const;

    virtual ~WebCrawler()
    {
    }

    struct NewsItem {
        std::string key;
        Json::Value data;
        std::mutex mtx;
    };

    static std::string s_dbName;
    std::string m_parameter;
    // 所有新闻来源
    static std::map<std::string, Json::Value> s_allNewsSrc;

private:
    const std::string m_paraMark = "{parameter}";
    NewsItem m_newData;
};

