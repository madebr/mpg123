#include "config.h"
#include "net123.h"
#include "compat.h"
#include <ws2tcpip.h>
#include <winhttp.h>

// The network implementation defines the struct for private use.
// The purpose is just to keep enough context to be able to
// call net123_read() and net123_close() afterwards.
#define URL_COMPONENTS_LENGTH 255
struct net123_handle_struct {
  HINTERNET session;
  HINTERNET connect;
  HINTERNET request;
  URL_COMPONENTS comps;
  wchar_t lpszHostName[URL_COMPONENTS_LENGTH];
  wchar_t lpszUserName[URL_COMPONENTS_LENGTH];
  wchar_t lpszPassword[URL_COMPONENTS_LENGTH];
  wchar_t lpszUrlPath[URL_COMPONENTS_LENGTH];
  wchar_t lpszExtraInfo[URL_COMPONENTS_LENGTH];
  DWORD supportedAuth, firstAuth, authTarget, authTried;
  char *headers;
  size_t headers_pos, headers_len;
};

#define MPG123CONCAT_(x,y) x ## y
#define MPG123CONCAT(x,y) MPG123CONCAT_(x,y)
#define MPG123STRINGIFY_(x) #x
#define MPG123STRINGIFY(x) MPG123STRINGIFY_(x)
#define MPG123WSTR(x) MPG123CONCAT(L,MPG123STRINGIFY(x))

static DWORD wrap_auth(net123_handle *nh){
  DWORD mode;
  DWORD ret;

  if(nh->comps.dwUserNameLength) {
    if(!nh->authTried) {
      ret = WinHttpQueryAuthSchemes(nh->request, &nh->supportedAuth, &nh->firstAuth, &nh->authTarget);
      if(!ret) return GetLastError();
      nh->authTried = 1;
    }

    mode = nh->supportedAuth & WINHTTP_AUTH_SCHEME_DIGEST ? WINHTTP_AUTH_SCHEME_DIGEST :
           nh->supportedAuth & WINHTTP_AUTH_SCHEME_BASIC ? WINHTTP_AUTH_SCHEME_BASIC : 0;

    /* no supported mode? */
    if(!mode)
      return ERROR_WINHTTP_INTERNAL_ERROR;

    ret = WinHttpSetCredentials(nh->request, WINHTTP_AUTH_TARGET_SERVER, mode, nh->comps.lpszUserName, nh->comps.lpszPassword, NULL);
    return GetLastError();
  }
}

// Open stream from URL, preparing output such that net123_read()
// later on gets the response header lines followed by one empty line
// and then the raw data.
// client_head contains header lines to send with the request, without
// line ending

net123_handle *net123_open(const char *url, const char * const *client_head){
  LPWSTR urlW = NULL, headers = NULL;
  URL_COMPONENTS comps;
  size_t ii;
  WINBOOL res;
  DWORD headerlen;
  const LPCWSTR useragent = MPG123WSTR(PACKAGE_NAME) L"/" MPG123WSTR(PACKAGE_VERSION);

  if(!WinHttpCheckPlatform())
    return NULL;

  win32_utf8_wide(url, &urlW, NULL);
  if(urlW == NULL) goto cleanup;

  net123_handle *ret = calloc(1, sizeof(net123_handle));
  if (!ret) return ret;

  ret->comps.dwStructSize = sizeof(comps);
  ret->comps.dwSchemeLength    = 0;
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

  if(WinHttpCrackUrl(urlW, 0, 0, &comps)) goto cleanup;

  ret->session = WinHttpOpen(useragent, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
  free(urlW);
  urlW = NULL;
  if(!ret->session) goto cleanup;

  ret->connect = WinHttpConnect(ret->session, ret->comps.lpszHostName, comps.nPort, 0);
  if(!ret->connect) goto cleanup;

  ret->request = WinHttpOpenRequest(ret->connect, L"GET", ret->comps.lpszUrlPath, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, ret->comps.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);
  if(!ret->request) goto cleanup;

  wrap_auth(ret);

  for(ii = 0; client_head[ii]; ii++){
    win32_utf8_wide(client_head[ii], &headers, NULL);
    if(!headers)
      goto cleanup;
    res = WinHttpAddRequestHeaders(ret->request, headers, (DWORD) -1, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
    free(headers);
    headers = NULL;
  }

  res = WinHttpSendRequest(ret->request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
  res = WinHttpReceiveResponse(ret->request, NULL);
  res = WinHttpQueryHeaders(ret->request, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, WINHTTP_NO_OUTPUT_BUFFER, &headerlen, WINHTTP_NO_HEADER_INDEX);
  if(GetLastError() == ERROR_INSUFFICIENT_BUFFER && headerlen > 0) {
    headers = calloc(1, headerlen);
    if (!headers) goto cleanup;
    WinHttpQueryHeaders(ret->request, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, headers, &headerlen, WINHTTP_NO_HEADER_INDEX);
    win32_wide_utf7(headers, &ret->headers, &ret->headers_len);
    /* bytes written, skip the terminating null, we want to stop at the \r\n\r\n */
    ret->headers_len --;
    free(headers);
    headers = NULL;
  } else goto cleanup;

  return ret;
cleanup:
  if (urlW) free(urlW);
  net123_close(ret);
  ret = NULL;
  return ret;
}

// Read data into buffer, return bytes read.
// This handles interrupts (EAGAIN, EINTR, ..) internally and only returns
// a short byte count on EOF or error. End of file or error is not distinguished:
// For the user, it only matters if there will be more bytes or not.
// Feel free to communicate errors via error() / merror() functions inside.
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
  if(!WinHttpReadData(nh->request, buf, to_copy, &bytesread)){
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
    WinHttpCloseHandle(nh->request);
    nh->request = NULL;
  }
  if(nh->connect) {
    WinHttpCloseHandle(nh->connect);
    nh->connect = NULL;
  }
  if(nh->session) {
    WinHttpCloseHandle(nh->session);
    nh->session = NULL;
  }
  free(nh);
}
