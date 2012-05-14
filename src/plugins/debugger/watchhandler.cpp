/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "watchhandler.h"

#include "breakhandler.h"
#include "debuggerinternalconstants.h"
#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerengine.h"
#include "debuggerdialogs.h"
#include "watchutils.h"

#if USE_WATCH_MODEL_TEST
#include "modeltest.h"
#endif

#include <utils/qtcassert.h>
#include <utils/savedaction.h>

#include <cplusplus/CppRewriter.h>

#include <QDebug>
#include <QEvent>
#include <QFile>
#include <QProcess>
#include <QTextStream>
#include <QtAlgorithms>

#include <QLabel>
#include <QTextEdit>

#include <ctype.h>
#include <utils/qtcassert.h>


namespace Debugger {
namespace Internal {

// Creates debug output for accesses to the model.
enum { debugModel = 0 };

#define MODEL_DEBUG(s) do { if (debugModel) qDebug() << s; } while (0)
#define MODEL_DEBUGX(s) qDebug() << s

static QHash<QByteArray, int> theWatcherNames;
static QHash<QByteArray, int> theTypeFormats;
static QHash<QByteArray, int> theIndividualFormats;
static int theUnprintableBase = -1;


static QByteArray stripForFormat(const QByteArray &ba)
{
    QByteArray res;
    res.reserve(ba.size());
    int inArray = 0;
    for (int i = 0; i != ba.size(); ++i) {
        const char c = ba.at(i);
        if (c == '<')
            break;
        if (c == '[')
            ++inArray;
        if (c == ']')
            --inArray;
        if (c == ' ')
            continue;
        if (inArray && c >= '0' && c <= '9')
            continue;
        res.append(c);
    }
    return res;
}

void WatchHandler::setUnprintableBase(int base)
{
    theUnprintableBase = base;
    emitAllChanged();
}

int WatchHandler::unprintableBase()
{
    return theUnprintableBase;
}

////////////////////////////////////////////////////////////////////
//
// WatchItem
//
////////////////////////////////////////////////////////////////////

class WatchItem : public WatchData
{
public:
    WatchItem() { parent = 0; }

    ~WatchItem() {
        if (parent != 0)
            parent->children.removeOne(this);
        qDeleteAll(children);
    }

    WatchItem(const WatchData &data) : WatchData(data)
        { parent = 0; }

    void setData(const WatchData &data)
        { static_cast<WatchData &>(*this) = data; }

