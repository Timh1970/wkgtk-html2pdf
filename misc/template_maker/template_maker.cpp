#include <fstream>
#include <getopt.h>
#include <iomanip>
#include <iostream>
#include <string>

void create_stylesheet(std::string name, double w, double h, std::string orientation, double marginH, double marginV);

/**
 * @brief main
 * @return
 *
 * A quick utility to generate the css templates for use with wkgtkprinter
 */
int main(int argc, char *argv[]) {

    double marginH      = 16.9;
    double marginV      = 15.8;
    int    value        = 0;
    int    option_index = 0;

    static struct option long_options[] = {
        {"help",   no_argument,       0, 'h'},
        {"margin", required_argument, 0, 'm'},
        {NULL,     0,                 0, 0  }
    };

    while ((value = getopt_long(argc, argv, ":hm:", long_options, &option_index)) != -1) {
        switch (value) {
            case 'h':
                std::cout << "USAGE: " << argv[0] << " [--margin (-m) X]" << std::endl;
                exit(0);
            case 'm': {
                std::string v(optarg);
                for (char c : v) {
                    if (!std::isdigit(c)) {
                        std::cout << "ERROR: Value is not numeric; USAGE: " << argv[0] << " --margin (-m) X" << std::endl;
                        exit(1);
                    }
                }
                marginH = atoi(optarg);
                marginV = atoi(optarg);
            } break;
            case ':': // Missing argument
                std::cout << "ERROR: Option -" << (char)optopt << " requires an argument." << std::endl;
                std::cout << "USAGE: " << argv[0] << " [--margin (-m) X]" << std::endl;
                exit(1);
            case '?': // Unknown option
                std::cout << "ERROR: Unknown option -" << (char)optopt << std::endl;
                exit(1);
        }
    }

    struct PaperSize {
            const char *sizeName;
            double      shortMM;
            double      longMM;
    };

    static const PaperSize isoPaperSizes[52]{
        {"A0",      841.0222,  1188.8611},
        {"A1",      593.7250,  841.0222 },
        {"A2",      419.8056,  593.7250 },
        {"A3",      296.6861,  419.8056 },
        {"A4",      209.9028,  296.6861 }, // The magic 296.68 (841pt)
        {"A5",      147.8139,  209.9028 },
        {"A6",      104.7750,  147.8139 },
        {"A7",      73.7306,   104.7750 },
        {"A8",      51.8583,   73.7306  },
        {"A9",      36.6889,   51.8583  },
        {"A10",     25.7528,   36.6889  },
        {"SRA0",    899.9361,  1279.8778},
        {"SRA1",    639.9389,  899.9361 },
        {"SRA2",    449.7917,  639.9389 },
        {"SRA3",    319.9694,  449.7917 },
        {"SRA4",    224.7194,  319.9694 },
        {"B0",      1000.1250, 1413.9306},
        {"B1",      706.9667,  1000.1250},
        {"B2",      499.8861,  706.9667 },
        {"B3",      352.7778,  499.8861 },
        {"B4",      249.7667,  352.7778 },
        {"B5",      175.6833,  249.7667 },
        {"B6",      124.8833,  175.6833 },
        {"B7",      87.8417,   124.8833 },
        {"B8",      61.7361,   87.8417  },
        {"B9",      43.7444,   61.7361  },
        {"B10",     30.6917,   43.7444  },
        {"C0",      916.8694,  1296.8111},
        {"C1",      647.7000,  916.8694 },
        {"C2",      457.9056,  647.7000 },
        {"C3",      323.8500,  457.9056 },
        {"C4",      228.9528,  323.8500 },
        {"C5",      161.9250,  228.9528 },
        {"C6",      113.9472,  161.9250 },
        {"C7",      80.7861,   113.9472 },
        {"C8",      56.7972,   80.7861  },
        {"C9",      39.8639,   56.7972  },
        {"C10",     27.8694,   39.8639  },
        {"ANSIA",   215.9000,  279.4000 }, // Perfect 612x792pt
        {"ANSIB",   279.4000,  431.8000 },
        {"ANSIC",   431.8000,  558.8000 },
        {"ANSID",   558.8000,  863.6000 },
        {"ANSIE",   863.6000,  1117.6000},
        {"Letter",  215.9000,  279.4000 }, // 11 inches exactly
        {"Legal",   215.9000,  355.6000 }, // 14 inches exactly
        {"Tabloid", 279.4000,  431.8000 }, // 17 inches exactly
        {"ArchA",   228.6000,  304.8000 },
        {"ArchB",   304.8000,  457.2000 },
        {"ArchC",   457.2000,  609.6000 },
        {"ArchD",   609.6000,  914.4000 },
        {"ArchE",   914.4000,  1219.2000},
        {nullptr,   0.0,       0.0      }
    };
    for (const PaperSize &size : isoPaperSizes) {
        if (size.sizeName == nullptr)
            break;
        // Portrait
        create_stylesheet(size.sizeName, size.shortMM, size.longMM, "portrait", marginH, marginV);
        // Landscape (Swap the Point-aligned dimensions)
        create_stylesheet(size.sizeName, size.longMM, size.shortMM, "landscape", marginH, marginV);
    }

    return (0);
}

template <typename... Args>
std::string build_string(Args &&...args) {
    std::ostringstream oss;
    // Set fixed precision for our high-precision doubles
    oss << std::fixed << std::setprecision(4);
    (oss << ... << args);
    return oss.str();
}

