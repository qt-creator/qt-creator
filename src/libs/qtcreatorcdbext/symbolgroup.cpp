/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "symbolgroup.h"
#include "stringutils.h"
#include "base64.h"

#include <algorithm>
#include <sstream>
#include <list>
#include <iterator>
#include <set>

enum { debug = 0 };

typedef std::vector<int>::size_type VectorIndexType;
typedef std::vector<std::string> StringVector;

const char rootNameC[] = "local";

enum { BufSize = 2048 };

std::ostream &operator<<(std::ostream &str, const DEBUG_SYMBOL_PARAMETERS &parameters)
{
    str << "parent=";
    if (parameters.ParentSymbol == DEBUG_ANY_ID) {
        str << "DEBUG_ANY_ID";
    } else {
        str << parameters.ParentSymbol ;
    }
    str << " flags=" << parameters.Flags;
    // Detailed flags:
    if (parameters.Flags & DEBUG_SYMBOL_EXPANDED)
        str << " EXPANDED";
    if (parameters.Flags & DEBUG_SYMBOL_READ_ONLY)
        str << " READONLY";
    if (parameters.Flags & DEBUG_SYMBOL_IS_ARRAY)
        str << " ARRAY";
    if (parameters.Flags & DEBUG_SYMBOL_IS_FLOAT)
        str << " FLOAT";
    if (parameters.Flags & DEBUG_SYMBOL_IS_ARGUMENT)
        str << " ARGUMENT";
    if (parameters.Flags & DEBUG_SYMBOL_IS_LOCAL)
        str << " LOCAL";
    str << " typeId=" << parameters.TypeId << " subElements="
              << parameters.SubElements;
    return str;
}

SymbolGroup::SymbolGroup(IDebugSymbolGroup2 *sg,
                         const SymbolParameterVector &vec,
                         ULONG threadId,
                         unsigned frame) :
    m_symbolGroup(sg),
    m_threadId(threadId),
    m_frame(frame),
    m_root(SymbolGroupNode::create(sg, rootNameC, vec))
{
}

SymbolGroup::~SymbolGroup()
{
    m_symbolGroup->Release();
    delete m_root;
}

static inline bool getSymbolCount(CIDebugSymbolGroup *symbolGroup,
                                  ULONG *count,
                                  std::string *errorMessage)
{
    const HRESULT hr = symbolGroup->GetNumberSymbols(count);
    if (FAILED(hr)) {
        *errorMessage = msgDebugEngineComFailed("GetNumberSymbols", hr);
        return false;
    }
    return true;
}

bool SymbolGroup::getSymbolParameters(CIDebugSymbolGroup *symbolGroup,
                                      SymbolParameterVector *vec,
                                      std::string *errorMessage)
{
    ULONG count;
    return getSymbolCount(symbolGroup, &count, errorMessage)
            && getSymbolParameters(symbolGroup, 0, count, vec, errorMessage);
}

bool SymbolGroup::getSymbolParameters(CIDebugSymbolGroup *symbolGroup,
                                      unsigned long start,
                                      unsigned long count,
                                      SymbolParameterVector *vec,
                                      std::string *errorMessage)
{
    if (!count) {
        vec->clear();
        return true;
    }
    // Trim the count to the maximum count available. When expanding elements
    // and passing SubElements as count, SubElements might be an estimate that
    // is too large and triggers errors.
    ULONG totalCount;
    if (!getSymbolCount(symbolGroup, &totalCount, errorMessage))
        return false;
    if (start >= totalCount) {
        std::ostringstream str;
        str << "SymbolGroup::getSymbolParameters: Start parameter "
            << start << " beyond total " << totalCount << '.';
        *errorMessage = str.str();
        return  false;
    }
    if (start + count > totalCount)
        count = totalCount - start;
    // Get parameters.
    vec->resize(count);
    const HRESULT hr = symbolGroup->GetSymbolParameters(start, count, &(*vec->begin()));
    if (FAILED(hr)) {
        std::ostringstream str;
        str << "SymbolGroup::getSymbolParameters failed for index=" << start << ", count=" << count
            << ": " << msgDebugEngineComFailed("GetSymbolParameters", hr);
        *errorMessage = str.str();
        return false;
    }
    return true;
}