    WatchItem *parent;
    QList<WatchItem *> children;  // fetched children
};


///////////////////////////////////////////////////////////////////////
//
// WatchModel
//
///////////////////////////////////////////////////////////////////////

WatchModel::WatchModel(WatchHandler *handler, WatchType type)
    : QAbstractItemModel(handler), m_generationCounter(0),
      m_handler(handler), m_type(type)
{
    m_root = new WatchItem;
    m_root->hasChildren = 1;
    m_root->state = 0;
    m_root->name = WatchHandler::tr("Root");
    m_root->parent = 0;

    switch (m_type) {
        case ReturnWatch:
            m_root->iname = "return";
            m_root->name = WatchHandler::tr("Return Value");
            break;
        case LocalsWatch:
            m_root->iname = "local";
            m_root->name = WatchHandler::tr("Locals");
            break;
        case WatchersWatch:
            m_root->iname = "watch";
            m_root->name = WatchHandler::tr("Expressions");
            break;
        case TooltipsWatch:
            m_root->iname = "tooltip";
            m_root->name = WatchHandler::tr("Tooltip");
            break;
        case InspectWatch:
            m_root->iname = "inspect";
            m_root->name = WatchHandler::tr("Inspector");
            break;
    }
}

WatchModel::~WatchModel()
{
    delete m_root;
}

WatchItem *WatchModel::rootItem() const
{
    return m_root;
}

void WatchModel::reinitialize()
{
    int n = m_root->children.size();
    if (n == 0)
        return;
    //MODEL_DEBUG("REMOVING " << n << " CHILDREN OF " << m_root->iname);
    QModelIndex index = watchIndex(m_root);
    beginRemoveRows(index, 0, n - 1);
    qDeleteAll(m_root->children);
    m_root->children.clear();
    endRemoveRows();
}

void WatchModel::emitAllChanged()
{
    emit layoutChanged();
}

void WatchModel::beginCycle(bool fullCycle)
{
    if (fullCycle)
        m_generationCounter++;

    //emit enableUpdates(false);
}

void WatchModel::endCycle()
{
    removeOutdated();
    m_fetchTriggered.clear();
    //emit enableUpdates(true);
}

DebuggerEngine *WatchModel::engine() const
{
    return m_handler->m_engine;
}

void WatchModel::dump()
{
    qDebug() << "\n";
    foreach (WatchItem *child, m_root->children)
        dumpHelper(child);
}

void WatchModel::dumpHelper(WatchItem *item)
{
    qDebug() << "ITEM: " << item->iname
        << (item->parent ? item->parent->iname : "<none>")
        << item->generation;
    foreach (WatchItem *child, item->children)
        dumpHelper(child);
}

void WatchModel::removeOutdated()
{
    foreach (WatchItem *child, m_root->children)
        removeOutdatedHelper(child);
#if DEBUG_MODEL
#if USE_WATCH_MODEL_TEST
    (void) new ModelTest(this, this);
#endif
#endif
}

void WatchModel::removeOutdatedHelper(WatchItem *item)
{
    if (item->generation < m_generationCounter) {
        destroyItem(item);
    } else {
        foreach (WatchItem *child, item->children)
            removeOutdatedHelper(child);
    }
}

void WatchModel::destroyItem(WatchItem *item)
{
    WatchItem *parent = item->parent;
    QModelIndex index = watchIndex(parent);
    int n = parent->children.indexOf(item);
    //MODEL_DEBUG("NEED TO REMOVE: " << item->iname << "AT" << n);
    beginRemoveRows(index, n, n);
    parent->children.removeAt(n);
    endRemoveRows();
    delete item;
}

void WatchModel::reinsertAllData()
{
    QList<WatchData> list;
    reinsertAllDataHelper(m_root, &list);
    reinitialize();
    foreach (WatchItem data, list) {
        data.setAllUnneeded();
        insertData(data);
    }
    layoutChanged();
}

void WatchModel::reinsertAllDataHelper(WatchItem *item, QList<WatchData> *data)
{
    data->append(*item);
    foreach (WatchItem *child, item->children)
        reinsertAllDataHelper(child, data);
}

static QByteArray parentName(const QByteArray &iname)
{
    int pos = iname.lastIndexOf('.');
    if (pos == -1)
        return QByteArray();
    return iname.left(pos);
}

static QString niceTypeHelper(const QByteArray &typeIn)
{
    typedef QMap<QByteArray, QString> Cache;
    static Cache cache;
    const Cache::const_iterator it = cache.constFind(typeIn);
    if (it != cache.constEnd())
        return it.value();
    const QString simplified = CPlusPlus::simplifySTLType(QLatin1String(typeIn));
    cache.insert(typeIn, simplified); // For simplicity, also cache unmodified types
    return simplified;
}

QString WatchModel::removeNamespaces(QString str) const
{
    if (!debuggerCore()->boolSetting(ShowStdNamespace))
        str.remove(QLatin1String("std::"));
    if (!debuggerCore()->boolSetting(ShowQtNamespace)) {
        const QString qtNamespace = QString::fromLatin1(engine()->qtNamespace());
        if (!qtNamespace.isEmpty())
            str.remove(qtNamespace);
    }
    return str;
}

QString WatchModel::removeInitialNamespace(QString str) const
{
    if (str.startsWith(QLatin1String("std::"))
            && debuggerCore()->boolSetting(ShowStdNamespace))
        str = str.mid(5);
    if (!debuggerCore()->boolSetting(ShowQtNamespace)) {
        const QByteArray qtNamespace = engine()->qtNamespace();
        if (!qtNamespace.isEmpty() && str.startsWith(QLatin1String(qtNamespace)))
            str = str.mid(qtNamespace.size());
    }
    return str;
}

QString WatchModel::displayType(const WatchData &data) const
{
    QString base = data.displayedType.isEmpty()
        ? niceTypeHelper(data.type)
        : data.displayedType;
    if (data.bitsize)
        base += QString::fromLatin1(":%1").arg(data.bitsize);
    base.remove(QLatin1Char('\''));
    return base;
}

static int formatToIntegerBase(int format)
{
    switch (format) {
        case HexadecimalFormat:
            return 16;
        case BinaryFormat:
            return 2;
        case OctalFormat:
            return 8;
    }
    return 10;
}

template <class IntType> QString reformatInteger(IntType value, int format)
{
    switch (format) {
        case HexadecimalFormat:
            return QLatin1String("(hex) ") + QString::number(value, 16);
        case BinaryFormat:
            return QLatin1String("(bin) ") + QString::number(value, 2);
        case OctalFormat:
            return QLatin1String("(oct) ") + QString::number(value, 8);
    }
    return QString::number(value); // not reached
}

// Format printable (char-type) characters
static QString reformatCharacter(int code, int format)
{
    const QString codeS = reformatInteger(code, format);
    if (code < 0) // Append unsigned value.
        return codeS + QLatin1String(" / ") + reformatInteger(256 + code, format);
    const QChar c = QLatin1Char(code);
    if (c.isPrint())
        return codeS + QLatin1String(" '") + c + QLatin1Char('\'');
    switch (code) {
    case 0:
        return codeS + QLatin1String(" '\\0'");
    case '\r':
        return codeS + QLatin1String(" '\\r'");
    case '\t':
        return codeS + QLatin1String(" '\\t'");
    case '\n':
        return codeS + QLatin1String(" '\\n'");
    }
    return codeS;
}

static QString quoteUnprintable(const QString &str)
{
    if (WatchHandler::unprintableBase() == 0)
        return str;

    QString encoded;
    if (WatchHandler::unprintableBase() == -1) {
        foreach (const QChar c, str) {
            int u = c.unicode();
            if (c.isPrint())
                encoded += c;
            else if (u == '\r')
                encoded += QLatin1String("\\r");
            else if (u == '\t')
                encoded += QLatin1String("\\t");
            else if (u == '\n')
                encoded += QLatin1String("\\n");
            else
                encoded += QString::fromLatin1("\\%1")
                    .arg(c.unicode(), 3, 8, QLatin1Char('0'));
        }
        return encoded;
    }

    foreach (const QChar c, str) {
        if (c.isPrint()) {
            encoded += c;
        } else if (WatchHandler::unprintableBase() == 8) {
            encoded += QString::fromLatin1("\\%1")
                .arg(c.unicode(), 3, 8, QLatin1Char('0'));
        } else {
            encoded += QString::fromLatin1("\\u%1")
                .arg(c.unicode(), 4, 16, QLatin1Char('0'));
        }
    }
    return encoded;
}

static QString translate(const QString &str)
{
    if (str.startsWith(QLatin1Char('<'))) {
        if (str == QLatin1String("<Edit>"))
            return WatchHandler::tr("<Edit>");
        if (str == QLatin1String("<empty>"))
            return WatchHandler::tr("<empty>");
        if (str == QLatin1String("<uninitialized>"))
            return WatchHandler::tr("<uninitialized>");
        if (str == QLatin1String("<invalid>"))
            return WatchHandler::tr("<invalid>");
        if (str == QLatin1String("<not accessible>"))
            return WatchHandler::tr("<not accessible>");
        if (str.endsWith(QLatin1String(" items>"))) {
            // '<10 items>' or '<>10 items>' (more than)
            bool ok;
            const bool moreThan = str.at(1) == QLatin1Char('>');
            const int numberPos = moreThan ? 2 : 1;
            const int len = str.indexOf(QLatin1Char(' ')) - numberPos;
            const int size = str.mid(numberPos, len).toInt(&ok);
            QTC_ASSERT(ok, qWarning("WatchHandler: Invalid item count '%s'",
                qPrintable(str)));
            return moreThan ?
                     WatchHandler::tr("<more than %n items>", 0, size) :
                     WatchHandler::tr("<%n items>", 0, size);
        }
    }
    return quoteUnprintable(str);
}

QString WatchModel::formattedValue(const WatchData &data) const
{
    const QString &value = data.value;

    if (data.type == "bool") {
        if (value == QLatin1String("0"))
            return QLatin1String("false");
        if (value == QLatin1String("1"))
            return QLatin1String("true");
        return value;
    }

    if (isIntType(data.type)) {
        if (value.isEmpty())
            return value;
        // Do not reformat booleans (reported as 'true, false').
        const QChar firstChar = value.at(0);
        if (!firstChar.isDigit() && firstChar != QLatin1Char('-'))
            return value;

        // Append quoted, printable character also for decimal.
        const int format = itemFormat(data);
        if (data.type.endsWith("char")) {
            bool ok;
            const int code = value.toInt(&ok);
            return ok ? reformatCharacter(code, format) : value;
        }
        // Rest: Leave decimal as is
        if (format <= 0)
            return value;
        // Evil hack, covers 'unsigned' as well as quint64.
        if (data.type.contains('u'))
            return reformatInteger(value.toULongLong(0, 0), format);
        return reformatInteger(value.toLongLong(), format);
    }

    if (data.type == "va_list")
        return value;

    if (!isPointerType(data.type) && !data.isVTablePointer()) {
        bool ok = false;
        qulonglong integer = value.toULongLong(&ok, 0);
        if (ok) {
            const int format = itemFormat(data);
            return reformatInteger(integer, format);
        }
    }

    return translate(value);
}

// Get a pointer address from pointer values reported by the debugger.
// Fix CDB formatting of pointers "0x00000000`000003fd class foo *", or
// "0x00000000`000003fd "Hallo"", or check gdb formatting of characters.
static inline quint64 pointerValue(QString data)
{
    const int blankPos = data.indexOf(QLatin1Char(' '));
    if (blankPos != -1)
        data.truncate(blankPos);
    data.remove(QLatin1Char('`'));
    return data.toULongLong(0, 0);
}

// Return the type used for editing
static inline int editType(const WatchData &d)
{
    if (d.type == "bool")
        return QVariant::Bool;
    if (isIntType(d.type))
        return d.type.contains('u') ? QVariant::ULongLong : QVariant::LongLong;
    if (isFloatType(d.type))
        return QVariant::Double;
    // Check for pointers using hex values (0xAD00 "Hallo")
    if (isPointerType(d.type) && d.value.startsWith(QLatin1String("0x")))
        return QVariant::ULongLong;
   return QVariant::String;
}

// Convert to editable (see above)
static inline QVariant editValue(const WatchData &d)
{
    switch (editType(d)) {
    case QVariant::Bool:
        return d.value != QLatin1String("0") && d.value != QLatin1String("false");
    case QVariant::ULongLong:
        if (isPointerType(d.type)) // Fix pointer values (0xAD00 "Hallo" -> 0xAD00)
            return QVariant(pointerValue(d.value));
        return QVariant(d.value.toULongLong());
    case QVariant::LongLong:
        return QVariant(d.value.toLongLong());
    case QVariant::Double:
        return QVariant(d.value.toDouble());
    default:
        break;
    }
    // Some string value: '0x434 "Hallo"':
    // Remove quotes and replace newlines, which will cause line edit troubles.
    QString stringValue = d.value;
    if (stringValue.endsWith(QLatin1Char('"'))) {
        const int leadingDoubleQuote = stringValue.indexOf(QLatin1Char('"'));
        if (leadingDoubleQuote != stringValue.size() - 1) {
            stringValue.truncate(stringValue.size() - 1);
            stringValue.remove(0, leadingDoubleQuote + 1);
            stringValue.replace(QLatin1String("\n"), QLatin1String("\\n"));
        }
    }
    return QVariant(translate(stringValue));
}

bool WatchModel::canFetchMore(const QModelIndex &index) const
{
    WatchItem *item = watchItem(index);
    QTC_ASSERT(item, return false);
    return index.isValid() && contentIsValid() && !m_fetchTriggered.contains(item->iname);
}

void WatchModel::fetchMore(const QModelIndex &index)
{
    QTC_ASSERT(index.isValid(), return);
    WatchItem *item = watchItem(index);
    QTC_ASSERT(item, return);
    QTC_ASSERT(!m_fetchTriggered.contains(item->iname), return);
    m_handler->m_expandedINames.insert(item->iname);
    m_fetchTriggered.insert(item->iname);
    if (item->children.isEmpty()) {
        WatchData data = *item;
        data.setChildrenNeeded();
        WatchUpdateFlags flags;
        flags.tryIncremental = true;
        engine()->updateWatchData(data, flags);
    }
}

QModelIndex WatchModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    const WatchItem *item = watchItem(parent);
    QTC_ASSERT(item, return QModelIndex());
    if (row >= item->children.size())
        return QModelIndex();
    return createIndex(row, column, (void*)(item->children.at(row)));
}

QModelIndex WatchModel::parent(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return QModelIndex();

    const WatchItem *item = watchItem(idx);
    if (!item->parent || item->parent == m_root)
        return QModelIndex();

    const WatchItem *grandparent = item->parent->parent;
    if (!grandparent)
        return QModelIndex();

    for (int i = 0; i < grandparent->children.size(); ++i)
        if (grandparent->children.at(i) == item->parent)
            return createIndex(i, 0, (void*) item->parent);

    return QModelIndex();
}

int WatchModel::rowCount(const QModelIndex &idx) const
{
    if (idx.column() > 0)
        return 0;
    return watchItem(idx)->children.size();
}

int WatchModel::columnCount(const QModelIndex &idx) const
{
    Q_UNUSED(idx)
    return 3;
}

bool WatchModel::hasChildren(const QModelIndex &parent) const
{
    WatchItem *item = watchItem(parent);
    return !item || item->hasChildren;
}

WatchItem *WatchModel::watchItem(const QModelIndex &idx) const
{
    return idx.isValid()
        ? static_cast<WatchItem*>(idx.internalPointer()) : m_root;
}

QModelIndex WatchModel::watchIndex(const WatchItem *item) const
{
    return watchIndexHelper(item, m_root, QModelIndex());
}

QModelIndex WatchModel::watchIndexHelper(const WatchItem *needle,
    const WatchItem *parentItem, const QModelIndex &parentIndex) const
{
    if (needle == parentItem)
        return parentIndex;
    for (int i = parentItem->children.size(); --i >= 0; ) {
        const WatchItem *childItem = parentItem->children.at(i);
        QModelIndex childIndex = index(i, 0, parentIndex);
        QModelIndex idx = watchIndexHelper(needle, childItem, childIndex);
        if (idx.isValid())
            return idx;
    }
    return QModelIndex();
}

void WatchModel::emitDataChanged(int column, const QModelIndex &parentIndex)
{
    QModelIndex idx1 = index(0, column, parentIndex);
    QModelIndex idx2 = index(rowCount(parentIndex) - 1, column, parentIndex);
    if (idx1.isValid() && idx2.isValid())
        emit dataChanged(idx1, idx2);
    //qDebug() << "CHANGING:\n" << idx1 << "\n" << idx2 << "\n"
    //    << data(parentIndex, INameRole).toString();
    for (int i = rowCount(parentIndex); --i >= 0; )
        emitDataChanged(column, index(i, 0, parentIndex));
}

void WatchModel::invalidateAll(const QModelIndex &parentIndex)
{
    QModelIndex idx1 = index(0, 0, parentIndex);
    QModelIndex idx2 = index(rowCount(parentIndex) - 1, columnCount(parentIndex) - 1, parentIndex);
    if (idx1.isValid() && idx2.isValid())
        emit dataChanged(idx1, idx2);
}

// Truncate value for item view, maintaining quotes.
static QString truncateValue(QString v)
{
    enum { maxLength = 512 };
    if (v.size() < maxLength)
        return v;
    const bool isQuoted = v.endsWith(QLatin1Char('"')); // check for 'char* "Hallo"'
    v.truncate(maxLength);
    v += isQuoted ? QLatin1String("...\"") : QLatin1String("...");
    return v;
}

int WatchModel::itemFormat(const WatchData &data) const
{
    const int individualFormat = theIndividualFormats.value(data.iname, -1);
    if (individualFormat != -1)
        return individualFormat;
    return theTypeFormats.value(stripForFormat(data.type), -1);
}

bool WatchModel::contentIsValid() const
{
    // inspector doesn't follow normal beginCycle()/endCycle()
    if (m_type == InspectWatch)
        return true;
    return m_handler->m_contentsValid;
}

static inline QString expression(const WatchItem *item)
{
    if (!item->exp.isEmpty())
         return QString::fromLatin1(item->exp);
    if (item->address && !item->type.isEmpty()) {
        return QString::fromLatin1("*(%1*)%2").
                arg(QLatin1String(item->type), QLatin1String(item->hexAddress()));
    }
    if (const WatchItem *parent = item->parent) {
        if (!parent->exp.isEmpty())
           return QString::fromLatin1("(%1).%2")
            .arg(QString::fromLatin1(parent->exp), item->name);
    }
    return QString();
}

QString WatchModel::display(const WatchItem *item, int col) const
{
    QString result;
    switch (col) {
        case 0:
            if (m_type == WatchersWatch && item->name.isEmpty())
                result = tr("<Edit>");
            else if (m_type == ReturnWatch && item->iname.count('.') == 1)
                result = tr("returned value");
            else if (item->name == QLatin1String("*") && item->parent)
                result = QLatin1Char('*') + item->parent->name;
            else
                result = removeInitialNamespace(item->name);
            break;
        case 1:
            result = removeInitialNamespace(
                truncateValue(formattedValue(*item)));
            if (item->referencingAddress) {
                result += QLatin1String(" @");
                result += QString::fromLatin1(item->hexReferencingAddress());
            }
            break;
        case 2:
            result = removeNamespaces(displayType(*item));
            break;
        default:
            break;
    }
    return result;
}

QString WatchModel::displayForAutoTest(const QByteArray &iname) const
{
    const WatchItem *item = findItem(iname, m_root);
    if (item)
        return display(item, 1) + QLatin1Char(' ') + display(item, 2);
    return QString();
}

QVariant WatchModel::data(const QModelIndex &idx, int role) const
{
    const WatchItem *item = watchItem(idx);
    const WatchItem &data = *item;

    switch (role) {
        case LocalsEditTypeRole:
            return QVariant(editType(data));

        case LocalsNameRole:
            return QVariant(data.name);

        case LocalsIntegerBaseRole:
            if (isPointerType(data.type)) // Pointers using 0x-convention
                return QVariant(16);
            return QVariant(formatToIntegerBase(itemFormat(data)));

        case Qt::EditRole: {
            switch (idx.column()) {
                case 0:
                    return QVariant(expression(item));
                case 1:
                    return editValue(data);
                case 2:
                    // FIXME:: To be tested: Can debuggers handle those?
                    if (!data.displayedType.isEmpty())
                        return data.displayedType;
                    return QString::fromUtf8(data.type);
                default:
                    break;
            }
        }

        case Qt::DisplayRole:
            return display(item, idx.column());

        case Qt::ToolTipRole:
            return debuggerCore()->boolSetting(UseToolTipsInLocalsView)
                ? data.toToolTip() : QVariant();

        case Qt::ForegroundRole: {
            static const QVariant red(QColor(200, 0, 0));
            static const QVariant gray(QColor(140, 140, 140));
            switch (idx.column()) {
                case 1: return (!data.valueEnabled || !contentIsValid()) ? gray
                            : data.changed ? red : QVariant();
            }
            break;
        }

        case LocalsExpressionRole:
            return QVariant(expression(item));

        case LocalsRawExpressionRole:
            return data.exp;

        case LocalsINameRole:
            return data.iname;

        case LocalsExpandedRole:
            return m_handler->m_expandedINames.contains(data.iname);

        case LocalsTypeFormatListRole:
            return m_handler->typeFormatList(data);

        case LocalsTypeRole:
            return removeNamespaces(displayType(data));

        case LocalsRawTypeRole:
            return QString::fromLatin1(data.type);

        case LocalsTypeFormatRole:
            return theTypeFormats.value(stripForFormat(data.type), -1);

        case LocalsIndividualFormatRole:
            return theIndividualFormats.value(data.iname, -1);

        case LocalsRawValueRole:
            return data.value;

        case LocalsPointerValueRole:
            return data.referencingAddress;

        case LocalsIsWatchpointAtAddressRole: {
            BreakpointParameters bp(WatchpointAtAddress);
            bp.address = data.coreAddress();
            return engine()->breakHandler()->findWatchpoint(bp) != 0;
        }

        case LocalsAddressRole:
            return QVariant(data.coreAddress());
        case LocalsReferencingAddressRole:
            return QVariant(data.referencingAddress);
        case LocalsSizeRole:
            return QVariant(data.size);

        case LocalsIsWatchpointAtPointerValueRole:
            if (isPointerType(data.type)) {
                BreakpointParameters bp(WatchpointAtAddress);
                bp.address = pointerValue(data.value);
                return engine()->breakHandler()->findWatchpoint(bp) != 0;
            }
            return false;

        default:
            break;
    }
    return QVariant();
}

bool WatchModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    WatchItem &data = *watchItem(index);

