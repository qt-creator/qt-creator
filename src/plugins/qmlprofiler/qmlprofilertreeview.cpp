/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmlprofilertreeview.h"
#include <utils/itemviews.h>
#include <QCoreApplication>
#include <QHeaderView>

namespace QmlProfiler {
namespace Internal {

QmlProfilerTreeView::QmlProfilerTreeView(QWidget *parent)
    : Utils::TreeView(parent)
{
    setFrameStyle(QFrame::NoFrame);
    header()->setSectionResizeMode(QHeaderView::Interactive);
    header()->setDefaultSectionSize(100);
    header()->setMinimumSectionSize(50);
}


// Translate from "old" context to keep 2.8 string freeze
QString QmlProfilerTreeView::displayHeader(Fields header) const
{
    static const char ctxt1[] = "QmlProfiler::Internal::QmlProfilerEventsParentsAndChildrenView";
    static const char ctxt2[] = "QmlProfiler::Internal::QmlProfilerEventsMainView";

    switch (header) {
    case Callee:
        return QCoreApplication::translate(ctxt1, "Callee");
    case CalleeDescription:
        return QCoreApplication::translate(ctxt1, "Callee Description");
    case Caller:
        return QCoreApplication::translate(ctxt1, "Caller");
    case CallerDescription:
        return QCoreApplication::translate(ctxt1, "Caller Description");
    case CallCount:
        return QCoreApplication::translate(ctxt2, "Calls");
    case Details:
        return QCoreApplication::translate(ctxt2, "Details");
    case Location:
        return QCoreApplication::translate(ctxt2, "Location");
    case MaxTime:
        return QCoreApplication::translate(ctxt2, "Longest Time");
    case TimePerCall:
        return QCoreApplication::translate(ctxt2, "Mean Time");
    case SelfTime:
        return QCoreApplication::translate(ctxt2, "Self Time");
    case SelfTimeInPercent:
        return QCoreApplication::translate(ctxt2, "Self Time in Percent");
    case MinTime:
        return QCoreApplication::translate(ctxt2, "Shortest Time");
    case TimeInPercent:
        return QCoreApplication::translate(ctxt2, "Time in Percent");
    case TotalTime:
        return QCoreApplication::translate(ctxt2, "Total Time");
    case Type:
        return QCoreApplication::translate(ctxt2, "Type");
    case MedianTime:
        return QCoreApplication::translate(ctxt2, "Median Time");
    default:
        return QString();
    }
}

} // namespace Internal
} // namespace QmlProfiler
