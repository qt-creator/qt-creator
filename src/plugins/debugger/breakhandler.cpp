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

#include "breakhandler.h"

#include "imports.h" // TextEditor::BaseTextMark

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QFileInfo>

using namespace Debugger;
using namespace Debugger::Internal;


//////////////////////////////////////////////////////////////////
//
// BreakpointMarker
//
//////////////////////////////////////////////////////////////////

namespace Debugger {
namespace Internal {


// The red blob on the left side in the cpp editor.
class BreakpointMarker : public TextEditor::BaseTextMark
{
    Q_OBJECT
public:
    BreakpointMarker(BreakpointData *data, const QString &fileName, int lineNumber)
      : BaseTextMark(fileName, lineNumber), m_data(data), m_pending(true)
    {
        //qDebug() << "CREATE MARKER " << fileName << lineNumber;
    }

    ~BreakpointMarker()
    {
        //qDebug() << "REMOVE MARKER ";
        m_data = 0;
    }

    QIcon icon() const
    {
        static const QIcon icon(":/gdbdebugger/images/breakpoint.svg");
        static const QIcon icon2(":/gdbdebugger/images/breakpoint_pending.svg");
        return m_pending ? icon2 : icon;
    }

    void setPending(bool pending)
    {
        if (pending == m_pending)
            return;
        m_pending = pending;
        updateMarker();
    }

    void updateBlock(const QTextBlock &)
    {
        //qDebug() << "BREAKPOINT MARKER UPDATE BLOCK";
    }

    void removedFromEditor()
    {
        if (!m_data)
            return;

        BreakHandler *handler = m_data->handler();
        handler->removeBreakpoint(handler->indexOf(m_data));
        handler->saveBreakpoints();
        handler->updateMarkers();
    }

    void updateLineNumber(int lineNumber)
    {
        if (!m_data)
            return;
        //if (m_data->markerLineNumber == lineNumber)
        //    return;
        if (m_data->markerLineNumber != lineNumber) {
            m_data->markerLineNumber = lineNumber;
            // FIXME: should we tell gdb about the change? 
            // Ignore it for now, as we would require re-compilation
            // and debugger re-start anyway.
            if (0 && !m_data->bpLineNumber.isEmpty()) {
                if (!m_data->bpNumber.trimmed().isEmpty()) {
                    m_data->pending = true;
                }
            }
        }
        m_data->lineNumber = QString::number(lineNumber);
        m_data->handler()->updateMarkers();
    }

private:
    BreakpointData *m_data;
    bool m_pending;
};

} // namespace Internal
} // namespace Debugger



//////////////////////////////////////////////////////////////////
//
// BreakpointData
//
//////////////////////////////////////////////////////////////////

BreakpointData::BreakpointData(BreakHandler *handler)
{
    //qDebug() << "CREATE BREAKPOINTDATA" << this;
    m_handler = handler;
    pending = true;
    marker = 0;
    markerLineNumber = 0;
    bpMultiple = false;
}

BreakpointData::~BreakpointData()
{
    removeMarker();
    //qDebug() << "DESTROY BREAKPOINTDATA" << this;
}

void BreakpointData::removeMarker()
{
    BreakpointMarker *m = marker;
    marker = 0;
    delete m;
}

void BreakpointData::updateMarker()
{
    if (marker && (markerFileName != marker->fileName()
            || markerLineNumber != marker->lineNumber()))
        removeMarker();

    if (!marker && !markerFileName.isEmpty() && markerLineNumber > 0)
        marker = new BreakpointMarker(this, markerFileName, markerLineNumber);

    if (marker)
        marker->setPending(pending);
}

QString BreakpointData::toToolTip() const
{
    QString str;
    str += "<table>";
    str += "<tr><td>Marker File:</td><td>" + markerFileName + "</td></tr>";
    str += "<tr><td>Marker Line:</td><td>" + QString::number(markerLineNumber) + "</td></tr>";
    str += "<tr><td>BP Number:</td><td>" + bpNumber + "</td></tr>";
    str += "<tr><td>BP Address:</td><td>" + bpAddress + "</td></tr>";
    str += "<tr><td>----------</td><td></td><td></td></tr>";
    str += "<tr><td>Property:</td><td>Wanted:</td><td>Actual:</td></tr>";
    str += "<tr><td></td><td></td><td></td></tr>";
    str += "<tr><td>Internal Number:</td><td>-</td><td>" + bpNumber + "</td></tr>";
    str += "<tr><td>File Name:</td><td>" + fileName + "</td><td>" + bpFileName + "</td></tr>";
    str += "<tr><td>Function Name:</td><td>" + funcName + "</td><td>" + bpFuncName + "</td></tr>";
    str += "<tr><td>Line Number:</td><td>" + lineNumber + "</td><td>" + bpLineNumber + "</td></tr>";
    str += "<tr><td>Condition:</td><td>" + condition + "</td><td>" + bpCondition + "</td></tr>";
    str += "<tr><td>Ignore count:</td><td>" + ignoreCount + "</td><td>" + bpIgnoreCount + "</td></tr>";
    str += "</table>";
    return str;
}

