#include "config.h"
#include "net123.h"
#include "compat.h"
#include "debug.h"
#include <ws2tcpip.h>
#include <wininet.h>

const char *net123_backends[] = { "(always wininet)", NULL };

// The network implementation defines the struct for private use.
// The purpose is just to keep enough context to be able to
// call net123_read() and net123_close() afterwards.
#define URL_COMPONENTS_LENGTH 255
struct net123_handle_struct {
  HINTERNET session;
  HINTERNET connect;
  HINTERNET request;
  URL_COMPONENTSW comps;
  wchar_t lpszHostName[URL_COMPONENTS_LENGTH];
  wchar_t lpszUserName[URL_COMPONENTS_LENGTH];
  wchar_t lpszPassword[URL_COMPONENTS_LENGTH];
  wchar_t lpszUrlPath[URL_COMPONENTS_LENGTH];
  wchar_t lpszExtraInfo[URL_COMPONENTS_LENGTH];
  wchar_t lpszScheme[URL_COMPONENTS_LENGTH];
  DWORD supportedAuth, firstAuth, authTarget, authTried;
  char *headers;
  size_t headers_pos, headers_len;
  DWORD HttpQueryInfoIndex;
  DWORD internetStatus, internetStatusLength;
  LPVOID additionalInfo;
};

#define MPG123CONCAT_(x,y) x ## y
#define MPG123CONCAT(x,y) MPG123CONCAT_(x,y)
#define MPG123STRINGIFY_(x) #x
#define MPG123STRINGIFY(x) MPG123STRINGIFY_(x)
#define MPG123WSTR(x) MPG123CONCAT(L,MPG123STRINGIFY(x))

#if DEBUG
static void debug_crack(URL_COMPONENTSW *comps){
  wprintf(L"dwStructSize: %lu\n", comps->dwStructSize);
  wprintf(L"lpszScheme: %s\n", comps->lpszScheme ? comps->lpszScheme : L"null");
  wprintf(L"dwSchemeLength: %lu\n", comps->dwSchemeLength);
  wprintf(L"nScheme: %u %s\n", comps->nScheme, comps->nScheme == 1 ? L"INTERNET_SCHEME_HTTP": comps->nScheme == 2 ? L"INTERNET_SCHEME_HTTPS" : L"UNKNOWN");
  wprintf(L"lpszHostName: %s\n", comps->lpszHostName ? comps->lpszHostName : L"null");
  wprintf(L"dwHostNameLength: %u\n", comps->dwHostNameLength);
  wprintf(L"nPort: %u\n", comps->nPort);
  wprintf(L"lpszUserName: %s\n", comps->lpszUserName ? comps->lpszUserName : L"null");
  wprintf(L"dwUserNameLength: %lu\n", comps->dwUserNameLength);
  wprintf(L"lpszPassword: %s\n", comps->lpszPassword ? comps->lpszPassword : L"null");
  wprintf(L"dwPasswordLength: %lu\n", comps->dwPasswordLength);
  wprintf(L"lpszUrlPath: %s\n", comps->lpszUrlPath ? comps->lpszUrlPath : L"null");
  wprintf(L"dwUrlPathLength: %lu\n", comps->dwUrlPathLength);
  wprintf(L"lpszExtraInfo: %s\n", comps->lpszExtraInfo? comps->lpszExtraInfo : L"null");
  wprintf(L"dwExtraInfoLength: %lu\n", comps->dwExtraInfoLength);
}
#else
static void debug_crack(URL_COMPONENTSW *comps){}
#endif

static
void WINAPI net123_ssl_errors(HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus, LPVOID lpvStatusInformation, DWORD dwStatusInformationLength){
  net123_handle *nh = (net123_handle *)dwContext;
  nh->internetStatus = dwInternetStatus;
  nh->additionalInfo = lpvStatusInformation;
  nh->internetStatusLength = dwStatusInformationLength;
}

