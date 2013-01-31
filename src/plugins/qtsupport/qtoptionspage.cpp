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

#include "qtoptionspage.h"
#include "ui_showbuildlog.h"
#include "ui_qtversionmanager.h"
#include "ui_qtversioninfo.h"
#include "ui_debugginghelper.h"
#include "qtsupportconstants.h"
#include "qtversionmanager.h"
#include "qtversionfactory.h"
#include "qmldumptool.h"
#include "qmldebugginglibrary.h"
#include "qmlobservertool.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <utils/treewidgetcolumnstretcher.h>
#include <utils/qtcassert.h>
#include <utils/buildablehelperlibrary.h>
#include <utils/pathchooser.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/hostosinfo.h>
#include <utils/runextensions.h>

#include <QDir>
#include <QToolTip>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextBrowser>
#include <QDesktopServices>

enum ModelRoles { VersionIdRole = Qt::UserRole, ToolChainIdRole, BuildLogRole, BuildRunningRole};

using namespace QtSupport;
using namespace QtSupport::Internal;
using namespace Utils;

///
// QtOptionsPage
///

QtOptionsPage::QtOptionsPage()
    : m_widget(0)
{
    setId(Constants::QTVERSION_SETTINGS_PAGE_ID);
    setDisplayName(QCoreApplication::translate("Qt4ProjectManager", Constants::QTVERSION_SETTINGS_PAGE_NAME));
    setCategory(ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("ProjectExplorer",
        ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_TR_CATEGORY));
    setCategoryIcon(QLatin1String(ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_CATEGORY_ICON));
}

QWidget *QtOptionsPage::createPage(QWidget *parent)
{
    m_widget = new QtOptionsPageWidget(parent);
    if (m_searchKeywords.isEmpty())
        m_searchKeywords = m_widget->searchKeywords();
    return m_widget;
}

void QtOptionsPage::apply()
{
    if (!m_widget) // page was never shown
        return;
    m_widget->finish();

    m_widget->apply();
}

bool QtOptionsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

//-----------------------------------------------------


QtOptionsPageWidget::QtOptionsPageWidget(QWidget *parent)
    : QWidget(parent)
    , m_specifyNameString(tr("<specify a name>"))
    , m_ui(new Internal::Ui::QtVersionManager())
    , m_versionUi(new Internal::Ui::QtVersionInfo())
    , m_debuggingHelperUi(new Internal::Ui::DebuggingHelper())
    , m_infoBrowser(new QTextBrowser)
    , m_invalidVersionIcon(QLatin1String(":/projectexplorer/images/compile_error.png"))
    , m_warningVersionIcon(QLatin1String(":/projectexplorer/images/compile_warning.png"))
    , m_configurationWidget(0)
    , m_autoItem(0)
    , m_manualItem(0)
{
    QWidget *versionInfoWidget = new QWidget();
    m_versionUi->setupUi(versionInfoWidget);
    m_versionUi->editPathPushButton->setText(QCoreApplication::translate("Utils::PathChooser", Utils::PathChooser::browseButtonLabel));

    QWidget *debuggingHelperDetailsWidget = new QWidget();
    m_debuggingHelperUi->setupUi(debuggingHelperDetailsWidget);

    m_ui->setupUi(this);

    m_infoBrowser->setOpenLinks(false);
    m_infoBrowser->setTextInteractionFlags(Qt::TextBrowserInteraction);
    connect(m_infoBrowser, SIGNAL(anchorClicked(QUrl)), this, SLOT(infoAnchorClicked(QUrl)));
    m_ui->infoWidget->setWidget(m_infoBrowser);
    connect(m_ui->infoWidget, SIGNAL(expanded(bool)),
            this, SLOT(setInfoWidgetVisibility()));

    m_ui->versionInfoWidget->setWidget(versionInfoWidget);
    m_ui->versionInfoWidget->setState(Utils::DetailsWidget::NoSummary);

    m_ui->debuggingHelperWidget->setWidget(debuggingHelperDetailsWidget);
    connect(m_ui->debuggingHelperWidget, SIGNAL(expanded(bool)),
            this, SLOT(setInfoWidgetVisibility()));

    // setup parent items for auto-detected and manual versions
    m_ui->qtdirList->header()->setResizeMode(QHeaderView::ResizeToContents);
    m_ui->qtdirList->header()->setStretchLastSection(false);
    m_ui->qtdirList->setTextElideMode(Qt::ElideNone);
    m_autoItem = new QTreeWidgetItem(m_ui->qtdirList);
    m_autoItem->setText(0, tr("Auto-detected"));
    m_autoItem->setFirstColumnSpanned(true);
    m_autoItem->setFlags(Qt::ItemIsEnabled);
    m_manualItem = new QTreeWidgetItem(m_ui->qtdirList);
    m_manualItem->setText(0, tr("Manual"));
    m_manualItem->setFirstColumnSpanned(true);
    m_manualItem->setFlags(Qt::ItemIsEnabled);

    QList<int> additions;
    QList<BaseQtVersion *> versions = QtVersionManager::instance()->versions();
    foreach (BaseQtVersion *v, versions)
        additions.append(v->uniqueId());

    updateQtVersions(additions, QList<int>(), QList<int>());

    m_ui->qtdirList->expandAll();

    connect(m_versionUi->nameEdit, SIGNAL(textEdited(QString)),
            this, SLOT(updateCurrentQtName()));

    connect(m_versionUi->editPathPushButton, SIGNAL(clicked()),
            this, SLOT(editPath()));

    connect(m_ui->addButton, SIGNAL(clicked()),
            this, SLOT(addQtDir()));
    connect(m_ui->delButton, SIGNAL(clicked()),
            this, SLOT(removeQtDir()));

    connect(m_ui->qtdirList, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
            this, SLOT(versionChanged(QTreeWidgetItem*,QTreeWidgetItem*)));

    connect(m_debuggingHelperUi->rebuildButton, SIGNAL(clicked()),
            this, SLOT(buildDebuggingHelper()));
    connect(m_debuggingHelperUi->gdbHelperBuildButton, SIGNAL(clicked()),
            this, SLOT(buildGdbHelper()));
    connect(m_debuggingHelperUi->qmlDumpBuildButton, SIGNAL(clicked()),
            this, SLOT(buildQmlDump()));
    connect(m_debuggingHelperUi->qmlDebuggingLibBuildButton, SIGNAL(clicked()),
            this, SLOT(buildQmlDebuggingLibrary()));
    connect(m_debuggingHelperUi->qmlObserverBuildButton, SIGNAL(clicked()),
            this, SLOT(buildQmlObserver()));

    connect(m_debuggingHelperUi->showLogButton, SIGNAL(clicked()),
            this, SLOT(slotShowDebuggingBuildLog()));
    connect(m_debuggingHelperUi->toolChainComboBox, SIGNAL(activated(int)),
            this, SLOT(selectedToolChainChanged(int)));

    connect(m_ui->cleanUpButton, SIGNAL(clicked()), this, SLOT(cleanUpQtVersions()));
    userChangedCurrentVersion();
    updateCleanUpButton();

    connect(QtVersionManager::instance(), SIGNAL(dumpUpdatedFor(Utils::FileName)),
            this, SLOT(qtVersionsDumpUpdated(Utils::FileName)));

    connect(QtVersionManager::instance(), SIGNAL(qtVersionsChanged(QList<int>,QList<int>,QList<int>)),
            this, SLOT(updateQtVersions(QList<int>,QList<int>,QList<int>)));

    connect(ProjectExplorer::ToolChainManager::instance(), SIGNAL(toolChainsChanged()),
            this, SLOT(toolChainsUpdated()));
}

