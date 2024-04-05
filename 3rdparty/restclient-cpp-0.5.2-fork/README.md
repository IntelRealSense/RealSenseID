# REST client for C++
[![Build Status](https://travis-ci.org/mrtazz/restclient-cpp.svg?branch=master)](https://travis-ci.org/mrtazz/restclient-cpp)
[![Coverage Status](https://coveralls.io/repos/mrtazz/restclient-cpp/badge.svg?branch=master&service=github)](https://coveralls.io/github/mrtazz/restclient-cpp?branch=master)
[![Packagecloud](https://img.shields.io/badge/packagecloud-available-brightgreen.svg)](https://packagecloud.io/mrtazz/restclient-cpp)
[![doxygen](https://img.shields.io/badge/doxygen-reference-blue.svg)](http://code.mrtazz.com/restclient-cpp/ref/)
[![MIT license](https://img.shields.io/badge/license-MIT-blue.svg)](http://opensource.org/licenses/MIT)


## About
This is a simple REST client for C++. It wraps [libcurl][] for HTTP requests.

## Usage
restclient-cpp provides two ways of interacting with REST endpoints. There is
a simple one, which doesn't need you to configure an object to interact with
an API. However the simple way doesn't provide a lot of configuration options
either. So if you need more than just a simple HTTP call, you will probably
want to check out the advanced usage.

### Simple Usage
The simple API is just some static methods modeled after the most common HTTP
verbs:

```cpp
#include "restclient-cpp/restclient.h"

RestClient::Response r = RestClient::get("http://url.com")
RestClient::Response r = RestClient::post("http://url.com/post", "application/json", "{\"foo\": \"bla\"}")
RestClient::Response r = RestClient::put("http://url.com/put", "application/json", "{\"foo\": \"bla\"}")
RestClient::Response r = RestClient::patch("http://url.com/patch", "application/json", "{\"foo\": \"bla\"}")
RestClient::Response r = RestClient::del("http://url.com/delete")
RestClient::Response r = RestClient::head("http://url.com")
RestClient::Response r = RestClient::options("http://url.com")
```

The response is of type [RestClient::Response][restclient_response] and has
three attributes:

```cpp
RestClient::Response.code // HTTP response code
RestClient::Response.body // HTTP response body
RestClient::Response.headers // HTTP response headers
```

### Advanced Usage
However if you want more sophisticated features like connection reuse,
timeouts or authentication, there is also a different, more configurable way.

```cpp
#include "restclient-cpp/connection.h"
#include "restclient-cpp/restclient.h"

// initialize RestClient
RestClient::init();

// get a connection object
RestClient::Connection* conn = new RestClient::Connection("http://url.com");

// configure basic auth
conn->SetBasicAuth("WarMachine68", "WARMACHINEROX");

// set connection timeout to 5s
conn->SetTimeout(5);

// set custom user agent
// (this will result in the UA "foo/cool restclient-cpp/VERSION")
conn->SetUserAgent("foo/cool");

// enable following of redirects (default is off)
conn->FollowRedirects(true);
// and limit the number of redirects (default is -1, unlimited)
conn->FollowRedirects(true, 3);

// set headers
RestClient::HeaderFields headers;
headers["Accept"] = "application/json";
conn->SetHeaders(headers)

// append additional headers
conn->AppendHeader("X-MY-HEADER", "foo")

// if using a non-standard Certificate Authority (CA) trust file
conn->SetCAInfoFilePath("/etc/custom-ca.crt")

RestClient::Response r = conn->get("/get")
RestClient::Response r = conn->head("/get")
RestClient::Response r = conn->del("/delete")
RestClient::Response r = conn->options("/options")

// set different content header for POST, PUT and PATCH
conn->AppendHeader("Content-Type", "application/json")
RestClient::Response r = conn->post("/post", "{\"foo\": \"bla\"}")
RestClient::Response r = conn->put("/put", "application/json", "{\"foo\": \"bla\"}")
RestClient::Response r = conn->patch("/patch", "text/plain", "foobar")

// deinit RestClient. After calling this you have to call RestClient::init()
// again before you can use it
RestClient::disable();
```

The responses are again of type [RestClient::Response][restclient_response]
and have three attributes:

```cpp
RestClient::Response.code // HTTP response code
RestClient::Response.body // HTTP response body
RestClient::Response.headers // HTTP response headers
```

The connection object also provides a simple way to get some diagnostics and
metrics information via `conn->GetInfo()`. The result is a
`RestClient::Connection::Info` struct and looks like this:

```cpp
typedef struct {
  std::string base_url;
  RestClients::HeaderFields headers;
  int timeout;
  struct {
    std::string username;
    std::string password;
  } basicAuth;

  std::string certPath;
  std::string certType;
  std::string keyPath;
  std::string keyPassword;
  std::string customUserAgent;
  std::string uriProxy;
  struct {
    // total time of the last request in seconds Total time of previous
    // transfer. See CURLINFO_TOTAL_TIME
    int totalTime;
    // time spent in DNS lookup in seconds Time from start until name
    // resolving completed. See CURLINFO_NAMELOOKUP_TIME
    int nameLookupTime;
    // time it took until Time from start until remote host or proxy
    // completed. See CURLINFO_CONNECT_TIME
    int connectTime;
    // Time from start until SSL/SSH handshake completed. See
    // CURLINFO_APPCONNECT_TIME
    int appConnectTime;
    // Time from start until just before the transfer begins. See
    // CURLINFO_PRETRANSFER_TIME
    int preTransferTime;
    // Time from start until just when the first byte is received. See
    // CURLINFO_STARTTRANSFER_TIME
    int startTransferTime;
    // Time taken for all redirect steps before the final transfer. See
    // CURLINFO_REDIRECT_TIME
    int redirectTime;
    // number of redirects followed. See CURLINFO_REDIRECT_COUNT
    int redirectCount;
  } lastRequest;
} Info;
```

#### Persistent connections/Keep-Alive
The connection object stores the curl easy handle in an instance variable and
uses that for the lifetime of the object. This means curl will [automatically
reuse connections][curl_keepalive] made with that handle.


## Thread Safety
restclient-cpp leans heavily on libcurl as it aims to provide a thin wrapper
around it. This means it adheres to the basic level of thread safety [provided
by libcurl][curl_threadsafety]. The `RestClient::init()` and
`RestClient::disable()` methods basically correspond to `curl_global_init` and
`curl_global_cleanup` and thus need to be called right at the beginning of
your program and before shutdown respectively. These set up the environment
and are **not thread-safe**. After that you can create connection objects in
your threads. Do not share connection objects across threads as this would
mean accessing curl handles from multiple threads at the same time which is
not allowed.

The connection level method SetNoSignal can be set to skip all signal handling. This is important in multi-threaded applications as DNS resolution timeouts use signals. The signal handlers quite readily get executed on other threads. Note that with this option DNS resolution timeouts do not work. If you have crashes in your multi-threaded executable that appear to be in DNS resolution, this is probably why.

In order to provide an easy to use API, the simple usage via the static
methods implicitly calls the curl global functions and is therefore also **not
thread-safe**.

## HTTPS User Certificate

Simple wrapper functions are provided to allow clients to authenticate using certificates.
Under the hood these wrappers set cURL options, e.g. `CURLOPT_SSLCERT`, using `curl_easy_setopt`.
Note: currently `libcurl` compiled with `gnutls` (e.g. `libcurl4-gnutls-dev` on
ubuntu) is buggy in that it returns a wrong error code when these options are set to invalid values.

```cpp
// set CURLOPT_SSLCERT
conn->SetCertPath(certPath);
// set CURLOPT_SSLCERTTYPE
conn->SetCertType(type);
// set CURLOPT_SSLKEY
conn->SetKeyPath(keyPath);
// set CURLOPT_KEYPASSWD
conn->SetKeyPassword(keyPassword);
```

## HTTP Proxy Tunneling Support

An HTTP Proxy can be set to use for the upcoming request.
To specify a port number, append :[port] to the end of the host name. If not specified, `libcurl` will default to using port 1080 for proxies. The proxy string may be prefixed with `http://` or `https://`. If no HTTP(S) scheme is specified, the address provided to `libcurl` will be prefixed with `http://` to specify an HTTP proxy. A proxy host string can embedded user + password.
The operation will be tunneled through the proxy as curl option `CURLOPT_HTTPPROXYTUNNEL` is enabled by default.
A numerical IPv6 address must be written within [brackets].

```cpp
// set CURLOPT_PROXY
conn->SetProxy("https://37.187.100.23:3128");
/* or you can set it without the protocol scheme and
http:// will be prefixed by default */
conn->SetProxy("37.187.100.23:3128");
/* the following request will be tunneled through the proxy */
RestClient::Response res = conn->get("/get");
```

## Unix Socket Support

- https://docs.docker.com/develop/sdk/examples/
- $ curl --unix-socket /var/run/docker.sock http:/v1.24/containers/json

Note that the URL used with a unix socket has only ONE leading forward slash.

```cpp
RestClient::Connection* conn = new RestClient::Connection("http:/v1.30");
conn->SetUnixSocketPath("/var/run/docker.sock");
RestClient::HeaderFields headers;
headers["Accept"] = "application/json; charset=UTF-8";
headers["Expect"] = "";
conn->SetHeaders(headers);
auto resp = conn->get("/images/json");
```

## Dependencies
- [libcurl][]

## Installation
There are some packages available for Linux on [packagecloud][packagecloud].
And for OSX you can get it from the mrtazz/oss homebrew tap:

```bash
brew tap mrtazz/oss
brew install restclient-cpp
```

Otherwise you can do the regular autotools dance:

```bash
./autogen.sh
./configure
make install
```

## Contribute
All contributions are highly appreciated. This includes filing issues,
updating documentation and writing code. Please take a look at the
[contributing guidelines][contributing] before so your contribution can be
merged as fast as possible.


[libcurl]: http://curl.haxx.se/libcurl/
[gtest]: http://code.google.com/p/googletest/
[packagecloud]: https://packagecloud.io/mrtazz/restclient-cpp
[contributing]: https://github.com/mrtazz/restclient-cpp/blob/master/.github/CONTRIBUTING.md
[curl_keepalive]: http://curl.haxx.se/docs/faq.html#What_about_Keep_Alive_or_persist
[curl_threadsafety]: http://curl.haxx.se/libcurl/c/threadsafe.html
[restclient_response]: http://code.mrtazz.com/restclient-cpp/ref/struct_rest_client_1_1_response.html