bool BreakpointData::isLocatedAt(const QString &fileName_, int lineNumber_) const
{
    /*
    if (lineNumber != QString::number(lineNumber_))
        return false;
    if (fileName == fileName_)
        return true;
    if (fileName_.endsWith(fileName))
        return true;
    return false;
    */
    return lineNumber_ == markerLineNumber && fileName_ == markerFileName;
}

bool BreakpointData::conditionsMatch() const
{
    // same versions of gdb "beautify" the passed condition
    QString s1 = condition;
    s1.remove(QChar(' '));
    QString s2 = bpCondition;
    s2.remove(QChar(' '));
    return s1 == s2;
}


//////////////////////////////////////////////////////////////////
//
// BreakHandler
//
//////////////////////////////////////////////////////////////////

BreakHandler::BreakHandler(QObject *parent)
    : QAbstractItemModel(parent)
{
}

BreakHandler::~BreakHandler()
{
    clear();
}

int BreakHandler::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 6;
}

int BreakHandler::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : size();
}

void BreakHandler::removeAt(int index)
{
    BreakpointData *data = at(index);
    m_bp.removeAt(index);
    delete data;
}

void BreakHandler::clear()
{
    for (int index = size(); --index >= 0; )
        removeAt(index);
}

int BreakHandler::findBreakpoint(const BreakpointData &needle)
{
    // looks for a breakpoint we might refer to
    for (int index = 0; index != size(); ++index) {
        const BreakpointData *data = at(index);
        // clear hit.
        if (data->bpNumber == needle.bpNumber)
            return index;
        // at least at a position we were looking for
        // FIXME: breaks multiple breakpoints at the same location
        if (data->fileName == needle.bpFileName
                && data->lineNumber == needle.bpLineNumber)
            return index;
    }
    return -1;
}

int BreakHandler::findBreakpoint(int bpNumber)
{
    for (int index = 0; index != size(); ++index)
        if (at(index)->bpNumber == QString::number(bpNumber))
            return index;
    return -1;
}

void BreakHandler::saveBreakpoints()
{
    QList<QVariant> list;
    for (int index = 0; index != size(); ++index) {
        const BreakpointData *data = at(index);
        QMap<QString, QVariant> map;
        if (!data->fileName.isEmpty())
            map["filename"] = data->fileName;
        if (!data->lineNumber.isEmpty())
            map["linenumber"] = data->lineNumber;
        if (!data->funcName.isEmpty())
            map["funcname"] = data->funcName;
        if (!data->condition.isEmpty())
            map["condition"] = data->condition;
        if (!data->ignoreCount.isEmpty())
            map["ignorecount"] = data->ignoreCount;
        list.append(map);
    }
    setSessionValueRequested("Breakpoints", list);
}

void BreakHandler::loadBreakpoints()
{
    QVariant value;
    sessionValueRequested("Breakpoints", &value);
    QList<QVariant> list = value.toList();

    clear();
    foreach (const QVariant &var, list) {
        const QMap<QString, QVariant> map = var.toMap();
        BreakpointData *data = new BreakpointData(this);
        data->fileName = map["filename"].toString();
        data->lineNumber = map["linenumber"].toString();
        data->condition = map["condition"].toString();
        data->ignoreCount = map["ignorecount"].toString();
        data->funcName = map["funcname"].toString();
        data->markerFileName = data->fileName;
        data->markerLineNumber = data->lineNumber.toInt();
        append(data);
    }
}

void BreakHandler::resetBreakpoints()
{
    for (int index = size(); --index >= 0;) {
        BreakpointData *data = at(index);
        data->pending = true;
        data->bpNumber.clear();
        data->bpFuncName.clear();
        data->bpFileName.clear();
        data->bpLineNumber.clear();
        data->bpCondition.clear();
        data->bpIgnoreCount.clear();
        // keep marker data if it was primary
        if (data->markerFileName != data->fileName)
            data->markerFileName.clear();
        if (data->markerLineNumber != data->lineNumber.toInt())
            data->markerLineNumber = 0;
    }
}

void BreakHandler::updateMarkers()
{
    for (int index = 0; index != size(); ++index)
        at(index)->updateMarker();
    emit layoutChanged();
}

QVariant BreakHandler::headerData(int section,
    Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        static QString headers[] = {
            tr("Number"),  tr("Function"), tr("File"), tr("Line"),
            tr("Condition"), tr("Ignore")
        };
        return headers[section];
    }
    return QVariant();
}

