#include "index_pdf.h"

#include "iclog.h"

#include <iostream>
#include <regex>
#include <vector>

using std::vector;

index_pdf::index_pdf(std::vector<PDFprinter::anchor> &links, const int tocPage, bool debug)
    : m_tocPage(tocPage),
      m_debug(debug),
      m_links(links) {
}

// language: c++
#include <iostream>
#include <vector>

using namespace PoDoFo;
#define PODOFO_010 // !!!MAKE SURE THIS IS REMOVED!!!
#ifdef PODOFO_010

static double scale_css_to_pdf(double pdf_page_width_pts, double css_page_width_px) {
    return pdf_page_width_pts / css_page_width_px;
}

/**
 * @brief index_pdf::parseNumbering
 * @param title
 * @return
 *
 * Parse numbering from the title
 */
std::vector<int> index_pdf::parseNumbering(const std::string &title) {
    std::vector<int> levels;
    std::regex       numberPattern(R"(^(\d+(?:\.\d+)*))");
    std::smatch      match;

    if (std::regex_search(title, match, numberPattern)) {
        std::string       numbering = match.str();
        std::stringstream ss(numbering);
        std::string       token;

        while (std::getline(ss, token, '.')) {
            levels.push_back(std::stoi(token));
        }
    }

    return levels;
}

// std::vector<int> index_pdf::parseNumbering(const std::string &title) {
//     std::vector<int> levels;
//     std::regex       numberPattern(R"(^(\d+(?:\.\d*)*))");
//     std::smatch      match;

//     if (std::regex_search(title, match, numberPattern)) {
//         std::string       numbering = match.str();
//         std::stringstream ss(numbering);
//         std::string       token;

//         while (std::getline(ss, token, '.')) {
//             if (!token.empty()) { // Skip empty tokens (e.g., after trailing dot)
//                 try {
//                     levels.push_back(std::stoi(token));
//                 } catch (...) {
//                     return {}; // Invalid number
//                 }
//             }
//         }
//     }
//     return levels;
// }

// Build nested outline structure
void index_pdf::buildNestedOutlines(PdfOutlines &outlines, std::vector<OutlineData> &outlineData, std::shared_ptr<PdfDestination> toc) {
    if (outlineData.empty())
        return;

    // Sort by numbering hierarchy
    std::sort(outlineData.begin(), outlineData.end(), [](OutlineData &a, OutlineData &b) {
        size_t minSize = std::min(a.levels.size(), b.levels.size());
        for (size_t i = 0; i < minSize; ++i) {
            if (a.levels[i] != b.levels[i]) {
                return a.levels[i] < b.levels[i];
            }
        }
        return a.levels.size() < b.levels.size();
    });

    PdfOutlineItem *root = outlines.CreateRoot(PdfString("Contents"));
    if (toc) {
        root->SetDestination(toc);
    }

    // Map to track the last item at each depth level
    std::map<int, PdfOutlineItem *> lastItemAtLevel{
        {0, root}
    };

    for (const auto &data : outlineData) {
        if (data.levels.empty())
            continue;

        if (!data.dest || !data.dest->GetPage()) {
            continue;
        }

        int             depth  = data.levels.size();
        PdfOutlineItem *parent = nullptr;

        // Find the appropriate parent
        // For "1.1.1", parent should be the last "1.1" item
        if (depth == 1) {
            parent = root;
        } else {
            // Look for parent at depth-1
            parent = lastItemAtLevel[depth - 1];
            if (!parent)
                parent = root;
        }

        PdfOutlineItem *newItem = nullptr;

        // Check if we need to create a child or sibling
        if (lastItemAtLevel.find(depth) == lastItemAtLevel.end() || lastItemAtLevel[depth] == nullptr) {
            // First item at this depth - create as child
            newItem = parent->CreateChild(PdfString(data.title.c_str()), data.dest);
        } else {
            // Compare with previous item at same depth
            // If parent is the same, create sibling; otherwise create child
            newItem = lastItemAtLevel[depth]->CreateNext(PdfString(data.title.c_str()), data.dest);
        }

        // Update tracking
        lastItemAtLevel[depth] = newItem;

        // Clear deeper levels (we've moved to a new branch)
        auto it = lastItemAtLevel.upper_bound(depth);
        lastItemAtLevel.erase(it, lastItemAtLevel.end());
    }
}

