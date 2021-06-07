/*
* Author: hpsocket (https://github.com/hpsocket)
* License: Code Project Open License
* Disclaimer: The software is provided "as-is". No claim of suitability, guarantee, or any warranty whatsoever is provided.
* Copyright (c) 2016-2017.
*/

#ifndef _STRING_HELPER_HPP_INCLUDED_
#define _STRING_HELPER_HPP_INCLUDED_

#include <stdarg.h>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <iomanip>

namespace string_helper
{
	inline std::string format(const char *fmt, ...)
	{
		std::string strResult = "";
		if (NULL != fmt)
		{
			va_list marker = NULL;
			va_start(marker, fmt);

			size_t nLength = _vscprintf(fmt, marker) + 1;
			std::vector<char> vBuffer(nLength, '\0');

			int nRet = _vsnprintf_s(&vBuffer[0], vBuffer.size(), nLength, fmt, marker);
			if (nRet > 0)
			{
				strResult = &vBuffer[0];

			}

			va_end(marker);
		}
		return strResult;
	}

	inline std::wstring format(const wchar_t *fmt, ...)
	{
		std::wstring strResult = L"";
		if (NULL != fmt)
		{
			va_list marker = NULL;
			va_start(marker, fmt);

			size_t nLength = _vscwprintf(fmt, marker) + 1;
			std::vector<wchar_t> vBuffer(nLength, L'\0');

			int nWritten = _vsnwprintf_s(&vBuffer[0], vBuffer.size(), nLength, fmt, marker);
			if (nWritten > 0)
			{
				strResult = &vBuffer[0];
			}

			va_end(marker);
		}
		return strResult;
	}

	template <class T>
	inline std::vector<std::basic_string<T>> str_compact(const std::vector<std::basic_string<T>> &tokens)
	{
		std::vector<std::basic_string<T>> compacted;
		for (size_t i = 0; i < tokens.size(); ++i) {
			if (!tokens[i].empty()) {
				compacted.push_back(tokens[i]);
			}
		}
		return compacted;
	}

	template <class T>
	inline std::vector<std::basic_string<T>> str_split(const std::basic_string<T>& str, const std::basic_string<T>& delim, const bool trim_empty = false)
	{
		size_t pos, last_pos = 0, len;
		std::vector<std::basic_string<T>> tokens;
		while (true) {
			pos = str.find(delim, last_pos);
			if (pos == std::wstring::npos) {
				pos = str.size();
			}
			len = pos - last_pos;
			if (!trim_empty || len != 0) {
				tokens.push_back(str.substr(last_pos, len));
			}
			if (pos == str.size()) {
				break;
			}
			else {
				last_pos = pos + delim.size();
			}
		}
		return tokens;
	}

	template <class T>
	inline std::basic_string<T> str_join(const std::vector<std::basic_string<T>> &tokens,const std::basic_string<T>& delim, const bool trim_empty = false)
	{
		if (trim_empty) {
			return join(str_compact(tokens), delim, false);
		}
		else {
			std::basic_stringstream<T> ss;
			for (size_t i = 0; i < tokens.size() - 1; ++i) {
				ss << tokens[i] << delim;
			}
			ss << tokens[tokens.size() - 1];
			return ss.str();
		}
	}

	template <class T>
	inline std::basic_string<T> str_trim(const std::basic_string<T>& str)
	{
		if (str.empty()) {
			return str;
		}
		std::basic_string<T> sstr(str);
		typename std::basic_string<T>::iterator c;
		// Erase whitespace before the string  
		for (c = sstr.begin(); c != sstr.end() && iswspace(*c++);); sstr.erase(sstr.begin(), --c);
		// Erase whitespace after the string  
		for (c = sstr.end(); c != sstr.begin() && iswspace(*--c);); sstr.erase(++c, sstr.end());
		return sstr;
	}

	template <class T>
	inline std::basic_string<T> str_repeat(const std::basic_string<T>& str, unsigned int times)
	{
		std::basic_stringstream<T> ss;
		for (unsigned int i = 0; i < times; ++i) {
			ss << str;
		}
		return ss.str();
	}

