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

    class log_manager {
        private:
            // Primitive integer FD: Zero memory overhead, zero stream buffer traps
            int        m_log_fd = -1;
            std::mutex m_log_mutex;

            // Default constructor initializes safely into a dormant state
            log_manager() = default;

        public:
            log_manager(const log_manager &)            = delete;
            log_manager &operator=(const log_manager &) = delete;

            static log_manager &get_instance() {
                static log_manager instance;
                return instance;
            }

            // RAII Destructor: Safely flushes and unmaps the kernel handle automatically
            ~log_manager() {
                std::lock_guard<std::mutex> lock(m_log_mutex);
                if (m_log_fd >= 0) {
                    close(m_log_fd);
                    m_log_fd = -1;
                }
            }

            bool set_file_target(const char *filepath) {
                if (!filepath || filepath[0] == '\0')
                    return false;

                std::lock_guard<std::mutex> lock(m_log_mutex);

                // Safely close the existing handle if the path is recycled
                if (m_log_fd >= 0) {
                    close(m_log_fd);
                    m_log_fd = -1;
                }

                // Open the file descriptor directly using native kernel flags
                // O_APPEND ensures multi-threaded batch jobs write atomically without collision
                m_log_fd = open(filepath, O_WRONLY | O_CREAT | O_APPEND, 0666);
                return (m_log_fd >= 0);
            }

            void write_to_file(const std::string &data) {
                std::lock_guard<std::mutex> lock(m_log_mutex);
                if (m_log_fd >= 0) {
                    // Raw, bare-metal kernel write operation
                    ::write(m_log_fd, data.c_str(), data.size());
                }
            }

            bool has_active_file() {
                std::lock_guard<std::mutex> lock(m_log_mutex);
                return (m_log_fd >= 0);
            }

            int log_file_fd() {
                return (m_log_fd);
            }
    };

    bool init_file_logging(const char *filepath) {
        return log_manager::get_instance().set_file_target(filepath);
    }

    int log_file_fd() {
        return log_manager::get_instance().log_file_fd();
    }

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
            int      m_level    = LOG_INFO;
            uint64_t m_category = CORE;

        private:
            inline static thread_local std::string tl_buffer;

        protected:
            int sync() override {
                if (!tl_buffer.empty()) {
                    if (!(LOG_IGNORE & m_category)) {

                        log_manager &manager = log_manager::get_instance();

                        if (manager.has_active_file()) {
                            // Path A: Pure file writing via the hot kernel descriptor
                            manager.write_to_file(tl_buffer);
                        } else {
                            // Path B: Fallback to your classic universal POSIX syslog channel
                            syslog(m_level, "%s", tl_buffer.c_str());
                        }
                    }
                    tl_buffer.clear();
                }
                return 0;
            }

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

            ostream_impl()
                : stream(&buf) {}
    };

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
