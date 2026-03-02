#include "iclog.h"

#include <fstream>
#include <iostream>
#include <sstream>

using std::endl;
using std::ofstream;
using std::string;
using std::stringstream;

namespace iclog {
    const std::pair<category, std::string> catLUT[]{
        {SQL_SEL,     "(SQL SELECT) "          },
        {SQL_UPD,     "(SQL UPDATE) "          },
        {SQL_INS,     "(SQL INSERT) "          },
        {SQL_STAT,    "(SQL STATEMENT) "       },
        {SQL,         "(SQL) "                 },
        {IF_NCURSEES, "(NCURSES INTERFACE) "   },
        {IF_WEB,      "(WEB INTERFACE) "       },
        {DAEMON,      "(DAEMON) "              },
        {CORE,        "(CORE) "                },
        {UNDEF,       "(UNDEFINED) "           },
        {SEC_WEB,     "SECURITY WEB "          },
        {SEC_NCURSES, "SECURITY NCURSES "      },
        {LIB,         "LIBRARY "               },
        {CLI,         "COMMAND LINE INTERFACE "},
        {LIB_HTML,    "HTML LIBS "             },
        {HTML,        "HTML "                  }
    };

    const std::pair<unsigned, std::string> levelLUT[]{
        {LOG_EMERG,   "EMERGENCY: "  },
        {LOG_ALERT,   "ALERT: "      },
        {LOG_CRIT,    "CRITICAL: "   },
        {LOG_ERR,     "ERROR: "      },
        {LOG_WARNING, "WARNING: "    },
        {LOG_NOTICE,  "NOTICE: "     },
        {LOG_INFO,    "INFORMATION: "},
        {LOG_DEBUG,   "DEBUG: "      }
    };

    static const std::string unknown_level = "UNKNOWN: ";
    static const std::string unknown_cat   = "(UNKNOWN) ";

    ICLOG_API const std::string &get_level(unsigned level) {
        for (const auto &it : levelLUT) {
            if (level == it.first)
                return it.second;
        }
        return unknown_level;
    }

    ICLOG_API const std::string &get_category(category cat) {
        for (const auto &it : catLUT) {
            if (cat == it.first)
                return it.second;
        }
        return unknown_cat;
    }

    typedef typename iclog::ostream::traits_type::int_type int_type;

    /**************************************/
    /*  CONSTRUCTOR                       */
    /**************************************/
    streambuf::streambuf()
        : m_buf(""),
          m_level(loglevel::debug),
          m_category(CORE) {
        m_buf.reserve(1024);
    }

    /**
     * @brief iclog::streambuf::level
     * @param level
     *
     * Set the level of the alert
     */
    void streambuf::level(loglevel level) { m_level = level; }

    /**
     * @brief iclog::streambuf::sync
     * @return
     *
     * Write the contents of the buffer to syslog()
     */
    int streambuf::sync() {
        if (m_buf.size()) {

            if ((LOG_IGNORE & m_category) == 0)
                syslog(m_level, "%s", m_buf.c_str());

            m_buf.erase();
        }
        return 0;
    }

    int_type streambuf::overflow(int_type c) {
        if (c == traits_type::eof())
            sync();
        else
            m_buf += static_cast<char>(c);
        return c;
    }

    void streambuf::category(BITWISE cat) { m_category = cat; }

    /**
     * @brief iclog::ostream::ostream
     *
     * Ostream class
     */
    ostream::ostream()
        : std::ostream(&m_logbuf) {}

    void ostream::category(BITWISE cat) {
        m_logbuf.category(cat);
    }

    /**
     * @brief iclog::ostream::level
     * @param level
     *
     * Set the log level
     */
    void ostream::level(loglevel level) { m_logbuf.level(level); }

    redirect::redirect(std::ostream &sSource)
        : m_sSource(sSource),
          m_sbuf(sSource.rdbuf(m_sSource.rdbuf())) {

        m_sSource << (&sSource == &std::cout ? loglevel::info : loglevel::error);
    }

    redirect::~redirect() {
        m_sSource.rdbuf(m_sbuf);
    }

} // namespace iclog

ICLOG_API unsigned LOG_LEVEL  = LOG_NOTICE;
ICLOG_API BITWISE  LOG_IGNORE = 0;
ICLOG_API iclog::ostream wkJlog;
