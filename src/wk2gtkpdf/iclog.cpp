#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "iclog.h"

#include <fcntl.h>
#include <mutex>
#include <string>
#include <syslog.h>
#include <unistd.h>

extern "C" {
uint32_t LOG_LEVEL  = LOG_DEBUG;
uint64_t LOG_IGNORE = 0;
}

namespace iclog {

    class file_target {
        private:
            int        m_fd = -1;
            std::mutex m_mutex;

        public:
            file_target() = default;

            ~file_target() {
                if (m_fd >= 0)
                    ::close(m_fd);
            }

            bool open_path(const char *filepath) {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (m_fd >= 0)
                    ::close(m_fd);
                m_fd = ::open(filepath, O_WRONLY | O_CREAT | O_APPEND, 0666);
                return (m_fd >= 0);
            }

            void write_data(const std::string &data) {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (m_fd >= 0)
                    ::write(m_fd, data.c_str(), data.size());
            }

            int get_fd() { return m_fd; }
    };

    // The Lightweight Pimpl Layer (0% Ostream buffer dependencies)
    struct ostream_impl {
            // High-performance thread-safe accumulation buffer
            inline static thread_local std::string tl_buffer;

            int          m_level    = LOG_INFO;
            uint64_t     m_category = CORE;
            file_target *target     = nullptr;

            ~ostream_impl() { delete target; }

            void append(const char *str) {
                if (str)
                    tl_buffer += str;
            }

            void commit_and_flush() {
                if (!tl_buffer.empty()) {
                    if (!(LOG_IGNORE & m_category)) {
                        if (target && target->get_fd() >= 0) {
                            target->write_data(tl_buffer);
                        } else {
                            syslog(m_level, "%s", tl_buffer.c_str());
                        }
                    }
                    tl_buffer.clear();
                }
            }
    };

    // logstream Core Lifecycle Management
    logstream::logstream()
        : m_pimpl(new ostream_impl()) {}

    logstream::~logstream() { delete m_pimpl; }

    logstream &logstream::operator<<(const char *s) {
        m_pimpl->append(s);
        return *this;
    }

    logstream &logstream::operator<<(char *s) {
        m_pimpl->append(s);
        return *this;
    }

    logstream &logstream::operator<<(int i) {
        m_pimpl->append(std::to_string(i).c_str());
        return *this;
    }

    logstream &logstream::operator<<(double d) {
        m_pimpl->append(std::to_string(d).c_str());
        return *this;
    }

    // Concrete safe extractor preserves cross-compilation parameters
    logstream &logstream::operator<<(const std::string &s) {
        m_pimpl->append(s.c_str());
        return *this;
    }

    void logstream::flush() { m_pimpl->commit_and_flush(); }

    void logstream::level(loglevel lev) { m_pimpl->m_level = static_cast<int>(lev); }

    void logstream::category(unsigned long long cat) { m_pimpl->m_category = cat; }

    logstream &endl(logstream &os) {
        os.m_pimpl->append("\n");
        os.m_pimpl->commit_and_flush();
        return os;
    }

    template <typename T>
    logstream &logstream::operator<<(const T &value) {
        m_pimpl->append(std::to_string(value).c_str());
        return *this;
    }

    // Manual Runtime Setup Endpoints
    bool init_file_logging(const char *filepath) {
        if (!filepath || filepath[0] == '\0')
            return false;
        if (!wkJlog.m_pimpl->target)
            wkJlog.m_pimpl->target = new file_target();
        return wkJlog.m_pimpl->target->open_path(filepath);
    }

    int log_file_fd() {
        return (wkJlog.m_pimpl && wkJlog.m_pimpl->target) ? wkJlog.m_pimpl->target->get_fd() : -1;
    }

    // Look-Up Tables
    const std::pair<category, std::string> catLUT[]{
        {SQL_SEL,     "(SQL SELECT) "            },
        {SQL_UPD,     "(SQL UPDATE) "            },
        {SQL_INS,     "(SQL INSERT) "            },
        {SQL_STAT,    "(SQL STATEMENT) "         },
        {SQL,         "(SQL) "                   },
        {IF_NCURSEES, "(NCURSES INTERFACE) "     },
        {IF_WEB,      "(WEB INTERFACE) "         },
        {DAEMON,      "(DAEMON) "                },
        {CORE,        "(CORE) "                  },
        {UNDEF,       "(UNDEFINED) "             },
        {SEC_WEB,     "(SECURITY WEB) "          },
        {SEC_NCURSES, "(SECURITY NCURSES) "      },
        {LIB,         "(LIBRARY) "               },
        {CLI,         "(COMMAND LINE INTERFACE) "},
        {LIB_HTML,    "(HTML LIBS) "             },
        {HTML,        "(HTML) "                  }
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

    const std::string &get_category_name(category cat) {
        for (const auto &pair : catLUT) {
            if (pair.first == cat)
                return pair.second;
        }
        static std::string unknown = "[UNK_CAT] ";
        return unknown;
    }

    const std::string &get_level_name(loglevel lev) {
        for (const auto &pair : levelLUT) {
            if (pair.first == lev)
                return pair.second;
        }
        static std::string unknown = "[UNK_LVL] ";
        return unknown;
    }

    logstream &operator<<(logstream &os, category cat) {
        os << get_category_name(cat).c_str();
        os.category(static_cast<unsigned long long>(cat));
        return os;
    }

    logstream &operator<<(logstream &os, loglevel lev) {
        os << get_level_name(lev).c_str();
        os.level(lev);
        return os;
    }

} // namespace iclog

ICLOG_API iclog::logstream wkJlog;

namespace iclog {
    // Explicit compilation mappings for primitive lookup variants
    template ICLOG_API logstream &logstream::operator<<(const unsigned int &);
    template ICLOG_API logstream &logstream::operator<<(const unsigned long &);
    template ICLOG_API logstream &logstream::operator<<(const unsigned short &);
    template ICLOG_API logstream &logstream::operator<<(const long &);
} // namespace iclog
