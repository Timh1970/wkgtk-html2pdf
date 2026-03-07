#include "pretty_html.h"

#include "iclog.h"

#include <cstdlib>
#include <stdarg.h>
#include <stdio.h>
#include <string>
#include <vector>
typedef typename iclog::category cat;
typedef typename iclog::loglevel lvl;
using std::string;
using std::vector;

namespace phtml {

    enum class status {
        UNPROCESSED,
        OPEN,
        CLOSED
    };

    /**
     * @brief PHTML_DEBUG
     * Internal testing only.  This should not be enables as it will
     * just clog up the logs with every single open and close on every
     * single element.
     */
    bool PHTML_DEBUG = false;

    // Void Tags (should not be closed)
    static const char *const voidTags[]{
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
        "wbr",
        nullptr
    };

    // Blocks (That should not have new lines)
    // This is a work in progress
    static const char *const blockTags[]{
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
        "label",
        nullptr
    };

    struct html_tree_impl {
            // 1. THE SHARED LUNGS: Every node's 'm_htmlPage' points to the same memory.
            std::string &m_htmlPage;

            // 2. THE MASTER STORAGE: Only the ROOT node actually uses this.
            std::string m_rootBuffer;

            // 3. THE NODE DATA: The stuff you had in your original class.
            std::string              m_htmlTag;
            std::vector<html_tree *> childNodes;
            status                   m_nodestatus   = status::UNPROCESSED;
            html_tree               *previousBranch = nullptr;

            struct TAG_CONTENT {
                    std::string text;
                    bool        lineBreak;
            };

            std::vector<TAG_CONTENT> m_tagContent;

            // --- THE TWO CONSTRUCTORS FOR THE PIMPL ---

            // A. For the ROOT: Point the reference to our own 'm_rootBuffer'
            html_tree_impl(const char *tag)
                : m_htmlPage(m_rootBuffer),
                  m_htmlTag(tag) {}

            // B. For the CHILDREN: Point the reference to the PARENT'S 'm_htmlPage'
            // Child Constructor
            html_tree_impl(const char *tag, std::string &master, html_tree *parent)
                : m_htmlPage(master),
                  m_htmlTag(tag),
                  previousBranch(parent) {}

            bool        is_special_tag(const std::string &tag, const char *const table[]);
            bool        is_leaf_node();
            std::string handle_special_characters(std::string text);
            status      closed();
            void        open_node(unsigned tabs = 0);
            void        close_node(unsigned tabs = 0);
    };

} // namespace phtml

phtml::html_tree::html_tree(const char *htmlTag, bool includeHeader)
    : m_pimpl(new html_tree_impl(htmlTag)) {
    // INITIALISE THE MASTER PAGE (Only happens once)
    if (includeHeader) {
        // Only add the "poison" if we're going to a web server
        m_pimpl->m_htmlPage = "Content-type:text/html\r\n\r\n<!DOCTYPE html>\n";
    } else {
        // Just the clean DocType for local files
        m_pimpl->m_htmlPage = "<!DOCTYPE html>\n";
    }
    m_pimpl->m_nodestatus = status::UNPROCESSED;
}

// THE CHILD (Private - used by new_node)
phtml::html_tree::html_tree(const char *htmlTag, html_tree *parent_handle)
    : m_pimpl(new html_tree_impl(htmlTag, parent_handle->m_pimpl->m_htmlPage, parent_handle)) {
    m_pimpl->m_nodestatus = status::UNPROCESSED;
}

/**************************************/
/*  DESTRUCTOR                        */
/**************************************/
phtml::html_tree::~html_tree() {
    for (html_tree *it : m_pimpl->childNodes) {
        delete it;
    }
    delete m_pimpl;
    m_pimpl = nullptr;
}

/**
 * @brief html_tree::is_special_tag
 * @param tag
 * @param table
 * @return true if element should not be explicitly closed
 *
 * Check if element should be closed
 */
bool phtml::html_tree_impl::is_special_tag(const string &tag, const char *const table[]) {
    for (int i = 0; table[i] != nullptr; ++i) {
        if (tag == table[i])
            return true;
    }
    return false;
}

/**
 * @brief html_tree::handle_special_characters
 * @param text
 * @return The string with the designated characters replaced by the escape
 * sequences.
 *
 * Search the string for characters that require escaping and escape them
 */
