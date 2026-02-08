#include "index_pdf.h"

#include "iclog.h"

#include <iostream>
#include <vector>

using std::vector;

index_pdf::index_pdf(std::vector<PDFprinter::anchor> &links, bool debug)
    : m_debug(debug),
      m_links(links) {
}

// language: c++
#include <iostream>
#include <vector>

using namespace PoDoFo;
#ifdef PODOFO_010

static double scale_css_to_pdf(double pdf_page_width_pts, double css_page_width_px) {
    return pdf_page_width_pts / css_page_width_px;
}

void index_pdf::do_annotation(PdfMemDocument &pdfDoc) {
    for (const auto &a : m_links) {
        int src_idx = a.index.pageNo - 1;
        int dst_idx = a.anchor.pageNo - 1;
        if (src_idx < 0 || src_idx >= static_cast<int>(pdfDoc.GetPages().GetCount()) || dst_idx < 0 || dst_idx >= static_cast<int>(pdfDoc.GetPages().GetCount())) {
            jlog << iclog::loglevel::debug << iclog::category::LIB
                 << "Skipping link '" << a.linkName << "': invalid page"
                 << std::endl;
            continue;
        }

        PdfPageCollection &pages = pdfDoc.GetPages();

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
        double cssDstW = a.anchor.page_width;
        double cssDstH = a.anchor.page_height;

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
        double dst_left_pts = a.anchor.xPos * scaleDst;
        double dst_top_pts  = (cssDstH - a.anchor.yPos) * scaleDst;

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

        // DEBUG:
        PdfObject   &pPageObj = dstPage.GetObject();
        PdfReference ref      = pPageObj.GetIndirectReference(); // alternate name sometimes used
        jlog << iclog::loglevel::debug << iclog::category::LIB
             << "dst page ref: " << ref.ObjectNumber() << " gen: " << ref.GenerationNumber() << "\n"
             << "Total objects: " << pdfDoc.GetObjects().GetSize()
             << std::endl;
    }
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
