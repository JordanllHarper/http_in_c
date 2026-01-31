#define WIN32_LEAN_AND_MEAN

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iphlpapi.h>
#include <sec_api/string_s.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

// HTTP in C
// NOTE: Optional means value can be NULL
const size_t nullTermSize = 1;
// Represents a query parameter in a URI
typedef struct Query {
    // e.g. name
    char const *key;
    // e.g. somename
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
    //
    // Either http or https
    char const *scheme;
    // Identifier of the host - can be IP
    //
    // e.g. www.something.com, 127.0.0.1
    char const *hostIdentifier;
    // Port number of target
    //
    // Optional
    //
    // Interpreted as uint_16
    //
    // e.g."8080"
    char const *port;
    // Sub path from the hostIdentifier
    char const *path;
    // List of query parameters
    //
    // Optional
    Query *query;
    // Method
    //
    // GET, POST, etc.
    char const *method;
    // Version
    //
    // e.g. HTTP/1.1
    char const *protocolVersion;
    // The additional headers to add onto the request
    //
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

// TODO: Check for userinfo in url and treat as error - deprecated and malicious
// TODO: Multiple headers with same key are concatenated together

size_t sizeHeaderContent(Header h) {
    return (strlen(h.key) + strlen(":") + strlen(" ") + strlen(h.value));
}

char *serializeHeader(Header h) {
    size_t headerSize = sizeHeaderContent(h) + strlen("\r\n") + nullTermSize;
    char *headerContent = (char *)malloc(headerSize);
    if (!headerContent) {
        printf("Failed to allocate memory in serializeHeader\n");
        return NULL;
    }
    int written =
        snprintf(headerContent, headerSize, "%s: %s\r\n", h.key, h.value);
    if (written < 0) {
        printf("Failed to encode header\n");
        return NULL;
    }
    return headerContent;
}

char *serializeControlData(char const *method, char const *path,
                           char const *version) {
    size_t controlDataSize =
        strlen(method) + strlen(path) + strlen(version) + 3;
    char *controlData = (char *)malloc(controlDataSize);
    if (!controlData) {
        printf("Failed to allocate memory in serializeControlData\n");
        return NULL;
    }
    int written = snprintf(controlData, controlDataSize, "%s %s %s", method,
                           path, version);
    if (written < 0) {
        printf("Control code error\r\n");
        return NULL;
    }
    return controlData;
}

size_t sizeHeadersContent(Header *headers, size_t len) {
    size_t s = 0;
    for (int i = 0; i < len; i++) {
        s += sizeHeaderContent(headers[i]);
    }
    return s;
}

char *serializeHeaders(Header *headers, size_t len) {
    size_t headersContentSize = sizeHeadersContent(headers, len);
    size_t headersSize =
        headersContentSize + (len * strlen("\r\n")) + nullTermSize;

    char *serial = (char *)calloc(headersSize, sizeof(char));

    for (int i = 0; i < len; i++) {
        char *headerContent = serializeHeader(headers[i]);
        printf("%s\r\n", headerContent);
        if (!headerContent) {
            printf("Error from serialize header\n");
            return NULL;
        }
        int badRes = strcat_s(serial, headersSize, headerContent);
        if (badRes) {
            printf("Error from strcat_s\n");
            return NULL;
        }
    }
    return serial;
}

char const *sendRequest(Request request) {
    char *control = serializeControlData(request.method, request.path,
                                         request.protocolVersion);

    Header headers[] = {Header{
        "Host",
        request.hostIdentifier,
    }};
    size_t headersLen = sizeof(headers) / sizeof(headers[0]);
    char *header = serializeHeaders(headers, headersLen);
    if (!header) {
        printf("Error from serializeHeaders\n");
        return NULL;
    }
    size_t len = strlen(control) + strlen("\r\n") + strlen(header) +
                 strlen("\r\n") + nullTermSize;
    char *msgBuf = (char *)malloc(len);
    int written = snprintf(msgBuf, len, "%s\r\n%s\r\n", control, header);
    char msg[strlen(msgBuf)];
    memcpy(msg, msgBuf, strlen(msgBuf) + 3);
    printf("%s", msg);
    printf("----\r\n");

    struct addrinfo *result = NULL, *ptr = NULL, hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    printf("Sending to: %s:%s\n", request.hostIdentifier, request.port);

    int iResult =
        getaddrinfo(request.hostIdentifier, request.port, &hints, &result);
    if (iResult) {
        printf("getaddrinfo failure: %d\n", iResult);
        WSACleanup();
        return NULL;
    }
    SOCKET connectSocket = INVALID_SOCKET;
    ptr = result;
    connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if (connectSocket == INVALID_SOCKET) {
        printf("Error at socket(): %d\n", WSAGetLastError());
        freeaddrinfo(result);
        return NULL;
    }
    iResult = connect(connectSocket, ptr->ai_addr, ptr->ai_addrlen);

    if (iResult == SOCKET_ERROR) {
        closesocket(connectSocket);
        connectSocket = INVALID_SOCKET;
    }

    freeaddrinfo(result);

    if (connectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return NULL;
    }
    printf("Successful connection to socket was made!\n");

    iResult = send(connectSocket, msg, strlen(msg) + 1, 0);
    if (iResult == SOCKET_ERROR) {
        printf("send Failed %d\n", WSAGetLastError());
        closesocket(connectSocket);
        WSACleanup();
        return NULL;
    }

    printf("Bytes sent: %d\n", iResult);
    iResult = shutdown(connectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed: %d\n", WSAGetLastError());
        closesocket(connectSocket);
        return NULL;
    }

    char *recvbuf = (char *)calloc(512, sizeof(char));
    if (!recvbuf) {
        printf("Error allocating memory in sendRequest()\n");
        return NULL;
    }
    do {
        iResult = recv(connectSocket, recvbuf, 512, 0);
        if (iResult > 0) {
            printf("Bytes received: %d\n", iResult);
        } else if (iResult == 0) {
            printf("Connection closed\n");
        } else {
            printf("recv failed: %d\n", WSAGetLastError());
        }
    } while (iResult > 0);
    closesocket(connectSocket);

    return recvbuf;
}

int main(int argc, char *argv[]) {
    printf("\r\n");

    // Setup of winsock
    WSADATA wsadata;

    int iResult = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }

    char const *body = "Hello, World!\r\n";

    Header headers[] = {
        Header{"Accept-Language", "en"},
        Header{"User-Agent", "httpinc/0.1"},
        Header{"Accept", "*/*"},
        Header{"Accept-Encoding", "gzip, deflate, br, zstd"},
        Header{"Upgrade-Insecure-Requests", "1"},
        Header{"Connection", "keep-alive"},

    };
    // size_t headersLen = sizeof(headers) / sizeof(headers[0]);
    Request request = {
        .scheme = "http",
        .hostIdentifier = "127.0.0.1",
        .port = "8080",
        .path = "/hello",
        .query = NULL,
        .method = "GET",
        .protocolVersion = "HTTP/1.1",
        .additionalHeaders = headers,
        .additionalHeadersLen = 0,
        .contentType = "text/plain; charset=utf-8",
        .body = NULL,
    };
    char const *rawResponse = sendRequest(request);
    if (!rawResponse) {
        printf("Error!\n");
        return 1;
    }
    printf("%s\n", rawResponse);

    return 0;
}
