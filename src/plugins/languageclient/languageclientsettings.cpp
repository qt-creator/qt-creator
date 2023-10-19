// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "languageclientsettings.h"

#include "client.h"
#include "languageclient_global.h"
#include "languageclientinterface.h"
#include "languageclientmanager.h"
#include "languageclienttr.h"

#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <texteditor/plaintexteditorfactory.h>
#include <texteditor/textmark.h>

#include <utils/algorithm.h>
#include <utils/delegates.h>
#include <utils/fancylineedit.h>
#include <utils/jsontreeitem.h>
#include <utils/macroexpander.h>
#include <utils/stringutils.h>
#include <utils/utilsicons.h>
#include <utils/variablechooser.h>

#include <QBoxLayout>
#include <QComboBox>
#include <QCompleter>
#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QGroupBox>
#include <QHeaderView>
#include <QJsonDocument>
#include <QLabel>
#include <QListView>
#include <QMenu>
#include <QMimeData>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStringListModel>
#include <QToolButton>
#include <QTreeView>

constexpr char typeIdKey[] = "typeId";
constexpr char nameKey[] = "name";
constexpr char idKey[] = "id";
constexpr char enabledKey[] = "enabled";
constexpr char startupBehaviorKey[] = "startupBehavior";
constexpr char mimeTypeKey[] = "mimeType";
constexpr char filePatternKey[] = "filePattern";
constexpr char initializationOptionsKey[] = "initializationOptions";
constexpr char configurationKey[] = "configuration";
constexpr char executableKey[] = "executable";
constexpr char argumentsKey[] = "arguments";
constexpr char settingsGroupKey[] = "LanguageClient";
constexpr char clientsKey[] = "clients";
constexpr char typedClientsKey[] = "typedClients";
constexpr char outlineSortedKey[] = "outlineSorted";
constexpr char mimeType[] = "application/language.client.setting";

using namespace Utils;

namespace LanguageClient {

class LanguageClientSettingsModel : public QAbstractListModel
{
public:
    LanguageClientSettingsModel() = default;
    ~LanguageClientSettingsModel() override;

    // QAbstractItemModel interface
    int rowCount(const QModelIndex &/*parent*/ = QModelIndex()) const final { return m_settings.count(); }
    QVariant data(const QModelIndex &index, int role) const final;
    bool removeRows(int row, int count = 1, const QModelIndex &parent = QModelIndex()) final;
    bool insertRows(int row, int count = 1, const QModelIndex &parent = QModelIndex()) final;
    bool setData(const QModelIndex &index, const QVariant &value, int role) final;
    Qt::ItemFlags flags(const QModelIndex &index) const final;
    Qt::DropActions supportedDropActions() const override { return Qt::MoveAction; }
    QStringList mimeTypes() const override { return {mimeType}; }
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool dropMimeData(const QMimeData *data,
                      Qt::DropAction action,
                      int row,
                      int column,
                      const QModelIndex &parent) override;

    void reset(const QList<BaseSettings *> &settings);
    QList<BaseSettings *> settings() const { return m_settings; }
    int insertSettings(BaseSettings *settings);
    void enableSetting(const QString &id, bool enable = true);
    QList<BaseSettings *> removed() const { return m_removed; }
    BaseSettings *settingForIndex(const QModelIndex &index) const;
    QModelIndex indexForSetting(BaseSettings *setting) const;

private:
    static constexpr int idRole = Qt::UserRole + 1;
    QList<BaseSettings *> m_settings; // owned
    QList<BaseSettings *> m_removed;
};

class LanguageClientSettingsPageWidget : public Core::IOptionsPageWidget
{
public:
    LanguageClientSettingsPageWidget(LanguageClientSettingsModel &settings,
                                     QSet<QString> &changedSettings);

    void currentChanged(const QModelIndex &index);
    int currentRow() const;
    void resetCurrentSettings(int row);
    void applyCurrentSettings();

    void apply() final
    {
        applyCurrentSettings();
        LanguageClientManager::applySettings();

        for (BaseSettings *setting : m_settings.removed()) {
            for (Client *client : LanguageClientManager::clientsForSetting(setting))
                LanguageClientManager::shutdownClient(client);
        }

        int row = currentRow();
        m_settings.reset(LanguageClientManager::currentSettings());
        resetCurrentSettings(row);
    }
    void finish()
    {
        m_settings.reset(LanguageClientManager::currentSettings());
        m_changedSettings.clear();
    }

private:
    QTreeView *m_view = nullptr;
    struct CurrentSettings {
        BaseSettings *setting = nullptr;
        QWidget *widget = nullptr;
    } m_currentSettings;

    void addItem(const Utils::Id &clientTypeId);
    void deleteItem();

