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
#include "containers.h"

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

// --------------- DumpParameters
DumpParameters::DumpParameters() : dumpFlags(0)
{
}

// typeformats: decode hex-encoded name, value pairs:
// '414A=2,...' -> map of "AB:2".
DumpParameters::FormatMap DumpParameters::decodeFormatArgument(const std::string &f)
{
    FormatMap rc;
    const std::string::size_type size = f.size();
    // Split 'hexname=4,'
    for (std::string::size_type pos = 0; pos < size ; ) {
        // Cut out key
        const std::string::size_type equalsPos = f.find('=', pos);
        if (equalsPos == std::string::npos)
            return rc;
        const std::string name = stringFromHex(f.c_str() + pos, f.c_str() + equalsPos);
        // Search for number
        const std::string::size_type numberPos = equalsPos + 1;
        std::string::size_type nextPos = f.find(',', numberPos);
        if (nextPos == std::string::npos)
            nextPos = size;
        int format;
        if (!integerFromString(f.substr(numberPos, nextPos - numberPos), &format))
            return rc;
        rc.insert(FormatMap::value_type(name, format));
        pos = nextPos + 1;
    }
    return rc;
}

int DumpParameters::format(const std::string &type, const std::string &iname) const
{
    if (!individualFormats.empty()) {
        const FormatMap::const_iterator iit = individualFormats.find(iname);
        if (iit != individualFormats.end())
            return iit->second;
    }
    if (!typeFormats.empty()) {
        const FormatMap::const_iterator tit = typeFormats.find(type);
        if (tit != typeFormats.end())
            return tit->second;
    }
    return -1;
}

enum PointerFormats // Watch data pointer format requests
{
    FormatAuto = 0,
    FormatLatin1String = 1,
    FormatUtf8String = 2,
    FormatUtf16String = 3,
    FormatUcs4String = 4
};

enum DumpEncoding // WatchData encoding of GDBMI values
{
    DumpEncodingAscii = 0,
    DumpEncodingBase64 = 1,
    DumpEncodingBase64_Utf16 = 2,
    DumpEncodingBase64_Ucs4 = 3,
    DumpEncodingHex_Latin1 = 6,
    DumpEncodingHex_Utf16 = 7,
    DumpEncodingHex_Ucs4_LittleEndian = 8,
    DumpEncodingHex_Utf8_LittleEndian = 9,
    DumpEncodingHex_Ucs4_BigEndian = 10,
    DumpEncodingHex_Utf16_BigEndian = 11,
    DumpEncodingHex_Utf16_LittleEndian = 12
};

/* Recode arrays/pointers of char*, wchar_t according to users
 * sepcification. Handles char formats for 'char *', '0x834478 "hallo.."'
 * and 'wchar_t *', '0x834478 "hallo.."'.
 * This is done by retrieving the address and the length (in characters)
 * of the CDB output, converting it to memory size, fetching the data
 * from memory, zero-terminating and recoding it using the encoding
 * defined in watchutils.cpp.
 * As a special case, if there is no user-defined format and the
 * CDB output contains '?' indicating non-printable characters,
 * append a hex dump of the memory (auto-format). */

