/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "watchhandler.h"

#if USE_MODEL_TEST
#include "modeltest.h"
#endif

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QEvent>

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

static const QString strNotInScope = QLatin1String("<not in scope>");

static bool isIntOrFloatType(const QString &type)
{
    static const QStringList types = QStringList()
        << "char" << "int" << "short" << "float" << "double" << "long"
        << "bool" << "signed char" << "unsigned" << "unsigned char"
        << "unsigned int" << "unsigned long" << "long long";
    return types.contains(type);
}

static bool isPointerType(const QString &type)
{
    return type.endsWith("*") || type.endsWith("* const");
}

static QString htmlQuote(const QString &str0)
{
    QString str = str0;
    str.replace('&', "&amp;");
    str.replace('<', "&lt;");
    str.replace('>', "&gt;");
    return str;
}

////////////////////////////////////////////////////////////////////
//
// WatchData
//
////////////////////////////////////////////////////////////////////

WatchData::WatchData()
{
    valuedisabled = false;
    state = InitialState;
    childCount = -1;
    parentIndex = -1;
    row = -1;
    level = -1;
    changed = false;
}

void WatchData::setError(const QString &msg)
{
    setAllUnneeded();
    value = msg;
    setChildCount(0);
    valuedisabled = true;
}

static QByteArray quoteUnprintable(const QByteArray &ba)
{
    QByteArray res;
    char buf[10];
    for (int i = 0, n = ba.size(); i != n; ++i) {
        unsigned char c = ba.at(i);
        if (isprint(c)) {
            res += c;
        } else {
            qsnprintf(buf, sizeof(buf) - 1, "\\%x", int(c));
            res += buf;
        }
    }
    return res;
}

