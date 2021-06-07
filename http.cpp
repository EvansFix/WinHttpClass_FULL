#include "http.h"

CWinHttp::CWinHttp()
{

}

CWinHttp::~CWinHttp()
{
	if (m_hRequest) WinHttpCloseHandle(m_hRequest);
	if (m_hConnect) WinHttpCloseHandle(m_hConnect);
	if (m_hSession) WinHttpCloseHandle(m_hSession);
}

BOOL CWinHttp::OpenSession(const std::string &userAgent, int dwAccessType, const std::string &proxyName, const std::string &proxyBypass, int dwFlags)
{
	LPWSTR pwszUserAgent = NULL;
	std::wstring wstrUserAgent;
	if (!userAgent.empty()) {
		wstrUserAgent = A2W_(userAgent);
		pwszUserAgent = const_cast<LPWSTR>(wstrUserAgent.c_str());
	}

	LPWSTR pwszProxyName = NULL;
	std::wstring wstrProxyName;
	if (!proxyName.empty()) {
		wstrProxyName = A2W_(proxyName);
		pwszProxyName = const_cast<LPWSTR>(wstrProxyName.c_str());
	}

	LPWSTR pwszProxyBypass = NULL;
	std::wstring wstrProxyBypass;
	if (!proxyBypass.empty()) {
		wstrProxyBypass = A2W_(proxyBypass);//U2W_
		pwszProxyBypass = const_cast<LPWSTR>(wstrProxyBypass.c_str());
	}

	if (dwAccessType == WINHTTP_ACCESS_TYPE_NO_PROXY)
		m_hSession = WinHttpOpen(pwszUserAgent, dwAccessType, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, dwFlags); //不使用代理IP
	else
		m_hSession = WinHttpOpen(pwszUserAgent, dwAccessType, pwszProxyName, pwszProxyBypass, dwFlags); //使用代理IP
	return m_hSession != NULL;
}

BOOL CWinHttp::OpenConnect(const std::string &url)
{
	if (m_hSession)
	{
		if (WinHttpCheckPlatform())
		{
			URL_COMPONENTS		urlComp;
			ZeroMemory(&urlComp, sizeof(urlComp));
			urlComp.dwStructSize = sizeof(urlComp);

			WCHAR wszScheme[64] = { 0 };
			urlComp.lpszScheme = wszScheme;
			urlComp.dwSchemeLength = ARRAYSIZE(wszScheme);
			WCHAR wszHostName[1024] = { 0 };
			urlComp.lpszHostName = wszHostName;
			urlComp.dwHostNameLength = ARRAYSIZE(wszHostName);
			WCHAR wszUrlPath[1024] = { 0 };
			urlComp.lpszUrlPath = wszUrlPath;
			urlComp.dwUrlPathLength = ARRAYSIZE(wszUrlPath);
			WCHAR wszExtraInfo[1024] = { 0 };
			urlComp.lpszExtraInfo = wszExtraInfo;
			urlComp.dwExtraInfoLength = ARRAYSIZE(wszExtraInfo);

			std::wstring wstrUrl = A2W_(url.c_str());
			if (WinHttpCrackUrl(wstrUrl.c_str(), wstrUrl.length(), NULL, &urlComp))
			{
				m_strHost = urlComp.lpszHostName;
				m_strPath = urlComp.lpszUrlPath;
				m_strExt = urlComp.lpszExtraInfo;
				m_strPath = (m_strPath + m_strExt); // - Fix No Path.
				m_nPort = urlComp.nPort;
				m_nScheme = urlComp.nScheme;
				m_hConnect = WinHttpConnect(m_hSession, m_strHost.c_str(), m_nPort, 0);
				return m_hConnect != NULL;
			}
		}
	}
	return FALSE;
}

