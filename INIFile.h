#ifndef LANCHAT_INIFILE_H
#define LANCHAT_INIFILE_H

#include <fstream>
#include <vector>
#include "Globals.h"

class INIFile
{
public:
    INIFile();
    ~INIFile();

    std::vector< Client > Read();
    bool Write();
    std::string GetName() const;
private:
    static bool validate(const std::string & _line);

    std::fstream m_ini_file;
    std::string m_name;
};


#endif //LANCHAT_INIFILE_H