SymbolGroup *SymbolGroup::create(CIDebugControl *control, CIDebugSymbols *debugSymbols,
                                 ULONG threadId, unsigned frame,
                                 std::string *errorMessage)
{
    errorMessage->clear();

    ULONG obtainedFrameCount = 0;
    const ULONG frameCount = frame + 1;

    DEBUG_STACK_FRAME *frames = new DEBUG_STACK_FRAME[frameCount];
    IDebugSymbolGroup2 *idebugSymbols = 0;
    bool success = false;
    SymbolParameterVector parameters;

    // Obtain symbol group at stack frame.
    do {
        HRESULT hr = control->GetStackTrace(0, 0, 0, frames, frameCount, &obtainedFrameCount);
        if (FAILED(hr)) {
            *errorMessage = msgDebugEngineComFailed("GetStackTrace", hr);
            break;
        }
        if (obtainedFrameCount < frameCount ) {
            std::ostringstream str;
            str << "Unable to obtain frame " << frame << " (" << obtainedFrameCount << ").";
            *errorMessage = str.str();
            break;
        }
        hr = debugSymbols->GetScopeSymbolGroup2(DEBUG_SCOPE_GROUP_LOCALS, NULL, &idebugSymbols);
        if (FAILED(hr)) {
            *errorMessage = msgDebugEngineComFailed("GetScopeSymbolGroup2", hr);
            break;
        }
        hr = debugSymbols->SetScope(0, frames + frame, NULL, 0);
        if (FAILED(hr)) {
            *errorMessage = msgDebugEngineComFailed("SetScope", hr);
            break;
        }
        // refresh with current frame
        hr = debugSymbols->GetScopeSymbolGroup2(DEBUG_SCOPE_GROUP_LOCALS, idebugSymbols, &idebugSymbols);
        if (FAILED(hr)) {
            *errorMessage = msgDebugEngineComFailed("GetScopeSymbolGroup2", hr);
            break;
        }
        if (!SymbolGroup::getSymbolParameters(idebugSymbols, &parameters, errorMessage))
            break;

        success = true;
    } while (false);
    delete [] frames;
    if (!success) {
        if (idebugSymbols)
            idebugSymbols->Release();
        return 0;
    }
    return new SymbolGroup(idebugSymbols, parameters, threadId, frame);
}

// ------- SymbolGroupNode
SymbolGroupNode::SymbolGroupNode(CIDebugSymbolGroup *symbolGroup,
                                 const std::string &name,
                                 const std::string &iname,
                                 SymbolGroupNode *parent) :
    m_symbolGroup(symbolGroup), m_parent(parent), m_name(name), m_iname(iname), m_flags(0)
{
    memset(&m_parameters, 0, sizeof(DEBUG_SYMBOL_PARAMETERS));
    m_parameters.ParentSymbol = DEBUG_ANY_ID;
}

void SymbolGroupNode::removeChildren()
{
    if (!m_children.empty()) {
        const SymbolGroupNodePtrVectorIterator end = m_children.end();
        for (SymbolGroupNodePtrVectorIterator it = m_children.begin(); it != end; ++it)
            delete *it;
        m_children.clear();
    }
}

bool SymbolGroupNode::isArrayElement() const
{
    return m_parent && (m_parent->m_parameters.Flags & DEBUG_SYMBOL_IS_ARRAY);
}

// Return full iname as 'locals.this.m_sth'.
std::string SymbolGroupNode::fullIName() const
{
    std::string rc = iName();
    for (const SymbolGroupNode *p = parent(); p; p = p->parent()) {
        rc.insert(0, 1, '.');
        rc.insert(0, p->iName());
    }
    return rc;
}