QVariant BreakHandler::data(const QModelIndex &mi, int role) const
{
    static const QIcon icon(":/gdbdebugger/images/breakpoint.svg");
    static const QIcon icon2(":/gdbdebugger/images/breakpoint_pending.svg");
    static const QString empty = QString(QLatin1Char('-'));

    QTC_ASSERT(mi.isValid(), return QVariant());

    if (mi.row() >= size())
        return QVariant();

    const BreakpointData *data = at(mi.row());
    switch (mi.column()) {
        case 0:
            if (role == Qt::DisplayRole) {
                QString str = data->bpNumber;
                return str.isEmpty() ? empty : str;
            }
            if (role == Qt::DecorationRole)
                return data->pending ? icon2 : icon;
            break;
        case 1:
            if (role == Qt::DisplayRole) {
                QString str = data->pending ? data->funcName : data->bpFuncName;
                return str.isEmpty() ? empty : str;
            }
            break;
        case 2:
            if (role == Qt::DisplayRole) {
                QString str = data->pending ? data->fileName : data->bpFileName;
                str = QFileInfo(str).fileName();
                //if (data->bpMultiple && str.isEmpty() && !data->markerFileName.isEmpty())
                //    str = data->markerFileName;
                return str.isEmpty() ? empty : str;
            }
            break;
        case 3:
            if (role == Qt::DisplayRole) {
                QString str = data->pending ? data->lineNumber : data->bpLineNumber;
                //if (data->bpMultiple && str.isEmpty() && !data->markerFileName.isEmpty())
                //    str = data->markerLineNumber;
                return str.isEmpty() ? empty : str;
            }
            break;
        case 4:
            if (role == Qt::DisplayRole)
                return data->pending ? data->condition : data->bpCondition;
            if (role == Qt::ToolTipRole)
                return tr("Breakpoint will only be hit if this condition is met.");
            break;
        case 5:
            if (role == Qt::DisplayRole)
                return data->pending ? data->ignoreCount : data->bpIgnoreCount;
            if (role == Qt::ToolTipRole)
                return tr("Breakpoint will only be hit after being ignored so many times.");
            break;
    }
    if (role == Qt::ToolTipRole)
        return data->toToolTip();
    return QVariant();
}

bool BreakHandler::setData(const QModelIndex &mi, const QVariant &value, int role)
{
    if (role != Qt::EditRole)
        return false;

    BreakpointData *data = at(mi.row());
    switch (mi.column()) {
        case 4: {
            QString val = value.toString();
            if (val != data->condition) {
                data->condition = val;
                dataChanged(mi, mi);
            }
            return true;
        }
        case 5: {
            QString val = value.toString();
            if (val != data->ignoreCount) {
                data->ignoreCount = val;
                dataChanged(mi, mi);
            }
            return true;
        }
        default: { 
            return false;
        }
    }
}

QList<BreakpointData *> BreakHandler::takeRemovedBreakpoints()
{
    QList<BreakpointData *> result = m_removed;
    m_removed.clear();
    return result;
}

void BreakHandler::removeBreakpointHelper(int index)
{
    BreakpointData *data = m_bp.at(index);
    m_bp.removeAt(index);
    data->removeMarker();
    m_removed.append(data);
}


void BreakHandler::removeBreakpoint(int index)
{
    if (index < 0 || index >= size())
        return;
    BreakHandler::removeBreakpointHelper(index);
    emit layoutChanged();
    saveBreakpoints();
}


int BreakHandler::indexOf(const QString &fileName, int lineNumber)
{
    for (int index = 0; index != size(); ++index)
        if (at(index)->isLocatedAt(fileName, lineNumber))
            return index;
    return -1;
}

void BreakHandler::setBreakpoint(const QString &fileName, int lineNumber)
{
    QFileInfo fi(fileName);

    BreakpointData *data = new BreakpointData(this);
    data->fileName = fileName;
    data->lineNumber = QString::number(lineNumber);
    data->pending = true;
    data->markerFileName = fileName;
    data->markerLineNumber = lineNumber;
    append(data);
    emit layoutChanged();
    saveBreakpoints();
    updateMarkers();
}

void BreakHandler::removeAllBreakpoints()
{
    for (int index = size(); --index >= 0;)
        removeBreakpointHelper(index);
    emit layoutChanged();
    saveBreakpoints();
    updateMarkers();
}

void BreakHandler::setAllPending()
{
    loadBreakpoints();
    for (int index = size(); --index >= 0;)
        at(index)->pending = true;
    saveBreakpoints();
    updateMarkers();
}

void BreakHandler::saveSessionData()
{
    saveBreakpoints();
    updateMarkers();
}

void BreakHandler::loadSessionData()
{
    //resetBreakpoints();
    loadBreakpoints();
    updateMarkers();
}

void BreakHandler::activateBreakPoint(int index)
{
    const BreakpointData *data = at(index);
    //qDebug() << "BREAKPOINT ACTIVATED: " << data->fileName;
    if (!data->markerFileName.isEmpty())
        emit gotoLocation(data->markerFileName, data->markerLineNumber, false);
}

void BreakHandler::breakByFunction(const QString &functionName)
{
    // One per function is enough for now
    for (int index = size(); --index >= 0;) {
        const BreakpointData *data = at(index);
        QTC_ASSERT(data, break);
        if (data->funcName == functionName && data->condition.isEmpty()
                && data->ignoreCount.isEmpty())
            return;
    }
    BreakpointData *data = new BreakpointData(this);
    data->funcName = functionName;
    append(data);
    saveBreakpoints();
    updateMarkers();
}

#include "breakhandler.moc"
