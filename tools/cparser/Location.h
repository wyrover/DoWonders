#ifndef LOCATION_H_
#define LOCATION_H_

//
// CP_Location
//
struct CP_Location
{
    std::string m_file;
    int m_line;

    CP_Location() : m_file(""), m_line(1) { }

    CP_Location(const CP_Location& loc)
    : m_file(loc.m_file), m_line(loc.m_line) { }

    CP_Location(const char *file, int line) : m_file(file), m_line(line) { }

    CP_Location(const std::string& file, int line) :
        m_file(file), m_line(line) { }

    void set(const char *file, int line) {
        m_file = file;
        m_line = line;
    }

    void operator=(const CP_Location& loc) {
        m_file = loc.m_file;
        m_line = loc.m_line;
    }

    CP_Location& operator++() {
        m_line++;
        return *this;
    }

    CP_Location operator++(int) {
        CP_Location loc(*this);
        m_line++;
        return loc;
    }

    std::string to_string() const {
        std::string str = m_file;
        char buf[32];
        std::sprintf(buf, " (%d)", m_line);
        str += buf;
        return str;
    }
};

#endif  // def LOCATION_H_
