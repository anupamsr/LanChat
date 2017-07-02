#ifndef LANCHAT_SINGLETON_H
#define LANCHAT_SINGLETON_H

#include <string>
#include "log4cxx/logger.h"

template<typename T>
class Singleton
{
private:
    static T instance; /* C++11 standard says it will be initialized in thread safe manner */

    Singleton();

    Singleton &
    operator=(const Singleton &);

public:
    static T &
    getInstance()
    {
        return instance;
    }
};

/* Static member variables must have fully qualified declarations outside class definition */
template<typename T> T Singleton<T>::instance;

class LanChatLogger
{
private:
    static log4cxx::LoggerPtr logger;

    LanChatLogger();

    LanChatLogger &
    operator=(const LanChatLogger &);
public:
    static log4cxx::LoggerPtr &
    getLogger();
};

#endif //LANCHAT_SINGLETON_H
