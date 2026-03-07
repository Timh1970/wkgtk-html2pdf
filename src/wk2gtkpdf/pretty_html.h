#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef _GUARD_PRETTY_HTML_H
#define _GUARD_PRETTY_HTML_H

#ifndef PHTML_API
#define PHTML_API __attribute__((visibility("default")))
#endif

#ifndef PHTML_API
#define PHTML_API __attribute__((visibility("default")))
#endif

namespace phtml {
    // Forward decls.
    class PHTML_API html_tree;
    void PHTML_API  process_nodes(html_tree *primaryNode);
    struct html_tree_impl;
    html_tree *find_first_open_sibling(html_tree *parent);

    class PHTML_API html_tree {
        public:
            // Use raw C-strings to cross the binary boundary
            html_tree(const char *htmlTag, bool includeHeader = false);
            ~html_tree();

            // ABI-safe handles for the tree structure
            html_tree *new_node(const char *htmlTag);
            html_tree *new_node_f(const char *format, ...) __attribute__((format(printf, 2, 3)));

            // Pass content as raw pointers
            void set_node_content(const char *content, bool lineBreak = false, bool escapeSpecialChars = true);
            void set_node_content_f(const char *format, ...) __attribute__((format(printf, 2, 3)));

            // Access the final master string as a C-string
            const char *get_html() const;

            friend void process_nodes(html_tree *primaryNode);

        private:
            // Internal constructor so children can share the root implementation
            html_tree(const char *htmlTag, html_tree *parent_handle);
            // The only member: The Pimpl pointer
            html_tree_impl   *m_pimpl;
            friend html_tree *find_first_open_sibling(html_tree *parent);
    };

} // namespace phtml

#endif