bool DumpParameters::recode(const std::string &type,
                            const std::string &iname,
                            const SymbolGroupValueContext &ctx,
                            std::wstring *value, int *encoding) const
{
    // We basically handle char formats for 'char *', '0x834478 "hallo.."'
    // and 'wchar_t *', '0x834478 "hallo.."'
    // Determine address and length from the pointer value output,
    // read the raw memory and recode if that is possible.
    if (type.empty() || type.at(type.size() - 1) != '*')
        return false;
    const int newFormat = format(type, iname);
    if (value->compare(0, 2, L"0x"))
        return false;
    const std::wstring::size_type quote1 = value->find(L'"', 2);
    if (quote1 == std::wstring::npos)
        return false;
    // The user did not specify any format, still, there are '?'
    // (indicating non-printable) in what the debugger prints. In that case,
    // append a hex dump to the normal output. If there are no '?'-> all happy.
    if (newFormat < FormatLatin1String && value->find(L'?', quote1 + 1) == std::wstring::npos)
        return false;
    const std::wstring::size_type quote2 = value->find(L'"', quote1 + 1);
    if (quote2 == std::wstring::npos)
        return false;
    std::wstring::size_type length = quote2 - quote1 - 1;
    if (!length)
        return false;
    // Get address from value
    ULONG64 address = 0;
    if (!integerFromWString(value->substr(0, quote1 - 1), &address) || !address)
        return false;
    // Get real size if this is for example a wchar_t *.
    const unsigned elementSize = SymbolGroupValue::sizeOf(SymbolGroupValue::stripPointerType(type).c_str());
    if (!elementSize)
        return false;
    length *= elementSize;
    // Allocate real length + 8 bytes ('\0') for largest format (Ucs4).
    // '\0' is not listed in the CDB output.
    const std::wstring::size_type allocLength = length + 8;
    unsigned char *buffer = new unsigned char[allocLength];
    std::fill(buffer, buffer + allocLength, 0);
    ULONG obtained = 0;
    if (FAILED(ctx.dataspaces->ReadVirtual(address, buffer, ULONG(length), &obtained))) {
        delete [] buffer;
        return false;
    }
    // Recode raw memory
    switch (newFormat) {
    case FormatLatin1String:
        *value = dataToHexW(buffer, buffer + length + 1); // Latin1 + 0
        *encoding = DumpEncodingHex_Latin1;
        break;
    case FormatUtf8String:
        *value = dataToHexW(buffer, buffer + length + 1); // UTF8 + 0
        *encoding = DumpEncodingHex_Utf8_LittleEndian;
        break;
    case FormatUtf16String: // Paranoia: make sure buffer is terminated at 2 byte borders
        if (length % 2) {
            length &= ~1;
            buffer[length] = '\0';
            buffer[length + 1] = '\0';
        }
        *value = base64EncodeToWString(buffer, length + 2);
        *encoding = DumpEncodingBase64_Utf16;
        break;
    case FormatUcs4String: // Paranoia: make sure buffer is terminated at 4 byte borders
        if (length % 4) {
            length &= ~3;
            std::fill(buffer + length, buffer + length + 4, 0);
        }
        *value = dataToHexW(buffer, buffer + length + 2); // UTF16 + 0
        *encoding = DumpEncodingHex_Ucs4_LittleEndian;
        break;
    default:  // See above, append hex dump
        value->push_back(' ');
        value->append(dataToReadableHexW(buffer, buffer + length));
        *encoding = DumpEncodingAscii;
        break;
    }
    delete [] buffer;
    return true;
}

// ------------- SymbolGroup
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
    m_symbolGroup(symbolGroup), m_parent(parent),
    m_index(index), m_referencedBy(0),
    m_name(name), m_iname(iname), m_flags(0), m_dumperType(-1),
    m_dumperContainerSize(-1)
{
    memset(&m_parameters, 0, sizeof(DEBUG_SYMBOL_PARAMETERS));
    m_parameters.ParentSymbol = DEBUG_ANY_ID;
}

void SymbolGroupNode::removeChildren()
{
    if (!m_children.empty()) {
        const SymbolGroupNodePtrVectorIterator end = m_children.end();
        for (SymbolGroupNodePtrVectorIterator it = m_children.begin(); it != end; ++it) {
            SymbolGroupNode *child = *it;
            if (child->parent() == this) // Do not delete references
                delete child;
        }
        m_children.clear();
    }
}