/**
 * @brief create_stylesheet
 * @param name
 * @param w
 * @param h
 * @param orientation
 * @param marginH
 * @param marginV
 *
 * @note The section entitled PREVENT WEBKIT INTERAL ROUNDING
 * may seeem a little convoluted but it is necessary to give
 * webkit a number that it does not need to calculate internally
 * because doing so causes rounding errors.
 *
 * We have not solved the rounding errors by doing this, we have
 * just taken control of them. This is what we are doing:
 *
 * - Convert mm to pixels
 * - Round down the pixel result
 * - Convert the rounded value back to points and knock a point off
 *
 *
 *  1. The mm to pt Calculation (The Creep Problem)
 *     This ensures Internal Consistency.
 *
 *  The Goal: You want your 12pt font and your 18pt line-height to fit perfectly into your page margins.
 *  The Reason: If you don't define the page in points, the browser has to "guess" how many 18pt lines fit into a 297mm page. Because
 *  is *, that infinite decimal causes the "creep" where line 40 is slightly further down than line 1.
 *  The Result: Total vertical harmony for your text.
 *
 *  2. The mm to px Calculation (The Overflow Problem)
 *     This ensures External Compatibility (with Webkit/Blink).
 *
 *  The Goal: To stop the "Ghost Page" (the 44th blank page).
 *  The Reason: Browsers don't actually "paint" in points or millimetres; they paint in Pixels. Webkit calculates the total page height in pixels and floors it (snaps to the nearest physical pixel). If your "Pure Math" height is even 0.001pt taller than Webkit’s "Floored Pixel" height, the engine thinks the content has overflowed the page.
 *  The Result: You are pre-calculating the browser's "Hard Ceiling."
 */
void create_stylesheet(std::string name, double w, double h, std::string orientation, double marginH, double marginV) {

    std::cout << "Generating stylesheet for " << name << " (" << orientation << ")" << std::endl;

    std::string css = build_string(
        /* clang-format off */
        ":root {\n",
        "    --page-width: ", w, ";\n",
        "    --page-height: ", h, ";\n",
        "    --page-margin-h: ", marginH, ";\n",
        "    --page-margin-v: ", marginV, ";\n",
        "    --default-font-size: 12;\n",
        "    --default-line-height: 1.5;\n",
        "    --default-font-family: \"Liberation Sans\", sans-serif;\n",

        "    /* --- INTERNAL CALCULATIONS (explicit) --- */\n",
        "    --height-pure-px: calc(var(--page-height) * 96 / 25.4);\n",
        "    --height-floored-px: round(down, var(--height-pure-px), 1);\n",
        "    --webkit-height-val: calc(var(--height-floored-px) * 0.75pt);\n",

        "    --margin-v-pure-px: calc(var(--page-margin-v) * 96 / 25.4);\n",
        "    --margin-v-floored: calc(round(down, var(--margin-v-pure-px), 1) * 0.75pt);\n",

        "    --lh-pure-px: calc(var(--default-font-size) * var(--default-line-height) * 96 / 72);\n",
        "    --lh-floored-px: round(down, var(--lh-pure-px), 1);\n",
        "    --real-line-height: calc(var(--lh-floored-px) * 0.75pt);\n",
        "}\n\n",

        "* {\n",
        "    box-sizing: border-box;\n",
        "    margin: 0; padding: 0;\n",
        "    line-height: var(--real-line-height);\n",
        "    font-family: var(--default-font-family);\n",
        "    font-size: calc(var(--default-font-size) * 1pt);\n",
        "}\n\n",

        "@page {\n",
        "    size: calc(var(--page-width) * 1mm) calc(var(--page-height) * 1mm);\n",
        "    margin: 0;\n",
        "}\n\n",

        "html, body {\n",
        "    width: calc(var(--page-width) * 1mm);\n",
        "    margin: 0; padding: 0;\n",
        "    background-color: transparent !important;\n",
        "}\n\n",

        ".page {\n",
        "    width: calc(var(--page-width) * 1mm);\n",
        "    height: var(--webkit-height-val);\n",
        "    background-color: white !important;\n",
        "    display: grid;\n",
        "    grid-template-columns: calc(var(--page-margin-h) * 1mm) 1fr calc(var(--page-margin-h) * 1mm);\n",
        "    grid-template-rows: var(--margin-v-floored) 1fr var(--margin-v-floored);\n",
        "    break-after: page;\n",
        "    position: relative;\n",
        "    overflow: hidden;\n",
        "    border-radius: 0;\n",
        "    box-shadow: 0 0 5px rgba(0, 0, 0, 0.1);\n",
        "}\n\n",

        ".subpage {\n",
        "    grid-area: 2 / 2 / 3 / 3;\n",
        "    display: flex;\n",
        "    flex-direction: column;\n",
        "    position: relative;\n",
        "    overflow: hidden;\n",
        "    outline: 1pt solid blue;\n",
        "}\n\n",

        "@media print {\n",
        "    .page { border: none; box-shadow: none; }\n",
        "    .subpage { outline: none; }\n",
        "}\n"
        /* clang-format on */
    );

    std::string   filename = name + "-" + orientation + ".css";
    std::ofstream file(filename);
    if (file) {
        file << css;
        file.close();
    }
}
