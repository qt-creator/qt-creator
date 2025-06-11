// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qtoptionspage.h"

#include "qtconfigwidget.h"
#include "qtsupportconstants.h"
#include "qtsupporttr.h"
#include "qtversionmanager.h"
#include "qtversionfactory.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <projectexplorer/kitaspect.h>
#include <projectexplorer/kitoptionspage.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>

#include <utils/algorithm.h>
#include <utils/buildablehelperlibrary.h>
#include <utils/detailswidget.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
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

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

const char kInstallSettingsKey[] = "Settings/InstallSettings";

namespace QtSupport {
namespace Internal {

static const QIcon &invalidVersionIcon()
{
    static const QIcon theIcon(Utils::Icons::CRITICAL.icon());
    return theIcon;
}

static const QIcon &warningVersionIcon()
{
    static const QIcon theIcon(Utils::Icons::WARNING.icon());
    return theIcon;
}

static const QIcon &validVersionIcon()
{
    static const QIcon theIcon;
    return theIcon;
}

QString nonUniqueDisplayNameWarning()
{
    return Tr::tr("Display Name is not unique.");
}

struct UnsupportedAbisInfo
{
    enum class Status { Ok, SomeMissing, AllMissing } status = Status::Ok;
    QString message;
};

UnsupportedAbisInfo checkForUnsupportedAbis(const QtVersion *version)
{
    Abis missingToolchains;
    const Abis qtAbis = version->qtAbis();

    for (const Abi &abi : qtAbis) {
        const auto abiComparePred = [&abi] (const Toolchain *tc) {
            return Utils::contains(tc->supportedAbis(),
                                   [&abi](const Abi &sabi) { return sabi.isCompatibleWith(abi); });
        };

        if (!ToolchainManager::toolchain(abiComparePred))
            missingToolchains.append(abi);
    }

    UnsupportedAbisInfo info;
    if (!missingToolchains.isEmpty()) {
        const auto formatAbiHtmlList = [](const Abis &abis) {
            QString result = QStringLiteral("<ul><li>");
            for (int i = 0, count = abis.size(); i < count; ++i) {
                if (i)
                    result += QStringLiteral("</li><li>");
                result += abis.at(i).toString();
            }
            result += QStringLiteral("</li></ul>");
            return result;
        };

        if (missingToolchains.count() == qtAbis.size()) {
            info.status = UnsupportedAbisInfo::Status::AllMissing;
            info.message =
                Tr::tr("No compiler can produce code for this Qt version."
                       " Please define one or more compilers for: %1").arg(formatAbiHtmlList(qtAbis));
        } else {
            info.status = UnsupportedAbisInfo::Status::SomeMissing;
            info.message = Tr::tr("The following ABIs are currently not supported: %1")
                               .arg(formatAbiHtmlList(missingToolchains));
        }
    }

    return info;
}

void QtVersionItem::setIsNameUnique(const std::function<bool(QtVersion *)> &isNameUnique)
{
    m_isNameUnique = isNameUnique;
}

QtVersionItem::Quality QtVersionItem::quality() const
{
    const QtVersion *version = this->version();
    QTC_ASSERT(version, return Quality::Bad);

    if (!version->isValid())
        return Quality::Bad;
    if (!version->warningReason().isEmpty() || hasNonUniqueDisplayName())
        return Quality::Limited;
    const UnsupportedAbisInfo abisInfo = checkForUnsupportedAbis(version);
    switch (abisInfo.status) {
    case UnsupportedAbisInfo::Status::AllMissing:
        return Quality::Bad;
    case UnsupportedAbisInfo::Status::SomeMissing:
        return Quality::Limited;
    case UnsupportedAbisInfo::Status::Ok:
        break;
    }
    return Quality::Good;
}

void QtVersionItem::setChanged(bool changed)
{
    if (changed == m_changed)
        return;
    m_changed = changed;
    update();
}

QVariant QtVersionItem::data(int column, int role) const
{
    const QtVersion *version = this->version();

    if (!version) {
        if (role == KitAspect::IsNoneRole && column == 0)
            return true;
        if (role == Qt::DisplayRole && column == 0)
            return Tr::tr("None", "No Qt version");
        if (role == KitAspect::IdRole)
            return -1;
        return TreeItem::data(column, role);
    }

    if (role == Qt::DisplayRole) {
        if (column == 0)
            return version->displayName();
        if (column == 1)
            return version->qmakeFilePath().toUserOutput();
    }

    if (role == Qt::FontRole && m_changed) {
        QFont font;
        font.setBold(true);
        return font;
    }

    if (role == Qt::DecorationRole && column == 0) {
        switch (quality()) {
        case Quality::Good:
            return validVersionIcon();
        case Quality::Limited:
            return warningVersionIcon();
        case Quality::Bad:
            return invalidVersionIcon();
        }
    }

    if (role == Qt::ToolTipRole) {
        const QString row = "<dt style=\"font-weight:bold\">%1:</dt>"
                            "<dd>%2</dd>";
        QString desc = "<dl style=\"white-space:pre\">";
        if (version->isValid())
            desc += row.arg(Tr::tr("Qt Version"), version->qtVersionString());
        desc += row.arg(Tr::tr("Location of qmake"), version->qmakeFilePath().toUserOutput());
        if (version->isValid()) {
            const UnsupportedAbisInfo abisInfo = checkForUnsupportedAbis(version);
            if (abisInfo.status == UnsupportedAbisInfo::Status::AllMissing)
                desc += row.arg(Tr::tr("Error"), abisInfo.message);
            if (abisInfo.status == UnsupportedAbisInfo::Status::SomeMissing)
                desc += row.arg(Tr::tr("Warning"), abisInfo.message);
            const QStringList warnings = version->warningReason();
            for (const QString &w : warnings)
                desc += row.arg(Tr::tr("Warning"), w);
        } else {
            desc += row.arg(Tr::tr("Error"), version->invalidReason());
        }
        if (hasNonUniqueDisplayName())
            desc += row.arg(Tr::tr("Warning"), nonUniqueDisplayNameWarning());
        desc += "</dl>";
        return desc;
    }

    if (role == KitAspect::IdRole)
        return uniqueId();

    if (role == KitAspect::QualityRole)
        return int(quality());

    return QVariant();
}

int QtVersionItem::uniqueId() const
{
    if (const auto v = std::get_if<QtVersion *>(&m_version))
        return *v ? (*v)->uniqueId() : -1;
    return *std::get_if<int>(&m_version);
}

QtVersion *QtVersionItem::version() const
{
    if (const auto v = std::get_if<QtVersion *>(&m_version))
        return *v;
    return QtVersionManager::version(*std::get_if<int>(&m_version));
}

void QtVersionItem::setVersion(QtVersion *version)
{
    m_version = version;
    update();
}

QtVersionItem::~QtVersionItem()
{
    if (const auto v = std::get_if<QtVersion *>(&m_version))
        delete *v;
}

// QtSettingsPageWidget

class QtSettingsPageWidget final : public IOptionsPageWidget
{
public:
    QtSettingsPageWidget();
    ~QtSettingsPageWidget();

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
    };
    ValidityInfo validInformation(const QtVersion *version);
    QList<ProjectExplorer::Toolchain*> toolChains(const QtVersion *version);
    QByteArray defaultToolchainId(const QtVersion *version);

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
    QtConfigWidget *m_configurationWidget;