    switch (role) {
        case Qt::EditRole:
            switch (index.column()) {
            case 0: // Watch expression: See delegate.
                break;
            case 1: // Change value
                engine()->assignValueInDebugger(&data, expression(&data), value);
                break;
            case 2: // TODO: Implement change type.
                engine()->assignValueInDebugger(&data, expression(&data), value);
                break;
            }
        case LocalsExpandedRole:
            if (value.toBool()) {
                // Should already have been triggered by fetchMore()
                //QTC_CHECK(m_handler->m_expandedINames.contains(data.iname));
                m_handler->m_expandedINames.insert(data.iname);
            } else {
                m_handler->m_expandedINames.remove(data.iname);
            }
            break;

        case LocalsTypeFormatRole:
            m_handler->setFormat(data.type, value.toInt());
            engine()->updateWatchData(data);
            break;

        case LocalsIndividualFormatRole: {
            const int format = value.toInt();
            if (format == -1) {
                theIndividualFormats.remove(data.iname);
            } else {
                theIndividualFormats[data.iname] = format;
            }
            engine()->updateWatchData(data);
            break;
        }
    }

    emit dataChanged(index, index);
    return true;
}

Qt::ItemFlags WatchModel::flags(const QModelIndex &idx) const
{
    if (!contentIsValid())
        return Qt::ItemFlags();

    if (!idx.isValid())
        return Qt::ItemFlags();

    // Enabled, editable, selectable, checkable, and can be used both as the
    // source of a drag and drop operation and as a drop target.

    static const Qt::ItemFlags notEditable
        = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    static const Qt::ItemFlags editable = notEditable | Qt::ItemIsEditable;

    // Disable editing if debuggee is positively running.
    const bool isRunning = engine() && engine()->state() == InferiorRunOk;
    if (isRunning && engine() && !engine()->hasCapability(AddWatcherWhileRunningCapability))
        return notEditable;

    const WatchData &data = *watchItem(idx);
    if (data.isWatcher()) {
        if (idx.column() == 0 && data.iname.count('.') == 1)
            return editable; // Watcher names are editable.

        if (!data.name.isEmpty()) {
            // FIXME: Forcing types is not implemented yet.
            //if (idx.column() == 2)
            //    return editable; // Watcher types can be set by force.
            if (idx.column() == 1 && data.valueEditable)
                return editable; // Watcher values are sometimes editable.
        }
    } else if (data.isLocal()) {
        if (idx.column() == 1 && data.valueEditable)
            return editable; // Locals values are sometimes editable.
    }
    return notEditable;
}

