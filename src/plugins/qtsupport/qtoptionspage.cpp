// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qtoptionspage.h"

#include "qtconfigwidget.h"
#include "qtsupportconstants.h"
#include "qtsupporttr.h"
#include "qtversionmanager.h"
#include "qtversionfactory.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/dialogs/restartdialog.h>
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <projectexplorer/kitoptionspage.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>

#include <utils/algorithm.h>
#include <utils/buildablehelperlibrary.h>
#include <utils/detailswidget.h>
#include <utils/filepath.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/treemodel.h>
#include <utils/utilsicons.h>
#include <utils/variablechooser.h>

#include <QComboBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QDir>
#include <QFormLayout>
#include <QGuiApplication>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTextBrowser>
#include <QTreeView>

#include <utility>

using namespace ProjectExplorer;
using namespace Utils;

const char kInstallSettingsKey[] = "Settings/InstallSettings";

namespace QtSupport {
namespace Internal {

class QtVersionItem : public TreeItem
{
public:
    explicit QtVersionItem(QtVersion *version)
        : m_version(version)
    {}

    ~QtVersionItem()
    {
        delete m_version;
    }

    void setVersion(QtVersion *version)
    {
        m_version = version;
        update();
    }

    int uniqueId() const
    {
        return m_version ? m_version->uniqueId() : -1;
    }

    QtVersion *version() const
    {
        return m_version;
    }

    QVariant data(int column, int role) const final
    {
        if (!m_version)
            return TreeItem::data(column, role);

        if (role == Qt::DisplayRole) {
            if (column == 0)
                return m_version->displayName();
            if (column == 1)
                return m_version->qmakeFilePath().toUserOutput();
        }

        if (role == Qt::FontRole && m_changed) {
            QFont font;
            font.setBold(true);
            return font;
         }

        if (role == Qt::DecorationRole && column == 0)
            return m_icon;

        if (role == Qt::ToolTipRole) {
            const QString row = "<dt style=\"font-weight:bold\">%1:</dt>"
                                "<dd>%2</dd>";
            return QString("<dl style=\"white-space:pre\">"
                         + row.arg(Tr::tr("Qt Version"), m_version->qtVersionString())
                         + row.arg(Tr::tr("Location of qmake"), m_version->qmakeFilePath().toUserOutput())
                         + "</dl>");
        }

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
    QtVersion *m_version = nullptr;
    QIcon m_icon;
    QString m_buildLog;
    bool m_changed = false;
};

// QtOptionsPageWidget

class QtOptionsPageWidget : public Core::IOptionsPageWidget
{
public:
    QtOptionsPageWidget();
    ~QtOptionsPageWidget();

    static void linkWithQt();

private:
    void apply() final;

    void updateDescriptionLabel();
    void userChangedCurrentVersion();
    void updateWidgets();
    void setupLinkWithQtButton();
    QtVersion *currentVersion() const;
    QtVersionItem *currentItem() const;

    void updateQtVersions(const QList<int> &, const QList<int> &, const QList<int> &);
    void versionChanged(const QModelIndex &current, const QModelIndex &previous);
    void addQtDir();
    void removeQtDir();
    void editPath();
    void updateCleanUpButton();
    void updateCurrentQtName();

    void cleanUpQtVersions();
    void toolChainsUpdated();

    void setInfoWidgetVisibility();
    void infoAnchorClicked(const QUrl &);

    struct ValidityInfo {
        QString description;
        QString message;
        QString toolTip;
        QIcon icon;
    };
    ValidityInfo validInformation(const QtVersion *version);
    QList<ProjectExplorer::ToolChain*> toolChains(const QtVersion *version);
    QByteArray defaultToolChainId(const QtVersion *version);

    bool isNameUnique(const QtVersion *version);
    void updateVersionItem(QtVersionItem *item);

    TreeModel<TreeItem, TreeItem, QtVersionItem> *m_model;
    KitSettingsSortModel *m_filterModel;
    TreeItem *m_autoItem;
    TreeItem *m_manualItem;

    const QString m_specifyNameString;