    QLineEdit *m_nameEdit;
    QLabel *m_qmakePath;
    QPushButton *m_editPathPushButton;
    QLabel *m_errorLabel;
    QFormLayout *m_formLayout;
};

QtSettingsPageWidget::QtSettingsPageWidget()
    : m_specifyNameString(Tr::tr("<specify a name>"))
    , m_infoBrowser(new QTextBrowser)
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
            this, &QtSettingsPageWidget::infoAnchorClicked);
    m_infoWidget->setWidget(m_infoBrowser);
    connect(m_infoWidget, &DetailsWidget::expanded,
            this, &QtSettingsPageWidget::setInfoWidgetVisibility);

    m_versionInfoWidget->setWidget(versionInfoWidget);
    m_versionInfoWidget->setState(DetailsWidget::NoSummary);

    m_autoItem = new StaticTreeItem({ProjectExplorer::Constants::msgAutoDetected()},
                                    {ProjectExplorer::Constants::msgAutoDetectedToolTip()});
    m_manualItem = new StaticTreeItem(ProjectExplorer::Constants::msgManual());

    m_model = new TreeModel<TreeItem, TreeItem, QtVersionItem>(this);
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
    m_documentationSetting->addItem(
        Tr::tr("All", "All documentation"), int(QtVersionManager::DocumentationSetting::All));
    m_documentationSetting->addItem(
        Tr::tr("None", "No documentation"), int(QtVersionManager::DocumentationSetting::None));
    const int selectedIndex = m_documentationSetting->findData(
        int(QtVersionManager::documentationSetting()));
    if (selectedIndex >= 0)
        m_documentationSetting->setCurrentIndex(selectedIndex);

    QList<int> additions = transform(QtVersionManager::versions(), &QtVersion::uniqueId);

    updateQtVersions(additions, QList<int>(), QList<int>());

    m_qtdirList->expandAll();

    connect(m_nameEdit, &QLineEdit::textEdited,
            this, &QtSettingsPageWidget::updateCurrentQtName);

    connect(m_editPathPushButton, &QAbstractButton::clicked,
            this, &QtSettingsPageWidget::editPath);

    connect(addButton, &QAbstractButton::clicked,
            this, &QtSettingsPageWidget::addQtDir);
    connect(m_delButton, &QAbstractButton::clicked,
            this, &QtSettingsPageWidget::removeQtDir);

    connect(m_qtdirList->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &QtSettingsPageWidget::versionChanged);

    connect(m_cleanUpButton, &QAbstractButton::clicked,
            this, &QtSettingsPageWidget::cleanUpQtVersions);
    userChangedCurrentVersion();
    updateCleanUpButton();

    connect(QtVersionManager::instance(), &QtVersionManager::qtVersionsChanged,
            this, &QtSettingsPageWidget::updateQtVersions);

    connect(ProjectExplorer::ToolchainManager::instance(), &ToolchainManager::toolchainsChanged,
            this, &QtSettingsPageWidget::toolChainsUpdated);

    auto chooser = new VariableChooser(this);
    chooser->addSupportedWidget(m_nameEdit, "Qt:Name");
    chooser->addMacroExpanderProvider([this] {
        QtVersion *version = currentVersion();
        return version ? version->macroExpander() : nullptr;
    });
}