QVariant WatchModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical)
        return QVariant();
    if (role == Qt::DisplayRole) {
        switch (section) {
            case 0: return QString(tr("Name")  + QLatin1String("     "));
            case 1: return QString(tr("Value") + QLatin1String("     "));
            case 2: return QString(tr("Type")  + QLatin1String("     "));
        }
    }
    return QVariant();
}

QStringList WatchHandler::typeFormatList(const WatchData &data) const
{
    if (data.referencingAddress || isPointerType(data.type))
        return QStringList()
            << tr("Raw pointer")
            << tr("Latin1 string")
            << tr("UTF8 string")
            << tr("Local 8bit string")
            << tr("UTF16 string")
            << tr("UCS4 string");
    if (data.type.contains("char[") || data.type.contains("char ["))
        return QStringList()
            << tr("Latin1 string")
            << tr("UTF8 string")
            << tr("Local 8bit string");
    bool ok = false;
    (void)data.value.toULongLong(&ok, 0);
    if ((isIntType(data.type) && data.type != "bool") || ok)
        return QStringList()
            << tr("Decimal")
            << tr("Hexadecimal")
            << tr("Binary")
            << tr("Octal");
    // Hack: Compensate for namespaces.
    QString type = QLatin1String(stripForFormat(data.type));
    int pos = type.indexOf(QLatin1String("::Q"));
    if (pos >= 0 && type.count(QLatin1Char(':')) == 2)
        type.remove(0, pos + 2);
    pos = type.indexOf(QLatin1Char('<'));
    if (pos >= 0)
        type.truncate(pos);
    type.replace(QLatin1Char(':'), QLatin1Char('_'));
    return m_reportedTypeFormats.value(type);
}

// Determine sort order of watch items by sort order or alphabetical inames
// according to setting 'SortStructMembers'. We need a map key for bulkInsert
// and a predicate for finding the insertion position of a single item.

// Set this before using any of the below according to action
static bool sortWatchDataAlphabetically = true;