    LanguageClientSettingsModel &m_settings;
    QSet<QString> &m_changedSettings;
};

QMap<Utils::Id, ClientType> &clientTypes()
{
    static QMap<Utils::Id, ClientType> types;
    return types;
}

LanguageClientSettingsPageWidget::LanguageClientSettingsPageWidget(LanguageClientSettingsModel &settings,
                                                                   QSet<QString> &changedSettings)
    : m_view(new QTreeView())
    , m_settings(settings)
    , m_changedSettings(changedSettings)
{
    auto mainLayout = new QVBoxLayout();
    auto layout = new QHBoxLayout();
    m_view->setModel(&m_settings);
    m_view->setHeaderHidden(true);
    m_view->setSelectionMode(QAbstractItemView::SingleSelection);
    m_view->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_view->setDragEnabled(true);
    m_view->viewport()->setAcceptDrops(true);
    m_view->setDropIndicatorShown(true);
    m_view->setDragDropMode(QAbstractItemView::InternalMove);
    connect(m_view->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &LanguageClientSettingsPageWidget::currentChanged);
    auto buttonLayout = new QVBoxLayout();
    auto addButton = new QPushButton(Tr::tr("&Add"));
    auto addMenu = new QMenu;
    addMenu->clear();
    for (const ClientType &type : clientTypes()) {
        auto action = new QAction(type.name);
        connect(action, &QAction::triggered, this, [this, id = type.id]() { addItem(id); });
        addMenu->addAction(action);
    }
    addButton->setMenu(addMenu);
    auto deleteButton = new QPushButton(Tr::tr("&Delete"));
    connect(deleteButton, &QPushButton::pressed, this, &LanguageClientSettingsPageWidget::deleteItem);
    mainLayout->addLayout(layout);
    setLayout(mainLayout);
    layout->addWidget(m_view);
    layout->addLayout(buttonLayout);
    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(deleteButton);
    buttonLayout->addStretch(10);
}

void LanguageClientSettingsPageWidget::currentChanged(const QModelIndex &index)
{
    if (m_currentSettings.widget) {
        applyCurrentSettings();
        layout()->removeWidget(m_currentSettings.widget);
        delete m_currentSettings.widget;
    }

    if (index.isValid()) {
        m_currentSettings.setting = m_settings.settingForIndex(index);
        m_currentSettings.widget = m_currentSettings.setting->createSettingsWidget(this);
        layout()->addWidget(m_currentSettings.widget);
    } else {
        m_currentSettings.setting = nullptr;
        m_currentSettings.widget = nullptr;
    }
}

int LanguageClientSettingsPageWidget::currentRow() const
{
    return m_settings.indexForSetting(m_currentSettings.setting).row();
}

void LanguageClientSettingsPageWidget::resetCurrentSettings(int row)
{
    if (m_currentSettings.widget) {
        layout()->removeWidget(m_currentSettings.widget);
        delete m_currentSettings.widget;
    }

    m_currentSettings.setting = nullptr;
    m_currentSettings.widget = nullptr;
    m_view->setCurrentIndex(m_settings.index(row));
}

void LanguageClientSettingsPageWidget::applyCurrentSettings()
{
    if (!m_currentSettings.setting)
        return;

    if (m_currentSettings.setting->applyFromSettingsWidget(m_currentSettings.widget)) {
        auto index = m_settings.indexForSetting(m_currentSettings.setting);
        emit m_settings.dataChanged(index, index);
    }
}

BaseSettings *generateSettings(const Utils::Id &clientTypeId)
{
    if (auto generator = clientTypes().value(clientTypeId).generator) {
        auto settings = generator();
        settings->m_settingsTypeId = clientTypeId;
        return settings;
    }
    return nullptr;
}

void LanguageClientSettingsPageWidget::addItem(const Utils::Id &clientTypeId)
{
    auto newSettings = generateSettings(clientTypeId);
    QTC_ASSERT(newSettings, return);
    m_view->setCurrentIndex(m_settings.index(m_settings.insertSettings(newSettings)));
}

void LanguageClientSettingsPageWidget::deleteItem()
{
    auto index = m_view->currentIndex();
    if (!index.isValid())
        return;

    m_settings.removeRows(index.row());
}

class LanguageClientSettingsPage : public Core::IOptionsPage
{
public:
    LanguageClientSettingsPage();

    void init();