BOOL CWinHttp::OpenRequest(bool bPost, bool bInitContentType)
{
	if (m_hConnect)
	{
		const wchar_t* pwszVerb = bPost ? L"POST" : L"GET";
		DWORD dwFlags = m_nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0;
		m_hRequest = WinHttpOpenRequest(m_hConnect, pwszVerb, m_strPath.c_str(), NULL,
			WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, dwFlags);
		if (bPost && bInitContentType)
			SetRequestHeader("Content-Type", "application/x-www-form-urlencoded");
		return m_hRequest != NULL;
	}
	return FALSE;
}

BOOL CWinHttp::Send(LPVOID lpbuffer, DWORD dwsize)
{
	BOOL  bResults = FALSE;
	if (m_hRequest) {
		if (lpbuffer != NULL && dwsize > 0)
			bResults = WinHttpSendRequest(m_hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, lpbuffer, dwsize, dwsize, 0);
		else
			bResults = WinHttpSendRequest(m_hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
	}
	return bResults;
}

BOOL CWinHttp::Write(LPCVOID lpbuffer, DWORD dwsize)
{
	if (m_hRequest)
	{
		return WinHttpWriteData(m_hRequest, lpbuffer, dwsize, NULL);
	}
	return FALSE;
}

std::vector<BYTE> CWinHttp::GetResponseBody()
{
	DWORD dwSize = 0;
	DWORD dwDownloaded = 0;

	std::vector<BYTE> list;

	if (WinHttpReceiveResponse(m_hRequest, NULL))
	{
		do
		{
			// Check for available data.
			dwSize = 0;
			if (!WinHttpQueryDataAvailable(m_hRequest, &dwSize))
			{
				printf("Error %u in WinHttpQueryDataAvailable.\n", GetLastError());
				break;
			}

			// No more available data.
			if (!dwSize)
				break;

			// Allocate space for the buffer.
			//pszOutBuffer = new char[dwSize + 1];
			BYTE* lpReceivedData = new BYTE[dwSize];
			if (!lpReceivedData)
			{
				printf("Out of memory\n");
				break;
			}
			else
			{
				// Read the Data.
				ZeroMemory(lpReceivedData, dwSize);

				if (!WinHttpReadData(m_hRequest, lpReceivedData, dwSize, &dwDownloaded)) {
					printf("Error %u in WinHttpReadData.\n", GetLastError());
				}
				else {
					for (size_t i = 0; i < dwSize; i++)
					{
						list.push_back(lpReceivedData[i]);
					}
				}

				// Free the memory allocated to the buffer.
				delete[] lpReceivedData;

				// This condition should never be reached since WinHttpQueryDataAvailable
				// reported that there are bits to read.
				if (!dwDownloaded)
					break;
			}
		} while (dwSize > 0);
	}
	return list;
}

std::string CWinHttp::GetResponseHeaderValue(int dwInfoLevel, DWORD dwIndex)
{
	DWORD dwSize = 0;
	LPVOID lpOutBuffer = NULL;
	BOOL  bResults = FALSE;
	bResults = WinHttpQueryHeaders(m_hRequest, dwInfoLevel, WINHTTP_HEADER_NAME_BY_INDEX, NULL, &dwSize, &dwIndex);
	if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		lpOutBuffer = new WCHAR[dwSize / sizeof(WCHAR)];
		bResults = WinHttpQueryHeaders(m_hRequest, dwInfoLevel, WINHTTP_HEADER_NAME_BY_INDEX, lpOutBuffer, &dwSize, &dwIndex);
		return W2A_((wchar_t*)lpOutBuffer);
	}
	return "";
}

std::string CWinHttp::GetResponseHeaderValue(const std::string &name)
{
	DWORD dwSize = 0;
	DWORD dwIndex = 0;
	LPVOID lpOutBuffer = NULL;

	BOOL  bResults = FALSE;

	std::wstring strHeaderName;
	strHeaderName = A2W_(name);

	bResults = WinHttpQueryHeaders(m_hRequest, WINHTTP_QUERY_CUSTOM, strHeaderName.c_str(), NULL, &dwSize, &dwIndex);
	if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		lpOutBuffer = new WCHAR[dwSize / sizeof(WCHAR)];
		bResults = WinHttpQueryHeaders(m_hRequest, WINHTTP_QUERY_CUSTOM, strHeaderName.c_str(), lpOutBuffer, &dwSize, &dwIndex);		
		return W2A_((wchar_t*)lpOutBuffer);
	}
	return "";
}

