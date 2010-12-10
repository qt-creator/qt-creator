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

#ifndef SYMBOLGROUP_H
#define SYMBOLGROUP_H

#include "common.h"

#include <string>
#include <vector>
#include <map>
#include <iosfwd>

std::ostream &operator<<(std::ostream &, const DEBUG_SYMBOL_PARAMETERS&p);

class SymbolGroupNodeVisitor;
class SymbolGroup;
struct SymbolGroupValueContext;

// All parameters for dumping in one struct.
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

/* Thin wrapper around a symbol group entry. Provides accessors for fixed-up
 * symbol group value and a dumping facility consisting of:
 * - 'Simple' dumping done when running the DumpVisitor. This produces one
 *   line of formatted output shown for the class. These values
 *   values should are displayed, while still allowing for expansion of the structure
 *   in the debugger.
 *   It also pre-determines some information for complex dumping (type, container).
 * - 'Complex' dumping: Obscures the symbol group children by fake children, for
 *   example container children, run when calling SymbolGroup::dump with an iname.
 *   The fake children are appended to the child list (other children are just marked as
 *   obscured for GDBMI dumping so that SymbolGroupValue expressions still work as before).
 * The dumping is mostly based on SymbolGroupValue expressions.
 * in the debugger. Evaluating those dumpers might expand symbol nodes, which are
 * then marked as 'ExpandedByDumper'. This stops the dump recursion to prevent
 * outputting data that were not explicitly expanded by the watch handler. */

class SymbolGroupNode {
    SymbolGroupNode(const SymbolGroupNode&);
    SymbolGroupNode& operator=(const SymbolGroupNode&);

    explicit SymbolGroupNode(SymbolGroup *symbolGroup,
                             ULONG index,
                             const std::string &name,
                             const std::string &iname,
                             SymbolGroupNode *parent = 0);

public:
    enum Flags {
        Uninitialized = 0x1,
        SimpleDumperNotApplicable = 0x2, // No dumper available for type
        SimpleDumperOk = 0x4,     // Internal dumper ran, value set
        SimpleDumperFailed = 0x8, // Internal dumper failed
        SimpleDumperMask = SimpleDumperNotApplicable|SimpleDumperOk|SimpleDumperFailed,
        ExpandedByDumper = 0x10,
        AdditionalSymbol = 0x20, // Introduced by addSymbol, should not be visible
        Obscured = 0x40,    // Symbol is obscured by (for example) fake container children
        ComplexDumperOk = 0x80
    };

    typedef std::vector<DEBUG_SYMBOL_PARAMETERS> SymbolParameterVector;
    typedef std::vector<SymbolGroupNode *> SymbolGroupNodePtrVector;
    typedef SymbolGroupNodePtrVector::iterator SymbolGroupNodePtrVectorIterator;
    typedef SymbolGroupNodePtrVector::const_iterator SymbolGroupNodePtrVectorConstIterator;

    ~SymbolGroupNode() { removeChildren(); }

    // Indicate reference
    void setReferencedBy(SymbolGroupNode *n);

    void removeChildren();
    void parseParameters(SymbolParameterVector::size_type index,
                         SymbolParameterVector::size_type parameterOffset,
                         const SymbolParameterVector &vec);

    static SymbolGroupNode *create(SymbolGroup *sg, const std::string &name, const SymbolParameterVector &vec);
    // For root nodes, only: Add a new symbol by name
    SymbolGroupNode *addSymbolByName(const std::string &name,  // Expression like 'myarray[1]'
                                     const std::string &iname, // Desired iname, defaults to name
                                     std::string *errorMessage);

    const std::string &name() const { return m_name; }
    std::string fullIName() const;
    const std::string &iName() const { return m_iname; }

    const SymbolGroupNodePtrVector &children() const { return m_children; }
    SymbolGroupNode *childAt(unsigned) const;

    unsigned indexByIName(const char *) const; // (unsigned(-1) on failure
    SymbolGroupNode *childByIName(const char *) const;