QtVersion *QtSettingsPageWidget::currentVersion() const
{
    QtVersionItem *item = currentItem();
    if (!item)
        return nullptr;
    return item->version();
}

QtVersionItem *QtSettingsPageWidget::currentItem() const
{
    QModelIndex idx = m_qtdirList->selectionModel()->currentIndex();
    QModelIndex sourceIdx = m_filterModel->mapToSource(idx);
    return m_model->itemForIndexAtLevel<2>(sourceIdx);
}

void QtSettingsPageWidget::cleanUpQtVersions()
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

void QtSettingsPageWidget::toolChainsUpdated()
{
    m_model->forItemsAtLevel<2>([this](QtVersionItem *item) {
        if (item == currentItem())
            updateDescriptionLabel();
        else
            updateVersionItem(item);
    });
}

void QtSettingsPageWidget::setInfoWidgetVisibility()
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

void QtSettingsPageWidget::infoAnchorClicked(const QUrl &url)
{
    QDesktopServices::openUrl(url);
}

QtSettingsPageWidget::ValidityInfo QtSettingsPageWidget::validInformation(const QtVersion *version)
{
    ValidityInfo info;

    if (!version)
        return info;

    info.description = Tr::tr("Qt version %1 for %2").arg(version->qtVersionString(), version->description());
    if (!version->isValid()) {
        info.message = version->invalidReason();
        return info;
    }

    bool useable = true;
    QStringList warnings;
    if (!isNameUnique(version))
        warnings << nonUniqueDisplayNameWarning();

    const UnsupportedAbisInfo unsupportedAbisInfo = checkForUnsupportedAbis(version);
    if (unsupportedAbisInfo.status == UnsupportedAbisInfo::Status::AllMissing) {
        info.message = unsupportedAbisInfo.message;
        useable = false;
    } else if (unsupportedAbisInfo.status == UnsupportedAbisInfo::Status::SomeMissing) {
        warnings << Tr::tr(
            "Not all possible target environments can be supported due to missing compilers.");
        info.toolTip = unsupportedAbisInfo.message;
    }

    if (useable) {
        warnings += version->warningReason();
        if (!warnings.isEmpty())
            info.message = warnings.join(QLatin1Char('\n'));
    }

    return info;
}

