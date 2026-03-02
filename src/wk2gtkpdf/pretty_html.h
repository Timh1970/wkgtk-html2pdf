#ifndef _GUARD_PRETTY_HTML_H
#define _GUARD_PRETTY_HTML_H

#include <string>
#include <vector>

#ifndef PHTML_API
#define PHTML_API __attribute__((visibility("default")))
#endif

typedef std::string WEBPAGE; /**<  HTML Page */

/**
 * @brief The html_tree class
 *
 * Stateful HTML Generator with Automatic Escaping and Tree-based Validation.
 */
class html_tree {
    public:
        enum class status {
            UNPROCESSED,
            OPEN,
            CLOSED
        };

    private:
        struct TAG_CONTENT {
                std::string text;
                bool        lineBreak;
        };

        // DECLARATIONS
        const std::string        m_htmlTag;
        std::string             &m_htmlPage;
        std::vector<TAG_CONTENT> m_tagContent;
        status                   m_nodestatus;

        bool        is_special_tag(const std::string &tag, const std::vector<std::string> &table);
        std::string handle_special_characters(std::string text);

    public:
        std::vector<html_tree *> childNodes;
        html_tree               *previousBranch;

        PHTML_API html_tree(
            std::string  htmlTag,
            std::string &htmlPage,
            html_tree   *previousBranch = nullptr
        );
        PHTML_API ~html_tree();

        PHTML_API html_tree *new_node(std::string htmlTag);
        bool                 is_leaf_node();
        void                 open_node(unsigned tabs);
        void                 close_node(unsigned tabs);
        status               closed();
        PHTML_API void       set_node_content(std::string content, bool lineBreak = true, bool escapeSpecialChars = false);
};

namespace pretty_html {
    html_tree     *find_first_open_sibling(html_tree *parent);
    PHTML_API void process_nodes(html_tree *primaryNode);
} // namespace pretty_html

#endif