net123_handle *net123_open(const char *url, const char * const *client_head){
  LPWSTR urlW = NULL, headers = NULL;
  size_t ii;
  WINBOOL res;
  DWORD headerlen;
  const LPCWSTR useragent = MPG123WSTR(PACKAGE_NAME) L"/" MPG123WSTR(PACKAGE_VERSION);
  INTERNET_STATUS_CALLBACK cb;

  win32_utf8_wide(url, &urlW, NULL);
  if(urlW == NULL) goto cleanup;

  net123_handle *ret = calloc(1, sizeof(net123_handle));
  if (!ret) return ret;

  ret->comps.dwStructSize = sizeof(ret->comps);
  ret->comps.dwSchemeLength    = URL_COMPONENTS_LENGTH - 1;
  ret->comps.dwUserNameLength  = URL_COMPONENTS_LENGTH - 1;
  ret->comps.dwPasswordLength  = URL_COMPONENTS_LENGTH - 1;
  ret->comps.dwHostNameLength  = URL_COMPONENTS_LENGTH - 1;
  ret->comps.dwUrlPathLength   = URL_COMPONENTS_LENGTH - 1;
  ret->comps.dwExtraInfoLength = URL_COMPONENTS_LENGTH - 1;
  ret->comps.lpszHostName = ret->lpszHostName;
  ret->comps.lpszUserName = ret->lpszUserName;
  ret->comps.lpszPassword = ret->lpszPassword;
  ret->comps.lpszUrlPath = ret->lpszUrlPath;
  ret->comps.lpszExtraInfo = ret->lpszExtraInfo;
  ret->comps.lpszScheme = ret->lpszScheme;

  debug1("net123_open start crack %S", urlW);

  if(!(res = InternetCrackUrlW(urlW, 0, 0, &ret->comps))) {
    debug1("net123_open crack fail %lu", GetLastError());
    goto cleanup;
  }

  debug("net123_open crack OK");
  debug_crack(&ret->comps);

  ret->session = InternetOpenW(useragent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
  free(urlW);
  urlW = NULL;
  debug("net123_open InternetOpenW OK");
  if(!ret->session) goto cleanup;

  debug2("net123_open InternetConnectW %S %u", ret->comps.lpszHostName, ret->comps.nPort);
  ret->connect = InternetConnectW(ret->session, ret->comps.lpszHostName, ret->comps.nPort,
    ret->comps.dwUserNameLength ? ret->comps.lpszUserName : NULL, ret->comps.dwPasswordLength ? ret->comps.lpszPassword : NULL,
    INTERNET_SERVICE_HTTP, 0, 0);
  if(!ret->connect) goto cleanup;
  debug("net123_open InternetConnectW OK");

  debug1("HttpOpenRequestW GET %S", ret->comps.lpszUrlPath);
  ret->request = HttpOpenRequestW(ret->connect, L"GET", ret->comps.lpszUrlPath, NULL, NULL, NULL, ret->comps.nScheme == INTERNET_SCHEME_HTTPS ? INTERNET_FLAG_SECURE : 0, (DWORD_PTR)ret);
  if(!ret->request) goto cleanup;
  debug("HttpOpenRequestW GET OK");

  cb = InternetSetStatusCallback(ret->request, (INTERNET_STATUS_CALLBACK)net123_ssl_errors);
  if(cb != NULL){
    error1("InternetSetStatusCallback failed to install callback, errors might not be reported properly! (%lu)", GetLastError());
  }

  for(ii = 0; client_head[ii]; ii++){
    win32_utf8_wide(client_head[ii], &headers, NULL);
    if(!headers)
      goto cleanup;
    debug1("HttpAddRequestHeadersW add %S", headers);
    res = HttpAddRequestHeadersW(ret->request, headers, (DWORD) -1, HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE);
    debug2("HttpAddRequestHeadersW returns %u %lu", res, res ? 0 : GetLastError());
    free(headers);
    headers = NULL;
  }

  debug("net123_open ADD HEADERS OK");

  res = HttpSendRequestW(ret->request, NULL, 0, NULL, 0);

  if (!res) {
    res = GetLastError();
    error1("HttpSendRequestW failed with %lu", res);
    goto cleanup;
  }

  // dummy, cannot be null
  headers = calloc(1,1);
  headerlen = 1;
  if(headers == NULL) {
    error("Cannot allocate dummy buffer for HttpQueryInfoW");
    goto cleanup;
  }
  res = HttpQueryInfoW(ret->request, HTTP_QUERY_RAW_HEADERS_CRLF, headers, &headerlen, &ret->HttpQueryInfoIndex);
  free(headers);

  if(!res && GetLastError() == ERROR_INSUFFICIENT_BUFFER && headerlen > 0) {
    /* buffer size is in bytes, not including terminator */
    headers = calloc(1, headerlen + sizeof(*headers));
    if (!headers) goto cleanup;
    res = HttpQueryInfoW(ret->request, HTTP_QUERY_RAW_HEADERS_CRLF, headers, &headerlen, &ret->HttpQueryInfoIndex);
    debug3("HttpQueryInfoW returned %u, err %u : %S", res, GetLastError(), headers ? headers : L"null");
    win32_wide_utf7(headers, &ret->headers, &ret->headers_len);
    /* bytes written, skip the terminating null, we want to stop at the \r\n\r\n */
    ret->headers_len --;
    free(headers);
    headers = NULL;
  } else {
    error("HttpQueryInfoW did not execute as expected");
    goto cleanup;
  }
  debug("net123_open OK");

  return ret;
cleanup:
  debug("net123_open error");
  if (urlW) free(urlW);
  net123_close(ret);
  ret = NULL;
  return ret;
}

size_t net123_read(net123_handle *nh, void *buf, size_t bufsize){
  size_t ret;
  size_t to_copy = nh->headers_len - nh->headers_pos;
  DWORD bytesread = 0;

  if(to_copy){
     ret = to_copy <= bufsize ? to_copy : bufsize;
     memcpy(buf, nh->headers + nh->headers_pos, ret);
     nh->headers_pos += ret;
     return ret;
  }

  /* is this needed? */
  to_copy = bufsize > ULONG_MAX ? ULONG_MAX : bufsize;
  if(!InternetReadFile(nh->request, buf, to_copy, &bytesread)){
    error1("InternetReadFile exited with %d", GetLastError());
    return EOF;
  }
  return bytesread;
}

// Call that to free up resources, end processes.
void net123_close(net123_handle *nh){
  if(nh->headers) {
    free(nh->headers);
    nh->headers = NULL;
  }
  if(nh->request) {
    InternetCloseHandle(nh->request);
    nh->request = NULL;
  }
  if(nh->connect) {
    InternetCloseHandle(nh->connect);
    nh->connect = NULL;
  }
  if(nh->session) {
    InternetCloseHandle(nh->session);
    nh->session = NULL;
  }
  free(nh);
}