    QTreeView *m_qtdirList;
    DetailsWidget *m_versionInfoWidget;
    DetailsWidget *m_infoWidget;
    QComboBox *m_documentationSetting;
    QPushButton *m_delButton;
    QPushButton *m_linkWithQtButton;
    QPushButton *m_cleanUpButton;

    QTextBrowser *m_infoBrowser;
    QIcon m_invalidVersionIcon;
    QIcon m_warningVersionIcon;
    QIcon m_validVersionIcon;
    QtConfigWidget *m_configurationWidget;

    QLineEdit *m_nameEdit;
    QLabel *m_qmakePath;
    QPushButton *m_editPathPushButton;
    QLabel *m_errorLabel;
    QFormLayout *m_formLayout;
};

QtOptionsPageWidget::QtOptionsPageWidget()
    : m_specifyNameString(Tr::tr("<specify a name>"))
    , m_infoBrowser(new QTextBrowser)
    , m_invalidVersionIcon(Utils::Icons::CRITICAL.icon())
    , m_warningVersionIcon(Utils::Icons::WARNING.icon())
    , m_configurationWidget(nullptr)
{
    m_qtdirList = new QTreeView(this);
    m_qtdirList->setObjectName("qtDirList");
    m_qtdirList->setUniformRowHeights(true);

    m_versionInfoWidget = new DetailsWidget(this);

    m_infoWidget = new DetailsWidget(this);

    m_documentationSetting = new QComboBox(this);

    auto addButton = new QPushButton(Tr::tr("Add..."));
    m_delButton = new QPushButton(Tr::tr("Remove"));
    m_linkWithQtButton = new QPushButton(Tr::tr("Link with Qt..."));
    m_cleanUpButton = new QPushButton(Tr::tr("Clean Up"));

    m_nameEdit = new QLineEdit;

    m_qmakePath = new QLabel;
    m_qmakePath->setObjectName("qmakePath"); // for Squish
    m_qmakePath->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_qmakePath->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::TextSelectableByMouse);

    m_editPathPushButton = new QPushButton(Tr::tr("Edit"));
    m_editPathPushButton->setText(PathChooser::browseButtonLabel());

    m_errorLabel = new QLabel;

    using namespace Layouting;

    auto versionInfoWidget = new QWidget;
    // clang-format off
    Form {
        Tr::tr("Name:"), m_nameEdit, br,
        Tr::tr("qmake path:"), Row { m_qmakePath, m_editPathPushButton }, br,
        Span(2, m_errorLabel),
        noMargin
    }.attachTo(versionInfoWidget);
    // clang-format on

    m_formLayout = qobject_cast<QFormLayout*>(versionInfoWidget->layout());

    // clang-format off
    Row {
        Column {
            m_qtdirList,
            m_versionInfoWidget,
            m_infoWidget,
            Row { Tr::tr("Register documentation:"), m_documentationSetting, st }
        },

        Column {
            addButton,
            m_delButton,
            Space(20),
            m_linkWithQtButton,
            m_cleanUpButton,
            st,
        }
    }.attachTo(this);
    // clang-format on

    setupLinkWithQtButton();

    m_infoBrowser->setOpenLinks(false);
    m_infoBrowser->setTextInteractionFlags(Qt::TextBrowserInteraction);
    connect(m_infoBrowser, &QTextBrowser::anchorClicked,
            this, &QtOptionsPageWidget::infoAnchorClicked);
    m_infoWidget->setWidget(m_infoBrowser);
    connect(m_infoWidget, &DetailsWidget::expanded,
            this, &QtOptionsPageWidget::setInfoWidgetVisibility);

    m_versionInfoWidget->setWidget(versionInfoWidget);
    m_versionInfoWidget->setState(DetailsWidget::NoSummary);

    m_autoItem = new StaticTreeItem({ProjectExplorer::Constants::msgAutoDetected()},
                                    {ProjectExplorer::Constants::msgAutoDetectedToolTip()});
    m_manualItem = new StaticTreeItem(ProjectExplorer::Constants::msgManual());

    m_model = new TreeModel<TreeItem, TreeItem, QtVersionItem>();
    m_model->setHeader({Tr::tr("Name"), Tr::tr("qmake Path")});
    m_model->rootItem()->appendChild(m_autoItem);
    m_model->rootItem()->appendChild(m_manualItem);