// Fix an array iname "[0]" -> "0" for sorting to work correctly
static inline void fixArrayIname(std::string *iname)
{
    if (!iname->empty() && iname->at(0) == '[') {
        const std::string::size_type last = iname->size() - 1;
        if (iname->at(last) == ']') {
            iname->erase(last, 1);
            iname->erase(0, 1);
        }
    }
}

// Fix up names and inames
static inline void fixNames(bool isTopLevel, StringVector *names, StringVector *inames)
{
    if (names->empty())
        return;
    unsigned unnamedId = 1;
    /* 1) Fix name="__formal", which occurs when someone writes "void foo(int /* x * /)..."
     * 2) Fix array inames for sorting: "[6]" -> name="[6]",iname="6"
     * 3) For toplevels: Fix shadowed variables in the order the debugger expects them:
       \code
       int x;             // Occurrence (1), should be reported as name="x <shadowed 1>"/iname="x#1"
       if (true) {
          int x = 5; (2)  // Occurrence (2), should be reported as name="x"/iname="x"
       }
      \endcode */
    StringVector::iterator nameIt = names->begin();
    const StringVector::iterator namesEnd = names->end();
    for (StringVector::iterator iNameIt = inames->begin(); nameIt != namesEnd ; ++nameIt, ++iNameIt) {
        std::string &name = *nameIt;
        std::string &iname = *iNameIt;
        if (name.empty() || name == "__formal") {
            const std::string number = toString(unnamedId++);
            name = "<unnamed "  + number + '>';
            iname = "unnamed#" + number;
        } else {
            fixArrayIname(&iname);
        }
        if (isTopLevel) {
            if (const StringVector::size_type shadowCount = std::count(nameIt + 1, namesEnd, name)) {
                const std::string number = toString(shadowCount);
                name += " <shadowed ";
                name += number;
                name += '>';
                iname += '#';
                iname += number;
            }
        }
    }
}

// Index: Index of symbol, parameterOffset: Looking only at a part of the symbol array, offset
void SymbolGroupNode::parseParameters(VectorIndexType index,
                                      VectorIndexType parameterOffset,
                                      const SymbolGroup::SymbolParameterVector &vec)
{
    static char buf[BufSize];
    ULONG obtainedSize;

    const bool isTopLevel = index == DEBUG_ANY_ID;
    if (isTopLevel) {
        m_parameters.Flags |= DEBUG_SYMBOL_EXPANDED;
    } else {
        m_parameters = vec.at(index - parameterOffset);
        if (m_parameters.SubElements == 0 || !(m_parameters.Flags & DEBUG_SYMBOL_EXPANDED))
            return; // No children
    }
    if (m_parameters.SubElements > 1)
        m_children.reserve(m_parameters.SubElements);

    const VectorIndexType size = vec.size();
    // Scan the top level elements
    StringVector names;
    names.reserve(size);
    // Pass 1) Determine names. We need the complete set first in order to do some corrections.
    const VectorIndexType startIndex = isTopLevel ? 0 : index + 1;
    for (VectorIndexType pos = startIndex - parameterOffset; pos < size ; pos++ ) {
        if (vec.at(pos).ParentSymbol == index) {
            const VectorIndexType symbolGroupIndex = pos + parameterOffset;
            if (FAILED(m_symbolGroup->GetSymbolName(ULONG(symbolGroupIndex), buf, BufSize, &obtainedSize)))
                buf[0] = '\0';
            names.push_back(std::string(buf));
        }
    }
    // 2) Fix names
    StringVector inames = names;
    fixNames(isTopLevel, &names, &inames);
    // Pass 3): Add nodes with fixed names
    StringVector::size_type nameIndex = 0;
    for (VectorIndexType pos = startIndex - parameterOffset; pos < size ; pos++ ) {
        if (vec.at(pos).ParentSymbol == index) {
            const VectorIndexType symbolGroupIndex = pos + parameterOffset;
            SymbolGroupNode *child = new SymbolGroupNode(m_symbolGroup, names.at(nameIndex),
                                                         inames.at(nameIndex), this);
            child->parseParameters(symbolGroupIndex, parameterOffset, vec);
            m_children.push_back(child);
            nameIndex++;
        }
    }
    if (isTopLevel)
        m_parameters.SubElements = ULONG(m_children.size());
}

