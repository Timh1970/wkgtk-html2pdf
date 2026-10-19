#ifndef NC_ERRLOG_H
#define NC_ERRLOG_H

#include <cstdint>
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

extern "C" {
extern ICLOG_API uint32_t LOG_LEVEL;
extern ICLOG_API uint64_t LOG_IGNORE;
}

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
        HTML        = 0x4000ULL
    } category;

    typedef enum {
        emerg    = LOG_EMERG,
        alert    = LOG_ALERT,
        critical = LOG_CRIT,
        error    = LOG_ERR,
        warning  = LOG_WARNING,
        notice   = LOG_NOTICE,
        info     = LOG_INFO,
        debug    = LOG_DEBUG
    } loglevel;

    struct ostream_impl;

    class logstream;
    typedef logstream &(*logstreamManipulator)(logstream &);

    class ICLOG_API logstream {
        public:
            logstream();
            ~logstream();

            logstream &operator<<(const char *s);
            logstream &operator<<(char *s); // Catches mutable char strings
            logstream &operator<<(int i);
            logstream &operator<<(double d);

            // Concrete member operator protects ABI stability across dynamic link rules
            logstream &operator<<(const std::string &s);

            logstream &operator<<(logstreamManipulator manip) {
                return manip(*this);
            }

            void flush();

            void level(loglevel lev);
            void category(unsigned long long cat);

            // Compiler Friend grants give global hooks explicit access to target Pimpls
            friend bool                 init_file_logging(const char *filepath);
            friend int                  log_file_fd();
            friend ICLOG_API logstream &endl(logstream &os);

            // Generic fallback template for primitives
            template <typename T>
            logstream &operator<<(const T &value);

        private:
            ostream_impl *m_pimpl;
    };

    ICLOG_API logstream &endl(logstream &os);
    ICLOG_API logstream &operator<<(logstream &os, loglevel lev);
    ICLOG_API logstream &operator<<(logstream &os, category cat);

    ICLOG_API bool init_file_logging(const char *filepath);
    ICLOG_API int  log_file_fd();

} // namespace iclog

extern ICLOG_API iclog::logstream wkJlog;

#endif // NC_ERRLOG_H