    QList<BaseSettings *> settings() const;
    QList<BaseSettings *> changedSettings() const;
    void addSettings(BaseSettings *settings);
    void enableSettings(const QString &id, bool enable = true);

private:
    LanguageClientSettingsModel m_model;
    QSet<QString> m_changedSettings;
};

LanguageClientSettingsPage::LanguageClientSettingsPage()
{
    setId(Constants::LANGUAGECLIENT_SETTINGS_PAGE);
    setDisplayName(Tr::tr("General"));
    setCategory(Constants::LANGUAGECLIENT_SETTINGS_CATEGORY);
    setDisplayCategory(Tr::tr(Constants::LANGUAGECLIENT_SETTINGS_TR));
    setCategoryIconPath(":/languageclient/images/settingscategory_languageclient.png");
    setWidgetCreator([this] { return new LanguageClientSettingsPageWidget(m_model, m_changedSettings); });
    QObject::connect(&m_model, &LanguageClientSettingsModel::dataChanged, [this](const QModelIndex &index) {
        if (BaseSettings *setting = m_model.settingForIndex(index))
            m_changedSettings << setting->m_id;
    });
}

void LanguageClientSettingsPage::init()
{
    m_model.reset(LanguageClientSettings::fromSettings(Core::ICore::settings()));
}

QList<BaseSettings *> LanguageClientSettingsPage::settings() const
{
    return m_model.settings();
}

QList<BaseSettings *> LanguageClientSettingsPage::changedSettings() const
{
    QList<BaseSettings *> result;
    const QList<BaseSettings *> &all = settings();
    for (BaseSettings *setting : all) {
        if (m_changedSettings.contains(setting->m_id))
            result << setting;
    }
    return result;
}

void LanguageClientSettingsPage::addSettings(BaseSettings *settings)
{
    m_model.insertSettings(settings);
    m_changedSettings << settings->m_id;
}

void LanguageClientSettingsPage::enableSettings(const QString &id, bool enable)
{
    m_model.enableSetting(id, enable);
}

LanguageClientSettingsModel::~LanguageClientSettingsModel()
{
    qDeleteAll(m_settings);
}

QVariant LanguageClientSettingsModel::data(const QModelIndex &index, int role) const
{
    BaseSettings *setting = settingForIndex(index);
    if (!setting)
        return QVariant();
    if (role == Qt::DisplayRole)
        return Utils::globalMacroExpander()->expand(setting->m_name);
    else if (role == Qt::CheckStateRole)
        return setting->m_enabled ? Qt::Checked : Qt::Unchecked;
    else if (role == idRole)
        return setting->m_id;
    return QVariant();
}

bool LanguageClientSettingsModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (row >= int(m_settings.size()))
        return false;
    const int end = qMin(row + count - 1, int(m_settings.size()) - 1);
    beginRemoveRows(parent, row, end);
    for (auto i = end; i >= row; --i)
        m_removed << m_settings.takeAt(i);
    endRemoveRows();
    return true;
}

bool LanguageClientSettingsModel::insertRows(int row, int count, const QModelIndex &parent)
{
    if (row > m_settings.size() || row < 0)
        return false;
    beginInsertRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i)
        m_settings.insert(row + i, new StdIOSettings());
    endInsertRows();
    return true;
}

bool LanguageClientSettingsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    BaseSettings *setting = settingForIndex(index);
    if (!setting || role != Qt::CheckStateRole)
        return false;

    if (setting->m_enabled != value.toBool()) {
        setting->m_enabled = !setting->m_enabled;
        emit dataChanged(index, index, { Qt::CheckStateRole });
    }
    return true;
}

Qt::ItemFlags LanguageClientSettingsModel::flags(const QModelIndex &index) const
{
    const Qt::ItemFlags dragndropFlags = index.isValid() ? Qt::ItemIsDragEnabled
                                                         : Qt::ItemIsDropEnabled;
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | dragndropFlags;
}

QMimeData *LanguageClientSettingsModel::mimeData(const QModelIndexList &indexes) const
{
    QTC_ASSERT(indexes.count() == 1, return nullptr);

    QMimeData *mimeData = new QMimeData;
    QByteArray encodedData;

    QDataStream stream(&encodedData, QIODevice::WriteOnly);

    for (const QModelIndex &index : indexes) {
        if (index.isValid())
            stream << data(index, idRole).toString();
    }

    mimeData->setData(mimeType, indexes.first().data(idRole).toString().toUtf8());
    return mimeData;
}

bool LanguageClientSettingsModel::dropMimeData(
    const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (!canDropMimeData(data, action, row, column, parent))
        return false;

    if (action == Qt::IgnoreAction)
        return true;

    const QString id = QString::fromUtf8(data->data(mimeType));
    auto setting = Utils::findOrDefault(m_settings, [id](const BaseSettings *setting) {
        return setting->m_id == id;
    });
    if (!setting)
        return false;

    if (row == -1)
        row = parent.isValid() ? parent.row() : rowCount(QModelIndex());

    beginInsertRows(parent, row, row);
    m_settings.insert(row, setting->copy());
    endInsertRows();

    return true;
}

void LanguageClientSettingsModel::reset(const QList<BaseSettings *> &settings)
{
    beginResetModel();
    qDeleteAll(m_settings);
    qDeleteAll(m_removed);
    m_removed.clear();
    m_settings = Utils::transform(settings, [](const BaseSettings *other) { return other->copy(); });
    endResetModel();
}

int LanguageClientSettingsModel::insertSettings(BaseSettings *settings)
{
    int row = rowCount();
    beginInsertRows(QModelIndex(), row, row);
    m_settings.insert(row, settings);
    endInsertRows();
    return row;
}

