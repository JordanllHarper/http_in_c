#include <cstdio>
#include <stdint.h>

// HTTP in C
// NOTE: Optional means value can be NULL

typedef char *Verb;
typedef char *Version;
typedef char *Scheme;
typedef char *HostIdentifier;
typedef char *Path;
typedef int16_t Port;

typedef struct Query {
    char *key;
    char *value;
} Query;

typedef struct Header {
    char *key;
    char *value;
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
    // TODO: properties of a request
} Request;

typedef struct Response {
    // TODO: properties of a response
} Response;

// TODO: Check for userinfo in url and treat as error - deprecated and malicious

int main(int argc, char *argv[]) {
    printf("Hello, World\n");
    return 0;
}