    const SymbolGroupNode *parent() const { return m_parent; }
    const SymbolGroupNode *referencedParent() const { return m_referencedBy ? m_referencedBy : m_parent; }
    SymbolGroup *symbolGroup() const { return m_symbolGroup; }

    // I/O: Gdbmi dump for Visitors
    void dump(std::ostream &str, const DumpParameters &p, const SymbolGroupValueContext &ctx);
    // I/O: debug for Visitors
    void debug(std::ostream &os, unsigned verbosity, unsigned depth) const;

    std::wstring symbolGroupRawValue() const;
    std::wstring symbolGroupFixedValue() const;

    std::string type() const;
    int dumperType() const { return m_dumperType; } // Valid after dumper run
    int dumperContainerSize() { return m_dumperContainerSize; } // Valid after dumper run
    unsigned size() const; // Size of value
    ULONG64 address() const;

    bool accept(SymbolGroupNodeVisitor &visitor, unsigned child, unsigned depth);

    bool expand(std::string *errorMessage);
    bool isExpanded() const { return !m_children.empty(); }
    bool canExpand() const { return m_parameters.SubElements > 0; }
    void runComplexDumpers(const SymbolGroupValueContext &ctx);
    // Cast to a different type. Works only on unexpanded nodes
    bool typeCast(const std::string &desiredType, std::string *errorMessage);

    ULONG subElements() const { return m_parameters.SubElements; }
    ULONG index() const { return m_index; }

    unsigned flags() const     { return m_flags; }
    void setFlags(unsigned f)  { m_flags = f; }
    void addFlags(unsigned f)  { m_flags |= f; }
    void clearFlags(unsigned f)  { m_flags &= ~f; }

private:
    bool isArrayElement() const;
    // Notify about expansion of a node, shift indexes
    bool notifyExpanded(ULONG index, ULONG insertedCount);
    std::wstring simpleDumpValue(const SymbolGroupValueContext &ctx,
                                 const DumpParameters &p);

    SymbolGroup *const m_symbolGroup;
    SymbolGroupNode *m_parent;
    // Indicates a fake child (container value). Used for the full iname
    SymbolGroupNode *m_referencedBy;
    ULONG m_index;
    DEBUG_SYMBOL_PARAMETERS m_parameters; // Careful when using ParentSymbol. It might not be correct.
    SymbolGroupNodePtrVector m_children;
    const std::string m_name;
    const std::string m_iname;
    unsigned m_flags;
    std::wstring m_dumperValue;
    int m_dumperType;
    int m_dumperContainerSize;
};

/* Visitor that takes care of iterating over the nodes
 * visit() is not called for the (invisible) root node, but starting with the
 * root's children with depth=0.
 * Return true from visit() to terminate the recursion. */

class SymbolGroupNodeVisitor {
    SymbolGroupNodeVisitor(const SymbolGroupNodeVisitor&);
    SymbolGroupNodeVisitor& operator=(const SymbolGroupNodeVisitor&);

    friend class SymbolGroupNode;
protected:
    SymbolGroupNodeVisitor() {}
public:
    virtual ~SymbolGroupNodeVisitor() {}

protected:
    enum VisitResult {
        VisitContinue,
        VisitSkipChildren,
        VisitStop
    };

private:
    virtual VisitResult visit(SymbolGroupNode *node, unsigned child, unsigned depth) = 0;
    // Helper for formatting output.
    virtual void childrenVisited(const SymbolGroupNode * /* node */, unsigned /* depth */) {}
};

// Thin wrapper around a symbol group storing a tree of expanded symbols rooted on
// a fake "locals" root element.
// Provides a find() method based on inames ("locals.this.i1.data") that retrieves
// that index based on the current expansion state.

class SymbolGroup {
public:
    typedef std::vector<DEBUG_SYMBOL_PARAMETERS> SymbolParameterVector;

private:
    explicit SymbolGroup(CIDebugSymbolGroup *,
                         const SymbolParameterVector &vec,
                         ULONG threadId, unsigned frame);

