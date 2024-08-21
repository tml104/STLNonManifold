#pragma once

#include <iostream>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <cstdarg>

#include <string>
#include <vector>
#include <map>

#ifdef _WIN32
#	include <windows.h>
#	ifndef _CRT_SECURE_NO_WARNINGS
#		define _CRT_SECURE_NO_WARNINGS
#	endif
#else
#	include <unistd.h>
#	include <pthread.h>
#endif

// ref: https://stackoverflow.com/questions/2915672/snprintf-and-visual-studio-2010
#if defined(_MSC_VER) && _MSC_VER <= 1916

#define snprintf c99_snprintf
#define vsnprintf c99_vsnprintf

__inline int c99_vsnprintf(char* outBuf, size_t size, const char* format, va_list ap)
{
	int count = -1;

	if (size != 0)
		count = _vsnprintf_s(outBuf, size, _TRUNCATE, format, ap);
	if (count == -1)
		count = _vscprintf(format, ap);

	return count;
}

__inline int c99_snprintf(char* outBuf, size_t size, const char* format, ...)
{
	int count;
	va_list ap;

	va_start(ap, format);
	count = c99_vsnprintf(outBuf, size, format, ap);
	va_end(ap);

	return count;
}

#endif

namespace Log44 {


	enum LogLevel {
		LOG_LEVEL_NULL,
		LOG_LEVEL_INFO,
		LOG_LEVEL_DEBUG,
		LOG_LEVEL_WARN,
		LOG_LEVEL_ERROR
	};

	namespace Utils {
		std::string getDateTimeString();
		std::string getLogLevelString(enum Log44::LogLevel);

		unsigned long getPID();
		unsigned long getTID();

	}

	class ILogFormatter {
	public:
		virtual ~ILogFormatter() = 0;
		virtual std::string getFormattedLog(enum Log44::LogLevel log_level, std::string&& file_name, std::string&& func_name, int line_no, std::string&& parsed_user_content) = 0;
	};

	class LogFormatter : public ILogFormatter {
	public:
		virtual std::string getFormattedLog(enum Log44::LogLevel log_level, std::string&& file_name, std::string&& func_name, int line_no, std::string&& parsed_user_content) override;
	};

	class ILogOutputTarget {
	public:
		virtual ~ILogOutputTarget() = 0;
		virtual void writeLog(std::string& final_log_content) = 0;
	};

	class LogOutputToConsole : public ILogOutputTarget {
	public:
		virtual ~LogOutputToConsole();
		virtual void writeLog(std::string& final_log_content) override;
	};

	class LogOutputToFile : public ILogOutputTarget {
	public:

		LogOutputToFile(std::string file_path);
		virtual ~LogOutputToFile();
		virtual void writeLog(std::string& final_log_content) override;

	private:
		FILE* fp;
	};

	class Logger {
	public:

		void init();
		static Logger& getInstance();
		void setLogLevel(enum Log44::LogLevel log_level, bool val);

		void log(enum Log44::LogLevel log_level, std::string&& file_name, std::string&& func_name, int line_no, const char* template_string, ...);

	private:
		Logger();
		~Logger();
		Logger(const Logger&);
		Logger& operator=(const Logger&); // this will not set default move constructor

		std::map<enum Log44::LogLevel, bool> logLevelControlMap;
		ILogFormatter* logFormatter;
		std::vector<ILogOutputTarget*> targetVec;
	};

#ifdef _MSC_VER

#ifndef LOG_INFO
#	define LOG_INFO(fmt, ...) Log44::Logger::getInstance().log(Log44::LogLevel::LOG_LEVEL_INFO, std::string(__FILE__), std::string(__FUNCTION__), __LINE__, fmt, __VA_ARGS__)
#endif

#ifndef LOG_DEBUG
#	define LOG_DEBUG(fmt, ...) Log44::Logger::getInstance().log(Log44::LogLevel::LOG_LEVEL_DEBUG, std::string(__FILE__), std::string(__FUNCTION__), __LINE__, fmt, __VA_ARGS__)
#endif

#ifndef LOG_WARN
#	define LOG_WARN(fmt, ...) Log44::Logger::getInstance().log(Log44::LogLevel::LOG_LEVEL_WARN, std::string(__FILE__), std::string(__FUNCTION__), __LINE__, fmt, __VA_ARGS__)
#endif

#ifndef LOG_ERROR
#	define LOG_ERROR(fmt, ...) Log44::Logger::getInstance().log(Log44::LogLevel::LOG_LEVEL_ERROR, std::string(__FILE__), std::string(__FUNCTION__), __LINE__, fmt, __VA_ARGS__)
#endif

#else

#ifndef LOG_INFO
#	define LOG_INFO(fmt, ...) Log44::Logger::getInstance().log(Log44::LogLevel::LOG_LEVEL_INFO, std::string(__FILE__), std::string(__FUNCTION__), __LINE__, fmt, ##__VA_ARGS__)
#endif

#ifndef LOG_DEBUG
#	define LOG_DEBUG(fmt, ...) Log44::Logger::getInstance().log(Log44::LogLevel::LOG_LEVEL_DEBUG, std::string(__FILE__), std::string(__FUNCTION__), __LINE__, fmt, ##__VA_ARGS__)
#endif

#ifndef LOG_WARN
#	define LOG_WARN(fmt, ...) Log44::Logger::getInstance().log(Log44::LogLevel::LOG_LEVEL_WARN, std::string(__FILE__), std::string(__FUNCTION__), __LINE__, fmt, ##__VA_ARGS__)
#endif

#ifndef LOG_ERROR
#	define LOG_ERROR(fmt, ...) Log44::Logger::getInstance().log(Log44::LogLevel::LOG_LEVEL_ERROR, std::string(__FILE__), std::string(__FUNCTION__), __LINE__, fmt, ##__VA_ARGS__)
#endif

#endif // _MSC_VER

} // namespace Log44