void WatchData::setValue(const QByteArray &value0)
{
    value = quoteUnprintable(value0);
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
        if (type.endsWith("const"))
            type.chop(5);
        else if (type.endsWith(" "))
            type.chop(1);
        else if (type.endsWith("&"))
            type.chop(1);
        else if (type.startsWith("const "))
            type = type.mid(6);
        else if (type.startsWith("volatile "))
            type = type.mid(9);
        else if (type.startsWith("class "))
            type = type.mid(6);
        else if (type.startsWith("struct "))
            type = type.mid(6);
        else if (type.startsWith(" "))
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

QString WatchData::toString() const
{
    QString res = "{";

    res += "level=\"" + QString::number(level) + "\",";
    res += "parent=\"" + QString::number(parentIndex) + "\",";
    res += "row=\"" + QString::number(row) + "\",";
    res += "child=\"";
    foreach (int index, childIndex)
        res += QString::number(index) + ",";
    if (res.endsWith(','))
        res[res.size() - 1] = '"';
    else
        res += '"';
    res += ",";
    

    if (!iname.isEmpty())
        res += "iname=\"" + iname + "\",";
    if (!exp.isEmpty())
        res += "exp=\"" + exp + "\",";

    if (!variable.isEmpty())
        res += "variable=\"" + variable + "\",";

    if (isValueNeeded())
        res += "value=<needed>,";
    if (isValueKnown() && !value.isEmpty())
        res += "value=\"" + value + "\",";

    if (!editvalue.isEmpty())
        res += "editvalue=\"" + editvalue + "\",";

    if (isTypeNeeded())
        res += "type=<needed>,";
    if (isTypeKnown() && !type.isEmpty())
        res += "type=\"" + type + "\",";

    if (isChildCountNeeded())
        res += "numchild=<needed>,";
    if (isChildCountKnown() && childCount == -1)
        res += "numchild=\"" + QString::number(childCount) + "\",";

    if (isChildrenNeeded())
        res += "children=<needed>,";

    if (res.endsWith(','))
        res[res.size() - 1] = '}';
    else
        res += '}';

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
    int pos = iname.lastIndexOf(".");
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
    root.name = "Root";
    root.parentIndex = -1;
    root.childIndex.append(1);
    root.childIndex.append(2);
    root.childIndex.append(3);
    result.append(root);

    WatchData local;
    local.iname = "local";
    local.name = "Locals";
    local.state = 0;
    local.level = 1;
    local.row = 0;
    local.parentIndex = 0;
    result.append(local);

    WatchData tooltip;
    tooltip.iname = "tooltip";
    tooltip.name = "Tooltip";
    tooltip.state = 0;
    tooltip.level = 1;
    tooltip.row = 1;
    tooltip.parentIndex = 0;
    result.append(tooltip);

    WatchData watch;
    watch.iname = "watch";
    watch.name = "Watchers";
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
}

bool WatchHandler::setData(const QModelIndex &idx,
    const QVariant &value, int role)
{
/*
    Q_UNUSED(idx);
    Q_UNUSED(value);
    Q_UNUSED(role);
    if (role == VisualRole) {
        QString iname = inameFromIndex(index);
        setDisplayedIName(iname, value.toBool());
        return true;
    }
    return true;
*/
    return QAbstractItemModel::setData(idx, value, role);
}

static QString niceType(QString type)
{
    if (type.contains("std::")) {
        // std::string
        type.replace("std::basic_string<char, std::char_traits<char>, "
                     "std::allocator<char> >", "std::string");

        // std::wstring
        type.replace("std::basic_string<wchar_t, std::char_traits<wchar_t>, "
                     "std::allocator<wchar_t> >", "std::wstring");

        // std::vector
        static QRegExp re1("std::vector<(.*), std::allocator<(.*)>\\s*>");
        re1.setMinimal(true);
        for (int i = 0; i != 10; ++i) {
            if (re1.indexIn(type) == -1 || re1.cap(1) != re1.cap(2)) 
                break;
            type.replace(re1.cap(0), "std::vector<" + re1.cap(1) + ">");
        }

        // std::list
        static QRegExp re2("std::list<(.*), std::allocator<(.*)>\\s*>");
        re2.setMinimal(true);
        for (int i = 0; i != 10; ++i) {
            if (re2.indexIn(type) == -1 || re2.cap(1) != re2.cap(2)) 
                break;
            type.replace(re2.cap(0), "std::list<" + re2.cap(1) + ">");
        }

        // std::map
        static QRegExp re3("std::map<(.*), (.*), std::less<(.*)\\s*>, "
            "std::allocator<std::pair<const (.*), (.*)\\s*> > >");
        re3.setMinimal(true);
        for (int i = 0; i != 10; ++i) {
            if (re3.indexIn(type) == -1 || re3.cap(1) != re3.cap(3)
                || re3.cap(1) != re3.cap(4) || re3.cap(2) != re3.cap(5)) 
                break;
            type.replace(re3.cap(0), "std::map<" + re3.cap(1) + ", " + re3.cap(2) + ">");
        }

        type.replace(" >", ">");
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

        case Qt::ToolTipRole: {
            QString val = data.value;
            if (val.size() > 1000)
                val = val.left(1000) + " ... <cut off>";

            QString tt = "<table>";
            //tt += "<tr><td>internal name</td><td> : </td><td>";
            //tt += htmlQuote(iname) + "</td></tr>";
            tt += "<tr><td>expression</td><td> : </td><td>";
            tt += htmlQuote(data.exp) + "</td></tr>";
            tt += "<tr><td>type</td><td> : </td><td>";
            tt += htmlQuote(data.type) + "</td></tr>";
            //if (!valuetooltip.isEmpty())
            //    tt += valuetooltip;
            //else
                tt += "<tr><td>value</td><td> : </td><td>"; 
                tt += htmlQuote(data.value) + "</td></tr>";
            tt += "<tr><td>addr</td><td> : </td><td>";
            tt += htmlQuote(data.addr) + "</td></tr>";
            tt += "<tr><td>iname</td><td> : </td><td>";
            tt += htmlQuote(data.iname) + "</td></tr>";
            tt += "</table>";
            tt.replace("@value@", htmlQuote(data.value));

            if (tt.size() > 10000)
                tt = tt.left(10000) + " ... <cut off>";
            return tt;
        }

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

        case INameRole:
            return data.iname;

        case VisualRole:
            return m_displayedINames.contains(data.iname);
    
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

    static const ItemFlags DefaultNotEditable =
          ItemIsSelectable
        | ItemIsDragEnabled
        | ItemIsDropEnabled
        // | ItemIsUserCheckable
        // | ItemIsTristate
        | ItemIsEnabled;

    static const ItemFlags DefaultEditable =
        DefaultNotEditable | ItemIsEditable;

    const WatchData &data = m_displaySet.at(node);
    return idx.column() == 1 &&
        data.isWatcher() ? DefaultEditable : DefaultNotEditable;
}

QVariant WatchHandler::headerData(int section, Qt::Orientation orientation,
    int role) const
{
    if (orientation == Qt::Vertical)
        return QVariant();
    if (role == Qt::DisplayRole) {
        switch (section) {
            case 0: return tr("Name")  + "     ";
            case 1: return tr("Value") + "     ";
            case 2: return tr("Type")  + "     ";
        }
    }
    return QVariant(); 
}

QString WatchHandler::toString() const
{
    QString res;
    res += "\nIncomplete:\n";
    for (int i = 0, n = m_incompleteSet.size(); i != n; ++i) {
        res += QString("%1: ").arg(i);
        res += m_incompleteSet.at(i).toString();
        res += '\n';
    }
    res += "\nComplete:\n";
    for (int i = 0, n = m_completeSet.size(); i != n; ++i) {
        res += QString("%1: ").arg(i);
        res += m_completeSet.at(i).toString();
        res += '\n';
    }
    res += "\nDisplay:\n";
    for (int i = 0, n = m_displaySet.size(); i != n; ++i) {
        res += QString("%1: ").arg(i);
        res += m_displaySet.at(i).toString();
        res += '\n';
    }
#if 0
    res += "\nOld:\n";
    for (int i = 0, n = m_oldSet.size(); i != n; ++i) {
        res += m_oldSet.at(i).toString();
        res += '\n';
    }
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

    if (oldTopINames != topINames) {
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
            dummy.iname = data.iname + ".dummy";
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
    QTC_ASSERT(m_displaySet.at(1).iname == "local", ok = false);
    QTC_ASSERT(m_displaySet.at(2).iname == "tooltip", ok = false);
    QTC_ASSERT(m_displaySet.at(3).iname == "watch", ok = false);
    if (ok) {
        for (int i = 1; i <= 3; ++i) {
            WatchData &data = m_displaySet[i];
            if (data.childIndex.size() == 0) {
                WatchData dummy;
                dummy.state = 0;
                dummy.row = 0;
                if (i == 1) {
                    dummy.iname = "local.dummy";
                    dummy.name  = "<No Locals>";
                } else if (i == 2) {
                    dummy.iname = "tooltip.dummy";
                    dummy.name  = "<No Tooltip>";
                } else {
                    dummy.iname = "watch.dummy";
                    dummy.name  = "<No Watchers>";
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
    emit reset();
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

void WatchHandler::collapseChildren(const QModelIndex &idx)
{
    if (m_inChange || m_completeSet.isEmpty()) {
        qDebug() << "WATCHHANDLER: COLLAPSE IGNORED" << idx;
        return;
    }
    QTC_ASSERT(checkIndex(idx.internalId()), return);
    QString iname0 = m_displaySet.at(idx.internalId()).iname;
    MODEL_DEBUG("COLLAPSE NODE" << iname0);
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
    m_expandedINames.remove(iname0);
    //MODEL_DEBUG(toString());
    //rebuildModel();
}

void WatchHandler::expandChildren(const QModelIndex &idx)
{
    if (m_inChange || m_completeSet.isEmpty()) {
        //qDebug() << "WATCHHANDLER: EXPAND IGNORED" << idx;
        return;
    }
    int index = idx.internalId();
    if (index == 0)
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
        qDebug() << "FIXME: expandChildren, no data " << display.iname << "found"
            << idx;
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

void WatchHandler::watchExpression(const QString &exp)
{
    // FIXME: 'exp' can contain illegal characters
    //MODEL_DEBUG("WATCH: " << exp);
    WatchData data;
    data.exp = exp;
    data.name = exp;
    data.iname = "watch." + exp;
    insertData(data);
    m_watchers.append(exp);
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
    if (data.type == "QImage") {
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
    } else if (data.type == "QPixmap") {
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
    } else if (data.type == "QString") {
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

void WatchHandler::removeWatchExpression(const QString &exp)
{
    MODEL_DEBUG("REMOVE WATCH: " << iname);
    m_watchers.removeOne(exp);
    for (int i = m_completeSet.size(); --i >= 0;) {
        const WatchData & data = m_completeSet.at(i);
        if (data.iname.startsWith("watch.") && data.exp == exp) {
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
    foreach (const QString &exp, m_watchers) {
        WatchData data;
        data.level = -1;
        data.row = -1;
        data.parentIndex = -1;
        data.variable.clear();
        data.setAllNeeded();
        data.valuedisabled = false;
        data.iname = "watch." + QString::number(i);
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
    MODEL_DEBUG("FETCH MORE: " << parent << ":" << iname << data.name);

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
    m_watchers = value.toStringList();
    //qDebug() << "LOAD WATCHERS: " << m_watchers;
    reinitializeWatchersHelper();
}

void WatchHandler::saveWatchers()
{
    //qDebug() << "SAVE WATCHERS: " << m_watchers;
    setSessionValueRequested("Watchers", m_watchers);
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
