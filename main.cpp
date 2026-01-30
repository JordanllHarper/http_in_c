#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// HTTP in C
// NOTE: Optional means value can be NULL

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

typedef struct Request {
    // HTTP scheme
    // Either http or https
    char const *scheme;
    // Identifier of the host - can be IP
    // e.g. www.something.com
    char const *hostIdentifier;
    // Port number of target
    // Optional
    // e.g.8080
    uint16_t port;
    // Sub path from the hostIdentifier
    char const *path;
    // List of query parameters
    // Optional
    Query *query;
    // Method - GET, POST, etc.
    char const *method;
    // Version - HTTP/1.1
    char const *protocolVersion;
    // The additional headers to add onto the request
    // e.g. User-Agent: HttpInC
    Header *additionalHeaders;
    // Number of headers given
    size_t additionalHeadersLen;
    // The content type of the body
    const char *contentType;
    // The body of the request
    char const *body;

} Request;

typedef struct Response {
    // TODO: properties of a response
} Response;

const size_t newlineSize = 2;

// TODO: Check for userinfo in url and treat as error - deprecated and malicious
// TODO: Multiple headers with same key are concatenated together

size_t getSize(Header h) { return (strlen(h.key) + strlen(h.value)) + 6; }

size_t getSize(Header headers[], size_t headerLen) {
    size_t size = 0;
    for (int i = 0; i < headerLen; i++) {
        size += getSize(headers[i]);
    }
    return size;
}

char *serialize(Header h) {
    size_t headerContentSize = getSize(h);
    char *headerContent = (char *)malloc(headerContentSize);
    int headerErr = snprintf(headerContent, headerContentSize, "%s: %s\r\n",
                             h.key, h.value);
    if (headerErr < 0) {
        return NULL;
    }
    return headerContent;
}

char *serialize(Header *headers, size_t headersLen) {
    size_t headersSize = getSize(headers, headersLen);
    char *headerData = (char *)malloc(headersSize);
    char *first = serialize(headers[0]);
    snprintf(headerData, headersSize, "%s", first);

    for (int i = 1; i < headersLen; i++) {
        Header h = headers[i];
        char *line = serialize(h);
        int headerErr =
            snprintf(headerData, headersSize, "%s%s", headerData, line);
        if (headerErr < 0) {
            printf("Parsing header data error\n");
            return NULL;
        }
    }
    return headerData;
}

char *getContentLength(char const *body) {
    size_t contentLength = strlen(body);
    size_t toAlloc = (size_t)(ceil(log10(strlen(body))) + 1) * sizeof(char);
    char *contentLengthHeaderContent = (char *)malloc(toAlloc);
    snprintf(contentLengthHeaderContent, toAlloc, "%zu", contentLength);
    return contentLengthHeaderContent;
}

char *serializeControlData(char const *method, char const *path,
                           char const *version) {
    const int controlDataSizeOverhead = 3;
    size_t controlDataSize = (strlen(method) + strlen(path) + strlen(version) +
                              controlDataSizeOverhead);
    char *controlData = (char *)malloc(controlDataSize);
    int controlCode = snprintf(controlData, controlDataSize, "%s %s %s\r\n",
                               method, path, version);
    if (controlCode < 0) {
        printf("Control code error\r\n");
        return NULL;
    }
    return controlData;
}

char *serializeRequestHeaders(const char *contentType, const char *body,
                              Header *additionalHeaders,
                              size_t additionalHeadersLen) {
    const int mandatoryHeadersLen = 2;
    size_t totalHeadersLen = additionalHeadersLen + mandatoryHeadersLen;

    Header contentTypeHeader = Header{"Content-Type", contentType};
    char const *contentLength = "0";
    if (body != NULL) {
        contentLength = getContentLength(body);
    }
    Header contentLengthHeader = Header{"Content-Length", contentLength};
    Header mandatoryHeaders[] = {contentTypeHeader, contentLengthHeader};
    char *mandatoryHeadersData =
        serialize(mandatoryHeaders, mandatoryHeadersLen);
    if (additionalHeadersLen != 0) {
        char *additionalHeaderData =
            serialize(additionalHeaders, additionalHeadersLen);
        if (additionalHeaderData == NULL) {
            printf("Serialize headers error: header data was NULL but "
                   "additional headers len was set > 0 \n");
            return NULL;
        }
        size_t totalHeadersDataSize =
            strlen(mandatoryHeadersData) + strlen(additionalHeaderData) + 4;
        char *totalHeadersData = (char *)malloc(totalHeadersDataSize);
        snprintf(totalHeadersData, totalHeadersDataSize, "%s%s",
                 mandatoryHeadersData, additionalHeaderData);
        return totalHeadersData;
    } else {
        return mandatoryHeadersData;
    }
}

// Serialize the given `Request` into an HTTP message.
// If an error occurs, this function will return NULL.
// Caller owns memory.
char *serializeHttpRequest(Request request) {
    char *controlData = serializeControlData(request.method, request.path,
                                             request.protocolVersion);
    if (controlData == NULL) {
        printf("SerializeControlData error: controlData was NULL\n");
        return NULL;
    }

    char *headerData = serializeRequestHeaders(
        request.contentType, request.body, request.additionalHeaders,
        request.additionalHeadersLen);

    size_t bodySize = 0;
    if (request.body != NULL) {
        // for the 2 new lines in the print
        bodySize = strlen(request.body) + 2;
    }
    size_t totalSize = strlen(controlData) + strlen(headerData) + bodySize;

    char *msg = (char *)malloc(totalSize);
    int msgCode;
    if (request.body != NULL) {
        msgCode = snprintf(msg, totalSize + 6, "%s\r\n%s\r\n%s\r\n",
                           controlData, headerData, request.body);
    } else {
        msgCode = snprintf(msg, totalSize + 4, "%s\r\n%s\r\n", controlData,
                           headerData);
    }

    if (msgCode < 0) {
        printf("Concat code error\n");
        return NULL;
    }

    free(controlData);
    free(headerData);

    return msg;
}

int main(int argc, char *argv[]) {
    printf("\n");

    char const *body = "Hello, World!";

    Header h1 = Header{"User-Agent", "HttpInC"};
    Header h2 = Header{"Accept-Language", "en"};

    Header headers[] = {h1, h2};
    size_t headersLen = sizeof(headers) / sizeof(headers[0]);
    Request request = {
        .scheme = "http",
        .hostIdentifier = "www.example.com",
        .port = 80,
        .path = "/testing",
        .query = NULL,
        .method = "GET",
        .protocolVersion = "HTTP/1.1",
        .additionalHeaders = headers,
        .additionalHeadersLen = headersLen,
        .contentType = "text/plain",
        .body = NULL,

    };
    char *msg = serializeHttpRequest(request);
    if (msg == NULL) {
        printf("Error!");
        return 0;
    }
    printf("%s\n\n", msg);
    free(msg);

    return 0;
}