void index_pdf::do_annotation(PdfMemDocument &pdfDoc) {

    /*  --- BEGIN EXPERIMENTAL (Create side index) --- */

    // Get or create the outlines structure
    PdfOutlines &outlines = pdfDoc.GetOrCreateOutlines();

    // PdfOutlines     outlines = PdfOutlines(pdfDoc);
    // PdfOutlineItem *root     = outlines.CreateRoot(PdfString("Contents"));
    // PdfOutlineItem *lastItem = nullptr;
    // TABLE OF CONTENTS
    PdfPageCollection &pages = pdfDoc.GetPages();

    /**
     * @brief tocPage
     *
     * This is the primary page of the index (the one that appears
     * when you click "contents" in the bookmarks bar).
     *
     * PDF pages coordinates begin from bottom left which is why we
     * are setting Height from the size of the page.
     */
    std::shared_ptr<PdfDestination> toc = nullptr;
    if (m_tocPage != UNSET) {
        PdfPage &tocPage        = pages.GetPageAt(m_tocPage);
        Rect     pdfSrcPageRect = tocPage.GetMediaBox();
        toc                     = std::make_shared<PdfDestination>(tocPage, 0.0, pdfSrcPageRect.Height, 0.0);
    }
    /*  --- END EXPERIMENTAL (Create side index) --- */

    for (const auto &a : m_links) {
        int src_idx = a.index.pageNo - 1;
        int dst_idx = a.target.pageNo - 1;
        if (src_idx < 0 || src_idx >= static_cast<int>(pdfDoc.GetPages().GetCount()) || dst_idx < 0 || dst_idx >= static_cast<int>(pdfDoc.GetPages().GetCount())) {
            jlog << iclog::loglevel::debug << iclog::category::LIB
                 << "Skipping link '" << a.linkName << "': invalid page"
                 << std::endl;
            continue;
        }

        PdfPage &srcPage = pages.GetPageAt(src_idx);
        PdfPage &dstPage = pages.GetPageAt(dst_idx);

        // PDF page sizes (points)
        Rect   pdfSrcPageRect = srcPage.GetMediaBox();
        double pdfSrcW        = pdfSrcPageRect.Width;

        Rect   pdfDstPageRect = dstPage.GetMediaBox();
        double pdfDstW        = pdfDstPageRect.Height;

        // CSS page size (pixels) from JS
        double cssSrcW = a.index.page_width;
        double cssSrcH = a.index.page_height;
        double cssDstW = a.target.page_width;
        double cssDstH = a.target.page_height;

        if (cssSrcW <= 0 || cssSrcH <= 0 || cssDstW <= 0 || cssDstH <= 0) {
            jlog << iclog::loglevel::debug << iclog::category::LIB
                 << "Skipping link '" << a.linkName << "': missing page sizes"
                 << std::endl;
            continue;
        }

        double scaleSrc = scale_css_to_pdf(pdfSrcW, cssSrcW);
        double scaleDst = scale_css_to_pdf(pdfDstW, cssDstW);

        // convert source rect: CSS top-left -> PDF bottom-left
        double src_left_pts   = a.index.xPos * scaleSrc;
        // --- BEGIN EXPERIMENTAL --- //
        // ORIGINAL
        // double src_top_pts    = (cssSrcH - a.index.yPos) * scaleSrc;
        // NEW
        double src_top_pts    = (cssSrcH - (a.index.yPos - (a.index.page_height * 0.004))) * scaleSrc;
        // --- END EXPERIMENTAL --- //
        double src_w_pts      = a.index.w * scaleSrc;
        double src_h_pts      = a.index.h * scaleSrc;
        double src_bottom_pts = src_top_pts - src_h_pts;

        Rect annotRect(src_left_pts, src_bottom_pts, src_w_pts, src_h_pts);

        // convert destination coords: left/top in PDF points
        double dst_left_pts = a.target.xPos * scaleDst;
        double dst_top_pts  = (cssDstH - a.target.yPos) * scaleDst;

        // SORUCE
        // Create a typed annotation via the page's annotation collection
        // This uses the templated factory in PdfAnnotationCollection
        PdfAnnotationLink &link = srcPage.GetAnnotations().CreateAnnot<PdfAnnotationLink>(annotRect);

        // DESTINATION
        // PdfDestination( const PdfPage* pPage, double dLeft, double dTop, double dZoom );
        // dLeft, dTop are PDF points; dZoom: 0.0 = leave viewer default, 1.0 = 100%
        PdfDestination                  destination(dstPage, dst_left_pts, dst_top_pts, 0.0);
        std::shared_ptr<PdfDestination> dest = std::make_shared<PdfDestination>(dstPage, dst_left_pts, dst_top_pts, 0.0);

        // ATTACH SOURCE TO DESTINATION
        link.SetDestination(dest);

        // Hide border (same API as older versions)
        if (m_debug)
            link.SetBorderStyle(1.0, 1.0, 1.0);
        else
            link.SetBorderStyle(0.0, 0.0, 0.0);

        /*  --- BEGIN EXPERIMENTAL (Create side index) --- */
        // if (!lastItem) {
        //     lastItem = root->CreateChild(PdfString(a.anchor.title.c_str()), dest);
        // } else {
        //     lastItem = lastItem->CreateNext(PdfString(a.anchor.title.c_str()), dest);
        // }
        // TRY NESTING INSTAED
        // Collect outline data

        vector<int> buf = parseNumbering(a.target.title);
        if (!buf.empty() && dest && dest->GetPage()) {
            OutlineData od;
            od.title   = a.target.title;
            od.dest    = dest;
            od.levels  = buf;
            // AVOID DUPLICATES
            bool found = false;
            for (OutlineData &it : outlineData) {
                if (it.title.compare(od.title) == 0)
                    found = true;
            }
            if (!found) {
                outlineData.push_back(od);
            }

        } else {
            jlog << iclog::loglevel::notice << iclog::category::LIB
                 << "Invalid outline: missing destination or page."
                 << std::endl;
        }
        /*  --- END EXPERIMENTAL (Create side index) --- */

        // DEBUG:
        PdfObject   &pPageObj = dstPage.GetObject();
        PdfReference ref      = pPageObj.GetIndirectReference(); // alternate name sometimes used
        jlog << iclog::loglevel::debug << iclog::category::LIB
             << "dst page ref: " << ref.ObjectNumber() << " gen: " << ref.GenerationNumber() << "\n"
             << "Total objects: " << pdfDoc.GetObjects().GetSize()
             << std::endl;
    }
    /*  --- BEGIN EXPERIMENTAL (Create side index) --- */
    // Second pass: build nested outline structure

    buildNestedOutlines(outlines, outlineData, toc);
    /*  --- END EXPERIMENTAL (Create side index) --- */
}

