/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "common.h"

#include <vector>
#include <string>
#include <map>
#include <iosfwd>

enum { symbolGroupDebug = 0 };

std::ostream &operator<<(std::ostream &, const DEBUG_SYMBOL_PARAMETERS&p);

class SymbolGroupNodeVisitor;
class SymbolGroup;
struct SymbolGroupValueContext;
class SymbolGroupNode;
class MemoryHandle;

// Helper struct used for check results when recoding CDB char pointer output.
struct DumpParameterRecodeResult
{
    unsigned char *buffer = 0;
    size_t size = 0;
    std::string recommendedFormat;
    bool isWide = false;
};

struct DumpParameters
{
    typedef std::map<std::string, std::string> FormatMap; // type or iname to format
    enum DumpFlags
    {
        DumpHumanReadable = 0x1,
        DumpComplexDumpers = 0x2,
        DumpAlphabeticallySorted = 0x4
    };

    bool humanReadable() const {  return dumpFlags & DumpHumanReadable; }
    bool isAlphabeticallySorted() const {  return (dumpFlags & DumpAlphabeticallySorted) != 0; }
    // Helper to decode format option arguments.
    static FormatMap decodeFormatArgument(const std::string &f, bool isHex);

    static DumpParameterRecodeResult
        checkRecode(const std::string &type, const std::string &iname,
                    const std::wstring &value,
                    const SymbolGroupValueContext &ctx,
                    ULONG64 address,
                    const DumpParameters *dp =0);

    bool recode(const std::string &type, const std::string &iname,
                const SymbolGroupValueContext &ctx,
                ULONG64 address,
                std::wstring *value, std::string *encoding) const;
    std::string format(const std::string &type, const std::string &iname) const;

    unsigned dumpFlags = 0;
    FormatMap typeFormats;
    FormatMap individualFormats;
};

std::ostream &operator<<(std::ostream &os, const DumpParameters &);

class AbstractSymbolGroupNode
{
    AbstractSymbolGroupNode(const AbstractSymbolGroupNode&);
    AbstractSymbolGroupNode& operator=(const AbstractSymbolGroupNode&);

public:
    typedef std::vector<AbstractSymbolGroupNode *> AbstractSymbolGroupNodePtrVector;
    typedef AbstractSymbolGroupNodePtrVector::iterator AbstractSymbolGroupNodePtrVectorIterator;
    typedef AbstractSymbolGroupNodePtrVector::const_iterator AbstractSymbolGroupNodePtrVectorConstIterator;

    virtual ~AbstractSymbolGroupNode();

    // Name to appear in watchwindow
    const std::string &name() const { return m_name; }
    // 'iname' used as an internal id.
    const std::string &iName() const { return m_iname; }
    // Full iname 'local.x.foo': WARNING: this returns the absolute path not
    // taking reference nodes into account by recursing up the parents.
    std::string absoluteFullIName() const;

    virtual const AbstractSymbolGroupNodePtrVector &children() const = 0;
    AbstractSymbolGroupNode *childAt(unsigned) const;

    unsigned indexByIName(const char *) const; // (unsigned(-1) on failure
    AbstractSymbolGroupNode *childByIName(const char *) const;
    unsigned indexOf(const AbstractSymbolGroupNode *) const;

    const AbstractSymbolGroupNode *parent() const { return m_parent; }
    AbstractSymbolGroupNode *parent() { return m_parent; }

    unsigned flags() const           { return m_flags; }
    bool testFlags(unsigned f) const { return (m_flags & f) != 0; }
    void addFlags(unsigned f)        { m_flags |= f; }
    void clearFlags(unsigned f)      { m_flags &= ~f; }

    virtual SymbolGroupNode *asSymbolGroupNode() { return 0; }
    virtual const SymbolGroupNode *asSymbolGroupNode() const { return 0; }
    virtual const AbstractSymbolGroupNode *resolveReference() const { return this; }
    virtual AbstractSymbolGroupNode *resolveReference() { return this; }

    bool accept(SymbolGroupNodeVisitor &visitor,
                const std::string &visitingParentIname,
                unsigned child, unsigned depth);

