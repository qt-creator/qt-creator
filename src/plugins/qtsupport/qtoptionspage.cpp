/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qtoptionspage.h"
#include "qtconfigwidget.h"
#include "ui_showbuildlog.h"
#include "ui_qtversionmanager.h"
#include "ui_qtversioninfo.h"
#include "qtsupportconstants.h"
#include "qtversionmanager.h"
#include "qtversionfactory.h"

#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/variablechooser.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/buildablehelperlibrary.h>
#include <utils/hostosinfo.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>
#include <utils/algorithm.h>
#include <utils/treemodel.h>
#include <utils/utilsicons.h>

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
    explicit QtVersionItem(BaseQtVersion *version)
        : m_version(version)
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

        if (role == Qt::FontRole && m_changed) {
            QFont font;
            font.setBold(true);
            return font;
         }

        if (role == Qt::DecorationRole && column == 0)
            return m_icon;

        return QVariant();
    }

    void setIcon(const QIcon &icon)
    {
        if (m_icon.cacheKey() == icon.cacheKey())
            return;
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

    void setChanged(bool changed)
    {
        if (changed == m_changed)
            return;
        m_changed = changed;
        update();
    }

private:
    BaseQtVersion *m_version = 0;
    QIcon m_icon;
    QString m_buildLog;
    bool m_changed = false;
};

///
// QtOptionsPage
///

