/*
author          Oliver Blaser
date            01.03.2025
copyright       MIT - Copyright (c) 2025 Oliver Blaser
*/

#ifndef IG_CURLTHREAD_TYPES_H
#define IG_CURLTHREAD_TYPES_H

#include <string>
#include <vector>


namespace curl {

enum class Method
{
    GET = 0,
    POST,
};

enum class Priority
{
    // min,
    // low,
    normal,
    high,
    max
};

class QueueId
{
public:
    enum
    {
        FAILED = -2,
        NONE = -1,
        BASE = 1,    ///< The first assigned queue id
        MAX = 32767, ///< The last assigned queue id
    };

    using id_type = int;

public:
    QueueId()
        : m_id(QueueId::NONE)
    {}

    QueueId(id_type id)
        : m_id(id)
    {}

    virtual ~QueueId() {}

    bool isValid() const { return (m_id >= QueueId::BASE) && (m_id <= QueueId::MAX); }

    std::string toString() const;

    operator id_type() const { return m_id; }

private:
    id_type m_id;
};

class HeaderField
{
private:
    static const char* const delimiter;

public:
    HeaderField() = delete;

    HeaderField(const std::string& key, const std::string& value)
        : m_curlStr(key + delimiter + value)
    {}

    virtual ~HeaderField() {}

    bool empty() const { return (m_curlStr == delimiter); }

    std::string key() const;
    std::string value() const;

    const char* curlStr() const { return m_curlStr.c_str(); }

private:
    std::string m_curlStr;
};

class Request
{
public:
    static const char* const defaultUserAgent;

public:
    Request() = delete;

    Request(const Method& method, const std::string& url, long connectTimeout = 0, long totalTimeout = 0, const std::string& userAgent = defaultUserAgent)
        : m_method(method), m_url(url), m_connectTimeout(connectTimeout), m_totalTimeout(totalTimeout), m_userAgent(userAgent), m_header(), m_body()
    {}

    virtual ~Request() {}

    const Method& method() const { return m_method; }
    const std::string& url() const { return m_url; }
    long connectTimeout() const { return m_connectTimeout; }
    long totalTimeout() const { return m_totalTimeout; }
    const std::string& userAgent() const { return m_userAgent; }
    const std::vector<HeaderField>& header() const { return m_header; }
    const std::string& body() const { return m_body; }

    void setBody(const std::string& body) { m_body = body; }
    void setHeader(const std::vector<HeaderField>& header) { m_header = header; }
    void addHeaderField(const HeaderField& headerField) { m_header.push_back(headerField); }

    std::string toString() const;

private:
    Method m_method;
    std::string m_url;
    long m_connectTimeout;
    long m_totalTimeout;
    std::string m_userAgent;
    std::vector<HeaderField> m_header;
    std::string m_body;
};

class GetRequest : public Request
{
public:
    GetRequest() = delete;

    explicit GetRequest(const std::string& url, long connectTimeout = 0, long totalTimeout = 0, const std::string& userAgent = defaultUserAgent)
        : Request(Method::GET, url, connectTimeout, totalTimeout, userAgent)
    {}

    GetRequest(const std::string& url, long connectTimeout, long totalTimeout, const std::vector<HeaderField>& header,
               const std::string& userAgent = defaultUserAgent)
        : Request(Method::GET, url, connectTimeout, totalTimeout, userAgent)
    {
        setHeader(header);
    }

    virtual ~GetRequest() {}
};

class PostRequest : public Request
{
public:
    PostRequest() = delete;

    explicit PostRequest(const std::string& url, long connectTimeout = 0, long totalTimeout = 0, const std::string& userAgent = defaultUserAgent)
        : Request(Method::POST, url, connectTimeout, totalTimeout, userAgent)
    {}

    PostRequest(const std::string& url, long connectTimeout, long totalTimeout, const std::string& body, const std::string& userAgent = defaultUserAgent)
        : Request(Method::POST, url, connectTimeout, totalTimeout, userAgent)
    {
        setBody(body);
    }

    PostRequest(const std::string& url, long connectTimeout, long totalTimeout, const std::vector<HeaderField>& header,
                const std::string& userAgent = defaultUserAgent)
        : Request(Method::POST, url, connectTimeout, totalTimeout, userAgent)
    {
        setHeader(header);
    }

    PostRequest(const std::string& url, long connectTimeout, long totalTimeout, const std::string& body, const std::vector<HeaderField>& header,
                const std::string& userAgent = defaultUserAgent)
        : Request(Method::POST, url, connectTimeout, totalTimeout, userAgent)
    {
        setBody(body);
        setHeader(header);
    }

    virtual ~PostRequest() {}
};

class Response
{
public:
    Response()
        : m_curlCode(), m_httpCode(), m_body()
    {
        m_clear();
    }

    Response(int curlCode, int httpCode, const std::string& body)
        : m_curlCode(curlCode), m_httpCode(httpCode), m_body(body)
    {}

    virtual ~Response() {}

    int curlCode() const { return m_curlCode; }
    int httpCode() const { return m_httpCode; }
    const std::string& body() const { return m_body; }

    bool aborted() const;
    bool curlOk() const; // curlCode == CURLE_OK (0)
    bool httpOk() const { return (m_httpCode == 200); }
    bool good() const { return (curlOk() && httpOk()); }

    std::string toString() const;
    std::string toString_noBody() const;

protected:
    virtual void m_clear();

private:
    int m_curlCode;
    int m_httpCode;
    std::string m_body;
};

} // namespace curl


#endif // IG_CURLTHREAD_TYPES_H