void SymbolGroupNode::setReferencedBy(SymbolGroupNode *n)
{
    if (m_referencedBy)
        dprintf("Internal error: Node %s Clearing reference by %s",
                name().c_str(), m_referencedBy->name().c_str());
    m_referencedBy = n;
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
    for (ReverseIt it = m_children.rbegin(); it != rend; ++it) {
        SymbolGroupNode *c = *it;
        if (c->parent() == this) // Skip fake children that are referenced only
            if (!(*it)->notifyExpanded(index, insertedCount))
                return false;
    }

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
    for (const SymbolGroupNode *p = referencedParent(); p; p = p->referencedParent()) {
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

static inline bool isHexDigit(wchar_t c)
{
    return (c >= L'0' && c <= L'9') || (c >= L'a' && c <= L'f') || (c >= L'A' && c <= L'F');
}

static void fixValue(const std::string &type, std::wstring *value)
{
    // Pointers/Unsigned integers: fix '0x00000000`00000AD bla' ... to "0xAD bla"
    const bool isHexNumber = value->size() > 3 && value->compare(0, 2, L"0x") == 0 && isHexDigit(value->at(2));
    if (isHexNumber) {
        // Remove dumb 64bit separator
        if (value->size() > 10 && value->at(10) == L'`')
            value->erase(10, 1);
        const std::string::size_type firstNonNullDigit = value->find_first_not_of(L"0", 2);
        // No on-null digits: plain null ptr.
        if (firstNonNullDigit == std::string::npos || value->at(firstNonNullDigit) == ' ') {
            *value = L"0x0";
        } else {
        // Strip
            if (firstNonNullDigit > 2)
                value->erase(2, firstNonNullDigit - 2);
        }
    }

    // Pointers: fix '0x00000000`00000AD class bla' ... to "0xAD", but leave
    // 'const char *' values as is ('0x00000000`00000AD "hallo").
    if (!type.empty() && type.at(type.size() - 1) == L'*') {
        // Strip ' Class bla"
        std::wstring::size_type classPos = value->find(L" struct", 2);
        if (classPos == std::string::npos)
            classPos = value->find(L" class", 2);
        if (classPos != std::string::npos)
            value->erase(classPos, value->size() - classPos);
        return;
    }

    // unsigned hex ints that are not pointers: Convert to decimal as not to confuse watch model:
    if (isHexNumber) {
        ULONG64 uv;
        std::wistringstream str(*value);
        str >> std::hex >> uv;
        if (!str.fail()) {
            *value = toWString(uv);
            return;
        }
    }

    // Integers: fix '0n10' -> '10'
    if (value->size() >= 3 && value->compare(0, 2, L"0n") == 0
        && (isdigit(value->at(2)) || value->at(2) == L'-')) {
        value->erase(0, 2);
        return;
    }
    // Fix long class names on std containers 'class std::tree<...>' -> 'class std::tree<>'
    if (value->compare(0, 6, L"class ") == 0 || value->compare(0, 7, L"struct ") == 0) {
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

unsigned SymbolGroupNode::size() const
{
    DEBUG_SYMBOL_ENTRY entry;
    if (SUCCEEDED(m_symbolGroup->debugSymbolGroup()->GetSymbolEntryInformation(m_index, &entry)))
        return entry.Size;
    return 0;
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

// Complex dumpers: Get container/fake children
void SymbolGroupNode::runComplexDumpers(const SymbolGroupValueContext &ctx)
{
    if (m_dumperContainerSize <= 0 || (m_flags & ComplexDumperOk) || !(m_flags & SimpleDumperOk))
        return;
    m_flags |= ComplexDumperOk;
    const SymbolGroupNodePtrVector children =
            containerChildren(this, m_dumperType, m_dumperContainerSize, ctx);
    m_dumperContainerSize = int(children.size()); // Just in case...
    if (children.empty())
        return;

    clearFlags(ExpandedByDumper);
    // Mark current children as obscured. We cannot show both currently
    // as this would upset the numerical sorting of the watch model
    SymbolGroupNodePtrVectorConstIterator cend = m_children.end();
    for (SymbolGroupNodePtrVectorConstIterator it = m_children.begin(); it != cend; ++it)
        (*it)->addFlags(Obscured);
    // Add children and mark them as referenced by us.
    cend = children.end();
    for (SymbolGroupNodePtrVectorConstIterator it = children.begin(); it != cend; ++it) {
        SymbolGroupNode *c = *it;
        c->setReferencedBy(this);
        m_children.push_back(c);
    }
}

// Run dumpers, format simple in-line dumper value and retrieve fake children
std::wstring SymbolGroupNode::simpleDumpValue(const SymbolGroupValueContext &ctx,
                                        const DumpParameters &)
{
    if (m_flags & Uninitialized)
        return L"<not in scope>";
    if ((m_flags & SimpleDumperMask) == 0) {
        m_flags |= dumpSimpleType(this , ctx, &m_dumperValue,
                                  &m_dumperType, &m_dumperContainerSize);
        if (m_flags & SimpleDumperOk)
            return m_dumperValue;
    }
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

void SymbolGroupNode::dump(std::ostream &str,
                           const DumpParameters &p,
                           const SymbolGroupValueContext &ctx)
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

    // Shall it be recoded?
    std::wstring value = simpleDumpValue(ctx, p);
    int encoding = 0;
    if (p.recode(t, iname, ctx, &value, &encoding)) {
        str << ",valueencoded=\"" << encoding
            << "\",value=\"" << gdbmiWStringFormat(value) <<'"';
    } else { // As is: ASCII or base64?
        if (isSevenBitClean(value.c_str(), value.size())) {
            str << ",valueencoded=\"" << DumpEncodingAscii << "\",value=\""
                << gdbmiWStringFormat(value) << '"';
        } else {
            str << ",valueencoded=\"" << DumpEncodingBase64_Utf16 << "\",value=\"";
            base64Encode(str, reinterpret_cast<const unsigned char *>(value.c_str()), value.size() * sizeof(wchar_t));
            str << '"';
        }
    }
    // Children: Dump all known non-obscured or subelements
    unsigned childCountGuess = 0;
    if (!uninitialized) {
        if (m_dumperContainerSize > 0) {
            childCountGuess = m_dumperContainerSize; // See Obscured handling
        } else {
            if (m_children.empty()) {
                childCountGuess = m_parameters.SubElements; // Guess
            } else {
                childCountGuess = unsigned(m_children.size());
            }
        }
    }
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
    if (m_referencedBy)
        str << ",referenced by \"" << m_referencedBy->fullIName() << '"';
    if (const VectorIndexType childCount = m_children.size())
        str << ", Children=" << childCount;
    str << ' ' << m_parameters;
    if (m_flags) {
        str << " node-flags=" << m_flags;
        if (m_flags & Uninitialized)
            str << " UNINITIALIZED";
        if (m_flags & SimpleDumperNotApplicable)
            str << " DumperNotApplicable";
        if (m_flags & SimpleDumperOk)
            str << " DumperOk";
        if (m_flags & SimpleDumperFailed)
            str << " DumperFailed";
        if (m_flags & ExpandedByDumper)
            str << " ExpandedByDumper";
        if (m_flags & AdditionalSymbol)
            str << " AdditionalSymbol";
        if (m_flags & Obscured)
            str << " Obscured";
        if (m_flags & ComplexDumperOk)
            str << " ComplexDumperOk";
        str << ' ';
    }
    if (verbosity) {
        str << ",name=\"" << m_name << "\", Address=0x" << std::hex << address() << std::dec
            << " Type=\"" << type() << '"';
        if (m_dumperType >= 0) {
            str << " ,dumperType=" << m_dumperType;
            if (m_dumperType & KT_Qt_Type)
                str << " qt";
            if (m_dumperType & KT_STL_Type)
                str << " STL";
            if (m_dumperType & KT_ContainerType)
                str << " container(" << m_dumperContainerSize << ')';
        }
        if (!(m_flags & Uninitialized))
            str << " Value=\"" << gdbmiWStringFormat(symbolGroupRawValue()) << '"';
    }
    str << '\n';
}

static inline std::string msgCannotCast(const std::string &nodeName,
                                        const std::string &fromType,
                                        const std::string &toType,
                                        const std::string &why)
{
    std::ostringstream str;
    str << "Cannot cast node '" << nodeName << "' from '" << fromType
        << "' to '" << toType << "': " << why;
    return str.str();
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

bool SymbolGroupNode::typeCast(const std::string &desiredType, std::string *errorMessage)
{
    const std::string fromType = type();
    if (fromType == desiredType)
        return true;
    if (isExpanded()) {
        *errorMessage = msgCannotCast(fullIName(), fromType, desiredType, "Already expanded");
        return false;
    }
    HRESULT hr = m_symbolGroup->debugSymbolGroup()->OutputAsType(m_index, desiredType.c_str());
    if (FAILED(hr)) {
        *errorMessage = msgCannotCast(fullIName(), fromType, desiredType, msgDebugEngineComFailed("OutputAsType", hr));
        return false;
    }
    hr = m_symbolGroup->debugSymbolGroup()->GetSymbolParameters(m_index, 1, &m_parameters);
    if (FAILED(hr)) { // Should never fail
        *errorMessage = msgCannotCast(fullIName(), fromType, desiredType, msgDebugEngineComFailed("GetSymbolParameters", hr));
        return false;
    }
    return true;
}

static inline std::string msgCannotAddSymbol(const std::string &name, const std::string &why)
{
    std::ostringstream str;
    str << "Cannot add symbol '" << name << "': " << why;
    return str.str();
}

// For root nodes, only: Add a new symbol by name
SymbolGroupNode *SymbolGroupNode::addSymbolByName(const std::string &name,
                                                  const std::string &iname,
                                                  std::string *errorMessage)
{
    ULONG index = DEBUG_ANY_ID; // Append
    HRESULT hr = m_symbolGroup->debugSymbolGroup()->AddSymbol(name.c_str(), &index);
    if (FAILED(hr)) {
        *errorMessage = msgCannotAddSymbol(name, msgDebugEngineComFailed("AddSymbol", hr));
        return 0;
    }
    SymbolParameterVector parameters(1, DEBUG_SYMBOL_PARAMETERS());
    hr = m_symbolGroup->debugSymbolGroup()->GetSymbolParameters(index, 1, &(*parameters.begin()));
    if (FAILED(hr)) { // Should never fail
        *errorMessage = msgCannotAddSymbol(name, msgDebugEngineComFailed("GetSymbolParameters", hr));
        return 0;
    }
    // Paranoia: Check for cuckoo's eggs (which should not happen)
    if (parameters.front().ParentSymbol != m_index) {
        *errorMessage = msgCannotAddSymbol(name, "Parent id mismatch");
        return 0;
    }
    SymbolGroupNode *node = new SymbolGroupNode(m_symbolGroup, index,
                                                name, iname.empty() ? name : iname,
                                                this);
    node->parseParameters(0, 0, parameters);
    node->addFlags(AdditionalSymbol);
    m_children.push_back(node);
    return node;
}

static inline std::string msgNotFound(const std::string &nodeName)
{
    std::ostringstream str;
    str << "Node '" << nodeName << "' not found.";
    return str.str();
}

std::string SymbolGroup::dump(const SymbolGroupValueContext &ctx,
                              const DumpParameters &p) const
{
    std::ostringstream str;
    DumpSymbolGroupNodeVisitor visitor(str, ctx, p);
    if (p.humanReadable())
        str << '\n';
    str << '[';
    accept(visitor);
    str << ']';
    return str.str();
}

// Dump a node, potentially expand
std::string SymbolGroup::dump(const std::string &iname,
                              const SymbolGroupValueContext &ctx,
                              const DumpParameters &p,
                              std::string *errorMessage)
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
    // After expansion, run the complex dumpers
    if (p.dumpFlags & DumpParameters::DumpComplexDumpers)
        node->runComplexDumpers(ctx);
    std::ostringstream str;
    if (p.humanReadable())
        str << '\n';
    DumpSymbolGroupNodeVisitor visitor(str, ctx, p);
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

// Cast an (unexpanded) node
bool SymbolGroup::typeCast(const std::string &iname, const std::string &desiredType, std::string *errorMessage)
{
    SymbolGroupNode *node = find(iname);
    if (!node) {
        *errorMessage = msgNotFound(iname);
        return false;
    }
    if (node == m_root) {
        *errorMessage = "Cannot cast root node";
        return false;
    }
    return node->typeCast(desiredType, errorMessage);
}

SymbolGroupNode *SymbolGroup::addSymbol(const std::string &name, const std::string &iname, std::string *errorMessage)
{
    return m_root->addSymbolByName(name, iname, errorMessage);
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
                                                       const DumpParameters &parameters) :
    m_os(os), m_context(context), m_parameters(parameters),
    m_visitChildren(false),m_lastDepth(unsigned(-1))
{
}

SymbolGroupNodeVisitor::VisitResult
    DumpSymbolGroupNodeVisitor::visit(SymbolGroupNode *node, unsigned /* child */, unsigned depth)
{
    // Show container children only
    if (node->flags() & SymbolGroupNode::Obscured)
        return VisitSkipChildren;
    // Recurse to children only if expanded by explicit watchmodel request
    // and initialized.
    const unsigned flags = node->flags();
    m_visitChildren = node->isExpanded()
            && (flags & (SymbolGroupNode::Uninitialized|SymbolGroupNode::ExpandedByDumper)) == 0;

    // Comma between same level children given obscured children
    if (depth == m_lastDepth) {
        m_os << ',';
    } else {
        m_lastDepth = depth;
    }
    if (m_parameters.humanReadable()) {
        m_os << '\n';
        indentStream(m_os, depth * 2);
    }
    m_os << '{';
    node->dump(m_os, m_parameters, m_context);
    if (m_visitChildren) { // open children array
        m_os << ",children=[";
    } else {               // No children, close array.
        m_os << '}';
    }
    if (m_parameters.humanReadable())
        m_os << '\n';
    return m_visitChildren ? VisitContinue : VisitSkipChildren;
}

void DumpSymbolGroupNodeVisitor::childrenVisited(const SymbolGroupNode *n, unsigned)
{
    m_os << "]}"; // Close children array and self
    if (m_parameters.humanReadable())
        m_os << "   /* end of '" << n->fullIName() << "' */\n";
}