    // I/O: GDBMI dump for Visitors, return child count
    virtual int dump(std::ostream &str, const std::string &visitingFullIname,
                     const DumpParameters &p, const SymbolGroupValueContext &ctx) = 0;
    // I/O: debug output for Visitors
    virtual void debug(std::ostream &os, const std::string &visitingFullIname,
                       unsigned verbosity, unsigned depth) const;

    // For BaseSymbolGroupNode only.
    void setParent(AbstractSymbolGroupNode *n);

protected:
    explicit AbstractSymbolGroupNode(const std::string &name, const std::string &iname);

    // Basic GDBMI dumping
    static inline void dumpBasicData(std::ostream &str, const std::string &aName,
                                  const std::string &aFullIname,
                                  const std::string &type = std::string(),
                                  const std::string &expression = std::string());

private:
    const std::string m_name;
    const std::string m_iname;
    AbstractSymbolGroupNode *m_parent = nullptr;
    unsigned m_flags = 0;
};

class BaseSymbolGroupNode : public AbstractSymbolGroupNode
{
public:
    virtual const AbstractSymbolGroupNodePtrVector &children() const { return m_children; }
    void addChild(AbstractSymbolGroupNode *c); // for watches
    void removeChildAt(unsigned);

protected:
    explicit BaseSymbolGroupNode(const std::string &name, const std::string &iname);
    virtual ~BaseSymbolGroupNode();

    void reserveChildren(AbstractSymbolGroupNodePtrVector::size_type s) { m_children.reserve(s); }

protected:
    AbstractSymbolGroupNodePtrVector m_children;
    void removeChildren();
};

// Dummy fake node to satisfy the watch model for failed additions. Reports error.
class ErrorSymbolGroupNode : public BaseSymbolGroupNode
{
public:
    explicit ErrorSymbolGroupNode(const std::string &name, const std::string &iname);

    virtual int dump(std::ostream &str, const std::string &fullIname,
                     const DumpParameters &p, const SymbolGroupValueContext &ctx);
    virtual void debug(std::ostream &os, const std::string &visitingFullIname,
                       unsigned verbosity, unsigned depth) const;
};

class SymbolGroupNode : public BaseSymbolGroupNode
{
    explicit SymbolGroupNode(SymbolGroup *symbolGroup,
                             ULONG index,
                             const std::string &module,
                             const std::string &name,
                             const std::string &iname);

public:
    enum Flags
    {
        Uninitialized = 0x1,
        SimpleDumperNotApplicable = 0x2, // No dumper available for type
        SimpleDumperOk = 0x4,     // Internal dumper ran, value set
        SimpleDumperFailed = 0x8, // Internal dumper failed
        SimpleDumperMask = SimpleDumperNotApplicable|SimpleDumperOk|SimpleDumperFailed,
        ExpandedByRequest = 0x10,
        AdditionalSymbol = 0x20, // Introduced by addSymbol, should not be visible
        Obscured = 0x40,    // Symbol is obscured by (for example) fake container children
        ComplexDumperOk = 0x80,
        WatchNode = 0x100,
        PreSortedChildren = 0x200
    };

    ~SymbolGroupNode();

    typedef std::vector<DEBUG_SYMBOL_PARAMETERS> SymbolParameterVector;

    void parseParameters(SymbolParameterVector::size_type index,
                         SymbolParameterVector::size_type parameterOffset,
                         const SymbolParameterVector &vec);

    static SymbolGroupNode *create(SymbolGroup *sg, const std::string &module, const std::string &name, const SymbolParameterVector &vec);
    // For root nodes, only: Add a new symbol by name
    SymbolGroupNode *addSymbolByName(const std::string &module,
                                     const std::string &name,  // Expression like 'myarray[1]'
                                     const std::string &displayName,  // Name to be displayed, defaults to name
                                     const std::string &iname, // Desired iname, defaults to name
                                     std::string *errorMessage);

    SymbolGroup *symbolGroup() const { return m_symbolGroup; }