void LanguageClientSettingsModel::enableSetting(const QString &id, bool enable)
{
    BaseSettings *setting = Utils::findOrDefault(m_settings, Utils::equal(&BaseSettings::m_id, id));
    if (!setting)
        return;
    if (setting->m_enabled == enable)
        return;
    setting->m_enabled = enable;
    const QModelIndex &index = indexForSetting(setting);
    if (index.isValid())
        emit dataChanged(index, index, {Qt::CheckStateRole});
}

BaseSettings *LanguageClientSettingsModel::settingForIndex(const QModelIndex &index) const
{
    if (!index.isValid() || index.row() >= m_settings.size())
        return nullptr;
    return m_settings[index.row()];
}

QModelIndex LanguageClientSettingsModel::indexForSetting(BaseSettings *setting) const
{
    const int index = m_settings.indexOf(setting);
    return index < 0 ? QModelIndex() : createIndex(index, 0, setting);
}

QJsonObject BaseSettings::initializationOptions() const
{
    return QJsonDocument::fromJson(Utils::globalMacroExpander()->
                                   expand(m_initializationOptions).toUtf8()).object();
}

QJsonValue BaseSettings::configuration() const
{
    const QJsonDocument document = QJsonDocument::fromJson(m_configuration.toUtf8());
    if (document.isArray())
        return document.array();
    if (document.isObject())
        return document.object();
    return {};
}

bool BaseSettings::applyFromSettingsWidget(QWidget *widget)
{
    bool changed = false;
    if (auto settingsWidget = qobject_cast<BaseSettingsWidget *>(widget)) {
        if (m_name != settingsWidget->name()) {
            m_name = settingsWidget->name();
            changed = true;
        }
        if (m_languageFilter != settingsWidget->filter()) {
            m_languageFilter = settingsWidget->filter();
            changed = true;
        }
        if (m_startBehavior != settingsWidget->startupBehavior()) {
            m_startBehavior = settingsWidget->startupBehavior();
            changed = true;
        }
        if (m_initializationOptions != settingsWidget->initializationOptions()) {
            m_initializationOptions = settingsWidget->initializationOptions();
            changed = true;
        }
    }
    return changed;
}

QWidget *BaseSettings::createSettingsWidget(QWidget *parent) const
{
    return new BaseSettingsWidget(this, parent);
}

bool BaseSettings::isValid() const
{
    return !m_name.isEmpty();
}

Client *BaseSettings::createClient() const
{
    return createClient(static_cast<ProjectExplorer::Project *>(nullptr));
}

Client *BaseSettings::createClient(ProjectExplorer::Project *project) const
{
    if (!isValid() || !m_enabled)
        return nullptr;
    BaseClientInterface *interface = createInterface(project);
    QTC_ASSERT(interface, return nullptr);
    auto *client = createClient(interface);
    client->setName(Utils::globalMacroExpander()->expand(m_name));
    client->setSupportedLanguage(m_languageFilter);
    client->setInitializationOptions(initializationOptions());
    client->setActivateDocumentAutomatically(true);
    client->setCurrentProject(project);
    client->updateConfiguration(m_configuration);
    return client;
}

BaseClientInterface *BaseSettings::createInterface(ProjectExplorer::Project *) const
{
    return nullptr;
}

Client *BaseSettings::createClient(BaseClientInterface *interface) const
{
    return new Client(interface);
}

Store BaseSettings::toMap() const
{
    Store map;
    map.insert(typeIdKey, m_settingsTypeId.toSetting());
    map.insert(nameKey, m_name);
    map.insert(idKey, m_id);
    map.insert(enabledKey, m_enabled);
    map.insert(startupBehaviorKey, m_startBehavior);
    map.insert(mimeTypeKey, m_languageFilter.mimeTypes);
    map.insert(filePatternKey, m_languageFilter.filePattern);
    map.insert(initializationOptionsKey, m_initializationOptions);
    map.insert(configurationKey, m_configuration);
    return map;
}

void BaseSettings::fromMap(const Store &map)
{
    m_name = map[nameKey].toString();
    m_id = map.value(idKey, QUuid::createUuid().toString()).toString();
    m_enabled = map[enabledKey].toBool();
    m_startBehavior = BaseSettings::StartBehavior(
        map.value(startupBehaviorKey, BaseSettings::RequiresFile).toInt());
    m_languageFilter.mimeTypes = map[mimeTypeKey].toStringList();
    m_languageFilter.filePattern = map[filePatternKey].toStringList();
    m_languageFilter.filePattern.removeAll(QString()); // remove empty entries
    m_initializationOptions = map[initializationOptionsKey].toString();
    m_configuration = map[configurationKey].toString();
}

static LanguageClientSettingsPage &settingsPage()
{
    static LanguageClientSettingsPage settingsPage;
    return settingsPage;
}

