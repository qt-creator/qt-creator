/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Nicolas Arnaud-Cormos, KDAB (nicolas.arnaud-cormos@kdab.com)
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

#include "ianalyzertool.h"
#include "analyzeroutputpane.h"

#include <utils/qtcassert.h>

#include <QtGui/QAbstractItemView>
#include <QtGui/QItemSelectionModel>
#include <QtCore/QAbstractItemModel>
#include <QtCore/QModelIndex>

namespace Analyzer {

IAnalyzerTool::IAnalyzerTool(QObject *parent) :
    QObject(parent)
{
}

/*!
    \class Analyzer::IAnalyzerOutputPaneAdapter

    \brief Adapter for handling multiple tools in the common 'Analysis' output pane.

    Provides the tool-specific output pane widget and optionally, a widget to be
    inserted into into the toolbar. Ownership of them is taken by the output pane.
    Forwards navigation calls issued by the output pane.
*/

IAnalyzerOutputPaneAdapter::IAnalyzerOutputPaneAdapter(QObject *parent) :
    QObject(parent)
{
}

IAnalyzerOutputPaneAdapter::~IAnalyzerOutputPaneAdapter()
{
}

/*!
    \class Analyzer::ListItemViewOutputPaneAdapter

    \brief Utility class implementing wrap-around navigation for flat lists.

    Provides an optional mechanism to pop up automatically in case errors show up.
*/

ListItemViewOutputPaneAdapter::ListItemViewOutputPaneAdapter(QObject *parent) :
    IAnalyzerOutputPaneAdapter(parent), m_listView(0), m_showOnRowsInserted(true)
{
}

void ListItemViewOutputPaneAdapter::connectNavigationSignals(QAbstractItemModel *model)
{
    connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)),
            this, SIGNAL(navigationStatusChanged()));
    connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)),
            this, SLOT(slotRowsInserted()));
    connect(model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
            this, SIGNAL(navigationStatusChanged()));
    connect(model, SIGNAL(modelReset()),
            this, SIGNAL(navigationStatusChanged()));
}

void ListItemViewOutputPaneAdapter::slotRowsInserted()
{
    if (m_showOnRowsInserted && !m_listView->isVisible())
        emit popup(true);
}

QWidget *ListItemViewOutputPaneAdapter::paneWidget()
{
    if (!m_listView) {
        m_listView = createItemView();
        if (QAbstractItemModel *model = m_listView->model())
            connectNavigationSignals(model);
    }
    return m_listView;
}

void ListItemViewOutputPaneAdapter::setFocus()
{
    if (m_listView)
        m_listView->setFocus();
}

bool ListItemViewOutputPaneAdapter::hasFocus() const
{
    return m_listView ? m_listView->hasFocus() : false;
}

bool ListItemViewOutputPaneAdapter::canFocus() const
{
    return true;
}

bool ListItemViewOutputPaneAdapter::canNavigate() const
{
    return true;
}

bool ListItemViewOutputPaneAdapter::canNext() const
{
    return rowCount() > 0;
}

bool ListItemViewOutputPaneAdapter::canPrevious() const
{
    return rowCount() > 0;
}

void ListItemViewOutputPaneAdapter::goToNext()
{
    setCurrentRow((currentRow() + 1) % rowCount());
}

void ListItemViewOutputPaneAdapter::goToPrev()
{
    const int prevRow = currentRow() - 1;
    setCurrentRow(prevRow >= 0 ? prevRow : rowCount() - 1);
}

bool ListItemViewOutputPaneAdapter::showOnRowsInserted() const
{
    return m_showOnRowsInserted;
}

void ListItemViewOutputPaneAdapter::setShowOnRowsInserted(bool v)
{
    m_showOnRowsInserted = v;
}

int ListItemViewOutputPaneAdapter::currentRow() const
{
    if (m_listView) {
        const QModelIndex index = m_listView->selectionModel()->currentIndex();
        if (index.isValid())
            return index.row();
    }
    return -1;
}

void ListItemViewOutputPaneAdapter::setCurrentRow(int r)
{
    QTC_ASSERT(m_listView, return; )
    const QModelIndex index = m_listView->model()->index(r, 0);
    m_listView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect|QItemSelectionModel::Current);
    m_listView->scrollTo(index);
}

int ListItemViewOutputPaneAdapter::rowCount() const
{
    return m_listView ? m_listView->model()->rowCount() : 0;
}

// -------------IAnalyzerTool
QString IAnalyzerTool::modeString()
{
    switch (mode()) {
        case IAnalyzerTool::DebugMode:
            return tr("Debug");
        case IAnalyzerTool::ReleaseMode:
            return tr("Release");
        case IAnalyzerTool::AnyMode:
            break;
    }
    return QString();
}

IAnalyzerOutputPaneAdapter *IAnalyzerTool::outputPaneAdapter()
{
    return 0;
}

} // namespace Analyzer
