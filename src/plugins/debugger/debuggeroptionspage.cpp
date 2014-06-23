/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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

#include <QFileInfo>
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

    m_versionLabel = new QLineEdit(this);
    m_versionLabel->setPlaceholderText(tr("Unknown"));
    m_versionLabel->setEnabled(false);

    m_abis = new QLineEdit(this);
    m_abis->setEnabled(false);

    QFormLayout *formLayout = new QFormLayout(this);
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    formLayout->addRow(new QLabel(tr("Name:")), m_displayNameLineEdit);
//    formLayout->addRow(new QLabel(tr("Type:")), m_engineTypeComboBox);
    formLayout->addRow(m_cdbLabel);
    formLayout->addRow(new QLabel(tr("Path:")), m_binaryChooser);
    formLayout->addRow(new QLabel(tr("ABIs:")), m_abis);
    formLayout->addRow(new QLabel(tr("Version:")), m_versionLabel);

    connect(m_binaryChooser, SIGNAL(changed(QString)), this, SLOT(binaryPathHasChanged()));
}

DebuggerItem DebuggerItemConfigWidget::item() const
{
    DebuggerItem item(m_id);
    if (m_id.isNull())
        return item;

    item.setDisplayName(m_displayNameLineEdit->text());
    item.setCommand(m_binaryChooser->fileName());
    item.setAutoDetected(m_autodetected);
    QList<ProjectExplorer::Abi> abiList;
    foreach (const QString &a, m_abis->text().split(QRegExp(QLatin1String("[^A-Za-z0-9-_]+")))) {
        if (a.isNull())
            continue;
        abiList << a;
    }
    item.setAbis(abiList);
    item.setVersion(m_versionLabel->text());
    item.setEngineType(m_engineType);
    return item;
}


void DebuggerItemConfigWidget::store() const
{
    DebuggerItem i = item();
    if (i.isValid())
        m_model->updateDebugger(i);
}

void DebuggerItemConfigWidget::setAbis(const QStringList &abiNames)
{
    m_abis->setText(abiNames.join(QLatin1String(", ")));
}

void DebuggerItemConfigWidget::handleCommandChange()
{
    // Use DebuggerItemManager as a cache:
    const DebuggerItem *existing
            = DebuggerItemManager::findByCommand(m_binaryChooser->fileName());
    if (existing) {
        setAbis(existing->abiNames());
        m_versionLabel->setText(existing->version());
        m_engineType = existing->engineType();
    } else {
        QFileInfo fi = QFileInfo(m_binaryChooser->path());
        if (fi.isExecutable()) {
            DebuggerItem tmp = item();
            tmp.reinitializeFromFile();
            setAbis(tmp.abiNames());
            m_versionLabel->setText(tmp.version());
            m_engineType = tmp.engineType();
        } else {
            setAbis(QStringList());
            m_versionLabel->setText(QString());
            m_engineType = NoEngineType;
        }
    }
    m_model->updateDebugger(item());
}

void DebuggerItemConfigWidget::setItem(const DebuggerItem &item)
{
    store(); // store away the (changed) settings for future use

    m_id = QVariant(); // reset Id to avoid intermediate signal handling

    // Set values:
    m_autodetected = item.isAutoDetected();

    m_displayNameLineEdit->setEnabled(!item.isAutoDetected());
    m_displayNameLineEdit->setText(item.displayName());

    m_binaryChooser->setReadOnly(item.isAutoDetected());
    m_binaryChooser->setFileName(item.command());

    QString text;
    QString versionCommand;
    if (item.engineType() == CdbEngineType) {
        const bool is64bit = is64BitWindowsSystem();
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
    m_versionLabel->setText(item.version());
    setAbis(item.abiNames());
    m_engineType = item.engineType();
    m_id = item.id();
}

void DebuggerItemConfigWidget::apply()
{
    DebuggerItem current = m_model->currentDebugger();
    if (!current.isValid())
        return; // Nothing was selected here.

    store();
    setItem(item());
}

void DebuggerItemConfigWidget::binaryPathHasChanged()
{
    // Ignore change if this is no valid DebuggerItem
    if (!m_id.isValid())
        return;

    handleCommandChange();
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

QWidget *DebuggerOptionsPage::widget()
{
    if (!m_configWidget) {
        m_configWidget = new QWidget;

        m_addButton = new QPushButton(tr("Add"), m_configWidget);
        m_cloneButton = new QPushButton(tr("Clone"), m_configWidget);
        m_delButton = new QPushButton(tr("Remove"), m_configWidget);

        m_container = new DetailsWidget(m_configWidget);
        m_container->setState(DetailsWidget::NoSummary);
        m_container->setVisible(false);

        m_debuggerView = new QTreeView(m_configWidget);
        m_model = new DebuggerItemModel(m_debuggerView);
        m_debuggerView->setModel(m_model);
        m_debuggerView->setUniformRowHeights(true);
        m_debuggerView->setSelectionMode(QAbstractItemView::SingleSelection);
        m_debuggerView->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_debuggerView->expandAll();

        QHeaderView *header = m_debuggerView->header();
        header->setStretchLastSection(false);
        header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        header->setSectionResizeMode(2, QHeaderView::Stretch);

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

        m_itemConfigWidget = new DebuggerItemConfigWidget(m_model);
        m_container->setWidget(m_itemConfigWidget);

        updateState();
    }
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
    newItem.createId();
    newItem.setAutoDetected(false);
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
    item.createId();
    item.setAutoDetected(false);
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
    delete m_configWidget;

    // Children of m_configWidget.
    m_model = 0;
    m_container = 0;
    m_debuggerView = 0;
    m_addButton = 0;
    m_cloneButton = 0;
    m_delButton = 0;
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

    DebuggerItem item = m_model->currentDebugger();

    bool canCopy = item.isValid() && item.canClone();
    bool canDelete = m_model->currentIndex().parent().isValid() && !item.isAutoDetected();

    m_cloneButton->setEnabled(canCopy);
    m_delButton->setEnabled(canDelete);
}

} // namespace Internal
} // namespace Debugger
