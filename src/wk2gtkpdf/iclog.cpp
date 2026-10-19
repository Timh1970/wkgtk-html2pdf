#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "iclog.h"

#include <fcntl.h>
#include <iostream>
#include <mutex>
#include <string>
#include <syslog.h>
#include <unistd.h>

// 1. Global Externs (C-Linkage)
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

            // Clean manual cleanup
            ~file_target() {
                if (m_fd >= 0) {
                    ::close(m_fd);
                }
            }

            bool open_path(const char *filepath) {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (m_fd >= 0) {
                    ::close(m_fd);
                }
                m_fd = ::open(filepath, O_WRONLY | O_CREAT | O_APPEND, 0666);
                return (m_fd >= 0);
            }

            void write_data(const std::string &data) {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (m_fd >= 0) {
                    ::write(m_fd, data.c_str(), data.size());
                }
            }

            int get_fd() {
                std::lock_guard<std::mutex> lock(m_mutex);
                return m_fd;
            }
    };

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

    // 2. The Internal Engine (Hidden from Header)
    // class streambuf_internal : public std::streambuf {
    //     public:
    //         std::string m_buf;
    //         int         m_level    = LOG_INFO;
    //         uint64_t    m_category = CORE;

    //     protected:
    //         int sync() override {
    //             if (!m_buf.empty()) {
    //                 if (!(LOG_IGNORE & m_category)) {
    //                     syslog(m_level, "%s", m_buf.c_str());
    //                 }
    //                 m_buf.clear();
    //             }
    //             return 0;
    //         }

    //         int_type overflow(int_type c) override {
    //             if (c != traits_type::eof()) {
    //                 m_buf += static_cast<char>(c);
    //             }
    //             return c;
    //         }
    // };

    class streambuf_internal : public std::streambuf {
        public:
            int           m_level    = LOG_INFO;
            uint64_t      m_category = CORE;
            ostream_impl *m_parent   = nullptr;

        private:
            inline static thread_local std::string tl_buffer;

        protected:
            int sync() override;

            int_type overflow(int_type c) override {
                if (c != traits_type::eof()) {
                    tl_buffer += static_cast<char>(c);
                }
                return c;
            }
    };

    // 3. The Pimpl Container
    struct ostream_impl {
            streambuf_internal buf;
            std::ostream       stream;
            file_target       *target = nullptr;

            ostream_impl()
                : stream(&buf) {
                buf.m_parent = this;
            }

            ~ostream_impl() {
                delete target;
            }
    };

    int streambuf_internal::sync() {
        if (!tl_buffer.empty()) {
            if (!(LOG_IGNORE & m_category)) {

                // Safe access: compiler knows exactly what members are inside m_parent
                if (m_parent && m_parent->target && m_parent->target->get_fd() >= 0) {
                    m_parent->target->write_data(tl_buffer);
                } else {
                    syslog(m_level, "%s", tl_buffer.c_str());
                }
            }
            tl_buffer.clear();
        }
        return 0;
    }

    // 4. logstream Method Implementations
    logstream::logstream()
        : m_pimpl(new ostream_impl()) {}

    logstream::~logstream() { delete m_pimpl; }

    logstream &logstream::operator<<(const char *s) {
        if (s)
            m_pimpl->stream << s;
        return *this;
    }

    logstream &logstream::operator<<(int i) {
        m_pimpl->stream << i;
        return *this;
    }

    logstream &logstream::operator<<(double d) {
        m_pimpl->stream << d;
        return *this;
    }

    void logstream::flush() {
        m_pimpl->stream.flush();
    }

    void logstream::level(loglevel lev) {
        m_pimpl->buf.m_level = static_cast<int>(lev);
    }

    void logstream::category(unsigned long long cat) {
        m_pimpl->buf.m_category = cat;
    }

    logstream &endl(logstream &os) {
        os.m_pimpl->stream << std::endl;
        return os;
    }

    // 6. Enum Operators (Found via ADL)
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

    template <>
    logstream &logstream::operator<<(std::ostream &(*f)(std::ostream &)) {
        m_pimpl->stream << f;
        return *this;
    }

    // This handles the "Standard" types that the header doesn't know about
    template <typename T>
    logstream &logstream::operator<<(const T &value) {
        m_pimpl->stream << value;
        return *this;
    }

    // Explicitly instantiate for std::string to keep the linker happy
    // template logstream &logstream::operator<<(const std::string &);

    bool init_file_logging(const char *filepath) {
        if (!filepath || filepath[0] == '\0')
            return false;

        // Directly access the private pointer via the friend permission
        ostream_impl *pimpl = wkJlog.m_pimpl;
        if (!pimpl)
            return false;

        // Lazily allocate the tracking object onto the heap only when requested
        if (!pimpl->target) {
            pimpl->target = new file_target();
        }

        return pimpl->target->open_path(filepath);
    }

    int log_file_fd() {
        ostream_impl *pimpl = wkJlog.m_pimpl;
        return (pimpl && pimpl->target) ? pimpl->target->get_fd() : -1;
    }

} // namespace iclog

ICLOG_API iclog::logstream wkJlog;

namespace iclog {
    // Force the compiler to generate the binary code for these types
    template ICLOG_API logstream &logstream::operator<<(const std::string &);
    template ICLOG_API logstream &logstream::operator<<(char *const &);
    template ICLOG_API logstream &logstream::operator<<(const unsigned int &);
    template ICLOG_API logstream &logstream::operator<<(const unsigned long &);
    template ICLOG_API logstream &logstream::operator<<(const unsigned short &);
    template ICLOG_API logstream &logstream::operator<<(const long &);
} // namespace iclog
