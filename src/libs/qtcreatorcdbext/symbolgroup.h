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
#include <iosfwd>

std::ostream &operator<<(std::ostream &, const DEBUG_SYMBOL_PARAMETERS&p);

class SymbolGroupNodeVisitor;

// Thin wrapper around a symbol group entry.
class SymbolGroupNode {
    SymbolGroupNode(const SymbolGroupNode&);
    SymbolGroupNode& operator=(const SymbolGroupNode&);
public:
    enum Flags {
        Uninitialized = 0x1
    };
    typedef std::vector<DEBUG_SYMBOL_PARAMETERS> SymbolParameterVector;
    typedef std::vector<SymbolGroupNode *> SymbolGroupNodePtrVector;
    typedef SymbolGroupNodePtrVector::iterator SymbolGroupNodePtrVectorIterator;
    typedef SymbolGroupNodePtrVector::const_iterator SymbolGroupNodePtrVectorConstIterator;

    explicit SymbolGroupNode(CIDebugSymbolGroup *symbolGroup,
                             const std::string &name,
                             const std::string &iname,
                             SymbolGroupNode *parent = 0);

    ~SymbolGroupNode() { removeChildren(); }

    void removeChildren();
    void parseParameters(SymbolParameterVector::size_type index,
                         SymbolParameterVector::size_type parameterOffset,
                         const SymbolParameterVector &vec);

    static SymbolGroupNode *create(CIDebugSymbolGroup *sg, const std::string &name, const SymbolParameterVector &vec);

    const std::string &name() const { return m_name; }
    std::string fullIName() const;
    const std::string &iName() const { return m_iname; }

    const SymbolGroupNodePtrVector &children() const { return m_children; }
    SymbolGroupNode *childAt(unsigned) const;
    const SymbolGroupNode *parent() const { return m_parent; }

    // I/O: Gdbmi dump for Visitors
    void dump(std::ostream &str, unsigned child, unsigned depth,
              bool humanReadable, ULONG &index) const;
    void dumpChildrenVisited(std::ostream &str, bool humanReadable) const;
    // I/O: debug for Visitors
    void debug(std::ostream &os, unsigned verbosity, unsigned depth, ULONG index) const;

    std::wstring rawValue(ULONG index) const;
    std::wstring fixedValue(ULONG index) const;
    ULONG64 address(ULONG index) const;

    bool accept(SymbolGroupNodeVisitor &visitor, unsigned child, unsigned depth, ULONG &index) const;

    // Skip indexes of all children
    ULONG recursiveIndexOffset() const;

    bool expand(ULONG index, std::string *errorMessage);

    ULONG subElements() const { return m_parameters.SubElements; }

    unsigned flags() const     { return m_flags; }
    void setFlags(unsigned f)  { m_flags = f; }

private:
    // Return allocated wide string array of value
    wchar_t *getValue(ULONG index, ULONG *obtainedSize = 0) const;
    std::string getType(ULONG index) const;
    bool isArrayElement() const;

    CIDebugSymbolGroup *const m_symbolGroup;
    SymbolGroupNode *m_parent;
    DEBUG_SYMBOL_PARAMETERS m_parameters; // Careful when using ParentSymbol. It might not be correct.
    SymbolGroupNodePtrVector m_children;
    const std::string m_name;
    const std::string m_iname;
    unsigned m_flags;
};

/* Visitor that takes care of iterating over the nodes and the index bookkeeping.
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

private:
    virtual bool visit(const SymbolGroupNode *node, unsigned child, unsigned depth, ULONG index) = 0;
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
    std::string dump(bool humanReadable = false) const;
    // Expand node and dump
    std::string dump(const std::string &name, bool humanReadable, std::string *errorMessage);
    std::string debug(unsigned verbosity = 0) const;

    unsigned frame() const { return m_frame; }
    ULONG threadId() const { return m_threadId; }
    const SymbolGroupNode *root() { return m_root; }

    // Expand a node list "locals.i1,locals.i2", expanding all nested child nodes
    // (think mkdir -p).
    unsigned expandList(const std::vector<std::string> &nodes, std::string *errorMessage);
    // Mark uninitialized (top level only)
    void markUninitialized(const std::vector<std::string> &nodes);

    // Expand a single node "locals.A.B" requiring that "locals.A.B" is already visible
    // (think mkdir without -p).
    bool expand(const std::string &node, std::string *errorMessage);

    bool accept(SymbolGroupNodeVisitor &visitor) const;

    // Assign a value by iname
    bool assign(const std::string &node,
                const std::string &value,
                std::string *errorMessage);

    static bool getSymbolParameters(CIDebugSymbolGroup *m_symbolGroup,
                                    unsigned long start,
                                    unsigned long count,
                                    SymbolParameterVector *vec,
                                    std::string *errorMessage);

private:
    SymbolGroupNode *find(const std::string &iname, ULONG *index = 0) const;
    inline SymbolGroupNode *findI(const std::string &iname, ULONG *index = 0) const;
    static bool getSymbolParameters(CIDebugSymbolGroup *m_symbolGroup,
                                    SymbolParameterVector *vec,
                                    std::string *errorMessage);

    CIDebugSymbolGroup * const m_symbolGroup;
    const unsigned m_frame;
    const ULONG m_threadId;
    SymbolGroupNode *const m_root;
};

// Debug output visitor.
class DebugSymbolGroupNodeVisitor : public SymbolGroupNodeVisitor {
public:
    explicit DebugSymbolGroupNodeVisitor(std::ostream &os, unsigned verbosity = 0);

private:
    virtual bool visit(const SymbolGroupNode *node, unsigned child, unsigned depth, ULONG index);

    std::ostream &m_os;
    const unsigned m_verbosity;
};

// Gdbmi dump output visitor.
class DumpSymbolGroupNodeVisitor : public SymbolGroupNodeVisitor {
public:
    explicit DumpSymbolGroupNodeVisitor(std::ostream &os, bool humanReadable);

private:
    virtual bool visit(const SymbolGroupNode *node, unsigned child, unsigned depth, ULONG index);
    virtual void childrenVisited(const SymbolGroupNode *  node, unsigned depth);

    std::ostream &m_os;
    const bool m_humanReadable;
};

#endif // SYMBOLGROUP_H