void LanguageClientSettings::init()
{
    settingsPage().init();
    LanguageClientManager::applySettings();
}

QList<BaseSettings *> LanguageClientSettings::fromSettings(QtcSettings *settingsIn)
{
    settingsIn->beginGroup(settingsGroupKey);
    QList<BaseSettings *> result;

    for (const QVariantList &varList :
         {settingsIn->value(clientsKey).toList(), settingsIn->value(typedClientsKey).toList()}) {
        for (const QVariant &var : varList) {
            const Store map = storeFromVariant(var);
            Id typeId = Id::fromSetting(map.value(typeIdKey));
            if (!typeId.isValid())
                typeId = Constants::LANGUAGECLIENT_STDIO_SETTINGS_ID;
            if (BaseSettings *settings = generateSettings(typeId)) {
                settings->fromMap(map);
                result << settings;
            }
        }
    }

    settingsIn->endGroup();
    return result;
}

QList<BaseSettings *> LanguageClientSettings::pageSettings()
{
    return settingsPage().settings();
}

QList<BaseSettings *> LanguageClientSettings::changedSettings()
{
    return settingsPage().changedSettings();
}

void LanguageClientSettings::registerClientType(const ClientType &type)
{
    QTC_ASSERT(!clientTypes().contains(type.id), return);
    clientTypes()[type.id] = type;
}

void LanguageClientSettings::addSettings(BaseSettings *settings)
{
    settingsPage().addSettings(settings);
}

void LanguageClientSettings::enableSettings(const QString &id, bool enable)
{
    settingsPage().enableSettings(id, enable);
}

void LanguageClientSettings::toSettings(QtcSettings *settings,
                                        const QList<BaseSettings *> &languageClientSettings)
{
    settings->beginGroup(settingsGroupKey);
    auto transform = [](const QList<BaseSettings *> &settings) {
        return Utils::transform(settings, [](const BaseSettings *setting) {
            return variantFromStore(setting->toMap());
        });
    };
    auto isStdioSetting = Utils::equal(&BaseSettings::m_settingsTypeId,
                                       Utils::Id(Constants::LANGUAGECLIENT_STDIO_SETTINGS_ID));
    auto [stdioSettings, typedSettings] = Utils::partition(languageClientSettings, isStdioSetting);
    settings->setValue(clientsKey, transform(stdioSettings));
    settings->setValue(typedClientsKey, transform(typedSettings));
    settings->endGroup();
}

bool LanguageClientSettings::outlineComboBoxIsSorted()
{
    auto settings = Core::ICore::settings();
    settings->beginGroup(settingsGroupKey);
    bool sorted = settings->value(outlineSortedKey).toBool();
    settings->endGroup();
    return sorted;
}

void LanguageClientSettings::setOutlineComboBoxSorted(bool sorted)
{
    auto settings = Core::ICore::settings();
    settings->beginGroup(settingsGroupKey);
    settings->setValue(outlineSortedKey, sorted);
    settings->endGroup();
}

bool StdIOSettings::applyFromSettingsWidget(QWidget *widget)
{
    bool changed = false;
    if (auto settingsWidget = qobject_cast<StdIOSettingsWidget *>(widget)) {
        changed = BaseSettings::applyFromSettingsWidget(settingsWidget);
        if (m_executable != settingsWidget->executable()) {
            m_executable = settingsWidget->executable();
            changed = true;
        }
        if (m_arguments != settingsWidget->arguments()) {
            m_arguments = settingsWidget->arguments();
            changed = true;
        }
    }
    return changed;
}

QWidget *StdIOSettings::createSettingsWidget(QWidget *parent) const
{
    return new StdIOSettingsWidget(this, parent);
}

bool StdIOSettings::isValid() const
{
    return BaseSettings::isValid() && !m_executable.isEmpty();
}

Store StdIOSettings::toMap() const
{
    Store map = BaseSettings::toMap();
    map.insert(executableKey, m_executable.toSettings());
    map.insert(argumentsKey, m_arguments);
    return map;
}

void StdIOSettings::fromMap(const Store &map)
{
    BaseSettings::fromMap(map);
    m_executable = Utils::FilePath::fromSettings(map[executableKey]);
    m_arguments = map[argumentsKey].toString();
}

QString StdIOSettings::arguments() const
{
    return Utils::globalMacroExpander()->expand(m_arguments);
}

Utils::CommandLine StdIOSettings::command() const
{
    return Utils::CommandLine(m_executable, arguments(), Utils::CommandLine::Raw);
}

BaseClientInterface *StdIOSettings::createInterface(ProjectExplorer::Project *project) const
{
    auto interface = new StdIOClientInterface;
    interface->setCommandLine(command());
    if (project)
        interface->setWorkingDirectory(project->projectDirectory());
    return interface;
}

