#ifndef NC_ERRLOG_H
#define NC_ERRLOG_H
#include <cstdint>
#include <syslog.h>
#define DEBUG     true
#define DEBUG_SQL true

#define iclog_FINFO      "[File: " << __FILE_NAME__ << " Line: " << __LINE__ << "] - "
#define iclog_BREAKPOINT "[File: " << __FILE_NAME__ << " Line: " << __LINE__ << "] - (" << __PRETTY_FUNCTION__ << ")"
#define iclog_FUNCTION   "[" << __PRETTY_FUNCTION__ << "] - "

#ifndef ICLOG_API
#define ICLOG_API __attribute__((visibility("default")))
#endif

// Wrapping in extern "C" makes the symbol names stable and easy to map
extern "C" {
extern ICLOG_API uint32_t LOG_LEVEL;
/**
 * @brief LOG_IGNORE
 *
 * A bitwise set of events **NOT** to log
 */
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

    struct ostream_impl;

    class logstream;
    typedef logstream &(*logstreamManipulator)(logstream &);

    class ICLOG_API logstream {
        public:
            logstream();
            ~logstream();

            logstream &operator<<(const char *s);
            logstream &operator<<(int i);
            logstream &operator<<(double d);

            logstream &operator<<(logstreamManipulator manip) {
                return manip(*this);
            }

            void flush();

            void                        level(loglevel lev);
            void                        category(unsigned long long cat);
            friend ICLOG_API logstream &endl(logstream &os);

            template <typename T>
            logstream &operator<<(T &(*f)(T &));

            template <typename T>
            logstream &operator<<(const T &value);

        private:
            ostream_impl *m_pimpl;
    };

    // This is the bridge that allows wkJlog << std::endl;
    // It exists only to catch the standard manipulator
    ICLOG_API logstream &endl(logstream &os);
    ICLOG_API logstream &operator<<(logstream &os, loglevel lev);
    ICLOG_API logstream &operator<<(logstream &os, category cat);
} // namespace iclog

// The global logger instance
extern ICLOG_API iclog::logstream wkJlog;

#endif // NC_ERRLOG_H