static void debug_check_annotations_and_streams(PdfMemDocument &doc) {
    PdfPageCollection &pages     = doc.GetPages();
    unsigned           pageCount = pages.GetCount();
    jlog << iclog::loglevel::debug << iclog::category::LIB
         << "Page count: " << pageCount
         << std::endl;

    for (unsigned p = 0; p < pageCount; ++p) {
        PdfPage &page = pages.GetPageAt(p);
        jlog << iclog::loglevel::debug << iclog::category::LIB
             << "Checking page " << (p + 1)
             << std::endl;

        auto    &annots     = page.GetAnnotations();
        unsigned annotCount = annots.GetCount();
        for (unsigned ai = 0; ai < annotCount; ++ai) {
            PdfAnnotation &annot = annots.GetAnnotAt(ai);
            PdfObject     &obj   = annot.GetObject();

            auto *stream = obj.GetStream();
            if (stream) {
                long len = -1;
                try {
                    len = static_cast<long>(stream->GetLength());
                } catch (...) {
                }
                jlog << iclog::loglevel::debug << iclog::category::LIB
                     << "Stream length: " << len
                     << std::endl;
                if (len == 0) {

                    jlog << iclog::loglevel::error << iclog::category::LIB
                         << "** EMPTY stream on annotation object **"
                         << std::endl;
                }
            } else {
                jlog << iclog::loglevel::debug << iclog::category::LIB
                     << "no stream on this annotation object."
                     << std::endl;
            }

            // Check for Type /Metadata (unlikely on annotation objects)
            try {
                if (obj.GetDictionary().HasKey(PdfName("Type"))) {
                    PdfObject *typeObj = obj.GetDictionary().GetKey(PdfName("Type"));
                    if (typeObj && typeObj->GetName() == "Metadata") {
                        jlog << iclog::loglevel::debug << iclog::category::LIB
                             << "-> annotation object typed as Metadata"
                             << std::endl;
                    }
                }
            } catch (...) {
            }
        }
    }
}

