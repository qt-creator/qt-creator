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
#include "symbolgroupvalue.h"
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
    if (parameters.Flags != 0 && parameters.Flags != 1)
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
    str << " typeId=" << parameters.TypeId;
    if (parameters.SubElements)
        str << " subElements=" << parameters.SubElements;
    return str;
}

SymbolGroup::SymbolGroup(IDebugSymbolGroup2 *sg,
                         const SymbolParameterVector &vec,
                         ULONG threadId,
                         unsigned frame) :
    m_symbolGroup(sg),
    m_threadId(threadId),
    m_frame(frame),
    m_root(0)
{
    m_root = SymbolGroupNode::create(this, rootNameC, vec);
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
SymbolGroupNode::SymbolGroupNode(SymbolGroup *symbolGroup,
                                 ULONG index,
                                 const std::string &name,
                                 const std::string &iname,
                                 SymbolGroupNode *parent) :
    m_symbolGroup(symbolGroup), m_parent(parent), m_index(index),
    m_name(name), m_iname(iname), m_flags(0)
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

// Notify about expansion of a node:
// Adapt our index and those of our children if we are behind it.
// Return true if a modification was required to be able to terminate the
// recursion.
bool SymbolGroupNode::notifyExpanded(ULONG index, ULONG insertedCount)
{
    typedef SymbolGroupNodePtrVector::reverse_iterator ReverseIt;
    // Looping backwards over the children. If a subtree has no modifications,
    // (meaning all other indexes are smaller) we can stop.
    const ReverseIt rend = m_children.rend();
    for (ReverseIt it = m_children.rbegin(); it != rend; ++it)
        if (!(*it)->notifyExpanded(index, insertedCount))
            return false;

    // Correct our own + parent index if applicable.
    if (m_index == DEBUG_ANY_ID || m_index < index)
        return false;

    m_index += insertedCount;
    if (m_parameters.ParentSymbol != DEBUG_ANY_ID && m_parameters.ParentSymbol >= index)
        m_parameters.ParentSymbol += insertedCount;
    return true;
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

// Fix names: fix complicated template base names
static inline void fixName(std::string *name)
{
    // Long template base classes 'std::tree_base<Key....>' -> 'std::tree<>'
    // for nice display
    const std::string::size_type templatePos = name->find('<');
    if (templatePos != std::string::npos) {
        name->erase(templatePos + 1, name->size() - templatePos - 1);
        name->push_back('>');
    }
}

// Fix inames: arrays and long, complicated template base names
static inline void fixIname(unsigned &id, std::string *iname)
{
    // Fix array iname "[0]" -> "0" for sorting to work correctly
    if (!iname->empty() && iname->at(0) == '[') {
        const std::string::size_type last = iname->size() - 1;
        if (iname->at(last) == ']') {
            iname->erase(last, 1);
            iname->erase(0, 1);
            return;
        }
    }
    // Long template base classes 'std::tree_base<Key....' -> 'tree@t1',
    // usable as identifier and command line parameter
    const std::string::size_type templatePos = iname->find('<');
    if (templatePos != std::string::npos) {
        iname->erase(templatePos, iname->size() - templatePos);
        if (iname->compare(0, 5, "std::") == 0)
            iname->erase(0, 5);
        iname->append("@t");
        iname->append(toString(id++));
    }
}

// Fix up names and inames
static inline void fixNames(bool isTopLevel, StringVector *names, StringVector *inames)
{
    if (names->empty())
        return;
    unsigned unnamedId = 1;
    unsigned templateId = 1;
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
            fixName(&name);
            fixIname(templateId, &iname);
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
            if (FAILED(m_symbolGroup->debugSymbolGroup()->GetSymbolName(ULONG(symbolGroupIndex), buf, BufSize, &obtainedSize)))
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
            SymbolGroupNode *child = new SymbolGroupNode(m_symbolGroup,
                                                         ULONG(symbolGroupIndex),
                                                         names.at(nameIndex),
                                                         inames.at(nameIndex), this);
            child->parseParameters(symbolGroupIndex, parameterOffset, vec);
            m_children.push_back(child);
            nameIndex++;
        }
    }
    if (isTopLevel)
        m_parameters.SubElements = ULONG(m_children.size());
}

