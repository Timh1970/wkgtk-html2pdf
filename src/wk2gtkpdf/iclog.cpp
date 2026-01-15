#include "iclog.h"

#include <fstream>
#include <iostream>
#include <sstream>

using std::endl;
using std::ofstream;
using std::string;
using std::stringstream;

string iclog::get_level(unsigned level) {
    for (const std::pair<unsigned, string> &it : levelLUT) {
        if (level == it.first) {
            return (it.second);
        }
    }
    return (string());
}

string iclog::get_category(category cat) {
    for (const std::pair<category, string> &it : catLUT) {
        if (cat == it.first) {
            return (it.second);
        }
    }
    return (string());
}

typedef typename iclog::ostream::traits_type::int_type int_type;

/**************************************/
/*  CONSTRUCTOR                       */
/**************************************/
iclog::streambuf::streambuf()
    : m_level(loglevel::debug) {}

/**
 * @brief iclog::streambuf::level
 * @param level
 *
 * Set the level of the alert
 */
void iclog::streambuf::level(loglevel level) { m_level = level; }

/**
 * @brief iclog::streambuf::sync
 * @return
 *
 * Write the contents of the buffer to syslog()
 */
int iclog::streambuf::sync() {
    if (m_buf.size()) {

        if ((LOG_IGNORE & m_category) == 0)
            syslog(m_level, "%s", m_buf.c_str());

        m_buf.erase();
    }
    return 0;
}

int_type iclog::streambuf::overflow(int_type c) {
    if (c == traits_type::eof())
        sync();
    else
        m_buf += static_cast<char>(c);
    return c;
}

void iclog::streambuf::category(BITWISE cat) { m_category = cat; }

/**
 * @brief iclog::ostream::ostream
 *
 * Ostream class
 */
iclog::ostream::ostream()
    : std::ostream(&m_logbuf) {}

void iclog::ostream::category(BITWISE cat) {
    m_logbuf.category(cat);
}

/**
 * @brief iclog::ostream::level
 * @param level
 *
 * Set the log level
 */
void iclog::ostream::level(loglevel level) { m_logbuf.level(level); }

/**
 * @brief iclog::redirect::redirect
 * @param src
 *
 * Redirect class
 *
 * @note This only redirects c++ style streams. (std::cout etc.) The underlying
 * streams are unaffected.
 */
iclog::redirect::redirect(std::ostream &src)
    : m_src(src),
      m_sbuf(src.rdbuf(m_dst.rdbuf())) {

    m_dst << (&src == &std::cout ? loglevel::info : loglevel::error);
}

iclog::redirect::~redirect() {
    m_src.rdbuf(m_sbuf);
}

unsigned       LOG_LEVEL  = LOG_NOTICE;
BITWISE        LOG_IGNORE = 0;
iclog::ostream jlog;
