#include "index_pdf.h"

#include "iclog.h"

#include <podofo/podofo.h>
#include <regex>
#include <string>
#include <vector>

using std::vector;
using namespace PoDoFo;

struct index_pdf_impl {
        struct OutlineData {
                std::string      title;
                std::vector<int> levels;
#ifdef PODOFO_010
                std::shared_ptr<PoDoFo::PdfDestination> dest;
#else
                PoDoFo::PdfDestination *dest;
#endif
        };

        int                      m_tocPage;
        bool                     m_debug;
        std::vector<PDF_Anchor>  m_links; // Internal copy of the data
        std::vector<OutlineData> m_outlineData;

        std::vector<int> parseNumbering(const std::string &title);
        void             do_annotation(PoDoFo::PdfMemDocument &pdfDoc);

#ifdef PODOFO_010
        void buildNestedOutlines(PoDoFo::PdfOutlines &outlines, std::vector<OutlineData> &outlineData, std::shared_ptr<PoDoFo::PdfDestination> toc);
#else
        void buildNestedOutlines(PoDoFo::PdfOutlines *pOutlines, std::vector<OutlineData> &outlineData, PoDoFo::PdfDestination *pTocDest);
#endif
};

/////////////////////////////////////////////////////////////////////////////////////

index_pdf::index_pdf(const PDF_Anchor *links, size_t count, int tocPage, bool debug)
    : m_pimpl(new index_pdf_impl()) {

    m_pimpl->m_tocPage = tocPage;
    m_pimpl->m_debug   = debug;

    // Copy the raw ABI data into our internal vector
    if (links && count > 0) {
        m_pimpl->m_links.assign(links, links + count);
    }
}

