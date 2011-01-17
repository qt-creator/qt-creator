/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef SYMBOLGROUPNODE_H
#define SYMBOLGROUPNODE_H

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

// All parameters for GDBMI dumping of a symbol group in one struct.
// The debugging engine passes maps of type names/inames to special
// integer values indicating hex/dec, etc.
struct DumpParameters
{
    typedef std::map<std::string, int> FormatMap; // type or iname to format
    enum DumpFlags
    {
        DumpHumanReadable = 0x1,
        DumpComplexDumpers = 0x2
    };

    DumpParameters();
    bool humanReadable() const {  return dumpFlags & DumpHumanReadable; }
    // Helper to decode format option arguments.
    static FormatMap decodeFormatArgument(const std::string &f);

    bool recode(const std::string &type, const std::string &iname,
                const SymbolGroupValueContext &ctx,
                std::wstring *value, int *encoding) const;
    int format(const std::string &type, const std::string &iname) const;

    unsigned dumpFlags;
    FormatMap typeFormats;
    FormatMap individualFormats;
};

// Abstract base class for a node of SymbolGroup providing the child list interface.
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
    AbstractSymbolGroupNode *m_parent;
    unsigned m_flags;
};

// Base class for a node of SymbolGroup with a flat list of children.
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

private:
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

/* SymbolGroupNode: 'Real' node within a symbol group, identified by its index
 * in IDebugSymbolGroup.
 * Provides accessors for fixed-up symbol group value and a dumping facility
 * consisting of:
 * - 'Simple' dumping done when running the DumpVisitor. This produces one
 *   line of formatted output shown for the class. These values
 *   values are always displayed, while still allowing for expansion of the structure
 *   in the debugger.
 *   It also pre-determines some information for complex dumping (type, container).
 * - 'Complex' dumping: Obscures the symbol group children by fake children, for
 *   example container children, to be run when calling SymbolGroup::dump with an iname.
 *   The fake children are appended to the child list (other children are just marked as
 *   obscured for GDBMI dumping so that SymbolGroupValue expressions still work as before).
 * The dumping is mostly based on SymbolGroupValue expressions.
 * in the debugger. Evaluating those dumpers might expand symbol nodes, which are
 * then marked as 'ExpandedByDumper'. This stops the dump recursion to prevent
 * outputting data that were not explicitly expanded by the watch handler. */

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
        ExpandedByDumper = 0x10,
        AdditionalSymbol = 0x20, // Introduced by addSymbol, should not be visible
        Obscured = 0x40,    // Symbol is obscured by (for example) fake container children
        ComplexDumperOk = 0x80,
        WatchNode = 0x100
    };

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
    void runComplexDumpers(const SymbolGroupValueContext &ctx);
    // Cast to a different type. Works only on unexpanded nodes
    bool typeCast(const std::string &desiredType, std::string *errorMessage);

    ULONG subElements() const { return m_parameters.SubElements; }
    ULONG index() const { return m_index; }

    virtual SymbolGroupNode *asSymbolGroupNode() { return this; }
    virtual const SymbolGroupNode *asSymbolGroupNode() const { return this; }

    // Remove self off parent and return the indexes to be shifted or unsigned(-1).
    bool removeSelf(SymbolGroupNode *root, std::string *errorMessage);

private:
    const SymbolGroupNode *symbolGroupNodeParent() const;
    SymbolGroupNode *symbolGroupNodeParent();
    bool isArrayElement() const;
    // Notify about expansion/collapsing of a node, shift indexes
    bool notifyIndexesMoved(ULONG index, bool inserted, ULONG offset);
    bool runSimpleDumpers(const SymbolGroupValueContext &ctx);
    std::wstring simpleDumpValue(const SymbolGroupValueContext &ctx);
    ULONG nextSymbolIndex() const;

    SymbolGroup *const m_symbolGroup;
    const std::string m_module;
    ULONG m_index;
    DEBUG_SYMBOL_PARAMETERS m_parameters; // Careful when using ParentSymbol. It might not be correct.
    std::wstring m_dumperValue;
    int m_dumperType;
    int m_dumperContainerSize;
    void *m_dumperSpecialInfo; // Opaque information passed from simple to complex dumpers
};

// Artificial node referencing another (real) SymbolGroupNode (added symbol or
// symbol from within an expanded linked list structure). Forwards the
// dumping to the referenced node using its own name.
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

// A [fake] map node with a fake array index and key/value entries consisting
// of ReferenceSymbolGroupNode.
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

/* Visitor that takes care of iterating over the nodes and
 * building the full iname path ('local.foo.bar') that is required for
 * GDBMI dumping. The full name depends on the path on which a node was reached
 * for referenced nodes (a linked list element can be reached via array index
 * or by expanding the whole structure).
 * visit() is not called for the (invisible) root node, but starting with the
 * root's children with depth=0.
 * Return VisitStop from visit() to terminate the recursion. */

class SymbolGroupNodeVisitor {
    SymbolGroupNodeVisitor(const SymbolGroupNodeVisitor&);
    SymbolGroupNodeVisitor& operator=(const SymbolGroupNodeVisitor&);

    friend class AbstractSymbolGroupNode;
protected:
    SymbolGroupNodeVisitor() {}
public:
    virtual ~SymbolGroupNodeVisitor() {}

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
};

// Debug output visitor.
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

// Debug filtering output visitor.
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

// GDBMI dump output visitor used to report locals values back to the
// debugging engine.
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

private:
    std::ostream &m_os;
    const SymbolGroupValueContext &m_context;
    const DumpParameters &m_parameters;
    unsigned m_lastDepth;
};

#endif // SYMBOLGROUPNODE_H
