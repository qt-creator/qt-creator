/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qtoptionspage.h"
#include "ui_showbuildlog.h"
#include "ui_qtversionmanager.h"
#include "ui_qtversioninfo.h"
#include "ui_debugginghelper.h"
#include "qt4projectmanagerconstants.h"
#include "qt4target.h"
#include "qtversionmanager.h"

#include <projectexplorer/abi.h>
#include <projectexplorer/debugginghelper.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <utils/detailsbutton.h>
#include <utils/treewidgetcolumnstretcher.h>
#include <utils/qtcassert.h>
#include <qtconcurrent/runextensions.h>

#include <QtCore/QFuture>
#include <QtCore/QtConcurrentRun>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QSet>
#include <QtCore/QTextStream>
#include <QtCore/QDateTime>
#include <QtGui/QHelpEvent>
#include <QtGui/QToolTip>
#include <QtGui/QMenu>

enum ModelRoles { VersionIdRole = Qt::UserRole, BuildLogRole, BuildRunningRole};

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

///
// QtOptionsPage
///

QtOptionsPage::QtOptionsPage()
    : m_widget(0)
{
}

QString QtOptionsPage::id() const
{
    return QLatin1String(Constants::QTVERSION_SETTINGS_PAGE_ID);
}

QString QtOptionsPage::displayName() const
{
    return QCoreApplication::translate("Qt4ProjectManager", Constants::QTVERSION_SETTINGS_PAGE_NAME);
}

QString QtOptionsPage::category() const
{
    return QLatin1String(Constants::QT_SETTINGS_CATEGORY);
}

QString QtOptionsPage::displayCategory() const
{
    return QCoreApplication::translate("Qt4ProjectManager", Constants::QT_SETTINGS_TR_CATEGORY);
}

QIcon QtOptionsPage::categoryIcon() const
{
    return QIcon(QLatin1String(Constants::QT_SETTINGS_CATEGORY_ICON));
}

QWidget *QtOptionsPage::createPage(QWidget *parent)
{
    QtVersionManager *vm = QtVersionManager::instance();
    m_widget = new QtOptionsPageWidget(parent, vm->versions());
    if (m_searchKeywords.isEmpty())
        m_searchKeywords = m_widget->searchKeywords();
    return m_widget;
}

void QtOptionsPage::apply()
{
    if (!m_widget) // page was never shown
        return;
    m_widget->finish();

    QtVersionManager *vm = QtVersionManager::instance();
    vm->setNewQtVersions(m_widget->versions());
}

bool QtOptionsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

//-----------------------------------------------------