static bool watchDataLessThan(const QByteArray &iname1, int sortId1,
    const QByteArray &iname2, int sortId2)
{
    if (!sortWatchDataAlphabetically)
        return sortId1 < sortId2;
    // Get positions of last part of iname 'local.this.i1" -> "i1"
    int cmpPos1 = iname1.lastIndexOf('.');
    if (cmpPos1 == -1) {
        cmpPos1 = 0;
    } else {
        cmpPos1++;
    }
    int cmpPos2 = iname2.lastIndexOf('.');
    if (cmpPos2 == -1) {
        cmpPos2 = 0;
    } else {
        cmpPos2++;
    }
    // Are we looking at an array with numerical inames 'local.this.i1.0" ->
    // Go by sort id.
    if (cmpPos1 < iname1.size() && cmpPos2 < iname2.size()
            && isdigit(iname1.at(cmpPos1)) && isdigit(iname2.at(cmpPos2)))
        return sortId1 < sortId2;
    // Alphabetically
    return qstrcmp(iname1.constData() + cmpPos1, iname2.constData() + cmpPos2) < 0;
}

// Sort key for watch data consisting of iname and numerical sort id.
struct WatchDataSortKey
{
    explicit WatchDataSortKey(const WatchData &wd)
        : iname(wd.iname), sortId(wd.sortId) {}
    QByteArray iname;
    int sortId;
};

inline bool operator<(const WatchDataSortKey &k1, const WatchDataSortKey &k2)
{
    return watchDataLessThan(k1.iname, k1.sortId, k2.iname, k2.sortId);
}

bool watchItemSorter(const WatchItem *item1, const WatchItem *item2)
{
    return watchDataLessThan(item1->iname, item1->sortId, item2->iname, item2->sortId);
}

static int findInsertPosition(const QList<WatchItem *> &list, const WatchItem *item)
{
    sortWatchDataAlphabetically = debuggerCore()->boolSetting(SortStructMembers);
    const QList<WatchItem *>::const_iterator it =
        qLowerBound(list.begin(), list.end(), item, watchItemSorter);
    return it - list.begin();
}

void WatchModel::insertData(const WatchData &data)
{
    QTC_ASSERT(!data.iname.isEmpty(), qDebug() << data.toString(); return);
    WatchItem *parent = findItem(parentName(data.iname), m_root);
    if (!parent) {
        WatchData parent;
        parent.iname = parentName(data.iname);
        MODEL_DEBUG("\nFIXING MISSING PARENT FOR\n" << data.iname);
        if (!parent.iname.isEmpty())
            insertData(parent);
        return;
    }
    QModelIndex index = watchIndex(parent);
    if (WatchItem *oldItem = findItem(data.iname, parent)) {
        bool hadChildren = oldItem->hasChildren;
        // Overwrite old entry.
        bool hasChanged = oldItem->hasChanged(data);
        oldItem->setData(data);
        oldItem->changed = hasChanged;
        oldItem->generation = m_generationCounter;
        QModelIndex idx = watchIndex(oldItem);
        emit dataChanged(idx, idx.sibling(idx.row(), 2));

        // This works around https://bugreports.qt-project.org/browse/QTBUG-7115
        // by creating and destroying a dummy child item.
        if (!hadChildren && oldItem->hasChildren) {
            WatchData dummy = data;
            dummy.iname = data.iname + ".x";
            dummy.hasChildren = false;
            dummy.setAllUnneeded();
            insertData(dummy);
            destroyItem(findItem(dummy.iname, m_root));
        }
    } else {
        // Add new entry.
        WatchItem *item = new WatchItem(data);
        item->parent = parent;
        item->generation = m_generationCounter;
        item->changed = true;
        const int n = findInsertPosition(parent->children, item);
        beginInsertRows(index, n, n);
        parent->children.insert(n, item);
        endInsertRows();
    }
}

void WatchModel::insertBulkData(const QList<WatchData> &list)
{
#if 0
    for (int i = 0; i != list.size(); ++i)
        insertData(list.at(i));
    return;
#endif
    // This method does not properly insert items in proper "iname sort
    // order", leading to random removal of items in removeOutdated();

    //qDebug() << "WMI:" << list.toString();
    //static int bulk = 0;
    //foreach (const WatchItem &data, list)
    //    qDebug() << "BULK: " << ++bulk << data.toString();
    QTC_ASSERT(!list.isEmpty(), return);
    QByteArray parentIName = parentName(list.at(0).iname);
    WatchItem *parent = findItem(parentIName, m_root);
    if (!parent) {
        WatchData parent;
        parent.iname = parentIName;
        insertData(parent);
        MODEL_DEBUG("\nFIXING MISSING PARENT FOR\n" << list.at(0).iname);
        return;
    }
    QModelIndex index = watchIndex(parent);

    sortWatchDataAlphabetically = debuggerCore()->boolSetting(SortStructMembers);
    QMap<WatchDataSortKey, WatchData> newList;
    typedef QMap<WatchDataSortKey, WatchData>::iterator Iterator;
    foreach (const WatchItem &data, list)
        newList.insert(WatchDataSortKey(data), data);
    if (newList.size() != list.size()) {
        qDebug() << "LIST: ";
        foreach (const WatchItem &data, list)
            qDebug() << data.toString();
        qDebug() << "NEW LIST: ";
        foreach (const WatchItem &data, newList)
            qDebug() << data.toString();
        qDebug() << "P->CHILDREN: ";
        foreach (const WatchItem *item, parent->children)
            qDebug() << item->toString();
        qDebug()
            << "P->CHILDREN.SIZE: " << parent->children.size()
            << "NEWLIST SIZE: " << newList.size()
            << "LIST SIZE: " << list.size();
    }
    QTC_ASSERT(newList.size() == list.size(), return);

    foreach (WatchItem *oldItem, parent->children) {
        const WatchDataSortKey oldSortKey(*oldItem);
        Iterator it = newList.find(oldSortKey);
        if (it == newList.end()) {
            WatchData data = *oldItem;
            data.generation = m_generationCounter;
            newList.insert(oldSortKey, data);
        } else {
            it->changed = it->hasChanged(*oldItem);
            if (it->generation == -1)
                it->generation = m_generationCounter;
        }
    }

    for (Iterator it = newList.begin(); it != newList.end(); ++it) {
        qDebug() << "  NEW: " << it->iname;
    }

    // overwrite existing items
    Iterator it = newList.begin();
    QModelIndex idx = watchIndex(parent);
    const int oldCount = parent->children.size();
    for (int i = 0; i < oldCount; ++i, ++it) {
        if (!parent->children[i]->isEqual(*it)) {
            qDebug() << "REPLACING" << parent->children.at(i)->iname
                << " WITH " << it->iname << it->generation;
            parent->children[i]->setData(*it);
            if (parent->children[i]->generation == -1)
                parent->children[i]->generation = m_generationCounter;
            //emit dataChanged(idx.sibling(i, 0), idx.sibling(i, 2));
        } else {
            //qDebug() << "SKIPPING REPLACEMENT" << parent->children.at(i)->iname;
        }
    }
    emit dataChanged(idx.sibling(0, 0), idx.sibling(oldCount - 1, 2));

    // add new items
    if (oldCount < newList.size()) {
        beginInsertRows(index, oldCount, newList.size() - 1);
        //MODEL_DEBUG("INSERT : " << data.iname << data.value);
        for (int i = oldCount; i < newList.size(); ++i, ++it) {
            WatchItem *item = new WatchItem(*it);
            qDebug() << "ADDING" << it->iname;
            item->parent = parent;
            if (item->generation == -1)
                item->generation = m_generationCounter;
            item->changed = true;
            parent->children.append(item);
        }
        endInsertRows();
    }
    //qDebug() << "ITEMS: " << parent->children.size();
    dump();
}

