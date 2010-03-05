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

#ifndef SYMBOLGROUPCONTEXT_H
#define SYMBOLGROUPCONTEXT_H

#include "cdbcom.h"

#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtCore/QList>
#include <QtCore/QStringList>
#include <QtCore/QPair>
#include <QtCore/QMap>
#include <QtCore/QSet>

namespace CdbCore {

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

class SymbolGroupContext
{
    Q_DISABLE_COPY(SymbolGroupContext);
protected:
    explicit SymbolGroupContext(const QString &prefix,
                                CIDebugSymbolGroup *symbolGroup,
                                CIDebugDataSpaces *dataSpaces,
                                const QStringList &uninitializedVariables = QStringList());
    bool init(QString *errorMessage);

public:
    virtual ~SymbolGroupContext();
    static SymbolGroupContext *create(const QString &prefix,
                                      CIDebugSymbolGroup *symbolGroup,
                                      CIDebugDataSpaces *dataSpaces,
                                      const QStringList &uninitializedVariables,
                                      QString *errorMessage);

    QString prefix() const { return m_prefix; }
    int size() const { return m_symbolParameters.size(); }

    // Format a shadowed variable name/iname using a format taking two arguments:
    // "x <shadowed n"
    QString shadowedNameFormat() const;
    void setShadowedNameFormat(const QString &);

    bool assignValue(const QString &iname, const QString &value,
                     QString *newValue /* = 0 */, QString *errorMessage);

    bool lookupPrefix(const QString &prefix, unsigned long *index) const;

    enum SymbolState { LeafSymbol, ExpandedSymbol, CollapsedSymbol };
    SymbolState symbolState(unsigned long index) const;
    SymbolState symbolState(const QString &prefix) const;

    inline bool isExpanded(unsigned long index) const   { return symbolState(index) == ExpandedSymbol; }
    inline bool isExpanded(const QString &prefix) const { return symbolState(prefix) == ExpandedSymbol; }

    // Dump name/type of an entry running the internal dumpers for known types
    // May expand symbols.

    enum ValueFlags {
        HasChildren = 0x1,
        OutOfScope = 0x2,
        InternalDumperSucceeded = 0x4,
        InternalDumperError = 0x8, // Hard error
        InternalDumperFailed = 0x10,
        InternalDumperMask = InternalDumperSucceeded|InternalDumperError|InternalDumperFailed
    };

    unsigned dumpValue(unsigned long index, QString *inameIn, QString *nameIn,                  ULONG64 *addrIn,
                       ULONG *typeIdIn, QString *typeNameIn, QString *valueIn);

    // For 32bit values (returned as dec)
    static bool getDecimalIntValue(CIDebugSymbolGroup *sg, int index, int *value);
    // For pointers and 64bit values (returned as hex)
    static bool getUnsignedHexValue(QString stringValue, quint64 *value);
    // Null-check for pointers
    static bool isNullPointer(const QString &type , QString valueS);
    // Symbol group values may contain template types which is not desired.
    static QString removeInnerTemplateType(QString value);

    QString debugToString(bool verbose = false) const;
    QString toString(); // calls dump/potentially expands

    // Filter out locale variables and arguments
    inline static bool isSymbolDisplayable(const DEBUG_SYMBOL_PARAMETERS &p);

protected:
    bool getChildSymbolsPosition(const QString &prefix,
                                 unsigned long *startPos,
                                 unsigned long *parentId,
                                 QString *errorMessage);

    const DEBUG_SYMBOL_PARAMETERS &symbolParameterAt(int i) const { return m_symbolParameters.at(i); }

private:
    typedef QMap<QString, unsigned long>  NameIndexMap;

    void clear();
    bool expandSymbol(const QString &prefix, unsigned long index, QString *errorMessage);
    void populateINameIndexMap(const QString &prefix, unsigned long parentId, unsigned long end);
    QString symbolINameAt(unsigned long index) const;
    inline QString formatShadowedName(const QString &name, int n) const;

    // Raw dump of an entry (without dumpers)
    unsigned dumpValueRaw(unsigned long index, QString *inameIn, QString *nameIn,
                          ULONG64 *addrIn, ULONG *typeIdIn, QString *typeNameIn,
                          QString *valueIn) const;

    int dumpQString(unsigned long index, const QString &inameIn, QString *valueIn);
    int dumpStdString(unsigned long index, const QString &inameIn, QString *valueIn);

    inline DEBUG_SYMBOL_PARAMETERS *symbolParameters() { return &(*m_symbolParameters.begin()); }
    inline const DEBUG_SYMBOL_PARAMETERS *symbolParameters() const { return &(*m_symbolParameters.constBegin()); }

    const QString m_prefix;
    const QChar m_nameDelimiter;
    const QSet<QString> m_uninitializedVariables;    

    CIDebugSymbolGroup *m_symbolGroup;
    CIDebugDataSpaces *m_dataSpaces;
    NameIndexMap m_inameIndexMap;
    QVector<DEBUG_SYMBOL_PARAMETERS> m_symbolParameters;
    int m_unnamedSymbolNumber;
    QString m_shadowedNameFormat;
};

// Filter out locale variables and arguments
bool SymbolGroupContext::isSymbolDisplayable(const DEBUG_SYMBOL_PARAMETERS &p)
{
    if (p.Flags & (DEBUG_SYMBOL_IS_LOCAL|DEBUG_SYMBOL_IS_ARGUMENT))
        return true;
    // Do not display static members.
    if (p.Flags & DEBUG_SYMBOL_READ_ONLY)
        return false;
    return true;
}

} // namespace CdbCore

#endif // SYMBOLGROUPCONTEXT_H
