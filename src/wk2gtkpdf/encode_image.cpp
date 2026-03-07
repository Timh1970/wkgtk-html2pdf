#include "encode_image.h"

#include "iclog.h"

#include <algorithm>
#include <fstream>
#include <vector>
using std::string;

namespace phtml {

    struct encode_image_impl {
            std::string m_fullPath;
            std::string m_resultBuffer; // To hold the final C-string for the caller

            // Graft your private methods here as members of the impl
            std::string base64_encode(const unsigned char *bytes, size_t len);
            std::string image_type(const std::string &file);
            std::string process_image();
    };

    encode_image::encode_image(const char *fPath)
        : m_pimpl(new encode_image_impl()) {
        m_pimpl->m_fullPath = fPath ? fPath : "";
    }

    encode_image::~encode_image() {
        delete m_pimpl;
    }

    const char *encode_image::b64_image() {
        // Build the string inside the impl
        std::string imageName = m_pimpl->m_fullPath.substr(m_pimpl->m_fullPath.find_last_of("/") + 1);

        m_pimpl->m_resultBuffer = "\"data:" + m_pimpl->image_type(imageName) + ";base64,";
        m_pimpl->m_resultBuffer.append(m_pimpl->process_image());
        m_pimpl->m_resultBuffer.append("\"");

        return m_pimpl->m_resultBuffer.c_str();
    }

    const struct b64 {
            /* clang-format off */
                const char m_encodingTable[64] = {
                    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
                    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
                    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
                    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
                    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
                };
            /* clang-format on */

            const int m_modTable[3]   = {0, 2, 1};
            char     *m_decodingTable = nullptr;
    } base64;

    /**
     * @brief encode_image::base64_encode
     * @param bytes_to_encode
     * @param in_len
     * @return
     *
     * Do the encoding
     */
    string encode_image_impl::base64_encode(unsigned char const *bytes_to_encode, size_t in_len) {

        wkJlog << iclog::loglevel::debug << iclog::category::CORE << iclog_FUNCTION
               << "Encoding image"
               << iclog::endl;

        string        ret;
        int           i = 0;
        int           j = 0;
        unsigned char char_array_3[3];
        unsigned char char_array_4[4];

        while (in_len--) {
            char_array_3[i++] = *(bytes_to_encode++);
            if (i == 3) {
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = static_cast<unsigned char>(
                    ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4)
                );
                char_array_4[2] = static_cast<unsigned char>(
                    ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6)
                );
                char_array_4[3] = char_array_3[2] & 0x3f;

                for (i = 0; (i < 4); i++) {
                    ret += base64.m_encodingTable[char_array_4[i]];
                }
                i = 0;
            }
        }

        if (i) {
            for (j = i; j < 3; j++) {
                char_array_3[j] = '\0';
            }

            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = static_cast<unsigned char>(
                ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4)
            );
            char_array_4[2] = static_cast<unsigned char>(
                ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6)
            );
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (j = 0; (j < i + 1); j++) {
                ret += base64.m_encodingTable[char_array_4[j]];
            }

            while ((i++ < 3)) {
                ret += '=';
            }
        }
        return ret;
    }

    /**
     * @brief image_type
     * @param extension
     * @return The correct declaratin for the image type
     */
    string encode_image_impl::image_type(const string &file) {

        string extension = file.substr(file.find_last_of('.') + 1);
        // CONVERT UPPER CASE EXTENSIONS TO LOWER CASE FOR COMPARISON PURPOSES
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

        wkJlog << iclog::loglevel::debug << iclog::category::CORE
               << "Seeking mime type for " << extension
               << iclog::endl;

        struct MIME_TYPE {
                const std::string extension;
                const std::string enctype;
        };

        const std::vector<MIME_TYPE> m_mimeLUT{
            {"bmp",  "image/bmp"               },
            {"gif",  "image/gif"               },
            {"ico",  "image/vnd.microsoft.icon"},
            {"jpeg", "image/jpeg"              },
            {"jpg",  "image/jpeg"              },
            {"png",  "image/png"               },
            {"svg",  "image/svg+xml"           },
            {"tif",  "image/tiff"              },
            {"tiff", "image/tiff"              },
            {"webp", "image/webp"              },
        };

        string e;
        for (const MIME_TYPE &t : m_mimeLUT) {
            if (t.extension.compare(extension) == 0) {
                wkJlog << iclog::loglevel::debug << iclog::category::CORE << iclog_FUNCTION
                       << "Applying mime type " << t.enctype << " to image."
                       << iclog::endl;

                return (t.enctype);
            }
        }

        wkJlog << iclog::loglevel::error << iclog::category::CORE << iclog_FUNCTION
               << "Cannot find mime type for extension " << extension
               << iclog::endl;

        return ("");
    }

    /**
     * @brief encode_image::process_image
     * @return A properly formatted and encoded base64 image string
     */
    string encode_image_impl::process_image() {

        std::ifstream file(m_fullPath);
        if (file.fail()) {
            wkJlog << iclog::loglevel::error << iclog::category::CORE << iclog_FUNCTION
                   << "Cannot read " << m_fullPath
                   << iclog::endl;

            return ("");
        }

        std::string attachment((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        string      encodedImage;
        if (file.is_open()) {
            file.close();
            encodedImage = base64_encode(
                reinterpret_cast<const unsigned char *>(attachment.c_str()),
                attachment.size()
            );
        }

        const unsigned maxLineLen = 72;

        for (unsigned i = maxLineLen; i < encodedImage.size(); i += maxLineLen) {
            encodedImage.insert(i, "\r\n");
            i += 2;
        }

        return (encodedImage);
    }
} // namespace phtml
