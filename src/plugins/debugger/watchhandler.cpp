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
#include <QtCore/QTextStream>

#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QLabel>
#include <QtGui/QToolTip>
#include <QtGui/QTextEdit>

#include <ctype.h>


// creates debug output regarding pending watch data results
#define DEBUG_PENDING 1
// creates debug output for accesses to the itemmodel
//#define DEBUG_MODEL 1

#if DEBUG_MODEL
#   define MODEL_DEBUG(s) qDebug() << s
#else
#   define MODEL_DEBUG(s) 
#endif
#define MODEL_DEBUGX(s) qDebug() << s

using namespace Debugger::Internal;

static const QString strNotInScope =
        QCoreApplication::translate("Debugger::Internal::WatchData", "<not in scope>");

static int watcherCounter = 0;

////////////////////////////////////////////////////////////////////
//
// WatchData
//
////////////////////////////////////////////////////////////////////

WatchData::WatchData() :
    childCount(-1),
    valuedisabled(false),
    source(0),
    state(InitialState),
    parentIndex(-1),
    row(-1),
    level(-1),
    changed(false)
{
}

void WatchData::setError(const QString &msg)
{
    setAllUnneeded();
    value = msg;
    setChildCount(0);
    valuedisabled = true;
}

void WatchData::setValue(const QString &value0)
{
    value = value0;
    if (value == "{...}") {
        value.clear();
        childCount = 1; // at least one...
    }

    // avoid duplicated information
    if (value.startsWith("(") && value.contains(") 0x"))
        value = value.mid(value.lastIndexOf(") 0x") + 2);

    // doubles are sometimes displayed as "@0x6141378: 1.2".
    // I don't want that.
    if (/*isIntOrFloatType(type) && */ value.startsWith("@0x")
         && value.contains(':')) {
        value = value.mid(value.indexOf(':') + 2);
        setChildCount(0);
    }

    // "numchild" is sometimes lying
    //MODEL_DEBUG("\n\n\nPOINTER: " << type << value);
    if (isPointerType(type))
        setChildCount(value != "0x0" && value != "<null>");

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
        setChildCount(0);
}

void WatchData::setAddress(const QString & str)
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
    str  <<"{state=\"0x" << QString::number(state, 16) << doubleQuoteComma;
    if (level != -1)
         str << "level=\"" << level << doubleQuoteComma;
    if (parentIndex != -1)
         str << "parent=\"" << parentIndex << doubleQuoteComma;
    if (row != -1)
         str << "row=\"" << row << doubleQuoteComma;
    if (childCount)
         str << "childCount=\"" << childCount << doubleQuoteComma;
    if (const int childCount = childIndex.size()) {
        str << "child=\"";
        for (int i = 0; i < childCount; i++) {
            if (i)
                str << ',';
            str << childIndex.at(i);
        }
        str << doubleQuoteComma;
    }

    if (!iname.isEmpty())
        str << "iname=\"" << iname << doubleQuoteComma;
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

    if (isChildCountNeeded())
        str << "numchild=<needed>,";
    if (isChildCountKnown() && childCount == -1)
        str << "numchild=\"" << childCount << doubleQuoteComma;

    if (isChildrenNeeded())
        str << "children=<needed>,";
    str.flush();
    if (res.endsWith(QLatin1Char(',')))
        res.truncate(res.size() - 1);
    return res + QLatin1Char('}');
}

// Format a tooltip fow with aligned colon
template <class Streamable>
        inline void formatToolTipRow(QTextStream &str,
                                     const QString &category,
                                     const Streamable &value)
{
    str << "<tr><td>" << category << "</td><td> : </td><td>" << value  << "</td></tr>";
}

QString WatchData::toToolTip() const
{
    if (!valuetooltip.isEmpty())
        return QString::number(valuetooltip.size());
    QString res;
    QTextStream str(&res);
    str << "<html><body><table>";
    formatToolTipRow(str, WatchHandler::tr("Expression"), Qt::escape(exp));
    formatToolTipRow(str, WatchHandler::tr("Type"), Qt::escape(type));
    QString val = value;
    if (value.size() > 1000) {
        val.truncate(1000);
        val +=  WatchHandler::tr(" ... <cut off>");
    }
    formatToolTipRow(str, WatchHandler::tr("Value"), Qt::escape(val));
    formatToolTipRow(str, WatchHandler::tr("Object Address"), Qt::escape(addr));
    formatToolTipRow(str, WatchHandler::tr("Stored Address"), Qt::escape(saddr));
    formatToolTipRow(str, WatchHandler::tr("iname"), Qt::escape(iname));
    str << "</table></body></html>";
    return res;
}