SymbolGroupNode *SymbolGroupNode::create(CIDebugSymbolGroup *sg, const std::string &name, const SymbolGroup::SymbolParameterVector &vec)
{
    SymbolGroupNode *rc = new SymbolGroupNode(sg, name, name);
    rc->parseParameters(DEBUG_ANY_ID, 0, vec);
    return rc;
}

// Fix some oddities in CDB values
static void fixValue(const std::string &type, std::wstring *value)
{
    // Pointers: fix '0x00000000`00000AD class bla' ... to "0xAD", but leave
    // 'const char *' values as is.
    if (!type.empty() && type.at(type.size() - 1) == L'*' && value->compare(0, 2, L"0x") == 0) {
        // Remove dumb 64bit separator
        if (value->size() > 10 && value->at(10) == L'`')
            value->erase(10, 1);
        const std::string::size_type firstNonNullDigit = value->find_first_not_of(L"0", 2);
        // No on-null digits: plain null ptr.
        if (firstNonNullDigit == std::string::npos || value->at(firstNonNullDigit) == ' ') {
            *value = L"0x0";
            return;
        }
        // Strip
        if (firstNonNullDigit > 2)
            value->erase(2, firstNonNullDigit - 2);
        // Strip ' Class bla"
        std::wstring::size_type classPos = value->find(L" struct", 2);
        if (classPos == std::string::npos)
            classPos = value->find(L" class", 2);
        if (classPos != std::string::npos)
            value->erase(classPos, value->size() - classPos);
        return;
    }
    // Integers: fix '0n10' -> '10'
    if (value->size() >= 3 && value->compare(0, 2, L"0n") == 0
        && (isdigit(value->at(2)) || value->at(2) == L'-')) {
        value->erase(0, 2);
        return;
    }
}

// Check for ASCII-encode-able stuff. Plain characters + tabs at the most, no newline.
static bool isSevenBitClean(const wchar_t *buf, ULONG size)
{
    const wchar_t *bufEnd = buf + size;
    for (const wchar_t *bufPtr = buf; bufPtr < bufEnd; bufPtr++) {
        const wchar_t c = *bufPtr;
        if (c > 127 || (c < 32 && c != 9))
            return false;
    }
    return true;
}

std::string SymbolGroupNode::getType(ULONG index) const
{
    static char buf[BufSize];
    const HRESULT hr = m_symbolGroup->GetSymbolTypeName(index, buf, BufSize, NULL);
    return SUCCEEDED(hr) ? std::string(buf) : std::string();
}

wchar_t *SymbolGroupNode::getValue(ULONG index,
                                   ULONG *obtainedSizeIn /* = 0 */) const
{
    // Determine size and return allocated buffer
    if (obtainedSizeIn)
        *obtainedSizeIn = 0;
    const ULONG maxValueSize = 262144;
    ULONG obtainedSize = 0;
    HRESULT hr = m_symbolGroup->GetSymbolValueTextWide(index, NULL, maxValueSize, &obtainedSize);
    if (FAILED(hr))
        return 0;
    if (obtainedSize > maxValueSize)
        obtainedSize = maxValueSize;
    wchar_t *buffer = new wchar_t[obtainedSize];
    hr = m_symbolGroup->GetSymbolValueTextWide(index, buffer, obtainedSize, &obtainedSize);
    if (FAILED(hr)) { // Whoops, should not happen
        delete [] buffer;
        return 0;
    }
    if (obtainedSizeIn)
        *obtainedSizeIn = obtainedSize;
    return buffer;
}

ULONG64 SymbolGroupNode::address(ULONG index) const
{
    ULONG64 address = 0;
    const HRESULT hr = m_symbolGroup->GetSymbolOffset(index, &address);
    if (SUCCEEDED(hr))
        return address;
    return 0;
}