	template <class T>
	inline std::basic_string<T> str_Toupper(const std::basic_string<T>& str)
	{
		if (str.empty()) {
			return str;
		}
		std::basic_string<T> s(str);
		std::transform(s.begin(), s.end(), s.begin(), toupper);
		return s;
	}

	template <class T>
	inline std::basic_string<T> str_Tolower(const std::basic_string<T>& str)
	{
		if (str.empty()) {
			return str;
		}
		std::basic_string<T> s(str);
		std::transform(s.begin(), s.end(), s.begin(), tolower);
		return s;
	}

	template <class T>
	inline std::basic_string<T> str_replace(const std::basic_string<T>& source,const std::basic_string<T>& target,const std::basic_string<T>& replacement)
	{
		std::basic_string<T> s(source);
		typename std::basic_string<T>::size_type pos = 0;
		typename std::basic_string<T>::size_type srclen = target.size();
		typename std::basic_string<T>::size_type dstlen = replacement.size();
		while ((pos = s.find(target, pos)) != std::basic_string<T>::npos)
		{
			s.replace(pos, srclen, replacement);
			pos += dstlen;
		}
		return s;
	}

	template <class T>
	inline std::basic_string<T> str_between(const std::basic_string<T>& str, const std::basic_string<T>& left, const std::basic_string<T>& right) {
		size_t left_pos, right_pos, last_pos = 0;
		left_pos = str.find(left, last_pos);
		if (left_pos == std::basic_string<T>::npos)
			return std::basic_string<T>();
		last_pos = left_pos + left.size();
		right_pos = str.find(right, last_pos);
		if (right_pos == std::basic_string<T>::npos)
			return std::basic_string<T>();
		return str.substr(left_pos + left.size(), right_pos - left_pos - left.size());
	}

	template <class T>
	inline std::vector<std::basic_string<T>> str_between_array(const std::basic_string<T>& str, const std::basic_string<T>& left, const std::basic_string<T>&right, const bool trim_empty = false)
	{
		size_t left_pos, right_pos, last_pos = 0, len;
		std::vector<std::basic_string<T>> result;
		while (true) {
			left_pos = str.find(left, last_pos);
			if (left_pos == std::basic_string<T>::npos) {
				break;
			}
			else
			{
				last_pos = left_pos + left.size();
				right_pos = str.find(right, last_pos);
				if (right_pos == std::basic_string<T>::npos) {
					break;
				}
				len = right_pos - left_pos - left.size();
				if (len != 0 || !trim_empty)
				{
					result.push_back(str.substr(last_pos, len));
				}
			}
			last_pos = right_pos + right.size();
		}
		return result;
	}

	template <class T>
	inline std::basic_string<T> str_left(const std::basic_string<T>& str, const std::basic_string<T>& left)
	{
		std::basic_string<T> s(str);
		size_t pos, last_pos = 0;
		pos = s.find(left, last_pos);
		if (pos != std::basic_string<T>::npos)
			return s.substr(0, pos);
		else
			return std::basic_string<T>();
	}

	template <class T>
	inline std::basic_string<T> str_right(const std::basic_string<T>& str, const std::basic_string<T>& right)
	{
		std::basic_string<T> s(str);
		size_t pos, last_pos = 0;
		pos = s.find(right, last_pos);
		if (pos != std::basic_string<T>::npos)
		{
			pos += right.length();
			return s.substr(pos, str.length() - pos);
		}
		else
			return std::basic_string<T>();
	}

	template <class T>
	inline bool is_start_with(const std::basic_string<T>& str, const std::basic_string<T>& src)
	{
		return str.compare(0, src.length(), src) == 0;
	}

	template <class T>
	inline bool is_end_with(const std::basic_string<T>& str, const std::basic_string<T>& src)
	{
		return str.compare(str.length() - src.length(), src.length(), src) == 0;
	}
}

#endif // _STRING_HELPER_HPP_INCLUDED_