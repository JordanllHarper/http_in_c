#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdint.h>
#include <stdio.h>

// HTTP in C
// NOTE: Optional means value can be NULL

typedef char const *Method;
typedef char const *ProtocolVersion;
typedef char const *Scheme;
typedef char const *HostIdentifier;
typedef char const *Path;
typedef int16_t Port;

typedef struct Query {
    char const *key;
    char const *value;
} Query;

typedef struct Header {
    // e.g. Content-Type:
    char const *key;
    // e.g. application/json
    char const *value;
} Header;

typedef struct Authority {
    // e.g. something.com
    HostIdentifier hostIdentifier;
    // Port number of target
    // Optional
    // e.g.8080
    Port *port;
} Authority;

// A complete HTTP URI
// Full example: http://www.someone.com:80/some/path?with=query
typedef struct HttpUri {
    // HTTP scheme
    // Either http or https
    Scheme scheme;
    // Authority - thought of as Host
    // e.g. someone.com
    Authority authority;
    // Path
    Path path;
    // List of query parameters
    // Optional
    Query *query;
} HttpUri;

typedef struct Request {
    Method method;
    ProtocolVersion protocolVersion;
    HttpUri httpUri;
    Header *additionalHeaders;
    size_t headersLen;
    char const *content;
} Request;

typedef struct Response {
    // TODO: properties of a response
} Response;

// TODO: Check for userinfo in url and treat as error - deprecated and malicious
// TODO: Multiple headers with same key are concatenated together

// Format the given `Request` into an HTTP message.
// If an error occurs, this function will return NULL.
// Caller owns memory.
char *format(Request request) {
    char const *method = request.method;
    size_t methodLen = strlen(method);
    char const *path = request.httpUri.path;
    size_t pathLen = strlen(path);
    char const *version = request.protocolVersion;
    size_t versionLen = strlen(version);

    size_t controlDataLen = methodLen + pathLen + versionLen + 3;

    size_t headersContentLen = 0;

    // TODO: We don't know here how much memory we need to allocate to hold all
    // the items so we'll probably need some dynamic allocation
    // char *headerData = malloc(size_t size);
    for (int i = 0; i < request.headersLen; i++) {
        Header h = request.additionalHeaders[i];

        const char *key = h.key;
        size_t keyLen = strlen(key);
        const char *value = h.value;
        size_t valueLen = strlen(value);

        // +2 for colon and extra space
        headersContentLen += keyLen + valueLen + 2;
    }

    char *controlData = (char *)malloc(controlDataLen);

    int code = snprintf(controlData, controlDataLen, "%s %s %s\n", method, path,
                        version);

    if (code < 0) {
        // error - handle and print
        return NULL;
    }
    return controlData;
}

int main(int argc, char *argv[]) {
    // expected:
    // GET /testing HTTP/1.1
    // Host: www.example.com
    // User-Agent: HttpInC
    // Accept-Language: en
    Authority authority = {.hostIdentifier = "www.example.com", .port = NULL};
    HttpUri httpUri = {.scheme = "http",
                       .authority = authority,
                       .path = "/testing",
                       .query = NULL};
    Header headers[] = {Header{.key = "User-Agent", .value = "HttpInC"},
                        Header{.key = "Accept-Language", .value = "en"}};
    Request request = {
        .method = "GET",
        .protocolVersion = "HTTP/1.1",
        .httpUri = httpUri,
        .additionalHeaders = headers,
        .headersLen = 2,
        .content = NULL,
    };
    char *str = format(request);
    printf("%s\n", str);

    return 0;
}