WatchItem *WatchModel::findItem(const QByteArray &iname, WatchItem *root) const
{
    if (root->iname == iname)
        return root;
    for (int i = root->children.size(); --i >= 0; )
        if (WatchItem *item = findItem(iname, root->children.at(i)))
            return item;
    return 0;
}

static void debugRecursion(QDebug &d, const WatchItem *item, int depth)
{
    d << QString(2 * depth, QLatin1Char(' ')) << item->toString() << '\n';
    foreach (const WatchItem *child, item->children)
        debugRecursion(d, child, depth + 1);
}

QDebug operator<<(QDebug d, const WatchModel &m)
{
    QDebug nospace = d.nospace();
    if (m.m_root)
        debugRecursion(nospace, m.m_root, 0);
    return d;
}

void WatchModel::formatRequests(QByteArray *out, const WatchItem *item) const
{
    int format = theIndividualFormats.value(item->iname, -1);
    if (format == -1)
        format = theTypeFormats.value(stripForFormat(item->type), -1);
    if (format != -1)
        *out += item->iname + ":format=" + QByteArray::number(format) + ',';
    foreach (const WatchItem *child, item->children)
        formatRequests(out, child);
}

///////////////////////////////////////////////////////////////////////
//
// WatchHandler
//
///////////////////////////////////////////////////////////////////////

WatchHandler::WatchHandler(DebuggerEngine *engine)
{
    m_engine = engine;
    m_inChange = false;
    m_watcherCounter = debuggerCore()->sessionValue(QLatin1String("Watchers"))
            .toStringList().count();

    m_return = new WatchModel(this, ReturnWatch);
    m_locals = new WatchModel(this, LocalsWatch);
    m_watchers = new WatchModel(this, WatchersWatch);
    m_tooltips = new WatchModel(this, TooltipsWatch);
    m_inspect = new WatchModel(this, InspectWatch);

    m_contentsValid = false;
    m_resetLocationScheduled = false;

    connect(debuggerCore()->action(SortStructMembers), SIGNAL(valueChanged(QVariant)),
           SLOT(reinsertAllData()));
    connect(debuggerCore()->action(ShowStdNamespace), SIGNAL(valueChanged(QVariant)),
           SLOT(reinsertAllData()));
    connect(debuggerCore()->action(ShowQtNamespace), SIGNAL(valueChanged(QVariant)),
           SLOT(reinsertAllData()));
}

void WatchHandler::beginCycle(bool fullCycle)
{
    m_return->beginCycle(fullCycle);
    m_locals->beginCycle(fullCycle);
    m_watchers->beginCycle(fullCycle);
    m_tooltips->beginCycle(fullCycle);
    // don't sync m_inspect here: It's updated on it's own
}

void WatchHandler::endCycle()
{
    m_return->endCycle();
    m_locals->endCycle();
    m_watchers->endCycle();
    m_tooltips->endCycle();

    m_contentsValid = true;
    m_resetLocationScheduled = false;

    updateWatchersWindow();
}

void WatchHandler::beginCycle(WatchType type, bool fullCycle)
{
    model(type)->beginCycle(fullCycle);
}

void WatchHandler::endCycle(WatchType type)
{
    model(type)->endCycle();
}

void WatchHandler::cleanup()
{
    m_expandedINames.clear();
    theWatcherNames.remove(QByteArray());
    m_return->reinitialize();
    m_locals->reinitialize();
    m_tooltips->reinitialize();
    m_inspect->reinitialize();
    m_return->m_fetchTriggered.clear();
    m_locals->m_fetchTriggered.clear();
    m_watchers->m_fetchTriggered.clear();
    m_tooltips->m_fetchTriggered.clear();
    m_inspect->m_fetchTriggered.clear();
#if 1
    for (EditHandlers::ConstIterator it = m_editHandlers.begin();
            it != m_editHandlers.end(); ++it) {
        if (!it.value().isNull())
            delete it.value();
    }
    m_editHandlers.clear();
#endif
}

void WatchHandler::emitAllChanged()
{
    m_return->emitAllChanged();
    m_locals->emitAllChanged();
    m_watchers->emitAllChanged();
    m_tooltips->emitAllChanged();
    m_inspect->emitAllChanged();
}

void WatchHandler::insertData(const WatchData &data)
{
    MODEL_DEBUG("INSERTDATA: " << data.toString());
    if (!data.isValid()) {
        qWarning("%s:%d: Attempt to insert invalid watch item: %s",
            __FILE__, __LINE__, qPrintable(data.toString()));
        return;
    }

    if (data.isSomethingNeeded() && data.iname.contains(".")) {
        MODEL_DEBUG("SOMETHING NEEDED: " << data.toString());
        if (!m_engine->isSynchronous()
                || data.iname.startsWith("inspect.")) {
            WatchModel *model = modelForIName(data.iname);
            QTC_ASSERT(model, return);
            model->insertData(data);

            m_engine->updateWatchData(data);
        } else {
            m_engine->showMessage(QLatin1String("ENDLESS LOOP: SOMETHING NEEDED: ")
                + data.toString());
            WatchData data1 = data;
            data1.setAllUnneeded();
            data1.setValue(QLatin1String("<unavailable synchronous data>"));
            data1.setHasChildren(false);
            WatchModel *model = modelForIName(data.iname);
            QTC_ASSERT(model, return);
            model->insertData(data1);
        }
    } else {
        WatchModel *model = modelForIName(data.iname);
        QTC_ASSERT(model, return);
        MODEL_DEBUG("NOTHING NEEDED: " << data.toString());
        model->insertData(data);
        showEditValue(data);
    }
}

void WatchHandler::reinsertAllData()
{
    m_locals->reinsertAllData();
    m_watchers->reinsertAllData();
    m_tooltips->reinsertAllData();
    m_return->reinsertAllData();
    m_inspect->reinsertAllData();
}

// Bulk-insertion
void WatchHandler::insertBulkData(const QList<WatchData> &list)
{
#if 1
    foreach (const WatchItem &data, list)
        insertData(data);
    return;
#endif

    if (list.isEmpty())
        return;
    QMap<QByteArray, QList<WatchData> > hash;

    foreach (const WatchData &data, list) {
        // we insert everything, including incomplete stuff
        // to reduce the number of row add operations in the model.
        if (data.isValid()) {
            hash[parentName(data.iname)].append(data);
        } else {
            qWarning("%s:%d: Attempt to bulk-insert invalid watch item: %s",
                __FILE__, __LINE__, qPrintable(data.toString()));
        }
    }
    foreach (const QByteArray &parentIName, hash.keys()) {
        WatchModel *model = modelForIName(parentIName);
        QTC_ASSERT(model, return);
        model->insertBulkData(hash[parentIName]);
    }

    foreach (const WatchData &data, list) {
        if (data.isSomethingNeeded())
            m_engine->updateWatchData(data);
    }
}

void WatchHandler::removeData(const QByteArray &iname)
{
    WatchModel *model = modelForIName(iname);
    if (!model)
        return;
    WatchItem *item = model->findItem(iname, model->m_root);
    if (item)
        model->destroyItem(item);
}

QByteArray WatchHandler::watcherName(const QByteArray &exp)
{
    return "watch." + QByteArray::number(theWatcherNames[exp]);
}

