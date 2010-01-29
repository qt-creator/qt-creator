/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef CDBSYMBOLGROUPCONTEXT_H
#define CDBSYMBOLGROUPCONTEXT_H

#include "cdbcom.h"
#include "watchhandler.h"

#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtCore/QList>
#include <QtCore/QStringList>
#include <QtCore/QPair>
#include <QtCore/QMap>
#include <QtCore/QSet>

namespace Debugger {
namespace Internal {

class WatchData;
class WatchHandler;
struct CdbSymbolGroupRecursionContext;

/* A thin wrapper around the IDebugSymbolGroup2 interface which represents
 * a flat list of symbols using an index (for example, belonging to a stack
 * frame). It uses the hierarchical naming convention of WatchHandler as in:
 * "local" (invisible root)
 * "local.string" (local class variable)
 * "local.string.data" (class member)
 * and maintains a mapping iname -> index.
 * IDebugSymbolGroup2 can "expand" expandable symbols, inserting them into the
 * flat list after their parent.
 *
 * Note the pecularity of IDebugSymbolGroup2 with regard to pointed to items:
 * 1) A pointer to a POD (say int *) will expand to a pointed-to integer named '*'.
 * 2) A pointer to a class (QString *), will expand to the class members right away,
 *    omitting the '*' derefenced item. That is a problem since the dumpers trigger
 *    only on the derefenced item, so, additional handling is required.
 */

class CdbSymbolGroupContext
{
    Q_DISABLE_COPY(CdbSymbolGroupContext);
     explicit CdbSymbolGroupContext(const QString &prefix,
                                    CIDebugSymbolGroup *symbolGroup,
                                    const QStringList &uninitializedVariables = QStringList());

public:
    ~CdbSymbolGroupContext();
    static CdbSymbolGroupContext *create(const QString &prefix,
                                         CIDebugSymbolGroup *symbolGroup,
                                         const QStringList &uninitializedVariables,
                                         QString *errorMessage);

    QString prefix() const { return m_prefix; }

    bool assignValue(const QString &iname, const QString &value,
                     QString *newValue /* = 0 */, QString *errorMessage);

    // Initially populate the locals model for a new stackframe.
    // Write a sequence of WatchData to it, recurse down if the
    // recursionPredicate agrees. The ignorePredicate can be used
    // to terminate processing after insertion of an item (if the calling
    // routine wants to insert another subtree).
    template <class OutputIterator, class RecursionPredicate, class IgnorePredicate>
    static bool populateModelInitially(const CdbSymbolGroupRecursionContext &ctx,
                                       OutputIterator it,
                                       RecursionPredicate recursionPredicate,
                                       IgnorePredicate ignorePredicate,
                                       QString *errorMessage);

    // Complete children of a WatchData item.
    // Write a sequence of WatchData to it, recurse if the
    // recursionPredicate agrees. The ignorePredicate can be used
    // to terminate processing after insertion of an item (if the calling
    // routine wants to insert another subtree).
    template <class OutputIterator, class RecursionPredicate, class IgnorePredicate>
    static bool completeData (const CdbSymbolGroupRecursionContext &ctx,
                              WatchData incompleteLocal,
                              OutputIterator it,
                              RecursionPredicate recursionPredicate,
                              IgnorePredicate ignorePredicate,
                              QString *errorMessage);

    // Retrieve child symbols of prefix as a sequence of WatchData.
    template <class OutputIterator>
            bool getChildSymbols(const QString &prefix, OutputIterator it, QString *errorMessage)
            { return getDumpChildSymbols(0, prefix, 0, it, errorMessage); }
    // Retrieve child symbols of prefix as a sequence of WatchData.
    // Is CIDebugDataSpaces is != 0, try internal dumper and set owner
    template <class OutputIterator>
            bool getDumpChildSymbols(CIDebugDataSpaces *ds, const QString &prefix,
                                      int dumpedOwner,
                                      OutputIterator it, QString *errorMessage);

