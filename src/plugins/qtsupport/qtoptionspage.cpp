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

#include "qtoptionspage.h"
#include "qtconfigwidget.h"
#include "ui_showbuildlog.h"
#include "ui_qtversionmanager.h"
#include "ui_qtversioninfo.h"
#include "ui_debugginghelper.h"
#include "qtsupportconstants.h"
#include "qtversionmanager.h"
#include "qtversionfactory.h"
#include "qmldumptool.h"

#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/variablechooser.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/hostosinfo.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>
#include <utils/algorithm.h>
#include <utils/themehelper.h>
#include <utils/treemodel.h>

#include <QDir>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextBrowser>
#include <QDesktopServices>
#include <QSortFilterProxyModel>

#include <utility>

using namespace ProjectExplorer;
using namespace Utils;

namespace QtSupport {
namespace Internal {

class QtVersionItem : public TreeItem
{
public:
    QtVersionItem(const QString &name) // for auto/manual node
        : TreeItem({name})
    {}

    QtVersionItem(BaseQtVersion *version) // for versions
        : TreeItem(),
          m_version(version)
    {}

    ~QtVersionItem()
    {
        delete m_version;
    }

    void setVersion(BaseQtVersion *version)
    {
        m_version = version;
        update();
    }

    int uniqueId() const
    {
        return m_version ? m_version->uniqueId() : -1;
    }

    BaseQtVersion *version() const
    {
        return m_version;
    }

    QVariant data(int column, int role) const
    {
        if (!m_version)
            return TreeItem::data(column, role);

        if (role == Qt::DisplayRole) {
            if (column == 0)
                return m_version->displayName();
            if (column == 1)
                return m_version->qmakeCommand().toUserOutput();
        }

        if (role == Qt::DecorationRole && column == 0)
            return m_icon;

        return QVariant();
    }

    void setIcon(const QIcon &icon)
    {
        m_icon = icon;
        update();
    }

    QString buildLog() const
    {
        return m_buildLog;
    }

    void setBuildLog(const QString &buildLog)
    {
        m_buildLog = buildLog;
    }

    QByteArray toolChainId() const
    {
        return m_toolChainId;
    }

    void setToolChainId(const QByteArray &id)
    {
        m_toolChainId = id;
    }

    DebuggingHelperBuildTask::Tools buildFlags() const
    {
        return m_buildFlags;
    }

