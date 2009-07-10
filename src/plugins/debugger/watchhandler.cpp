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
** contact the sales department at http://www.qtsoftware.com/contact.
**
**************************************************************************/

#include "watchhandler.h"
#include "watchutils.h"
#include "debuggeractions.h"

#if USE_MODEL_TEST
#include "modeltest.h"
#endif

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QEvent>
#include <QtCore/QtAlgorithms>
#include <QtCore/QTextStream>
#include <QtCore/QTimer>

#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QLabel>
#include <QtGui/QToolTip>
#include <QtGui/QTextEdit>
#include <QtGui/QTreeView>

#include <ctype.h>


// creates debug output regarding pending watch data results
//#define DEBUG_PENDING 1
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

static int watcherCounter = 0;
////////////////////////////////////////////////////////////////////
//
// WatchData
//
////////////////////////////////////////////////////////////////////

WatchData::WatchData() :
    hasChildren(false),
    generation(-1),
    valuedisabled(false),
    source(0),
    state(InitialState),
    changed(false)
{
}

void WatchData::setError(const QString &msg)
{
    setAllUnneeded();
    value = msg;
    setHasChildren(false);
    valuedisabled = true;
}

void WatchData::setValue(const QString &value0)
{
    value = value0;
    if (value == "{...}") {
        value.clear();
        hasChildren = true; // at least one...
    }

    // avoid duplicated information
    if (value.startsWith("(") && value.contains(") 0x"))
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
        setHasChildren(value != "0x0" && value != "<null>");

    // pointer type information is available in the 'type'
    // column. No need to duplicate it here.
    if (value.startsWith("(" + type + ") 0x"))
        value = value.section(" ", -1, -1);

    setValueUnneeded();
}

void WatchData::setValueToolTip(const QString &tooltip)
{
    valuetooltip = tooltip;
}

void WatchData::setType(const QString &str)
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
    if (isIntOrFloatType(type))
        setHasChildren(false);
}

void WatchData::setAddress(const QString &str)
{
    addr = str;
}

WatchData WatchData::pointerChildPlaceHolder() const
{
    WatchData data1;
    data1.iname = iname + QLatin1String(".*");
    data1.name = QLatin1Char('*') + name;
    data1.exp = QLatin1String("(*(") + exp + QLatin1String("))");
    data1.type = stripPointerType(type);
    data1.setValueNeeded();
    return data1;
}

QString WatchData::toString() const
{
    const char *doubleQuoteComma = "\",";
    QString res;
    QTextStream str(&res);
    if (!iname.isEmpty())
        str << "iname=\"" << iname << doubleQuoteComma;
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
        str << "editvalue=\"" << editvalue << doubleQuoteComma;

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
    str.flush();
    if (res.endsWith(QLatin1Char(',')))
        res.truncate(res.size() - 1);
    return res + QLatin1Char('}');
}

// Format a tooltip fow with aligned colon
static void formatToolTipRow(QTextStream &str, const QString &category, const QString &value)
{
    str << "<tr><td>" << category << "</td><td> : </td><td>"
        << Qt::escape(value) << "</td></tr>";
}

QString WatchData::toToolTip() const
{
    if (!valuetooltip.isEmpty())
        return QString::number(valuetooltip.size());
    QString res;
    QTextStream str(&res);
    str << "<html><body><table>";
    formatToolTipRow(str, WatchHandler::tr("Expression"), exp);
    formatToolTipRow(str, WatchHandler::tr("Type"), type);
    QString val = value;
    if (value.size() > 1000) {
        val.truncate(1000);
        val +=  WatchHandler::tr(" ... <cut off>");
    }
    formatToolTipRow(str, WatchHandler::tr("Value"), val);
    formatToolTipRow(str, WatchHandler::tr("Object Address"), addr);
    formatToolTipRow(str, WatchHandler::tr("Stored Address"), saddr);
    formatToolTipRow(str, WatchHandler::tr("Internal ID"), iname);
    str << "</table></body></html>";
    return res;
}

////////////////////////////////////////////////////////////////////
//
// WatchItem
//
////////////////////////////////////////////////////////////////////

WatchItem::WatchItem() :
    parent(0),
    fetchTriggered(false)
{
}

WatchItem::WatchItem(const WatchData &data) :
    WatchData(data),
    parent(0),
    fetchTriggered(false)
{
}

void WatchItem::setData(const WatchData &data)
{
    static_cast<WatchData &>(*this) = data;
}

