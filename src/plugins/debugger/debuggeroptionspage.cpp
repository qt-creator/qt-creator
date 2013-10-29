/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "debuggeroptionspage.h"

#include "debuggeritemmanager.h"
#include "debuggeritemmodel.h"

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/detailswidget.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/winutils.h>

#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QObject>
#include <QPushButton>
#include <QTreeView>
#include <QWidget>

using namespace Utils;

namespace Debugger {
namespace Internal {

static const char debuggingToolsWikiLinkC[] = "http://qt-project.org/wiki/Qt_Creator_Windows_Debugging";

// -----------------------------------------------------------------------
// DebuggerItemConfigWidget
// -----------------------------------------------------------------------

class DebuggerItemConfigWidget : public QWidget
{
    Q_DECLARE_TR_FUNCTIONS(Debugger::Internal::DebuggerItemConfigWidget)

public:
    explicit DebuggerItemConfigWidget(DebuggerItemModel *model);
    void setItem(const DebuggerItem &item);
    void apply();

private:
    QLineEdit *m_displayNameLineEdit;
    QLabel *m_cdbLabel;
    PathChooser *m_binaryChooser;
    QLineEdit *m_abis;
    DebuggerItemModel *m_model;
};

DebuggerItemConfigWidget::DebuggerItemConfigWidget(DebuggerItemModel *model) :
    m_model(model)
{
    QTC_CHECK(model);

    m_displayNameLineEdit = new QLineEdit(this);

    m_binaryChooser = new PathChooser(this);
    m_binaryChooser->setExpectedKind(PathChooser::ExistingCommand);
    m_binaryChooser->setMinimumWidth(400);
    m_binaryChooser->setHistoryCompleter(QLatin1String("DebuggerPaths"));

    m_cdbLabel = new QLabel(this);
    m_cdbLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_cdbLabel->setOpenExternalLinks(true);

    m_abis = new QLineEdit(this);
    m_abis->setEnabled(false);

    QFormLayout *formLayout = new QFormLayout(this);
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    formLayout->addRow(new QLabel(tr("Name:")), m_displayNameLineEdit);
//    formLayout->addRow(new QLabel(tr("Type:")), m_engineTypeComboBox);
    formLayout->addRow(m_cdbLabel);
    formLayout->addRow(new QLabel(tr("Path:")), m_binaryChooser);
    formLayout->addRow(new QLabel(tr("ABIs:")), m_abis);
}

void DebuggerItemConfigWidget::setItem(const DebuggerItem &item)
{
    m_displayNameLineEdit->setEnabled(!item.isAutoDetected());
    m_displayNameLineEdit->setText(item.displayName());

    m_binaryChooser->setEnabled(!item.isAutoDetected());
    m_binaryChooser->setFileName(item.command());

    QString text;
    QString versionCommand;
    if (item.engineType() == CdbEngineType) {
#ifdef Q_OS_WIN
        const bool is64bit = winIs64BitSystem();
#else
        const bool is64bit = false;
#endif
        const QString versionString = is64bit ? tr("64-bit version") : tr("32-bit version");
        //: Label text for path configuration. %2 is "x-bit version".
        text = tr("<html><body><p>Specify the path to the "
                  "<a href=\"%1\">Windows Console Debugger executable</a>"
                  " (%2) here.</p>""</body></html>").
                arg(QLatin1String(debuggingToolsWikiLinkC), versionString);
        versionCommand = QLatin1String("-version");
    } else {
        versionCommand = QLatin1String("--version");
    }

    m_cdbLabel->setText(text);
    m_cdbLabel->setVisible(!text.isEmpty());
    m_binaryChooser->setCommandVersionArguments(QStringList(versionCommand));

    m_abis->setText(item.abiNames().join(QLatin1String(", ")));
}

void DebuggerItemConfigWidget::apply()
{
    DebuggerItem item = m_model->currentDebugger();
    if (!item.isValid())
        return; // Nothing was selected here.

    item.setDisplayName(m_displayNameLineEdit->text());
    item.setCommand(m_binaryChooser->fileName());
    item.reinitializeFromFile();
    m_model->updateDebugger(item);
}

// --------------------------------------------------------------------------
// DebuggerOptionsPage
// --------------------------------------------------------------------------

DebuggerOptionsPage::DebuggerOptionsPage()
{
    m_model = 0;
    m_debuggerView = 0;
    m_container = 0;
    m_addButton = 0;
    m_cloneButton = 0;
    m_delButton = 0;

    setId(ProjectExplorer::Constants::DEBUGGER_SETTINGS_PAGE_ID);
    setDisplayName(tr("Debuggers"));
    setCategory(ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("ProjectExplorer",
        ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_TR_CATEGORY));
    setCategoryIcon(QLatin1String(ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_CATEGORY_ICON));
}

QWidget *DebuggerOptionsPage::createPage(QWidget *parent)
{
    m_configWidget = new QWidget(parent);

    m_addButton = new QPushButton(tr("Add"), m_configWidget);
    m_cloneButton = new QPushButton(tr("Clone"), m_configWidget);
    m_delButton = new QPushButton(tr("Remove"), m_configWidget);

    m_container = new DetailsWidget(m_configWidget);
    m_container->setState(DetailsWidget::NoSummary);
    m_container->setVisible(false);

    m_model = new DebuggerItemModel(parent);

    m_debuggerView = new QTreeView(m_configWidget);
    m_debuggerView->setModel(m_model);
    m_debuggerView->setUniformRowHeights(true);
    m_debuggerView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_debuggerView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_debuggerView->expandAll();

    QHeaderView *header = m_debuggerView->header();
    header->setStretchLastSection(false);
    header->setResizeMode(0, QHeaderView::ResizeToContents);
    header->setResizeMode(1, QHeaderView::ResizeToContents);
    header->setResizeMode(2, QHeaderView::Stretch);

    QVBoxLayout *buttonLayout = new QVBoxLayout();
    buttonLayout->setSpacing(6);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->addWidget(m_addButton);
    buttonLayout->addWidget(m_cloneButton);
    buttonLayout->addWidget(m_delButton);
    buttonLayout->addItem(new QSpacerItem(10, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

    QVBoxLayout *verticalLayout = new QVBoxLayout();
    verticalLayout->addWidget(m_debuggerView);
    verticalLayout->addWidget(m_container);

    QHBoxLayout *horizontalLayout = new QHBoxLayout(m_configWidget);
    horizontalLayout->addLayout(verticalLayout);
    horizontalLayout->addLayout(buttonLayout);

    connect(m_debuggerView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(debuggerSelectionChanged()));

    connect(m_addButton, SIGNAL(clicked()), this, SLOT(addDebugger()), Qt::QueuedConnection);
    connect(m_cloneButton, SIGNAL(clicked()), this, SLOT(cloneDebugger()), Qt::QueuedConnection);
    connect(m_delButton, SIGNAL(clicked()), this, SLOT(removeDebugger()), Qt::QueuedConnection);

    m_searchKeywords = tr("Debuggers");

    m_itemConfigWidget = new DebuggerItemConfigWidget(m_model);
    m_container->setWidget(m_itemConfigWidget);

    updateState();

    return m_configWidget;
}

void DebuggerOptionsPage::apply()
{
    m_itemConfigWidget->apply();
    m_model->apply();
}

void DebuggerOptionsPage::cloneDebugger()
{
    DebuggerItem item = m_model->currentDebugger();
    if (!item.isValid())
        return;

    DebuggerItem newItem;
    newItem.setCommand(item.command());
    newItem.setEngineType(item.engineType());
    newItem.setAbis(item.abis());
    newItem.setDisplayName(DebuggerItemManager::uniqueDisplayName(tr("Clone of %1").arg(item.displayName())));
    newItem.setAutoDetected(false);
    m_model->addDebugger(newItem);
    m_debuggerView->setCurrentIndex(m_model->lastIndex());
}

void DebuggerOptionsPage::addDebugger()
{
    DebuggerItem item;
    item.setEngineType(NoEngineType);
    item.setDisplayName(DebuggerItemManager::uniqueDisplayName(tr("New Debugger")));
    item.setAutoDetected(false);
    m_model->addDebugger(item);
    m_debuggerView->setCurrentIndex(m_model->lastIndex());
}

void DebuggerOptionsPage::removeDebugger()
{
    QVariant id = m_model->currentDebuggerId();
    m_model->removeDebugger(id);
    m_debuggerView->setCurrentIndex(m_model->lastIndex());
}

void DebuggerOptionsPage::finish()
{
    // Deleted by settingsdialog.
    m_configWidget = 0;

    // Children of m_configWidget.
    m_container = 0;
    m_debuggerView = 0;
    m_addButton = 0;
    m_cloneButton = 0;
    m_delButton = 0;
}

bool DebuggerOptionsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

void DebuggerOptionsPage::debuggerSelectionChanged()
{
    QTC_ASSERT(m_container, return);

    QModelIndex mi = m_debuggerView->currentIndex();
    mi = mi.sibling(mi.row(), 0);
    m_model->setCurrentIndex(mi);

    DebuggerItem item = m_model->currentDebugger();

    m_itemConfigWidget->setItem(item);
    m_container->setVisible(item.isValid());
    updateState();
}

void DebuggerOptionsPage::debuggerModelChanged()
{
    QTC_ASSERT(m_container, return);

    QVariant id = m_model->currentDebuggerId();
    const DebuggerItem *item = DebuggerItemManager::findById(id);
    if (!item)
        return;

    m_itemConfigWidget->setItem(*item);
    m_container->setVisible(m_model->currentDebuggerId().isValid());
    m_debuggerView->setCurrentIndex(m_model->currentIndex());
    updateState();
}

void DebuggerOptionsPage::updateState()
{
    if (!m_cloneButton)
        return;

    bool canCopy = false;
    bool canDelete = false;

    DebuggerItem item = m_model->currentDebugger();

    canCopy = item.isValid() && item.canClone();
    canDelete = !item.isAutoDetected();

    m_cloneButton->setEnabled(canCopy);
    m_delButton->setEnabled(canDelete);
}

} // namespace Internal
} // namespace Debugger