QtOptionsPageWidget::QtOptionsPageWidget(QWidget *parent, QList<QtVersion *> versions)
    : QWidget(parent)
    , m_specifyNameString(tr("<specify a name>"))
    , m_specifyPathString(tr("<specify a qmake location>"))
    , m_ui(new Internal::Ui::QtVersionManager())
    , m_versionUi(new Internal::Ui::QtVersionInfo())
    , m_debuggingHelperUi(new Internal::Ui::DebuggingHelper())
{
    // Initialize m_versions
    foreach(QtVersion *version, versions)
        m_versions.push_back(new QtVersion(*version));

    QWidget *versionInfoWidget = new QWidget();
    m_versionUi->setupUi(versionInfoWidget);
    m_versionUi->qmakePath->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_versionUi->qmakePath->setPromptDialogTitle(tr("Select qmake Executable"));
    m_versionUi->s60SDKPath->setExpectedKind(Utils::PathChooser::ExistingDirectory);
    m_versionUi->s60SDKPath->setPromptDialogTitle(tr("Select S60 SDK Root"));

    QWidget *debuggingHelperDetailsWidget = new QWidget();
    m_debuggingHelperUi->setupUi(debuggingHelperDetailsWidget);

    m_ui->setupUi(this);

    m_ui->versionInfoWidget->setWidget(versionInfoWidget);
    m_ui->versionInfoWidget->setState(Utils::DetailsWidget::NoSummary);

    m_ui->debuggingHelperWidget->setWidget(debuggingHelperDetailsWidget);

    new Utils::TreeWidgetColumnStretcher(m_ui->qtdirList, 1);

    // setup parent items for auto-detected and manual versions
    m_ui->qtdirList->header()->setResizeMode(QHeaderView::ResizeToContents);
    QTreeWidgetItem *autoItem = new QTreeWidgetItem(m_ui->qtdirList);
    m_ui->qtdirList->installEventFilter(this);
    autoItem->setText(0, tr("Auto-detected"));
    autoItem->setFirstColumnSpanned(true);
    QTreeWidgetItem *manualItem = new QTreeWidgetItem(m_ui->qtdirList);
    manualItem->setText(0, tr("Manual"));
    manualItem->setFirstColumnSpanned(true);

    for (int i = 0; i < m_versions.count(); ++i) {
        const QtVersion * const version = m_versions.at(i);
        QTreeWidgetItem *item = new QTreeWidgetItem(version->isAutodetected()? autoItem : manualItem);
        item->setText(0, version->displayName());
        item->setText(1, QDir::toNativeSeparators(version->qmakeCommand()));
        item->setData(0, VersionIdRole, version->uniqueId());
    }
    m_ui->qtdirList->expandAll();

    connect(m_versionUi->nameEdit, SIGNAL(textEdited(const QString &)),
            this, SLOT(updateCurrentQtName()));

    connect(m_versionUi->qmakePath, SIGNAL(changed(QString)),
            this, SLOT(updateCurrentQMakeLocation()));
    connect(m_versionUi->s60SDKPath, SIGNAL(changed(QString)),
            this, SLOT(updateCurrentS60SDKDirectory()));
    connect(m_versionUi->sbsV2Path, SIGNAL(changed(QString)),
            this, SLOT(updateCurrentSbsV2Directory()));

    connect(m_ui->addButton, SIGNAL(clicked()),
            this, SLOT(addQtDir()));
    connect(m_ui->delButton, SIGNAL(clicked()),
            this, SLOT(removeQtDir()));

    connect(m_versionUi->qmakePath, SIGNAL(browsingFinished()),
            this, SLOT(onQtBrowsed()));

    connect(m_ui->qtdirList, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
            this, SLOT(versionChanged(QTreeWidgetItem *, QTreeWidgetItem *)));

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

    showEnvironmentPage(0);
    updateState();
}

bool QtOptionsPageWidget::eventFilter(QObject *o, QEvent *e)
{
    // Set the items tooltip, which may cause costly initialization
    // of QtVersion and must be up-to-date
    if (o != m_ui->qtdirList || e->type() != QEvent::ToolTip)
        return false;
    QHelpEvent *helpEvent = static_cast<QHelpEvent *>(e);
    const QPoint treePos = helpEvent->pos() - QPoint(0, m_ui->qtdirList->header()->height());
    QTreeWidgetItem *item = m_ui->qtdirList->itemAt(treePos);
    if (!item)
        return false;
    const int index = indexForTreeItem(item);
    if (index == -1)
        return false;
    const QString tooltip = m_versions.at(index)->toHtml();
    QToolTip::showText(helpEvent->globalPos(), tooltip, m_ui->qtdirList);
    helpEvent->accept();
    return true;
}

int QtOptionsPageWidget::currentIndex() const
{
    if (QTreeWidgetItem *currentItem = m_ui->qtdirList->currentItem())
        return indexForTreeItem(currentItem);
    return -1;
}

QtVersion *QtOptionsPageWidget::currentVersion() const
{
    const int currentItemIndex = currentIndex();
    if (currentItemIndex >= 0 && currentItemIndex < m_versions.size())
        return m_versions.at(currentItemIndex);
    return 0;
}

static inline int findVersionById(const QList<QtVersion *> &l, int id)
{
    const int size = l.size();
    for (int i = 0; i < size; i++)
        if (l.at(i)->uniqueId() == id)
            return i;
    return -1;
}