void index_pdf::create_anchors(std::string sourcePath, std::string destPath) {
    PdfMemDocument doc;
    doc.Load(sourcePath);
    do_annotation(doc);
    debug_check_annotations_and_streams(doc);

    jlog << iclog::loglevel::debug << iclog::category::LIB
         << "Saving page"
         << std::endl;
    doc.Save(destPath, PoDoFo::PdfSaveOptions::Clean | PoDoFo::PdfSaveOptions::NoMetadataUpdate);
    jlog << iclog::loglevel::debug << iclog::category::LIB
         << destPath << " written"
         << std::endl;
}

#else

static double scale_css_to_pdf(double pdf_page_width_pts, double css_page_width_px) {
    return pdf_page_width_pts / css_page_width_px;
}

void index_pdf::do_annotation(PdfMemDocument &pdfDoc) {
    for (const auto &a : m_links) {
        int src_idx = a.index.pageNo - 1;
        int dst_idx = a.anchor.pageNo - 1;
        if (src_idx < 0 || src_idx >= static_cast<int>(pdfDoc.GetPageCount()) || dst_idx < 0 || dst_idx >= static_cast<int>(pdfDoc.GetPageCount())) {
            std::cerr << "Skipping link '" << a.linkName << "': invalid page\n";
            continue;
        }

        PdfPage *srcPage = pdfDoc.GetPage(src_idx);
        PdfPage *dstPage = pdfDoc.GetPage(dst_idx);

        // PDF page sizes (points)
        PdfRect pdfSrcPageRect = srcPage->GetPageSize();
        double  pdfSrcW        = pdfSrcPageRect.GetWidth();

        PdfRect pdfDstPageRect = dstPage->GetPageSize();
        double  pdfDstW        = pdfDstPageRect.GetWidth();

        // CSS page size (pixels) from your JS
        double cssSrcW = a.index.page_width;
        double cssSrcH = a.index.page_height;
        double cssDstW = a.anchor.page_width;
        double cssDstH = a.anchor.page_height;

        if (cssSrcW <= 0 || cssSrcH <= 0 || cssDstW <= 0 || cssDstH <= 0) {
            std::cerr << "Skipping link '" << a.linkName << "': missing page sizes\n";
            continue;
        }

        double scaleSrc = scale_css_to_pdf(pdfSrcW, cssSrcW);
        double scaleDst = scale_css_to_pdf(pdfDstW, cssDstW);

        // convert source rect: CSS top-left -> PDF bottom-left
        double src_left_pts   = a.index.xPos * scaleSrc;
        double src_top_pts    = (cssSrcH - a.index.yPos) * scaleSrc;
        double src_w_pts      = a.index.w * scaleSrc;
        double src_h_pts      = a.index.h * scaleSrc;
        double src_bottom_pts = src_top_pts - src_h_pts;

        PdfRect annotRect(src_left_pts, src_bottom_pts, src_w_pts, src_h_pts);

        // convert destination coords: left/top in PDF points
        double dst_left_pts = a.anchor.xPos * scaleDst;
        double dst_top_pts  = (cssDstH - a.anchor.yPos) * scaleDst;

        // PdfDestination( const PdfPage* pPage, double dLeft, double dTop, double dZoom );
        // dLeft, dTop are PDF points; dZoom: 0.0 = leave viewer default, 1.0 = 100%
        PdfDestination destination(dstPage, dst_left_pts, dst_top_pts, 0.0);

        // Create annotation on source page and attach destination
        PdfAnnotation *pAnnot = srcPage->CreateAnnotation(ePdfAnnotation_Link, annotRect);
        if (!pAnnot) {
            std::cerr << "Failed to create annotation for '" << a.linkName << "'\n";
            continue;
        }

        // Attach destination to the annotation
        pAnnot->SetDestination(destination);

        // Make annotation invisible for production; use >0 for debugging
        pAnnot->SetBorderStyle(0.0, 0.0, 0.0);

        // If CreateAnnotation already adds the annotation to the page, nothing more to do.
        // Otherwise call srcPage->AddAnnotation(pAnnot); depending on your PoDoFo version.
    }
}

void index_pdf::create_anchors(std::string sourcePath, std::string destPath) {
    PdfMemDocument doc;
    doc.Load(sourcePath.c_str());
    do_annotation(doc);

    jlog << iclog::loglevel::debug << iclog::category::LIB
         << "Saving page"
         << std::endl;

    doc.Write(destPath.c_str());
    jlog << iclog::loglevel::debug << iclog::category::LIB
         << destPath << " written"
         << std::endl;
}

#endif