WatchItem::~WatchItem()
{
    qDeleteAll(children);
}

void WatchItem::removeChildren()
{
    if (!children.empty()) {
        qDeleteAll(children);
        children.clear();
    }
}

void WatchItem::addChild(WatchItem *child)
{
    children.push_back(child);
    child->parent = this;
}

///////////////////////////////////////////////////////////////////////
//
// WatchPredicate
//
///////////////////////////////////////////////////////////////////////

WatchPredicate::WatchPredicate(Mode mode, const QString &pattern) :
    m_pattern(pattern),
    m_mode(mode)
{
}

bool WatchPredicate::operator()(const WatchData &w) const
{
    switch (m_mode) {
        case INameMatch:
        return m_pattern == w.iname;
        case ExpressionMatch:
        return m_pattern == w.exp;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////
//
// WatchModel
//
///////////////////////////////////////////////////////////////////////

WatchModel::WatchModel(WatchHandler *handler, WatchType type, QObject *parent) :
    QAbstractItemModel(parent),
    m_root(new WatchItem),
    m_handler(handler),
    m_type(type)
{
    WatchItem *dummyRoot = new WatchItem;

    switch (type) {
        case LocalsWatch:
            dummyRoot->iname = QLatin1String("local");
            dummyRoot->name = WatchHandler::tr("Locals");
            break;
        case WatchersWatch:
            dummyRoot->iname = QLatin1String("watch");
            dummyRoot->name = WatchHandler::tr("Watchers");
            break;
        case TooltipsWatch:
            dummyRoot->iname = QLatin1String("tooltip");
            dummyRoot->name = WatchHandler::tr("Tooltip");
            break;
        case WatchModelCount:
            break;
        }
    dummyRoot->hasChildren = true;
    dummyRoot->state = 0;

    m_root->addChild(dummyRoot);

    m_root->hasChildren = 1;
    m_root->state = 0;
    m_root->name = WatchHandler::tr("Root");
}

WatchModel::~WatchModel()
{
    delete m_root;
}

QString WatchModel::parentName(const QString &iname)
{
    const int pos = iname.lastIndexOf(QLatin1Char('.'));
    if (pos == -1)
        return QString();
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

QString niceType(const QString typeIn)
{
    static QMap<QString, QString> cache;
    const QMap<QString, QString>::const_iterator it = cache.constFind(typeIn);
    if (it != cache.constEnd()) {
        return it.value();
    }
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
            static const QRegExp stringRegexp = stdStringRegExp(inner);
            type.replace(stringRegexp, QLatin1String("string"));
        } else if (inner == QLatin1String("wchar_t")) { // std::wstring
            static const QRegExp wchartStringRegexp = stdStringRegExp(inner);
            type.replace(wchartStringRegexp, QLatin1String("wstring"));
        } else if (inner == QLatin1String("unsigned short")) { // std::wstring/MSVC
            static const QRegExp usStringRegexp = stdStringRegExp(inner);
            type.replace(usStringRegexp, QLatin1String("wstring"));
        }
        // std::vector, std::deque, std::list
        static const QRegExp re1(QString::fromLatin1("(vector|list|deque)<%1,[ ]?%2\\s*>").arg(inner, alloc));
        Q_ASSERT(re1.isValid());
        if (re1.indexIn(type) != -1)
            type.replace(re1.cap(0), QString::fromLatin1("%1<%2>").arg(re1.cap(1), inner));

        // std::stack
        static QRegExp re6(QString::fromLatin1("stack<%1,[ ]?std::deque<%2> >").arg(inner, inner));
        if (!re6.isMinimal())
            re6.setMinimal(true);
        Q_ASSERT(re6.isValid());
        if (re6.indexIn(type) != -1)
            type.replace(re6.cap(0), QString::fromLatin1("stack<%1>").arg(inner));

        // std::set
        static QRegExp re4(QString::fromLatin1("set<%1,[ ]?std::less<%2>,[ ]?%3\\s*>").arg(inner, inner, alloc));
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
            QString value = inner.mid(pos + 2, inner.size() - 3 - pos);

            static QRegExp re5(QString("map<%1,[ ]?%2,[ ]?std::less<%3>,[ ]?%4\\s*>")
                .arg(key, value, key, alloc));
            if (!re5.isMinimal())
                re5.setMinimal(true);
            Q_ASSERT(re5.isValid());
            if (re5.indexIn(type) != -1)
                type.replace(re5.cap(0), QString("map<%1, %2>").arg(key, value));
            else {
                static QRegExp re7(QString("map<const %1,[ ]?%2,[ ]?std::less<const %3>,[ ]?%4\\s*>")
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

static QString formattedValue(const WatchData &data,
    int individualFormat, int typeFormat)
{
    if (isIntType(data.type)) {
        int format = individualFormat == -1 ? typeFormat : individualFormat;
        int value = data.value.toInt();
        if (format == HexadecimalFormat)
            return ("(hex) ") + QString::number(value, 16);
        if (format == BinaryFormat)
            return ("(bin) ") + QString::number(value, 2);
        if (format == OctalFormat)
            return ("(oct) ") + QString::number(value, 8);
        return data.value;
    }

    return data.value;
}

int WatchModel::columnCount(const QModelIndex &idx) const
{
    Q_UNUSED(idx);
    return 3;
}

int WatchModel::rowCount(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return 1;
    if (idx.column() > 0)
        return 0;
    return watchItem(idx)->children.size();
}

bool WatchModel::hasChildren(const QModelIndex &idx) const
{
    if (const WatchItem *item = watchItem(idx)) {
        if (!item->children.empty())
            return true;
        QTC_ASSERT(item->isHasChildrenKnown(), return false);
        return item->hasChildren;
    }
    return false;
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

QVariant WatchModel::data(const QModelIndex &idx, int role) const
{
    if (const WatchData *wdata = watchItem(idx))
        return data(*wdata, idx.column(), role);
    return QVariant();
}

QVariant WatchModel::data(const WatchData &data, int column, int role) const
{
    switch (role) {
        case Qt::DisplayRole: {
            switch (column) {
                case 0: return data.name;
                case 1: return formattedValue(data,
                    m_handler->m_individualFormats[data.iname],
                    m_handler->m_typeFormats[data.type]);
                case 2: return niceType(data.type);
                default: break;
            }
            break;
        }

        case Qt::ToolTipRole:
            return data.toToolTip();

        case Qt::ForegroundRole: {
            static const QVariant red(QColor(200, 0, 0));
            static const QVariant black(QColor(0, 0, 0));
            static const QVariant gray(QColor(140, 140, 140));
            switch (column) {
                case 0: return black;
                case 1: return data.valuedisabled ? gray : data.changed ? red : black;
                case 2: return black;
            }
            break;
        }

        case ExpressionRole:
            return data.exp;

        case INameRole:
            return data.iname;

        case ExpandedRole:
            return m_handler->m_expandedINames.contains(data.iname);
            //FIXME return node < 4 || m_expandedINames.contains(data.iname);

        case ActiveDataRole:
            qDebug() << "ASK FOR" << data.iname;
            return true;

        case TypeFormatListRole:
            if (isIntType(data.type))
                return QStringList() << tr("decimal") << tr("hexadecimal")
                    << tr("binary") << tr("octal");
            break;

        case TypeFormatRole:
            return m_handler->m_typeFormats[data.type];

        case IndividualFormatRole: {
            int format = m_handler->m_individualFormats[data.iname];
            if (format == -1)
                return m_handler->m_typeFormats[data.type];
            return format;
        }

        default:
            break;
    }
    return QVariant();
}

bool WatchModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    WatchData &data = *watchItem(index);
    if (role == ExpandedRole) {
        if (value.toBool())
            m_handler->m_expandedINames.insert(data.iname);
        else
            m_handler->m_expandedINames.remove(data.iname);
    } else if (role == TypeFormatRole) {
        m_handler->setFormat(data.type, value.toInt());
    } else if (role == IndividualFormatRole) {
        m_handler->m_individualFormats[data.iname] = value.toInt();
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
    if (idx.column() == 1)
        return editable; // locals and watcher values are editable
    return  notEditable;
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

void WatchModel::setValueByIName(const QString &iname, const QString &value)
{
    if (WatchItem *wd = findItemByIName(iname, dummyRoot())) {
        if (wd->value != value) {
            wd->setValue(value);
            const QModelIndex index = watchIndex(wd);
            emit dataChanged(index.sibling(0, 1), index.sibling(0, 2));
        }
    }
}

void WatchModel::setValueByExpression(const QString &exp, const QString &value)
{
    if (WatchItem *wd = findItemByExpression(exp, dummyRoot())) {
        if (wd->value != value) {
            wd->setValue(value);
            const QModelIndex index = watchIndex(wd);
            emit dataChanged(index.sibling(0, 1), index.sibling(0, 2));
        }
    }
}

WatchItem *WatchModel::root() const
{
    return m_root;
}

WatchItem *WatchModel::dummyRoot() const
{
    if (!m_root->children.isEmpty())
        return m_root->children.front();
    return 0;
}

void WatchModel::reinitialize()
{
    WatchItem *item = dummyRoot();
    QTC_ASSERT(item, return);
    const QModelIndex index = watchIndex(item);
    const int n = item->children.size();
    if (n == 0)
        return;
    //MODEL_DEBUG("REMOVING " << n << " CHILDREN OF " << item->iname);
    beginRemoveRows(index, 0, n - 1);
    item->removeChildren();
    endRemoveRows();
}

void WatchModel::removeItem(WatchItem *itemIn)
{
    WatchItem *item = static_cast<WatchItem *>(itemIn);
    WatchItem *parent = item->parent;
    const QModelIndex index = watchIndex(parent);
    const int n = parent->children.indexOf(item);
    //MODEL_DEBUG("NEED TO REMOVE: " << item->iname << "AT" << n);
    beginRemoveRows(index, n, n);
    parent->children.removeAt(n);
    endRemoveRows();
}

QModelIndex WatchModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    const WatchItem *item = watchItem(parent);
    QTC_ASSERT(item, return QModelIndex());
    return createIndex(row, column, (void*)(item->children.at(row)));
}

QModelIndex WatchModel::parent(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return QModelIndex();

    const WatchItem *item = static_cast<WatchItem *>(watchItem(idx));
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

static WatchItem *findRecursion(WatchItem *root, const WatchPredicate &p)
{
    if (p(*root))
        return root;
    for (int i = root->children.size(); --i >= 0; )
        if (WatchItem *item = findRecursion(root->children.at(i), p))
            return item;
    return 0;
}

WatchItem *WatchModel::findItemByExpression(const QString &exp, WatchItem *root) const
{
    return findRecursion(root, WatchPredicate(WatchPredicate::ExpressionMatch, exp));
}

WatchItem *WatchModel::findItemByIName(const QString &iname, WatchItem *root) const
{
    return findRecursion(root, WatchPredicate(WatchPredicate::INameMatch, iname));
}

static void debugItemRecursion(QDebug d, WatchItem *item, QString indent = QString())
{
    qDebug() << (indent + item->toString());
    if (!item->children.isEmpty()) {
        indent += QLatin1String("   ");
        foreach(WatchItem *c, item->children)
            debugItemRecursion(d, c, indent);
    }
}

QDebug operator<<(QDebug d, const WatchModel &wm)
{
    if (WatchItem *root = wm.dummyRoot())
        debugItemRecursion(d.nospace(), root);
    return d;
}

///////////////////////////////////////////////////////////////////////
//
// WatchHandler
//
///////////////////////////////////////////////////////////////////////

WatchHandler::WatchHandler() :
    m_expandPointers(true),
    m_inChange(false)
{
    connect(theDebuggerAction(WatchExpression),
        SIGNAL(triggered()), this, SLOT(watchExpression()));
    connect(theDebuggerAction(RemoveWatchExpression),
        SIGNAL(triggered()), this, SLOT(removeWatchExpression()));
    qFill(m_models, m_models + WatchModelCount, static_cast<WatchModel*>(0));
}

void WatchHandler::cleanup()
{
    m_expandedINames.clear();
    m_displayedINames.clear();
    if (m_models[LocalsWatch])
        m_models[LocalsWatch]->reinitialize();
    if (m_models[TooltipsWatch])
        m_models[TooltipsWatch]->reinitialize();
#if 0
    for (EditWindows::ConstIterator it = m_editWindows.begin();
            it != m_editWindows.end(); ++it) {
        if (!it.value().isNull())
            delete it.value();
    }
    m_editWindows.clear();
#endif
}

void WatchHandler::removeData(const QString &iname)
{
    WatchModel *model = modelForIName(iname);
    if (!model)
        return;
    if (WatchItem *item = model->findItemByIName(iname, model->root()))
        model->removeItem(item);
}

void WatchHandler::watchExpression()
{
    if (QAction *action = qobject_cast<QAction *>(sender()))
        watchExpression(action->data().toString());
}

QString WatchHandler::watcherName(const QString &exp)
{
    return QLatin1String("watch.") + QString::number(m_watcherNames[exp]);
}

void WatchHandler::watchExpression(const QString &exp)
{
    // FIXME: 'exp' can contain illegal characters
    m_watcherNames[exp] = watcherCounter++;
    WatchData data;
    data.exp = exp;
    data.name = exp;
    if (exp.isEmpty() || exp == watcherEditPlaceHolder())
        data.setAllUnneeded();
    data.iname = watcherName(exp);
    emit watcherInserted(data);
    saveWatchers();
    //emit watchModelUpdateRequested();
}

void WatchHandler::setDisplayedIName(const QString &iname, bool on)
{
    Q_UNUSED(iname);
    Q_UNUSED(on);
/*
    WatchData *d = findData(iname);
    if (!on || !d) {
        delete m_editWindows.take(iname);
        m_displayedINames.remove(iname);
        return;
    }
    if (d->exp.isEmpty()) {
        //emit statusMessageRequested(tr("Sorry. Cannot visualize objects without known address."), 5000);
        return;
    }
    d->setValueNeeded();
    m_displayedINames.insert(iname);
    insertData(*d);
*/
}

void WatchHandler::showEditValue(const WatchData &data)
{
    // editvalue is always base64 encoded
    QByteArray ba = QByteArray::fromBase64(data.editvalue);
    //QByteArray ba = data.editvalue;
    QWidget *w = m_editWindows.value(data.iname);
    qDebug() << "SHOW_EDIT_VALUE " << data.toString() << data.type
            << data.iname << w;
    if (data.type == QLatin1String("QImage")) {
        if (!w) {
            w = new QLabel;
            m_editWindows[data.iname] = w;
        }
        QDataStream ds(&ba, QIODevice::ReadOnly);
        QVariant v;
        ds >> v;
        QString type = QString::fromAscii(v.typeName());
        QImage im = v.value<QImage>();
        if (QLabel *l = qobject_cast<QLabel *>(w))
            l->setPixmap(QPixmap::fromImage(im));
    } else if (data.type == QLatin1String("QPixmap")) {
        if (!w) {
            w = new QLabel;
            m_editWindows[data.iname] = w;
        }
        QDataStream ds(&ba, QIODevice::ReadOnly);
        QVariant v;
        ds >> v;
        QString type = QString::fromAscii(v.typeName());
        QPixmap im = v.value<QPixmap>();
        if (QLabel *l = qobject_cast<QLabel *>(w))
            l->setPixmap(im);
    } else if (data.type == QLatin1String("QString")) {
        if (!w) {
            w = new QTextEdit;
            m_editWindows[data.iname] = w;
        }
#if 0
        QDataStream ds(&ba, QIODevice::ReadOnly);
        QVariant v;
        ds >> v;
        QString type = QString::fromAscii(v.typeName());
        QString str = v.value<QString>();
#else
        MODEL_DEBUG("DATA: " << ba);
        QString str = QString::fromUtf16((ushort *)ba.constData(), ba.size()/2);
#endif
        if (QTextEdit *t = qobject_cast<QTextEdit *>(w))
            t->setText(str);
    }
    if (w)
        w->show();
}

void WatchHandler::removeWatchExpression()
{
    if (QAction *action = qobject_cast<QAction *>(sender()))
        removeWatchExpression(action->data().toString());
}

void WatchHandler::removeWatchExpression(const QString &exp)
{
    MODEL_DEBUG("REMOVE WATCH: " << exp);
    m_watcherNames.remove(exp);
    if (WatchModel *model = m_models[WatchersWatch])
        if (WatchItem *item = model->findItemByExpression(exp, model->root())) {
            model->removeItem(item);
            saveWatchers();
        }
}

void WatchHandler::loadWatchers()
{
    QVariant value;
    sessionValueRequested("Watchers", &value);
    foreach (const QString &exp, value.toStringList())
        m_watcherNames[exp] = watcherCounter++;

    //qDebug() << "LOAD WATCHERS: " << m_watchers;
    //reinitializeWatchersHelper();
}

void WatchHandler::saveWatchers()
{
    //qDebug() << "SAVE WATCHERS: " << m_watchers;
    // Filter out valid watchers.
    QStringList watcherNames;
    QHashIterator<QString, int> it(m_watcherNames);
    while (it.hasNext()) {
        it.next();
        const QString &watcherName = it.key();
        if (!watcherName.isEmpty() && watcherName != watcherEditPlaceHolder())
            watcherNames.push_back(watcherName);
    }
    setSessionValueRequested("Watchers", QVariant(watcherNames));
}

void WatchHandler::loadTypeFormats()
{
    QVariant value;
    sessionValueRequested("DefaultFormats", &value);
    QMap<QString, QVariant> typeFormats = value.toMap();
    QMapIterator<QString, QVariant> it(typeFormats);
    while (it.hasNext()) {
        it.next();
        if (!it.key().isEmpty())
            m_typeFormats.insert(it.key(), it.value().toInt());
    }
}

QStringList WatchHandler::watcherExpressions() const
{
    return m_watcherNames.keys();
}

void WatchHandler::saveTypeFormats()
{
    QMap<QString, QVariant> typeFormats;
    QHashIterator<QString, int> it(m_typeFormats);
    while (it.hasNext()) {
        it.next();
        QString key = it.key().trimmed();
        if (!key.isEmpty())
            typeFormats.insert(key, it.value());
    }
    setSessionValueRequested("DefaultFormats", QVariant(typeFormats));
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
    initWatchModel();
}

void WatchHandler::initWatchModel()
{
    foreach (const QString &exp, m_watcherNames.keys()) {
        WatchData data;
        data.iname = watcherName(exp);
        data.setAllUnneeded();
        data.name = exp;
        data.exp = exp;
        emit watcherInserted(data);
    }
}

WatchModel *WatchHandler::model(WatchType type) const
{
    return m_models[type];
}

void WatchHandler::setModel(WatchType type, WatchModel *model)
{
    if (m_models[type] == model)
        return;
    if (type == WatchersWatch && m_models[type])
        saveWatchers();
    m_models[type] = model;
    if (type == WatchersWatch)
        initWatchModel();
}

WatchType WatchHandler::watchTypeOfIName(const QString &iname)
{
    if (iname.startsWith(QLatin1String("local.")))
        return LocalsWatch;
    if (iname.startsWith(QLatin1String("watch.")))
        return WatchersWatch;
    if (iname.startsWith(QLatin1String("tooltip")))
        return TooltipsWatch;
    return WatchModelCount;
}

WatchModel *WatchHandler::modelForIName(const QString &iname) const
{
    const WatchType t = watchTypeOfIName(iname);
    if (t == WatchModelCount)
        return 0;
    return m_models[t];
}

WatchData *WatchHandler::findItem(const QString &iname) const
{
    const WatchModel *model = modelForIName(iname);
    QTC_ASSERT(model, return 0);
    return model->findItemByIName(iname, model->root());
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
    for (int m = 0; m < WatchModelCount; m++)
        m_models[m]->emitDataChanged(1);
}

void WatchHandler::init(QTreeView *localsView)
{
    m_localsView = localsView;
}

void WatchHandler::saveLocalsViewState(int frame)
{
    WatchModel *model = m_models[LocalsWatch];
    if (!model || !m_localsView)
        return;
    LocalsViewStateMap::iterator it = m_localsViewState.find(frame);
    if (it == m_localsViewState.end())
        it = m_localsViewState.insert(frame, LocalsViewState());

    const QModelIndex topIndex = m_localsView->indexAt(QPoint(0, 0));
    it.value().firstVisibleRow = topIndex.isValid() ? topIndex.row() : 0;
    it.value().expandedINames = m_expandedINames;
}

void WatchHandler::restoreLocalsViewState(int frame)
{
    WatchModel *model = m_models[LocalsWatch];
    if (!model || !m_localsView)
        return;
    int firstVisibleRow = 0;
    const LocalsViewStateMap::const_iterator it = m_localsViewState.constFind(frame);
    if (it !=  m_localsViewState.constEnd()) {
        firstVisibleRow = it.value().firstVisibleRow;
        m_expandedINames = it.value().expandedINames;
    }
    // Loop over and expand valid inames, purge out invalid.
    WatchItem *root = model->root();
    for (QSet<QString>::iterator it = m_expandedINames.begin(); it != m_expandedINames.end(); ) {
        if (const WatchItem *wd = model->findItemByIName(*it, root)) {
            m_localsView->expand(model->watchIndex(wd));
            ++it;
        } else {
            it = m_expandedINames.erase(it);
        }
    }
    if (firstVisibleRow) {
        const QModelIndex index = model->index(0, 0).child(firstVisibleRow, 0);
        if (index.isValid())
            m_localsView->scrollTo(index, QAbstractItemView::PositionAtTop);
    }
}

} // namespace Internal
} // namespace Debugger