// Update with results of terminated helper build
void QtOptionsPageWidget::debuggingHelperBuildFinished(int qtVersionId, DebuggingHelperBuildTask::Tools tools, const QString &output)
{
    const int index = findVersionById(m_versions, qtVersionId);
    if (index == -1)
        return; // Oops, somebody managed to delete the version

    m_versions.at(index)->invalidateCache();

    // Update item view
    QTreeWidgetItem *item = treeItemForIndex(index);
    QTC_ASSERT(item, return);
    DebuggingHelperBuildTask::Tools buildFlags
            = item->data(0, BuildRunningRole).value<DebuggingHelperBuildTask::Tools>();
    buildFlags &= ~tools;
    item->setData(0, BuildRunningRole,  QVariant::fromValue(buildFlags));
    item->setData(0, BuildLogRole, output);

    QtVersion *qtVersion = m_versions.at(index);

    bool success = true;
    if (tools & DebuggingHelperBuildTask::GdbDebugging)
        success &= qtVersion->hasGdbDebuggingHelper();
    if (tools & DebuggingHelperBuildTask::QmlDebugging)
        success &= qtVersion->hasQmlDebuggingLibrary();
    if (tools & DebuggingHelperBuildTask::QmlDump)
        success &= qtVersion->hasQmlDump();
    if (tools & DebuggingHelperBuildTask::QmlObserver)
        success &= qtVersion->hasQmlObserver();

    // Update bottom control if the selection is still the same
    if (index == currentIndex()) {
        updateDebuggingHelperUi();
    }
    if (!success)
        showDebuggingBuildLog(item);
}