    void setBuildFlags(DebuggingHelperBuildTask::Tools flags)
    {
        m_buildFlags = flags;
    }

private:
    BaseQtVersion *m_version = 0;
    QIcon m_icon;
    QString m_buildLog;
    QByteArray m_toolChainId;
    DebuggingHelperBuildTask::Tools m_buildFlags;
};

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

QWidget *QtOptionsPage::widget()
{
    if (!m_widget)
        m_widget = new QtOptionsPageWidget;
    return m_widget;
}

void QtOptionsPage::apply()
{
    if (!m_widget) // page was never shown
        return;
    m_widget->apply();
}

void QtOptionsPage::finish()
{
    delete m_widget;
}

//-----------------------------------------------------


QtOptionsPageWidget::QtOptionsPageWidget(QWidget *parent)
    : QWidget(parent)
    , m_specifyNameString(tr("<specify a name>"))
    , m_ui(new Internal::Ui::QtVersionManager())
    , m_versionUi(new Internal::Ui::QtVersionInfo())
    , m_debuggingHelperUi(new Internal::Ui::DebuggingHelper())
    , m_infoBrowser(new QTextBrowser)
    , m_invalidVersionIcon(Utils::ThemeHelper::themedIcon(QLatin1String(Core::Constants::ICON_ERROR)))
    , m_warningVersionIcon(Utils::ThemeHelper::themedIcon(QLatin1String(Core::Constants::ICON_WARNING)))
    , m_configurationWidget(0)
    , m_autoItem(0)
    , m_manualItem(0)
{
    QWidget *versionInfoWidget = new QWidget();
    m_versionUi->setupUi(versionInfoWidget);
    m_versionUi->editPathPushButton->setText(PathChooser::browseButtonLabel());

    QWidget *debuggingHelperDetailsWidget = new QWidget();
    m_debuggingHelperUi->setupUi(debuggingHelperDetailsWidget);

    m_ui->setupUi(this);

    m_infoBrowser->setOpenLinks(false);
    m_infoBrowser->setTextInteractionFlags(Qt::TextBrowserInteraction);
    connect(m_infoBrowser, &QTextBrowser::anchorClicked,
            this, &QtOptionsPageWidget::infoAnchorClicked);
    m_ui->infoWidget->setWidget(m_infoBrowser);
    connect(m_ui->infoWidget, &DetailsWidget::expanded,
            this, &QtOptionsPageWidget::setInfoWidgetVisibility);

    m_ui->versionInfoWidget->setWidget(versionInfoWidget);
    m_ui->versionInfoWidget->setState(DetailsWidget::NoSummary);

    m_ui->debuggingHelperWidget->setWidget(debuggingHelperDetailsWidget);
    connect(m_ui->debuggingHelperWidget, &DetailsWidget::expanded,
            this, &QtOptionsPageWidget::setInfoWidgetVisibility);

    auto rootItem = new QtVersionItem(QLatin1String("root"));
    m_autoItem = new QtVersionItem(tr("Auto-detected"));
    rootItem->appendChild(m_autoItem);
    m_manualItem = new QtVersionItem(tr("Manual"));
    rootItem->appendChild(m_manualItem);

    m_model = new TreeModel(rootItem);
    m_model->setHeader({tr("Name"), tr("qmake Location"), tr("Type")});

    m_filterModel = new QSortFilterProxyModel(this);
    m_filterModel->setSourceModel(m_model);
    m_filterModel->setSortCaseSensitivity(Qt::CaseInsensitive);

    m_ui->qtdirList->setModel(m_filterModel);
    m_ui->qtdirList->setSortingEnabled(true);

    m_ui->qtdirList->setFirstColumnSpanned(0, QModelIndex(), true);
    m_ui->qtdirList->setFirstColumnSpanned(1, QModelIndex(), true);

    m_ui->qtdirList->header()->setStretchLastSection(false);
    m_ui->qtdirList->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_ui->qtdirList->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_ui->qtdirList->setTextElideMode(Qt::ElideNone);
    m_ui->qtdirList->sortByColumn(0, Qt::AscendingOrder);

    QList<int> additions = transform(QtVersionManager::versions(), &BaseQtVersion::uniqueId);

    updateQtVersions(additions, QList<int>(), QList<int>());

    m_ui->qtdirList->expandAll();

    connect(m_versionUi->nameEdit, &QLineEdit::textEdited,
            this, &QtOptionsPageWidget::updateCurrentQtName);

    connect(m_versionUi->editPathPushButton, &QAbstractButton::clicked,
            this, &QtOptionsPageWidget::editPath);

    connect(m_ui->addButton, &QAbstractButton::clicked,
            this, &QtOptionsPageWidget::addQtDir);
    connect(m_ui->delButton, &QAbstractButton::clicked,
            this, &QtOptionsPageWidget::removeQtDir);

    connect(m_ui->qtdirList->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &QtOptionsPageWidget::versionChanged);

    connect(m_debuggingHelperUi->rebuildButton, &QAbstractButton::clicked,
            this, [this]() { buildDebuggingHelper(); });
    connect(m_debuggingHelperUi->qmlDumpBuildButton, &QAbstractButton::clicked,
            this, &QtOptionsPageWidget::buildQmlDump);

    connect(m_debuggingHelperUi->showLogButton, &QAbstractButton::clicked,
            this, &QtOptionsPageWidget::slotShowDebuggingBuildLog);
    connect(m_debuggingHelperUi->toolChainComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
            this, &QtOptionsPageWidget::selectedToolChainChanged);

    connect(m_ui->cleanUpButton, &QAbstractButton::clicked,
            this, &QtOptionsPageWidget::cleanUpQtVersions);
    userChangedCurrentVersion();
    updateCleanUpButton();

    connect(QtVersionManager::instance(), &QtVersionManager::dumpUpdatedFor,
            this, &QtOptionsPageWidget::qtVersionsDumpUpdated);

    connect(QtVersionManager::instance(), &QtVersionManager::qtVersionsChanged,
            this, &QtOptionsPageWidget::updateQtVersions);

    connect(ProjectExplorer::ToolChainManager::instance(), &ToolChainManager::toolChainsChanged,
            this, &QtOptionsPageWidget::toolChainsUpdated);

    auto chooser = new Core::VariableChooser(this);
    chooser->addSupportedWidget(m_versionUi->nameEdit, "Qt:Name");
    chooser->addMacroExpanderProvider(
        [this]() -> Utils::MacroExpander * {
            BaseQtVersion *version = currentVersion();
            return version ? version->macroExpander() : 0;
        });
}

BaseQtVersion *QtOptionsPageWidget::currentVersion() const
{
    QtVersionItem *item = currentItem();
    if (!item)
        return 0;
    return item->version();
}

QtVersionItem *QtOptionsPageWidget::currentItem() const
{
    QModelIndex idx = m_ui->qtdirList->selectionModel()->currentIndex();
    QModelIndex sourceIdx = m_filterModel->mapToSource(idx);
    QtVersionItem *item = static_cast<QtVersionItem *>(m_model->itemForIndex(sourceIdx));
    return item;
}

// Update with results of terminated helper build
void QtOptionsPageWidget::debuggingHelperBuildFinished(int qtVersionId, const QString &output, DebuggingHelperBuildTask::Tools tools)
{
    auto findItem = [qtVersionId](Utils::TreeItem *parent) {
        foreach (Utils::TreeItem *child, parent->children()) {
            auto item = static_cast<QtVersionItem *>(child);
            if (item->version()->uniqueId() == qtVersionId)
                return item;
        }
        return (QtVersionItem *)nullptr;
    };

    QtVersionItem *item = findItem(m_manualItem);
    if (!item)
        item = findItem(m_autoItem);

    if (!item)
        return;


    DebuggingHelperBuildTask::Tools buildFlags = item->buildFlags();
    buildFlags &= ~tools;
    item->setBuildFlags(buildFlags);
    item->setBuildLog(output);

    bool success = true;
    if (tools & DebuggingHelperBuildTask::QmlDump)
        success &= item->version()->hasQmlDump();

    if (!success)
        showDebuggingBuildLog(item);

    updateDebuggingHelperUi();
}

void QtOptionsPageWidget::cleanUpQtVersions()
{
    QVector<QtVersionItem *> toRemove;
    QString text;

    foreach (Utils::TreeItem *child, m_manualItem->children()) {
        auto item = static_cast<QtVersionItem *>(child);
        if (item->version() && !item->version()->isValid()) {
            toRemove.append(item);
            if (!text.isEmpty())
                text.append(QLatin1String("</li><li>"));
            text.append(item->version()->displayName());
        }
    }

    if (toRemove.isEmpty())
        return;


    if (QMessageBox::warning(0, tr("Remove Invalid Qt Versions"),
                             tr("Do you want to remove all invalid Qt Versions?<br>"
                                "<ul><li>%1</li></ul><br>"
                                "will be removed.").arg(text),
                             QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
        return;

    foreach (QtVersionItem *item, toRemove) {
        m_model->takeItem(item);
        delete item;
    }

    updateCleanUpButton();
}

void QtOptionsPageWidget::toolChainsUpdated()
{
    auto update = [this](Utils::TreeItem *parent) {
        foreach (Utils::TreeItem *child, parent->children()) {
            if (child == currentItem()) {
                updateDescriptionLabel();
                updateDebuggingHelperUi();
            } else {
                updateVersionItem(static_cast<QtVersionItem *>(child));
            }
        }
    };

    update(m_autoItem);
    update(m_manualItem);
}

void QtOptionsPageWidget::selectedToolChainChanged(int comboIndex)
{
    QtVersionItem *item = currentItem();
    if (!item)
        return;

    QByteArray toolChainId = m_debuggingHelperUi->toolChainComboBox->itemData(comboIndex).toByteArray();
    item->setToolChainId(toolChainId);
}

void QtOptionsPageWidget::qtVersionsDumpUpdated(const FileName &qmakeCommand)
{
    auto recheck = [qmakeCommand](Utils::TreeItem *parent) {
        foreach (Utils::TreeItem *child, parent->children()) {
            auto item = static_cast<QtVersionItem *>(child);
            if (item->version()->qmakeCommand() == qmakeCommand)
                item->version()->recheckDumper();
        }
    };

    recheck(m_autoItem);
    recheck(m_manualItem);

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
    foreach (const Abi &abi, version->qtAbis()) {
        if (ToolChainManager::findToolChains(abi).isEmpty())
            missingToolChains.append(abi.toString());
        ++abiCount;
    }

    bool useable = true;
    QStringList warnings;
    if (!isNameUnique(version))
        warnings << tr("Display Name is not unique.");

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
            info.message = warnings.join(QLatin1Char('\n'));
            info.icon = m_warningVersionIcon;
        }
    }

    return info;
}

QList<ToolChain*> QtOptionsPageWidget::toolChains(const BaseQtVersion *version)
{
    QList<ToolChain*> toolChains;
    if (!version)
        return toolChains;

    QSet<QByteArray> ids;
    foreach (const Abi &a, version->qtAbis()) {
        foreach (ToolChain *tc, ToolChainManager::findToolChains(a)) {
            if (ids.contains(tc->id()))
                continue;
            ids.insert(tc->id());
            toolChains.append(tc);
        }
    }

    return toolChains;
}

QByteArray QtOptionsPageWidget::defaultToolChainId(const BaseQtVersion *version)
{
    QList<ToolChain*> possibleToolChains = toolChains(version);
    if (!possibleToolChains.isEmpty())
        return possibleToolChains.first()->id();
    return QByteArray();
}

bool QtOptionsPageWidget::isNameUnique(const BaseQtVersion *version)
{
    const QString name = version->displayName().trimmed();

    auto isUnique = [name, version](Utils::TreeItem *parent) {
        foreach (Utils::TreeItem *child, parent->children()) {
            auto item = static_cast<QtVersionItem *>(child);
            if (item->version() == version)
                continue;
            if (item->version()->displayName().trimmed() == name)
                return false;
        }
        return true;
    };

    return isUnique(m_manualItem) && isUnique(m_autoItem);
}

void QtOptionsPageWidget::updateVersionItem(QtVersionItem *item)
{
    if (!item)
        return;
    if (!item->version())
        return;

    const ValidityInfo info = validInformation(item->version());
    item->update();
    item->setIcon(info.icon);
}

void QtOptionsPageWidget::buildDebuggingHelper(DebuggingHelperBuildTask::Tools tools)
{
    QtVersionItem *item = currentItem();
    if (!item)
        return;

    if (!item->version())
        return;

    // remove tools that cannot be build
    tools &= DebuggingHelperBuildTask::availableTools(currentVersion());

    DebuggingHelperBuildTask::Tools buildFlags = item->buildFlags();
    buildFlags |= tools;
    item->setBuildFlags(buildFlags);

    updateDebuggingHelperUi();

    // Run a debugging helper build task in the background.
    QByteArray toolChainId = m_debuggingHelperUi->toolChainComboBox->itemData(
                m_debuggingHelperUi->toolChainComboBox->currentIndex()).toByteArray();
    ToolChain *toolChain = ToolChainManager::findToolChain(toolChainId);
    if (!toolChain)
        return;

    DebuggingHelperBuildTask *buildTask = new DebuggingHelperBuildTask(item->version(), toolChain, tools);
    // Don't open General Messages pane with errors
    buildTask->showOutputOnError(false);
    connect(buildTask, SIGNAL(finished(int,QString,DebuggingHelperBuildTask::Tools)),
            this, SLOT(debuggingHelperBuildFinished(int,QString,DebuggingHelperBuildTask::Tools)),
            Qt::QueuedConnection);
    QFuture<void> task = QtConcurrent::run(&DebuggingHelperBuildTask::run, buildTask);
    const QString taskName = tr("Building Helpers");

    Core::ProgressManager::addTask(task, taskName, "QmakeProjectManager::BuildHelpers");
}

void QtOptionsPageWidget::buildQmlDump()
{
    buildDebuggingHelper(DebuggingHelperBuildTask::QmlDump);
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
    if (const QtVersionItem *item = currentItem())
        showDebuggingBuildLog(item);
}

void QtOptionsPageWidget::showDebuggingBuildLog(const QtVersionItem *item)
{
    BaseQtVersion *version = item->version();
    if (!version)
        return;
    BuildLogDialog *dialog = new BuildLogDialog(this->window());
    dialog->setWindowTitle(tr("Debugging Helper Build Log for \"%1\"").arg(version->displayName()));
    dialog->setText(item->buildLog());
    dialog->show();
}

void QtOptionsPageWidget::updateQtVersions(const QList<int> &additions, const QList<int> &removals,
                                           const QList<int> &changes)
{
    QList<QtVersionItem *> toRemove;
    QList<int> toAdd = additions;

    // Generate list of all existing items:
    QList<QtVersionItem *> itemList;
    for (int i = 0; i < m_autoItem->childCount(); ++i)
        itemList.append(static_cast<QtVersionItem *>(m_autoItem->child(i)));
    for (int i = 0; i < m_manualItem->childCount(); ++i)
        itemList.append(static_cast<QtVersionItem *>(m_manualItem->child(i)));

    // Find existing items to remove/change:
    foreach (QtVersionItem *item, itemList) {
        int id = item->uniqueId();
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
    foreach (QtVersionItem *item, toRemove) {
        m_model->takeItem(item);
        delete item;
    }

    // Add changed/added items:
    foreach (int a, toAdd) {
        BaseQtVersion *version = QtVersionManager::version(a)->clone();
        auto *item = new QtVersionItem(version);

        item->setToolChainId(defaultToolChainId(version));

        // Insert in the right place:
        Utils::TreeItem *parent = version->isAutodetected()? m_autoItem : m_manualItem;
        parent->appendChild(item);
    }

    auto update = [this](Utils::TreeItem *parent) {
        foreach (Utils::TreeItem *child, parent->children())
            updateVersionItem(static_cast<QtVersionItem *>(child));
    };

    update(m_autoItem);
    update(m_manualItem);
}

QtOptionsPageWidget::~QtOptionsPageWidget()
{
    delete m_ui;
    delete m_versionUi;
    delete m_debuggingHelperUi;
    delete m_configurationWidget;
}

void QtOptionsPageWidget::addQtDir()
{
    FileName qtVersion = FileName::fromString(
                QFileDialog::getOpenFileName(this,
                                             tr("Select a qmake Executable"),
                                             QString(),
                                             BuildableHelperLibrary::filterForQmakeFileDialog(),
                                             0,
                                             QFileDialog::DontResolveSymlinks));
    if (qtVersion.isNull())
        return;

    QFileInfo fi = qtVersion.toFileInfo();
    // should add all qt versions here ?
    if (BuildableHelperLibrary::isQtChooser(fi))
        qtVersion = FileName::fromString(BuildableHelperLibrary::qtChooserToQmakePath(fi.symLinkTarget()));

    auto checkAlreadyExists = [qtVersion](Utils::TreeItem *parent) {
        for (int i = 0; i < parent->childCount(); ++i) {
            auto item = static_cast<QtVersionItem *>(parent->childAt(i));
            if (item->version()->qmakeCommand() == qtVersion) {
                return std::make_pair(true, item->version()->displayName());
            }
        }
        return std::make_pair(false, QString());
    };

    bool alreadyExists;
    QString otherName;
    std::tie(alreadyExists, otherName) = checkAlreadyExists(m_autoItem);
    if (!alreadyExists)
        std::tie(alreadyExists, otherName) = checkAlreadyExists(m_manualItem);

    if (alreadyExists) {
        // Already exist
        QMessageBox::warning(this, tr("Qt Version Already Known"),
                             tr("This Qt version was already registered as \"%1\".")
                             .arg(otherName));
        return;
    }

    QString error;
    BaseQtVersion *version = QtVersionFactory::createQtVersionFromQMakePath(qtVersion, false, QString(), &error);
    if (version) {
        auto item = new QtVersionItem(version);
        item->setIcon(version->isValid()? m_validVersionIcon : m_invalidVersionIcon);
        m_manualItem->appendChild(item);
        item->setToolChainId(defaultToolChainId(version));

        QModelIndex source = m_model->indexForItem(item);
        m_ui->qtdirList->setCurrentIndex(m_filterModel->mapFromSource(source)); // should update the rest of the ui
        m_versionUi->nameEdit->setFocus();
        m_versionUi->nameEdit->selectAll();
    } else {
        QMessageBox::warning(this, tr("Qmake Not Executable"),
                             tr("The qmake executable %1 could not be added: %2").arg(qtVersion.toUserOutput()).arg(error));
        return;
    }
    updateCleanUpButton();
}

void QtOptionsPageWidget::removeQtDir()
{
    QtVersionItem *item = currentItem();
    if (!item)
        return;

    m_model->takeItem(item);
    delete item;

    updateCleanUpButton();
}

void QtOptionsPageWidget::editPath()
{
    BaseQtVersion *current = currentVersion();
    QString dir = currentVersion()->qmakeCommand().toFileInfo().absolutePath();
    FileName qtVersion = FileName::fromString(
                QFileDialog::getOpenFileName(this,
                                             tr("Select a qmake Executable"),
                                             dir,
                                             BuildableHelperLibrary::filterForQmakeFileDialog(),
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
    if (current->unexpandedDisplayName() != current->defaultUnexpandedDisplayName(current->qmakeCommand()))
        version->setUnexpandedDisplayName(current->displayName());

    // Update ui
    QtVersionItem *item = currentItem();
    item->setVersion(version);
    item->setToolChainId(defaultToolChainId(version));
    item->setIcon(version->isValid()? m_validVersionIcon : m_invalidVersionIcon);
    userChangedCurrentVersion();

    delete current;
}

void QtOptionsPageWidget::updateDebuggingHelperUi()
{
    BaseQtVersion *version = currentVersion();
    const QtVersionItem *item = currentItem();

    QList<ToolChain*> toolchains = toolChains(currentVersion());

    if (!version || !version->isValid() || toolchains.isEmpty()) {
        m_ui->debuggingHelperWidget->setVisible(false);
    } else {
        const DebuggingHelperBuildTask::Tools availableTools = DebuggingHelperBuildTask::availableTools(version);
        const bool canBuildQmlDumper = availableTools & DebuggingHelperBuildTask::QmlDump;

        const bool hasQmlDumper = version->hasQmlDump();
        const bool needsQmlDumper = version->needsQmlDump();

        bool isBuildingQmlDumper = false;

        if (item) {
            DebuggingHelperBuildTask::Tools buildingTools = item->buildFlags();
            isBuildingQmlDumper = buildingTools & DebuggingHelperBuildTask::QmlDump;
        }

        // get names of tools from labels
        QStringList helperNames;
        const QChar colon = QLatin1Char(':');
        if (hasQmlDumper)
            helperNames << m_debuggingHelperUi->qmlDumpLabel->text().remove(colon);

        QString status;
        if (helperNames.isEmpty()) {
            status = tr("Helpers: None available");
        } else {
            //: %1 is list of tool names.
            status = tr("Helpers: %1.").arg(helperNames.join(QLatin1String(", ")));
        }

        m_ui->debuggingHelperWidget->setSummaryText(status);

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

        QList<ToolChain*> toolchains = toolChains(currentVersion());
        QByteArray selectedToolChainId = item->toolChainId();
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

        const bool hasLog = item && !item->buildLog().isEmpty();
        m_debuggingHelperUi->showLogButton->setEnabled(hasLog);

        const bool canBuild = canBuildQmlDumper;
        const bool isBuilding = isBuildingQmlDumper;

        m_debuggingHelperUi->rebuildButton->setEnabled(canBuild && !isBuilding);
        m_debuggingHelperUi->toolChainComboBox->setEnabled(canBuild && !isBuilding);
        setInfoWidgetVisibility();
    }
}

// To be called if a Qt version was removed or added
void QtOptionsPageWidget::updateCleanUpButton()
{
    bool hasInvalidVersion = false;
    foreach (Utils::TreeItem *child, m_manualItem->children()) {
        auto item = static_cast<QtVersionItem *>(child);
        if (item->version() && !item->version()->isValid()) {
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
    QtVersionItem *item = currentItem();
    const BaseQtVersion *version = item->version();
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
        item->setIcon(info.icon);

    if (version) {
        m_infoBrowser->setHtml(version->toHtml(true));
        setInfoWidgetVisibility();
    } else {
        m_infoBrowser->clear();
        m_ui->versionInfoWidget->setVisible(false);
        m_ui->infoWidget->setVisible(false);
        m_ui->debuggingHelperWidget->setVisible(false);
    }
}

void QtOptionsPageWidget::versionChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
    userChangedCurrentVersion();
}

void QtOptionsPageWidget::updateWidgets()
{
    delete m_configurationWidget;
    m_configurationWidget = 0;
    BaseQtVersion *version = currentVersion();
    if (version) {
        m_versionUi->nameEdit->setText(version->unexpandedDisplayName());
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
        m_versionUi->qmakePath->clear();
    }

    const bool enabled = version != 0;
    const bool isAutodetected = enabled && version->isAutodetected();
    m_ui->delButton->setEnabled(enabled && !isAutodetected);
    m_versionUi->nameEdit->setEnabled(enabled);
    m_versionUi->editPathPushButton->setEnabled(enabled && !isAutodetected);
}

void QtOptionsPageWidget::updateCurrentQtName()
{
    QtVersionItem *item = currentItem();
    if (!item || !item->version())
        return;

    item->version()->setUnexpandedDisplayName(m_versionUi->nameEdit->text());

    updateDescriptionLabel();

    auto update = [this](Utils::TreeItem *parent) {
        foreach (Utils::TreeItem *child, parent->children())
            updateVersionItem(static_cast<QtVersionItem *>(child));
    };

    update(m_autoItem);
    update(m_manualItem);
}

void QtOptionsPageWidget::apply()
{
    disconnect(QtVersionManager::instance(), &QtVersionManager::qtVersionsChanged,
            this, &QtOptionsPageWidget::updateQtVersions);

    QtVersionManager::setNewQtVersions(versions());

    connect(QtVersionManager::instance(), &QtVersionManager::qtVersionsChanged,
            this, &QtOptionsPageWidget::updateQtVersions);
}

QList<BaseQtVersion *> QtOptionsPageWidget::versions() const
{
    QList<BaseQtVersion *> result;
    auto gather = [&result](TreeItem *parent) {
        result.reserve(result.size() + parent->childCount());
        for (int i = 0; i < parent->childCount(); ++i)
            result.append(static_cast<QtVersionItem *>(parent->childAt(i))->version()->clone());
    };

    gather(m_autoItem);
    gather(m_manualItem);

    return result;
}

} // namespace Internal
} // namespace QtSupport
