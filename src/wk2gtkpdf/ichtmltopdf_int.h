#ifndef ICHTMLTOPDF_INT_H
#define ICHTMLTOPDF_INT_H

#ifndef PDF_INTI_API
#define PDF_INTI_API __attribute__((visibility("default")))
#endif

// Opaque implementations (only the .cpp knows what's inside these)
struct WKGTK_init_impl;
struct icGTK_impl;
/**
 * @brief The XvfbMode enum
 *
 * Modes to run xvfb in.
 *
 * @note START_STOP may be used in some edge cases, but is only
 * really intened for testing.  The recommended way to run the
 * aplicaiton is to leave xvfb running.
 *
 * @note xvfb Is only required on headless servers, it will not be started
 * unless it is needed.
 *
 */
enum class XvfbMode {
    START_STOP,   // Start on demand, stop on exit
    KEEP_RUNNING, // Start on demand, keep running
};

class PDF_INTI_API WKGTK_init {
    public:
        WKGTK_init();
        // MOVE CONSTRUCTOR
        WKGTK_init(WKGTK_init &&other) noexcept;
        ~WKGTK_init();

    private:
        WKGTK_init_impl *m_pimpl;
};

class PDF_INTI_API icGTK {
    public:
        static icGTK &init();
        static icGTK &init(XvfbMode runMode);

        icGTK(const icGTK &)            = delete;
        icGTK &operator=(const icGTK &) = delete;

    private:
        icGTK(XvfbMode runMode);
        ~icGTK();
        icGTK_impl *m_pimpl;
};

#endif // ICHTMLTOPDF_INT_H