static bool iNameSorter(const WatchData &d1, const WatchData &d2)
{
    if (d1.level != d2.level)
        return d1.level < d2.level;

    for (int level = 0; level != d1.level; ++level) {
        QString name1 = d1.iname.section('.', level, level);
        QString name2 = d2.iname.section('.', level, level);
        //MODEL_DEBUG(" SORT: " << name1 << name2 << (name1 < name2));
        if (name1 != name2 && !name1.isEmpty() && !name2.isEmpty()) {
            if (name1.at(0).isDigit() && name2.at(0).isDigit())
                return name1.toInt() < name2.toInt();
            return name1 < name2;
        }
    }
    return false; 
}

static QString parentName(const QString &iname)
{
    int pos = iname.lastIndexOf(QLatin1Char('.'));
    if (pos == -1)
        return QString();
    return iname.left(pos);
}


static void insertDataHelper(QList<WatchData> &list, const WatchData &data)
{
    // FIXME: Quadratic algorithm
    for (int i = list.size(); --i >= 0; ) {
        if (list.at(i).iname == data.iname) {
            list[i] = data;
            return;
        }
    }
    list.append(data);
}

static WatchData take(const QString &iname, QList<WatchData> *list)
{
    for (int i = list->size(); --i >= 0;) {
        if (list->at(i).iname == iname) {
            WatchData res = list->at(i);
            (*list)[i] = list->back();
            (void) list->takeLast();
            return res;
            //return list->takeAt(i);
        }
    }
    return WatchData();
}

static QList<WatchData> initialSet()
{
    QList<WatchData> result;

    WatchData root;
    root.state = 0;
    root.level = 0;
    root.row = 0;
    root.name = WatchHandler::tr("Root");
    root.parentIndex = -1;
    root.childIndex.append(1);
    root.childIndex.append(2);
    root.childIndex.append(3);
    result.append(root);

    WatchData local;
    local.iname = QLatin1String("local");
    local.name = WatchHandler::tr("Locals");
    local.state = 0;
    local.level = 1;
    local.row = 0;
    local.parentIndex = 0;
    result.append(local);

    WatchData tooltip;
    tooltip.iname = QLatin1String("tooltip");
    tooltip.name = WatchHandler::tr("Tooltip");
    tooltip.state = 0;
    tooltip.level = 1;
    tooltip.row = 1;
    tooltip.parentIndex = 0;
    result.append(tooltip);

    WatchData watch;
    watch.iname = QLatin1String("watch");
    watch.name = WatchHandler::tr("Watchers");
    watch.state = 0;
    watch.level = 1;
    watch.row = 2;
    watch.parentIndex = 0;
    result.append(watch);

    return result;
}

///////////////////////////////////////////////////////////////////////
//
// WatchHandler
//
///////////////////////////////////////////////////////////////////////