int QtOptionsPageWidget::currentIndex() const
{
    if (QTreeWidgetItem *currentItem = m_ui->qtdirList->currentItem())
        return indexForTreeItem(currentItem);
    return -1;
}

BaseQtVersion *QtOptionsPageWidget::currentVersion() const
{
    const int currentItemIndex = currentIndex();
    if (currentItemIndex >= 0 && currentItemIndex < m_versions.size())
        return m_versions.at(currentItemIndex);
    return 0;
}

static inline int findVersionById(const QList<BaseQtVersion *> &l, int id)
{
    const int size = l.size();
    for (int i = 0; i < size; i++)
        if (l.at(i)->uniqueId() == id)
            return i;
    return -1;
}

// Update with results of terminated helper build
void QtOptionsPageWidget::debuggingHelperBuildFinished(int qtVersionId, const QString &output, DebuggingHelperBuildTask::Tools tools)
{
    const int index = findVersionById(m_versions, qtVersionId);
    if (index == -1)
        return; // Oops, somebody managed to delete the version

    BaseQtVersion *version = m_versions.at(index);

    // Update item view
    QTreeWidgetItem *item = treeItemForIndex(index);
    QTC_ASSERT(item, return);
    DebuggingHelperBuildTask::Tools buildFlags
            = item->data(0, BuildRunningRole).value<DebuggingHelperBuildTask::Tools>();
    buildFlags &= ~tools;
    item->setData(0, BuildRunningRole,  QVariant::fromValue(buildFlags));
    item->setData(0, BuildLogRole, output);

    bool success = true;
    if (tools & DebuggingHelperBuildTask::GdbDebugging)
        success &= version->hasGdbDebuggingHelper();
    if (tools & DebuggingHelperBuildTask::QmlDebugging)
        success &= version->hasQmlDebuggingLibrary();
    if (tools & DebuggingHelperBuildTask::QmlDump)
        success &= version->hasQmlDump();
    if (tools & DebuggingHelperBuildTask::QmlObserver)
        success &= version->hasQmlObserver();

    if (!success)
        showDebuggingBuildLog(item);

    updateDebuggingHelperUi();
}