    WatchData watchDataAt(unsigned long index) const;
    // Run the internal dumpers on the symbol
    WatchData dumpSymbolAt(CIDebugDataSpaces *ds, unsigned long index);

    bool lookupPrefix(const QString &prefix, unsigned long *index) const;

    enum SymbolState { LeafSymbol, ExpandedSymbol, CollapsedSymbol };
    SymbolState symbolState(unsigned long index) const;
    SymbolState symbolState(const QString &prefix) const;

    inline bool isExpanded(unsigned long index) const   { return symbolState(index) == ExpandedSymbol; }
    inline bool isExpanded(const QString &prefix) const { return symbolState(prefix) == ExpandedSymbol; }

    // Helper to convert a DEBUG_VALUE structure to a string representation
    static QString debugValueToString(const DEBUG_VALUE &dv, CIDebugControl *ctl, QString *type = 0, int integerBase = 10);
    static bool debugValueToInteger(const DEBUG_VALUE &dv, qint64 *value);

    // format an array of unsigned longs as "0x323, 0x2322, ..."
    static QString hexFormatArray(const unsigned short *array, int size);

    // Dump
    enum DumperResult { DumperOk, DumperError, DumperNotHandled };
    DumperResult dump(CIDebugDataSpaces *ds, WatchData *wd);

private:
    typedef QMap<QString, unsigned long>  NameIndexMap;

    static inline bool isSymbolDisplayable(const DEBUG_SYMBOL_PARAMETERS &p);

    bool init(QString *errorMessage);
    void clear();
    QString toString(bool verbose = false) const;
    bool getChildSymbolsPosition(const QString &prefix,
                                 unsigned long *startPos,
                                 unsigned long *parentId,
                                 QString *errorMessage);
    bool expandSymbol(const QString &prefix, unsigned long index, QString *errorMessage);
    void populateINameIndexMap(const QString &prefix, unsigned long parentId, unsigned long end);
    QString symbolINameAt(unsigned long index) const;

    int dumpQString(CIDebugDataSpaces *ds, WatchData *wd);
    int dumpStdString(WatchData *wd);

    inline DEBUG_SYMBOL_PARAMETERS *symbolParameters() { return &(*m_symbolParameters.begin()); }
    inline const DEBUG_SYMBOL_PARAMETERS *symbolParameters() const { return &(*m_symbolParameters.constBegin()); }

    const QString m_prefix;
    const QChar m_nameDelimiter;
    const QSet<QString> m_uninitializedVariables;

    CIDebugSymbolGroup *m_symbolGroup;
    NameIndexMap m_inameIndexMap;
    QVector<DEBUG_SYMBOL_PARAMETERS> m_symbolParameters;
    int m_unnamedSymbolNumber;
};


// A convenience struct to save parameters for the model recursion.
struct CdbSymbolGroupRecursionContext {
    explicit CdbSymbolGroupRecursionContext(CdbSymbolGroupContext *ctx, int internalDumperOwner, CIDebugDataSpaces *ds);

    CdbSymbolGroupContext *context;
    int internalDumperOwner;
    CIDebugDataSpaces *dataspaces;
};

// Helper to a sequence of  WatchData into a list.
class WatchDataBackInserter
{
public:
    explicit WatchDataBackInserter(QList<WatchData> &wh) : m_wh(wh) {}

    inline WatchDataBackInserter & operator*() { return *this; }
    inline WatchDataBackInserter &operator=(const WatchData &wd) {
        m_wh.push_back(wd);
        return *this;
    }
    inline WatchDataBackInserter &operator++() { return *this; }

private:
    QList<WatchData> &m_wh;
};

} // namespace Internal
} // namespace Debugger

#include "cdbsymbolgroupcontext_tpl.h"

#endif // CDBSYMBOLGROUPCONTEXT_H