std::wstring SymbolGroupNode::rawValue(ULONG index) const
{
    std::wstring rc;
    if (const wchar_t *wbuf = getValue(index)) {
        rc = wbuf;
        delete[] wbuf;
    }
    return rc;
}

std::wstring SymbolGroupNode::fixedValue(ULONG index) const
{
    std::wstring value = rawValue(index);
    fixValue(getType(index), &value);
    return value;
}

SymbolGroupNode *SymbolGroupNode::childAt(unsigned i) const
{
    return i < m_children.size() ? m_children.at(i) : static_cast<SymbolGroupNode *>(0);
}

static inline void indentStream(std::ostream &str, unsigned depth)
{
    for (unsigned d = 0; d < depth; d++)
        str << "  ";
}

void SymbolGroupNode::dump(std::ostream &str, unsigned child, unsigned depth,
                           bool humanReadable, ULONG &index) const
{
    const std::string iname = fullIName();
    const std::string type = getType(index);

    if (child) { // Separate list of children
        str << ',';
        if (humanReadable)
            str << '\n';
    }

    if (humanReadable)
        indentStream(str, depth);
    str << "{iname=\"" << iname << "\",exp=\"" << iname << "\",name=\"" << m_name
        << "\",type=\"" << type << '"';

    if (const ULONG64 addr = address(index))
        str << ",addr=\"" << std::hex << std::showbase << addr << std::noshowbase << std::dec
               << '"';

    bool valueEditable = true;
    bool valueEnabled = true;

    const bool uninitialized = m_flags & Uninitialized;
    if (uninitialized) {
        valueEditable = valueEnabled = false;
        str << ",valueencoded=\"0\",value=\"<not in scope>\"";
    } else {
        ULONG obtainedSize = 0;
        if (const wchar_t *wbuf = getValue(index, &obtainedSize)) {
            const ULONG valueSize = obtainedSize - 1;
            // ASCII or base64?
            if (isSevenBitClean(wbuf, valueSize)) {
                std::wstring value = wbuf;
                fixValue(type, &value);
                str << ",valueencoded=\"0\",value=\"" << gdbmiWStringFormat(value) << '"';
            } else {
                str << ",valueencoded=\"2\",value=\"";
                base64Encode(str, reinterpret_cast<const unsigned char *>(wbuf), valueSize * sizeof(wchar_t));
                str << '"';
            }
            delete [] wbuf;
        }
    }
    // Children: Dump all known or subelements (guess).
    const VectorIndexType childCountGuess = uninitialized ? 0 :
             (m_children.empty() ? m_parameters.SubElements : m_children.size());
    // No children..suppose we are editable and enabled
    if (childCountGuess != 0)
        valueEditable = false;
    str << ",valueenabled=\"" << (valueEnabled ? "true" : "false") << '"'
        << ",valueeditable=\"" << (valueEditable ? "true" : "false") << '"'
        << ",numchild=\"" << childCountGuess << '"';
    if (!uninitialized && !m_children.empty()) {
        str << ",children=[";
        if (humanReadable)
            str << '\n';
    }
}

void SymbolGroupNode::dumpChildrenVisited(std::ostream &str, bool humanReadable) const
{
    if (!m_children.empty())
        str << ']';
    str << '}';
    if (humanReadable)
        str << '\n';

}
bool SymbolGroupNode::accept(SymbolGroupNodeVisitor &visitor, unsigned child, unsigned depth, ULONG &index) const
{
    // If we happen to be the root node, just skip over

    const bool invisibleRoot = index == DEBUG_ANY_ID;
    const unsigned childDepth = invisibleRoot ? 0 : depth + 1;

    if (invisibleRoot) {
        index = 0;
    } else {
        // Visit us and move index forward.
        if (visitor.visit(this, child, depth, index))
            return true;
        index++;
    }

    const unsigned childCount = unsigned(m_children.size());
    for (unsigned c = 0; c < childCount; c++)
        if (m_children.at(c)->accept(visitor, c, childDepth, index))
            return true;
    if (!invisibleRoot)
        visitor.childrenVisited(this, depth);
    return false;
}

