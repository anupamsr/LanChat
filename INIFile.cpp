#include "INIFile.h"

INIFile::INIFile()
    : m_ini_file(), m_name("SERVER")
{
    m_ini_file.open(INI_FILE, std::ios::in | std::ios::out);
}

INIFile::~INIFile()
{
    m_ini_file.close();
}

std::vector< Client >
INIFile::Read()
{
    std::vector< Client > client_list;
    if (m_ini_file.is_open())
    {
        std::string line;
        bool name_read = false;
        while (m_ini_file.eof())
        {
            std::getline(m_ini_file, line);
            if (validate(line))
            {
                client_list.push_back(Client(line));
            }
            else if (!name_read)
            {
                m_name = line;
                name_read = true;
            }
        }
    }
    return client_list;
}

bool
INIFile::Write()
{
    return false;
}

bool
INIFile::validate(const std::string & _line)
{
    bool valid = false;
    std::string address = _line.substr(0, _line.find(" "));
    valid = address.size() > 0;
    size_t dot_pos = address.find(".");
    if (dot_pos != std::string::npos && dot_pos > 0)
    {
        size_t dot2_pos = address.find(".", dot_pos);
        if (dot2_pos != std::string::npos && dot2_pos > dot_pos)
        {
            dot_pos = address.find(".", dot2_pos);
            if (dot_pos != std::string::npos && dot_pos > dot2_pos)
            {
                valid = true;
            }
        }
    }
    return valid;
}

std::string
INIFile::GetName() const
{
    return m_name;
}