void QtOptionsPageWidget::buildDebuggingHelper(DebuggingHelperBuildTask::Tools tools)
{
    const int index = currentIndex();
    if (index < 0)
        return;

    QTreeWidgetItem *item = treeItemForIndex(index);
    QTC_ASSERT(item, return);

    DebuggingHelperBuildTask::Tools buildFlags
            = item->data(0, BuildRunningRole).value<DebuggingHelperBuildTask::Tools>();
    buildFlags |= tools;
    item->setData(0, BuildRunningRole, QVariant::fromValue(buildFlags));

    QtVersion *version = m_versions.at(index);
    if (!version)
        return;

    updateDebuggingHelperUi();

    // Run a debugging helper build task in the background.
    DebuggingHelperBuildTask *buildTask = new DebuggingHelperBuildTask(version, tools);
    connect(buildTask, SIGNAL(finished(int,DebuggingHelperBuildTask::Tools,QString)),
            this, SLOT(debuggingHelperBuildFinished(int,DebuggingHelperBuildTask::Tools,QString)),
            Qt::QueuedConnection);
    QFuture<void> task = QtConcurrent::run(&DebuggingHelperBuildTask::run, buildTask);
    const QString taskName = tr("Building helpers");

    Core::ICore::instance()->progressManager()->addTask(task, taskName,
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
    BuildLogDialog *dialog = new BuildLogDialog(this);
    dialog->setWindowTitle(tr("Debugging Helper Build Log for '%1'").arg(currentItem->text(0)));
    dialog->setText(currentItem->data(0, BuildLogRole).toString());
    dialog->show();
}

QtOptionsPageWidget::~QtOptionsPageWidget()
{
    delete m_ui;
    delete m_versionUi;
    delete m_debuggingHelperUi;
    qDeleteAll(m_versions);
}

void QtOptionsPageWidget::addQtDir()
{
    QtVersion *newVersion = new QtVersion(m_specifyNameString, m_specifyPathString);
    m_versions.append(newVersion);

    QTreeWidgetItem *item = new QTreeWidgetItem(m_ui->qtdirList->topLevelItem(1));
    item->setText(0, newVersion->displayName());
    item->setText(1, QDir::toNativeSeparators(newVersion->qmakeCommand()));
    item->setData(0, VersionIdRole, newVersion->uniqueId());

    m_ui->qtdirList->setCurrentItem(item);

    m_versionUi->nameEdit->setText(newVersion->displayName());
    m_versionUi->qmakePath->setPath(newVersion->qmakeCommand());
    m_versionUi->nameEdit->setFocus();
    m_versionUi->nameEdit->selectAll();
}

void QtOptionsPageWidget::removeQtDir()
{
    QTreeWidgetItem *item = m_ui->qtdirList->currentItem();
    int index = indexForTreeItem(item);
    if (index < 0)
        return;

    delete item;

    QtVersion *version = m_versions.at(index);
    m_versions.removeAt(index);
    delete version;
    updateState();
}

void QtOptionsPageWidget::updateDebuggingHelperUi()
{
    const QtVersion *version = currentVersion();
    const QTreeWidgetItem *currentItem = m_ui->qtdirList->currentItem();

    if (!version || !version->supportsBinaryDebuggingHelper()) {
        m_ui->debuggingHelperWidget->setVisible(false);
    } else {
        const DebuggingHelperBuildTask::Tools availableTools = DebuggingHelperBuildTask::availableTools(version);
        const bool canBuildGdbHelper = availableTools & DebuggingHelperBuildTask::GdbDebugging;
        const bool canBuildQmlDumper = availableTools & DebuggingHelperBuildTask::QmlDump;
        const bool canBuildQmlDebuggingLib = availableTools & DebuggingHelperBuildTask::QmlDebugging;
        const bool canBuildQmlObserver = availableTools & DebuggingHelperBuildTask::QmlObserver;

        const bool hasGdbHelper = !version->gdbDebuggingHelperLibrary().isEmpty();
        const bool hasQmlDumper = version->hasQmlDump();
        const bool hasQmlDebuggingLib = version->hasQmlDebuggingLibrary();
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
        if (hasGdbHelper)
            helperNames << m_debuggingHelperUi->gdbHelperLabel->text().remove(':');
        if (hasQmlDumper)
            helperNames << m_debuggingHelperUi->qmlDumpLabel->text().remove(':');
        if (hasQmlDebuggingLib)
            helperNames << m_debuggingHelperUi->qmlDebuggingLibLabel->text().remove(':');
        if (hasQmlObserver)
            helperNames << m_debuggingHelperUi->qmlObserverLabel->text().remove(':');

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
            if (canBuildGdbHelper) {
                gdbHelperText =  tr("<i>Not yet built.</i>");
            } else {
                gdbHelperText =  tr("<i>Not needed.</i>");
            }
        }
        m_debuggingHelperUi->gdbHelperStatus->setText(gdbHelperText);
        m_debuggingHelperUi->gdbHelperStatus->setTextInteractionFlags(gdbHelperTextFlags);
        m_debuggingHelperUi->gdbHelperBuildButton->setEnabled(canBuildGdbHelper && !isBuildingGdbHelper);

        QString qmlDumpStatusText;
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
            if (canBuildQmlDumper) {
                qmlDumpStatusText = tr("<i>Not yet built.</i>");
            } else {
                qmlDumpStatusText = tr("<i>Cannot be compiled.</i>");
            }
        }
        m_debuggingHelperUi->qmlDumpStatus->setText(qmlDumpStatusText);
        m_debuggingHelperUi->qmlDumpStatus->setTextInteractionFlags(qmlDumpStatusTextFlags);
        m_debuggingHelperUi->qmlDumpBuildButton->setEnabled(canBuildQmlDumper & !isBuildingQmlDumper);

        QString qmlDebuggingLibStatusText;
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
        }  else {
            if (canBuildQmlDebuggingLib) {
                qmlDebuggingLibStatusText = tr("<i>Not yet built.</i>");
            } else {
                qmlDebuggingLibStatusText = tr("<i>Cannot be compiled.</i>");
            }
        }
        m_debuggingHelperUi->qmlDebuggingLibStatus->setText(qmlDebuggingLibStatusText);
        m_debuggingHelperUi->qmlDebuggingLibStatus->setTextInteractionFlags(qmlDebuggingLibStatusTextFlags);
        m_debuggingHelperUi->qmlDebuggingLibBuildButton->setEnabled(canBuildQmlDebuggingLib
                                                                    && !isBuildingQmlDebuggingLib);


        QString qmlObserverStatusText;
        Qt::TextInteractionFlags qmlObserverStatusTextFlags = Qt::NoTextInteraction;
        if (hasQmlObserver) {
            qmlObserverStatusText = QDir::toNativeSeparators(version->qmlObserverTool());
            qmlObserverStatusTextFlags = Qt::TextSelectableByMouse;
        }  else {
            if (canBuildQmlObserver) {
                qmlObserverStatusText = tr("<i>Not yet built.</i>");
            } else {
                qmlObserverStatusText = tr("<i>Cannot be compiled.</i>");
            }
        }
        m_debuggingHelperUi->qmlObserverStatus->setText(qmlObserverStatusText);
        m_debuggingHelperUi->qmlObserverStatus->setTextInteractionFlags(qmlObserverStatusTextFlags);
        m_debuggingHelperUi->qmlObserverBuildButton->setEnabled(canBuildQmlObserver
                                                                & !isBuildingQmlObserver);

        const bool hasLog = currentItem && !currentItem->data(0, BuildLogRole).toString().isEmpty();
        m_debuggingHelperUi->showLogButton->setEnabled(hasLog);

        m_debuggingHelperUi->rebuildButton->setEnabled(!isBuildingGdbHelper
                                                       && !isBuildingQmlDumper
                                                       && !isBuildingQmlDebuggingLib
                                                       && !isBuildingQmlObserver);

        m_ui->debuggingHelperWidget->setVisible(true);
    }

}