string phtml::html_tree_impl::handle_special_characters(string text) {

    string subjectString(text);

    struct FIND_REPLACE {
            string search;
            string replace;
    };

    // ADD ALL ESCAPE SEQUENCES HERE
    // NOTE THAT THE AMPERSAND CHAR MUST BE FIRST OTHERWISE IT WILL STRIP OUT THE
    // AMPERSANDS FROM ANY ESCAPE SEQUENCES THAT PRECEDE IT
    vector<FIND_REPLACE> FRStrings{
        {"&",  "&amp;" },
        {"\"", "&quot;"},
        {"'",  "&apos;"},
        {"<",  "&lt;"  },
        {">",  "&gt;"  },
        {"/",  "&#x2F;"}
    };

    for (FIND_REPLACE &it : FRStrings) {
        size_t start_pos = 0;
        while ((start_pos = subjectString.find(it.search, start_pos)) != std::string::npos) {
            subjectString.replace(start_pos, it.search.length(), it.replace);
            start_pos += it.replace.length(); // Handles case where 'to' is a substring of 'from'
        }
    }

    return (subjectString);
}

/**
 * @brief html_tree::new_node
 * @param htmlTag
 * @return A pointer to the newly created node.
 *
 * Create a new node
 */
phtml::html_tree *phtml::html_tree::new_node(const char *htmlTag) {
    // We pass 'this' (the current node) as the parent of the new child
    html_tree *child = new html_tree(htmlTag, this);
    m_pimpl->childNodes.push_back(child);
    return child;
}

phtml::html_tree *phtml::html_tree::new_node_f(const char *format, ...) {
    char   *buffer = nullptr;
    va_list args;
    va_start(args, format);
    html_tree *result = nullptr;

    if (vasprintf(&buffer, format, args) != -1) {
        result = new_node(buffer);
        free(buffer);
    }
    va_end(args);
    return result;
}

void phtml::html_tree::set_node_content_f(const char *format, ...) {
    char   *buffer = nullptr;
    va_list args;
    va_start(args, format);
    // vasprintf handles the size calculation and allocation for you
    if (vasprintf(&buffer, format, args) != -1) {
        // Call your existing ABI-safe method
        set_node_content(buffer);
        free(buffer); // Clean up the temporary string
    }
    va_end(args);
}

/**
 * @brief html_tree::is_leaf_node
 * @return
 *
 * Does this element have children
 */
bool phtml::html_tree_impl::is_leaf_node() {
    return ((childNodes.empty()) ? true : false);
}

/**
 * @brief html_tree::open_node
 * @param tabs
 *
 * Apply an open "<" to the declaration, also indent the node
 * to make the resultant html more readable.
 *
 * Apply a ">" at the end of the element instantiation.
 */
void phtml::html_tree_impl::open_node(unsigned tabs) {
    if (tabs)
        m_htmlPage.append(tabs, '\t');

    std::string tag = m_htmlTag.substr(0, m_htmlTag.find_first_of(" "));
    bool        blockTag(is_special_tag(tag, blockTags));

    m_htmlPage.append("<" + m_htmlTag + ">");

    if (!blockTag)
        m_htmlPage.append("\n");

    if (!m_tagContent.empty()) {
        for (phtml::html_tree_impl::TAG_CONTENT &s : m_tagContent) {

            if (blockTag) {
                m_htmlPage.append(s.text);
                if ((s.lineBreak) && (&s != &m_tagContent.back()))
                    m_htmlPage.append("<br>");
            } else {
                m_htmlPage.append(tabs + 1, '\t');
                m_htmlPage.append(s.text);
                if ((s.lineBreak) && (&s != &m_tagContent.back()))
                    m_htmlPage.append("<br>");

                m_htmlPage.append("\n");
            }
        }
    }
    m_nodestatus = status::OPEN;
    wkJlog
        << lvl::debug << cat::LIB_HTML << iclog_FUNCTION
        << m_htmlTag
        << iclog::endl;
}

/**
 * @brief html_tree::close_node
 * @param tabs - the number of indent tabs
 *
 * Close the element
 */
void phtml::html_tree_impl::close_node(unsigned tabs) {

    string tag(m_htmlTag.substr(0, m_htmlTag.find_first_of(" ")));

    if (!is_special_tag(tag, voidTags)) {

        if (!is_special_tag(tag, blockTags)) {
            if (tabs)
                m_htmlPage.append(tabs, '\t');
        }

        m_htmlPage.append("</" + tag + ">\n");
    }

    m_nodestatus = status::CLOSED;
    if (PHTML_DEBUG)
        wkJlog
            << lvl::debug << cat::LIB_HTML << iclog_FUNCTION
            << "Closed " << tag
            << iclog::endl;
}