    // I/O: Gdbmi dump for Visitors
    virtual int dump(std::ostream &str, const std::string &fullIname,
                      const DumpParameters &p, const SymbolGroupValueContext &ctx);
    // Dumper functionality for reference nodes, returns child count guess
    int dumpNode(std::ostream &str, const std::string &aName, const std::string &aFullIName,
                  const DumpParameters &p, const SymbolGroupValueContext &ctx);

    // I/O: debug for Visitors
    virtual void debug(std::ostream &os, const std::string &visitingFullIname,
                       unsigned verbosity, unsigned depth) const;

    std::wstring symbolGroupRawValue() const;
    std::wstring symbolGroupFixedValue() const;

    bool assign(const std::string &value, std::string *errorMessage = 0);
    std::wstring simpleDumpValue(const SymbolGroupValueContext &ctx, std::string *encoding);

    // A quick check if symbol is valid by checking for inaccessible value
    bool isMemoryAccessible() const;

    std::string type() const;
    int dumperType() const { return m_dumperType; } // Valid after dumper run
    int dumperContainerSize() { return m_dumperContainerSize; } // Valid after dumper run
    unsigned size() const; // Size of value
    ULONG64 address() const;
    std::string module() const { return m_module; }

    bool expand(std::string *errorMessage);
    bool expandRunComplexDumpers(const SymbolGroupValueContext &ctx, std::string *errorMessage);
    bool isExpanded() const { return !children().empty(); }
    bool canExpand() const { return m_parameters.SubElements > 0; }
    bool collapse(std::string *errorMessage);
    void runComplexDumpers(const SymbolGroupValueContext &ctx);
    // Cast to a different type. Works only on unexpanded nodes
    bool typeCast(const std::string &desiredType, std::string *errorMessage);

    ULONG subElements() const { return m_parameters.SubElements; }
    ULONG index() const { return m_index; }

    MemoryHandle *memory() const { return m_memory; }

    virtual SymbolGroupNode *asSymbolGroupNode() { return this; }
    virtual const SymbolGroupNode *asSymbolGroupNode() const { return this; }

    // Remove self off parent and return the indexes to be shifted or unsigned(-1).
    bool removeSelf(SymbolGroupNode *root, std::string *errorMessage);

    static std::string msgAssignError(const std::string &nodeName,
                                      const std::string &value,
                                      const std::string &why);

private:
    const SymbolGroupNode *symbolGroupNodeParent() const;
    SymbolGroupNode *symbolGroupNodeParent();
    bool isArrayElement() const;
    // Notify about expansion/collapsing of a node, shift indexes
    bool notifyIndexesMoved(ULONG index, bool inserted, ULONG offset);
    bool runSimpleDumpers(const SymbolGroupValueContext &ctx);
    ULONG nextSymbolIndex() const;

    SymbolGroup *const m_symbolGroup = nullptr;
    const std::string m_module;
    ULONG m_index = 0;
    DEBUG_SYMBOL_PARAMETERS m_parameters; // Careful when using ParentSymbol. It might not be correct.
    std::wstring m_dumperValue;
    std::string m_dumperValueEncoding;
    int m_dumperType = -1;
    int m_dumperContainerSize = -1;
    void *m_dumperSpecialInfo = nullptr; // Opaque information passed from simple to complex dumpers
    MemoryHandle *m_memory = nullptr; // Memory shared between simple dumper and edit value.
};

class ReferenceSymbolGroupNode : public AbstractSymbolGroupNode
{
public:
    explicit ReferenceSymbolGroupNode(const std::string &name,
                                      const std::string &iname,
                                      SymbolGroupNode *referencedNode);
    // Convenience to create a node name name='[1]', iname='1' for arrays
    static ReferenceSymbolGroupNode *createArrayNode(int index,
                                                     SymbolGroupNode *referencedNode);

    virtual int dump(std::ostream &str, const std::string &visitingFullIname,
                      const DumpParameters &p, const SymbolGroupValueContext &ctx);
    virtual void debug(std::ostream &os, const std::string &visitingFullIname,
                       unsigned verbosity, unsigned depth) const;