void QtOptionsPageWidget::updateState()
{
    const QtVersion *version  = currentVersion();
    const bool enabled = version != 0;
    const bool isAutodetected = enabled && version->isAutodetected();
    m_ui->delButton->setEnabled(enabled && !isAutodetected);
    m_versionUi->nameEdit->setEnabled(enabled && !isAutodetected);
    m_versionUi->qmakePath->setEnabled(enabled && !isAutodetected);
    bool s60SDKPathEnabled = enabled &&
                             (isAutodetected ? version->systemRoot().isEmpty() : true);
    m_versionUi->s60SDKPath->setEnabled(s60SDKPathEnabled);

    updateDebuggingHelperUi();
}

void QtOptionsPageWidget::makeS60Visible(bool visible)
{
    m_versionUi->s60SDKLabel->setVisible(visible);
    m_versionUi->s60SDKPath->setVisible(visible);
    m_versionUi->sbsV2Label->setVisible(visible);
    m_versionUi->sbsV2Path->setVisible(visible);
}

void QtOptionsPageWidget::showEnvironmentPage(QTreeWidgetItem *item)
{
    makeS60Visible(false);
    m_versionUi->errorLabel->setText("");
    if (!item)
        return;

    int index = indexForTreeItem(item);

    if (index < 0)
        return;

    QtVersion *qtVersion = m_versions.at(index);
    if (!qtVersion->isValid()) {
        m_versionUi->errorLabel->setText(qtVersion->invalidReason());
        return;
    }

    ProjectExplorer::Abi qtAbi = qtVersion->qtAbis().at(0);

    if (qtAbi.os() == ProjectExplorer::Abi::SymbianOS) {
        makeS60Visible(true);
        m_versionUi->s60SDKPath->setPath(QDir::toNativeSeparators(m_versions.at(index)->systemRoot()));
        m_versionUi->sbsV2Path->setPath(m_versions.at(index)->sbsV2Directory());
        m_versionUi->sbsV2Path->setEnabled(m_versions.at(index)->isBuildWithSymbianSbsV2());
    }
    m_versionUi->errorLabel->setText(m_versions.at(index)->description());
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
            if (item->data(0, VersionIdRole).toInt() == uniqueId) {
                return item;
            }
        }
    }
    return 0;
}

void QtOptionsPageWidget::versionChanged(QTreeWidgetItem *item, QTreeWidgetItem *old)
{
    if (old) {
        fixQtVersionName(indexForTreeItem(old));
    }
    int itemIndex = indexForTreeItem(item);
    if (itemIndex >= 0) {
        m_versionUi->nameEdit->setText(item->text(0));
        m_versionUi->qmakePath->setPath(item->text(1));
    } else {
        m_versionUi->nameEdit->clear();
        m_versionUi->qmakePath->setPath(QString()); // clear()

    }
    showEnvironmentPage(item);
    updateState();
}

