#ifndef LANCHAT_SINGLETON_H
#define LANCHAT_SINGLETON_H

#include <string>

template<typename T>
class Singleton
{
private:
    static T instance; /* Standard says it will be set to nullptr */

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

#endif //LANCHAT_SINGLETON_H