class JsonTreeItemDelegate : public QStyledItemDelegate
{
public:
    QString displayText(const QVariant &value, const QLocale &) const override
    {
        QString result = value.toString();
        if (result.size() == 1) {
            switch (result.at(0).toLatin1()) {
            case '\n':
                return QString("\\n");
            case '\t':
                return QString("\\t");
            case '\r':
                return QString("\\r");
            }
        }
        return result;
    }
};

static QString startupBehaviorString(BaseSettings::StartBehavior behavior)
{
    switch (behavior) {
    case BaseSettings::AlwaysOn:
        return Tr::tr("Always On");
    case BaseSettings::RequiresFile:
        return Tr::tr("Requires an Open File");
    case BaseSettings::RequiresProject:
        return Tr::tr("Start Server per Project");
    default:
        break;
    }
    return {};
}

BaseSettingsWidget::BaseSettingsWidget(const BaseSettings *settings, QWidget *parent)
    : QWidget(parent)
    , m_name(new QLineEdit(settings->m_name, this))
    , m_mimeTypes(new QLabel(settings->m_languageFilter.mimeTypes.join(filterSeparator), this))
    , m_filePattern(new QLineEdit(settings->m_languageFilter.filePattern.join(filterSeparator), this))
    , m_startupBehavior(new QComboBox)
    , m_initializationOptions(new Utils::FancyLineEdit(this))
{
    int row = 0;
    auto *mainLayout = new QGridLayout;

    mainLayout->addWidget(new QLabel(Tr::tr("Name:")), row, 0);
    mainLayout->addWidget(m_name, row, 1);
    auto chooser = new Utils::VariableChooser(this);
    chooser->addSupportedWidget(m_name);

    mainLayout->addWidget(new QLabel(Tr::tr("Language:")), ++row, 0);
    auto mimeLayout = new QHBoxLayout;
    mimeLayout->addWidget(m_mimeTypes);
    mimeLayout->addStretch();
    auto addMimeTypeButton = new QPushButton(Tr::tr("Set MIME Types..."), this);
    mimeLayout->addWidget(addMimeTypeButton);
    mainLayout->addLayout(mimeLayout, row, 1);
    m_filePattern->setPlaceholderText(Tr::tr("File pattern"));
    m_filePattern->setToolTip(
        Tr::tr("List of file patterns.\nExample: *.cpp%1*.h").arg(filterSeparator));
    mainLayout->addWidget(m_filePattern, ++row, 1);

    mainLayout->addWidget(new QLabel(Tr::tr("Startup behavior:")), ++row, 0);
    for (int behavior = 0; behavior < BaseSettings::LastSentinel ; ++behavior)
        m_startupBehavior->addItem(startupBehaviorString(BaseSettings::StartBehavior(behavior)));
    m_startupBehavior->setCurrentIndex(settings->m_startBehavior);
    mainLayout->addWidget(m_startupBehavior, row, 1);


    connect(addMimeTypeButton, &QPushButton::pressed,
            this, &BaseSettingsWidget::showAddMimeTypeDialog);

    mainLayout->addWidget(new QLabel(Tr::tr("Initialization options:")), ++row, 0);
    mainLayout->addWidget(m_initializationOptions, row, 1);
    chooser->addSupportedWidget(m_initializationOptions);
    m_initializationOptions->setValidationFunction([](Utils::FancyLineEdit *edit, QString *errorMessage) {
        const QString value = Utils::globalMacroExpander()->expand(edit->text());

        if (value.isEmpty())
            return true;

        QJsonParseError parseInfo;
        const QJsonDocument json = QJsonDocument::fromJson(value.toUtf8(), &parseInfo);

        if (json.isNull()) {
            if (errorMessage)
                *errorMessage = Tr::tr("Failed to parse JSON at %1: %2")
                    .arg(parseInfo.offset)
                    .arg(parseInfo.errorString());
            return false;
        }
        return true;
    });
    m_initializationOptions->setText(settings->m_initializationOptions);
    m_initializationOptions->setPlaceholderText(Tr::tr("Language server-specific JSON to pass via "
                                                   "\"initializationOptions\" field of \"initialize\" "
                                                   "request."));

    setLayout(mainLayout);
}

QString BaseSettingsWidget::name() const
{
    return m_name->text();
}

LanguageFilter BaseSettingsWidget::filter() const
{
    return {m_mimeTypes->text().split(filterSeparator, Qt::SkipEmptyParts),
                m_filePattern->text().split(filterSeparator, Qt::SkipEmptyParts)};
}

BaseSettings::StartBehavior BaseSettingsWidget::startupBehavior() const
{
    return BaseSettings::StartBehavior(m_startupBehavior->currentIndex());
}

QString BaseSettingsWidget::initializationOptions() const
{
    return m_initializationOptions->text();
}

