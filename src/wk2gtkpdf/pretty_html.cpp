#include "pretty_html.h"

#include "iclog.h"

typedef typename iclog::category cat;
typedef typename iclog::loglevel lvl;
using std::endl;
using std::string;
using std::vector;

/**
 * @brief html_tree::html_tree
 * @param htmlTag
 * @param htmlPage
 * @param previousBranch
 *
 * Each instanance of this class is an HTML element with a link to
 * parent, if it has one.
 */
html_tree::html_tree(
    std::string  htmlTag,
    std::string &htmlPage,
    html_tree   *previousBranch
)
    : m_htmlTag(htmlTag),
      m_htmlPage(htmlPage),
      previousBranch(previousBranch) {

    // INITIALISE THE WEB PAGE
    if (m_htmlPage.empty())
        m_htmlPage = "Content-type:text/html\r\n\r\n<!DOCTYPE html>\n";
    m_nodestatus = status::UNPROCESSED;
}

/**************************************/
/*  DESTRUCTOR                        */
/**************************************/
html_tree::~html_tree() {
    for (html_tree *it : childNodes) {
        delete it;
    }
}

/**
 * @brief html_tree::is_special_tag
 * @param tag
 * @param table
 * @return true if element should not be explicitly closed
 *
 * Check if element should be closed
 */
bool html_tree::is_special_tag(string tag, const vector<string> &table) {
    bool voidTag = false;

    for (auto &it : table) {
        if (tag == it)
            voidTag = true;
    }

    return (voidTag);
}

/**
 * @brief html_tree::handle_special_characters
 * @param text
 * @return The string with the designated characters replaced by the escape
 * sequences.
 *
 * Search the string for characters that require escaping and escape them
 */
string html_tree::handle_special_characters(string text) {

    // NOTE:
    //  YOU NEED TO CALL delete[] ON THE RETURNED VALUE WHEN YOU
    //  ARE DONE WITH IT.

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
html_tree *html_tree::new_node(string htmlTag) {
    // html_tree *t = (new html_tree(htmlTag, m_htmlPage, this));
    childNodes.push_back(new html_tree(htmlTag, m_htmlPage, this));
    return (childNodes.back());
}

/**
 * @brief html_tree::is_leaf_node
 * @return
 *
 * Does this element have children
 */
bool html_tree::is_leaf_node() {
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
void html_tree::open_node(unsigned tabs) {

    if (tabs)
        m_htmlPage.append(tabs, '\t');

    string tag(m_htmlTag.substr(0, m_htmlTag.find_first_of(" ")));
    bool   blockTag(is_special_tag(tag, blockTags));

    m_htmlPage.append("<" + m_htmlTag + ">");

    if (!blockTag)
        m_htmlPage.append("\n");

    if (!m_tagContent.empty()) {
        for (TAG_CONTENT &s : m_tagContent) {

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
    jlog
        << lvl::debug << cat::LIB_HTML << iclog_FUNCTION
        << m_htmlTag
        << std::endl;
}

/**
 * @brief html_tree::close_node
 * @param tabs - the number of indent tabs
 *
 * Close the element
 */
void html_tree::close_node(unsigned tabs) {

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
        jlog
            << lvl::debug << cat::LIB_HTML << iclog_FUNCTION
            << tag
            << std::endl;
}

/**
 * @brief html_tree::closed
 * @return the status of the element.
 */
html_tree::status html_tree::closed() { return (m_nodestatus); }

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
void html_tree::set_node_content(string content, bool lineBreak, bool escapeSpecialChars) {
    if (escapeSpecialChars)
        m_tagContent.push_back({handle_special_characters(content), lineBreak});
    else
        m_tagContent.push_back({content, lineBreak});
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
html_tree *pretty_html::find_first_open_sibling(html_tree *parent) {

    html_tree *workNode = nullptr;

    for (html_tree *it : parent->childNodes) {
        if (it->closed() != html_tree::status::CLOSED) {
            workNode = it;
            if (PHTML_DEBUG)
                jlog
                    << lvl::debug << cat::LIB_HTML << iclog_FUNCTION
                    << "found child node"
                    << std::endl;
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
void pretty_html::process_nodes(html_tree *primaryNode) {

    html_tree *curNode = primaryNode;
    unsigned   tabs    = 0;
    while (primaryNode->closed() != html_tree::status::CLOSED) {

        if (curNode->closed() == html_tree::status::UNPROCESSED) {
            curNode->open_node(tabs);
            ++tabs;
            if (PHTML_DEBUG)
                jlog
                    << lvl::debug << cat::LIB_HTML << iclog_FUNCTION
                    << "opening unprocessed node"
                    << std::endl;
        }

        // if the current node is a leaf node and it is open then close it
        if ((curNode->is_leaf_node()) && (curNode->closed() == html_tree::status::OPEN)) {

            --tabs;
            curNode->close_node(tabs);
            if (PHTML_DEBUG)
                jlog
                    << lvl::debug << cat::LIB_HTML << iclog_FUNCTION
                    << "closing leaf node"
                    << std::endl;
        }

        // NEXT NODE
        // find the next node to work on
        // first of all we need to see if the node has any open children
        html_tree *sibling = find_first_open_sibling(curNode);

        // if the node does have children then we need to drill down to the first unclosed child
        if (sibling) {

            curNode = sibling;
            if (PHTML_DEBUG)
                jlog
                    << lvl::debug << cat::LIB_HTML << iclog_FUNCTION
                    << "setting current node to child"
                    << std::endl;

        }

        // if it has no children then it can be closed regardless of whether it is a leaf node
        // or not.
        else {

            // if the current node has no open children then close it
            if (curNode->closed() == html_tree::status::OPEN) {
                --tabs;
                curNode->close_node(tabs);
            }

            // we can then make our way back up to the parent node
            curNode = curNode->previousBranch;
            if (PHTML_DEBUG)
                jlog
                    << lvl::debug << cat::LIB_HTML << iclog_FUNCTION
                    << "returninng to previous branch"
                    << std::endl;
        }
    }
}