QtOptionsPage::QtOptionsPage()
    : m_widget(0)
{
    setId(Constants::QTVERSION_SETTINGS_PAGE_ID);
    setDisplayName(QCoreApplication::translate("QtSupport", Constants::QTVERSION_SETTINGS_PAGE_NAME));
    setCategory(ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("ProjectExplorer",
        ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_TR_CATEGORY));
    setCategoryIcon(Utils::Icon(ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_CATEGORY_ICON));
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
    , m_infoBrowser(new QTextBrowser)
    , m_invalidVersionIcon(Utils::Icons::CRITICAL.icon())
    , m_warningVersionIcon(Utils::Icons::WARNING.icon())
    , m_configurationWidget(0)
{
    QWidget *versionInfoWidget = new QWidget();
    m_versionUi->setupUi(versionInfoWidget);
    m_versionUi->editPathPushButton->setText(PathChooser::browseButtonLabel());

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

    m_autoItem = new StaticTreeItem(tr("Auto-detected"));
    m_manualItem = new StaticTreeItem(tr("Manual"));

    m_model = new TreeModel<Utils::TreeItem, Utils::TreeItem, QtVersionItem>();
    m_model->setHeader({tr("Name"), tr("qmake Location")});
    m_model->rootItem()->appendChild(m_autoItem);
    m_model->rootItem()->appendChild(m_manualItem);

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
    m_ui->qtdirList->setTextElideMode(Qt::ElideMiddle);
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
    return m_model->itemForIndexAtLevel<2>(sourceIdx);
}

void QtOptionsPageWidget::cleanUpQtVersions()
{
    QVector<QtVersionItem *> toRemove;
    QString text;

    for (TreeItem *child : *m_manualItem) {
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

    foreach (QtVersionItem *item, toRemove)
        m_model->destroyItem(item);

    updateCleanUpButton();
}

void QtOptionsPageWidget::toolChainsUpdated()
{
    m_model->forItemsAtLevel<2>([this](QtVersionItem *item) {
        if (item == currentItem())
            updateDescriptionLabel();
        else
            updateVersionItem(item);
    });
}

void QtOptionsPageWidget::qtVersionsDumpUpdated(const FileName &qmakeCommand)
{
    m_model->forItemsAtLevel<2>([this, qmakeCommand](QtVersionItem *item) {
        if (item->version()->qmakeCommand() == qmakeCommand)
            item->version()->recheckDumper();
    });

    if (currentVersion() && currentVersion()->qmakeCommand() == qmakeCommand) {
        updateWidgets();
        updateDescriptionLabel();
    }
}

void QtOptionsPageWidget::setInfoWidgetVisibility()
{
    m_ui->versionInfoWidget->setVisible(m_ui->infoWidget->state() == DetailsWidget::Collapsed);
    m_ui->infoWidget->setVisible(true);
}

void QtOptionsPageWidget::infoAnchorClicked(const QUrl &url)
{
    QDesktopServices::openUrl(url);
}

static QString formatAbiHtmlList(const QList<Abi> &abis)
{
    QString result = QStringLiteral("<ul><li>");
    for (int i = 0, count = abis.size(); i < count; ++i) {
        if (i)
            result += QStringLiteral("</li><li>");
        result += abis.at(i).toString();
    }
    result += QStringLiteral("</li></ul>");
    return result;
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
    QList<Abi> missingToolChains;
    const QList<Abi> qtAbis = version->qtAbis();

    for (const Abi &abi : qtAbis) {
        const auto abiCompatePred = [&abi] (const ToolChain *tc)
        {
            return Utils::contains(tc->supportedAbis(),
                                   [&abi](const Abi &sabi) { return sabi.isCompatibleWith(abi); });
        };

        if (!ToolChainManager::toolChain(abiCompatePred))
            missingToolChains.append(abi);
    }

    bool useable = true;
    QStringList warnings;
    if (!isNameUnique(version))
        warnings << tr("Display Name is not unique.");

    if (!missingToolChains.isEmpty()) {
        if (missingToolChains.count() == qtAbis.size()) {
            // Yes, this Qt version can't be used at all!
            info.message =
                tr("No compiler can produce code for this Qt version."
                   " Please define one or more compilers for: %1").arg(formatAbiHtmlList(qtAbis));
            info.icon = m_invalidVersionIcon;
            useable = false;
        } else {
            // Yes, some ABIs are unsupported
            warnings << tr("Not all possible target environments can be supported due to missing compilers.");
            info.toolTip = tr("The following ABIs are currently not supported: %1")
                    .arg(formatAbiHtmlList(missingToolChains));
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

    return !m_model->findItemAtLevel<2>([name, version](QtVersionItem *item) {
        BaseQtVersion *v = item->version();
        return v != version && v->displayName().trimmed() == name;
    });
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

    // Find existing items to remove/change:
    m_model->forItemsAtLevel<2>([&](QtVersionItem *item) {
        int id = item->uniqueId();
        if (removals.contains(id)) {
            toRemove.append(item);
        } else if (changes.contains(id)) {
            toAdd.append(id);
            toRemove.append(item);
        }
    });

    // Remove changed/removed items:
    foreach (QtVersionItem *item, toRemove)
        m_model->destroyItem(item);

    // Add changed/added items:
    foreach (int a, toAdd) {
        BaseQtVersion *version = QtVersionManager::version(a)->clone();
        auto *item = new QtVersionItem(version);

        // Insert in the right place:
        Utils::TreeItem *parent = version->isAutodetected()? m_autoItem : m_manualItem;
        parent->appendChild(item);
    }

    m_model->forItemsAtLevel<2>([this](QtVersionItem *item) { updateVersionItem(item); });
}

QtOptionsPageWidget::~QtOptionsPageWidget()
{
    delete m_ui;
    delete m_versionUi;
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

    m_model->destroyItem(item);

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
    if (QtVersionItem *item = currentItem()) {
        item->setVersion(version);
        item->setIcon(version->isValid()? m_validVersionIcon : m_invalidVersionIcon);
    }
    userChangedCurrentVersion();

    delete current;
}

// To be called if a Qt version was removed or added
void QtOptionsPageWidget::updateCleanUpButton()
{
    bool hasInvalidVersion = false;
    for (TreeItem *child : *m_manualItem) {
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
}

void QtOptionsPageWidget::updateDescriptionLabel()
{
    QtVersionItem *item = currentItem();
    const BaseQtVersion *version = item ? item->version() : 0;
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
            connect(m_configurationWidget, &QtConfigWidget::changed,
                    this, &QtOptionsPageWidget::updateDescriptionLabel);
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

    item->setChanged(true);
    item->version()->setUnexpandedDisplayName(m_versionUi->nameEdit->text());

    updateDescriptionLabel();
    m_model->forItemsAtLevel<2>([this](QtVersionItem *item) { updateVersionItem(item); });
}

void QtOptionsPageWidget::apply()
{
    disconnect(QtVersionManager::instance(), &QtVersionManager::qtVersionsChanged,
            this, &QtOptionsPageWidget::updateQtVersions);

    QList<BaseQtVersion *> versions;

    m_model->forItemsAtLevel<2>([this, &versions](QtVersionItem *item) {
        item->setChanged(false);
        versions.append(item->version()->clone());
    });

    QtVersionManager::setNewQtVersions(versions);


    connect(QtVersionManager::instance(), &QtVersionManager::qtVersionsChanged,
            this, &QtOptionsPageWidget::updateQtVersions);
}

} // namespace Internal
} // namespace QtSupport