    m_filterModel = new KitSettingsSortModel(this);
    m_filterModel->setSortedCategories({ProjectExplorer::Constants::msgAutoDetected(),
                                        ProjectExplorer::Constants::msgManual()});
    m_filterModel->setSourceModel(m_model);

    m_qtdirList->setModel(m_filterModel);
    m_qtdirList->setSortingEnabled(true);

    m_qtdirList->setFirstColumnSpanned(0, QModelIndex(), true);
    m_qtdirList->setFirstColumnSpanned(1, QModelIndex(), true);

    m_qtdirList->header()->setStretchLastSection(false);
    m_qtdirList->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_qtdirList->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_qtdirList->setTextElideMode(Qt::ElideMiddle);
    m_qtdirList->sortByColumn(0, Qt::AscendingOrder);

    m_documentationSetting->addItem(Tr::tr("Highest Version Only"),
                                        int(QtVersionManager::DocumentationSetting::HighestOnly));
    m_documentationSetting->addItem(Tr::tr("All"), int(QtVersionManager::DocumentationSetting::All));
    m_documentationSetting->addItem(Tr::tr("None"),
                                        int(QtVersionManager::DocumentationSetting::None));
    const int selectedIndex = m_documentationSetting->findData(
        int(QtVersionManager::documentationSetting()));
    if (selectedIndex >= 0)
        m_documentationSetting->setCurrentIndex(selectedIndex);

    QList<int> additions = transform(QtVersionManager::versions(), &QtVersion::uniqueId);

    updateQtVersions(additions, QList<int>(), QList<int>());

    m_qtdirList->expandAll();

    connect(m_nameEdit, &QLineEdit::textEdited,
            this, &QtOptionsPageWidget::updateCurrentQtName);

    connect(m_editPathPushButton, &QAbstractButton::clicked,
            this, &QtOptionsPageWidget::editPath);

    connect(addButton, &QAbstractButton::clicked,
            this, &QtOptionsPageWidget::addQtDir);
    connect(m_delButton, &QAbstractButton::clicked,
            this, &QtOptionsPageWidget::removeQtDir);

    connect(m_qtdirList->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &QtOptionsPageWidget::versionChanged);

    connect(m_cleanUpButton, &QAbstractButton::clicked,
            this, &QtOptionsPageWidget::cleanUpQtVersions);
    userChangedCurrentVersion();
    updateCleanUpButton();

    connect(QtVersionManager::instance(), &QtVersionManager::qtVersionsChanged,
            this, &QtOptionsPageWidget::updateQtVersions);

    connect(ProjectExplorer::ToolChainManager::instance(), &ToolChainManager::toolChainsChanged,
            this, &QtOptionsPageWidget::toolChainsUpdated);

