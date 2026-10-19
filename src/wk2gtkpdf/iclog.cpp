#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

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

extern "C" {
uint32_t LOG_LEVEL  = LOG_DEBUG;
uint64_t LOG_IGNORE = 0;
}

namespace iclog {

    // Forward declaration of the container so the buffer can reference it safely
    struct ostream_impl;

    class streambuf_internal : public std::streambuf {
        public:
            int           m_level    = LOG_INFO;
            uint64_t      m_category = CORE;
            ostream_impl *m_parent   = nullptr; // Pointer back to our layout holder

        private:
            inline static std::string tl_buffer;
            inline static std::mutex  tl_mutex;

        protected:
            int sync() override;

            int_type overflow(int_type c) override {
                if (c != traits_type::eof()) {
                    tl_buffer += static_cast<char>(c);
                }
                return c;
            }
    };

    // 3. The Pimpl Container on the Heap
    struct ostream_impl {
            streambuf_internal buf;
            std::ostream       stream;
            int                log_fd = -1; // The file handle lives right here on the heap!
            std::mutex         log_mutex;

            ostream_impl()
                : stream(&buf) {
                buf.m_parent = this; // Link the buffer back to this holder
            }

            ~ostream_impl() {
                std::lock_guard<std::mutex> lock(log_mutex);
                if (log_fd >= 0) {
                    ::close(log_fd);
                    log_fd = -1;
                }
            }
    };

