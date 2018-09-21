/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "languageclientmanager.h"
#include "languageclientsettings.h"
#include "languageclient_global.h"

#include <coreplugin/icore.h>
#include <utils/algorithm.h>
#include <utils/delegates.h>
#include <utils/qtcprocess.h>
#include <utils/mimetypes/mimedatabase.h>
#include <languageserverprotocol/lsptypes.h>

#include <QBoxLayout>
#include <QComboBox>
#include <QCompleter>
#include <QCoreApplication>
#include <QFileInfo>
#include <QHeaderView>
#include <QPushButton>
#include <QSettings>
#include <QStyledItemDelegate>
#include <QTreeView>

constexpr char nameKey[] = "name";
constexpr char enabledKey[] = "enabled";
constexpr char mimeTypeKey[] = "mimeType";
constexpr char executableKey[] = "executable";
constexpr char argumentsKey[] = "arguments";
constexpr char settingsGroupKey[] = "LanguageClient";
constexpr char clientsKey[] = "clients";

namespace LanguageClient {

class LanguageClientSettingsModel : public QAbstractTableModel
{
public:
    LanguageClientSettingsModel() = default;

    // QAbstractItemModel interface
    int rowCount(const QModelIndex &/*parent*/ = QModelIndex()) const override { return m_settings.count(); }
    int columnCount(const QModelIndex &/*parent*/ = QModelIndex()) const override { return ColumnCount; }
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    bool removeRows(int row, int count = 1, const QModelIndex &parent = QModelIndex()) override;
    bool insertRows(int row, int count = 1, const QModelIndex &parent = QModelIndex()) override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void toSettings(QSettings *settings) const;
    void fromSettings(QSettings *settings);

    void applyChanges();

    enum Columns {
        DisplayNameColumn = 0,
        EnabledColumn,
        MimeTypeColumn,
        ExecutableColumn,
        ArgumentsColumn,
        ColumnCount
    };

private:
    QList<BaseSettings *> m_settings;
};

class LanguageClientSettingsPageWidget : public QWidget
{
public:
    LanguageClientSettingsPageWidget(LanguageClientSettingsModel &settings);

private:
    LanguageClientSettingsModel &m_settings;
    QTreeView *m_view;

    void addItem();
    void deleteItem();
};

class LanguageClientSettingsPage : public Core::IOptionsPage
{
public:
    LanguageClientSettingsPage();
    ~LanguageClientSettingsPage() override;

    void init();