void QtOptionsPageWidget::cleanUpQtVersions()
{
    QStringList toRemove;
    foreach (const BaseQtVersion *v, m_versions) {
        if (!v->isValid() && !v->isAutodetected())
            toRemove.append(v->displayName());
    }

    if (toRemove.isEmpty())
        return;

    if (QMessageBox::warning(0, tr("Remove Invalid Qt Versions"),
                             tr("Do you want to remove all invalid Qt Versions?<br>"
                                "<ul><li>%1</li></ul><br>"
                                "will be removed.").arg(toRemove.join(QLatin1String("</li><li>"))),
                             QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
        return;

    for (int i = m_versions.count() - 1; i >= 0; --i) {
        if (!m_versions.at(i)->isValid()) {
            QTreeWidgetItem *item = treeItemForIndex(i);
            delete item;

            delete m_versions.at(i);
            m_versions.removeAt(i);
        }
    }
    updateCleanUpButton();
}

void QtOptionsPageWidget::toolChainsUpdated()
{
    for (int i = 0; i < m_versions.count(); ++i) {
        QTreeWidgetItem *item = treeItemForIndex(i);
        if (item == m_ui->qtdirList->currentItem()) {
            updateDescriptionLabel();
            updateDebuggingHelperUi();
        } else {
            const ValidityInfo info = validInformation(m_versions.at(i));
            item->setIcon(0, info.icon);
        }
    }
}

void QtOptionsPageWidget::selectedToolChainChanged(int comboIndex)
{
    const int index = currentIndex();
    if (index < 0)
        return;

    QTreeWidgetItem *item = treeItemForIndex(index);
    QTC_ASSERT(item, return);

    QString toolChainId = m_debuggingHelperUi->toolChainComboBox->itemData(comboIndex).toString();

    item->setData(0, ToolChainIdRole, toolChainId);
}

void QtOptionsPageWidget::qtVersionsDumpUpdated(const Utils::FileName &qmakeCommand)
{
    foreach (BaseQtVersion *version, m_versions) {
        if (version->qmakeCommand() == qmakeCommand)
            version->recheckDumper();
    }
    if (currentVersion()
            && currentVersion()->qmakeCommand() == qmakeCommand) {
        updateWidgets();
        updateDescriptionLabel();
        updateDebuggingHelperUi();
    }
}

void QtOptionsPageWidget::setInfoWidgetVisibility()
{
    m_ui->versionInfoWidget->setVisible((m_ui->infoWidget->state() == DetailsWidget::Collapsed)
                                        && (m_ui->debuggingHelperWidget->state() == DetailsWidget::Collapsed));
    m_ui->infoWidget->setVisible(m_ui->debuggingHelperWidget->state() == DetailsWidget::Collapsed);
    m_ui->debuggingHelperWidget->setVisible(m_ui->infoWidget->state() == DetailsWidget::Collapsed);
}

void QtOptionsPageWidget::infoAnchorClicked(const QUrl &url)
{
    QDesktopServices::openUrl(url);
}

QtOptionsPageWidget::ValidityInfo QtOptionsPageWidget::validInformation(const BaseQtVersion *version)
{
    ValidityInfo info;
    info.icon = m_validVersionIcon;

    if (!version)
        return info;

    info.description = tr("Qt version %1 for %2").arg(version->qtVersionString(), version->description());
    if (!version->isValid()) {
        info.icon = m_invalidVersionIcon;
        info.message = version->invalidReason();
        return info;
    }

    // Do we have tool chain issues?
    QStringList missingToolChains;
    int abiCount = 0;
    foreach (const ProjectExplorer::Abi &a, version->qtAbis()) {
        if (ProjectExplorer::ToolChainManager::instance()->findToolChains(a).isEmpty())
            missingToolChains.append(a.toString());
        ++abiCount;
    }

    bool useable = true;
    QStringList warnings;
    if (!missingToolChains.isEmpty()) {
        if (missingToolChains.count() == abiCount) {
            // Yes, this Qt version can't be used at all!
            info.message = tr("No compiler can produce code for this Qt version. Please define one or more compilers.");
            info.icon = m_invalidVersionIcon;
            useable = false;
        } else {
            // Yes, some ABIs are unsupported
            warnings << tr("Not all possible target environments can be supported due to missing compilers.");
            info.toolTip = tr("The following ABIs are currently not supported:<ul><li>%1</li></ul>")
                    .arg(missingToolChains.join(QLatin1String("</li><li>")));
            info.icon = m_warningVersionIcon;
        }
    }

    if (useable) {
        warnings += version->warningReason();
        if (!warnings.isEmpty()) {
            info.message = warnings.join(QLatin1String("\n"));
            info.icon = m_warningVersionIcon;
        }
    }

    return info;
}

QList<ProjectExplorer::ToolChain*> QtOptionsPageWidget::toolChains(const BaseQtVersion *version)
{
    QHash<QString,ProjectExplorer::ToolChain*> toolChains;
    if (!version)
        return toolChains.values();

    foreach (const ProjectExplorer::Abi &a, version->qtAbis()) {
        foreach (ProjectExplorer::ToolChain *tc,
                 ProjectExplorer::ToolChainManager::instance()->findToolChains(a)) {
            toolChains.insert(tc->id(), tc);
        }
    }

    return toolChains.values();
}

QString QtOptionsPageWidget::defaultToolChainId(const BaseQtVersion *version)
{
    QList<ProjectExplorer::ToolChain*> possibleToolChains = toolChains(version);
    if (!possibleToolChains.isEmpty())
        return possibleToolChains.first()->id();
    return QString();
}

void QtOptionsPageWidget::buildDebuggingHelper(DebuggingHelperBuildTask::Tools tools)
{
    const int index = currentIndex();
    if (index < 0)
        return;

    // remove tools that cannot be build
    tools &= DebuggingHelperBuildTask::availableTools(currentVersion());

    QTreeWidgetItem *item = treeItemForIndex(index);
    QTC_ASSERT(item, return);

    DebuggingHelperBuildTask::Tools buildFlags
            = item->data(0, BuildRunningRole).value<DebuggingHelperBuildTask::Tools>();
    buildFlags |= tools;
    item->setData(0, BuildRunningRole, QVariant::fromValue(buildFlags));

    BaseQtVersion *version = m_versions.at(index);
    if (!version)
        return;

    updateDebuggingHelperUi();

    // Run a debugging helper build task in the background.
    QString toolChainId = m_debuggingHelperUi->toolChainComboBox->itemData(
                m_debuggingHelperUi->toolChainComboBox->currentIndex()).toString();
    ProjectExplorer::ToolChainManager *tcMgr = ProjectExplorer::ToolChainManager::instance();
    ProjectExplorer::ToolChain *toolChain = tcMgr->findToolChain(toolChainId);
    if (!toolChain)
        return;

    DebuggingHelperBuildTask *buildTask = new DebuggingHelperBuildTask(version, toolChain, tools);
    // Don't open General Messages pane with errors
    buildTask->showOutputOnError(false);
    connect(buildTask, SIGNAL(finished(int,QString,DebuggingHelperBuildTask::Tools)),
            this, SLOT(debuggingHelperBuildFinished(int,QString,DebuggingHelperBuildTask::Tools)),
            Qt::QueuedConnection);
    QFuture<void> task = QtConcurrent::run(&DebuggingHelperBuildTask::run, buildTask);
    const QString taskName = tr("Building helpers");

    Core::ICore::progressManager()->addTask(task, taskName,
                                                        QLatin1String("Qt4ProjectManager::BuildHelpers"));
}
void QtOptionsPageWidget::buildGdbHelper()
{
    buildDebuggingHelper(DebuggingHelperBuildTask::GdbDebugging);
}

void QtOptionsPageWidget::buildQmlDump()
{
    buildDebuggingHelper(DebuggingHelperBuildTask::QmlDump);
}

void QtOptionsPageWidget::buildQmlDebuggingLibrary()
{
    buildDebuggingHelper(DebuggingHelperBuildTask::QmlDebugging);
}

void QtOptionsPageWidget::buildQmlObserver()
{
    DebuggingHelperBuildTask::Tools qmlDbgTools =
            DebuggingHelperBuildTask::QmlObserver;
    qmlDbgTools |= DebuggingHelperBuildTask::QmlDebugging;
    buildDebuggingHelper(qmlDbgTools);
}

// Non-modal dialog
class BuildLogDialog : public QDialog {
public:
    explicit BuildLogDialog(QWidget *parent = 0);
    void setText(const QString &text);

private:
    Ui_ShowBuildLog m_ui;
};

BuildLogDialog::BuildLogDialog(QWidget *parent) : QDialog(parent)
{
    m_ui.setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose, true);
}

void BuildLogDialog::setText(const QString &text)
{
    m_ui.log->setPlainText(text); // Show and scroll to bottom
    m_ui.log->moveCursor(QTextCursor::End);
    m_ui.log->ensureCursorVisible();
}

void QtOptionsPageWidget::slotShowDebuggingBuildLog()
{
    if (const QTreeWidgetItem *currentItem = m_ui->qtdirList->currentItem())
        showDebuggingBuildLog(currentItem);
}

void QtOptionsPageWidget::showDebuggingBuildLog(const QTreeWidgetItem *currentItem)
{
    const int currentItemIndex = indexForTreeItem(currentItem);
    if (currentItemIndex < 0)
        return;
    BuildLogDialog *dialog = new BuildLogDialog(this->window());
    dialog->setWindowTitle(tr("Debugging Helper Build Log for '%1'").arg(currentItem->text(0)));
    dialog->setText(currentItem->data(0, BuildLogRole).toString());
    dialog->show();
}

void QtOptionsPageWidget::updateQtVersions(const QList<int> &additions, const QList<int> &removals,
                                           const QList<int> &changes)
{
    QtVersionManager *vm = QtVersionManager::instance();

    QList<QTreeWidgetItem *> toRemove;
    QList<int> toAdd = additions;

    // Generate list of all existing items:
    QList<QTreeWidgetItem *> itemList;
    for (int i = 0; i < m_autoItem->childCount(); ++i)
        itemList.append(m_autoItem->child(i));
    for (int i = 0; i < m_manualItem->childCount(); ++i)
        itemList.append(m_manualItem->child(i));

    // Find existing items to remove/change:
    foreach (QTreeWidgetItem *item, itemList) {
        int id = item->data(0, VersionIdRole).toInt();
        if (removals.contains(id)) {
            toRemove.append(item);
            continue;
        }

        if (changes.contains(id)) {
            toAdd.append(id);
            toRemove.append(item);
            continue;
        }
    }

    // Remove changed/removed items:
    foreach (QTreeWidgetItem *item, toRemove) {
        int index = indexForTreeItem(item);
        delete m_versions.at(index);
        m_versions.removeAt(index);
        delete item;
    }

    // Add changed/added items:
    foreach (int a, toAdd) {
        BaseQtVersion *version = vm->version(a)->clone();
        m_versions.append(version);
        QTreeWidgetItem *item = new QTreeWidgetItem;

        item->setText(0, version->displayName());
        item->setText(1, version->qmakeCommand().toUserOutput());
        item->setData(0, VersionIdRole, version->uniqueId());
        item->setData(0, ToolChainIdRole, defaultToolChainId(version));
        const ValidityInfo info = validInformation(version);
        item->setIcon(0, info.icon);

        // Insert in the right place:
        QTreeWidgetItem *parent = version->isAutodetected()? m_autoItem : m_manualItem;
        for (int i = 0; i < parent->childCount(); ++i) {
            BaseQtVersion *currentVersion = m_versions.at(indexForTreeItem(parent->child(i)));
            if (currentVersion->qtVersion() > version->qtVersion())
                continue;
            parent->insertChild(i, item);
            parent = 0;
            break;
        }

        if (parent)
            parent->addChild(item);
    }
}

QtOptionsPageWidget::~QtOptionsPageWidget()
{
    delete m_ui;
    delete m_versionUi;
    delete m_debuggingHelperUi;
    delete m_configurationWidget;
    qDeleteAll(m_versions);
}

static QString filterForQmakeFileDialog()
{
    QString filter = QLatin1String("qmake (");
    const QStringList commands = Utils::BuildableHelperLibrary::possibleQMakeCommands();
    for (int i = 0; i < commands.size(); ++i) {
        if (i)
            filter += QLatin1Char(' ');
        if (Utils::HostOsInfo::isMacHost())
            // work around QTBUG-7739 that prohibits filters that don't start with *
            filter += QLatin1Char('*');
        filter += commands.at(i);
        if (Utils::HostOsInfo::isAnyUnixHost() && !Utils::HostOsInfo::isMacHost())
            // kde bug, we need at least one wildcard character
            // see QTCREATORBUG-7771
            filter += QLatin1Char('*');
    }
    filter += QLatin1Char(')');
    return filter;
}

void QtOptionsPageWidget::addQtDir()
{
    Utils::FileName qtVersion = Utils::FileName::fromString(
                QFileDialog::getOpenFileName(this,
                                             tr("Select a qmake Executable"),
                                             QString(),
                                             filterForQmakeFileDialog(),
                                             0,
                                             QFileDialog::DontResolveSymlinks));
    if (qtVersion.isNull())
        return;
    BaseQtVersion *version = 0;
    foreach (BaseQtVersion *v, m_versions) {
        if (v->qmakeCommand() == qtVersion) {
            version = v;
            break;
        }
    }
    if (version) {
        // Already exist
        QMessageBox::warning(this, tr("Qt Version Already Known"),
                             tr("This Qt version was already registered as \"%1\".")
                             .arg(version->displayName()));
        return;
    }

    QString error;
    version = QtVersionFactory::createQtVersionFromQMakePath(qtVersion, false, QString(), &error);
    if (version) {
        m_versions.append(version);

        QTreeWidgetItem *item = new QTreeWidgetItem(m_ui->qtdirList->topLevelItem(1));
        item->setText(0, version->displayName());
        item->setText(1, version->qmakeCommand().toUserOutput());
        item->setData(0, VersionIdRole, version->uniqueId());
        item->setData(0, ToolChainIdRole, defaultToolChainId(version));
        item->setIcon(0, version->isValid()? m_validVersionIcon : m_invalidVersionIcon);
        m_ui->qtdirList->setCurrentItem(item); // should update the rest of the ui
        m_versionUi->nameEdit->setFocus();
        m_versionUi->nameEdit->selectAll();
    } else {
        QMessageBox::warning(this, tr("Qmake not Executable"),
                             tr("The qmake executable %1 could not be added: %2").arg(qtVersion.toUserOutput()).arg(error));
        return;
    }
    updateCleanUpButton();
}

void QtOptionsPageWidget::removeQtDir()
{
    QTreeWidgetItem *item = m_ui->qtdirList->currentItem();
    int index = indexForTreeItem(item);
    if (index < 0)
        return;

    delete item;

    BaseQtVersion *version = m_versions.at(index);
    m_versions.removeAt(index);
    delete version;
    updateCleanUpButton();
}

void QtOptionsPageWidget::editPath()
{
    BaseQtVersion *current = currentVersion();
    QString dir = currentVersion()->qmakeCommand().toFileInfo().absolutePath();
    Utils::FileName qtVersion = Utils::FileName::fromString(
                QFileDialog::getOpenFileName(this,
                                             tr("Select a qmake executable"),
                                             dir,
                                             filterForQmakeFileDialog(),
                                             0,
                                             QFileDialog::DontResolveSymlinks));
    if (qtVersion.isNull())
        return;
    BaseQtVersion *version = QtVersionFactory::createQtVersionFromQMakePath(qtVersion);
    if (!version)
        return;
    // Same type? then replace!
    if (current->type() != version->type()) {
        // not the same type, error out
        QMessageBox::critical(this, tr("Incompatible Qt Versions"),
                              tr("The Qt version selected must match the device type."),
                              QMessageBox::Ok);
        delete version;
        return;
    }
    // same type, replace
    version->setId(current->uniqueId());
    if (current->displayName() != current->defaultDisplayName(current->qtVersionString(), current->qmakeCommand()))
        version->setDisplayName(current->displayName());
    m_versions.replace(m_versions.indexOf(current), version);
    delete current;

    // Update ui
    userChangedCurrentVersion();
    QTreeWidgetItem *item = m_ui->qtdirList->currentItem();
    item->setText(0, version->displayName());
    item->setText(1, version->qmakeCommand().toUserOutput());
    item->setData(0, VersionIdRole, version->uniqueId());
    item->setData(0, ToolChainIdRole, defaultToolChainId(version));
    item->setIcon(0, version->isValid()? m_validVersionIcon : m_invalidVersionIcon);
}

void QtOptionsPageWidget::updateDebuggingHelperUi()
{
    BaseQtVersion *version = currentVersion();
    const QTreeWidgetItem *currentItem = m_ui->qtdirList->currentItem();

    QList<ProjectExplorer::ToolChain*> toolchains = toolChains(currentVersion());

    if (!version || !version->isValid() || toolchains.isEmpty()) {
        m_ui->debuggingHelperWidget->setVisible(false);
    } else {
        const DebuggingHelperBuildTask::Tools availableTools = DebuggingHelperBuildTask::availableTools(version);
        const bool canBuildGdbHelper = availableTools & DebuggingHelperBuildTask::GdbDebugging;
        const bool canBuildQmlDumper = availableTools & DebuggingHelperBuildTask::QmlDump;
        const bool canBuildQmlDebuggingLib = availableTools & DebuggingHelperBuildTask::QmlDebugging;
        const bool canBuildQmlObserver = availableTools & DebuggingHelperBuildTask::QmlObserver;

        const bool hasGdbHelper = !version->gdbDebuggingHelperLibrary().isEmpty();
        const bool hasQmlDumper = version->hasQmlDump();
        const bool needsQmlDumper = version->needsQmlDump();
        const bool hasQmlDebuggingLib = version->hasQmlDebuggingLibrary();
        const bool needsQmlDebuggingLib = version->needsQmlDebuggingLibrary();
        const bool hasQmlObserver = !version->qmlObserverTool().isEmpty();

        bool isBuildingGdbHelper = false;
        bool isBuildingQmlDumper = false;
        bool isBuildingQmlDebuggingLib = false;
        bool isBuildingQmlObserver = false;

        if (currentItem) {
            DebuggingHelperBuildTask::Tools buildingTools
                    = currentItem->data(0, BuildRunningRole).value<DebuggingHelperBuildTask::Tools>();
            isBuildingGdbHelper = buildingTools & DebuggingHelperBuildTask::GdbDebugging;
            isBuildingQmlDumper = buildingTools & DebuggingHelperBuildTask::QmlDump;
            isBuildingQmlDebuggingLib = buildingTools & DebuggingHelperBuildTask::QmlDebugging;
            isBuildingQmlObserver = buildingTools & DebuggingHelperBuildTask::QmlObserver;
        }

        // get names of tools from labels
        QStringList helperNames;
        const QChar colon = QLatin1Char(':');
        if (hasGdbHelper)
            helperNames << m_debuggingHelperUi->gdbHelperLabel->text().remove(colon);
        if (hasQmlDumper)
            helperNames << m_debuggingHelperUi->qmlDumpLabel->text().remove(colon);
        if (hasQmlDebuggingLib)
            helperNames << m_debuggingHelperUi->qmlDebuggingLibLabel->text().remove(colon);
        if (hasQmlObserver)
            helperNames << m_debuggingHelperUi->qmlObserverLabel->text().remove(colon);

        QString status;
        if (helperNames.isEmpty()) {
            status = tr("Helpers: None available");
        } else {
            //: %1 is list of tool names.
            status = tr("Helpers: %1.").arg(helperNames.join(QLatin1String(", ")));
        }

        m_ui->debuggingHelperWidget->setSummaryText(status);

        QString gdbHelperText;
        Qt::TextInteractionFlags gdbHelperTextFlags = Qt::NoTextInteraction;
        if (hasGdbHelper) {
            gdbHelperText = QDir::toNativeSeparators(version->gdbDebuggingHelperLibrary());
            gdbHelperTextFlags = Qt::TextSelectableByMouse;
        } else {
            if (canBuildGdbHelper)
                gdbHelperText =  tr("<i>Not yet built.</i>");
            else
                gdbHelperText =  tr("<i>Not needed.</i>");
        }
        m_debuggingHelperUi->gdbHelperStatus->setText(gdbHelperText);
        m_debuggingHelperUi->gdbHelperStatus->setTextInteractionFlags(gdbHelperTextFlags);
        m_debuggingHelperUi->gdbHelperBuildButton->setEnabled(canBuildGdbHelper && !isBuildingGdbHelper);

        QString qmlDumpStatusText, qmlDumpStatusToolTip;
        Qt::TextInteractionFlags qmlDumpStatusTextFlags = Qt::NoTextInteraction;
        if (hasQmlDumper) {
            qmlDumpStatusText = QDir::toNativeSeparators(version->qmlDumpTool(false));
            const QString debugQmlDumpPath = QDir::toNativeSeparators(version->qmlDumpTool(true));
            if (qmlDumpStatusText != debugQmlDumpPath) {
                if (!qmlDumpStatusText.isEmpty()
                        && !debugQmlDumpPath.isEmpty())
                    qmlDumpStatusText += QLatin1String("\n");
                qmlDumpStatusText += debugQmlDumpPath;
            }
            qmlDumpStatusTextFlags = Qt::TextSelectableByMouse;
        } else {
            if (!needsQmlDumper) {
                qmlDumpStatusText = tr("<i>Not needed.</i>");
            } else if (canBuildQmlDumper) {
                qmlDumpStatusText = tr("<i>Not yet built.</i>");
            } else {
                qmlDumpStatusText = tr("<i>Cannot be compiled.</i>");
                QmlDumpTool::canBuild(version, &qmlDumpStatusToolTip);
            }
        }
        m_debuggingHelperUi->qmlDumpStatus->setText(qmlDumpStatusText);
        m_debuggingHelperUi->qmlDumpStatus->setTextInteractionFlags(qmlDumpStatusTextFlags);
        m_debuggingHelperUi->qmlDumpStatus->setToolTip(qmlDumpStatusToolTip);
        m_debuggingHelperUi->qmlDumpBuildButton->setEnabled(canBuildQmlDumper & !isBuildingQmlDumper);

        QString qmlDebuggingLibStatusText, qmlDebuggingLibToolTip;
        Qt::TextInteractionFlags qmlDebuggingLibStatusTextFlags = Qt::NoTextInteraction;
        if (hasQmlDebuggingLib) {
            qmlDebuggingLibStatusText = QDir::toNativeSeparators(
                        version->qmlDebuggingHelperLibrary(false));
            const QString debugPath = QDir::toNativeSeparators(
                        version->qmlDebuggingHelperLibrary(true));

            if (qmlDebuggingLibStatusText != debugPath) {
                if (!qmlDebuggingLibStatusText.isEmpty()
                        && !debugPath.isEmpty()) {
                    qmlDebuggingLibStatusText += QLatin1Char('\n');
                }
                qmlDebuggingLibStatusText += debugPath;
            }
            qmlDebuggingLibStatusTextFlags = Qt::TextSelectableByMouse;
        } else {
            if (!needsQmlDebuggingLib) {
                qmlDebuggingLibStatusText = tr("<i>Not needed.</i>");
            } else if (canBuildQmlDebuggingLib) {
                qmlDebuggingLibStatusText = tr("<i>Not yet built.</i>");
            } else {
                qmlDebuggingLibStatusText = tr("<i>Cannot be compiled.</i>");
                QmlDebuggingLibrary::canBuild(version, &qmlDebuggingLibToolTip);
            }
        }
        m_debuggingHelperUi->qmlDebuggingLibStatus->setText(qmlDebuggingLibStatusText);
        m_debuggingHelperUi->qmlDebuggingLibStatus->setTextInteractionFlags(qmlDebuggingLibStatusTextFlags);
        m_debuggingHelperUi->qmlDebuggingLibStatus->setToolTip(qmlDebuggingLibToolTip);
        m_debuggingHelperUi->qmlDebuggingLibBuildButton->setEnabled(needsQmlDebuggingLib
                                                                    && canBuildQmlDebuggingLib
                                                                    && !isBuildingQmlDebuggingLib);

        QString qmlObserverStatusText, qmlObserverToolTip;
        Qt::TextInteractionFlags qmlObserverStatusTextFlags = Qt::NoTextInteraction;
        if (hasQmlObserver) {
            qmlObserverStatusText = QDir::toNativeSeparators(version->qmlObserverTool());
            qmlObserverStatusTextFlags = Qt::TextSelectableByMouse;
        }  else {
            if (!needsQmlDebuggingLib) {
                qmlObserverStatusText = tr("<i>Not needed.</i>");
            } else if (canBuildQmlObserver) {
                qmlObserverStatusText = tr("<i>Not yet built.</i>");
            } else {
                qmlObserverStatusText = tr("<i>Cannot be compiled.</i>");
                QmlObserverTool::canBuild(version, &qmlObserverToolTip);
            }
        }
        m_debuggingHelperUi->qmlObserverStatus->setText(qmlObserverStatusText);
        m_debuggingHelperUi->qmlObserverStatus->setTextInteractionFlags(qmlObserverStatusTextFlags);
        m_debuggingHelperUi->qmlObserverStatus->setToolTip(qmlObserverToolTip);
        m_debuggingHelperUi->qmlObserverBuildButton->setEnabled(canBuildQmlObserver
                                                                & !isBuildingQmlObserver);

        QList<ProjectExplorer::ToolChain*> toolchains = toolChains(currentVersion());
        QString selectedToolChainId = currentItem->data(0, ToolChainIdRole).toString();
        m_debuggingHelperUi->toolChainComboBox->clear();
        for (int i = 0; i < toolchains.size(); ++i) {
            if (!toolchains.at(i)->isValid())
                continue;
            if (i >= m_debuggingHelperUi->toolChainComboBox->count()) {
                m_debuggingHelperUi->toolChainComboBox->insertItem(i, toolchains.at(i)->displayName(),
                                                                   toolchains.at(i)->id());
            }
            if (toolchains.at(i)->id() == selectedToolChainId)
                m_debuggingHelperUi->toolChainComboBox->setCurrentIndex(i);
        }

        const bool hasLog = currentItem && !currentItem->data(0, BuildLogRole).toString().isEmpty();
        m_debuggingHelperUi->showLogButton->setEnabled(hasLog);

        const bool canBuild = canBuildGdbHelper
                               || canBuildQmlDumper
                               || (canBuildQmlDebuggingLib && needsQmlDebuggingLib)
                               || canBuildQmlObserver;
        const bool isBuilding = isBuildingGdbHelper
                                 || isBuildingQmlDumper
                                 || isBuildingQmlDebuggingLib
                                 || isBuildingQmlObserver;

        m_debuggingHelperUi->rebuildButton->setEnabled(canBuild && !isBuilding);
        m_debuggingHelperUi->toolChainComboBox->setEnabled(canBuild && !isBuilding);
        setInfoWidgetVisibility();
    }
}

// To be called if a Qt version was removed or added
void QtOptionsPageWidget::updateCleanUpButton()
{
    bool hasInvalidVersion = false;
    for (int i = 0; i < m_versions.count(); ++i) {
        if (!m_versions.at(i)->isValid()) {
            hasInvalidVersion = true;
            break;
        }
    }
    m_ui->cleanUpButton->setEnabled(hasInvalidVersion);
}

void QtOptionsPageWidget::userChangedCurrentVersion()
{
    updateWidgets();
    updateDescriptionLabel();
    updateDebuggingHelperUi();
}

void QtOptionsPageWidget::qtVersionChanged()
{
    updateDescriptionLabel();
    updateDebuggingHelperUi();
}

void QtOptionsPageWidget::updateDescriptionLabel()
{
    QTreeWidgetItem *item = m_ui->qtdirList->currentItem();
    const BaseQtVersion *version = currentVersion();
    const ValidityInfo info = validInformation(version);
    if (info.message.isEmpty()) {
        m_versionUi->errorLabel->setVisible(false);
    } else {
        m_versionUi->errorLabel->setVisible(true);
        m_versionUi->errorLabel->setText(info.message);
        m_versionUi->errorLabel->setToolTip(info.toolTip);
    }
    m_ui->infoWidget->setSummaryText(info.description);
    if (item)
        item->setIcon(0, info.icon);

    if (version) {
        m_infoBrowser->setHtml(version->toHtml(true));
        setInfoWidgetVisibility();
    } else {
        m_infoBrowser->setHtml(QString());
        m_ui->versionInfoWidget->setVisible(false);
        m_ui->infoWidget->setVisible(false);
        m_ui->debuggingHelperWidget->setVisible(false);
    }
}

int QtOptionsPageWidget::indexForTreeItem(const QTreeWidgetItem *item) const
{
    if (!item || !item->parent())
        return -1;
    const int uniqueId = item->data(0, VersionIdRole).toInt();
    for (int index = 0; index < m_versions.size(); ++index) {
        if (m_versions.at(index)->uniqueId() == uniqueId)
            return index;
    }
    return -1;
}

QTreeWidgetItem *QtOptionsPageWidget::treeItemForIndex(int index) const
{
    const int uniqueId = m_versions.at(index)->uniqueId();
    for (int i = 0; i < m_ui->qtdirList->topLevelItemCount(); ++i) {
        QTreeWidgetItem *toplevelItem = m_ui->qtdirList->topLevelItem(i);
        for (int j = 0; j < toplevelItem->childCount(); ++j) {
            QTreeWidgetItem *item = toplevelItem->child(j);
            if (item->data(0, VersionIdRole).toInt() == uniqueId)
                return item;
        }
    }
    return 0;
}

void QtOptionsPageWidget::versionChanged(QTreeWidgetItem *newItem, QTreeWidgetItem *old)
{
    Q_UNUSED(newItem)
    if (old)
        fixQtVersionName(indexForTreeItem(old));
    userChangedCurrentVersion();
}

void QtOptionsPageWidget::updateWidgets()
{
    delete m_configurationWidget;
    m_configurationWidget = 0;
    BaseQtVersion *version = currentVersion();
    if (version) {
        m_versionUi->nameEdit->setText(version->displayName());
        m_versionUi->qmakePath->setText(version->qmakeCommand().toUserOutput());
        m_configurationWidget = version->createConfigurationWidget();
        if (m_configurationWidget) {
            m_versionUi->formLayout->addRow(m_configurationWidget);
            m_configurationWidget->setEnabled(!version->isAutodetected());
            connect(m_configurationWidget, SIGNAL(changed()),
                    this, SLOT(qtVersionChanged()));
        }
    } else {
        m_versionUi->nameEdit->clear();
        m_versionUi->qmakePath->setText(QString()); // clear()
    }

    const bool enabled = version != 0;
    const bool isAutodetected = enabled && version->isAutodetected();
    m_ui->delButton->setEnabled(enabled && !isAutodetected);
    m_versionUi->nameEdit->setEnabled(enabled && !isAutodetected);
    m_versionUi->editPathPushButton->setEnabled(enabled && !isAutodetected);
}

void QtOptionsPageWidget::updateCurrentQtName()
{
    QTreeWidgetItem *currentItem = m_ui->qtdirList->currentItem();
    Q_ASSERT(currentItem);
    int currentItemIndex = indexForTreeItem(currentItem);
    if (currentItemIndex < 0)
        return;
    m_versions[currentItemIndex]->setDisplayName(m_versionUi->nameEdit->text());
    currentItem->setText(0, m_versions[currentItemIndex]->displayName());
    updateDescriptionLabel();
}


void QtOptionsPageWidget::finish()
{
    if (QTreeWidgetItem *item = m_ui->qtdirList->currentItem())
        fixQtVersionName(indexForTreeItem(item));
}

void QtOptionsPageWidget::apply()
{
    disconnect(QtVersionManager::instance(), SIGNAL(qtVersionsChanged(QList<int>,QList<int>,QList<int>)),
            this, SLOT(updateQtVersions(QList<int>,QList<int>,QList<int>)));

    QtVersionManager *vm = QtVersionManager::instance();
    vm->setNewQtVersions(versions());

    connect(QtVersionManager::instance(), SIGNAL(qtVersionsChanged(QList<int>,QList<int>,QList<int>)),
            this, SLOT(updateQtVersions(QList<int>,QList<int>,QList<int>)));
}

/* Checks that the Qt version name is unique
 * and otherwise changes the name
 *
 */
void QtOptionsPageWidget::fixQtVersionName(int index)
{
    if (index < 0)
        return;
    int count = m_versions.count();
    QString name = m_versions.at(index)->displayName();
    if (name.isEmpty())
        return;
    for (int i = 0; i < count; ++i) {
        if (i != index) {
            if (m_versions.at(i)->displayName() == m_versions.at(index)->displayName()) {
                // Same name, find new name
                QRegExp regexp(QLatin1String("^(.*)\\((\\d)\\)$"));
                if (regexp.exactMatch(name)) {
                    // Already in Name (#) format
                    name = regexp.cap(1);
                    name += QLatin1Char('(');
                    name += QString::number(regexp.cap(2).toInt() + 1);
                    name += QLatin1Char(')');
                } else {
                    name +=  QLatin1String(" (2)");
                }
                // set new name
                m_versions[index]->setDisplayName(name);
                treeItemForIndex(index)->setText(0, name);

                // Now check again...
                fixQtVersionName(index);
            }
        }
    }
}

QList<BaseQtVersion *> QtOptionsPageWidget::versions() const
{
    QList<BaseQtVersion *> result;
    for (int i = 0; i < m_versions.count(); ++i)
        result.append(m_versions.at(i)->clone());
    return result;
}

QString QtOptionsPageWidget::searchKeywords() const
{
    QString rc;
    QLatin1Char sep(' ');
    QTextStream ts(&rc);
    ts << sep << m_versionUi->versionNameLabel->text()
       << sep << m_versionUi->pathLabel->text()
       << sep << m_debuggingHelperUi->gdbHelperLabel->text()
       << sep << m_debuggingHelperUi->qmlDumpLabel->text()
       << sep << m_debuggingHelperUi->qmlObserverLabel->text();

    rc.remove(QLatin1Char('&'));
    return rc;
}