void SymbolGroupNode::debug(std::ostream &str, unsigned verbosity, unsigned depth, ULONG index) const
{
    indentStream(str, depth);
    str << '"' << m_name << "\" Children=" << m_children.size() << ' ' << m_parameters
           << " flags=" << m_flags;
    if (verbosity) {
        str << " Address=0x" << std::hex << address(index) << std::dec
            << " Type=\"" << getType(index) << '"';
        if (!(m_flags & Uninitialized))
            str << "\" Value=\"" << gdbmiWStringFormat(rawValue(index)) << '"';
    }
    str << '\n';
}

// Index offset when stepping past this node in a symbol parameter array. Basically
// self + recursive all child counts.
ULONG SymbolGroupNode::recursiveIndexOffset() const
{
    ULONG rc = 1u;
    if (!m_children.empty()) {
        const SymbolGroupNodePtrVectorConstIterator cend = m_children.end();
        for (SymbolGroupNodePtrVectorConstIterator it = m_children.begin(); it != cend; ++it)
            rc += (*it)->recursiveIndexOffset();
    }
    return rc;
}

// Expand!
bool SymbolGroupNode::expand(ULONG index, std::string *errorMessage)
{
    if (::debug > 1)
        DebugPrint() << "SymbolGroupNode::expand "  << m_name << ' ' << index;
    if (!m_children.empty())
        return true;
    if (m_parameters.SubElements == 0) {
        *errorMessage = "No subelements to expand in node: " + fullIName();
        return false;
    }
    if (m_flags & Uninitialized) {
        *errorMessage = "Refusing to expand uninitialized node: " + fullIName();
        return false;
    }

    const HRESULT hr = m_symbolGroup->ExpandSymbol(index, TRUE);

    if (FAILED(hr)) {
        *errorMessage = msgDebugEngineComFailed("ExpandSymbol", hr);
        return false;
    }
    SymbolGroup::SymbolParameterVector parameters;
    // Retrieve parameters (including self, re-retrieve symbol parameters to get new 'expanded' flag
    // and corrected SubElement count (might be estimate)) and create child nodes.
    if (!SymbolGroup::getSymbolParameters(m_symbolGroup,
                                          index, m_parameters.SubElements + 1,
                                          &parameters, errorMessage))
        return false;
    parseParameters(index, index, parameters);
    return true;
}

static inline std::string msgNotFound(const std::string &nodeName)
{
    std::ostringstream str;
    str << "Node '" << nodeName << "' not found.";
    return str.str();
}

std::string SymbolGroup::dump(bool humanReadable) const
{
    std::ostringstream str;
    DumpSymbolGroupNodeVisitor visitor(str, humanReadable);
    if (humanReadable)
        str << '\n';
    str << '[';
    accept(visitor);
    str << ']';
    return str.str();
}

// Dump a node, potentially expand
std::string SymbolGroup::dump(const std::string &name, bool humanReadable, std::string *errorMessage)
{
    ULONG index;
    SymbolGroupNode *const node = find(name, &index);
    if (node == 0) {
        *errorMessage = msgNotFound(name);
        return std::string();
    }
    if (node->subElements() && node->children().empty()) {
        if (!expand(name, errorMessage))
            return false;
    }
    std::ostringstream str;
    if (humanReadable)
        str << '\n';
    DumpSymbolGroupNodeVisitor visitor(str, humanReadable);
    str << '[';
    node->accept(visitor, 0, 0, index);
    str << ']';
    return str.str();
}

std::string SymbolGroup::debug(unsigned verbosity) const
{
    std::ostringstream str;
    DebugSymbolGroupNodeVisitor visitor(str, verbosity);
    accept(visitor);
    return str.str();
}

/* expandList: Expand a list of inames with a 'mkdir -p'-like behaviour, that is,
 * expand all sub-paths. The list of inames has thus to be reordered to expand the
 * parent items first, for example "locals.this.i1.data,locals.this.i2" --->:
 * "locals, locals.this, locals.this.i1, locals.this.i2, locals.this.i1.data".
 * This is done here by creating a set of name parts keyed by level and name
 * (thus purging duplicates). */