void WatchHandler::watchExpression(const QString &exp)
{
    QTC_ASSERT(m_engine, return);
    // Do not insert the same entry more then once.
    if (theWatcherNames.value(exp.toLatin1()))
        return;

    // FIXME: 'exp' can contain illegal characters
    WatchData data;
    data.exp = exp.toLatin1();
    data.name = exp;
    theWatcherNames[data.exp] = m_watcherCounter++;
    saveWatchers();

    if (exp.isEmpty())
        data.setAllUnneeded();
    data.iname = watcherName(data.exp);
    if (m_engine->state() == DebuggerNotReady) {
        data.setAllUnneeded();
        data.setValue(QString(QLatin1Char(' ')));
        data.setHasChildren(false);
        insertData(data);
    } else if (m_engine->isSynchronous()) {
        m_engine->updateWatchData(data);
    } else {
        insertData(data);
    }
    updateWatchersWindow();
    emitAllChanged();
}

static void swapEndian(char *d, int nchar)
{
    QTC_ASSERT(nchar % 4 == 0, return);
    for (int i = 0; i < nchar; i += 4) {
        char c = d[i];
        d[i] = d[i + 3];
        d[i + 3] = c;
        c = d[i + 1];
        d[i + 1] = d[i + 2];
        d[i + 2] = c;
    }
}

void WatchHandler::showEditValue(const WatchData &data)
{
    const QByteArray key  = data.address ? data.hexAddress() : data.iname;
    QObject *w = m_editHandlers.value(key);
    if (data.editformat == 0x0) {
        m_editHandlers.remove(data.iname);
        delete w;
    } else if (data.editformat == 1 || data.editformat == 3) {
        // QImage
        QLabel *l = qobject_cast<QLabel *>(w);
        if (!l) {
            delete w;
            l = new QLabel;
            const QString title = data.address ?
                tr("%1 Object at %2").arg(QLatin1String(data.type),
                    QLatin1String(data.hexAddress())) :
                tr("%1 Object at Unknown Address").arg(QLatin1String(data.type));
            l->setWindowTitle(title);
            m_editHandlers[key] = l;
        }
        int width, height, format;
        QByteArray ba;
        uchar *bits;
        if (data.editformat == 1) {
            ba = QByteArray::fromHex(data.editvalue);
            const int *header = (int *)(ba.data());
            swapEndian(ba.data(), ba.size());
            bits = 12 + (uchar *)(ba.data());
            width = header[0];
            height = header[1];
            format = header[2];
        } else { // data.editformat == 3
            QTextStream ts(data.editvalue);
            QString fileName;
            ts >> width >> height >> format >> fileName;
            QFile f(fileName);
            f.open(QIODevice::ReadOnly);
            ba = f.readAll();
            bits = (uchar*)ba.data();
        }
        QImage im(bits, width, height, QImage::Format(format));

#if 1
        // Qt bug. Enforce copy of image data.
        QImage im2(im);
        im.detach();
#endif

        l->setPixmap(QPixmap::fromImage(im));
        l->resize(width, height);
        l->show();
    } else if (data.editformat == 2) {
        // Display QString in a separate widget.
        QTextEdit *t = qobject_cast<QTextEdit *>(w);
        if (!t) {
            delete w;
            t = new QTextEdit;
            m_editHandlers[key] = t;
        }
        QByteArray ba = QByteArray::fromHex(data.editvalue);
        QString str = QString::fromUtf16((ushort *)ba.constData(), ba.size()/2);
        t->setText(str);
        t->resize(400, 200);
        t->show();
    } else if (data.editformat == 4) {
        // Generic Process.
        int pos = data.editvalue.indexOf('|');
        QByteArray cmd = data.editvalue.left(pos);
        QByteArray input = data.editvalue.mid(pos + 1);
        QProcess *p = qobject_cast<QProcess *>(w);
        if (!p) {
            p = new QProcess;
            p->start(QLatin1String(cmd));
            p->waitForStarted();
            m_editHandlers[key] = p;
        }
        p->write(input + '\n');
    } else {
        QTC_ASSERT(false, qDebug() << "Display format: " << data.editformat);
    }
}

void WatchHandler::clearWatches()
{
    if (theWatcherNames.isEmpty())
        return;
    const QList<WatchItem *> watches = m_watchers->rootItem()->children;
    for (int i = watches.size() - 1; i >= 0; i--)
        m_watchers->destroyItem(watches.at(i));
    theWatcherNames.clear();
    m_watcherCounter = 0;
    updateWatchersWindow();
    emitAllChanged();
    saveWatchers();
}

void WatchHandler::removeWatchExpression(const QString &exp0)
{
    QByteArray exp = exp0.toLatin1();
    MODEL_DEBUG("REMOVE WATCH: " << exp);
    theWatcherNames.remove(exp);
    foreach (WatchItem *item, m_watchers->rootItem()->children) {
        if (item->exp == exp) {
            m_watchers->destroyItem(item);
            saveWatchers();
            updateWatchersWindow();
            emitAllChanged();
            break;
        }
    }
}

void WatchHandler::updateWatchersWindow()
{
    // Force show/hide of watchers and return view.
    debuggerCore()->updateWatchersWindow();
}

QStringList WatchHandler::watchedExpressions()
{
    // Filter out invalid watchers.
    QStringList watcherNames;
    QHashIterator<QByteArray, int> it(theWatcherNames);
    while (it.hasNext()) {
        it.next();
        const QByteArray &watcherName = it.key();
        if (!watcherName.isEmpty())
            watcherNames.push_back(QLatin1String(watcherName));
    }
    return watcherNames;
}

void WatchHandler::saveWatchers()
{
    debuggerCore()->setSessionValue(QLatin1String("Watchers"), QVariant(watchedExpressions()));
}

void WatchHandler::loadTypeFormats()
{
    QVariant value = debuggerCore()->sessionValue(QLatin1String("DefaultFormats"));
    QMap<QString, QVariant> typeFormats = value.toMap();
    QMapIterator<QString, QVariant> it(typeFormats);
    while (it.hasNext()) {
        it.next();
        if (!it.key().isEmpty())
            theTypeFormats.insert(it.key().toUtf8(), it.value().toInt());
    }
}

void WatchHandler::saveTypeFormats()
{
    QMap<QString, QVariant> typeFormats;
    QHashIterator<QByteArray, int> it(theTypeFormats);
    while (it.hasNext()) {
        it.next();
        const int format = it.value();
        if (format != DecimalFormat) {
            const QByteArray key = it.key().trimmed();
            if (!key.isEmpty())
                typeFormats.insert(QLatin1String(key), format);
        }
    }
    debuggerCore()->setSessionValue(QLatin1String("DefaultFormats"),
                                    QVariant(typeFormats));
}

void WatchHandler::saveSessionData()
{
    saveWatchers();
    saveTypeFormats();
}

void WatchHandler::loadSessionData()
{
    loadTypeFormats();
    theWatcherNames.clear();
    m_watcherCounter = 0;
    QVariant value = debuggerCore()->sessionValue(QLatin1String("Watchers"));
    foreach (WatchItem *item, m_watchers->rootItem()->children)
        m_watchers->destroyItem(item);
    foreach (const QString &exp, value.toStringList())
        watchExpression(exp);
    updateWatchersWindow();
    emitAllChanged();
}

void WatchHandler::updateWatchers()
{
    foreach (WatchItem *item, m_watchers->rootItem()->children)
        m_watchers->destroyItem(item);
    // Copy over all watchers and mark all watchers as incomplete.
    foreach (const QByteArray &exp, theWatcherNames.keys()) {
        WatchData data;
        data.iname = watcherName(exp);
        data.setAllNeeded();
        data.name = QLatin1String(exp);
        data.exp = exp;
        insertData(data);
    }
}

