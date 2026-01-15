#include "encode_image.h"

#include "iclog.h"

#include <algorithm>
#include <fstream>
#include <vector>
using std::string;

encode_image::encode_image(const std::string fPath)
    : m_fullPath(fPath) {
}

/**
 * @brief encode_image::base64_encode
 * @param bytes_to_encode
 * @param in_len
 * @return
 *
 * Do the encoding
 */
string encode_image::base64_encode(unsigned char const *bytes_to_encode, size_t in_len) {

    jlog << iclog::loglevel::debug << iclog::category::CORE << iclog_FUNCTION
         << "Encoding image"
         << std::endl;

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
string encode_image::image_type(const string file) {

    string extension = file.substr(file.find_last_of('.') + 1);
    // CONVERT UPPER CASE EXTENSIONS TO LOWER CASE FOR COMPARISON PURPOSES
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    jlog << iclog::loglevel::debug << iclog::category::CORE
         << "Seeking mime type for " << extension
         << std::endl;

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
            jlog << iclog::loglevel::debug << iclog::category::CORE << iclog_FUNCTION
                 << "Applying mime type " << t.enctype << " to image."
                 << std::endl;

            return (t.enctype);
        }
    }

    jlog << iclog::loglevel::error << iclog::category::CORE << iclog_FUNCTION
         << "Cannot find mime type for extension " << extension
         << std::endl;

    return ("");
}

/**
 * @brief encode_image::process_image
 * @return A properly formatted and encoded base64 image string
 */
string encode_image::process_image() {

    std::ifstream file(m_fullPath);
    if (file.fail()) {
        jlog << iclog::loglevel::error << iclog::category::CORE << iclog_FUNCTION
             << "Cannot read " << m_fullPath
             << std::endl;

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

/**
 * @brief encode_image::b64_image
 * @return A base64 encoded image with the correct mime declaration
 *
 * When embedding an image in html you use
 *
 * "<img src=" + b64_image() + ">
 */
string encode_image::b64_image() {

    string imageName(m_fullPath.substr(
        m_fullPath.find_last_of("/") + 1
    ));

    // Path not supplied, assumme working directory.
    if (imageName.empty()) {
        jlog << iclog::loglevel::debug << iclog::category::CORE << iclog_FUNCTION
             << "No path found, using current folder." << m_fullPath
             << std::endl;
        imageName = m_fullPath;
    }

    string encImage("\"data:" + image_type(imageName) + ";base64,");
    encImage.append(process_image());
    encImage.append("\"");

    return (encImage);
}
