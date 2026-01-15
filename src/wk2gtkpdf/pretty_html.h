#ifndef _GUARD_PRETTY_HTML_H
#define _GUARD_PRETTY_HTML_H

#include <string>
#include <vector>

#ifndef PHTML_API
#define PHTML_API __attribute__((visibility("default")))
#endif

/**
 * @brief PHTML_DEBUG
 *
 * Enabling debug on pretty_html can clog up the logs
 * so we have turned it off by default; to enable set
 * to true.
 */
bool PHTML_DEBUG = false;

typedef std::string WEBPAGE; /**<  HTML Page */

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

        // Void Tags (should not be closed)
        const std::vector<std::string> voidTags{
            "area",
            "base",
            "br",
            "col",
            "command",
            "embed",
            "hr",
            "img",
            "input",
            "keygen",
            "link",
            "meta",
            "param",
            "source",
            "track",
            "wbr"
        };

        // Blocks (That should not have new lines)
        // This is a work in progress
        const std::vector<std::string> blockTags{
            "textarea",
            "p",
            "h1",
            "h2",
            "h3",
            "h4",
            "h5",
            "h6",
            "title",
            "a",
            "button",
            "label"
        };

        bool        is_special_tag(std::string tag, const std::vector<std::string> &table);
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