std::string CWinHttp::GetResponseHeaders()
{
	return GetResponseHeaderValue(WINHTTP_QUERY_RAW_HEADERS_CRLF);
}

BOOL CWinHttp::SetRequestHeader(const std::string &name, const std::string &value)
{
	if (name.empty() || value.empty())
		return FALSE;
	BOOL  bResults = FALSE;
	if (m_hRequest)
	{
		std::string strHeader(name + ": " + value);
		LPWSTR pwszHeaders = NULL;
		std::wstring wstrHeaders;
		if (!strHeader.empty()) {
			wstrHeaders = A2W_(strHeader);
			pwszHeaders = const_cast<LPWSTR>(wstrHeaders.c_str());
			bResults = WinHttpAddRequestHeaders(m_hRequest, pwszHeaders, (ULONG)-1L, WINHTTP_ADDREQ_FLAG_ADD_IF_NEW | WINHTTP_ADDREQ_FLAG_REPLACE);
		}
	}
	return bResults;
}

BOOL CWinHttp::SetReferer(const std::string &referer)
{
	return(SetRequestHeader("Referer", referer));
}

BOOL CWinHttp::SetTimeout(int nResolveTimeout, int nConnectTimeout, int nSendTimeout, int nReceiveTimeout)
{
	if (m_hSession)
		return WinHttpSetTimeouts(m_hSession, nResolveTimeout, nConnectTimeout, nSendTimeout, nReceiveTimeout);
	return FALSE;
}

BOOL CWinHttp::SetOption(int Option, int value)
{
	if (m_hRequest)
		return WinHttpSetOption(m_hRequest, Option, (LPVOID)&value, 4);
	return FALSE;
}

BOOL CWinHttp::SetLocal(bool Is)
{
	return SetOption(WINHTTP_OPTION_REDIRECT_POLICY, Is ? WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS : WINHTTP_OPTION_REDIRECT_POLICY_NEVER);
}

std::string CWinHttp::GetLocal()
{
	return GetResponseHeaderValue(WINHTTP_QUERY_LOCATION);
}

BOOL CWinHttp::SetCookie(const std::string &cookies)
{
	return(SetRequestHeader("cookie", cookies));
}

std::string CWinHttp::GetCookie()
{
	std::map<std::string, std::string> map_cookies;
	std::map<std::string, std::string>::iterator iter;

	std::string strHeaders , fuck_a,fuckb,fuckc, fuckd,fucke;
	strHeaders = GetResponseHeaders();

	fuck_a = "Set-Cookie: ";
	fuckb = "Set-Cookie:";
	fuckc = "\r\n";
	fuckd = ";";
	fucke = "=";
	strHeaders = str_replace(strHeaders, fuck_a, fuckb);

	std::vector<std::string> list = str_between_array(strHeaders, fuckb, fuckc, true);

	std::vector<std::string> array_list;

	for (size_t i = 0; i < list.size(); ++i) {
		std::vector<std::string> temp_list = str_split(list[i], fuckd, true);
		array_list.insert(array_list.end(), temp_list.begin(), temp_list.end());
	}

	for (size_t i = 0; i < array_list.size(); ++i) {
		std::string name = str_left(array_list[i], fucke);
		std::string value = str_right(array_list[i], fucke);
		name = str_trim(name);
		//value = str_trim(value);
		if (!name.empty() && !value.empty())
		{
			iter = map_cookies.find(name);
			if (iter != map_cookies.end())
				iter->second = value;
			else
				map_cookies.insert(make_pair(name, value));
		}
	}

	std::string cookies;

	for (iter = map_cookies.begin(); iter != map_cookies.end(); ++iter)
	{
		if (iter->second == "-" || iter->second == "''")
			continue;

		if (cookies.empty())
			cookies.append(iter->first).append("=").append(iter->second);
		else
			cookies.append("; ").append(iter->first).append("=").append(iter->second);
	}
	return cookies;

	//DWORD dwSize = 0;
	//DWORD dwIndex = 0;
	//LPVOID lpOutBuffer = NULL;

	//std::wstring result;

	//do
	//{
	//	dwSize = 0;
	//	WinHttpQueryHeaders(m_hRequest, WINHTTP_QUERY_SET_COOKIE, WINHTTP_HEADER_NAME_BY_INDEX, NULL, &dwSize, &dwIndex);

	//	lpOutBuffer = new WCHAR[dwSize / sizeof(WCHAR)];

	//	WinHttpQueryHeaders(m_hRequest, WINHTTP_QUERY_SET_COOKIE, WINHTTP_HEADER_NAME_BY_INDEX, lpOutBuffer, &dwSize, &dwIndex);

	//	if (dwSize > 0) 
	//		result.append((wchar_t *)lpOutBuffer).append(L"\n");

	//	delete lpOutBuffer;

	//} while (dwSize > 0);
	//return CEncoder::W2A_(result);
}