    virtual const AbstractSymbolGroupNodePtrVector &children() const { return m_referencedNode->children(); }

    virtual const AbstractSymbolGroupNode *resolveReference() const { return m_referencedNode; }
    virtual AbstractSymbolGroupNode *resolveReference() { return m_referencedNode; }

private:
    SymbolGroupNode * const m_referencedNode;
};

class MapNodeSymbolGroupNode : public BaseSymbolGroupNode
{
private:
    explicit MapNodeSymbolGroupNode(const std::string &name,
                                    const std::string &iname,
                                    ULONG64 address /* = 0 */,
                                    const std::string &type,
                                    AbstractSymbolGroupNode *key,
                                    AbstractSymbolGroupNode *value);

public:
    static MapNodeSymbolGroupNode *
        create(int i, ULONG64 address /* = 0 */, const std::string &type,
               SymbolGroupNode *key, SymbolGroupNode *value);

    virtual int dump(std::ostream &str, const std::string &visitingFullIname,
                      const DumpParameters &p, const SymbolGroupValueContext &ctx);
    virtual void debug(std::ostream &os, const std::string &visitingFullIname,
                       unsigned verbosity, unsigned depth) const;

private:
    const ULONG64 m_address;
    const std::string m_type;
};

class SymbolGroupNodeVisitor {
    SymbolGroupNodeVisitor(const SymbolGroupNodeVisitor&);
    SymbolGroupNodeVisitor& operator=(const SymbolGroupNodeVisitor&);

    friend class AbstractSymbolGroupNode;
protected:
    SymbolGroupNodeVisitor() = default;
public:
    virtual ~SymbolGroupNodeVisitor() = default;

    // "local.vi" -> "local"
    static std::string parentIname(const std::string &iname);
    static const char iNamePathSeparator = '.';

protected:
    enum VisitResult
    {
        VisitContinue,
        VisitSkipChildren,
        VisitStop
    };

protected:
    virtual VisitResult visit(AbstractSymbolGroupNode *node,
                              const std::string &fullIname,
                              unsigned child, unsigned depth) = 0;
    // Helper for formatting output.
    virtual void childrenVisited(const AbstractSymbolGroupNode * /* node */, unsigned /* depth */) {}
    virtual bool sortChildrenAlphabetically() const { return false; }
};

class DebugSymbolGroupNodeVisitor : public SymbolGroupNodeVisitor {
public:
    explicit DebugSymbolGroupNodeVisitor(std::ostream &os, unsigned verbosity = 0);

protected:
    virtual VisitResult visit(AbstractSymbolGroupNode *node,
                              const std::string &fullIname,
                              unsigned child, unsigned depth);

private:
    std::ostream &m_os;
    const unsigned m_verbosity;
};

class DebugFilterSymbolGroupNodeVisitor : public DebugSymbolGroupNodeVisitor {
public:
    explicit DebugFilterSymbolGroupNodeVisitor(std::ostream &os,
                                               const std::string &filter,
                                               const unsigned verbosity = 0);

protected:
    virtual VisitResult visit(AbstractSymbolGroupNode *node,
                              const std::string &fullIname,
                              unsigned child, unsigned depth);

private:
    const std::string m_filter;
};

class DumpSymbolGroupNodeVisitor : public SymbolGroupNodeVisitor {
public:
    explicit DumpSymbolGroupNodeVisitor(std::ostream &os,
                                        const SymbolGroupValueContext &context,
                                        const DumpParameters &parameters = DumpParameters());

protected:
    virtual VisitResult visit(AbstractSymbolGroupNode *node,
                              const std::string &fullIname,
                              unsigned child, unsigned depth);
    virtual void childrenVisited(const AbstractSymbolGroupNode *  node, unsigned depth);
    bool sortChildrenAlphabetically() const override { return m_parameters.isAlphabeticallySorted(); }

private:
    std::ostream &m_os;
    const SymbolGroupValueContext &m_context;
    const DumpParameters &m_parameters;
    unsigned m_lastDepth = unsigned(-1);
};
