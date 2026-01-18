#include <fstream>
#include <iostream>
#include <string>
#include <vector>

void create_stylesheet(std::string name, int w, int h, std::string orientation);

/**
 * @brief main
 * @return
 *
 * A quick utility to generate the css templates for use with wkgtkprinter
 */
int main() {
    struct PaperSize {
            std::string sizeName;
            uint        shortMM;
            uint        longMM;
    };

    const std::vector<PaperSize> isoPaperSizes = {
        {"A0",      841,  1189},
        {"A1",      594,  841 },
        {"A2",      420,  594 },
        {"A3",      297,  420 },
        {"A4",      210,  297 },
        {"A5",      148,  210 },
        {"A6",      105,  148 },
        {"A7",      74,   105 },
        {"A8",      52,   74  },
        {"A9",      37,   52  },
        {"A10",     26,   37  },
        {"SRA0",    900,  1280},
        {"SRA1",    640,  900 },
        {"SRA2",    450,  640 },
        {"SRA3",    320,  450 },
        {"SRA4",    225,  320 },
        {"B0",      1000, 1414},
        {"B1",      707,  1000},
        {"B2",      500,  707 },
        {"B3",      353,  500 },
        {"B4",      250,  353 },
        {"B5",      176,  250 },
        {"B6",      125,  176 },
        {"B7",      88,   125 },
        {"B8",      62,   88  },
        {"B9",      44,   62  },
        {"B10",     31,   44  },
        {"C0",      917,  1297},
        {"C1",      648,  917 },
        {"C2",      458,  648 },
        {"C3",      324,  458 },
        {"C4",      229,  324 },
        {"C5",      162,  229 },
        {"C6",      114,  162 },
        {"C7",      81,   114 },
        {"C8",      57,   81  },
        {"C9",      40,   57  },
        {"C10",     28,   40  },
        {"ANSIA",   216,  279 },
        {"ANSIB",   279,  432 },
        {"ANSIC",   432,  559 },
        {"ANSID",   559,  864 },
        {"ANSIE",   864,  1118},
        {"Letter",  216,  279 },
        {"Legal",   216,  356 },
        {"Tabloid", 279,  432 },
        {"ArchA",   229,  305 },
        {"ArchB",   305,  457 },
        {"ArchC",   457,  610 },
        {"ArchD",   610,  914 },
        {"ArchE",   914,  1219}
    };
    for (const auto &size : isoPaperSizes) {
        create_stylesheet(size.sizeName, size.shortMM, size.longMM, "portrait");
    }
    for (const auto &size : isoPaperSizes) {
        create_stylesheet(size.sizeName, size.longMM, size.shortMM, "landscape");
    }
}

void create_stylesheet(std::string name, int w, int h, std::string orientation) {

    std::cout << "Generating stylesheet for " << name << ": " << w << " x " << h << std::endl;

    std::string css = R"(/******************************************************************************/
/*                                                                            */
/*  PDF LAYOUT CONFIGURATION                                                  */
/*  ------------------------                                                  */
/*  The only setting that should be adjusted in this file is the page margin  */
/*                                                                            */
/*      - Sizes are in mm                                                     */
/*      - If you require different margins then you should make copies of     */
/*        this style sheet, retain them in this folder, and link them as      */
/*        needed.                                                             */
/*                                                                            */
/******************************************************************************/

:root {
    --page-width: )" + std::to_string(w)
                      + R"(;
    --page-height: )" + std::to_string(h)
                      + R"(;
    --page-margin: 8;
}

@page {
    size: var(--page-width)mm var(--page-height)mm;
    margin: 0;
}

/* End of layout configuration */


/******************************************************************************/
/*                                                                            */
/*  REQUIRED                                                                  */
/*  --------                                                                  */
/*  Only modify if you understand the implications                            */
/*                                                                            */
/******************************************************************************/
html, body {
    width: calc(var(--page-width) * 1mm);
    height: calc((var(--page-height) * .999) * 1mm);
}

body {
    margin: 0;
    padding: 0;
    background-color: white;
    font-family: Arial, sans-serif;
}

* {
    box-sizing: border-box;
}

.page {
    height: calc((var(--page-height) * .999) * 1mm);
    width: calc(var(--page-width) * 1mm);
    padding: calc(var(--page-margin) * 1mm);
    border: 0.5mm #D3D3D3 solid;
    border-radius: 0;
    box-shadow: 0 0 5px rgba(0, 0, 0, 0.1);
}

.subpage {
    /* show the margin (form design helper, not printed) */
    border: .5mm red solid;
    /* Height needs to be slightly shorter to prevent creation of a blank page */
    height: calc(((var(--page-height) - (var(--page-margin) * 2)) *.999) * 1mm);
    width: calc((var(--page-width) - (var(--page-margin) * 2)) * 1mm);
    display: flex;
    flex-direction: column;
    overflow: hidden;
}

@media print {
    .page {
        margin: 0;
        border: none;
        box-shadow: none;
        background: white;
    }

    .subpage {
        /* Critical for correct page rendering - prevents extra blank pages */
        border: .5mm white solid;
    }
}
/* End of required section */

)";

    std::string   filename = name + "-" + orientation + ".css";
    std::ofstream file(filename);
    if (file) {
        file << css;
        file.close();
    }
}
