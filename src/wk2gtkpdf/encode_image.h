#ifndef ENCODE_IMAGE_H
#define ENCODE_IMAGE_H

#ifndef B64ENC_API
#define B64ENC_API __attribute__((visibility("default")))
#endif
namespace phtml {
    struct encode_image_impl;

    class B64ENC_API encode_image {
        public:
            // Use raw C-strings to keep the ABI "Virgin"
            encode_image(const char *fPath);
            ~encode_image();

            // Returns the full "data:image/..." string as a C-string
            const char *b64_image();

        private:
            encode_image_impl *m_pimpl;
    };

} // namespace phtml

#endif // ENCODE_IMAGE_H
