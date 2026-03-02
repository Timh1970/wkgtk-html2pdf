#ifndef ENCODE_IMAGE_H
#define ENCODE_IMAGE_H
#include <string>
#ifndef B64ENC_API
#define B64ENC_API __attribute__((visibility("default")))
#endif
class encode_image {
    private:
        const std::string m_fullPath;

        std::string base64_encode(unsigned char const *bytes_to_encode, size_t in_len);
        std::string process_image();
        std::string image_type(const std::string file);

    public:
        B64ENC_API encode_image();
        B64ENC_API encode_image(const std::string fPath);
        B64ENC_API std::string b64_image();
};

#endif // ENCODE_IMAGE_H