SymbolGroupNode *SymbolGroupNode::create(SymbolGroup *sg, const std::string &name, const SymbolGroup::SymbolParameterVector &vec)
{
    SymbolGroupNode *rc = new SymbolGroupNode(sg, DEBUG_ANY_ID, name, name);
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
    // Fix long class names on std containers 'class std::tree<...>' -> 'class std::tree<>'
    if (value->compare(0, 6, L"class ") == 0) {
        const std::string::size_type openTemplate = value->find(L'<');
        if (openTemplate != std::string::npos) {
            value->erase(openTemplate + 1, value->size() - openTemplate - 2);
            return;
        }
    }
}

// Check for ASCII-encode-able stuff. Plain characters + tabs at the most, no newline.
static bool isSevenBitClean(const wchar_t *buf, size_t size)
{
    const wchar_t *bufEnd = buf + size;
    for (const wchar_t *bufPtr = buf; bufPtr < bufEnd; bufPtr++) {
        const wchar_t c = *bufPtr;
        if (c > 127 || (c < 32 && c != 9))
            return false;
    }
    return true;
}

std::string SymbolGroupNode::type() const
{
    static char buf[BufSize];
    const HRESULT hr = m_symbolGroup->debugSymbolGroup()->GetSymbolTypeName(m_index, buf, BufSize, NULL);
    return SUCCEEDED(hr) ? std::string(buf) : std::string();
}

ULONG64 SymbolGroupNode::address() const
{
    ULONG64 address = 0;
    const HRESULT hr = m_symbolGroup->debugSymbolGroup()->GetSymbolOffset(m_index, &address);
    if (SUCCEEDED(hr))
        return address;
    return 0;
}

std::wstring SymbolGroupNode::symbolGroupRawValue() const
{
    // Determine size and return allocated buffer
    const ULONG maxValueSize = 262144;
    ULONG obtainedSize = 0;
    HRESULT hr = m_symbolGroup->debugSymbolGroup()->GetSymbolValueTextWide(m_index, NULL, maxValueSize, &obtainedSize);
    if (FAILED(hr))
        return std::wstring();
    if (obtainedSize > maxValueSize)
        obtainedSize = maxValueSize;
    wchar_t *buffer = new wchar_t[obtainedSize];
    hr = m_symbolGroup->debugSymbolGroup()->GetSymbolValueTextWide(m_index, buffer, obtainedSize, &obtainedSize);
    if (FAILED(hr)) // Whoops, should not happen
        buffer[0] = 0;
    const std::wstring rc(buffer);
    delete [] buffer;
    return rc;
}

std::wstring SymbolGroupNode::symbolGroupFixedValue() const
{
    std::wstring value = symbolGroupRawValue();
    fixValue(type(), &value);
    return value;
}

// Value to be reported to debugger
std::wstring SymbolGroupNode::displayValue(const SymbolGroupValueContext &ctx)
{
    if (m_flags & Uninitialized)
        return L"<not in scope>";
    if ((m_flags & DumperMask) == 0)
        m_flags |= dumpSimpleType(this , ctx, &m_dumperValue);
    if (m_flags & DumperOk)
        return m_dumperValue;
    return symbolGroupFixedValue();
}

SymbolGroupNode *SymbolGroupNode::childAt(unsigned i) const
{
    return i < m_children.size() ? m_children.at(i) : static_cast<SymbolGroupNode *>(0);
}

unsigned SymbolGroupNode::indexByIName(const char *n) const
{
    const VectorIndexType size = m_children.size();
    for (VectorIndexType i = 0; i < size; i++)
        if ( m_children.at(i)->iName() == n )
            return unsigned(i);
    return unsigned(-1);
}