class MimeTypeModel : public QStringListModel
{
public:
    using QStringListModel::QStringListModel;
    QVariant data(const QModelIndex &index, int role) const final
    {
        if (index.isValid() && role == Qt::CheckStateRole)
            return m_selectedMimeTypes.contains(index.data().toString()) ? Qt::Checked : Qt::Unchecked;
        return QStringListModel::data(index, role);
    }
    bool setData(const QModelIndex &index, const QVariant &value, int role) final
    {
        if (index.isValid() && role == Qt::CheckStateRole) {
            QString mimeType = index.data().toString();
            if (value.toInt() == Qt::Checked) {
                if (!m_selectedMimeTypes.contains(mimeType))
                    m_selectedMimeTypes.append(index.data().toString());
            } else {
                m_selectedMimeTypes.removeAll(index.data().toString());
            }
            return true;
        }
        return QStringListModel::setData(index, value, role);
    }

    Qt::ItemFlags flags(const QModelIndex &index) const final
    {
        if (!index.isValid())
            return Qt::NoItemFlags;
        return (QStringListModel::flags(index)
                & ~(Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled))
                | Qt::ItemIsUserCheckable;
    }
    QStringList m_selectedMimeTypes;
};

class MimeTypeDialog : public QDialog
{
public:
    explicit MimeTypeDialog(const QStringList &selectedMimeTypes, QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle(Tr::tr("Select MIME Types"));
        auto mainLayout = new QVBoxLayout;
        auto filter = new Utils::FancyLineEdit(this);
        filter->setFiltering(true);
        mainLayout->addWidget(filter);
        auto listView = new QListView(this);
        mainLayout->addWidget(listView);
        auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        mainLayout->addWidget(buttons);
        setLayout(mainLayout);

        filter->setPlaceholderText(Tr::tr("Filter"));
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        auto proxy = new QSortFilterProxyModel(this);
        m_mimeTypeModel = new MimeTypeModel(Utils::transform(Utils::allMimeTypes(),
                                                             &Utils::MimeType::name), this);
        m_mimeTypeModel->m_selectedMimeTypes = selectedMimeTypes;
        proxy->setSourceModel(m_mimeTypeModel);
        proxy->sort(0);
        connect(filter, &QLineEdit::textChanged, proxy, &QSortFilterProxyModel::setFilterWildcard);
        listView->setModel(proxy);

        setModal(true);
    }

    MimeTypeDialog(const MimeTypeDialog &other) = delete;
    MimeTypeDialog(MimeTypeDialog &&other) = delete;

    MimeTypeDialog operator=(const MimeTypeDialog &other) = delete;
    MimeTypeDialog operator=(MimeTypeDialog &&other) = delete;


    QStringList mimeTypes() const
    {
        return m_mimeTypeModel->m_selectedMimeTypes;
    }
private:
    MimeTypeModel *m_mimeTypeModel = nullptr;
};

void BaseSettingsWidget::showAddMimeTypeDialog()
{
    MimeTypeDialog dialog(m_mimeTypes->text().split(filterSeparator, Qt::SkipEmptyParts),
                          Core::ICore::dialogParent());
    if (dialog.exec() == QDialog::Rejected)
        return;
    m_mimeTypes->setText(dialog.mimeTypes().join(filterSeparator));
}

StdIOSettingsWidget::StdIOSettingsWidget(const StdIOSettings *settings, QWidget *parent)
    : BaseSettingsWidget(settings, parent)
    , m_executable(new Utils::PathChooser(this))
    , m_arguments(new QLineEdit(settings->m_arguments, this))
{
    auto mainLayout = qobject_cast<QGridLayout *>(layout());
    QTC_ASSERT(mainLayout, return);
    const int baseRows = mainLayout->rowCount();
    mainLayout->addWidget(new QLabel(Tr::tr("Executable:")), baseRows, 0);
    mainLayout->addWidget(m_executable, baseRows, 1);
    mainLayout->addWidget(new QLabel(Tr::tr("Arguments:")), baseRows + 1, 0);
    m_executable->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_executable->setFilePath(settings->m_executable);
    mainLayout->addWidget(m_arguments, baseRows + 1, 1);

    auto chooser = new Utils::VariableChooser(this);
    chooser->addSupportedWidget(m_arguments);
}

Utils::FilePath StdIOSettingsWidget::executable() const
{
    return m_executable->filePath();
}

QString StdIOSettingsWidget::arguments() const
{
    return m_arguments->text();
}

bool LanguageFilter::isSupported(const Utils::FilePath &filePath, const QString &mimeType) const
{
    if (mimeTypes.contains(mimeType))
        return true;
    if (filePattern.isEmpty() && filePath.isEmpty())
        return mimeTypes.isEmpty();
    const QRegularExpression::PatternOptions options
            = Utils::HostOsInfo::fileNameCaseSensitivity() == Qt::CaseInsensitive
            ? QRegularExpression::CaseInsensitiveOption : QRegularExpression::NoPatternOption;
    auto regexps = Utils::transform(filePattern, [&options](const QString &pattern){
        return QRegularExpression(QRegularExpression::wildcardToRegularExpression(pattern),
                                  options);
    });
    return Utils::anyOf(regexps, [filePath](const QRegularExpression &reg){
        return reg.match(filePath.toString()).hasMatch()
                || reg.match(filePath.fileName()).hasMatch();
    });
}