typedef std::pair<unsigned, std::string> InamePathEntry;

struct InamePathEntryLessThan : public std::binary_function<InamePathEntry, InamePathEntry, bool> {
    bool operator()(const InamePathEntry &i1, const InamePathEntry& i2) const
    {
        if (i1.first < i2.first)
            return true;
        if (i1.first != i2.first)
            return false;
        return i1.second < i2.second;
    }
};

typedef std::set<InamePathEntry, InamePathEntryLessThan> InamePathEntrySet;

// Expand a node list "locals.i1,locals.i2"
unsigned SymbolGroup::expandList(const std::vector<std::string> &nodes, std::string *errorMessage)
{
    if (nodes.empty())
        return 0;
    // Create a set with a key <level, name>. Also required for 1 node (see above).
    InamePathEntrySet pathEntries;
    const VectorIndexType nodeCount = nodes.size();
    for (VectorIndexType i= 0; i < nodeCount; i++) {
        const std::string &iname = nodes.at(i);
        std::string::size_type pos = 0;
        for (unsigned level = 0; pos < iname.size(); level++) {
            std::string::size_type dotPos = iname.find('.', pos);
            if (dotPos == std::string::npos)
                dotPos = iname.size();
            pathEntries.insert(InamePathEntry(level, iname.substr(0, dotPos)));
            pos = dotPos + 1;
        }
    }
    // Now expand going by level.
    unsigned succeeded = 0;
    std::string nodeError;
    InamePathEntrySet::const_iterator cend = pathEntries.end();
    for (InamePathEntrySet::const_iterator it = pathEntries.begin(); it != cend; ++it)
        if (expand(it->second, &nodeError)) {
            succeeded++;
        } else {
            if (!errorMessage->empty())
                errorMessage->append(", ");
            errorMessage->append(nodeError);
        }
    return succeeded;
}

bool SymbolGroup::expand(const std::string &nodeName, std::string *errorMessage)
{
    ULONG index = DEBUG_ANY_ID;
    SymbolGroupNode *node = find(nodeName, &index);
    if (::debug)
        DebugPrint() << "expand: " << nodeName << " found=" << (node != 0) << " index= " << index << '\n';
    if (!node) {
        *errorMessage = msgNotFound(nodeName);
        return false;
    }
    if (node == m_root) // Shouldn't happen, still, all happy
        return true;
    return node->expand(index, errorMessage);
}

// Mark uninitialized (top level only)
void SymbolGroup::markUninitialized(const std::vector<std::string> &uniniNodes)
{
    if (m_root && !m_root->children().empty() && !uniniNodes.empty()) {
        const std::vector<std::string>::const_iterator unIniNodesBegin = uniniNodes.begin();
        const std::vector<std::string>::const_iterator unIniNodesEnd = uniniNodes.end();

        const SymbolGroupNodePtrVector::const_iterator childrenEnd = m_root->children().end();
        for (SymbolGroupNodePtrVector::const_iterator it = m_root->children().begin(); it != childrenEnd; ++it) {
            if (std::find(unIniNodesBegin, unIniNodesEnd, (*it)->fullIName()) != unIniNodesEnd)
                (*it)->setFlags((*it)->flags() | SymbolGroupNode::Uninitialized);
        }
    }
}

static inline std::string msgAssignError(const std::string &nodeName,
                                         const std::string &value,
                                         const std::string &why)
{
    std::ostringstream str;
    str << "Unable to assign '" << value << "' to '" << nodeName << "': " << why;
    return str.str();
}

bool SymbolGroup::assign(const std::string &nodeName, const std::string &value,
                         std::string *errorMessage)
{
    ULONG index;
    SymbolGroupNode *node = find(nodeName, &index);
    if (node == 0) {
        *errorMessage = msgAssignError(nodeName, value, "No such node");
        return false;
    }
    const HRESULT hr = m_symbolGroup->WriteSymbol(index, const_cast<char *>(value.c_str()));
    if (FAILED(hr)) {
        *errorMessage = msgAssignError(nodeName, value, msgDebugEngineComFailed("WriteSymbol", hr));
        return false;
    }
    return true;
}

