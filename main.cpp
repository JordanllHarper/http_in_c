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
    // Size of the additional headers content
    // e.g. sizeof("Content-Type")
    size_t additionalHeadersSize;
    // The body of the request
    char const *body;
} Request;

typedef struct Response {
    // TODO: properties of a response
} Response;

// TODO: Check for userinfo in url and treat as error - deprecated and malicious
// TODO: Multiple headers with same key are concatenated together

size_t measureHeader(Header h) { return strlen(h.key) + strlen(h.value) + 3; }

char *formatHeader(Header h) {
    const char *key = h.key;
    size_t keyLen = strlen(key);
    const char *value = h.value;
    size_t valueLen = strlen(value);

    size_t lineSize = keyLen + valueLen + 3;
    char *line = (char *)malloc(lineSize);
    int headerErr = snprintf(line, lineSize, "%s: %s\n", key, value);
    if (headerErr < 0) {
        return NULL;
    }
    return line;
}

// Format the given `Request` into an HTTP message.
// If an error occurs, this function will return NULL.
// Caller owns memory.
char *format(Request request) {
    char const *method = request.method;
    size_t methodLen = strlen(method);
    char const *path = request.path;
    size_t pathLen = strlen(path);
    char const *version = request.protocolVersion;
    size_t versionLen = strlen(version);

    size_t controlDataLen = (methodLen + pathLen + versionLen + 3);

    char *controlData = (char *)malloc(controlDataLen);

    int controlCode = snprintf(controlData, controlDataLen, "%s %s %s\n",
                               method, path, version);
    if (controlCode < 0) {
        printf("Control code error\n");
        return NULL;
    }

    char *headerData = (char *)malloc(request.additionalHeadersSize);

    int headerErr = snprintf(headerData, request.additionalHeadersSize, "%s\n",
                             formatHeader(request.additionalHeaders[0]));

    if (headerErr < 0) {
        printf("Parsing header data error\n");
        return NULL;
    }

    // offset by as we handled previous header outside loop
    for (int i = 1; i < request.additionalHeadersLen; i++) {
        Header h = request.additionalHeaders[i];
        char *line = formatHeader(h);
        headerErr = snprintf(headerData, request.additionalHeadersSize,
                             "%s%s\n", headerData, line);
        if (strcmp(h.key, "Content-Length") == 0) {
            continue;
        }
        if (headerErr < 0) {
            printf("Parsing header data error\n");
            return NULL;
        }
    }

    size_t bodySize = strlen(request.body);

    int size = (int)(ceil(log10(bodySize)) + 1);

    char *bodySizeBuf = (char *)malloc(size);
    sprintf(bodySizeBuf, "%zu", bodySize);
    Header contentTypeHeader =
        Header{.key = "Content-Length", .value = bodySizeBuf};

    size_t contentTypeHeaderSize = measureHeader(contentTypeHeader);
    char *contentTypeHeaderStr = formatHeader(contentTypeHeader);

    size_t totalLen =
        controlDataLen + request.additionalHeadersSize + contentTypeHeaderSize;
    char *buf = (char *)malloc(totalLen);
    int concatCode = snprintf(buf, totalLen, "%s\n%s\n%s", controlData,
                              contentTypeHeaderStr, headerData);
    if (concatCode < 0) {
        printf("Concat code error\n");
        return NULL;
    }

    free(controlData);
    free(headerData);

    return buf;
}

int main(int argc, char *argv[]) {
    // expected:
    // GET /testing HTTP/1.1
    // Host: www.example.com
    // User-Agent: HttpInC
    // Accept-Language: en
    Header h1 = Header{"User-Agent", "HttpInC"};
    Header h2 = Header{"Accept-Language", "en"};

    size_t headersSize = measureHeader(h1) + measureHeader(h2);
    size_t headersLen = 2;
    Header headers[] = {h1, h2};
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
        .additionalHeadersSize = headersSize,
        .body = NULL,
    };
    char *msg = format(request);
    if (msg == NULL) {
        printf("Error!");
        return 0;
    }
    printf("%s\n", msg);

    return 0;
}
