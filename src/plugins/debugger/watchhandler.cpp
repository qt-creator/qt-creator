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

#include "watchhandler.h"
#include "watchutils.h"
#include "debuggeractions.h"
#include "debuggermanager.h"
#include "idebuggerengine.h"

#if USE_MODEL_TEST
#include "modeltest.h"
#endif

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QEvent>
#include <QtCore/QFile>
#include <QtCore/QProcess>
#include <QtCore/QTextStream>
#include <QtCore/QTimer>
#include <QtCore/QtAlgorithms>

#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QLabel>
#include <QtGui/QToolTip>
#include <QtGui/QTextEdit>

#include <ctype.h>


// creates debug output for accesses to the model
//#define DEBUG_MODEL 1

#if DEBUG_MODEL
#   define MODEL_DEBUG(s) qDebug() << s
#else
#   define MODEL_DEBUG(s)
#endif
#define MODEL_DEBUGX(s) qDebug() << s

namespace Debugger {
namespace Internal {

static const QString strNotInScope =
    QCoreApplication::translate("Debugger::Internal::WatchData", "<not in scope>");

static int watcherCounter = 0;
static int generationCounter = 0;

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

////////////////////////////////////////////////////////////////////
//
// WatchData
//
////////////////////////////////////////////////////////////////////

WatchData::WatchData() :
    editformat(0),
    hasChildren(false),
    generation(-1),
    valueEnabled(true),
    valueEditable(true),
    error(false),
    source(0),
    state(InitialState),
    changed(false)
{
}

bool WatchData::isEqual(const WatchData &other) const
{
    return iname == other.iname
      && exp == other.exp
      && name == other.name
      && value == other.value
      && editvalue == other.editvalue
      && valuetooltip == other.valuetooltip
      && type == other.type
      && displayedType == other.displayedType
      && variable == other.variable
      && addr == other.addr
      && saddr == other.saddr
      && framekey == other.framekey
      && hasChildren == other.hasChildren
      && valueEnabled == other.valueEnabled
      && valueEditable == other.valueEditable
      && error == other.error;
}

void WatchData::setError(const QString &msg)
{
    setAllUnneeded();
    value = msg;
    setHasChildren(false);
    valueEnabled = false;
    valueEditable = false;
    error = true;
}

void WatchData::setValue(const QString &value0)
{
    value = value0;
    if (value == "{...}") {
        value.clear();
        hasChildren = true; // at least one...
    }

    // avoid duplicated information
    if (value.startsWith(QLatin1Char('(')) && value.contains(") 0x"))
        value = value.mid(value.lastIndexOf(") 0x") + 2);

    // doubles are sometimes displayed as "@0x6141378: 1.2".
    // I don't want that.
    if (/*isIntOrFloatType(type) && */ value.startsWith("@0x")
         && value.contains(':')) {
        value = value.mid(value.indexOf(':') + 2);
        setHasChildren(false);
    }

    // "numchild" is sometimes lying
    //MODEL_DEBUG("\n\n\nPOINTER: " << type << value);
    if (isPointerType(type))
        setHasChildren(value != "0x0" && value != "<null>"
            && !isCharPointerType(type));

    // pointer type information is available in the 'type'
    // column. No need to duplicate it here.
    if (value.startsWith(QLatin1Char('(') + type + ") 0x"))
        value = value.section(QLatin1Char(' '), -1, -1);

    setValueUnneeded();
}

void WatchData::setValueToolTip(const QString &tooltip)
{
    valuetooltip = tooltip;
}

void WatchData::setType(const QString &str, bool guessChildrenFromType)
{
    type = str.trimmed();
    bool changed = true;
    while (changed) {
        if (type.endsWith(QLatin1String("const")))
            type.chop(5);
        else if (type.endsWith(QLatin1Char(' ')))
            type.chop(1);
        else if (type.endsWith(QLatin1Char('&')))
            type.chop(1);
        else if (type.startsWith(QLatin1String("const ")))
            type = type.mid(6);
        else if (type.startsWith(QLatin1String("volatile ")))
            type = type.mid(9);
        else if (type.startsWith(QLatin1String("class ")))
            type = type.mid(6);
        else if (type.startsWith(QLatin1String("struct ")))
            type = type.mid(6);
        else if (type.startsWith(QLatin1Char(' ')))
            type = type.mid(1);
        else
            changed = false;
    }
    setTypeUnneeded();
    if (guessChildrenFromType) {
        switch (guessChildren(type)) {
        case HasChildren:
            setHasChildren(true);
            break;
        case HasNoChildren:
            setHasChildren(false);
            break;
        case HasPossiblyChildren:
            setHasChildren(true); // FIXME: bold assumption
            break;
        }
    }
}

void WatchData::setAddress(const QByteArray &a)
{
    addr = a;
}

QString WatchData::toString() const
{
    const char *doubleQuoteComma = "\",";
    QString res;
    QTextStream str(&res);
    str << QLatin1Char('{');
    if (!iname.isEmpty())
        str << "iname=\"" << iname << doubleQuoteComma;
    if (!name.isEmpty() && name != iname)
        str << "name=\"" << name << doubleQuoteComma;
    if (error)
        str << "error,";
    if (!addr.isEmpty())
        str << "addr=\"" << addr << doubleQuoteComma;
    if (!exp.isEmpty())
        str << "exp=\"" << exp << doubleQuoteComma;

    if (!variable.isEmpty())
        str << "variable=\"" << variable << doubleQuoteComma;

    if (isValueNeeded())
        str << "value=<needed>,";
    if (isValueKnown() && !value.isEmpty())
        str << "value=\"" << value << doubleQuoteComma;

    if (!editvalue.isEmpty())
        str << "editvalue=\"<...>\",";
    //    str << "editvalue=\"" << editvalue << doubleQuoteComma;

    if (isTypeNeeded())
        str << "type=<needed>,";
    if (isTypeKnown() && !type.isEmpty())
        str << "type=\"" << type << doubleQuoteComma;

    if (isHasChildrenNeeded())
        str << "hasChildren=<needed>,";
    if (isHasChildrenKnown())
        str << "hasChildren=\"" << (hasChildren ? "true" : "false") << doubleQuoteComma;

    if (isChildrenNeeded())
        str << "children=<needed>,";
    if (source)
        str << "source=" << source;
    str.flush();
    if (res.endsWith(QLatin1Char(',')))
        res.truncate(res.size() - 1);
    return res + QLatin1Char('}');
}

// Format a tooltip fow with aligned colon.
static void formatToolTipRow(QTextStream &str,
    const QString &category, const QString &value)
{
    str << "<tr><td>" << category << "</td><td> : </td><td>"
        << Qt::escape(value) << "</td></tr>";
}

static inline QString typeToolTip(const WatchData &wd)
{
    if (wd.displayedType.isEmpty())
        return wd.type;
    QString rc = wd.displayedType;
    rc += QLatin1String(" (");
    rc += wd.type;
    rc += QLatin1Char(')');
    return rc;
}

QString WatchData::toToolTip() const
{
    if (!valuetooltip.isEmpty())
        return QString::number(valuetooltip.size());
    QString res;
    QTextStream str(&res);
    str << "<html><body><table>";
    formatToolTipRow(str, WatchHandler::tr("Name"), name);
    formatToolTipRow(str, WatchHandler::tr("Expression"), exp);
    formatToolTipRow(str, WatchHandler::tr("Type"), typeToolTip(*this));
    QString val = value;
    if (value.size() > 1000) {
        val.truncate(1000);
        val +=  WatchHandler::tr(" ... <cut off>");
    }
    formatToolTipRow(str, WatchHandler::tr("Value"), val);
    formatToolTipRow(str, WatchHandler::tr("Object Address"), addr);
    formatToolTipRow(str, WatchHandler::tr("Stored Address"), saddr);
    formatToolTipRow(str, WatchHandler::tr("Internal ID"), iname);
    formatToolTipRow(str, WatchHandler::tr("Generation"),
        QString::number(generation));
    str << "</table></body></html>";
    return res;
}

QString WatchData::msgNotInScope()
{
    static const QString rc = QCoreApplication::translate("Debugger::Internal::WatchData", "<not in scope>");
    return rc;
}

const QString &WatchData::shadowedNameFormat()
{
    static const QString format = QCoreApplication::translate("Debugger::Internal::WatchData", "%1 <shadowed %2>");
    return format;
}

QString WatchData::shadowedName(const QString &name, int seen)
{
    if (seen <= 0)
        return name;
    return shadowedNameFormat().arg(name, seen);
}

///////////////////////////////////////////////////////////////////////
//
// WatchModel
//
///////////////////////////////////////////////////////////////////////

WatchModel::WatchModel(WatchHandler *handler, WatchType type)
    : QAbstractItemModel(handler), m_handler(handler), m_type(type)
{
    m_root = new WatchItem;
    m_root->hasChildren = 1;
    m_root->state = 0;
    m_root->name = WatchHandler::tr("Root");
    m_root->parent = 0;

    switch (m_type) {
        case LocalsWatch:
            m_root->iname = "local";
            m_root->name = WatchHandler::tr("Locals");
            break;
        case WatchersWatch:
            m_root->iname = "watch";
            m_root->name = WatchHandler::tr("Watchers");
            break;
        case TooltipsWatch:
            m_root->iname = "tooltip";
            m_root->name = WatchHandler::tr("Tooltip");
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

void WatchModel::beginCycle()
{
    emit enableUpdates(false);
}

void WatchModel::endCycle()
{
    removeOutdated();
    m_fetchTriggered.clear();
    emit enableUpdates(true);
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
#if USE_MODEL_TEST
    //(void) new ModelTest(this, this);
#endif
#endif
}

void WatchModel::removeOutdatedHelper(WatchItem *item)
{
    if (item->generation < generationCounter) {
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

static QByteArray parentName(const QByteArray &iname)
{
    int pos = iname.lastIndexOf('.');
    if (pos == -1)
        return QByteArray();
    return iname.left(pos);
}


static QString chopConst(QString type)
{
   while (1) {
        if (type.startsWith("const"))
            type = type.mid(5);
        else if (type.startsWith(' '))
            type = type.mid(1);
        else if (type.endsWith("const"))
            type.chop(5);
        else if (type.endsWith(' '))
            type.chop(1);
        else
            break;
    }
    return type;
}

static inline QRegExp stdStringRegExp(const QString &charType)
{
    QString rc = QLatin1String("basic_string<");
    rc += charType;
    rc += QLatin1String(",[ ]?std::char_traits<");
    rc += charType;
    rc += QLatin1String(">,[ ]?std::allocator<");
    rc += charType;
    rc += QLatin1String("> >");
    const QRegExp re(rc);
    Q_ASSERT(re.isValid());
    return re;
}

static QString niceTypeHelper(const QString typeIn)
{
    static QMap<QString, QString> cache;
    const QMap<QString, QString>::const_iterator it = cache.constFind(typeIn);
    if (it != cache.constEnd())
        return it.value();

    QString type = typeIn;
    type.replace(QLatin1Char('*'), QLatin1Char('@'));

    for (int i = 0; i < 10; ++i) {
        int start = type.indexOf("std::allocator<");
        if (start == -1)
            break;
        // search for matching '>'
        int pos;
        int level = 0;
        for (pos = start + 12; pos < type.size(); ++pos) {
            int c = type.at(pos).unicode();
            if (c == '<') {
                ++level;
            } else if (c == '>') {
                --level;
                if (level == 0)
                    break;
            }
        }
        QString alloc = type.mid(start, pos + 1 - start).trimmed();
        QString inner = alloc.mid(15, alloc.size() - 16).trimmed();

        if (inner == QLatin1String("char")) { // std::string
            const QRegExp stringRegexp = stdStringRegExp(inner);
            type.replace(stringRegexp, QLatin1String("string"));
        } else if (inner == QLatin1String("wchar_t")) { // std::wstring
            const QRegExp wchartStringRegexp = stdStringRegExp(inner);
            type.replace(wchartStringRegexp, QLatin1String("wstring"));
        } else if (inner == QLatin1String("unsigned short")) { // std::wstring/MSVC
            const QRegExp usStringRegexp = stdStringRegExp(inner);
            type.replace(usStringRegexp, QLatin1String("wstring"));
        }
        // std::vector, std::deque, std::list
        const QRegExp re1(QString::fromLatin1("(vector|list|deque)<%1, ?%2\\s*>").arg(inner, alloc));
        Q_ASSERT(re1.isValid());
        if (re1.indexIn(type) != -1)
            type.replace(re1.cap(0), QString::fromLatin1("%1<%2>").arg(re1.cap(1), inner));

        // std::stack
        QRegExp re6(QString::fromLatin1("stack<%1, ?std::deque<%2> >").arg(inner, inner));
        if (!re6.isMinimal())
            re6.setMinimal(true);
        Q_ASSERT(re6.isValid());
        if (re6.indexIn(type) != -1)
            type.replace(re6.cap(0), QString::fromLatin1("stack<%1>").arg(inner));

        // std::set
        QRegExp re4(QString::fromLatin1("set<%1, ?std::less<%2>, ?%3\\s*>").arg(inner, inner, alloc));
        if (!re4.isMinimal())
            re4.setMinimal(true);
        Q_ASSERT(re4.isValid());
        if (re4.indexIn(type) != -1)
            type.replace(re4.cap(0), QString::fromLatin1("set<%1>").arg(inner));

        // std::map
        if (inner.startsWith("std::pair<")) {
            // search for outermost ','
            int pos;
            int level = 0;
            for (pos = 10; pos < inner.size(); ++pos) {
                int c = inner.at(pos).unicode();
                if (c == '<')
                    ++level;
                else if (c == '>')
                    --level;
                else if (c == ',' && level == 0)
                    break;
            }
            QString ckey = inner.mid(10, pos - 10);
            QString key = chopConst(ckey);
            QString value = inner.mid(pos + 2, inner.size() - 3 - pos).trimmed();
            QRegExp re5(QString("map<%1, ?%2, ?std::less<%3 ?>, ?%4\\s*>")
                .arg(key, value, key, alloc));
            if (!re5.isMinimal())
                re5.setMinimal(true);
            Q_ASSERT(re5.isValid());
            if (re5.indexIn(type) != -1) {
                type.replace(re5.cap(0), QString("map<%1, %2>").arg(key, value));
            } else {
                QRegExp re7(QString("map<const %1, ?%2, ?std::less<const %3>, ?%4\\s*>")
                    .arg(key, value, key, alloc));
                if (!re7.isMinimal())
                    re7.setMinimal(true);
                if (re7.indexIn(type) != -1)
                    type.replace(re7.cap(0), QString("map<const %1, %2>").arg(key, value));
            }
        }
    }
    type.replace(QLatin1Char('@'), QLatin1Char('*'));
    type.replace(QLatin1String(" >"), QString(QLatin1Char('>')));
    cache.insert(typeIn, type); // For simplicity, also cache unmodified types
    return type;
}

QString WatchModel::niceType(const QString &typeIn) const
{
    QString type = niceTypeHelper(typeIn);
    if (!theDebuggerBoolSetting(ShowStdNamespace))
        type = type.remove("std::");
    IDebuggerEngine *engine = m_handler->m_manager->currentEngine();
    if (engine && !theDebuggerBoolSetting(ShowQtNamespace))
        type = type.remove(engine->qtNamespace());
    return type;
}

template <class IntType> QString reformatInteger(IntType value, int format)
{
    switch (format) {
    case HexadecimalFormat:
        return ("(hex) ") + QString::number(value, 16);
    case BinaryFormat:
        return ("(bin) ") + QString::number(value, 2);
    case OctalFormat:
        return ("(oct) ") + QString::number(value, 8);
    }
    return QString::number(value); // not reached
}

static QString formattedValue(const WatchData &data, int format)
{
    if (isIntType(data.type)) {
        if (format <= 0)
            return data.value;
        // Evil hack, covers 'unsigned' as well as quint64.
        if (data.type.contains(QLatin1Char('u')))
            return reformatInteger(data.value.toULongLong(), format);
        return reformatInteger(data.value.toLongLong(), format);
    }
    return data.value;
}

bool WatchModel::canFetchMore(const QModelIndex &index) const
{
    WatchItem *item = watchItem(index);
    QTC_ASSERT(item, return false);
    return index.isValid() && !m_fetchTriggered.contains(item->iname);
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
        m_handler->m_manager->updateWatchData(data);
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

// Truncate value for item view, maintaining quotes
static inline QString truncateValue(QString v)
{
    enum { maxLength = 512 };
    if (v.size() < maxLength)
        return v;
    const bool isQuoted = v.endsWith(QLatin1Char('"')); // check for 'char* "Hallo"'
    v.truncate(maxLength);
    v += isQuoted ? QLatin1String("...\"") : QLatin1String("...");
    return v;
}

QVariant WatchModel::data(const QModelIndex &idx, int role) const
{
    const WatchItem *item = watchItem(idx);
    const WatchItem &data = *item;

    switch (role) {
        case Qt::EditRole:
        case Qt::DisplayRole: {
            switch (idx.column()) {
                case 0:
                    if (data.name == QLatin1String("*") && item->parent)
                        return QVariant(QLatin1Char('*') + item->parent->name);
                    return data.name;
                case 1: {
                    int format = m_handler->m_individualFormats.value(data.addr, -1);
                    if (format == -1)
                        format = m_handler->m_typeFormats.value(data.type, -1);
                    return truncateValue(formattedValue(data, format));
                }
                case 2: {
                    if (!data.displayedType.isEmpty())
                        return data.displayedType;
                    return niceType(data.type);
                }
                default: break;
            }
            break;
        }

        case Qt::ToolTipRole:
            return theDebuggerBoolSetting(UseToolTipsInLocalsView)
                ? data.toToolTip() : QVariant();

        case Qt::ForegroundRole: {
            static const QVariant red(QColor(200, 0, 0));
            static const QVariant gray(QColor(140, 140, 140));
            switch (idx.column()) {
                case 1: return !data.valueEnabled ? gray : data.changed ? red : QVariant();
            }
            break;
        }

        case ExpressionRole:
            return data.exp;

        case INameRole:
            return data.iname;

        case ExpandedRole:
            return m_handler->m_expandedINames.contains(data.iname);

        case TypeFormatListRole:
            if (!data.typeFormats.isEmpty())
                return data.typeFormats.split(',');
            if (isIntType(data.type))
                return QStringList() << tr("decimal") << tr("hexadecimal")
                    << tr("binary") << tr("octal");
            if (data.type.endsWith(QLatin1Char('*')))
                return QStringList()
                    << tr("Bald pointer")
                    << tr("Latin1 string")
                    << tr("UTF8 string")
                    << tr("UTF16 string")
                    << tr("UCS4 string");
            break;

        case TypeFormatRole:
            return m_handler->m_typeFormats.value(data.type, -1);

        case IndividualFormatRole:
            return m_handler->m_individualFormats.value(data.addr, -1);

        case AddressRole: {
            if (!data.addr.isEmpty())
                return data.addr;
            bool ok;
            (void) data.value.toULongLong(&ok, 0);
            if (ok)
                return data.value;
            return QVariant();
        }

        default:
            break;
    }
    return QVariant();
}

bool WatchModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    WatchItem &data = *watchItem(index);
    if (role == ExpandedRole) {
        if (value.toBool()) {
            // Should already have been triggered by fetchMore()
            //QTC_ASSERT(m_handler->m_expandedINames.contains(data.iname), /**/);
            m_handler->m_expandedINames.insert(data.iname);
        } else {
            m_handler->m_expandedINames.remove(data.iname);
        }
    } else if (role == TypeFormatRole) {
        m_handler->setFormat(data.type, value.toInt());
        m_handler->m_manager->updateWatchData(data);
    } else if (role == IndividualFormatRole) {
        const int format = value.toInt();
        if (format == -1) {
            m_handler->m_individualFormats.remove(data.addr);
        } else {
            m_handler->m_individualFormats[data.addr] = format;
        }
        m_handler->m_manager->updateWatchData(data);
    }
    emit dataChanged(index, index);
    return true;
}

Qt::ItemFlags WatchModel::flags(const QModelIndex &idx) const
{
    using namespace Qt;

    if (!idx.isValid())
        return ItemFlags();

    // enabled, editable, selectable, checkable, and can be used both as the
    // source of a drag and drop operation and as a drop target.

    static const ItemFlags notEditable =
          ItemIsSelectable
        | ItemIsDragEnabled
        | ItemIsDropEnabled
        // | ItemIsUserCheckable
        // | ItemIsTristate
        | ItemIsEnabled;

    static const ItemFlags editable = notEditable | ItemIsEditable;

    const WatchData &data = *watchItem(idx);

    if (data.isWatcher() && idx.column() == 0)
        return editable; // watcher names are editable
    if (data.isWatcher() && idx.column() == 2)
        return editable; // watcher types are
    if (idx.column() == 1 && data.valueEditable)
        return editable; // locals and watcher values are sometimes editable
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

struct IName : public QByteArray
{
    IName(const QByteArray &iname) : QByteArray(iname) {}
};

bool iNameLess(const QString &iname1, const QString &iname2)
{
    QString name1 = iname1.section('.', -1);
    QString name2 = iname2.section('.', -1);
    if (!name1.isEmpty() && !name2.isEmpty()) {
        if (name1.at(0).isDigit() && name2.at(0).isDigit()) {
            bool ok1 = false, ok2 = false;
            int i1 = name1.toInt(&ok1), i2 = name2.toInt(&ok2);
            if (ok1 && ok2)
                return i1 < i2;
        }
    }
    return name1 < name2;
}

bool operator<(const IName &iname1, const IName &iname2)
{
    return iNameLess(iname1, iname2);
}

static bool iNameSorter(const WatchItem *item1, const WatchItem *item2)
{
    return iNameLess(item1->iname, item2->iname);
}

static int findInsertPosition(const QList<WatchItem *> &list, const WatchItem *item)
{
    QList<WatchItem *>::const_iterator it =
        qLowerBound(list.begin(), list.end(), item, iNameSorter);
    return it - list.begin();
}

void WatchModel::insertData(const WatchData &data)
{
    //qDebug() << "WMI:" << data.toString();
    //static int bulk = 0;
    //qDebug() << "SINGLE: " << ++bulk << data.toString();
    if (data.iname.isEmpty()) {
        int x;
        x = 1;
    }
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
        bool changed = !data.value.isEmpty()
            && data.value != oldItem->value
            && data.value != strNotInScope;
        oldItem->setData(data);
        oldItem->changed = changed;
        oldItem->generation = generationCounter;
        QModelIndex idx = watchIndex(oldItem);
        emit dataChanged(idx, idx.sibling(idx.row(), 2));

        // This works around http://bugreports.qt.nokia.com/browse/QTBUG-7115
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
        item->generation = generationCounter;
        item->changed = true;
        int n = findInsertPosition(parent->children, item);
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
    // order", leading to random removal of items in removeOutDated();

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

    QMap<IName, WatchData> newList;
    typedef QMap<IName, WatchData>::iterator Iterator;
    foreach (const WatchItem &data, list)
        newList[data.iname] = data;
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
        Iterator it = newList.find(oldItem->iname);
        if (it == newList.end()) {
            WatchData data = *oldItem;
            data.generation = generationCounter;
            newList[oldItem->iname] = data;
        } else {
            bool changed = !it->value.isEmpty()
                && it->value != oldItem->value
                && it->value != strNotInScope;
            it->changed = changed;
            if (it->generation == -1)
                it->generation = generationCounter;
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
                parent->children[i]->generation = generationCounter;
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
                item->generation = generationCounter;
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
    int format = m_handler->m_individualFormats.value(item->iname, -1);
    if (format == -1)
        format = m_handler->m_typeFormats.value(item->type, -1);
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

WatchHandler::WatchHandler(DebuggerManager *manager)
{
    m_manager = manager;
    m_expandPointers = true;
    m_inChange = false;

    m_locals = new WatchModel(this, LocalsWatch);
    m_watchers = new WatchModel(this, WatchersWatch);
    m_tooltips = new WatchModel(this, TooltipsWatch);

    connect(theDebuggerAction(WatchExpression),
        SIGNAL(triggered()), this, SLOT(watchExpression()));
    connect(theDebuggerAction(RemoveWatchExpression),
        SIGNAL(triggered()), this, SLOT(removeWatchExpression()));
    connect(theDebuggerAction(ShowStdNamespace),
        SIGNAL(triggered()), this, SLOT(emitAllChanged()));
    connect(theDebuggerAction(ShowQtNamespace),
        SIGNAL(triggered()), this, SLOT(emitAllChanged()));
}

void WatchHandler::beginCycle()
{
    ++generationCounter;
    m_locals->beginCycle();
    m_watchers->beginCycle();
    m_tooltips->beginCycle();
}

void WatchHandler::endCycle()
{
    m_locals->endCycle();
    m_watchers->endCycle();
    m_tooltips->endCycle();
}

void WatchHandler::cleanup()
{
    m_expandedINames.clear();
    m_locals->reinitialize();
    m_tooltips->reinitialize();
    m_locals->m_fetchTriggered.clear();
    m_watchers->m_fetchTriggered.clear();
    m_tooltips->m_fetchTriggered.clear();
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
    m_locals->emitAllChanged();
    m_watchers->emitAllChanged();
    m_tooltips->emitAllChanged();
}

void WatchHandler::insertData(const WatchData &data)
{
    MODEL_DEBUG("INSERTDATA: " << data.toString());
    if (!data.isValid()) {
        qWarning("%s:%d: Attempt to insert invalid watch item: %s",
            __FILE__, __LINE__, qPrintable(data.toString()));
        return;
    }

    if (data.isSomethingNeeded() && data.iname.contains('.')) {
        MODEL_DEBUG("SOMETHING NEEDED: " << data.toString());
        IDebuggerEngine *engine = m_manager->currentEngine();
        if (engine && !engine->isSynchroneous()) {
            m_manager->updateWatchData(data);
        } else {
            qDebug() << "ENDLESS LOOP: SOMETHING NEEDED: " << data.toString();
            WatchData data1 = data;
            data1.setAllUnneeded();
            data1.setValue(QLatin1String("<unavailable synchroneous data>"));
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
            m_manager->updateWatchData(data);
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

void WatchHandler::watchExpression()
{
    if (QAction *action = qobject_cast<QAction *>(sender()))
        watchExpression(action->data().toString());
}

QByteArray WatchHandler::watcherName(const QByteArray &exp)
{
    return "watch." + QByteArray::number(m_watcherNames[exp]);
}

void WatchHandler::watchExpression(const QString &exp)
{
    // FIXME: 'exp' can contain illegal characters
    WatchData data;
    data.exp = exp.toLatin1();
    data.name = exp;
    m_watcherNames[data.exp] = watcherCounter++;
    if (exp.isEmpty() || exp == watcherEditPlaceHolder())
        data.setAllUnneeded();
    data.iname = watcherName(data.exp);
    IDebuggerEngine *engine = m_manager->currentEngine();
    if (engine && engine->isSynchroneous())
        m_manager->updateWatchData(data);
    else
        insertData(data);
    m_manager->updateWatchData(data);
    saveWatchers();
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
    QObject *w = m_editHandlers.value(data.iname);
    if (data.editformat == 0x0) {
        m_editHandlers.remove(data.iname);
        delete w;
    } else if (data.editformat == 1 || data.editformat == 3) {
        // QImage
        QLabel *l = qobject_cast<QLabel *>(w);
        if (!l) {
            delete w;
            l = new QLabel;
            QString addr = tr("unknown address");
            if (!data.addr.isEmpty())
                addr =  QString::fromLatin1(data.addr);
            l->setWindowTitle(tr("%1 object at %2").arg(data.type, addr));
            m_editHandlers[data.iname] = l;
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
        l->setPixmap(QPixmap::fromImage(im));
        l->resize(width, height);
        l->show();
    } else if (data.editformat == 2) {
        // QString
        QTextEdit *t = qobject_cast<QTextEdit *>(w);
        if (!t) {
            delete w;
            t = new QTextEdit;
            m_editHandlers[data.iname] = t;
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
            p->start(cmd);
            p->waitForStarted();
            m_editHandlers[data.iname] = p;
        }
        p->write(input + "\n");
    } else {
        QTC_ASSERT(false, qDebug() << "Display format: " << data.editformat);
    }
}

void WatchHandler::removeWatchExpression()
{
    if (QAction *action = qobject_cast<QAction *>(sender()))
        removeWatchExpression(action->data().toString());
}

void WatchHandler::removeWatchExpression(const QString &exp0)
{
    QByteArray exp = exp0.toLatin1();
    MODEL_DEBUG("REMOVE WATCH: " << exp);
    m_watcherNames.remove(exp);
    foreach (WatchItem *item, m_watchers->rootItem()->children) {
        if (item->exp == exp) {
            m_watchers->destroyItem(item);
            saveWatchers();
            break;
        }
    }
}

void WatchHandler::updateWatchers()
{
    //qDebug() << "UPDATE WATCHERS";
    // copy over all watchers and mark all watchers as incomplete
    foreach (const QByteArray &exp, m_watcherNames.keys()) {
        WatchData data;
        data.iname = watcherName(exp);
        data.setAllNeeded();
        data.name = exp;
        data.exp = exp;
        insertData(data);
    }
}

void WatchHandler::loadWatchers()
{
    QVariant value = m_manager->sessionValue("Watchers");
    foreach (const QString &exp, value.toStringList())
        m_watcherNames[exp.toLatin1()] = watcherCounter++;

    //qDebug() << "LOAD WATCHERS: " << m_watchers;
    //reinitializeWatchersHelper();
}

QStringList WatchHandler::watchedExpressions() const
{
    // Filter out invalid watchers.
    QStringList watcherNames;
    QHashIterator<QByteArray, int> it(m_watcherNames);
    while (it.hasNext()) {
        it.next();
        const QString &watcherName = it.key();
        if (!watcherName.isEmpty() && watcherName != watcherEditPlaceHolder())
            watcherNames.push_back(watcherName);
    }
    return watcherNames;
}

void WatchHandler::saveWatchers()
{
    //qDebug() << "SAVE WATCHERS: " << m_watchers;
    m_manager->setSessionValue("Watchers", QVariant(watchedExpressions()));
}

void WatchHandler::loadTypeFormats()
{
    QVariant value = m_manager->sessionValue("DefaultFormats");
    QMap<QString, QVariant> typeFormats = value.toMap();
    QMapIterator<QString, QVariant> it(typeFormats);
    while (it.hasNext()) {
        it.next();
        if (!it.key().isEmpty())
            m_typeFormats.insert(it.key(), it.value().toInt());
    }
}

void WatchHandler::saveTypeFormats()
{
    QMap<QString, QVariant> typeFormats;
    QHashIterator<QString, int> it(m_typeFormats);
    while (it.hasNext()) {
        it.next();
        const int format = it.value();
        if (format != DecimalFormat) {
            const QString key = it.key().trimmed();
            if (!key.isEmpty())
                typeFormats.insert(key, format);
        }
    }
    m_manager->setSessionValue("DefaultFormats", QVariant(typeFormats));
}

void WatchHandler::saveSessionData()
{
    saveWatchers();
    saveTypeFormats();
}

void WatchHandler::loadSessionData()
{
    loadWatchers();
    loadTypeFormats();
    foreach (const QByteArray &exp, m_watcherNames.keys()) {
        WatchData data;
        data.iname = watcherName(exp);
        data.setAllUnneeded();
        data.name = exp;
        data.exp = exp;
        insertData(data);
    }
}

WatchModel *WatchHandler::model(WatchType type) const
{
    switch (type) {
        case LocalsWatch: return m_locals;
        case WatchersWatch: return m_watchers;
        case TooltipsWatch: return m_tooltips;
    }
    QTC_ASSERT(false, /**/);
    return 0;
}

WatchModel *WatchHandler::modelForIName(const QByteArray &iname) const
{
    if (iname.startsWith("local"))
        return m_locals;
    if (iname.startsWith("tooltip"))
        return m_tooltips;
    if (iname.startsWith("watch"))
        return m_watchers;
    QTC_ASSERT(false, qDebug() << "INAME: " << iname);
    return 0;
}

WatchData *WatchHandler::findItem(const QByteArray &iname) const
{
    const WatchModel *model = modelForIName(iname);
    QTC_ASSERT(model, return 0);
    return model->findItem(iname, model->m_root);
}

QString WatchHandler::watcherEditPlaceHolder()
{
    static const QString rc = tr("<Edit>");
    return rc;
}

void WatchHandler::setFormat(const QString &type, int format)
{
    m_typeFormats[type] = format;
    saveTypeFormats();
    m_locals->emitDataChanged(1);
    m_watchers->emitDataChanged(1);
    m_tooltips->emitDataChanged(1);
}

int WatchHandler::format(const QByteArray &iname) const
{
    int result = -1;
    if (const WatchData *item = findItem(iname)) {
        int result = m_individualFormats.value(iname, -1);
        if (result == -1)
            result = m_typeFormats.value(item->type, -1);
    }
    return result;
}

QByteArray WatchHandler::formatRequests() const
{
    QByteArray ba;
    //m_locals->formatRequests(&ba, m_locals->m_root);
    //m_watchers->formatRequests(&ba, m_watchers->m_root);

    ba.append("expanded:");
    if (!m_expandedINames.isEmpty()) {
        QSetIterator<QByteArray> jt(m_expandedINames);
        while (jt.hasNext()) {
            QByteArray iname = jt.next();
            ba.append(iname);
            ba.append(',');
        }
        ba.chop(1);
    }
    ba.append(' ');

    ba.append("typeformats:");
    if (!m_typeFormats.isEmpty()) {
        QHashIterator<QString, int> it(m_typeFormats);
        while (it.hasNext()) {
            it.next();
            ba.append(it.key().toLatin1().toHex());
            ba.append('=');
            ba.append(QByteArray::number(it.value()));
            ba.append(',');
        }
        ba.chop(1);
    }
    ba.append(' ');

    ba.append("formats:");
    if (!m_individualFormats.isEmpty()) {
        QHashIterator<QByteArray, int> it(m_individualFormats);
        while (it.hasNext()) {
            it.next();
            ba.append(it.key());
            ba.append('=');
            ba.append(QByteArray::number(it.value()));
            ba.append(',');
        }
        ba.chop(1);
    }
    ba.append(' ');

    return ba;
}

} // namespace Internal
} // namespace Debugger
