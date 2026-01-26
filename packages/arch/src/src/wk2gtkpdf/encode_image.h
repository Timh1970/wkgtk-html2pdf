#ifndef ENCODE_IMAGE_H
#define ENCODE_IMAGE_H
#include <string>
#ifndef B64ENC_API
#define B64ENC_API __attribute__((visibility("default")))
#endif
class encode_image {
    private:
        const std::string m_fullPath;

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

        std::string base64_encode(unsigned char const *bytes_to_encode, size_t in_len);
        std::string process_image();
        std::string image_type(const std::string file);

    public:
        B64ENC_API encode_image(const std::string fPath);
        B64ENC_API std::string b64_image();
};

#endif // ENCODE_IMAGE_H