std::string CWinHttp::MergeCookie(const std::string &oldCookies, const std::string &nowCookies)
{
	std::map<std::string, std::string> map_cookies;
	std::map<std::string, std::string>::iterator iter;
	std::string o_fuck,s_fuck;

	o_fuck = ";";
	s_fuck = "=";
	std::vector<std::string> array_list = str_split(oldCookies, o_fuck, true);
	std::vector<std::string> now_array_list = str_split(nowCookies, o_fuck, true);

	array_list.insert(array_list.end(), now_array_list.begin(), now_array_list.end());

	for (size_t i = 0; i < array_list.size(); ++i) {
		std::string name = str_left(array_list[i], s_fuck);
		std::string value = str_right(array_list[i], s_fuck);
		name = str_trim(name);
		//value = str_trim(value);
		if (!name.empty() && !value.empty())
		{
			iter = map_cookies.find(name);
			if (iter != map_cookies.end())
				iter->second = value;
			else
				map_cookies.insert(make_pair(name, value));
		}
	}

	std::string cookies;

	for (iter = map_cookies.begin(); iter != map_cookies.end(); ++iter)
	{
		if (iter->second == "-" || iter->second == "''")
			continue;

		if (cookies.empty())
			cookies.append(iter->first).append("=").append(iter->second);
		else
			cookies.append("; ").append(iter->first).append("=").append(iter->second);
	}
	return cookies;
}



//SysMultiByteToWide 
std::wstring CWinHttp::A2W_(const std::string& mb , uint32_t code_page) {
  if (mb.empty())
    return std::wstring();

  int mb_length = static_cast<int>(mb.length());
  // Compute the length of the buffer.
  int charcount = MultiByteToWideChar(code_page, 0,
                                      mb.data(), mb_length, NULL, 0);
  if (charcount == 0)
    return std::wstring();

  std::wstring wide;
  wide.resize(charcount);
  MultiByteToWideChar(code_page, 0, mb.data(), mb_length, &wide[0], charcount);

  return wide;
}

//SysWideToMultiByte
std::string CWinHttp::W2A_(const std::wstring& wide , uint32_t code_page) {
  int wide_length = static_cast<int>(wide.length());
  if (wide_length == 0)
    return std::string();

  // Compute the length of the buffer we'll need.
  int charcount = WideCharToMultiByte(code_page, 0, wide.data(), wide_length,
                                      NULL, 0, NULL, NULL);
  if (charcount == 0)
    return std::string();

  std::string mb;
  mb.resize(charcount);
  WideCharToMultiByte(code_page, 0, wide.data(), wide_length,
                      &mb[0], charcount, NULL, NULL);

  return mb;
}


std::string CWinHttp::U2A_(const std::string& mb)
{
	return W2A_(A2W_(mb, CP_UTF8),936);
}