index_pdf::~index_pdf() {
    delete m_pimpl;
}

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
std::vector<int> index_pdf_impl::parseNumbering(const std::string &title) {
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

#ifdef PODOFO_010

// Build nested outline structure
void index_pdf_impl::buildNestedOutlines(PdfOutlines &outlines, std::vector<OutlineData> &outlineData, std::shared_ptr<PdfDestination> toc) {
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

void index_pdf_impl::do_annotation(PdfMemDocument &pdfDoc) {

    // Get or create the outlines structure
    PdfOutlines &outlines = pdfDoc.GetOrCreateOutlines();

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
    if (m_tocPage != index_pdf::UNSET) {
        PdfPage &tocPage        = pages.GetPageAt(m_tocPage);
        Rect     pdfSrcPageRect = tocPage.GetMediaBox();
        toc                     = std::make_shared<PdfDestination>(tocPage, 0.0, pdfSrcPageRect.Height, 0.0);
    }
    /*  --- END EXPERIMENTAL (Create side index) --- */

    for (const auto &a : m_links) {
        int src_idx = a.index.pageNo - 1;
        int dst_idx = a.target.pageNo - 1;
        if (src_idx < 0 || src_idx >= static_cast<int>(pdfDoc.GetPages().GetCount()) || dst_idx < 0 || dst_idx >= static_cast<int>(pdfDoc.GetPages().GetCount())) {
            wkJlog << iclog::loglevel::debug << iclog::category::LIB
                   << "Skipping link '" << a.linkName << "': invalid page"
                   << iclog::endl;
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
            wkJlog << iclog::loglevel::debug << iclog::category::LIB
                   << "Skipping link '" << a.linkName << "': missing page sizes"
                   << iclog::endl;
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

        vector<int> buf = parseNumbering(a.target.title);
        if (!buf.empty() && dest && dest->GetPage()) {
            OutlineData od;
            od.title   = a.target.title;
            od.dest    = dest;
            od.levels  = buf;
            // AVOID DUPLICATES
            bool found = false;
            for (OutlineData &it : m_outlineData) {
                if (it.title.compare(od.title) == 0)
                    found = true;
            }
            if (!found) {
                m_outlineData.push_back(od);
            }

        } else {
            wkJlog << iclog::loglevel::notice << iclog::category::LIB
                   << "Invalid outline: missing destination or page."
                   << iclog::endl;
        }

        // DEBUG:
        PdfObject   &pPageObj = dstPage.GetObject();
        PdfReference ref      = pPageObj.GetIndirectReference(); // alternate name sometimes used
        wkJlog << iclog::loglevel::debug << iclog::category::LIB
               << "dst page ref: " << ref.ObjectNumber() << " gen: " << ref.GenerationNumber() << "\n"
               << "Total objects: " << pdfDoc.GetObjects().GetSize()
               << iclog::endl;
    }

    buildNestedOutlines(outlines, m_outlineData, toc);
}

static void debug_check_annotations_and_streams(PdfMemDocument &doc) {
    PdfPageCollection &pages     = doc.GetPages();
    unsigned           pageCount = pages.GetCount();
    wkJlog << iclog::loglevel::debug << iclog::category::LIB
           << "Page count: " << pageCount
           << iclog::endl;

    for (unsigned p = 0; p < pageCount; ++p) {
        PdfPage &page = pages.GetPageAt(p);
        wkJlog << iclog::loglevel::debug << iclog::category::LIB
               << "Checking page " << (p + 1)
               << iclog::endl;

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
                wkJlog << iclog::loglevel::debug << iclog::category::LIB
                       << "Stream length: " << len
                       << iclog::endl;
                if (len == 0) {

                    wkJlog << iclog::loglevel::error << iclog::category::LIB
                           << "** EMPTY stream on annotation object **"
                           << iclog::endl;
                }
            } else {
                wkJlog << iclog::loglevel::debug << iclog::category::LIB
                       << "no stream on this annotation object."
                       << iclog::endl;
            }

            // Check for Type /Metadata (unlikely on annotation objects)
            try {
                if (obj.GetDictionary().HasKey(PdfName("Type"))) {
                    PdfObject *typeObj = obj.GetDictionary().GetKey(PdfName("Type"));
                    if (typeObj && typeObj->GetName() == "Metadata") {
                        wkJlog << iclog::loglevel::debug << iclog::category::LIB
                               << "-> annotation object typed as Metadata"
                               << iclog::endl;
                    }
                }
            } catch (...) {
            }
        }
    }
}

void index_pdf::create_anchors(const char *sourcePath, const char *destPath) {
    PdfMemDocument doc;
    doc.Load(sourcePath);
    m_pimpl->do_annotation(doc);
    debug_check_annotations_and_streams(doc);

    wkJlog << iclog::loglevel::debug << iclog::category::LIB
           << "Saving page"
           << iclog::endl;
    doc.Save(destPath, PoDoFo::PdfSaveOptions::Clean | PoDoFo::PdfSaveOptions::NoMetadataUpdate);
    wkJlog << iclog::loglevel::debug << iclog::category::LIB
           << destPath << " written"
           << iclog::endl;
}

#else

void index_pdf::create_anchors(const char *sourcePath, const char *destPath) { // 0.9.x: Loading is done via the constructor or Load()
    PdfMemDocument doc;
    try {
        doc.Load(sourcePath);

        // Call our 0.9.x backported do_annotation
        m_pimpl->do_annotation(doc);

        wkJlog << iclog::loglevel::debug << iclog::category::LIB
               << "Saving page to " << destPath
               << iclog::endl;

        // 0.9.x: Write() is the equivalent of 0.10's Save()
        // Note: 0.9.x doesn't have the same 'PdfSaveOptions' enum;
        // it uses different flags or defaults to 'Clean' via Write().
        doc.Write(destPath);

        wkJlog << iclog::loglevel::debug << iclog::category::LIB
               << destPath << " written"
               << iclog::endl;

    } catch (const PdfError &e) {
        wkJlog << iclog::loglevel::error << iclog::category::LIB
               << "PoDoFo Error in create_anchors: " << e.what()
               << iclog::endl;
    }
}

void index_pdf_impl::buildNestedOutlines(PdfOutlines *pOutlines, std::vector<OutlineData> &outlineData, PdfDestination *pTocDest) {
    if (outlineData.empty() || !pOutlines)
        return;

    // Sorting logic remains identical to your 0.10 code
    std::sort(outlineData.begin(), outlineData.end(), [](OutlineData &a, OutlineData &b) {
        size_t minSize = std::min(a.levels.size(), b.levels.size());
        for (size_t i = 0; i < minSize; ++i) {
            if (a.levels[i] != b.levels[i])
                return a.levels[i] < b.levels[i];
        }
        return a.levels.size() < b.levels.size();
    });

    // In 0.9.x, CreateRoot returns the first item.
    // Usually, we create a top-level "Contents" item.
    PdfOutlineItem *root = pOutlines->CreateRoot(PdfString("Contents"));
    if (pTocDest) {
        root->SetDestination(*pTocDest);
    }

    // Tracking map remains the same
    std::map<int, PdfOutlineItem *> lastItemAtLevel;
    lastItemAtLevel[0] = root;

    for (const auto &data : outlineData) {
        if (data.levels.empty())
            continue;

        int             depth  = data.levels.size();
        PdfOutlineItem *parent = (depth == 1) ? root : lastItemAtLevel[depth - 1];
        if (!parent)
            parent = root;

        PdfOutlineItem *newItem = nullptr;

        // 0.9.x uses CreateChild and CreateNext with slightly different signatures
        // Note: data.dest in 0.9.x is likely a PdfDestination object, not a pointer
        if (lastItemAtLevel.find(depth) == lastItemAtLevel.end()) {
            newItem = parent->CreateChild(PdfString(data.title.c_str()), *(data.dest));
        } else {
            newItem = lastItemAtLevel[depth]->CreateNext(PdfString(data.title.c_str()), *(data.dest));
        }

        lastItemAtLevel[depth] = newItem;

        // Branch cleanup logic remains the same
        auto it = lastItemAtLevel.upper_bound(depth);
        lastItemAtLevel.erase(it, lastItemAtLevel.end());
    }
}

///////////////////////////

#ifndef PODOFO_010
void index_pdf_impl::do_annotation(PdfMemDocument &pdfDoc) {
    // 0.9.x: Outlines are accessed directly from the document
    PdfOutlines *pOutlines = pdfDoc.GetOutlines();

    // 0.9.x: No PageCollection; pages are accessed by index from the document
    int pageCount = pdfDoc.GetPageCount();

    PdfDestination *pTocDest = nullptr;
    if (m_tocPage != index_pdf::UNSET && m_tocPage < pageCount) {
        PdfPage *pTocPage = pdfDoc.GetPage(m_tocPage);
        PdfRect  pageRect = pTocPage->GetMediaBox();
        // Destination(pPage, left, top, zoom)
        pTocDest          = new PdfDestination(pTocPage, 0.0, pageRect.GetHeight(), 0.0);
    }

    for (const auto &a : m_links) {
        int src_idx = a.index.pageNo - 1;
        int dst_idx = a.target.pageNo - 1;

        if (src_idx < 0 || src_idx >= pageCount || dst_idx < 0 || dst_idx >= pageCount)
            continue;

        PdfPage *pSrcPage = pdfDoc.GetPage(src_idx);
        PdfPage *pDstPage = pdfDoc.GetPage(dst_idx);

        // 0.9.x uses PdfRect instead of Rect
        PdfRect srcMedia = pSrcPage->GetMediaBox();
        PdfRect dstMedia = pDstPage->GetMediaBox();

        double scaleSrc = scale_css_to_pdf(srcMedia.GetWidth(), a.index.page_width);
        double scaleDst = scale_css_to_pdf(dstMedia.GetHeight(), a.target.page_height);

        // Y-axis logic (Bottom-up in PDF)
        double  src_bottom_pts = (a.index.page_height - (a.index.yPos + a.index.h)) * scaleSrc;
        PdfRect annotRect(a.index.xPos * scaleSrc, src_bottom_pts, a.index.w * scaleSrc, a.index.h * scaleSrc);

        // Create Annotation: In 0.9.x, you specify the subtype via enum
        PdfAnnotation *pAnnot = pSrcPage->CreateAnnotation(ePdfAnnotation_Link, annotRect);

        double         dst_top_pts = (a.target.page_height - a.target.yPos) * scaleDst;
        PdfDestination dest(pDstPage, a.target.xPos * scaleDst, dst_top_pts, 0.0);

        // Attach Destination
        pAnnot->SetDestination(dest);

        // Border: 0.9.x uses SetFlags or specific border methods
        if (!m_debug) {
            // Hide border via an empty border array
            pAnnot->GetObject()->GetDictionary().AddKey(PdfName("Border"), PdfArray());
        }

        // Outline Tracking
        std::vector<int> buf = parseNumbering(a.target.title);
        if (!buf.empty()) {
            OutlineData od;
            od.title  = a.target.title;
            // In 0.9.x, we usually store the destination as a member, not a shared_ptr
            od.dest   = new PdfDestination(dest);
            od.levels = buf;

            bool found = false;
            for (auto &it : m_outlineData) {
                if (it.title == od.title)
                    found = true;
            }
            if (!found)
                m_outlineData.push_back(od);
        }
    }

    buildNestedOutlines(pOutlines, m_outlineData, pTocDest);

    // Cleanup if you used 'new' for local storage
    if (pTocDest)
        delete pTocDest;
}
#endif
#endif
