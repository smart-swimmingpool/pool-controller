#pragma once

#include <string>
#include <map>
#include <vector>
#include <functional>
#include "Arduino.h"
#include "Update.h"

#define HTTP_GET 0
#define HTTP_POST 1
#define HTTP_PUT 2
#define HTTP_DELETE 3
#define HTTP_ANY 4

// Content types
const char APPLICATION_JSON[] = "application/json";
const char TEXT_HTML[] = "text/html";
const char TEXT_PLAIN[] = "text/plain";
const char APPLICATION_OCTET_STREAM[] = "application/octet-stream";

// Forward declare so we can friend test helpers
class WebServer;

// Test capture globals — tests can inspect what was sent
struct WebServerCapture {
  int lastStatusCode = 0;
  std::string lastContentType;
  std::string lastBody;
  std::string lastUri;
  std::string lastMethod;
  
  // HTTP headers received
  std::map<std::string, std::string> headers;
  
  // POST body arguments
  std::map<std::string, std::string> args;
  
  void clear() {
    lastStatusCode = 0;
    lastContentType.clear();
    lastBody.clear();
    lastUri.clear();
    lastMethod.clear();
    headers.clear();
    args.clear();
  }
};

extern WebServerCapture wsCapture;

class WebServerRoute {
public:
  std::string uri;
  int method;
  std::function<void()> handler;
  
  bool matches(const std::string &requestUri, int requestMethod) const {
    // Simple URI matching (no wildcards for now)
    return method == requestMethod && uri == requestUri;
  }
};

class WebServer {
public:
  WebServer(int port) : port_(port) {}
  
  void on(const char *uri, std::function<void()> handler) {
    routes_.push_back({uri, HTTP_ANY, handler});
  }
  
   void on(const char *uri, int method, std::function<void()> handler) {
    routes_.push_back({uri, method, handler});
  }
  
   void on(const char *uri, int method, std::function<void()> handler, std::function<void()> uploadHandler) {
    routes_.push_back({uri, method, handler});
  }
  
  void begin() {}
  void handleClient() {}
  
  void sendHeader(const char *name, const char *value, bool first = false) {
    wsCapture.headers[name] = value;
  }
  
  void sendHeader(const char *name, const String &value, bool first = false) {
    wsCapture.headers[name] = value.c_str();
  }
  
  void onNotFound(std::function<void()> handler) {
    notFoundHandler_ = handler;
  }
  
  void collectHeaders(const char **headers, int count) {}
  
  // Upload access (returns a single static instance to avoid null ref)
  HTTPUpload &upload() { static HTTPUpload up; return up; }
  
  // streamFile stub
  File streamFile(File &f, const String &contentType) { return f; }
  
  // Send response
  void send(int code, const char *contentType, const String &content) {
    wsCapture.lastStatusCode = code;
    wsCapture.lastContentType = contentType;
    wsCapture.lastBody = content.c_str();
  }
  
  void send(int code, const char *contentType, const char *content) {
    wsCapture.lastStatusCode = code;
    wsCapture.lastContentType = contentType;
    wsCapture.lastBody = content;
  }
  
  // Request header access
  bool hasHeader(const char *name) const {
    return wsCapture.headers.find(name) != wsCapture.headers.end();
  }
  
  String header(const char *name) const {
    auto it = wsCapture.headers.find(name);
    if (it != wsCapture.headers.end()) return String(it->second.c_str());
    return String();
  }
  
  // POST body arg access
  bool hasArg(const char *name) const {
    return wsCapture.args.find(name) != wsCapture.args.end();
  }
  
  String arg(const char *name) const {
    auto it = wsCapture.args.find(name);
    if (it != wsCapture.args.end()) return String(it->second.c_str());
    return String();
  }
  
  String uri() const { return String(wsCapture.lastUri.c_str()); }
  int method() const { return 0; }
  
  // Test helpers
  const WebServerRoute *findRoute(const char *uri, int method = HTTP_ANY) const {
    for (const auto &r : routes_) {
      if (r.matches(uri, method)) return &r;
    }
    return nullptr;
  }
  
  bool invokeRoute(const char *uri, int method = HTTP_GET) {
    for (const auto &r : routes_) {
      if (r.matches(uri, method)) {
        wsCapture.lastUri = uri;
        wsCapture.lastMethod = (method == HTTP_GET) ? "GET" : "POST";
        r.handler();
        return true;
      }
    }
    return false;
  }

private:
  int port_;
  std::vector<WebServerRoute> routes_;
  std::function<void()> notFoundHandler_;
};

// Capture instance
extern WebServerCapture wsCapture;