void QtOptionsPageWidget::onQtBrowsed()
{
    const QString dir = m_versionUi->qmakePath->path();
    if (dir.isEmpty())
        return;

    updateCurrentQMakeLocation();
    updateState();
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
    m_versionUi->errorLabel->setText(m_versions.at(currentItemIndex)->description());
}


void QtOptionsPageWidget::finish()
{
    if (QTreeWidgetItem *item = m_ui->qtdirList->currentItem())
        fixQtVersionName(indexForTreeItem(item));
}

/* Checks that the qt version name is unique
 * and otherwise changes the name
 *
 */
void QtOptionsPageWidget::fixQtVersionName(int index)
{
    if (index < 0)
        return;
    int count = m_versions.count();
    QString name = m_versions.at(index)->displayName();
    for (int i = 0; i < count; ++i) {
        if (i != index) {
            if (m_versions.at(i)->displayName() == m_versions.at(index)->displayName()) {
                // Same name, find new name
                QRegExp regexp("^(.*)\\((\\d)\\)$");
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

void QtOptionsPageWidget::updateCurrentQMakeLocation()
{
    QTreeWidgetItem *currentItem = m_ui->qtdirList->currentItem();
    Q_ASSERT(currentItem);
    int currentItemIndex = indexForTreeItem(currentItem);
    if (currentItemIndex < 0)
        return;
    QtVersion *version = m_versions.at(currentItemIndex);
    QFileInfo fi(m_versionUi->qmakePath->path());
    if (!fi.exists() || !fi.isFile() || version->qmakeCommand() == fi.absoluteFilePath())
        return;
    version->setQMakeCommand(fi.absoluteFilePath());
    currentItem->setText(1, QDir::toNativeSeparators(version->qmakeCommand()));
    showEnvironmentPage(currentItem);

    updateDebuggingHelperUi();

    if (m_versionUi->nameEdit->text().isEmpty() || m_versionUi->nameEdit->text() == m_specifyNameString) {
        QString name = ProjectExplorer::DebuggingHelperLibrary::qtVersionForQMake(version->qmakeCommand());
        if (!name.isEmpty())
            m_versionUi->nameEdit->setText(name);
        updateCurrentQtName();
    }
}

void QtOptionsPageWidget::updateCurrentS60SDKDirectory()
{
    QTreeWidgetItem *currentItem = m_ui->qtdirList->currentItem();
    Q_ASSERT(currentItem);
    int currentItemIndex = indexForTreeItem(currentItem);
    if (currentItemIndex < 0)
        return;
    m_versions[currentItemIndex]->setSystemRoot(m_versionUi->s60SDKPath->path());
}

void QtOptionsPageWidget::updateCurrentSbsV2Directory()
{
    QTreeWidgetItem *currentItem = m_ui->qtdirList->currentItem();
    Q_ASSERT(currentItem);
    int currentItemIndex = indexForTreeItem(currentItem);
    if (currentItemIndex < 0)
        return;
    m_versions[currentItemIndex]->setSbsV2Directory(m_versionUi->sbsV2Path->path());
}

QList<QtVersion *> QtOptionsPageWidget::versions() const
{
    QList<QtVersion *> result;
    for (int i = 0; i < m_versions.count(); ++i)
        if (m_versions.at(i)->qmakeCommand() != m_specifyPathString)
            result.append(new QtVersion(*(m_versions.at(i))));
    return result;
}

QString QtOptionsPageWidget::searchKeywords() const
{
    QString rc;
    QLatin1Char sep(' ');
    QTextStream(&rc)
            << sep << m_versionUi->versionNameLabel->text()
            << sep << m_versionUi->pathLabel->text()
            << sep << m_versionUi->s60SDKLabel->text()
            << sep << m_versionUi->sbsV2Label->text()
            << sep << m_debuggingHelperUi->gdbHelperLabel->text()
            << sep << m_debuggingHelperUi->qmlDumpLabel->text()
            << sep << m_debuggingHelperUi->qmlObserverLabel->text();
    rc.remove(QLatin1Char('&'));
    return rc;
}