bool SymbolGroup::accept(SymbolGroupNodeVisitor &visitor) const
{
    if (!m_root || m_root->children().empty())
        return false;
    ULONG index = DEBUG_ANY_ID;
    return m_root->accept(visitor, 0, 0, index);
}

// Find  "locals.this.i1" and move index recursively
static SymbolGroupNode *findNodeRecursion(const std::vector<std::string> &iname,
                                          unsigned depth,
                                          std::vector<SymbolGroupNode *> nodes,
                                          ULONG *index = 0)
{
    typedef std::vector<SymbolGroupNode *>::const_iterator ConstIt;

    if (::debug > 1)
        DebugPrint() << "findNodeRecursion " << iname.size() << '/'
                     << iname.back() << " depth="  << depth
                     << " nodes=" << nodes.size() << " index=" << (index ? *index : ULONG(0));

    if (nodes.empty())
        return 0;
    // Find the child that matches the iname part at depth
    const ConstIt cend = nodes.end();
    for (ConstIt it = nodes.begin(); it != cend; ++it) {
        SymbolGroupNode *c = *it;
        if (c->name() == iname.at(depth)) {
            if (depth == iname.size() - 1) { // Complete iname matched->happy.
                return c;
            } else {
                // Sub-part of iname matched. Forward index and check children.
                if (index)
                    (*index)++; // Skip ourselves
                return findNodeRecursion(iname, depth + 1, c->children(), index);
            }
        } else {
            if (index) // No match for this child, forward the index past all expanded children
                *index += c->recursiveIndexOffset();
        }
    }
    return 0;
}

SymbolGroupNode *SymbolGroup::findI(const std::string &iname, ULONG *index) const
{
    if (index)
        *index = DEBUG_ANY_ID;

    if (iname.empty())
        return 0;
    // Match the root element only: Shouldn't happen, still, all happy
    if (iname == m_root->name())
        return m_root;

    std::vector<std::string> inameTokens;
    split(iname, '.', std::back_inserter(inameTokens));

    // Must begin with root
    if (inameTokens.front() != m_root->name())
        return 0;

    // Start with index = 0 at root's children
    if (index)
        *index = 0;
    return findNodeRecursion(inameTokens, 1, m_root->children(), index);
}

SymbolGroupNode *SymbolGroup::find(const std::string &iname, ULONG *index) const
{
    SymbolGroupNode *rc = findI(iname, index);
    if (::debug > 1)
        DebugPrint() << "SymbolGroup::find " << iname << ' ' << rc << ' ' << (index ? *index : ULONG(0));
    return rc;
}

// --------- DebugSymbolGroupNodeVisitor
DebugSymbolGroupNodeVisitor::DebugSymbolGroupNodeVisitor(std::ostream &os, unsigned verbosity) :
    m_os(os), m_verbosity(verbosity)
{
}

bool DebugSymbolGroupNodeVisitor::visit(const SymbolGroupNode *node,
                                        unsigned /* child */, unsigned depth, ULONG index)
{
    node->debug(m_os, m_verbosity, depth, index);
    return false;
}

// --------------------- DumpSymbolGroupNodeVisitor
DumpSymbolGroupNodeVisitor::DumpSymbolGroupNodeVisitor(std::ostream &os,
                                                       bool humanReadable) :
    m_os(os), m_humanReadable(humanReadable)
{
}

bool DumpSymbolGroupNodeVisitor::visit(const SymbolGroupNode *node, unsigned child, unsigned depth, ULONG index)
{
    node->dump(m_os, child, depth, m_humanReadable, index);
    return false;
}

void DumpSymbolGroupNodeVisitor::childrenVisited(const SymbolGroupNode *node, unsigned)
{
    node->dumpChildrenVisited(m_os, m_humanReadable);
}