SymbolGroupNode *SymbolGroupNode::childByIName(const char *n) const
{
    const unsigned index = indexByIName(n);
    if (index != unsigned(-1))
        return m_children.at(index);
    return 0;
}

static inline void indentStream(std::ostream &str, unsigned depth)
{
    for (unsigned d = 0; d < depth; d++)
        str << "  ";
}

void SymbolGroupNode::dump(std::ostream &str, const SymbolGroupValueContext &ctx)
{
    const std::string iname = fullIName();
    const std::string t = type();

    str << "iname=\"" << iname << "\",exp=\"" << iname << "\",name=\"" << m_name
        << "\",type=\"" << t << '"';

    if (const ULONG64 addr = address())
        str << ",addr=\"" << std::hex << std::showbase << addr << std::noshowbase << std::dec
               << '"';

    const bool uninitialized = m_flags & Uninitialized;
    bool valueEditable = !uninitialized;
    bool valueEnabled = !uninitialized;

    const std::wstring value = displayValue(ctx);
    // ASCII or base64?
    if (isSevenBitClean(value.c_str(), value.size())) {
        str << ",valueencoded=\"0\",value=\"" << gdbmiWStringFormat(value) << '"';
    } else {
        str << ",valueencoded=\"2\",value=\"";
        base64Encode(str, reinterpret_cast<const unsigned char *>(value.c_str()), value.size() * sizeof(wchar_t));
        str << '"';
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
}

bool SymbolGroupNode::accept(SymbolGroupNodeVisitor &visitor, unsigned child, unsigned depth)
{
    // If we happen to be the root node, just skip over
    const bool invisibleRoot = m_index == DEBUG_ANY_ID;
    const unsigned childDepth = invisibleRoot ? 0 : depth + 1;

    const SymbolGroupNodeVisitor::VisitResult vr =
            invisibleRoot ? SymbolGroupNodeVisitor::VisitContinue :
                            visitor.visit(this, child, depth);
    switch (vr) {
    case SymbolGroupNodeVisitor::VisitStop:
        return true;
    case SymbolGroupNodeVisitor::VisitSkipChildren:
        break;
    case SymbolGroupNodeVisitor::VisitContinue: {
        const unsigned childCount = unsigned(m_children.size());
        for (unsigned c = 0; c < childCount; c++)
            if (m_children.at(c)->accept(visitor, c, childDepth))
                return true;
        if (!invisibleRoot)
            visitor.childrenVisited(this, depth);
    }
        break;
    }
    return false;
}

void SymbolGroupNode::debug(std::ostream &str, unsigned verbosity, unsigned depth) const
{
    indentStream(str, depth);
    str << '"' << fullIName() << "\",index=" << m_index;
    if (const VectorIndexType childCount = m_children.size())
        str << ", Children=" << childCount;
    str << ' ' << m_parameters;
    if (m_flags) {
        str << " node-flags=" << m_flags;
        if (m_flags & Uninitialized)
            str << " UNINITIALIZED";
        if (m_flags & DumperNotApplicable)
            str << " DumperNotApplicable";
        if (m_flags & DumperOk)
            str << " DumperOk";
        if (m_flags & DumperFailed)
            str << " DumperFailed";
        if (m_flags & ExpandedByDumper)
            str << " ExpandedByDumper";
        str << ' ';
    }
    if (verbosity) {
        str << ",name=\"" << m_name << "\", Address=0x" << std::hex << address() << std::dec
            << " Type=\"" << type() << '"';
        if (!(m_flags & Uninitialized))
            str << "\" Value=\"" << gdbmiWStringFormat(symbolGroupRawValue()) << '"';
    }
    str << '\n';
}

// Expand!
bool SymbolGroupNode::expand(std::string *errorMessage)
{
    if (::debug > 1)
        DebugPrint() << "SymbolGroupNode::expand "  << m_name << ' ' << m_index;
    if (isExpanded()) {
        // Clear the flag indication dumper expansion on a second, explicit request
        clearFlags(ExpandedByDumper);
        return true;
    }
    if (!canExpand()) {
        *errorMessage = "No subelements to expand in node: " + fullIName();
        return false;
    }
    if (m_flags & Uninitialized) {
        *errorMessage = "Refusing to expand uninitialized node: " + fullIName();
        return false;
    }

    const HRESULT hr = m_symbolGroup->debugSymbolGroup()->ExpandSymbol(m_index, TRUE);

    if (FAILED(hr)) {
        *errorMessage = msgDebugEngineComFailed("ExpandSymbol", hr);
        return false;
    }
    SymbolGroup::SymbolParameterVector parameters;
    // Retrieve parameters (including self, re-retrieve symbol parameters to get new 'expanded' flag
    // and corrected SubElement count (might be estimate))
    if (!SymbolGroup::getSymbolParameters(m_symbolGroup->debugSymbolGroup(),
                                          m_index, m_parameters.SubElements + 1,
                                          &parameters, errorMessage))
        return false;
    // Before inserting children, correct indexes on whole group
    m_symbolGroup->root()->notifyExpanded(m_index + 1, parameters.at(0).SubElements);
    // Parse parameters, correct our own) and create child nodes.
    parseParameters(m_index, m_index, parameters);
    return true;
}

static inline std::string msgNotFound(const std::string &nodeName)
{
    std::ostringstream str;
    str << "Node '" << nodeName << "' not found.";
    return str.str();
}

std::string SymbolGroup::dump(const SymbolGroupValueContext &ctx, bool humanReadable) const
{
    std::ostringstream str;
    DumpSymbolGroupNodeVisitor visitor(str, ctx, humanReadable);
    if (humanReadable)
        str << '\n';
    str << '[';
    accept(visitor);
    str << ']';
    return str.str();
}

// Dump a node, potentially expand
std::string SymbolGroup::dump(const std::string &iname, const SymbolGroupValueContext &ctx, bool humanReadable, std::string *errorMessage)
{
    SymbolGroupNode *const node = find(iname);
    if (node == 0) {
        *errorMessage = msgNotFound(iname);
        return std::string();
    }
    if (node->isExpanded()) { // Mark expand request by watch model
        node->clearFlags(SymbolGroupNode::ExpandedByDumper);
    } else {
        if (node->canExpand() && !node->expand(errorMessage))
            return false;
    }
    std::ostringstream str;
    if (humanReadable)
        str << '\n';
    DumpSymbolGroupNodeVisitor visitor(str, ctx, humanReadable);
    str << '[';
    node->accept(visitor, 0, 0);
    str << ']';
    return str.str();
}

std::string SymbolGroup::debug(const std::string &iname, unsigned verbosity) const
{
    std::ostringstream str;
    str << '\n';
    DebugSymbolGroupNodeVisitor visitor(str, verbosity);
    if (iname.empty()) {
        accept(visitor);
    } else {
        if (SymbolGroupNode *const node = find(iname)) {
            node->accept(visitor, 0, 0);
        } else {
            str << msgNotFound(iname);
        }
    }
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
    SymbolGroupNode *node = find(nodeName);
    if (::debug)
        DebugPrint() << "expand: " << nodeName << " found=" << (node != 0) << '\n';
    if (!node) {
        *errorMessage = msgNotFound(nodeName);
        return false;
    }
    if (node == m_root) // Shouldn't happen, still, all happy
        return true;
    return node->expand(errorMessage);
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
                (*it)->addFlags(SymbolGroupNode::Uninitialized);
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
    SymbolGroupNode *node = find(nodeName);
    if (node == 0) {
        *errorMessage = msgAssignError(nodeName, value, "No such node");
        return false;
    }
    const HRESULT hr = m_symbolGroup->WriteSymbol(node->index(), const_cast<char *>(value.c_str()));
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
    return m_root->accept(visitor, 0, 0);
}

// Find  "locals.this.i1" and move index recursively
static SymbolGroupNode *findNodeRecursion(const std::vector<std::string> &iname,
                                          unsigned depth,
                                          std::vector<SymbolGroupNode *> nodes)
{
    typedef std::vector<SymbolGroupNode *>::const_iterator ConstIt;

    if (debug > 1) {
        DebugPrint() <<"findNodeRecursion: Looking for " << iname.back() << " (" << iname.size()
           << "),depth="  << depth << ",matching=" << iname.at(depth) << " in " << nodes.size();
    }

    if (nodes.empty())
        return 0;
    // Find the child that matches the iname part at depth
    const ConstIt cend = nodes.end();
    for (ConstIt it = nodes.begin(); it != cend; ++it) {
        SymbolGroupNode *c = *it;
        if (c->iName() == iname.at(depth)) {
            if (depth == iname.size() - 1) { // Complete iname matched->happy.
                return c;
            } else {
                // Sub-part of iname matched. Forward index and check children.
                return findNodeRecursion(iname, depth + 1, c->children());
            }
        }
    }
    return 0;
}

SymbolGroupNode *SymbolGroup::findI(const std::string &iname) const
{
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
    return findNodeRecursion(inameTokens, 1, m_root->children());
}

SymbolGroupNode *SymbolGroup::find(const std::string &iname) const
{
    SymbolGroupNode *rc = findI(iname);
    if (::debug > 1)
        DebugPrint() << "SymbolGroup::find " << iname << ' ' << rc;
    return rc;
}

// --------- DebugSymbolGroupNodeVisitor
DebugSymbolGroupNodeVisitor::DebugSymbolGroupNodeVisitor(std::ostream &os, unsigned verbosity) :
    m_os(os), m_verbosity(verbosity)
{
}

SymbolGroupNodeVisitor::VisitResult
    DebugSymbolGroupNodeVisitor::visit(SymbolGroupNode *node,
                                       unsigned /* child */, unsigned depth)
{
    node->debug(m_os, m_verbosity, depth);
    return VisitContinue;
}

// --------------------- DumpSymbolGroupNodeVisitor
DumpSymbolGroupNodeVisitor::DumpSymbolGroupNodeVisitor(std::ostream &os,
                                                       const SymbolGroupValueContext &context,
                                                       bool humanReadable) :
    m_os(os), m_humanReadable(humanReadable),m_context(context), m_visitChildren(false)
{
}

SymbolGroupNodeVisitor::VisitResult
    DumpSymbolGroupNodeVisitor::visit(SymbolGroupNode *node, unsigned child, unsigned depth)
{
    // Recurse to children if expanded by explicit watchmodel request
    // and initialized.
    const unsigned flags = node->flags();
    m_visitChildren = node->isExpanded()
            && (flags & (SymbolGroupNode::Uninitialized|SymbolGroupNode::ExpandedByDumper)) == 0;

    // Do not recurse into children unless the node was expanded by the watch model
    if (child)
        m_os << ','; // Separator in parents list
    if (m_humanReadable) {
        m_os << '\n';
        indentStream(m_os, depth * 2);
    }
    m_os << '{';
    node->dump(m_os, m_context);
    if (m_visitChildren) { // open children array
        m_os << ",children=[";
    } else {               // No children, close array.
        m_os << '}';
    }
    if (m_humanReadable)
        m_os << '\n';
    return m_visitChildren ? VisitContinue : VisitSkipChildren;
}

void DumpSymbolGroupNodeVisitor::childrenVisited(const SymbolGroupNode *, unsigned)
{
    m_os << "]}"; // Close children array and self
    if (m_humanReadable)
        m_os << '\n';
}
