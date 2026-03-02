#ifndef NC_ERRLOG_H
#define NC_ERRLOG_H
#include <ostream>
#include <streambuf>
#include <string>
#include <syslog.h>

#define DEBUG     true
#define DEBUG_SQL true

#define iclog_FINFO      "[File: " << __FILE_NAME__ << " Line: " << __LINE__ << "] - "
#define iclog_BREAKPOINT "[File: " << __FILE_NAME__ << " Line: " << __LINE__ << "] - (" << __PRETTY_FUNCTION__ << ")"
#define iclog_FUNCTION   "[" << __PRETTY_FUNCTION__ << "] - "

#ifndef ICLOG_API
#define ICLOG_API __attribute__((visibility("default")))
#endif

extern unsigned ICLOG_API LOG_LEVEL;

typedef ulong             BITWISE;
/**
 * @brief LOG_IGNORE
 *
 * A bitwise set of events **NOT** to log
 */
extern BITWISE LOG_IGNORE ICLOG_API;

namespace iclog {

    typedef enum : unsigned long long {
        SQL         = 0x01ULL,
        SQL_INS     = 0x02ULL,
        SQL_UPD     = 0x04ULL,
        SQL_SEL     = 0x08ULL,
        SQL_STAT    = 0x10ULL,
        IF_NCURSEES = 0x20ULL,
        IF_WEB      = 0x40ULL,
        DAEMON      = 0x80ULL,
        CORE        = 0x100ULL,
        UNDEF       = 0x200ULL,
        SEC_WEB     = 0x400ULL,
        SEC_NCURSES = 0x800ULL,
        LIB         = 0x1000ULL,
        CLI         = 0x2000ULL,
        LIB_HTML    = 0x4000ULL,
        HTML        = 0x4000ULL /**<  Large HTML strings with embedded images may be flushed by journal */
    } category;

    typedef enum {
        emerg    = LOG_EMERG,   // A panic condition
        alert    = LOG_ALERT,   // A condition that should be corrected
        critical = LOG_CRIT,    // Critical condition, e.g, hard device error
        error    = LOG_ERR,     // Errors
        warning  = LOG_WARNING, // Warning messages
        notice   = LOG_NOTICE,  // Possibly be handled specially
        info     = LOG_INFO,    // Informational
        debug    = LOG_DEBUG    // For debugging program
    } loglevel;

    extern ICLOG_API const std::pair<category, std::string> catLUT[];
    extern ICLOG_API const std::pair<unsigned, std::string> levelLUT[];

    const std::string &get_level(unsigned level);
    const std::string &get_category(category cat);

    //  STREAMBUF CLASS
    class ICLOG_API streambuf : public std::streambuf {
        private:
            std::string m_buf;
            loglevel    m_level;
            int         m_category;

        public:
            ICLOG_API      streambuf();
            void ICLOG_API level(loglevel level);
            void ICLOG_API category(BITWISE cat);

        protected:
            int ICLOG_API      sync();
            int_type ICLOG_API overflow(int_type c);
    };

    // OSTREAM CLASS
    class ICLOG_API ostream : public std::ostream {

        private:
            streambuf m_logbuf;

        public:
            ICLOG_API      ostream();
            void ICLOG_API level(loglevel level);
            void ICLOG_API category(BITWISE cat);
    };

    inline ostream &operator<<(ostream &os, const loglevel lev) {
        os << get_level(lev);
        os.level(lev);
        return os;
    }

    inline ostream &operator<<(ostream &os, const category cat) {
        os << get_category(cat);
        os.category(cat);
        return os;
    }

    // REDIRECT CLASS
    class ICLOG_API redirect {
        private:
            ostream               m_sDest;
            std::ostream         &m_sSource;
            std::streambuf *const m_sbuf;

        public:
            redirect(std::ostream &sSource);
            ~redirect();
    };

} // namespace iclog

extern ICLOG_API iclog::ostream wkJlog;

#endif // NC_ERRLOG_H
