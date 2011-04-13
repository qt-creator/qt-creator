/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#include "analyzeroutputpane.h"
#include "analyzermanager.h"
#include "ianalyzertool.h"

#include <utils/qtcassert.h>
#include <utils/styledbar.h>

#include <QtCore/QVariant>
#include <QtCore/QDebug>

#include <QtGui/QWidget>
#include <QtGui/QStackedLayout>
#include <QtGui/QLabel>
#include <QtGui/QStackedWidget>

static const char dummyWidgetPropertyC[] = "dummyWidget";

enum { debug = 0 };
enum { dummyIndex = 0 };

namespace Analyzer {
namespace Internal {

static inline QWidget *createDummyWidget()
{
    QWidget *widget = new QWidget;
    widget->setProperty(dummyWidgetPropertyC, QVariant(true));
    return widget;
}

/*!
  \class AnalyzerPane::Internal::AnalyzerOutputPane

  \brief Common analysis output pane managing several tools.

  Output pane for all tools derived from IAnalyzerTool.
  The IAnalyzerOutputPaneAdapter (unless 0) provides
  \list
  \i Pane widget
  \i Optional toolbar widget
  \endlist

  Both are inserted into a pane stacked layout and a stacked toolbar widget respectively.

  The indexes of the stacked widgets/layouts and the adapter list go in sync
  (dummy widgets on the toolbar are used to achieve this).
  Dummy widgets that are shown  in case there is no tool with an output pane
  are added at index 0 to the stacks (usage of index 0 is to avoid using
  QStackedWidget::insert() when adding adapters, which causes flicker).

  Besides the tool-specific toolbar widget, the start/stop buttons and the combo
  box of the AnalyzerManager are shown in the toolbar.

  The initialization is a bit tricky here, as the sequence of calls to
  setTool(), outputWindow()/toolBarWidgets() is basically undefined. The pane widget
  should be created on the correct parent when outputWindow()
  is called, tools will typically be added before.

  \sa AnalyzerPane::Internal::IAnalyzerOutputPaneAdapter
*/

AnalyzerOutputPane::AnalyzerOutputPane(QObject *parent) :
    Core::IOutputPane(parent),
    m_paneWidget(0),
    m_paneStackedLayout(0),
    m_toolbarStackedWidget(0),
    m_toolBarSeparator(0)
{
    setObjectName(QLatin1String("AnalyzerOutputPane"));
}

void AnalyzerOutputPane::clearTool()
{
    // No tool. Show dummy label, which is the last widget.
    if (m_paneWidget) {
        m_paneStackedLayout->setCurrentIndex(dummyIndex);
        m_toolbarStackedWidget->setCurrentIndex(dummyIndex);
        emit navigateStateChanged();
    }
    hide();
}

int AnalyzerOutputPane::currentIndex() const
{
    return m_paneStackedLayout ? m_paneStackedLayout->currentIndex() : -1;
}

IAnalyzerOutputPaneAdapter *AnalyzerOutputPane::currentAdapter() const
{
    const int index = currentIndex(); // Rule out leading dummy widget
    if (index != dummyIndex && index < m_adapters.size())
        return m_adapters.at(index);
    return 0;
}

void AnalyzerOutputPane::setCurrentIndex(int i)
{
    QTC_ASSERT(isInitialized(), return )

    if (i != currentIndex()) {
        // Show up pane widget and optional toolbar widget. Hide
        // the toolbar if the toolbar widget is a dummy.
        m_paneStackedLayout->setCurrentIndex(i);
        m_toolbarStackedWidget->setCurrentIndex(i);
        const bool hasToolBarWidget = !m_toolbarStackedWidget->currentWidget()->property(dummyWidgetPropertyC).toBool();
        m_toolbarStackedWidget->setVisible(hasToolBarWidget);
        m_toolBarSeparator->setVisible(hasToolBarWidget);
        navigateStateChanged();
    }
}

void AnalyzerOutputPane::add(IAnalyzerOutputPaneAdapter *adapter)
{
    if (m_adapters.isEmpty())
        m_adapters.push_back(0); // Index for leading dummy widgets.
    m_adapters.push_back(adapter);
    connect(adapter, SIGNAL(navigationStatusChanged()), this, SLOT(slotNavigationStatusChanged()));
    connect(adapter, SIGNAL(popup(bool)), this, SLOT(slotPopup(bool)));
    if (isInitialized())
        addToWidgets(adapter);
}

void AnalyzerOutputPane::addToWidgets(IAnalyzerOutputPaneAdapter *adapter)
{
    QTC_ASSERT(m_paneWidget, return; )
    QWidget *toolPaneWidget =  adapter->paneWidget();
    QTC_ASSERT(toolPaneWidget, return; )
    m_paneStackedLayout->addWidget(toolPaneWidget);
    QWidget *toolBarWidget = adapter->toolBarWidget(); // Might be 0
    m_toolbarStackedWidget->addWidget(toolBarWidget ? toolBarWidget : createDummyWidget());
}

void AnalyzerOutputPane::setTool(IAnalyzerTool *t)
{
    if (debug)
        qDebug() << "AnalyzerOutputPane::setTool" << t;
    // No tool. show dummy label.
    IAnalyzerOutputPaneAdapter *adapter = t ? t->outputPaneAdapter() :
                                              static_cast<IAnalyzerOutputPaneAdapter *>(0);
    // Re-show or add.
    if (adapter) {
        int index = m_adapters.indexOf(adapter);
        if (index == -1) {
            index = m_adapters.size();
            add(adapter);
        }
        if (isInitialized()) {
            popup(false);
            setCurrentIndex(index);
        }
    } else {
        clearTool();
    }
}

QWidget * AnalyzerOutputPane::outputWidget(QWidget *parent)
{
    if (debug)
        qDebug() << "AnalyzerOutputPane::outputWidget";
    // Delayed creation of main pane widget. Add a trailing dummy widget
    // and add all adapters.
    if (!isInitialized())
        createWidgets(parent);
    return m_paneWidget;
}

void AnalyzerOutputPane::createWidgets(QWidget *paneParent)
{
    // Create pane and toolbar stack with leading dummy widget.
    m_paneWidget = new QWidget(paneParent);
    m_paneStackedLayout = new QStackedLayout(m_paneWidget);
    m_paneWidget->setObjectName(objectName() + QLatin1String("Widget"));
    m_paneStackedLayout->addWidget(new QLabel(tr("No current analysis tool"))); // placeholder

    // Temporarily assign to (wrong) parent to suppress flicker in conjunction with QStackedWidget.
    m_toolbarStackedWidget = new QStackedWidget(paneParent);
    m_toolbarStackedWidget->setObjectName(objectName() + QLatin1String("ToolBarStackedWidget"));
    m_toolbarStackedWidget->addWidget(createDummyWidget()); // placeholder
    m_toolBarSeparator = new Utils::StyledSeparator(paneParent);
    m_toolBarSeparator->setObjectName(objectName() + QLatin1String("ToolBarSeparator"));

    // Add adapters added before.
    const int adapterCount = m_adapters.size();
    const int firstAdapter = dummyIndex + 1;
    for (int i = firstAdapter; i < adapterCount; i++)
        addToWidgets(m_adapters.at(i));
    // Make last one current
    if (adapterCount > firstAdapter)
        setCurrentIndex(adapterCount - 1);
}

QWidgetList AnalyzerOutputPane::toolBarWidgets() const
{
    if (debug)
        qDebug() << "AnalyzerOutputPane::toolBarWidget";
    QTC_ASSERT(isInitialized(), return QWidgetList(); )

    QWidgetList list;
    list << m_toolBarSeparator << m_toolbarStackedWidget;
    AnalyzerManager::instance()->addOutputPaneToolBarWidgets(&list);
    return list;
}

QString AnalyzerOutputPane::displayName() const
{
    return tr("Analysis");
}

int AnalyzerOutputPane::priorityInStatusBar() const
{
    return -1; // Not visible in status bar.
}

void AnalyzerOutputPane::clearContents()
{
    if (IAnalyzerOutputPaneAdapter *adapter = currentAdapter())
        adapter->clearContents();
}

void AnalyzerOutputPane::visibilityChanged(bool v)
{
    Q_UNUSED(v)
}

void AnalyzerOutputPane::setFocus()
{
    if (IAnalyzerOutputPaneAdapter *adapter = currentAdapter())
        adapter->setFocus();
}

bool AnalyzerOutputPane::hasFocus()
{
    const IAnalyzerOutputPaneAdapter *adapter = currentAdapter();
    return adapter ? adapter->hasFocus() : false;
}

bool AnalyzerOutputPane::canFocus()
{
    const IAnalyzerOutputPaneAdapter *adapter = currentAdapter();
    return adapter ? adapter->canFocus() : false;
}

bool AnalyzerOutputPane::canNavigate()
{
    const IAnalyzerOutputPaneAdapter *adapter = currentAdapter();
    return adapter ? adapter->canNavigate() : false;
}

bool AnalyzerOutputPane::canNext()
{
    const IAnalyzerOutputPaneAdapter *adapter = currentAdapter();
    return adapter ? adapter->canNext() : false;
}

bool AnalyzerOutputPane::canPrevious()
{
    const IAnalyzerOutputPaneAdapter *adapter = currentAdapter();
    return adapter ? adapter->canPrevious() : false;
}

void AnalyzerOutputPane::goToNext()
{
    if (IAnalyzerOutputPaneAdapter *adapter = currentAdapter())
        adapter->goToNext();
}

void AnalyzerOutputPane::goToPrev()
{
    if (IAnalyzerOutputPaneAdapter *adapter = currentAdapter())
        adapter->goToPrev();
}

void AnalyzerOutputPane::slotPopup(bool withFocus)
{
    popup(withFocus);
}

void AnalyzerOutputPane::slotNavigationStatusChanged()
{
    IAnalyzerOutputPaneAdapter *adapter = qobject_cast<IAnalyzerOutputPaneAdapter *>(sender());
    const int index = m_adapters.indexOf(adapter);
    QTC_ASSERT(adapter != 0 && index != -1, return; )
    // Forward navigation status if it is the current pane.
    if (index == currentIndex())
        navigateStateChanged();
}

} // namespace Internal
} // namespace Analyzer