QList<Toolchain*> QtSettingsPageWidget::toolChains(const QtVersion *version)
{
    QList<Toolchain*> toolChains;
    if (!version)
        return toolChains;

    QSet<QByteArray> ids;
    const Abis abis = version->qtAbis();
    for (const Abi &a : abis) {
        const Toolchains tcList = ToolchainManager::findToolchains(a);
        for (Toolchain *tc : tcList) {
            if (Utils::insert(ids, tc->id()))
                toolChains.append(tc);
        }
    }

    return toolChains;
}

QByteArray QtSettingsPageWidget::defaultToolchainId(const QtVersion *version)
{
    QList<Toolchain*> possibleToolChains = toolChains(version);
    if (!possibleToolChains.isEmpty())
        return possibleToolChains.first()->id();
    return QByteArray();
}

bool QtSettingsPageWidget::isNameUnique(const QtVersion *version)
{
    const QString name = version->displayName().trimmed();

    return !m_model->findItemAtLevel<2>([name, version](QtVersionItem *item) {
        QtVersion *v = item->version();
        return v != version && v->displayName().trimmed() == name;
    });
}

void QtSettingsPageWidget::updateVersionItem(QtVersionItem *item)
{
    if (item)
        item->update();
}

void QtSettingsPageWidget::updateQtVersions(const QList<int> &additions, const QList<int> &removals,
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
        QtVersion *version = QtVersionManager::version(a)->clone();
        auto *item = new QtVersionItem(version);
        item->setIsNameUnique([this](QtVersion *v) { return isNameUnique(v); });

        // Insert in the right place:
        TreeItem *parent = version->isAutodetected()? m_autoItem : m_manualItem;
        parent->appendChild(item);
    }

    m_model->forItemsAtLevel<2>([this](QtVersionItem *item) { updateVersionItem(item); });
}

QtSettingsPageWidget::~QtSettingsPageWidget()
{
    delete m_configurationWidget;
}

void QtSettingsPageWidget::addQtDir()
{
    FilePath qtVersion
        = FileUtils::getOpenFilePath(Tr::tr("Select a qmake Executable"),
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
        item->setIsNameUnique([this](QtVersion *v) { return isNameUnique(v); });
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

void QtSettingsPageWidget::removeQtDir()
{
    QtVersionItem *item = currentItem();
    if (!item)
        return;

    m_model->destroyItem(item);

    updateCleanUpButton();
}

void QtSettingsPageWidget::editPath()
{
    QtVersion *current = currentVersion();
    FilePath qtVersion =
            FileUtils::getOpenFilePath(Tr::tr("Select a qmake Executable"),
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
    if (QtVersionItem *item = currentItem())
        item->setVersion(version);

    userChangedCurrentVersion();

    delete current;
}

// To be called if a Qt version was removed or added
void QtSettingsPageWidget::updateCleanUpButton()
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

void QtSettingsPageWidget::userChangedCurrentVersion()
{
    updateWidgets();
    updateDescriptionLabel();
}

void QtSettingsPageWidget::updateDescriptionLabel()
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
        item->update();

    m_infoBrowser->clear();
    if (version) {
        setInfoWidgetVisibility();
    } else {
        m_versionInfoWidget->setVisible(false);
        m_infoWidget->setVisible(false);
    }
}

void QtSettingsPageWidget::versionChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(current)
    Q_UNUSED(previous)
    userChangedCurrentVersion();
}

void QtSettingsPageWidget::updateWidgets()
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
                    this, &QtSettingsPageWidget::updateDescriptionLabel);
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