bool LanguageFilter::isSupported(const Core::IDocument *document) const
{
    return isSupported(document->filePath(), document->mimeType());
}

bool LanguageFilter::operator==(const LanguageFilter &other) const
{
    return this->filePattern == other.filePattern && this->mimeTypes == other.mimeTypes;
}

bool LanguageFilter::operator!=(const LanguageFilter &other) const
{
    return this->filePattern != other.filePattern || this->mimeTypes != other.mimeTypes;
}

TextEditor::BaseTextEditor *jsonEditor()
{
    using namespace TextEditor;
    using namespace Utils::Text;
    BaseTextEditor *textEditor = nullptr;
    for (Core::IEditorFactory *factory : Core::IEditorFactory::preferredEditorFactories("foo.json")) {
        Core::IEditor *editor = factory->createEditor();
        if (textEditor = qobject_cast<BaseTextEditor *>(editor); textEditor)
            break;
        delete editor;
    }
    QTC_ASSERT(textEditor, textEditor = PlainTextEditorFactory::createPlainTextEditor());
    TextDocument *document = textEditor->textDocument();
    TextEditorWidget *widget = textEditor->editorWidget();
    widget->configureGenericHighlighter(Utils::mimeTypeForName("application/json"));
    widget->setLineNumbersVisible(false);
    widget->setRevisionsVisible(false);
    widget->setCodeFoldingSupported(false);
    QObject::connect(document, &TextDocument::contentsChanged, widget, [document](){
        const Utils::Id jsonMarkId("LanguageClient.JsonTextMarkId");
        const TextMarks marks = document->marks();
        for (TextMark *mark : marks) {
            if (mark->category().id == jsonMarkId)
                delete mark;
        }
        const QString content = document->plainText().trimmed();
        if (content.isEmpty())
            return;
        QJsonParseError error;
        QJsonDocument::fromJson(content.toUtf8(), &error);
        if (error.error == QJsonParseError::NoError)
            return;
        const Position pos = Position::fromPositionInDocument(document->document(), error.offset);
        if (!pos.isValid())
            return;
        auto mark = new TextMark(Utils::FilePath(),
                                 pos.line,
                                 {::LanguageClient::Tr::tr("JSON Error"), jsonMarkId});
        mark->setLineAnnotation(error.errorString());
        mark->setColor(Utils::Theme::CodeModel_Error_TextMarkColor);
        mark->setIcon(Utils::Icons::CODEMODEL_ERROR.icon());
        document->addMark(mark);
    });
    return textEditor;
}

constexpr const char projectSettingsId[] = "LanguageClient.ProjectSettings";

ProjectSettings::ProjectSettings(ProjectExplorer::Project *project)
    : m_project(project)
{
    m_json = m_project->namedSettings(projectSettingsId).toByteArray();
}

QJsonValue ProjectSettings::workspaceConfiguration() const
{
    const auto doc = QJsonDocument::fromJson(m_json);
    if (doc.isObject())
        return doc.object();
    if (doc.isArray())
        return doc.array();
    return {};
}

QByteArray ProjectSettings::json() const
{
    return m_json;
}

void ProjectSettings::setJson(const QByteArray &json)
{
    const QJsonValue oldConfig = workspaceConfiguration();
    m_json = json;
    m_project->setNamedSettings(projectSettingsId, m_json);
    const QJsonValue newConfig = workspaceConfiguration();
    if (oldConfig != newConfig)
        LanguageClientManager::updateWorkspaceConfiguration(m_project, newConfig);
}

ProjectSettingsWidget::ProjectSettingsWidget(ProjectExplorer::Project *project)
    : m_settings(project)
{
    setUseGlobalSettingsCheckBoxVisible(false);
    setGlobalSettingsId(Constants::LANGUAGECLIENT_SETTINGS_PAGE);
    setExpanding(true);

    TextEditor::BaseTextEditor *editor = jsonEditor();
    editor->document()->setContents(m_settings.json());

    auto layout = new QVBoxLayout;
    setLayout(layout);
    auto group = new QGroupBox(Tr::tr("Workspace Configuration"));
    group->setLayout(new QVBoxLayout);
    group->layout()->addWidget(new QLabel(Tr::tr(
        "Additional JSON configuration sent to all running language servers for this project.\n"
        "See the documentation of the specific language server for valid settings.")));
    group->layout()->addWidget(editor->widget());
    layout->addWidget(group);

    connect(editor->editorWidget()->textDocument(),
            &TextEditor::TextDocument::contentsChanged,
            this,
            [=]() { m_settings.setJson(editor->document()->contents()); });
}

} // namespace LanguageClient
