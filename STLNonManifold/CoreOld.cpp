#include "CoreOld.h"

/*
	Add prefix (like "[Date][PID][TID][Loglevel][FILE][FUNC][LINE]")
*/
std::string Log44::LogFormatter::getFormattedLog(enum Log44::LogLevel log_level, std::string&& file_name, std::string&& func_name, int line_no, std::string&& parsed_user_content)
{
	static const unsigned int BUF_SIZE = 2000;
	static char buf[BUF_SIZE];
	auto prefix_template = "[%s][PID: %4ld][TID: %4ld][%-5s][FILE: %-s][%s: %4d] ";

	int sz = snprintf(buf, BUF_SIZE, prefix_template,
		Utils::getDateTimeString().c_str(),
		Utils::getPID(),
		Utils::getTID(),
		Utils::getLogLevelString(log_level).c_str(),
		file_name.c_str(),
		func_name.c_str(),
		line_no
	);

	if (sz < 0 || sz >= BUF_SIZE)
	{
		throw std::runtime_error("snprintf in getFormattedLog: Out of BUF_SIZE");
	}

	return std::string(buf) + parsed_user_content;
}

void Log44::Logger::init()
{
	for (auto e = Log44::LogLevel::LOG_LEVEL_INFO; e <= Log44::LogLevel::LOG_LEVEL_ERROR; e = static_cast<Log44::LogLevel>(e + 1))
	{
		logLevelControlMap[e] = true;
	}

	// new a LogFormatter
	this->logFormatter = new LogFormatter();

	// new a LogOutputToConsole & LogOutputToFile
	targetVec.emplace_back(new LogOutputToConsole());
	targetVec.emplace_back(new LogOutputToFile("./TestLogFile.txt"));
}

/*
	Meyers' Singleton
*/
Log44::Logger& Log44::Logger::getInstance()
{
	static Logger logger;
	return logger;
}

void Log44::Logger::setLogLevel(enum Log44::LogLevel log_level, bool val)
{
	logLevelControlMap[log_level] = val;
}

void Log44::Logger::log(Log44::LogLevel log_level, std::string&& file_name, std::string&& func_name, int line_no, const char* template_string, ...)
{
	// log_level Check
	if (logLevelControlMap[log_level] == false) {
		return;
	}

	va_list args;
	va_start(args, template_string);

	static const unsigned int BUF_SIZE = 5000;
	static char buf[BUF_SIZE];

	int sz = vsnprintf(buf, BUF_SIZE, template_string, args);
	if (sz < 0 || sz >= BUF_SIZE) {
		throw std::runtime_error("snprintf in log: Out of BUF_SIZE");
	}
	va_end(args);

	std::string final_log_content = this->logFormatter->getFormattedLog(
		log_level,
		std::forward<std::string>(file_name),
		std::forward<std::string>(func_name),
		line_no,
		std::string(buf)
	);

	// write log to target
	for (auto it = targetVec.begin(); it != targetVec.end(); it++)
	{
		auto target = *(it);
		target->writeLog(final_log_content);
	}
}

Log44::Logger::~Logger()
{

	if (this->logFormatter != nullptr)
	{
		delete this->logFormatter;
		this->logFormatter = nullptr;
	}

	for (auto it = targetVec.begin(); it != targetVec.end(); it++)
	{
		auto p = (*it);

		if (p != nullptr)
		{
			delete p;
		}
		p = nullptr;
	}
}

Log44::Logger::Logger() {
	init();
}

std::string Log44::Utils::getDateTimeString()
{
	std::string result;

	std::time_t now = time(0);
	//tm* ltm = localtime(&now);
	std::tm ltm;

	// ref: https://stackoverflow.com/questions/38034033/c-localtime-this-function-or-variable-may-be-unsafe
#if defined(__unix__)
	localtime_r(&now, &ltm);
#elif defined(_MSC_VER)
	localtime_s(&ltm, &now);
#else
	static std::mutex mtx;
	std::lock_guard<std::mutex> lock(mtx);
	ltm = *std::localtime(&timer);
#endif

	result += std::to_string(static_cast<long long>(1900 + ltm.tm_year)) + "-"
		+ std::to_string(static_cast<long long>(1 + ltm.tm_mon)) + "-"
		+ std::to_string(static_cast<long long>(ltm.tm_mday)) + " "
		+ std::to_string(static_cast<long long>(ltm.tm_hour)) + ":"
		+ std::to_string(static_cast<long long>(ltm.tm_min)) + ":"
		+ std::to_string(static_cast<long long>(ltm.tm_sec));
	return result;
}

std::string Log44::Utils::getLogLevelString(enum Log44::LogLevel log_level)
{
	switch (log_level) {
	case LOG_LEVEL_NULL:
		return "NULL";
		break;
	case LOG_LEVEL_INFO:
		return "INFO";
		break;
	case LOG_LEVEL_DEBUG:
		return "DEBUG";
		break;
	case LOG_LEVEL_WARN:
		return "WARN";
		break;
	case LOG_LEVEL_ERROR:
		return "ERROR";
		break;
	}

	return "UNKNOWN";
}

unsigned long Log44::Utils::getPID()
{
#ifdef _WIN32
	DWORD pid = GetCurrentProcessId();
#else
	pid_t pid = getpid();
#endif

	return pid;
}

unsigned long Log44::Utils::getTID()
{
#ifdef _WIN32
	DWORD tid = GetCurrentThreadId();
#else
	pthread_t tid = pthread_self();
#endif

	return tid;
}

Log44::LogOutputToConsole::~LogOutputToConsole()
{
	fflush(stdout);
}

/*
	write to stdout
*/
void Log44::LogOutputToConsole::writeLog(std::string& final_log_content)
{
	// Writes the C string pointed by str to the standard output (stdout) and appends a newline character ('\n').
	puts(final_log_content.c_str());

	//fflush(stdout);
}

Log44::LogOutputToFile::LogOutputToFile(std::string file_path)
{
	fp = fopen(file_path.c_str(), "w");
	if (fp == nullptr) {
		throw std::runtime_error("fopen in LogOutputToFile: Failed to open file.");
	}
}

Log44::LogOutputToFile::~LogOutputToFile()
{
	fflush(fp);
	fclose(fp);
}

void Log44::LogOutputToFile::writeLog(std::string& final_log_content)
{
	fputs((final_log_content + "\n").c_str(), fp);
}

Log44::ILogFormatter::~ILogFormatter() {}
Log44::ILogOutputTarget::~ILogOutputTarget() {}