static FilePath settingsFile(const QString &baseDir)
{
    return FilePath::fromString(baseDir + (baseDir.isEmpty() ? "" : "/")
                                + QCoreApplication::organizationName() + '/'
                                + QCoreApplication::applicationName() + ".ini");
}

static FilePath qtVersionsFile(const QString &baseDir)
{
    return FilePath::fromString(baseDir + (baseDir.isEmpty() ? "" : "/")
                                + QCoreApplication::organizationName() + '/'
                                + QCoreApplication::applicationName() + '/' + "qtversion.xml");
}

static std::optional<FilePath> currentlyLinkedQtDir(bool *hasInstallSettings)
{
    const FilePath installSettingsFilePath = settingsFile(ICore::resourcePath().path());
    const bool installSettingsExist = installSettingsFilePath.exists();
    if (hasInstallSettings)
        *hasInstallSettings = installSettingsExist;
    if (installSettingsExist) {
        const QVariant value = QSettings(installSettingsFilePath.toFSPathString(), QSettings::IniFormat)
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
        "tools that were installed with that Qt installer, in this %1 installation. Other %1 "
        "installations are not affected.").arg(QGuiApplication::applicationDisplayName());
}

static bool canLinkWithQt(QString *toolTip)
{
    bool canLink = true;
    bool installSettingsExist;
    const std::optional<FilePath> installSettingsValue = currentlyLinkedQtDir(
        &installSettingsExist);
    QStringList tip;
    tip << linkingPurposeText();
    if (!ICore::resourcePath().isWritableDir()) {
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

void QtSettingsPageWidget::setupLinkWithQtButton()
{
    QString tip;
    const bool canLink = canLinkWithQt(&tip);
    m_linkWithQtButton->setEnabled(canLink);
    m_linkWithQtButton->setToolTip(tip);
    connect(m_linkWithQtButton, &QPushButton::clicked, this, &LinkWithQtSupport::linkWithQt);
}

void QtSettingsPageWidget::updateCurrentQtName()
{
    QtVersionItem *item = currentItem();
    if (!item || !item->version())
        return;

    item->setChanged(true);
    item->version()->setUnexpandedDisplayName(m_nameEdit->text());

    updateDescriptionLabel();
    m_model->forItemsAtLevel<2>([this](QtVersionItem *item) { updateVersionItem(item); });
}

void QtSettingsPageWidget::apply()
{
    disconnect(QtVersionManager::instance(),
               &QtVersionManager::qtVersionsChanged,
               this,
               &QtSettingsPageWidget::updateQtVersions);

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
            &QtSettingsPageWidget::updateQtVersions);
}

const QStringList kSubdirsToCheck = {"",
                                     "Tools/sdktool", // macOS
                                     "Tools/sdktool/share/qtcreator", // Windows/Linux
                                     "Qt Creator.app/Contents/Resources",
                                     "Contents/Resources",
                                     "Tools/QtCreator/share/qtcreator",
                                     "share/qtcreator"};

static FilePaths settingsFilesToCheck()
{
    return Utils::transform(kSubdirsToCheck, [](const QString &dir) { return settingsFile(dir); });
}

static FilePaths qtversionFilesToCheck()
{
    return Utils::transform(kSubdirsToCheck, [](const QString &dir) { return qtVersionsFile(dir); });
}

static FilePath settingsDirForQtDir(const FilePath &baseDirectory, const FilePath &qtDir)
{
    const FilePaths dirsToCheck = Utils::transform(kSubdirsToCheck, [qtDir](const QString &dir) {
        return qtDir / dir;
    });
    return Utils::findOrDefault(dirsToCheck, [baseDirectory](const FilePath &dir) {
        return settingsFile(baseDirectory.resolvePath(dir).path()).exists()
            || qtVersionsFile(baseDirectory.resolvePath(dir).path()).exists();
    });
}

static FancyLineEdit::AsyncValidationResult validateQtInstallDir(const QString &input,
                                                                 const FilePath &baseDirectory)
{
    const FilePath qtDir = FilePath::fromUserInput(input);
    if (!settingsDirForQtDir(baseDirectory, qtDir).isEmpty()) {
        const FilePaths filesToCheck = settingsFilesToCheck() + qtversionFilesToCheck();
        return make_unexpected(
            "<html><body>"
            + ::QtSupport::Tr::tr("Qt installation information was not found in \"%1\". "
                                  "Choose a directory that contains one of the files %2")
                  .arg(qtDir.toUserOutput(), "<pre>" +
                            FilePath::formatFilePaths(filesToCheck, "\n") + "</pre>"));
    }
    return input;
}

static FilePath defaultQtInstallationPath()
{
    if (HostOsInfo::isWindowsHost())
        return FilePath::fromString({"C:/Qt"});
    return FileUtils::homePath() / "Qt";
}

void QtSettingsPageWidget::linkWithQt()
{
    const QString title = Tr::tr("Choose Qt Installation");
    const QString restartText = Tr::tr("The change will take effect after restart.");
    bool askForRestart = false;
    QDialog dialog(ICore::dialogParent());
    dialog.setWindowTitle(title);
    auto tipLabel = new QLabel(linkingPurposeText());
    tipLabel->setWordWrap(true);
    auto pathLabel = new QLabel(Tr::tr("Qt installation path:"));
    pathLabel->setToolTip(
        Tr::tr("Choose the Qt installation directory, or a directory that contains \"%1\".")
            .arg(settingsFile("").toUserOutput()));
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
        const FilePath filePath = settingsFile(ICore::resourcePath().path());
        {
            QSettings installSettings(filePath.toFSPathString(), QSettings::IniFormat);
            installSettings.remove(kInstallSettingsKey);
            if (installSettings.allKeys().isEmpty())
                removeSettingsFile = true;
        }
        if (removeSettingsFile)
            filePath.removeFile();
        askForRestart = true;
        dialog.reject();
    });
    connect(pathInput, &PathChooser::validChanged, linkButton, &QPushButton::setEnabled);
    linkButton->setEnabled(pathInput->isValid());

    dialog.setMinimumWidth(520);
    dialog.exec();
    if (dialog.result() == QDialog::Accepted) {
        const FilePath settingsDir = settingsDirForQtDir(pathInput->baseDirectory(),
                                                         pathInput->unexpandedFilePath());
        if (QTC_GUARD(!settingsDir.isEmpty())) {
            const FilePath settingsFilePath = settingsFile(ICore::resourcePath().path());
            QSettings settings(settingsFilePath.toFSPathString(), QSettings::IniFormat);
            settings.setValue(kInstallSettingsKey, settingsDir.toVariant());
            settings.sync();
            if (settings.status() == QSettings::AccessError) {
                QMessageBox::critical(ICore::dialogParent(),
                                      Tr::tr("Error Linking With Qt"),
                                      Tr::tr("Could not write to \"%1\".")
                                        .arg(settingsFilePath.toUserOutput()));
                return;
            }

            askForRestart = true;
        }
    }
    if (askForRestart)
        ICore::askForRestart(restartText);
}

// QtSettingsPage

class QtSettingsPage final : public IOptionsPage
{
public:
    QtSettingsPage()
    {
        setId(Constants::QTVERSION_SETTINGS_PAGE_ID);
        setDisplayName(Tr::tr("Qt Versions"));
        setCategory(ProjectExplorer::Constants::KITS_SETTINGS_CATEGORY);
        setWidgetCreator([] { return new QtSettingsPageWidget; });
    }

    QStringList keywords() const final
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
};

void setupQtSettingsPage()
{
    static QtSettingsPage theQtSettingsPage;
}

} // namespace Internal

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
    Internal::QtSettingsPageWidget::linkWithQt();
}

} // QtSupport
