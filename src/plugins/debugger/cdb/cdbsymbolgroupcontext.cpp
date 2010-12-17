/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "cdbsymbolgroupcontext.h"
#include "cdbengine_p.h"
#include "cdbdumperhelper.h"
#include "watchhandler.h"
#include "watchutils.h"
#include "debuggeractions.h"
#include "coreengine.h"
#include "debuggercore.h"

#include <QtCore/QDebug>
#include <QtCore/QTextStream>
#include <QtCore/QCoreApplication>
#include <QtCore/QRegExp>

enum { debug = 0 };
enum { debugInternalDumpers = 0 };

namespace Debugger {
namespace Internal {

enum { OwnerNewItem, OwnerSymbolGroup, OwnerSymbolGroupDumper , OwnerDumper };

typedef QSharedPointer<CdbDumperHelper> SharedPointerCdbDumperHelper;
typedef QList<WatchData> WatchDataList;

// Predicates for parametrizing the symbol group
inline bool truePredicate(const WatchData & /* whatever */) { return true; }
inline bool falsePredicate(const WatchData & /* whatever */) { return false; }
inline bool isDumperPredicate(const WatchData &wd)
{ return (wd.source & CdbSymbolGroupContext::SourceMask) == OwnerDumper; }

static inline void debugWatchDataList(const QList<WatchData> &l, const char *why = 0)
{
    QDebug nospace = qDebug().nospace();
    if (why)
        nospace << why << '\n';
    foreach(const WatchData &wd, l)
        nospace << wd.toString() << '\n';
    nospace << '\n';
}

// Match an item that is expanded in the watchhandler.
class WatchHandlerExpandedPredicate {
public:
    explicit inline WatchHandlerExpandedPredicate(const WatchHandler *wh) : m_wh(wh) {}
    inline bool operator()(const WatchData &wd) { return m_wh->isExpandedIName(wd.iname); }
private:
    const WatchHandler *m_wh;
};

// Match an item by iname
class MatchINamePredicate {
public:
    explicit inline MatchINamePredicate(const QString &iname) : m_iname(iname) {}
    inline bool operator()(const WatchData &wd) { return wd.iname == m_iname; }
private:
    const QString &m_iname;
};

// Put a sequence of  WatchData into the model for the non-dumper case
class WatchHandlerModelInserter {
public:
    explicit WatchHandlerModelInserter(WatchHandler *wh) : m_wh(wh) {}

    inline WatchHandlerModelInserter & operator*() { return *this; }
    inline WatchHandlerModelInserter &operator=(const WatchData &wd)  {
        m_wh->insertData(wd);
        return *this;
    }
    inline WatchHandlerModelInserter &operator++() { return *this; }

private:
    WatchHandler *m_wh;
};

// Put a sequence of  WatchData into the model for the dumper case.
// Sorts apart a sequence of  WatchData using the Dumpers.
// Puts the stuff for which there is no dumper in the model
// as is and sets ownership to symbol group. The rest goes
// to the dumpers. Additionally, checks for items pointing to a
// dumpeable type and inserts a fake dereferenced item and value.
class WatchHandleDumperInserter {
public:
    explicit WatchHandleDumperInserter(WatchHandler *wh,
                                        const SharedPointerCdbDumperHelper &dumper);

    inline WatchHandleDumperInserter & operator*() { return *this; }
    inline WatchHandleDumperInserter &operator=(WatchData &wd);
    inline WatchHandleDumperInserter &operator++() { return *this; }

private:
    bool expandPointerToDumpable(const WatchData &wd, QString *errorMessage);

