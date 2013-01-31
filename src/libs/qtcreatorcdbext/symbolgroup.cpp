/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "symbolgroup.h"
#include "symbolgroupvalue.h"
#include "stringutils.h"
#include "gdbmihelpers.h"

#include <set>
#include <algorithm>
#include <iterator>
#include <memory>
#include <cctype>

typedef std::vector<int>::size_type VectorIndexType;
typedef std::vector<std::string> StringVector;

enum { debug = 0 };

// ------------- SymbolGroup

/*!
  \class SymbolGroup A symbol group storing a tree of expanded symbols rooted on a fake "locals" root element.

  Provides a find() method based on inames ("locals.this.i1.data") and
  dump() methods used for GDBMI-format dumping and debug helpers.
  Qt Creator's WatchModel is fed from this class. It basically represents the
  symbol group tree with some additional node types (Reference and Map Node
  types.
  \ingroup qtcreatorcdbext
*/

SymbolGroup::SymbolGroup(IDebugSymbolGroup2 *sg,
                         const SymbolParameterVector &vec,
                         const std::string &rootModule,
                         const char *rootName) :
    m_symbolGroup(sg),
    m_root(0)
{
    m_root = SymbolGroupNode::create(this, rootModule, rootName, vec);
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

bool SymbolGroup::removeSymbol(AbstractSymbolGroupNode *an, std::string *errorMessage)
{
    bool ok = false;
    // Remove a real SymbolGroupNode: Call its special remove method to correct indexes.
    if (SymbolGroupNode *n = an->asSymbolGroupNode()) {
        ok = n->removeSelf(root(), errorMessage);
    } else {
        // Normal removal, assuming parent is a base (needed for removing
        // error nodes by the Watch group).
        if (BaseSymbolGroupNode *bParent = dynamic_cast<BaseSymbolGroupNode *>(an->parent())) {
            bParent->removeChildAt(bParent->indexOf(an));
            ok = true;
        }
    }
    if (!ok && errorMessage->empty())
        *errorMessage = "SymbolGroup::removeSymbol: Unimplemented removal operation for " + an->name();

    if (!ok && SymbolGroupValue::verbose)
        DebugPrint() << "SymbolGroup::removeSymbol failed: " << *errorMessage;
    return ok;
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
    QTC_TRACE_IN
    if (symbolGroupDebug)
        DebugPrint() << "<SymbolGroup::dump()";
    std::ostringstream str;
    DumpSymbolGroupNodeVisitor visitor(str, ctx, p);
    if (p.humanReadable())
        str << '\n';
    str << '[';
    accept(visitor);
    str << ']';
    QTC_TRACE_OUT
    if (symbolGroupDebug)
        DebugPrint() << "<SymbolGroup::dump()";
    return str.str();
}

// Dump a node, potentially expand
std::string SymbolGroup::dump(const std::string &iname,
                              const SymbolGroupValueContext &ctx,
                              const DumpParameters &p,
                              std::string *errorMessage)
{
    if (symbolGroupDebug)
        DebugPrint() << ">SymbolGroup::dump(" << iname << ")";
    QTC_TRACE_IN
    AbstractSymbolGroupNode *const aNode = find(iname);
    if (!aNode) {
        *errorMessage = msgNotFound(iname);
        return std::string();
    }

    // Real nodes: Expand and complex dumpers
    if (SymbolGroupNode *node = aNode->resolveReference()->asSymbolGroupNode()) {
        if (symbolGroupDebug)
            DebugPrint() << "SymbolGroup::dump(" << iname << '/'
                         << aNode->absoluteFullIName() <<" resolves to " << node->absoluteFullIName()
                         << " expanded=" << node->isExpanded();
        if (node->isExpanded()) { // Mark expand request by watch model
            node->clearFlags(SymbolGroupNode::ExpandedByDumper);
        } else {
            if (node->canExpand() && !node->expand(errorMessage))
                return std::string();
        }
        // After expansion, run the complex dumpers
        if (p.dumpFlags & DumpParameters::DumpComplexDumpers)
            node->runComplexDumpers(ctx);
        if (symbolGroupDebug)
            DebugPrint() << "SymbolGroup::dump(" << iname << ") ran complex dumpers 0x" << std::hex << node->flags();
    }

    std::ostringstream str;
    if (p.humanReadable())
        str << '\n';
    DumpSymbolGroupNodeVisitor visitor(str, ctx, p);
    str << '[';
    aNode->accept(visitor, SymbolGroupNodeVisitor::parentIname(iname), 0, 0);
    str << ']';
    QTC_TRACE_OUT
    if (symbolGroupDebug)
            DebugPrint() << "<SymbolGroup::dump(" << iname << ')';
    return str.str();
}

std::string SymbolGroup::debug(const std::string &iname,
                               const std::string &filter,
                               unsigned verbosity) const
{
    std::ostringstream str;
    str << '\n';
    std::auto_ptr<DebugSymbolGroupNodeVisitor>
            visitor(filter.empty() ?
            new DebugSymbolGroupNodeVisitor(str, verbosity) :
            new DebugFilterSymbolGroupNodeVisitor(str, filter, verbosity));
    if (iname.empty()) {
        accept(*visitor);
    } else {
        if (AbstractSymbolGroupNode *const node = find(iname))
            node->accept(*visitor, SymbolGroupNodeVisitor::parentIname(iname), 0, 0);
        else
            str << msgNotFound(iname);
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

static inline InamePathEntrySet expandEntrySet(const std::vector<std::string> &nodes, const std::string &root)
{
    InamePathEntrySet pathEntries;
    const std::string::size_type rootSize = root.size();
    const VectorIndexType nodeCount = nodes.size();
    for (VectorIndexType i= 0; i < nodeCount; ++i) {
        const std::string &iname = nodes.at(i); // Silently skip items of another group
        if (iname.size() >= rootSize && iname.compare(0, rootSize, root) == 0) {
            std::string::size_type pos = 0;
            // Split a path 'local.foo' and insert (0,'local'), (1,'local.foo') (see above)
            for (unsigned level = 0; pos < iname.size(); ++level) {
                std::string::size_type dotPos = iname.find(SymbolGroupNodeVisitor::iNamePathSeparator, pos);
                if (dotPos == std::string::npos)
                    dotPos = iname.size();
                pathEntries.insert(InamePathEntry(level, iname.substr(0, dotPos)));
                pos = dotPos + 1;
            }
        }
    }
    if (SymbolGroupValue::verbose) {
        DebugPrint dp;
        dp << "expandEntrySet ";
        const InamePathEntrySet::const_iterator cend = pathEntries.end();
        for (InamePathEntrySet::const_iterator it = pathEntries.begin(); it != cend; ++it)
            dp << it->second << ' ';
    }
    return pathEntries;
}

// Expand a node list "locals.i1,locals.i2"
unsigned SymbolGroup::expandList(const std::vector<std::string> &nodes, std::string *errorMessage)
{
    if (symbolGroupDebug)
        DebugPrint() << ">SymbolGroup::expandList" << nodes.size();
    if (nodes.empty())
        return 0;
    // Create a set with a key <level, name>. Also required for 1 node (see above).
    const InamePathEntrySet pathEntries = expandEntrySet(nodes, root()->iName());
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
    if (symbolGroupDebug)
        DebugPrint() << "<SymbolGroup::expandList returns " << succeeded;
    return succeeded;
}

unsigned SymbolGroup::expandListRunComplexDumpers(const std::vector<std::string> &nodes,
                                     const SymbolGroupValueContext &ctx,
                                     std::string *errorMessage)
{
    if (symbolGroupDebug)
        DebugPrint() << ">SymbolGroup::expandListRunComplexDumpers" << nodes.size();
    QTC_TRACE_IN
    if (nodes.empty())
        return 0;
    // Create a set with a key <level, name>. Also required for 1 node (see above).
    const InamePathEntrySet pathEntries = expandEntrySet(nodes, root()->iName());
    // Now expand going by level.
    unsigned succeeded = 0;
    std::string nodeError;
    InamePathEntrySet::const_iterator cend = pathEntries.end();
    for (InamePathEntrySet::const_iterator it = pathEntries.begin(); it != cend; ++it)
        if (expandRunComplexDumpers(it->second, ctx, &nodeError)) {
            succeeded++;
        } else {
            if (!errorMessage->empty())
                errorMessage->append(", ");
            errorMessage->append(nodeError);
        }
    QTC_TRACE_OUT
    if (symbolGroupDebug)
            DebugPrint() << "<SymbolGroup::expandListRunComplexDumpers returns " << succeeded;
    return succeeded;
}

// Find a node for expansion, skipping reference nodes.
static inline SymbolGroupNode *
    findNodeForExpansion(const SymbolGroup *sg,
                         const std::string &nodeName,
                         std::string *errorMessage)
{
    AbstractSymbolGroupNode *aNode = sg->find(nodeName);
    if (!aNode) {
        *errorMessage = msgNotFound(nodeName);
        return 0;
    }

    SymbolGroupNode *node = aNode->resolveReference()->asSymbolGroupNode();
    if (!node) {
        *errorMessage = "Node type error in expand: " + nodeName;
        return 0;
    }
    return node;
}

bool SymbolGroup::expand(const std::string &nodeName, std::string *errorMessage)
{
    if (SymbolGroupNode *node = findNodeForExpansion(this, nodeName, errorMessage))
        return node == m_root ? true : node->expand(errorMessage);
    return false;
}

bool SymbolGroup::collapse(const std::string &nodeName, std::string *errorMessage)
{
    SymbolGroupNode *node = findNodeForExpansion(this, nodeName, errorMessage);
    if (!node)
        return false;
    return node->collapse(errorMessage);
}

bool SymbolGroup::expandRunComplexDumpers(const std::string &nodeName, const SymbolGroupValueContext &ctx, std::string *errorMessage)
{
    if (SymbolGroupNode *node = findNodeForExpansion(this, nodeName, errorMessage))
        return node == m_root ? true : node->expandRunComplexDumpers(ctx, errorMessage);
    return false;
}

// Cast an (unexpanded) node
bool SymbolGroup::typeCast(const std::string &iname, const std::string &desiredType, std::string *errorMessage)
{
    AbstractSymbolGroupNode *aNode = find(iname);
    if (!aNode) {
        *errorMessage = msgNotFound(iname);
        return false;
    }
    if (aNode == m_root) {
        *errorMessage = "Cannot cast root node";
        return false;
    }
    SymbolGroupNode *node = aNode->asSymbolGroupNode();
    if (!node) {
        *errorMessage = "Node type error in typeCast: " + iname;
        return false;
    }
    return node->typeCast(desiredType, errorMessage);
}

SymbolGroupNode *SymbolGroup::addSymbol(const std::string &module,
                                        const std::string &name,
                                        const std::string &displayName,
                                        const std::string &iname,
                                        std::string *errorMessage)
{
    return m_root->addSymbolByName(module, name, displayName, iname, errorMessage);
}

SymbolGroupNode *SymbolGroup::addSymbol(const std::string &module, const std::string &name,
                                        const std::string &iname, std::string *errorMessage)
{
    return addSymbol(module, name, std::string(), iname, errorMessage);
}

// Mark uninitialized (top level only)
void SymbolGroup::markUninitialized(const std::vector<std::string> &uniniNodes)
{
    if (m_root && !m_root->children().empty() && !uniniNodes.empty()) {
        const std::vector<std::string>::const_iterator unIniNodesBegin = uniniNodes.begin();
        const std::vector<std::string>::const_iterator unIniNodesEnd = uniniNodes.end();

        const AbstractSymbolGroupNodePtrVector::const_iterator childrenEnd = m_root->children().end();
        for (AbstractSymbolGroupNodePtrVector::const_iterator it = m_root->children().begin(); it != childrenEnd; ++it) {
            if (std::find(unIniNodesBegin, unIniNodesEnd, (*it)->absoluteFullIName()) != unIniNodesEnd)
                (*it)->addFlags(SymbolGroupNode::Uninitialized);
        }
    }
}

bool SymbolGroup::assign(const std::string &nodeName,
                         int valueEncoding,
                         const std::string &value,
                         const SymbolGroupValueContext &ctx,
                         std::string *errorMessage)
{
    AbstractSymbolGroupNode *aNode = find(nodeName);
    if (aNode == 0) {
        *errorMessage = SymbolGroupNode::msgAssignError(nodeName, value, "No such node");
        return false;
    }
    SymbolGroupNode *node = aNode->resolveReference()->asSymbolGroupNode();
    if (node == 0) {
        *errorMessage = SymbolGroupNode::msgAssignError(nodeName, value, "Invalid node type");
        return false;
    }

    return (node->dumperType() & KT_Editable) ? // Edit complex types
        assignType(node, valueEncoding, value, ctx, errorMessage) :
        node->assign(value, errorMessage);
}

bool SymbolGroup::accept(SymbolGroupNodeVisitor &visitor) const
{
    if (!m_root || m_root->children().empty())
        return false;
    return m_root->accept(visitor, std::string(), 0, 0);
}

// Find  "locals.this.i1" and move index recursively
static AbstractSymbolGroupNode *findNodeRecursion(const std::vector<std::string> &iname,
                                                  unsigned depth,
                                                  std::vector<AbstractSymbolGroupNode *> nodes)
{
    typedef std::vector<AbstractSymbolGroupNode *>::const_iterator ConstIt;

    if (debug > 1) {
        DebugPrint() <<"findNodeRecursion: Looking for " << iname.back() << " (" << iname.size()
           << "),depth="  << depth << ",matching=" << iname.at(depth) << " in " << nodes.size();
    }

    if (nodes.empty())
        return 0;
    // Find the child that matches the iname part at depth
    const ConstIt cend = nodes.end();
    for (ConstIt it = nodes.begin(); it != cend; ++it) {
        AbstractSymbolGroupNode *c = *it;
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

AbstractSymbolGroupNode *SymbolGroup::findI(const std::string &iname) const
{
    if (iname.empty())
        return 0;
    // Match the root element only: Shouldn't happen, still, all happy
    if (iname == m_root->name())
        return m_root;

    std::vector<std::string> inameTokens;
    split(iname, SymbolGroupNodeVisitor::iNamePathSeparator, std::back_inserter(inameTokens));

    // Must begin with root
    if (inameTokens.front() != m_root->name())
        return 0;

    // Start with index = 0 at root's children
    return findNodeRecursion(inameTokens, 1, m_root->children());
}

AbstractSymbolGroupNode *SymbolGroup::find(const std::string &iname) const
{
    AbstractSymbolGroupNode *rc = findI(iname);
    if (::debug > 1)
        DebugPrint() << "SymbolGroup::find " << iname << ' ' << rc;
    return rc;
}

/*!
    \class LocalsSymbolGroup

    Symbol group representing the Locals view. It is firmly associated
    with stack frame, function (module) and thread.
    \ingroup qtcreatorcdbext
*/

LocalsSymbolGroup::LocalsSymbolGroup(CIDebugSymbolGroup *sg,
                                     const SymbolParameterVector &vec,
                                     ULONG threadId, unsigned frame,
                                     const std::string &function) :
    SymbolGroup(sg, vec, SymbolGroupValue::moduleOfType(function), "local"),
    m_threadId(threadId),
    m_frame(frame),
    m_function(function)
{
    // "module!function" -> "function"
    if (const std::string::size_type moduleLengh = module().size())
        m_function.erase(0, moduleLengh + 1);
}

LocalsSymbolGroup *LocalsSymbolGroup::create(CIDebugControl *control, CIDebugSymbols *debugSymbols,
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
    std::string func;

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
        StackFrame frameData;
        if (!getFrame(frame, &frameData, errorMessage))
            break;
        func = wStringToString(frameData.function);

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
    return new LocalsSymbolGroup(idebugSymbols, parameters, threadId, frame, func);
}

std::string LocalsSymbolGroup::module() const
{
    return root()->module();
}

/*!
  \class WatchesSymbolGroup

  Watch symbol group. Contains watches as added by Qt Creator as iname='watch.0',
  name='<expression>'. The IDebugSymbolGroup is created without scope.
  \ingroup qtcreatorcdbext
*/

const char *WatchesSymbolGroup::watchInamePrefix = "watch";

WatchesSymbolGroup::WatchesSymbolGroup(CIDebugSymbolGroup *sg) :
    SymbolGroup(sg, SymbolParameterVector(), std::string(), WatchesSymbolGroup::watchInamePrefix)
{
}

/* Helpers for parsing a watch expression of the form "*(<type>[ ]*[*])0xaddress"
 * (like '*(Foo*)0x47b' that is created by our dumpers or for the case the users
 * inputs something similar. The position of the data type is determined so that
 * it can be qualified by the module to make the expressions faster ('*(MyModule!Foo*)0x47b').
 * The checking should be somewhat strict since the user can input arbitrary stuff.
 * Lacking regular expression support, use a small state machine. */

enum WatchExpressionParseState
{
    WEPS_Error,
    WEPS_WithinType,
    WEPS_WithinBlanksAfterType,
    WEPS_WithinAsterisksAfterType,
    WEPS_AtParenthesisAfterType,
    WEPS_WithinAddress
};

static inline WatchExpressionParseState
    nextWatchExpressionParseState(WatchExpressionParseState s, char c, unsigned *templateLevel)
{
    switch (s) {
    case WEPS_Error:
        break;
    case WEPS_WithinType:
        switch (c) {
        case '<':
            (*templateLevel)++;
            return WEPS_WithinType;
        case '>':
            (*templateLevel)--;
            return WEPS_WithinType;
        case ' ':
            return *templateLevel ? WEPS_WithinType : WEPS_WithinBlanksAfterType;
        case '*':
            return WEPS_WithinAsterisksAfterType;
        case ',':
            return *templateLevel ? WEPS_WithinType : WEPS_Error;
        case ':':
        case '_':
            return WEPS_WithinType;
        default:
            if (std::isalnum(c))
                return WEPS_WithinType;
            break;
        }
        break;
    case WEPS_WithinBlanksAfterType:
        if (c == ' ')
            return WEPS_WithinBlanksAfterType;
        if (c == '*')
            return WEPS_WithinAsterisksAfterType;
        break;
    case WEPS_WithinAsterisksAfterType:
        if (c == '*')
            return WEPS_WithinAsterisksAfterType;
        if (c == ')')
            return WEPS_AtParenthesisAfterType;
        break;
    case WEPS_AtParenthesisAfterType:
        if (c == '0')
            return WEPS_WithinAddress;
        break;
    case WEPS_WithinAddress:
        if (std::isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F') || c == 'x')
            return WEPS_WithinAddress;
        break;
    }
    return WEPS_Error;
}

static bool parseWatchExpression(const std::string &expression,
                                 std::string::size_type *typeStart, std::string::size_type *typeEnd)
{
    *typeStart = *typeEnd = std::string::npos;
    if (expression.size() < 3 || expression.compare(0, 2, "*(") || expression.find("*)0x") == std::string::npos)
        return false;
    unsigned templateLevel = 0;
    const std::string::size_type size = expression.size();
    WatchExpressionParseState state = WEPS_WithinType;
    std::string::size_type pos = 2;
    *typeStart = pos;
    for ( ; pos < size ; pos++) {
        const char c = expression.at(pos);
        const WatchExpressionParseState nextState = nextWatchExpressionParseState(state, c, &templateLevel);
        if (nextState == WEPS_Error)
            return false;
        if (nextState != state && state == WEPS_WithinType)
            *typeEnd = pos;
        state = nextState;
    }
    return state == WEPS_WithinAddress;
}

std::string WatchesSymbolGroup::fixWatchExpressionI(CIDebugSymbols *s, const std::string &expression)
{
    // Fix a symbol of the form "*(<type>[ ]*[*])0xaddress"
    // to be qualified by the module "*(module!<type>[ ]*[*])0xaddress"
    if (expression.find('!') != std::string::npos)
        return expression;
    // Check if it matches the form
    std::string::size_type typeStartPos;
    std::string::size_type typeEndPos;
    if (!parseWatchExpression(expression, &typeStartPos, &typeEndPos))
        return expression;
    std::string type = expression.substr(typeStartPos, typeEndPos - typeStartPos);
    trimFront(type);
    trimBack(type);
    // Do not qualify POD types
    const KnownType kt = knownType(type, 0);
    if (kt & KT_POD_Type)
        return expression;
    SymbolGroupValueContext ctx;
    ctx.symbols = s;
    const std::string resolved = SymbolGroupValue::resolveType(type, ctx);
    if (resolved.empty() || resolved == type)
        return expression;
    std::string fixed = expression;
    fixed.replace(typeStartPos, typeEndPos - typeStartPos, resolved);
    return fixed;
}

// Wrapper with debug output.
std::string WatchesSymbolGroup::fixWatchExpression(CIDebugSymbols *s, const std::string &expression)
{
    const std::string fixed = fixWatchExpressionI(s, expression);
    if (SymbolGroupValue::verbose)
        DebugPrint() << ">WatchesSymbolGroup::fixWatchExpression " << expression << " -> " << fixed;
    return fixed;
}

bool WatchesSymbolGroup::addWatch(CIDebugSymbols *s, std::string iname, const std::string &expression, std::string *errorMessage)
{
    // "watch.0" -> "0"
    const size_t prefLen = std::strlen(WatchesSymbolGroup::watchInamePrefix);
    if (!iname.compare(0, prefLen, WatchesSymbolGroup::watchInamePrefix))
        iname.erase(0, prefLen + 1);
    // Already in?
    if (root()->childByIName(iname.c_str()))
        return true;
    // Resolve the expressions, but still display the original name obtained to
    // avoid cycles re-adding symbols
    SymbolGroupNode *node = addSymbol(std::string(), fixWatchExpression(s, expression),
                                      expression, iname, errorMessage);
    if (!node)
        return false;
    node->addFlags(SymbolGroupNode::WatchNode);
    return true;
}

// Compile map of current state root-iname->root-expression (top-level)
WatchesSymbolGroup::InameExpressionMap
    WatchesSymbolGroup::currentInameExpressionMap() const
{
    // Skip additional, expanded nodes
    InameExpressionMap rc;
    if (unsigned size = unsigned(root()->children().size())) {
        for (unsigned i = 0; i < size; ++i) {
            const AbstractSymbolGroupNode *n = root()->childAt(i);
            if (n->testFlags(SymbolGroupNode::WatchNode))
                rc.insert(InameExpressionMap::value_type(n->iName(), n->name()));
        }
    }
    return rc;
}

/*!
    \brief Collapse all expanded pointer items.

    If we have an item '*(Foo*)(address_of_Foo_D_Ptr)' and the
    D-Ptr changes due to detaching, the expanded items (whose address
    change) do not adapt their value. Force a re-evaluation by collapsing
    them.
*/

bool WatchesSymbolGroup::collapsePointerItems(std::string *errorMessage)
{
    typedef AbstractSymbolGroupNodePtrVector::const_iterator CIT;

    const AbstractSymbolGroupNodePtrVector existingWatches = root()->children();
    if (existingWatches.empty())
        return true;

    const CIT cend = existingWatches.end();
    for (CIT it = existingWatches.begin(); it != cend; ++it) {
        if (SymbolGroupNode *n = (*it)->asSymbolGroupNode())
            if (n->isExpanded() && SymbolGroupValue::isPointerType(n->type()))
                if (!n->collapse(errorMessage))
                return false;
    }
    return true;
}

// Synchronize watches passing on a map of '0' -> '*(int *)(0xA0)'
bool WatchesSymbolGroup::synchronize(CIDebugSymbols *s,
                                     const InameExpressionMap &newInameExpMap,
                                     std::string *errorMessage)
{
    typedef std::set<std::string> StringSet;
    typedef InameExpressionMap::const_iterator InameExpressionMapConstIt;
    InameExpressionMap oldInameExpMap = currentInameExpressionMap();
    const bool changed = oldInameExpMap != newInameExpMap;
    if (SymbolGroupValue::verbose)
        DebugPrint() << "WatchesSymbolGroup::synchronize oldsize=" << oldInameExpMap.size()
                     << " newsize=" << newInameExpMap.size() << " items, changed=" << changed;
    if (!changed) // Quick check: All ok
        return collapsePointerItems(errorMessage);
    // Check both maps against each other and determine elements to be deleted/added.
    StringSet deletionSet;
    InameExpressionMap addMap;
    InameExpressionMapConstIt ocend = oldInameExpMap.end();
    InameExpressionMapConstIt ncend = newInameExpMap.end();
    // Check for new items or ones whose expression changed.
    for (InameExpressionMapConstIt nit = newInameExpMap.begin(); nit != ncend; ++nit) {
        const InameExpressionMapConstIt oit = oldInameExpMap.find(nit->first);
        if (oit == ocend || oit->second != nit->second)
            addMap.insert(InameExpressionMap::value_type(nit->first, nit->second));
    }
    for (InameExpressionMapConstIt oit = oldInameExpMap.begin(); oit != ocend; ++oit) {
        const InameExpressionMapConstIt nit = newInameExpMap.find(oit->first);
        if (nit == ncend || nit->second != oit->second)
            deletionSet.insert(oit->first);
    }
    if (SymbolGroupValue::verbose)
        DebugPrint() << "add=" << addMap.size() << " delete=" << deletionSet.size();
    // Do insertion/deletion
    if (!deletionSet.empty()) {
        const StringSet::const_iterator dcend = deletionSet.end();
        for (StringSet::const_iterator it = deletionSet.begin(); it != dcend; ++it) {
            AbstractSymbolGroupNode *n = root()->childByIName(it->c_str());
            if (!n)
                return false;
            if (SymbolGroupValue::verbose)
                DebugPrint() << " Deleting " << n->name();
            if (!removeSymbol(n, errorMessage))
                return false;
        }
    }
    if (!collapsePointerItems(errorMessage))
        return false;
    // Insertion: We cannot possible fail here since this will trigger an
    // endless loop of the watch model. Insert a dummy item.
    if (!addMap.empty()) {
        const InameExpressionMapConstIt acend = addMap.end();
        for (InameExpressionMapConstIt it = addMap.begin(); it != acend; ++it) {
            const bool success = addWatch(s, it->first, it->second, errorMessage);
            if (SymbolGroupValue::verbose)
                DebugPrint() << " Adding " << it->first << ',' << it->second << ",success=" << success;
            if (!success)
                root()->addChild(new ErrorSymbolGroupNode(it->second, it->first));
        }
    }
    return true;
}

// Create scopeless group.
WatchesSymbolGroup *WatchesSymbolGroup::create(CIDebugSymbols *symbols,
                                               std::string *errorMessage)
{
    errorMessage->clear();

    IDebugSymbolGroup2 *idebugSymbols = 0;
    const HRESULT hr = symbols->CreateSymbolGroup2(&idebugSymbols);
    if (FAILED(hr)) {
        *errorMessage = msgDebugEngineComFailed("CreateSymbolGroup", hr);
        return 0;
    }
    return new WatchesSymbolGroup(idebugSymbols);
}

//! @}
