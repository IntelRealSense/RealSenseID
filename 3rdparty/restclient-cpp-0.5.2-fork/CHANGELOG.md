# Changelog

## v0.5.2 (12th February 2020)
- add a pkg-config setup
- Use the standard CMake-flag BUILD_SHARED_LIBS. (tastytea)
- Add CURL and Threads as dependency in CMake config. (tastytea)
- Replace hardcoded installation directories with GNUInstallDirs. (tastytea)
- Set minimum CMake version to 3.12.0 (tastytea)
- Added default compile type to CMakeLists.txt (Jorge Marizan)
- Added configurable compile type to CMakeLists.txt (Jorge Marizan)
- use lambda over ptr_fun (Cynthia Coan)
- mac os x uses glibtoolize not libtoolize (Cynthia Coan)
- Use std::make_unique when C++14 is available (Heikki Hellgren)
- fix git version problems during yocto build (Giuseppe Ursino)
- fix autogen to work with yocto (Giuseppe Ursino)
- fix relative path issues (Giuseppe Ursino)
- Added support for Unix sockets. (Dennis Jenkins)
- update CI dist to xenial (Daniel Schauenberg)

## v0.5.1 (6th May 2019)
- add initial cmake build
- switch httpbin tests over to https

## v0.5.0 (31st October 2018)

### Features
* Add PATCH and OPTIONS methods (#106) (drodil)
* Add support for limiting the number of redirects (Bas van Kervel)
* Added web proxy tunneling support. (amzoughi)
* add client cert & key support (Fan Zhang)
* add HEAD method (limhyungseok)
* no-signal capability (Chris Kruger)

### Improvements and fixes
* Remove duplicate FollowRedirects function (#109) (drodil)
* Changed content type to application/json (Hajder Rabiee)
* fix link to CONTRIBUTING guidelines (Daniel Schauenberg)
* update distro repositories to push packages to (Daniel Schauenberg)
* fix isspace is not a member of std with Visual Studio 2017 (Yoann Long)
* add CODE_OF_CONDUCT.md  (Daniel Schauenberg)
* fix linter error for redundant blank line (Daniel Schauenberg)
* remove unneeded curl include in restclient.h (Daniel Schauenberg)
* Make SetProxy behave like curl's CURLOPT_PROXY (Daniel Schauenberg)
* change prefixed address for proxy to HTTP (Daniel Schauenberg)
* add unit test for http2 connection (Daniel Schauenberg)
* move docker setup into Makefile (Daniel Schauenberg)
* update CONTRIBUTING docs with proxy server details (Daniel Schauenberg)
* add proxy server as docker service (Daniel Schauenberg)
* default to 10s timeout in unit tests (Daniel Schauenberg)
* update docs for HTTPS user certificate feature (Daniel Schauenberg)
* add .github folder with issue/PR templates (Daniel Schauenberg)
* move CI to run on travis trusty beta (Daniel Schauenberg)
* Small optimizations (Martin Gerhardy)
* Fix -Wpedantic warning (Daniel Perez)
* fix README examples for conn->post, conn->put (Daniel Schauenberg)
* add OSX install instructions to README (Daniel Schauenberg)

## v0.4.4 (13th May 2016)
- bump version because v0.4.3 was mistagged

## v0.4.3 (13th May 2016)
- only release packages from gcc builds (Daniel Schauenberg)

## v0.4.2 (13th May 2016)
- add an ldconfig script to package builds (Daniel Schauenberg)
- Add option to set the Certificate Authority (CA) Info option. (tgoetze)
- add clang to the travis build matrix (Daniel Schauenberg)
- README: Update Advanced Example (Marc Zych)
- Connection: Initialize timeout and followRedirects (Marc Zych)

## v0.4.1 (2nd February 2016)
- fix package build to include new header files from refactoring

## v0.4.0 (2nd February 2016)

### Features
**This is a major backwards-incompatible API refactor please read carefully**

- split out into a simple and an advanced API
  - simple API via static methods
  - advanced usage via a connection object
- support for custom user agent strings
- support for connection pooling and keep-alive
- support for following redirects
- support for getting request metrics from the Connection object

### Improvements and fixes
- increased test coverage
- addition of valgrind and lint runs on CI
- fixed some memory leaks
- improved documentation coverage


## v0.3.0 (10th January 2016)

### Features
- added support for connection timeouts (Islam Yasar)
- Allow for setting additional HTTP headers (Karol Samborski)

### Improvements and Bug fixes
- improve general documentation and add docs for auth (Daniel Schauenberg)
- remove calls to curl_global_cleanup (Daniel Schauenberg)
- add unit test for basic auth (Daniel Schauenberg)
- switch unit tests to use [http://httpbin.org]() (Daniel Schauenberg)
- move two include instructions (Jos)
- added reporting for test coverage (Daniel Schauenberg)
- cleanup version gen to be driven by git tags (Daniel Schauenberg)
- clean up build and support building on FreeBSD (Daniel Schauenberg)
- automate package deployment to packagecloud (Daniel Schauenberg)
- add automatic doc build & deploy (Daniel Schauenberg)
- Ensure the built shared library does not have dynamic references to libgtest (Thomas Goetze)

## v0.2.0 (19th April 2015)
- support basic auth
- Add header's to response object
- fix memory leaks related to curl cleanup
- add autotools build
- add CONTRIBUTING.md
- add vendorized gtest

## v0.1.2 (22nd March 2011)
- return -1 for failed query instead of exit(1)
- add unit tests for failed query returncode == -1
- fix typos in README and add more sections
- include cstring for memcpy
- add cstdlib as include for proper dependencies

## v0.1.1 (27th October 2010)
- bump version to v0.1.1
- add target for static library
- add target for dynamic library
- ignore lib folder in gitignore
- include meta.h from local include folder
- update README

## v0.1.0 (15th October 2010)
- initial release
- support basic HTTP verbs (GET, POST, PUT, DELETE)