    SymbolGroup(const SymbolGroup &);
    SymbolGroup &operator=(const SymbolGroup &);

public:
    typedef SymbolGroupNode::SymbolGroupNodePtrVector SymbolGroupNodePtrVector;

    static SymbolGroup *create(CIDebugControl *control,
                               CIDebugSymbols *,
                               ULONG threadId,
                               unsigned frame,
                               std::string *errorMessage);
    ~SymbolGroup();

    // Dump all
    std::string dump(const SymbolGroupValueContext &ctx,
                     const DumpParameters &p = DumpParameters()) const;
    // Expand node and dump
    std::string dump(const std::string &iname, const SymbolGroupValueContext &ctx,
                     const DumpParameters &p, std::string *errorMessage);
    std::string debug(const std::string &iname = std::string(), unsigned verbosity = 0) const;

    unsigned frame() const { return m_frame; }
    ULONG threadId() const { return m_threadId; }
    SymbolGroupNode *root() { return m_root; }
    const SymbolGroupNode *root() const { return m_root; }
    SymbolGroupNode *find(const std::string &iname) const;

    // Expand a node list "locals.i1,locals.i2", expanding all nested child nodes
    // (think mkdir -p).
    unsigned expandList(const std::vector<std::string> &nodes, std::string *errorMessage);
    // Mark uninitialized (top level only)
    void markUninitialized(const std::vector<std::string> &nodes);

    // Expand a single node "locals.A.B" requiring that "locals.A.B" is already visible
    // (think mkdir without -p).
    bool expand(const std::string &node, std::string *errorMessage);
    // Cast an (unexpanded) node
    bool typeCast(const std::string &iname, const std::string &desiredType, std::string *errorMessage);
    // Add a symbol by name expression
    SymbolGroupNode *addSymbol(const std::string &name, // Expression like 'myarray[1]'
                               const std::string &iname, // Desired iname, defaults to name
                               std::string *errorMessage);

    bool accept(SymbolGroupNodeVisitor &visitor) const;

    // Assign a value by iname
    bool assign(const std::string &node,
                const std::string &value,
                std::string *errorMessage);

    CIDebugSymbolGroup *debugSymbolGroup() const { return m_symbolGroup; }

    static bool getSymbolParameters(CIDebugSymbolGroup *m_symbolGroup,
                                    unsigned long start,
                                    unsigned long count,
                                    SymbolParameterVector *vec,
                                    std::string *errorMessage);

private:
    inline SymbolGroupNode *findI(const std::string &iname) const;
    static bool getSymbolParameters(CIDebugSymbolGroup *m_symbolGroup,
                                    SymbolParameterVector *vec,
                                    std::string *errorMessage);

    CIDebugSymbolGroup * const m_symbolGroup;
    const unsigned m_frame;
    const ULONG m_threadId;
    SymbolGroupNode *m_root;
};

// Debug output visitor.
class DebugSymbolGroupNodeVisitor : public SymbolGroupNodeVisitor {
public:
    explicit DebugSymbolGroupNodeVisitor(std::ostream &os, unsigned verbosity = 0);

private:
    virtual VisitResult visit(SymbolGroupNode *node, unsigned child, unsigned depth);

    std::ostream &m_os;
    const unsigned m_verbosity;
};

// Gdbmi dump output visitor.
class DumpSymbolGroupNodeVisitor : public SymbolGroupNodeVisitor {
public:
    explicit DumpSymbolGroupNodeVisitor(std::ostream &os,
                                        const SymbolGroupValueContext &context,
                                        const DumpParameters &parameters = DumpParameters());

private:
    virtual VisitResult visit(SymbolGroupNode *node, unsigned child, unsigned depth);
    virtual void childrenVisited(const SymbolGroupNode *  node, unsigned depth);

    std::ostream &m_os;
    const SymbolGroupValueContext &m_context;
    const DumpParameters &m_parameters;
    bool m_visitChildren;
    unsigned m_lastDepth;
};

#endif // SYMBOLGROUP_H