WatchHandler::WatchHandler()
{
    m_expandPointers = true;
    m_inFetchMore = false;
    m_inChange = false;

    m_completeSet = initialSet();
    m_incompleteSet.clear();
    m_displaySet = m_completeSet;

    connect(theDebuggerAction(WatchExpression),
        SIGNAL(triggered()), this, SLOT(watchExpression()));
    connect(theDebuggerAction(RemoveWatchExpression),
        SIGNAL(triggered()), this, SLOT(removeWatchExpression()));
    connect(theDebuggerAction(ExpandItem),
        SIGNAL(triggered()), this, SLOT(expandChildren()));
    connect(theDebuggerAction(CollapseItem),
        SIGNAL(triggered()), this, SLOT(collapseChildren()));
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

static QString niceType(QString type)
{
    if (type.contains(QLatin1String("std::"))) {
        // std::string
        type.replace(QLatin1String("basic_string<char, std::char_traits<char>, "
                                   "std::allocator<char> >"), QLatin1String("string"));

        // std::wstring
        type.replace(QLatin1String("basic_string<wchar_t, std::char_traits<wchar_t>, "
                                   "std::allocator<wchar_t> >"), QLatin1String("wstring"));

        // std::vector
        static QRegExp re1(QLatin1String("vector<(.*), std::allocator<(.*)>\\s*>"));
        re1.setMinimal(true);
        for (int i = 0; i != 10; ++i) {
            if (re1.indexIn(type) == -1 || re1.cap(1) != re1.cap(2)) 
                break;
            type.replace(re1.cap(0), QLatin1String("vector<") + re1.cap(1) + QLatin1Char('>'));
        }

        // std::deque
        static QRegExp re5(QLatin1String("deque<(.*), std::allocator<(.*)>\\s*>"));
        re5.setMinimal(true);
        for (int i = 0; i != 10; ++i) {
            if (re5.indexIn(type) == -1 || re5.cap(1) != re5.cap(2)) 
                break;
            type.replace(re5.cap(0), QLatin1String("deque<") + re5.cap(1) + QLatin1Char('>'));
        }

        // std::stack
        static QRegExp re6(QLatin1String("stack<(.*), std::deque<(.*), std::allocator<(.*)>\\s*> >"));
        re6.setMinimal(true);
        for (int i = 0; i != 10; ++i) {
            if (re6.indexIn(type) == -1 || re6.cap(1) != re6.cap(2)
                    || re6.cap(1) != re6.cap(2)) 
                break;
            type.replace(re6.cap(0), QLatin1String("stack<") + re6.cap(1) + QLatin1Char('>'));
        }

        // std::list
        static QRegExp re2(QLatin1String("list<(.*), std::allocator<(.*)>\\s*>"));
        re2.setMinimal(true);
        for (int i = 0; i != 10; ++i) {
            if (re2.indexIn(type) == -1 || re2.cap(1) != re2.cap(2)) 
                break;
            type.replace(re2.cap(0), QLatin1String("list<") + re2.cap(1) + QLatin1Char('>'));
        }

        // std::map
        {
            static QRegExp re(QLatin1String("map<(.*), (.*), std::less<(.*)>, "
                "std::allocator<std::pair<(.*), (.*)> > >"));
            re.setMinimal(true);
            for (int i = 0; i != 10; ++i) {
                if (re.indexIn(type) == -1)
                    break;
                QString key = chopConst(re.cap(1));
                QString value = chopConst(re.cap(2));
                QString key1 = chopConst(re.cap(3));
                QString key2 = chopConst(re.cap(4));
                QString value2 = chopConst(re.cap(5));
                if (value != value2 || key != key1 || key != key2) {
                    qDebug() << key << key1 << key2 << value << value2
                        << (key == key1) << (key == key2) << (value == value2);
                    break;
                }
                type.replace(re.cap(0), QString("map<%1, %2>").arg(key).arg(value));
            }
        }

        // std::set
        static QRegExp re4(QLatin1String("set<(.*), std::less<(.*)>, std::allocator<(.*)>\\s*>"));
        re1.setMinimal(true);
        for (int i = 0; i != 10; ++i) {
            if (re4.indexIn(type) == -1 || re4.cap(1) != re4.cap(2)
                || re4.cap(1) != re4.cap(3)) 
                break;
            type.replace(re4.cap(0), QLatin1String("set<") + re4.cap(1) + QLatin1Char('>'));
        }

        type.replace(QLatin1String(" >"), QString(QLatin1Char('>')));
    }
    return type;
}

QVariant WatchHandler::data(const QModelIndex &idx, int role) const
{
    int node = idx.internalId();
    if (node < 0)
        return QVariant();
    QTC_ASSERT(node < m_displaySet.size(), return QVariant());

    const WatchData &data = m_displaySet.at(node);

    switch (role) {
        case Qt::DisplayRole: {
            switch (idx.column()) {
                case 0: return data.name;
                case 1: return data.value;
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
            switch (idx.column()) {
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
            //qDebug() << " FETCHING: " << data.iname
            //    << m_expandedINames.contains(data.iname)
            //    << m_expandedINames;
            // Level 0 and 1 are always expanded
            return node < 4 || m_expandedINames.contains(data.iname);
    
        default:
            break; 
    }
    return QVariant();
}

bool WatchHandler::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(role);
    Q_UNUSED(value);
    emit dataChanged(index, index);
    return true;
}

Qt::ItemFlags WatchHandler::flags(const QModelIndex &idx) const
{
    using namespace Qt;

    if (!idx.isValid())
        return ItemFlags();

    int node = idx.internalId();
    if (node < 0)
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

    const WatchData &data = m_displaySet.at(node);

    if (data.isWatcher() && idx.column() == 0)
        return editable; // watcher names are editable
    if (data.isWatcher() && idx.column() == 2)
        return editable; // watcher types are
    if (idx.column() == 1)
        return editable; // locals and watcher values are editable
    return  notEditable;
}

QVariant WatchHandler::headerData(int section, Qt::Orientation orientation,
    int role) const
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

QString WatchHandler::toString() const
{
    QString res;
    QTextStream str(&res);
    str << "\nIncomplete:\n";
    for (int i = 0, n = m_incompleteSet.size(); i != n; ++i)
        str << i << ' ' << m_incompleteSet.at(i).toString() << '\n';
    str << "\nComplete:\n";
    for (int i = 0, n = m_completeSet.size(); i != n; ++i)
        str << i << ' ' << m_completeSet.at(i).toString() << '\n';
    str << "\nDisplay:\n";
    for (int i = 0, n = m_displaySet.size(); i != n; ++i)
        str << i << ' ' << m_displaySet.at(i).toString() << '\n';

#if 0
    str << "\nOld:\n";
    for (int i = 0, n = m_oldSet.size(); i != n; ++i)
        str << m_oldSet.at(i).toString() <<  '\n';
#endif
    return res;
}

WatchData *WatchHandler::findData(const QString &iname)
{
    for (int i = m_completeSet.size(); --i >= 0; )
        if (m_completeSet.at(i).iname == iname)
            return &m_completeSet[i];
    return 0;
}

WatchData WatchHandler::takeData(const QString &iname)
{
    WatchData data = take(iname, &m_incompleteSet);
    if (data.isValid())
        return data;
    return take(iname, &m_completeSet);
}

QList<WatchData> WatchHandler::takeCurrentIncompletes()
{
    QList<WatchData> res = m_incompleteSet;
    //MODEL_DEBUG("TAKING INCOMPLETES" << toString());
    m_incompleteSet.clear();
    return res;
}

void WatchHandler::rebuildModel()
{
    if (m_inChange) {
        MODEL_DEBUG("RECREATE MODEL IGNORED, CURRENT SET:\n" << toString());
        return;
    }

    #ifdef DEBUG_PENDING
    MODEL_DEBUG("RECREATE MODEL, CURRENT SET:\n" << toString());
    #endif

    QHash<QString, int> oldTopINames;
    QHash<QString, QString> oldValues;
    for (int i = 0, n = m_oldSet.size(); i != n; ++i) {
        WatchData &data = m_oldSet[i];
        oldValues[data.iname] = data.value;
        if (data.level == 2)
            ++oldTopINames[data.iname];
    }
    #ifdef DEBUG_PENDING
    MODEL_DEBUG("OLD VALUES: " << oldValues);
    #endif

    for (int i = m_completeSet.size(); --i >= 0; ) {
        WatchData &data = m_completeSet[i];
        data.level = data.iname.isEmpty() ? 0 : data.iname.count('.') + 1;
        data.childIndex.clear();
    }

    qSort(m_completeSet.begin(), m_completeSet.end(), &iNameSorter);

    // This helps to decide whether the view has completely changed or not.
    QHash<QString, int> topINames;

    QHash<QString, int> iname2idx;

    for (int i = m_completeSet.size(); --i > 0; ) {
        WatchData &data = m_completeSet[i];
        data.parentIndex = 0;
        data.childIndex.clear();
        iname2idx[data.iname] = i;
        if (data.level == 2)
            ++topINames[data.iname];
    }
    //qDebug() << "TOPINAMES: " << topINames << "\nOLD: " << oldTopINames;

    for (int i = 1; i < m_completeSet.size(); ++i) {
        WatchData &data = m_completeSet[i];
        QString parentIName = parentName(data.iname);
        data.parentIndex = iname2idx.value(parentIName, 0);
        WatchData &parent = m_completeSet[data.parentIndex];
        data.row = parent.childIndex.size();
        parent.childIndex.append(i);
    }

    m_oldSet = m_completeSet;
    m_oldSet += m_incompleteSet;

    for (int i = 0, n = m_completeSet.size(); i != n; ++i) {
        WatchData &data = m_completeSet[i];
        data.changed = !data.value.isEmpty()
            && data.value != oldValues[data.iname]
            && data.value != strNotInScope;
    }

    emit layoutAboutToBeChanged();

    if (0 && oldTopINames != topINames) {
        m_displaySet = initialSet();
        m_expandedINames.clear();
        emit reset();
    }

    m_displaySet = m_completeSet;

    #ifdef DEBUG_PENDING
    MODEL_DEBUG("SET  " << toString());
    #endif

#if 1
    // Append dummy item to get the [+] effect
    for (int i = 0, n = m_displaySet.size(); i != n; ++i) {
        WatchData &data = m_displaySet[i];
        if (data.childCount > 0 && data.childIndex.size() == 0) {
            WatchData dummy;
            dummy.state = 0;
            dummy.row = 0;
            dummy.iname = data.iname + QLatin1String(".dummy");
            //dummy.name = data.iname + ".dummy";
            //dummy.name  = "<loading>";
            dummy.level = data.level + 1;
            dummy.parentIndex = i;
            dummy.childCount = 0;
            data.childIndex.append(m_displaySet.size());
            m_displaySet.append(dummy); 
        }
    }
#endif

    // Possibly append dummy items to prevent empty views
    bool ok = true;
    QTC_ASSERT(m_displaySet.size() >= 2, ok = false);
    QTC_ASSERT(m_displaySet.at(1).iname == QLatin1String("local"), ok = false);
    QTC_ASSERT(m_displaySet.at(2).iname == QLatin1String("tooltip"), ok = false);
    QTC_ASSERT(m_displaySet.at(3).iname == QLatin1String("watch"), ok = false);
    if (ok) {
        for (int i = 1; i <= 3; ++i) {
            WatchData &data = m_displaySet[i];
            if (data.childIndex.size() == 0) {
                WatchData dummy;
                dummy.state = 0;
                dummy.row = 0;
                if (i == 1) {
                    dummy.iname = QLatin1String("local.dummy");
                    dummy.name  = tr("<No Locals>");
                } else if (i == 2) {
                    dummy.iname = QLatin1String("tooltip.dummy");
                    dummy.name  = tr("<No Tooltip>");
                } else {
                    dummy.iname = QLatin1String("watch.dummy");
                    dummy.name  = tr("<No Watchers>");
                }
                dummy.level = 2;
                dummy.parentIndex = i;
                dummy.childCount = 0;
                data.childIndex.append(m_displaySet.size());
                m_displaySet.append(dummy); 
            }
        }
    }

    m_inChange = true;
    //qDebug() << "WATCHHANDLER: RESET ABOUT TO EMIT";
    //emit reset();
    emit layoutChanged();
    //qDebug() << "WATCHHANDLER: RESET EMITTED";
    m_inChange = false;

    #if DEBUG_MODEL
    #if USE_MODEL_TEST
    //(void) new ModelTest(this, this);
    #endif
    #endif
    
    #ifdef DEBUG_PENDING
    MODEL_DEBUG("SORTED: " << toString());
    MODEL_DEBUG("EXPANDED INAMES: " << m_expandedINames);
    #endif
}

void WatchHandler::cleanup()
{
    m_oldSet.clear();
    m_expandedINames.clear();
    m_displayedINames.clear();

    m_incompleteSet.clear();
    m_completeSet = initialSet();
    m_displaySet = m_completeSet;

    rebuildModel(); // to get the dummy entries back

#if 0
    for (EditWindows::ConstIterator it = m_editWindows.begin();
            it != m_editWindows.end(); ++it) {
        if (!it.value().isNull())
            delete it.value();
    }
    m_editWindows.clear();
#endif
    emit reset();
}

void WatchHandler::collapseChildren()
{
    if (QAction *act = qobject_cast<QAction *>(sender()))
        collapseChildren(act->data().toString());
}

void WatchHandler::collapseChildren(const QString &iname)
{
    if (m_inChange || m_completeSet.isEmpty()) {
        qDebug() << "WATCHHANDLER: COLLAPSE IGNORED" << iname;
        return;
    }
    MODEL_DEBUG("COLLAPSE NODE" << iname);
#if 0
    QString iname1 = iname0 + '.';
    for (int i = m_completeSet.size(); --i >= 0; ) {
        QString iname = m_completeSet.at(i).iname;
        if (iname.startsWith(iname1)) {
            // Better leave it in in case the user re-enters the branch?
            (void) m_completeSet.takeAt(i);
            MODEL_DEBUG(" REMOVING " << iname);
            m_expandedINames.remove(iname);
        }
    }
#endif
    m_expandedINames.remove(iname);
    //MODEL_DEBUG(toString());
    //rebuildModel();
}

void WatchHandler::expandChildren()
{
    if (QAction *act = qobject_cast<QAction *>(sender()))
        expandChildren(act->data().toString());
}

void WatchHandler::expandChildren(const QString &iname)
{
    if (m_inChange || m_completeSet.isEmpty()) {
        //qDebug() << "WATCHHANDLER: EXPAND IGNORED" << iname;
        return;
    }
    int index = -1;
    for (int i = 0; i != m_displaySet.size(); ++i) {
        if (m_displaySet.at(i).iname == iname) {
            index = i;
            break;
        }
    }

    if (index == -1)
        return;
    QTC_ASSERT(index >= 0, qDebug() << toString() << index; return);
    QTC_ASSERT(index < m_completeSet.size(), qDebug() << toString() << index; return);
    const WatchData &display = m_displaySet.at(index);
    QTC_ASSERT(index >= 0, qDebug() << toString() << index; return);
    QTC_ASSERT(index < m_completeSet.size(), qDebug() << toString() << index; return);
    const WatchData &complete = m_completeSet.at(index);
    MODEL_DEBUG("\n\nEXPAND" << display.iname);
    if (display.iname.isEmpty()) {
        // This should not happen but the view seems to send spurious
        // "expand()" signals folr the root item from time to time.
        // Try to handle that gracfully.
        //MODEL_DEBUG(toString());
        qDebug() << "FIXME: expandChildren, no data " << display.iname << "found";
        //rebuildModel();
        return;
    }

    //qDebug() << "   ... NODE: " << display.toString()
    //         << complete.childIndex.size() << complete.childCount;

    if (m_expandedINames.contains(display.iname))
        return;

    // This is a performance hack and not strictly necessary.
    // Remove it if there are troubles when expanding nodes.
    if (0 && complete.childCount > 0 && complete.childIndex.size() > 0) {
        MODEL_DEBUG("SKIP FETCHING CHILDREN");
        return;
    }

    WatchData data = takeData(display.iname); // remove previous data
    m_expandedINames.insert(data.iname);
    if (data.iname.contains('.')) // not for top-level items
        data.setChildrenNeeded();
    insertData(data);
    emit watchModelUpdateRequested();
}

void WatchHandler::insertData(const WatchData &data)
{
    //MODEL_DEBUG("INSERTDATA: " << data.toString());
    QTC_ASSERT(data.isValid(), return);
    if (data.isSomethingNeeded())
        insertDataHelper(m_incompleteSet, data);
    else
        insertDataHelper(m_completeSet, data);
    //MODEL_DEBUG("INSERT RESULT" << toString());
}

void WatchHandler::watchExpression()
{
    if (QAction *action = qobject_cast<QAction *>(sender()))
        watchExpression(action->data().toString());
}

QString WatchHandler::watcherName(const QString &exp)
{
    return QLatin1String("watch.") + QString::number(m_watchers[exp]);
}

void WatchHandler::watchExpression(const QString &exp)
{
    // FIXME: 'exp' can contain illegal characters
    m_watchers[exp] = watcherCounter++;
    WatchData data;
    data.exp = exp;
    data.name = exp;
    data.iname = watcherName(exp);
    insertData(data);
    saveWatchers();
    emit watchModelUpdateRequested();
}

void WatchHandler::setDisplayedIName(const QString &iname, bool on)
{
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
    m_watchers.remove(exp);
    for (int i = m_completeSet.size(); --i >= 0;) {
        const WatchData & data = m_completeSet.at(i);
        if (data.iname.startsWith(QLatin1String("watch.")) && data.exp == exp) {
            m_completeSet.takeAt(i);
            break;
        }
    }
    saveWatchers();
    rebuildModel();
    emit watchModelUpdateRequested();
}

void WatchHandler::reinitializeWatchers()
{
    m_completeSet = initialSet();
    m_incompleteSet.clear();
    reinitializeWatchersHelper();
}

void WatchHandler::reinitializeWatchersHelper()
{
    // copy over all watchers and mark all watchers as incomplete
    int i = 0;
    foreach (const QString &exp, m_watchers.keys()) {
        WatchData data;
        data.iname = watcherName(exp);
        data.level = -1;
        data.row = -1;
        data.parentIndex = -1;
        data.variable.clear();
        data.setAllNeeded();
        data.valuedisabled = false;
        data.name = exp;
        data.exp = exp;
        insertData(data);
        ++i;
    }
}

bool WatchHandler::canFetchMore(const QModelIndex &parent) const
{
    MODEL_DEBUG("CAN FETCH MORE: " << parent << "false");
#if 1
    Q_UNUSED(parent);
    return false;
#else
    // FIXME: not robust enough. Problem is that fetchMore
    // needs to be made synchronous to be useful. Busy loop is no good.
    if (!parent.isValid())
        return false;
    QTC_ASSERT(checkIndex(parent.internalId()), return false);
    const WatchData &data = m_displaySet.at(parent.internalId());
    MODEL_DEBUG("CAN FETCH MORE: " << parent << " children: " << data.childCount
        << data.iname);
    return data.childCount > 0;
#endif
}

void WatchHandler::fetchMore(const QModelIndex &parent)
{
    MODEL_DEBUG("FETCH MORE: " << parent);
    return;

    QTC_ASSERT(checkIndex(parent.internalId()), return);
    QString iname = m_displaySet.at(parent.internalId()).iname;

    if (m_inFetchMore) {
        MODEL_DEBUG("LOOP IN FETCH MORE" << iname);
        return;
    }
    m_inFetchMore = true;

    WatchData data = takeData(iname);
    MODEL_DEBUG("FETCH MORE: " << parent << ':' << iname << data.name);

    if (!data.isValid()) {
        MODEL_DEBUG("FIXME: FETCH MORE, no data " << iname << "found");
        return;
    }

    m_expandedINames.insert(data.iname);
    if (data.iname.contains('.')) // not for top-level items
        data.setChildrenNeeded();

    MODEL_DEBUG("FETCH MORE: data:" << data.toString());
    insertData(data);
    //emit watchUpdateRequested();

    while (m_inFetchMore) {
        QApplication::processEvents();
    }
    m_inFetchMore = false;
    MODEL_DEBUG("BUSY LOOP FINISHED, data:" << data.toString());
}

QModelIndex WatchHandler::index(int row, int col, const QModelIndex &parent) const
{
    #ifdef DEBUG_MODEL
    MODEL_DEBUG("INDEX " << row << col << parent);
    #endif
    //if (col != 0) {
    //    MODEL_DEBUG(" -> " << QModelIndex() << " (3) ");
    //    return QModelIndex();
    //}
    if (row < 0) {
        MODEL_DEBUG(" -> " << QModelIndex() << " (4) ");
        return QModelIndex();
    }
    if (!parent.isValid()) {
        if (row == 0 && col >= 0 && col < 3 && parent.row() == -1) {
            MODEL_DEBUG(" ->  " << createIndex(0, 0, 0) << " (B) ");
            return createIndex(0, col, 0);
        }
        MODEL_DEBUG(" -> " << QModelIndex() << " (1) ");
        return QModelIndex();
    }
    int parentIndex = parent.internalId();
    if (parentIndex < 0) {
        //MODEL_DEBUG("INDEX " << row << col << parentIndex << "INVALID");
        MODEL_DEBUG(" -> " << QModelIndex() << " (2) ");
        return QModelIndex(); 
    }
    QTC_ASSERT(checkIndex(parentIndex), return QModelIndex());
    const WatchData &data = m_displaySet.at(parentIndex);
    QTC_ASSERT(row >= 0, qDebug() << "ROW: " << row  << "PARENT: " << parent
        << data.toString() << toString(); return QModelIndex());
    QTC_ASSERT(row < data.childIndex.size(),
        MODEL_DEBUG("ROW: " << row << data.toString() << toString());
        return QModelIndex());
    QModelIndex idx = createIndex(row, col, data.childIndex.at(row));
    QTC_ASSERT(idx.row() == m_displaySet.at(idx.internalId()).row,
        return QModelIndex());
    MODEL_DEBUG(" -> " << idx << " (A) ");
    return idx;
}

QModelIndex WatchHandler::parent(const QModelIndex &idx) const
{
    if (!idx.isValid()) {
        MODEL_DEBUG(" -> " << QModelIndex() << " (1) ");
        return QModelIndex();
    }
    MODEL_DEBUG("PARENT " << idx);
    int currentIndex = idx.internalId();
    QTC_ASSERT(checkIndex(currentIndex), return QModelIndex());
    QTC_ASSERT(idx.row() == m_displaySet.at(currentIndex).row,
        MODEL_DEBUG("IDX: " << idx << toString(); return QModelIndex()));
    int parentIndex = m_displaySet.at(currentIndex).parentIndex;
    if (parentIndex < 0) {
        MODEL_DEBUG(" -> " << QModelIndex() << " (2) ");
        return QModelIndex();
    }
    QTC_ASSERT(checkIndex(parentIndex), return QModelIndex());
    QModelIndex parent = 
        createIndex(m_displaySet.at(parentIndex).row, 0, parentIndex);
    MODEL_DEBUG(" -> " << parent);
    return parent;
}

int WatchHandler::rowCount(const QModelIndex &idx) const
{
    MODEL_DEBUG("ROW COUNT " << idx);
    if (idx.column() > 0) {
        MODEL_DEBUG(" -> " << 0 << " (A) ");
        return 0;
    }
    int thisIndex = idx.internalId();
    QTC_ASSERT(checkIndex(thisIndex), return 0);
    if (idx.row() == -1 && idx.column() == -1) {
        MODEL_DEBUG(" -> " << 3 << " (B) ");
        return 1;
    }
    if (thisIndex < 0) {
        MODEL_DEBUG(" -> " << 0 << " (C) ");
        return 0;
    }
    if (thisIndex == 0) {
        MODEL_DEBUG(" -> " << 3 << " (D) ");
        return 3;
    }
    const WatchData &data = m_displaySet.at(thisIndex);
    int rows = data.childIndex.size();
    MODEL_DEBUG(" -> " << rows << " (E) ");
    return rows;
	// Try lazy evaluation
    //if (rows > 0)
    //    return rows;
    //return data.childCount;
}

int WatchHandler::columnCount(const QModelIndex &idx) const
{
    MODEL_DEBUG("COLUMN COUNT " << idx);
    if (idx == QModelIndex()) {
        MODEL_DEBUG(" -> " << 3 << " (C) ");
        return 3;
    }
    if (idx.column() != 0) {
        MODEL_DEBUG(" -> " << 0 << " (A) ");
        return 0;
    }
    MODEL_DEBUG(" -> " << 3 << " (B) ");
    QTC_ASSERT(checkIndex(idx.internalId()), return 3);
    return 3;
}

bool WatchHandler::hasChildren(const QModelIndex &idx) const
{
    // that's the base implementation:
    bool base = rowCount(idx) > 0 && columnCount(idx) > 0;
    MODEL_DEBUG("HAS CHILDREN: " << idx << base);
    return base;
    QTC_ASSERT(checkIndex(idx.internalId()), return false);
    const WatchData &data = m_displaySet.at(idx.internalId());
    MODEL_DEBUG("HAS CHILDREN: " << idx << data.toString());
    return data.childCount > 0; // || data.childIndex.size() > 0;
}

bool WatchHandler::checkIndex(int id) const
{
    if (id < 0) {
        MODEL_DEBUG("CHECK INDEX FAILED" << id);
        return false;
    }
    if (id >= m_displaySet.size()) {
        MODEL_DEBUG("CHECK INDEX FAILED" << id << toString());
        return false;
    }
    return true;
}

void WatchHandler::loadWatchers()
{
    QVariant value;
    sessionValueRequested("Watchers", &value);
    foreach (const QString &exp, value.toStringList())
        m_watchers[exp] = watcherCounter++;
    //qDebug() << "LOAD WATCHERS: " << m_watchers;
    reinitializeWatchersHelper();
}

void WatchHandler::saveWatchers()
{
    //qDebug() << "SAVE WATCHERS: " << m_watchers.keys();
    setSessionValueRequested("Watchers", QVariant(m_watchers.keys()));
}

void WatchHandler::saveSessionData()
{
    saveWatchers();
}

void WatchHandler::loadSessionData()
{
    loadWatchers();
    rebuildModel();
}