    const QRegExp m_hexNullPattern;
    WatchHandler *m_wh;
    const SharedPointerCdbDumperHelper m_dumper;
    QList<WatchData> m_dumperResult;
};

WatchHandleDumperInserter::WatchHandleDumperInserter(WatchHandler *wh,
                                                       const SharedPointerCdbDumperHelper &dumper) :
    m_hexNullPattern(QLatin1String("0x0+")),
    m_wh(wh),
    m_dumper(dumper)
{
    Q_ASSERT(m_hexNullPattern.isValid());
}

// Prevent recursion of the model by setting value and type
static inline bool fixDumperType(WatchData *wd, const WatchData *source = 0)
{
    const bool missing = wd->isTypeNeeded() || wd->type.isEmpty();
    if (missing) {
        static const QByteArray unknownType =
                QCoreApplication::translate("CdbSymbolGroupContext", "<Unknown Type>")
                        .toUtf8();
        wd->setType(source ? source->type : unknownType, false);
    }
    return missing;
}

static inline bool fixDumperValue(WatchData *wd, const WatchData *source = 0)
{
    const bool missing = wd->isValueNeeded();
    if (missing) {
        if (source && source->isValueKnown()) {
            wd->setValue(source->value);
        } else {
            static const QString unknownValue = QCoreApplication::translate("CdbSymbolGroupContext", "<Unknown Value>");
            wd->setValue(unknownValue);
        }
    }
    return missing;
}

// When querying an item, the queried item is sometimes returned in incomplete form.
// Take over values from source.
static inline void fixDumperResult(const WatchData &source,
                                   QList<WatchData> *result,
                                   bool suppressGrandChildren)
{

    const int size = result->size();
    if (!size)
        return;
    if (debugCDBWatchHandling)
        debugWatchDataList(*result, suppressGrandChildren ? ">fixDumperResult suppressGrandChildren" : ">fixDumperResult");
    WatchData &returned = result->front();
    if (returned.iname != source.iname)
        return;
    fixDumperType(&returned, &source);
    fixDumperValue(&returned, &source);
    // Indicate owner and known children
    returned.source = OwnerDumper;
    if (returned.isChildrenKnown() && returned.isHasChildrenKnown() && returned.hasChildren)
        returned.source |= CdbSymbolGroupContext::ChildrenKnownBit;
    if (size == 1)
        return;
    // If the model queries the expanding item by pretending childrenNeeded=1,
    // refuse the request as the children are already known
    returned.source |= CdbSymbolGroupContext::ChildrenKnownBit;
    // Fix the children: Assign sort id , if the address is missing, we cannot query any further.
    const int resultSize = result->size();
    for (int i = 1; i < resultSize; i++) {
        WatchData &wd = (*result)[i];
        wd.sortId = i;
        // Indicate owner and known children
        wd.source = OwnerDumper;
        if (wd.isChildrenKnown() && wd.isHasChildrenKnown() && wd.hasChildren)
            wd.source |= CdbSymbolGroupContext::ChildrenKnownBit;
        // Cannot dump items with missing addresses or missing types
        const bool typeFixed = fixDumperType(&wd); // Order of evaluation!
        if ((wd.address == 0 && wd.isSomethingNeeded()) || typeFixed) {
            wd.setHasChildren(false);
            wd.setAllUnneeded();
        } else {
            // Hack: Suppress endless recursion of the model. To be fixed,
            // the model should not query non-visible items.
            if (suppressGrandChildren && (wd.isChildrenNeeded() || wd.isHasChildrenNeeded()))
                wd.setHasChildren(false);
        }
    }
    if (debugCDBWatchHandling)
        debugWatchDataList(*result, "<fixDumperResult");
}

// Is this a non-null pointer to a dumpeable item with a value
// "0x4343 class QString *" ? - Insert a fake '*' dereferenced item
// and run dumpers on it. If that succeeds, insert the fake items owned by dumpers,
// which will trigger the ignore predicate.
// Note that the symbol context does not create '*' dereferenced items for
// classes (see note in its header documentation).
bool WatchHandleDumperInserter::expandPointerToDumpable(const WatchData &wd, QString *errorMessage)
{
    if (debugCDBWatchHandling > 1)
        qDebug() << ">expandPointerToDumpable" << wd.toString();

    bool handled = false;
    do {
        // Must be a clas pointer item and a non-null-pointer.
        if (wd.error ||  !(wd.source & CdbSymbolGroupContext::ClassPointerBit)
            || !isPointerType(wd.type))
            break;
        quint64 address = 0;
        if (!CdbCore::SymbolGroupContext::getUnsignedHexValue(wd.value, &address) || address == 0)
            break;
        const QByteArray type = stripPointerType(wd.type);
        WatchData derefedWd;
        derefedWd.setType(type);
        derefedWd.address = address;
        derefedWd.name = QString(QLatin1Char('*'));
        derefedWd.iname = wd.iname + ".*";
        derefedWd.source = OwnerDumper | CdbSymbolGroupContext::ChildrenKnownBit;
        const CdbDumperHelper::DumpResult dr = m_dumper->dumpType(derefedWd, true, &m_dumperResult, errorMessage);
        if (dr != CdbDumperHelper::DumpOk)
            break;
        fixDumperResult(derefedWd, &m_dumperResult, false);
        // Insert the pointer item with 1 additional child + its dumper results
        // Note: formal arguments might already be expanded in the symbol group.
        WatchData ptrWd = wd;
        ptrWd.source = OwnerDumper | CdbSymbolGroupContext::ChildrenKnownBit;
        ptrWd.setHasChildren(true);
        ptrWd.setChildrenUnneeded();
        m_wh->insertData(ptrWd);
        m_wh->insertBulkData(m_dumperResult);
        handled = true;
    } while (false);
    if (debugCDBWatchHandling > 1)
        qDebug() << "<expandPointerToDumpable returns " << handled << *errorMessage;
    return handled;
}

WatchHandleDumperInserter &WatchHandleDumperInserter::operator=(WatchData &wd)
{
    if (debugCDBWatchHandling)
        qDebug() << "WatchHandleDumperInserter::operator=" << wd.toString();
    // Check pointer to dumpeable, dumpeable, insert accordingly.
    QString errorMessage;
    if (expandPointerToDumpable(wd, &errorMessage)) {
        // Nasty side effect: Modify owner for the ignore predicate
        wd.source = OwnerDumper;
        return *this;
    }
    // Expanded by internal dumper? : ok
    if ((wd.source & CdbSymbolGroupContext::SourceMask) == OwnerSymbolGroupDumper) {
        m_wh->insertData(wd);
        return *this;
    }
    // Try library dumpers.
    switch (m_dumper->dumpType(wd, true, &m_dumperResult, &errorMessage)) {
    case CdbDumperHelper::DumpOk:
        if (debugCDBWatchHandling)
            qDebug() << "dumper triggered";
        // Dumpers omit types for complicated templates
        fixDumperResult(wd, &m_dumperResult, false);
        // Discard the original item and insert the dumper results
        m_wh->insertBulkData(m_dumperResult);
        // Nasty side effect: Modify owner for the ignore predicate
        wd.source = OwnerDumper;
        break;
    case CdbDumperHelper::DumpNotHandled:
    case CdbDumperHelper::DumpError:
        wd.source = OwnerSymbolGroup;
        m_wh->insertData(wd);
        break;
    }
    return *this;
}


CdbSymbolGroupRecursionContext::CdbSymbolGroupRecursionContext(CdbSymbolGroupContext *ctx,
                                                         int ido) :
    context(ctx),
    internalDumperOwner(ido)
{
}

static inline CdbSymbolGroupContext::SymbolState getSymbolState(const DEBUG_SYMBOL_PARAMETERS &p)
{
    if (p.SubElements == 0u)
        return CdbSymbolGroupContext::LeafSymbol;
    return (p.Flags & DEBUG_SYMBOL_EXPANDED) ?
               CdbSymbolGroupContext::ExpandedSymbol :
               CdbSymbolGroupContext::CollapsedSymbol;
}

CdbSymbolGroupContext::CdbSymbolGroupContext(const QString &prefix,
                                             CIDebugSymbolGroup *symbolGroup,
                                             const QSharedPointer<CdbDumperHelper> &dumper,
                                             const QStringList &uninitializedVariables) :
    CdbCore::SymbolGroupContext(prefix, symbolGroup,
                                dumper->comInterfaces()->debugDataSpaces,
                                uninitializedVariables),
    m_useDumpers(dumper->isEnabled() && debuggerCore()->boolSetting(UseDebuggingHelpers)),
    m_dumper(dumper)
{
    setShadowedNameFormat(WatchData::shadowedNameFormat());
}

CdbSymbolGroupContext *CdbSymbolGroupContext::create(const QString &prefix,
                                                     CIDebugSymbolGroup *symbolGroup,
                                                     const QSharedPointer<CdbDumperHelper> &dumper,
                                                     const QStringList &uninitializedVariables,
                                                     QString *errorMessage)
{
    CdbSymbolGroupContext *rc = new CdbSymbolGroupContext(prefix, symbolGroup,
                                                          dumper,
                                                          uninitializedVariables);
    if (!rc->init(errorMessage)) {
        delete rc;
        return 0;
    }
    return rc;
}

// Fix display values: Pass through strings, convert unsigned integers
// to decimal ('0x5454`fedf'), remove inner templates from
// "0x4343 class list<>".
static inline QString fixValue(const QString &value, const QByteArray &type, bool *isClassPointer)
{
    *isClassPointer = false;
    // Pass through complete strings. Note that char pointers
    // (0x00A "hallo") will be reformatted below.
    const QChar doubleQuote = QLatin1Char('"');
    if (value.startsWith(doubleQuote) && value.endsWith(doubleQuote))
        return value;
    // Real Integer numbers Unsigned hex numbers (0x)/decimal numbers (0n)
    if (type != "bool" && isIntType(type)) {
        const QVariant intValue = CdbCore::SymbolGroupContext::getIntValue(value);
        if (intValue.isValid())
            return intValue.toString();
    }
    /* Pointers: Fix the address and strip off lengthy class type specifications
     * "0x00000000`000045AA "hallo"                 -> "0x45AA "hallo"
     * "0x00000000`000045AA class Bla<std::....> *" -> "0x45AA" */
    if (isPointerType(type)) {
        quint64 ptrValue;
        int endPos;
        if (CdbCore::SymbolGroupContext::getUnsignedHexValue(value, &ptrValue, &endPos)) {
            QString fixedValue = QString::fromAscii("0x%1").arg(ptrValue, 0, 16);
            // What is the second token...
            if (endPos < value.size() - 1) {
                const QString token = value.mid(endPos + 1);
                if (token.startsWith(QLatin1String("class")) || token.startsWith(QLatin1String("struct"))) {
                    *isClassPointer = true;
                } else {
                    fixedValue += QLatin1Char(' ');
                    fixedValue += token;
                }
            } // has token
            return fixedValue;
        } // pointer value conversion ok
    }
    return value;
}

unsigned CdbSymbolGroupContext::watchDataAt(unsigned long index, WatchData *wd)
{
    ULONG typeId;
    ULONG64 address;
    QString iname;
    QString value;
    QString type;
    const unsigned rc = dumpValue(index, &iname, &(wd->name), &address,
                                  &typeId, &type, &value);
    wd->exp = wd->iname = iname.toLatin1();
    wd->setAddress(address);
    wd->setType(type.toUtf8(), false);
    if (rc & OutOfScope) {
        wd->setError(WatchData::msgNotInScope());
    } else {
        bool isClassPointer;
        wd->setValue(fixValue(value, type.toUtf8(), &isClassPointer));
        if (isClassPointer)
            wd->source |= CdbSymbolGroupContext::ClassPointerBit;
        const bool hasChildren = rc & HasChildren;
        wd->setHasChildren(hasChildren);
        if (hasChildren)
            wd->setChildrenNeeded();
    }
    if (debug > 1)
        qDebug() << "watchDataAt" << index << QString::number(rc, 16) << wd->toString();
    return rc;
}

bool CdbSymbolGroupContext::populateModelInitially(WatchHandler *wh, QString *errorMessage)
{
    if (debugCDBWatchHandling)
        qDebug() << ">populateModelInitially dumpers=" << m_useDumpers;
    // Recurse down items that are initially expanded in the view, stop processing for
    // dumper items.
    const CdbSymbolGroupRecursionContext rctx(this, OwnerSymbolGroupDumper);
    const bool rc = m_useDumpers ?
        populateModelInitiallyHelper(rctx,
                                     WatchHandleDumperInserter(wh, m_dumper),
                                     WatchHandlerExpandedPredicate(wh),
                                     isDumperPredicate,
                                     errorMessage) :
        populateModelInitiallyHelper(rctx,
                                     WatchHandlerModelInserter(wh),
                                     WatchHandlerExpandedPredicate(wh),
                                     falsePredicate,
                                     errorMessage);
    if (debugCDBWatchHandling)
        qDebug("<populateModelInitially\n%s\n", qPrintable(toString()));
    return rc;
}

bool CdbSymbolGroupContext::completeData(const WatchData &incompleteLocal,
                                         WatchHandler *wh,
                                         QString *errorMessage)
{
    if (debugCDBWatchHandling)
        qDebug(">completeData src=%d %s\n%s\n",
               incompleteLocal.source, qPrintable(incompleteLocal.toString()),
               qPrintable(toString()));

    const CdbSymbolGroupRecursionContext rctx(this, OwnerSymbolGroupDumper);
    // Expand symbol group items, recurse one level from desired item
    if (!m_useDumpers) {
        return completeDataHelper(rctx, incompleteLocal,
                                  WatchHandlerModelInserter(wh),
                                  MatchINamePredicate(incompleteLocal.iname),
                                  falsePredicate,
                                  errorMessage);
    }

    // Expand artifical dumper items
    if ((incompleteLocal.source & CdbSymbolGroupContext::SourceMask) == OwnerDumper) {
        // If the model queries the expanding item by pretending childrenNeeded=1,
        // refuse the request if the children are already known
        if (incompleteLocal.state == WatchData::ChildrenNeeded && (incompleteLocal.source & CdbSymbolGroupContext::ChildrenKnownBit)) {
            WatchData local = incompleteLocal;
            local.setChildrenUnneeded();
            wh->insertData(local);
            return true;
        }
        QList<WatchData> dumperResult;
        const CdbDumperHelper::DumpResult dr = m_dumper->dumpType(incompleteLocal, true, &dumperResult, errorMessage);
        if (dr == CdbDumperHelper::DumpOk) {
            // Hack to stop endless model recursion
            const bool suppressGrandChildren = !wh->isExpandedIName(incompleteLocal.iname);
            fixDumperResult(incompleteLocal, &dumperResult, suppressGrandChildren);
            wh->insertBulkData(dumperResult);
        } else {
            const QString msg = QString::fromLatin1("Unable to further expand dumper watch data: '%1' (%2): %3/%4")
                .arg(incompleteLocal.name)
                .arg(QString::fromUtf8(incompleteLocal.type))
                .arg(int(dr)).arg(*errorMessage);
            qWarning("%s", qPrintable(msg));
            WatchData wd = incompleteLocal;
            if (wd.isValueNeeded())
                wd.setValue(QCoreApplication::translate("CdbSymbolGroupContext", "<Unknown>"));
            wd.setHasChildren(false);
            wd.setAllUnneeded();
            wh->insertData(wd);
        }
        return true;
    }

    // Expand symbol group items, recurse one level from desired item
    return completeDataHelper(rctx, incompleteLocal,
                              WatchHandleDumperInserter(wh, m_dumper),
                              MatchINamePredicate(incompleteLocal.iname),
                              isDumperPredicate,
                              errorMessage);
}

bool CdbSymbolGroupContext::editorToolTip(const QString &iname,
                                         QString *value,
                                         QString *errorMessage)
{
    if (debugToolTips)
        qDebug() << iname;
    value->clear();
    unsigned long index;
    if (!lookupPrefix(iname, &index)) {
        *errorMessage = QString::fromLatin1("%1 not found.").arg(iname);
        return false;
    }
    // Check dumpers. Should actually be just one item.

    WatchData wd;
    const unsigned rc = watchDataAt(index, &wd);
    if (m_useDumpers && !wd.error
        && (0u == (rc & CdbCore::SymbolGroupContext::InternalDumperMask))
        && m_dumper->state() != CdbDumperHelper::Disabled) {
        QList<WatchData> result;
        if (CdbDumperHelper::DumpOk == m_dumper->dumpType(wd, false, &result, errorMessage))  {
            foreach (const WatchData &dwd, result) {
                if (!value->isEmpty())
                    value->append(QLatin1Char('\n'));
                value->append(dwd.toToolTip());
            }
            return true;
        } // Dumped ok
    }     // has Dumpers
    if (debugToolTips)
        qDebug() << iname << wd.toString();
    *value = wd.toToolTip();
    return true;
}

} // namespace Internal
} // namespace Debugger