    auto chooser = new VariableChooser(this);
    chooser->addSupportedWidget(m_nameEdit, "Qt:Name");
    chooser->addMacroExpanderProvider([this] {
        QtVersion *version = currentVersion();
        return version ? version->macroExpander() : nullptr;
    });
}

QtVersion *QtOptionsPageWidget::currentVersion() const
{
    QtVersionItem *item = currentItem();
    if (!item)
        return nullptr;
    return item->version();
}

QtVersionItem *QtOptionsPageWidget::currentItem() const
{
    QModelIndex idx = m_qtdirList->selectionModel()->currentIndex();
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

    if (QMessageBox::warning(nullptr, Tr::tr("Remove Invalid Qt Versions"),
                             Tr::tr("Do you want to remove all invalid Qt Versions?<br>"
                                    "<ul><li>%1</li></ul><br>"
                                    "will be removed.").arg(text),
                             QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
        return;

    for (QtVersionItem *item : std::as_const(toRemove))
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

void QtOptionsPageWidget::setInfoWidgetVisibility()
{
    bool isExpanded = m_infoWidget->state() == DetailsWidget::Expanded;
    if (isExpanded && m_infoBrowser->toPlainText().isEmpty()) {
        QtVersionItem *item = currentItem();
        const QtVersion *version = item ? item->version() : nullptr;
        if (version)
            m_infoBrowser->setHtml(version->toHtml(true));
    }

    m_versionInfoWidget->setVisible(!isExpanded);
    m_infoWidget->setVisible(true);
}

void QtOptionsPageWidget::infoAnchorClicked(const QUrl &url)
{
    QDesktopServices::openUrl(url);
}

static QString formatAbiHtmlList(const Abis &abis)
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

QtOptionsPageWidget::ValidityInfo QtOptionsPageWidget::validInformation(const QtVersion *version)
{
    ValidityInfo info;
    info.icon = m_validVersionIcon;

    if (!version)
        return info;

    info.description = Tr::tr("Qt version %1 for %2").arg(version->qtVersionString(), version->description());
    if (!version->isValid()) {
        info.icon = m_invalidVersionIcon;
        info.message = version->invalidReason();
        return info;
    }

    // Do we have tool chain issues?
    Abis missingToolChains;
    const Abis qtAbis = version->qtAbis();

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
        warnings << Tr::tr("Display Name is not unique.");

    if (!missingToolChains.isEmpty()) {
        if (missingToolChains.count() == qtAbis.size()) {
            // Yes, this Qt version can't be used at all!
            info.message =
                Tr::tr("No compiler can produce code for this Qt version."
                       " Please define one or more compilers for: %1").arg(formatAbiHtmlList(qtAbis));
            info.icon = m_invalidVersionIcon;
            useable = false;
        } else {
            // Yes, some ABIs are unsupported
            warnings << Tr::tr("Not all possible target environments can be supported due to missing compilers.");
            info.toolTip = Tr::tr("The following ABIs are currently not supported: %1")
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

QList<ToolChain*> QtOptionsPageWidget::toolChains(const QtVersion *version)
{
    QList<ToolChain*> toolChains;
    if (!version)
        return toolChains;

    QSet<QByteArray> ids;
    const Abis abis = version->qtAbis();
    for (const Abi &a : abis) {
        const Toolchains tcList = ToolChainManager::findToolChains(a);
        for (ToolChain *tc : tcList) {
            if (Utils::insert(ids, tc->id()))
                toolChains.append(tc);
        }
    }

    return toolChains;
}

QByteArray QtOptionsPageWidget::defaultToolChainId(const QtVersion *version)
{
    QList<ToolChain*> possibleToolChains = toolChains(version);
    if (!possibleToolChains.isEmpty())
        return possibleToolChains.first()->id();
    return QByteArray();
}

bool QtOptionsPageWidget::isNameUnique(const QtVersion *version)
{
    const QString name = version->displayName().trimmed();

    return !m_model->findItemAtLevel<2>([name, version](QtVersionItem *item) {
        QtVersion *v = item->version();
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
    for (QtVersionItem *item : std::as_const(toRemove))
        m_model->destroyItem(item);

    // Add changed/added items:
    for (int a : std::as_const(toAdd)) {
        QtVersion *version = QtVersionManager::version(a)->clone(true);
        auto *item = new QtVersionItem(version);

        // Insert in the right place:
        TreeItem *parent = version->isAutodetected()? m_autoItem : m_manualItem;
        parent->appendChild(item);
    }

    m_model->forItemsAtLevel<2>([this](QtVersionItem *item) { updateVersionItem(item); });
}

QtOptionsPageWidget::~QtOptionsPageWidget()
{
    delete m_configurationWidget;
}

void QtOptionsPageWidget::addQtDir()
{
    FilePath qtVersion
        = FileUtils::getOpenFilePath(this,
                                     Tr::tr("Select a qmake Executable"),
                                     {},
                                     BuildableHelperLibrary::filterForQmakeFileDialog(),
                                     nullptr,
                                     QFileDialog::DontResolveSymlinks,
                                     true);

    if (qtVersion.isEmpty())
        return;

    // should add all qt versions here ?
    if (BuildableHelperLibrary::isQtChooser(qtVersion))
        qtVersion = BuildableHelperLibrary::qtChooserToQmakePath(qtVersion.symLinkTarget());

    auto checkAlreadyExists = [qtVersion](TreeItem *parent) -> QPair<bool, QString> {
        for (int i = 0; i < parent->childCount(); ++i) {
            auto item = static_cast<QtVersionItem *>(parent->childAt(i));
            if (item->version()->qmakeFilePath() == qtVersion) {
                return {true, item->version()->displayName()};
            }
        }
        return {false, {}};
    };

    bool alreadyExists;
    QString otherName;
    std::tie(alreadyExists, otherName) = checkAlreadyExists(m_autoItem);
    if (!alreadyExists)
        std::tie(alreadyExists, otherName) = checkAlreadyExists(m_manualItem);

    if (alreadyExists) {
        // Already exist
        QMessageBox::warning(this, Tr::tr("Qt Version Already Known"),
                             Tr::tr("This Qt version was already registered as \"%1\".")
                             .arg(otherName));
        return;
    }

    QString error;
    QtVersion *version = QtVersionFactory::createQtVersionFromQMakePath(qtVersion, false, QString(), &error);
    if (version) {
        auto item = new QtVersionItem(version);
        item->setIcon(version->isValid()? m_validVersionIcon : m_invalidVersionIcon);
        m_manualItem->appendChild(item);
        QModelIndex source = m_model->indexForItem(item);
        m_qtdirList->setCurrentIndex(m_filterModel->mapFromSource(source)); // should update the rest of the ui
        m_nameEdit->setFocus();
        m_nameEdit->selectAll();
    } else {
        QMessageBox::warning(this, Tr::tr("Qmake Not Executable"),
                             Tr::tr("The qmake executable %1 could not be added: %2").arg(qtVersion.toUserOutput()).arg(error));
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
    QtVersion *current = currentVersion();
    FilePath qtVersion =
            FileUtils::getOpenFilePath(this,
                                       Tr::tr("Select a qmake Executable"),
                                       current->qmakeFilePath().absolutePath(),
                                       BuildableHelperLibrary::filterForQmakeFileDialog(),
                                       nullptr,
                                       QFileDialog::DontResolveSymlinks);
    if (qtVersion.isEmpty())
        return;
    QtVersion *version = QtVersionFactory::createQtVersionFromQMakePath(qtVersion);
    if (!version)
        return;
    // Same type? then replace!
    if (current->type() != version->type()) {
        // not the same type, error out
        QMessageBox::critical(this, Tr::tr("Incompatible Qt Versions"),
                              Tr::tr("The Qt version selected must match the device type."),
                              QMessageBox::Ok);
        delete version;
        return;
    }
    // same type, replace
    version->setId(current->uniqueId());
    if (current->unexpandedDisplayName() != current->defaultUnexpandedDisplayName())
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

    m_cleanUpButton->setEnabled(hasInvalidVersion);
}

void QtOptionsPageWidget::userChangedCurrentVersion()
{
    updateWidgets();
    updateDescriptionLabel();
}

void QtOptionsPageWidget::updateDescriptionLabel()
{
    QtVersionItem *item = currentItem();
    const QtVersion *version = item ? item->version() : nullptr;
    const ValidityInfo info = validInformation(version);
    if (info.message.isEmpty()) {
        m_errorLabel->setVisible(false);
    } else {
        m_errorLabel->setVisible(true);
        m_errorLabel->setText(info.message);
        m_errorLabel->setToolTip(info.toolTip);
    }
    m_infoWidget->setSummaryText(info.description);
    if (item)
        item->setIcon(info.icon);

    m_infoBrowser->clear();
    if (version) {
        setInfoWidgetVisibility();
    } else {
        m_versionInfoWidget->setVisible(false);
        m_infoWidget->setVisible(false);
    }
}

void QtOptionsPageWidget::versionChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(current)
    Q_UNUSED(previous)
    userChangedCurrentVersion();
}

void QtOptionsPageWidget::updateWidgets()
{
    delete m_configurationWidget;
    m_configurationWidget = nullptr;
    QtVersion *version = currentVersion();
    if (version) {
        m_nameEdit->setText(version->unexpandedDisplayName());
        m_qmakePath->setText(version->qmakeFilePath().toUserOutput());
        m_configurationWidget = version->createConfigurationWidget();
        if (m_configurationWidget) {
            m_formLayout->addRow(m_configurationWidget);
            m_configurationWidget->setEnabled(!version->isAutodetected());
            connect(m_configurationWidget, &QtConfigWidget::changed,
                    this, &QtOptionsPageWidget::updateDescriptionLabel);
        }
    } else {
        m_nameEdit->clear();
        m_qmakePath->clear();
    }

    const bool enabled = version != nullptr;
    const bool isAutodetected = enabled && version->isAutodetected();
    m_delButton->setEnabled(enabled && !isAutodetected);
    m_nameEdit->setEnabled(enabled);
    m_editPathPushButton->setEnabled(enabled && !isAutodetected);
}

static QString settingsFile(const QString &baseDir)
{
    return baseDir + (baseDir.isEmpty() ? "" : "/") + QCoreApplication::organizationName() + '/'
           + QCoreApplication::applicationName() + ".ini";
}

static QString qtVersionsFile(const QString &baseDir)
{
    return baseDir + (baseDir.isEmpty() ? "" : "/") + QCoreApplication::organizationName() + '/'
           + QCoreApplication::applicationName() + '/' + "qtversion.xml";
}

static std::optional<FilePath> currentlyLinkedQtDir(bool *hasInstallSettings)
{
    const QString installSettingsFilePath = settingsFile(Core::ICore::resourcePath().toString());
    const bool installSettingsExist = QFileInfo::exists(installSettingsFilePath);
    if (hasInstallSettings)
        *hasInstallSettings = installSettingsExist;
    if (installSettingsExist) {
        const QVariant value = QSettings(installSettingsFilePath, QSettings::IniFormat)
                                   .value(kInstallSettingsKey);
        if (value.isValid())
            return FilePath::fromSettings(value);
    }
    return {};
}

static QString linkingPurposeText()
{
    return Tr::tr(
        "Linking with a Qt installation automatically registers Qt versions and kits, and other "
        "tools that were installed with that Qt installer, in this Qt Creator installation. Other "
        "Qt Creator installations are not affected.");
}

static bool canLinkWithQt(QString *toolTip)
{
    bool canLink = true;
    bool installSettingsExist;
    const std::optional<FilePath> installSettingsValue = currentlyLinkedQtDir(
        &installSettingsExist);
    QStringList tip;
    tip << linkingPurposeText();
    if (!Core::ICore::resourcePath().isWritableDir()) {
        canLink = false;
        tip << Tr::tr("%1's resource directory is not writable.")
                   .arg(QGuiApplication::applicationDisplayName());
    }
    const FilePath link = installSettingsValue ? *installSettingsValue : FilePath();
    if (!link.isEmpty())
        tip << Tr::tr("%1 is currently linked to \"%2\".")
                   .arg(QGuiApplication::applicationDisplayName(), link.toUserOutput());
    if (toolTip)
        *toolTip = tip.join("\n\n");
    return canLink;
}

void QtOptionsPageWidget::setupLinkWithQtButton()
{
    QString tip;
    const bool canLink = canLinkWithQt(&tip);
    m_linkWithQtButton->setEnabled(canLink);
    m_linkWithQtButton->setToolTip(tip);
    connect(m_linkWithQtButton, &QPushButton::clicked, this, &LinkWithQtSupport::linkWithQt);
}

void QtOptionsPageWidget::updateCurrentQtName()
{
    QtVersionItem *item = currentItem();
    if (!item || !item->version())
        return;

    item->setChanged(true);
    item->version()->setUnexpandedDisplayName(m_nameEdit->text());

    updateDescriptionLabel();
    m_model->forItemsAtLevel<2>([this](QtVersionItem *item) { updateVersionItem(item); });
}

void QtOptionsPageWidget::apply()
{
    disconnect(QtVersionManager::instance(),
               &QtVersionManager::qtVersionsChanged,
               this,
               &QtOptionsPageWidget::updateQtVersions);

    QtVersionManager::setDocumentationSetting(
        QtVersionManager::DocumentationSetting(m_documentationSetting->currentData().toInt()));

    QtVersions versions;
    m_model->forItemsAtLevel<2>([&versions](QtVersionItem *item) {
        item->setChanged(false);
        versions.append(item->version()->clone());
    });
    QtVersionManager::setNewQtVersions(versions);

    connect(QtVersionManager::instance(),
            &QtVersionManager::qtVersionsChanged,
            this,
            &QtOptionsPageWidget::updateQtVersions);
}

const QStringList kSubdirsToCheck = {"",
                                     "Tools/sdktool", // macOS
                                     "Tools/sdktool/share/qtcreator", // Windows/Linux
                                     "Qt Creator.app/Contents/Resources",
                                     "Contents/Resources",
                                     "Tools/QtCreator/share/qtcreator",
                                     "share/qtcreator"};

static QStringList settingsFilesToCheck()
{
    return Utils::transform(kSubdirsToCheck, [](const QString &dir) { return settingsFile(dir); });
}

static QStringList qtversionFilesToCheck()
{
    return Utils::transform(kSubdirsToCheck, [](const QString &dir) { return qtVersionsFile(dir); });
}

static std::optional<FilePath> settingsDirForQtDir(const FilePath &baseDirectory,
                                                   const FilePath &qtDir)
{
    const FilePaths dirsToCheck = Utils::transform(kSubdirsToCheck, [qtDir](const QString &dir) {
        return qtDir / dir;
    });
    const FilePath validDir = Utils::findOrDefault(dirsToCheck, [baseDirectory](const FilePath &dir) {
        return QFileInfo::exists(settingsFile(baseDirectory.resolvePath(dir).toString()))
               || QFileInfo::exists(qtVersionsFile(baseDirectory.resolvePath(dir).toString()));
    });
    if (!validDir.isEmpty())
        return validDir;
    return {};
}

static FancyLineEdit::AsyncValidationResult validateQtInstallDir(const QString &input,
                                                                 const FilePath &baseDirectory)
{
    const FilePath qtDir = FilePath::fromUserInput(input);
    if (!settingsDirForQtDir(baseDirectory, qtDir)) {
        const QStringList filesToCheck = settingsFilesToCheck() + qtversionFilesToCheck();
        return make_unexpected(
            "<html><body>"
            + ::QtSupport::Tr::tr("Qt installation information was not found in \"%1\". "
                                  "Choose a directory that contains one of the files %2")
                  .arg(qtDir.toUserOutput(), "<pre>" + filesToCheck.join('\n') + "</pre>"));
    }
    return input;
}

static FilePath defaultQtInstallationPath()
{
    if (HostOsInfo::isWindowsHost())
        return FilePath::fromString({"C:/Qt"});
    return FileUtils::homePath() / "Qt";
}

void QtOptionsPageWidget::linkWithQt()
{
    const QString title = Tr::tr("Choose Qt Installation");
    const QString restartText = Tr::tr("The change will take effect after restart.");
    bool askForRestart = false;
    QDialog dialog(Core::ICore::dialogParent());
    dialog.setWindowTitle(title);
    auto tipLabel = new QLabel(linkingPurposeText());
    tipLabel->setWordWrap(true);
    auto pathLabel = new QLabel(Tr::tr("Qt installation path:"));
    pathLabel->setToolTip(
        Tr::tr("Choose the Qt installation directory, or a directory that contains \"%1\".")
            .arg(settingsFile("")));
    auto pathInput = new PathChooser;
    pathInput->setExpectedKind(PathChooser::ExistingDirectory);
    pathInput->setBaseDirectory(FilePath::fromString(QCoreApplication::applicationDirPath()));
    pathInput->setPromptDialogTitle(title);
    pathInput->setMacroExpander(nullptr);
    pathInput->setValidationFunction(
        [pathInput](const QString &input) -> FancyLineEdit::AsyncValidationFuture {
            return pathInput->defaultValidationFunction()(input).then(
                [baseDir = pathInput->baseDirectory()](
                    const FancyLineEdit::AsyncValidationResult &result)
                    -> FancyLineEdit::AsyncValidationResult {
                    if (!result)
                        return result;
                    return validateQtInstallDir(result.value(), baseDir);
                });
        });
    const std::optional<FilePath> currentLink = currentlyLinkedQtDir(nullptr);
    pathInput->setFilePath(currentLink ? *currentLink : defaultQtInstallationPath());
    pathInput->setAllowPathFromDevice(true);
    auto buttons = new QDialogButtonBox;

    using namespace Layouting;
    Column {
        tipLabel,
        Form {
            Tr::tr("Qt installation path:"), pathInput, br,
        },
        st,
        buttons,
    }.attachTo(&dialog);

    auto linkButton = buttons->addButton(Tr::tr("Link with Qt"), QDialogButtonBox::AcceptRole);
    connect(linkButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    auto cancelButton = buttons->addButton(Tr::tr("Cancel"), QDialogButtonBox::RejectRole);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
    auto unlinkButton = buttons->addButton(Tr::tr("Remove Link"), QDialogButtonBox::DestructiveRole);
    unlinkButton->setEnabled(currentLink.has_value());
    connect(unlinkButton, &QPushButton::clicked, &dialog, [&dialog, &askForRestart] {
        bool removeSettingsFile = false;
        const QString filePath = settingsFile(Core::ICore::resourcePath().toString());
        {
            QSettings installSettings(filePath, QSettings::IniFormat);
            installSettings.remove(kInstallSettingsKey);
            if (installSettings.allKeys().isEmpty())
                removeSettingsFile = true;
        }
        if (removeSettingsFile)
            QFile::remove(filePath);
        askForRestart = true;
        dialog.reject();
    });
    connect(pathInput, &PathChooser::validChanged, linkButton, &QPushButton::setEnabled);
    linkButton->setEnabled(pathInput->isValid());

    dialog.setMinimumWidth(520);
    dialog.exec();
    if (dialog.result() == QDialog::Accepted) {
        const std::optional<FilePath> settingsDir = settingsDirForQtDir(pathInput->baseDirectory(),
                                                                        pathInput->rawFilePath());
        if (QTC_GUARD(settingsDir)) {
            const QString settingsFilePath = settingsFile(Core::ICore::resourcePath().toString());
            QSettings settings(settingsFilePath, QSettings::IniFormat);
            settings.setValue(kInstallSettingsKey, settingsDir->toVariant());
            settings.sync();
            if (settings.status() == QSettings::AccessError) {
                QMessageBox::critical(Core::ICore::dialogParent(),
                                      Tr::tr("Error Linking With Qt"),
                                      Tr::tr("Could not write to \"%1\".").arg(settingsFilePath));
                return;
            }

            askForRestart = true;
        }
    }
    if (askForRestart) {
        Core::RestartDialog restartDialog(Core::ICore::dialogParent(), restartText);
        restartDialog.exec();
    }
}

// QtOptionsPage

QtOptionsPage::QtOptionsPage()
{
    setId(Constants::QTVERSION_SETTINGS_PAGE_ID);
    setDisplayName(Tr::tr("Qt Versions"));
    setCategory(ProjectExplorer::Constants::KITS_SETTINGS_CATEGORY);
    setWidgetCreator([] { return new QtOptionsPageWidget; });
}

QStringList QtOptionsPage::keywords() const
{
    return {
        Tr::tr("Add..."),
        Tr::tr("Remove"),
        Tr::tr("Clean Up"),
        Tr::tr("Link with Qt"),
        Tr::tr("Remove Link"),
        Tr::tr("Qt installation path:"),
        Tr::tr("qmake path:"),
        Tr::tr("Register documentation:")
    };
}

} // Internal

bool LinkWithQtSupport::canLinkWithQt()
{
    return Internal::canLinkWithQt(nullptr);
}

bool LinkWithQtSupport::isLinkedWithQt()
{
    return Internal::currentlyLinkedQtDir(nullptr).has_value();
}

Utils::FilePath LinkWithQtSupport::linkedQt()
{
    return Internal::currentlyLinkedQtDir(nullptr).value_or(Utils::FilePath());
}

void LinkWithQtSupport::linkWithQt()
{
    Internal::QtOptionsPageWidget::linkWithQt();
}

} // QtSupport