    // IOptionsPage interface
    QWidget *widget() override;
    void apply() override;
    void finish() override;

private:
    LanguageClientSettingsModel m_settings;
    QPointer<LanguageClientSettingsPageWidget> m_widget;
};

class LanguageChooseDelegate : public QStyledItemDelegate
{
public:
    QWidget *createEditor(QWidget *parent,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
};

QWidget *LanguageChooseDelegate::createEditor(QWidget *parent,
                                              const QStyleOptionViewItem &option,
                                              const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    auto editor = new QComboBox(parent);
    editor->addItem(noLanguageFilter);
    editor->addItems(LanguageServerProtocol::languageIds().values());
    return editor;
}

void LanguageChooseDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    if (auto comboBox = qobject_cast<QComboBox*>(editor))
        comboBox->setCurrentText(index.data().toString());
}

LanguageClientSettingsPageWidget::LanguageClientSettingsPageWidget(LanguageClientSettingsModel &settings)
    : m_settings(settings)
    , m_view(new QTreeView())
{
    auto layout = new QHBoxLayout();
    m_view->setModel(&m_settings);
    m_view->header()->setStretchLastSection(true);
    m_view->setRootIsDecorated(false);
    m_view->setItemsExpandable(false);
    auto mimeTypes = Utils::transform(Utils::allMimeTypes(), [](const Utils::MimeType &mimeType){
        return mimeType.name();
    });
    auto mimeTypeCompleter = new QCompleter(mimeTypes);
    mimeTypeCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    mimeTypeCompleter->setFilterMode(Qt::MatchContains);
    m_view->setItemDelegateForColumn(LanguageClientSettingsModel::MimeTypeColumn,
                                     new Utils::CompleterDelegate(mimeTypeCompleter));
    auto executableDelegate = new Utils::PathChooserDelegate();
    executableDelegate->setExpectedKind(Utils::PathChooser::File);
    executableDelegate->setHistoryCompleter("LanguageClient.ServerPathHistory");
    m_view->setItemDelegateForColumn(LanguageClientSettingsModel::ExecutableColumn, executableDelegate);
    auto buttonLayout = new QVBoxLayout();
    auto addButton = new QPushButton(tr("&Add"));
    connect(addButton, &QPushButton::pressed, this, &LanguageClientSettingsPageWidget::addItem);
    auto deleteButton = new QPushButton(tr("&Delete"));
    connect(deleteButton, &QPushButton::pressed, this, &LanguageClientSettingsPageWidget::deleteItem);

    setLayout(layout);
    layout->addWidget(m_view);
    layout->addLayout(buttonLayout);
    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(deleteButton);
    buttonLayout->addStretch(10);
}

void LanguageClientSettingsPageWidget::addItem()
{
    const int row = m_settings.rowCount();
    m_settings.insertRows(row);
}

void LanguageClientSettingsPageWidget::deleteItem()
{
    auto index = m_view->currentIndex();
    if (index.isValid())
        m_settings.removeRows(index.row());
}

LanguageClientSettingsPage::LanguageClientSettingsPage()
{
    setId("LanguageClient.General");
    setDisplayName(tr("General"));
    setCategory(Constants::LANGUAGECLIENT_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("LanguageClient",
                                                   Constants::LANGUAGECLIENT_SETTINGS_TR));
    //setCategoryIcon( /* TODO */ );
}

LanguageClientSettingsPage::~LanguageClientSettingsPage()
{
    if (m_widget)
        delete m_widget;
}

void LanguageClientSettingsPage::init()
{
    m_settings.fromSettings(Core::ICore::settings());
    m_settings.applyChanges();
}

QWidget *LanguageClientSettingsPage::widget()
{
    if (!m_widget)
        m_widget = new LanguageClientSettingsPageWidget(m_settings);
    return m_widget;
}

void LanguageClientSettingsPage::apply()
{
    m_settings.toSettings(Core::ICore::settings());
    m_settings.applyChanges();
}

void LanguageClientSettingsPage::finish()
{
    m_settings.fromSettings(Core::ICore::settings());
}

QVariant LanguageClientSettingsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    BaseSettings *setting = m_settings[index.row()];
    QTC_ASSERT(setting, return false);
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
        case DisplayNameColumn: return setting->m_name;
        case MimeTypeColumn: return setting->m_mimeType;
        case ExecutableColumn: return setting->m_executable;
        case ArgumentsColumn: return setting->m_arguments;
        }
    } else if (role == Qt::CheckStateRole && index.column() == EnabledColumn) {
        return setting->m_enabled ? Qt::Checked : Qt::Unchecked;
    }
    return QVariant();
}

QVariant LanguageClientSettingsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QVariant();

    switch (section) {
    case DisplayNameColumn: return tr("Name");
    case EnabledColumn: return tr("Enabled");
    case MimeTypeColumn: return tr("Mime Type");
    case ExecutableColumn: return tr("Executable");
    case ArgumentsColumn: return tr("Arguments");
    }
    return QVariant();
}

bool LanguageClientSettingsModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (row >= int(m_settings.size()))
        return false;
    const int end = qMin(row + count - 1, int(m_settings.size()) - 1);
    beginRemoveRows(parent, row, end);
    for (auto i = end; i >= row; --i)
        delete m_settings.takeAt(i);
    endRemoveRows();
    return true;
}

bool LanguageClientSettingsModel::insertRows(int row, int count, const QModelIndex &parent)
{
    if (row > m_settings.size() || row < 0)
        return false;
    beginInsertRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i)
        m_settings.insert(row + i, new BaseSettings());
    endInsertRows();
    return true;
}