    // Implement the sync mechanism using our nested parent context
    int streambuf_internal::sync() {
        if (!tl_buffer.empty()) {
            if (!(LOG_IGNORE & m_category)) {

                // Safe extraction: check if our parent context has an active file descriptor
                if (m_parent && m_parent->log_fd >= 0) {
                    std::lock_guard<std::mutex> lock(m_parent->log_mutex);
                    if (m_parent->log_fd >= 0) {
                        ::write(m_parent->log_fd, tl_buffer.c_str(), tl_buffer.size());
                    }
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

    // 5. Clean, Non-Flaky Initialization Hooks
    bool init_file_logging(const char *filepath) {
        if (!filepath || filepath[0] == '\0')
            return false;

        std::lock_guard<std::mutex> lock(wkJlog.m_pimpl->log_mutex);

        if (wkJlog.m_pimpl->log_fd >= 0) {
            ::close(wkJlog.m_pimpl->log_fd);
        }

        wkJlog.m_pimpl->log_fd = ::open(filepath, O_WRONLY | O_CREAT | O_APPEND, 0666);
        return (wkJlog.m_pimpl->log_fd >= 0);
    }

    int log_file_fd() {
        return wkJlog.m_pimpl->log_fd;
    }

    // --- Keep the rest of your original stream operators / LUT mappings completely identical ---
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

    void logstream::flush() { m_pimpl->stream.flush(); }

    void logstream::level(loglevel lev) { m_pimpl->buf.m_level = static_cast<int>(lev); }

    void logstream::category(unsigned long long cat) { m_pimpl->buf.m_category = cat; }

    logstream &endl(logstream &os) {
        os.m_pimpl->stream << std::endl;
        return os;
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

    template <>
    logstream &logstream::operator<<(std::ostream &(*f)(std::ostream &)) {
        m_pimpl->stream << f;
        return *this;
    }

    template <typename T>
    logstream &logstream::operator<<(const T &value) {
        m_pimpl->stream << value;
        return *this;
    }

} // namespace iclog

ICLOG_API iclog::logstream wkJlog;

namespace iclog {
    template ICLOG_API logstream &logstream::operator<<(const std::string &);
    template ICLOG_API logstream &logstream::operator<<(char *const &);
    template ICLOG_API logstream &logstream::operator<<(const unsigned int &);
    template ICLOG_API logstream &logstream::operator<<(const unsigned long &);
    template ICLOG_API logstream &logstream::operator<<(const unsigned short &);
    template ICLOG_API logstream &logstream::operator<<(const long &);
} // namespace iclog

// #include "iclog.h"

// #include <fcntl.h>
// #include <iostream>
// #include <mutex>
// #include <string>
// #include <syslog.h>
// #include <unistd.h>

// // 1. Global Externs (C-Linkage)
// extern "C" {
// uint32_t LOG_LEVEL  = LOG_DEBUG;
// uint64_t LOG_IGNORE = 0;
// }

// namespace iclog {

//     const std::pair<category, std::string> catLUT[]{
//         {SQL_SEL,     "(SQL SELECT) "            },
//         {SQL_UPD,     "(SQL UPDATE) "            },
//         {SQL_INS,     "(SQL INSERT) "            },
//         {SQL_STAT,    "(SQL STATEMENT) "         },
//         {SQL,         "(SQL) "                   },
//         {IF_NCURSEES, "(NCURSES INTERFACE) "     },
//         {IF_WEB,      "(WEB INTERFACE) "         },
//         {DAEMON,      "(DAEMON) "                },
//         {CORE,        "(CORE) "                  },
//         {UNDEF,       "(UNDEFINED) "             },
//         {SEC_WEB,     "(SECURITY WEB) "          },
//         {SEC_NCURSES, "(SECURITY NCURSES) "      },
//         {LIB,         "(LIBRARY) "               },
//         {CLI,         "(COMMAND LINE INTERFACE) "},
//         {LIB_HTML,    "(HTML LIBS) "             },
//         {HTML,        "(HTML) "                  }
//     };

//     const std::pair<unsigned, std::string> levelLUT[]{
//         {LOG_EMERG,   "EMERGENCY: "  },
//         {LOG_ALERT,   "ALERT: "      },
//         {LOG_CRIT,    "CRITICAL: "   },
//         {LOG_ERR,     "ERROR: "      },
//         {LOG_WARNING, "WARNING: "    },
//         {LOG_NOTICE,  "NOTICE: "     },
//         {LOG_INFO,    "INFORMATION: "},
//         {LOG_DEBUG,   "DEBUG: "      }
//     };

//     const std::string &get_category_name(category cat) {
//         for (const auto &pair : catLUT) {
//             if (pair.first == cat)
//                 return pair.second;
//         }
//         static std::string unknown = "[UNK_CAT] ";
//         return unknown;
//     }

//     const std::string &get_level_name(loglevel lev) {
//         for (const auto &pair : levelLUT) {
//             if (pair.first == lev)
//                 return pair.second;
//         }
//         static std::string unknown = "[UNK_LVL] ";
//         return unknown;
//     }

//     // 2. The Internal Engine (Hidden from Header)
//     // class streambuf_internal : public std::streambuf {
//     //     public:
//     //         std::string m_buf;
//     //         int         m_level    = LOG_INFO;
//     //         uint64_t    m_category = CORE;

//     //     protected:
//     //         int sync() override {
//     //             if (!m_buf.empty()) {
//     //                 if (!(LOG_IGNORE & m_category)) {
//     //                     syslog(m_level, "%s", m_buf.c_str());
//     //                 }
//     //                 m_buf.clear();
//     //             }
//     //             return 0;
//     //         }

//     //         int_type overflow(int_type c) override {
//     //             if (c != traits_type::eof()) {
//     //                 m_buf += static_cast<char>(c);
//     //             }
//     //             return c;
//     //         }
//     // };

//     class streambuf_internal : public std::streambuf {
//         public:
//             int      m_level    = LOG_INFO;
//             uint64_t m_category = CORE;

//         private:
//             inline static thread_local std::string tl_buffer;

//         protected:
//             int sync() override {
//                 if (!tl_buffer.empty()) {
//                     if (!(LOG_IGNORE & m_category)) {

//                         log_manager &manager = log_manager::get_instance();

//                         if (manager.has_active_file()) {
//                             // Path A: Pure file writing via the hot kernel descriptor
//                             manager.write_to_file(tl_buffer);
//                         } else {
//                             // Path B: Fallback to your classic universal POSIX syslog channel
//                             syslog(m_level, "%s", tl_buffer.c_str());
//                         }
//                     }
//                     tl_buffer.clear();
//                 }
//                 return 0;
//             }

//             int_type overflow(int_type c) override {
//                 if (c != traits_type::eof()) {
//                     tl_buffer += static_cast<char>(c);
//                 }
//                 return c;
//             }
//     };

//     // 3. The Pimpl Container
//     struct ostream_impl {
//             streambuf_internal buf;
//             std::ostream       stream;

//             ostream_impl()
//                 : stream(&buf) {}
//     };

//     // 4. logstream Method Implementations
//     logstream::logstream()
//         : m_pimpl(new ostream_impl()) {}

//     logstream::~logstream() { delete m_pimpl; }

//     logstream &logstream::operator<<(const char *s) {
//         if (s)
//             m_pimpl->stream << s;
//         return *this;
//     }

//     logstream &logstream::operator<<(int i) {
//         m_pimpl->stream << i;
//         return *this;
//     }

//     logstream &logstream::operator<<(double d) {
//         m_pimpl->stream << d;
//         return *this;
//     }

//     void logstream::flush() {
//         m_pimpl->stream.flush();
//     }

//     void logstream::level(loglevel lev) {
//         m_pimpl->buf.m_level = static_cast<int>(lev);
//     }

//     void logstream::category(unsigned long long cat) {
//         m_pimpl->buf.m_category = cat;
//     }

//     logstream &endl(logstream &os) {
//         os.m_pimpl->stream << std::endl;
//         return os;
//     }

//     // 6. Enum Operators (Found via ADL)
//     logstream &operator<<(logstream &os, category cat) {
//         os << get_category_name(cat).c_str();
//         os.category(static_cast<unsigned long long>(cat));
//         return os;
//     }

//     logstream &operator<<(logstream &os, loglevel lev) {
//         os << get_level_name(lev).c_str();
//         os.level(lev);
//         return os;
//     }

//     template <>
//     logstream &logstream::operator<<(std::ostream &(*f)(std::ostream &)) {
//         m_pimpl->stream << f;
//         return *this;
//     }

//     // This handles the "Standard" types that the header doesn't know about
//     template <typename T>
//     logstream &logstream::operator<<(const T &value) {
//         m_pimpl->stream << value;
//         return *this;
//     }

//     // Explicitly instantiate for std::string to keep the linker happy
//     // template logstream &logstream::operator<<(const std::string &);

// } // namespace iclog

// ICLOG_API iclog::logstream wkJlog;

// namespace iclog {
//     // Force the compiler to generate the binary code for these types
//     template ICLOG_API logstream &logstream::operator<<(const std::string &);
//     template ICLOG_API logstream &logstream::operator<<(char *const &);
//     template ICLOG_API logstream &logstream::operator<<(const unsigned int &);
//     template ICLOG_API logstream &logstream::operator<<(const unsigned long &);
//     template ICLOG_API logstream &logstream::operator<<(const unsigned short &);
//     template ICLOG_API logstream &logstream::operator<<(const long &);
// } // namespace iclog