WatchModel *WatchHandler::model(WatchType type) const
{
    switch (type) {
        case ReturnWatch: return m_return;
        case LocalsWatch: return m_locals;
        case WatchersWatch: return m_watchers;
        case TooltipsWatch: return m_tooltips;
        case InspectWatch: return m_inspect;
    }
    QTC_CHECK(false);
    return 0;
}

WatchModel *WatchHandler::modelForIName(const QByteArray &iname) const
{
    if (iname.startsWith("return"))
        return m_return;
    if (iname.startsWith("local"))
        return m_locals;
    if (iname.startsWith("tooltip"))
        return m_tooltips;
    if (iname.startsWith("watch"))
        return m_watchers;
    if (iname.startsWith("inspect"))
        return m_inspect;
    QTC_ASSERT(false, qDebug() << "INAME: " << iname);
    return 0;
}

const WatchData *WatchHandler::watchData(WatchType type, const QModelIndex &index) const
{
    if (index.isValid())
        if (const WatchModel *m = model(type))
            return m->watchItem(index);
    return 0;
}

const WatchData *WatchHandler::findItem(const QByteArray &iname) const
{
    const WatchModel *model = modelForIName(iname);
    QTC_ASSERT(model, return 0);
    return model->findItem(iname, model->m_root);
}

QString WatchHandler::displayForAutoTest(const QByteArray &iname) const
{
    const WatchModel *model = modelForIName(iname);
    QTC_ASSERT(model, return QString());
    return model->displayForAutoTest(iname);
}

QModelIndex WatchHandler::itemIndex(const QByteArray &iname) const
{
    if (const WatchModel *model = modelForIName(iname))
        if (WatchItem *item = model->findItem(iname, model->m_root))
            return model->watchIndex(item);
    return QModelIndex();
}

void WatchHandler::setFormat(const QByteArray &type0, int format)
{
    const QByteArray type = stripForFormat(type0);
    if (format == -1)
        theTypeFormats.remove(type);
    else
        theTypeFormats[type] = format;
    saveTypeFormats();
    m_return->emitDataChanged(1);
    m_locals->emitDataChanged(1);
    m_watchers->emitDataChanged(1);
    m_tooltips->emitDataChanged(1);
    m_inspect->emitDataChanged(1);
}

int WatchHandler::format(const QByteArray &iname) const
{
    int result = -1;
    if (const WatchData *item = findItem(iname)) {
        int result = theIndividualFormats.value(item->iname, -1);
        if (result == -1)
            result = theTypeFormats.value(stripForFormat(item->type), -1);
    }
    return result;
}

QByteArray WatchHandler::expansionRequests() const
{
    QByteArray ba;
    //m_locals->formatRequests(&ba, m_locals->m_root);
    //m_watchers->formatRequests(&ba, m_watchers->m_root);
    if (!m_expandedINames.isEmpty()) {
        QSetIterator<QByteArray> jt(m_expandedINames);
        while (jt.hasNext()) {
            QByteArray iname = jt.next();
            ba.append(iname);
            ba.append(',');
        }
        ba.chop(1);
    }
    return ba;
}

QByteArray WatchHandler::typeFormatRequests() const
{
    QByteArray ba;
    if (!theTypeFormats.isEmpty()) {
        QHashIterator<QByteArray, int> it(theTypeFormats);
        while (it.hasNext()) {
            it.next();
            ba.append(it.key().toHex());
            ba.append('=');
            ba.append(QByteArray::number(it.value()));
            ba.append(',');
        }
        ba.chop(1);
    }
    return ba;
}

QByteArray WatchHandler::individualFormatRequests() const
{
    QByteArray ba;
    if (!theIndividualFormats.isEmpty()) {
        QHashIterator<QByteArray, int> it(theIndividualFormats);
        while (it.hasNext()) {
            it.next();
            ba.append(it.key());
            ba.append('=');
            ba.append(QByteArray::number(it.value()));
            ba.append(',');
        }
        ba.chop(1);
    }
    return ba;
}

void WatchHandler::addTypeFormats(const QByteArray &type, const QStringList &formats)
{
    m_reportedTypeFormats.insert(QLatin1String(stripForFormat(type)), formats);
}

QString WatchHandler::editorContents()
{
    QString contents;
    showInEditorHelper(&contents, m_locals->m_root, 0);
    showInEditorHelper(&contents, m_watchers->m_root, 0);
    return contents;
}

void WatchHandler::showInEditorHelper(QString *contents, WatchItem *item, int depth)
{
    const QChar tab = QLatin1Char('\t');
    const QChar nl = QLatin1Char('\n');
    contents->append(QString(depth, tab));
    contents->append(item->name);
    contents->append(tab);
    contents->append(item->value);
    contents->append(tab);
    contents->append(item->type);
    contents->append(nl);
    foreach (WatchItem *child, item->children)
       showInEditorHelper(contents, child, depth + 1);
}

void WatchHandler::removeTooltip()
{
    m_tooltips->reinitialize();
    m_tooltips->emitAllChanged();
}

void WatchHandler::rebuildModel()
{
    beginCycle();

    const QList<WatchItem *> watches = m_watchers->rootItem()->children;
    for (int i = watches.size() - 1; i >= 0; i--)
        m_watchers->destroyItem(watches.at(i));

    foreach (const QString &exp, watchedExpressions()) {
        WatchData data;
        data.exp = exp.toLatin1();
        data.name = exp;
        data.iname = watcherName(data.exp);
        data.setAllUnneeded();

        insertData(data);
    }

    endCycle();
}

void WatchHandler::setTypeFormats(const TypeFormats &typeFormats)
{
    m_reportedTypeFormats = typeFormats;
}

TypeFormats WatchHandler::typeFormats() const
{
    return m_reportedTypeFormats;
}

void WatchHandler::editTypeFormats(bool includeLocals, const QByteArray &iname)
{
    Q_UNUSED(includeLocals);
    TypeFormatsDialog dlg(0);

    //QHashIterator<QString, QStringList> it(m_reportedTypeFormats);
    QList<QString> l = m_reportedTypeFormats.keys();
    qSort(l.begin(), l.end());
    foreach (const QString &ba, l) {
        int f = iname.isEmpty() ? -1 : format(iname);
        dlg.addTypeFormats(ba, m_reportedTypeFormats.value(ba), f);
    }
    if (dlg.exec())
        setTypeFormats(dlg.typeFormats());
}

void WatchHandler::scheduleResetLocation()
{
    m_contentsValid = false;
    m_resetLocationScheduled = true;
}

void WatchHandler::resetLocation()
{
    if (m_resetLocationScheduled) {
        m_resetLocationScheduled = false;
        m_return->invalidateAll();
        m_locals->invalidateAll();
        m_watchers->invalidateAll();
        m_tooltips->invalidateAll();
        m_inspect->invalidateAll();
    }
}

bool WatchHandler::isValidToolTip(const QByteArray &iname) const
{
    WatchItem *item = m_tooltips->findItem(iname, m_tooltips->m_root);
    return item && !item->type.trimmed().isEmpty();
}

void WatchHandler::setCurrentModelIndex(WatchType modelType,
                                        const QModelIndex &index)
{
    if (WatchModel *m = model(modelType)) {
        emit m->setCurrentIndex(index);
    }
}

QHash<QByteArray, int> WatchHandler::watcherNames()
{
    return theWatcherNames;
}

} // namespace Internal
} // namespace Debugger