/**
 * @brief html_tree::closed
 * @return the status of the element.
 */
phtml::status phtml::html_tree_impl::closed() { return (m_nodestatus); }

/******************************************************************************/
/*  SET NODE CONTENT                                                          */
/******************************************************************************/
/**
 * @brief html_tree::set_node_content
 * @param content
 * @param lineBreak
 * @param escapeSpecialChars - (replce < with &lt; etc..)
 *
 * @note If enables you cannot use inline formattting so <br> would be printed
 * literally rather than be used as a line break.
 *
 */
void phtml::html_tree::set_node_content(const char *content, bool lineBreak, bool escapeSpecialChars) {
    if (escapeSpecialChars)
        m_pimpl->m_tagContent.push_back({m_pimpl->handle_special_characters(content), lineBreak});
    else
        m_pimpl->m_tagContent.push_back({content, lineBreak});
}

const char *phtml::html_tree::get_html() const {
    return m_pimpl->m_htmlPage.c_str();
}

// END OF CLASS  --  ^^^^^^^^
//------------------------------------------------------------------------------------------//

/**
 * @brief pretty_html::find_first_open_sibling
 * @param parent
 * @return
 *
 * Iterate through each node until you find the first one that is
 * not marked closed.
 */
phtml::html_tree *phtml::find_first_open_sibling(html_tree *parent) {

    html_tree *workNode = nullptr;

    for (html_tree *it : parent->m_pimpl->childNodes) {
        if (it->m_pimpl->closed() != status::CLOSED) {
            workNode = it;
            if (PHTML_DEBUG)
                wkJlog
                    << lvl::debug << cat::LIB_HTML << iclog_FUNCTION
                    << "found child node"
                    << iclog::endl;
            break;
        }
    }

    return (workNode);
}

/**
 * @brief pretty_html::process_nodes
 * @param primaryNode
 *
 * Takes the instanace of html_tree that represents the primary
 * element (the dom) and iterate through all its children in order
 * to generate the html.
 *
 * The HTML itself is contained within the std::string attached to
 * the primary html_tree instance.
 */
void phtml::process_nodes(html_tree *primaryNode) {
    html_tree *curNode = primaryNode;

    unsigned tabs = 0;
    while (primaryNode->m_pimpl->closed() != status::CLOSED) {

        if (curNode->m_pimpl->closed() == status::UNPROCESSED) {
            curNode->m_pimpl->open_node(tabs);
            ++tabs;
            if (PHTML_DEBUG)
                wkJlog
                    << lvl::debug << cat::LIB_HTML << iclog_FUNCTION
                    << "opening unprocessed node"
                    << iclog::endl;
        }

        // if the current node is a leaf node and it is open then close it
        if ((curNode->m_pimpl->is_leaf_node()) && (curNode->m_pimpl->closed() == status::OPEN)) {

            --tabs;
            curNode->m_pimpl->close_node(tabs);
            if (PHTML_DEBUG)
                wkJlog
                    << lvl::debug << cat::LIB_HTML << iclog_FUNCTION
                    << "closing leaf node"
                    << iclog::endl;
        }

        // NEXT NODE
        // find the next node to work on
        // first of all we need to see if the node has any open children
        html_tree *sibling = find_first_open_sibling(curNode);

        // if the node does have children then we need to drill down to the first unclosed child
        if (sibling) {

            curNode = sibling;
            if (PHTML_DEBUG)
                wkJlog
                    << lvl::debug << cat::LIB_HTML << iclog_FUNCTION
                    << "setting current node to child"
                    << iclog::endl;

        }

        // if it has no children then it can be closed regardless of whether it is a leaf node
        // or not.
        else {

            // if the current node has no open children then close it
            if (curNode->m_pimpl->closed() == status::OPEN) {
                --tabs;
                curNode->m_pimpl->close_node(tabs);
            }

            // we can then make our way back up to the parent node
            curNode = curNode->m_pimpl->previousBranch;
            if (PHTML_DEBUG)
                wkJlog
                    << lvl::debug << cat::LIB_HTML << iclog_FUNCTION
                    << "returninng to previous branch"
                    << iclog::endl;
        }
    }
}
