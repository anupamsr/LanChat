#include "Singleton.h"

log4cxx::LoggerPtr &
LanChatLogger::getLogger()
{
    return logger;
}

log4cxx::LoggerPtr LanChatLogger::logger(log4cxx::Logger::getLogger("LanChat"));