bool LanguageClientSettingsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;
    BaseSettings *setting = m_settings[index.row()];
    QTC_ASSERT(setting, return false);
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
        case DisplayNameColumn: setting->m_name = value.toString(); break;
        case MimeTypeColumn: setting->m_mimeType = value.toString(); break;
        case ExecutableColumn: setting->m_executable = value.toString(); break;
        case ArgumentsColumn: setting->m_arguments = value.toString(); break;
        default:
            return false;
        }
        emit dataChanged(index, index, { Qt::EditRole, Qt::DisplayRole });
        return true;
    }
    if (role == Qt::CheckStateRole && index.column() == EnabledColumn) {
        setting->m_enabled = value.toBool();
        emit dataChanged(index, index, { Qt::CheckStateRole });
        return true;
    }
    return false;
}

Qt::ItemFlags LanguageClientSettingsModel::flags(const QModelIndex &index) const
{
    const auto defaultFlags = Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled;
    if (index.column() == EnabledColumn)
        return defaultFlags | Qt::ItemIsUserCheckable;
    return defaultFlags;
}

void LanguageClientSettingsModel::toSettings(QSettings *settings) const
{
    settings->beginGroup(settingsGroupKey);
    settings->setValue(clientsKey, Utils::transform(m_settings,
                                                    [](const BaseSettings *setting){
        return QVariant(setting->toMap());
    }));
    settings->endGroup();
}

void LanguageClientSettingsModel::fromSettings(QSettings *settings)
{
    settings->beginGroup(settingsGroupKey);
    auto variants = settings->value(clientsKey).toList();
    m_settings.reserve(variants.size());
    m_settings = Utils::transform(variants, [](const QVariant& var){
        auto settings = new BaseSettings();
        settings->fromMap(var.toMap());
        return settings;
    });
    settings->endGroup();
}

void LanguageClientSettingsModel::applyChanges()
{
    const QVector<BaseClient *> interfaces(LanguageClientManager::clients());
    QVector<BaseClient *> toShutdown;
    QList<BaseSettings *> toStart = m_settings;
    // check currently registered interfaces
    for (auto interface : interfaces) {
        auto setting = Utils::findOr(m_settings, nullptr,
                                     [interface](const BaseSettings *setting){
            return interface->matches(setting);
        });
        if (setting && setting->isValid() && setting->m_enabled) {
            toStart.removeAll(setting);
            if (!interface->isSupportedMimeType(setting->m_mimeType))
                interface->setSupportedMimeType({setting->m_mimeType});
        } else {
            toShutdown << interface;
        }
    }
    for (auto interface : toShutdown) {
        if (interface->reachable())
            interface->shutdown();
        else
            LanguageClientManager::deleteClient(interface);
    }
    for (auto setting : toStart) {
        if (setting && setting->isValid() && setting->m_enabled)
            LanguageClientManager::startClient(setting->createClient());
    }
}

bool BaseSettings::isValid()
{
    return !m_name.isEmpty() && !m_executable.isEmpty() && QFile::exists(m_executable);
}

BaseClient *BaseSettings::createClient()
{
    auto client = new StdIOClient(m_executable, m_arguments);
    client->setName(m_name);
    if (m_mimeType != noLanguageFilter)
        client->setSupportedMimeType({m_mimeType});
    return client;
}

QVariantMap BaseSettings::toMap() const
{
    QVariantMap map;
    map.insert(nameKey, m_name);
    map.insert(enabledKey, m_enabled);
    map.insert(mimeTypeKey, m_mimeType);
    map.insert(executableKey, m_executable);
    map.insert(argumentsKey, m_arguments);
    return map;
}

void BaseSettings::fromMap(const QVariantMap &map)
{
    m_name = map[nameKey].toString();
    m_enabled = map[enabledKey].toBool();
    m_mimeType = map[mimeTypeKey].toString();
    m_executable = map[executableKey].toString();
    m_arguments = map[argumentsKey].toString();
}

void LanguageClientSettings::init()
{
    static LanguageClientSettingsPage settingsPage;
    settingsPage.init();
}

} // namespace LanguageClient
