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
#include <languageserverprotocol/lsptypes.h>

#include <QBoxLayout>
#include <QComboBox>
#include <QCoreApplication>
#include <QFileInfo>
#include <QHeaderView>
#include <QPushButton>
#include <QSettings>
#include <QStyledItemDelegate>
#include <QTreeView>

constexpr char nameKey[] = "name";
constexpr char enabledKey[] = "enabled";
constexpr char languageKey[] = "language";
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
        LanguageColumn,
        ExecutableColumn,
        ArgumentsColumn,
        ColumnCount
    };

private:
    QList<LanguageClientSettings> m_settings;
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
    m_view->setItemDelegateForColumn(LanguageClientSettingsModel::LanguageColumn, new LanguageChooseDelegate());
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
    LanguageClientSettings setting = m_settings[index.row()];
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
        case DisplayNameColumn: return setting.m_name;
        case LanguageColumn: return setting.m_language;
        case ExecutableColumn: return setting.m_executable;
        case ArgumentsColumn: return setting.m_arguments.join(' ');
        }
    } else if (role == Qt::CheckStateRole && index.column() == EnabledColumn) {
        return setting.m_enabled ? Qt::Checked : Qt::Unchecked;
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
    case LanguageColumn: return tr("Language");
    case ExecutableColumn: return tr("Executable");
    case ArgumentsColumn: return tr("Arguments");
    }
    return QVariant();
}

bool LanguageClientSettingsModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (row >= int(m_settings.size()))
        return false;
    const auto first = m_settings.begin() + row;
    const int end = qMin(row + count - 1, int(m_settings.size()) - 1);
    beginRemoveRows(parent, row, end);
    m_settings.erase(first, first + count);
    endRemoveRows();
    return true;
}

bool LanguageClientSettingsModel::insertRows(int row, int count, const QModelIndex &parent)
{
    if (row > m_settings.size() || row < 0)
        return false;
    beginInsertRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i)
        m_settings.insert(row + i, {});
    endInsertRows();
    return true;
}

bool LanguageClientSettingsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;
    LanguageClientSettings &setting = m_settings[index.row()];
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
        case DisplayNameColumn: setting.m_name = value.toString(); break;
        case LanguageColumn: setting.m_language = value.toString(); break;
        case ExecutableColumn: setting.m_executable = value.toString(); break;
        case ArgumentsColumn: setting.m_arguments = value.toString().split(' '); break;
        default:
            return false;
        }
        emit dataChanged(index, index, { Qt::EditRole, Qt::DisplayRole });
        return true;
    }
    if (role == Qt::CheckStateRole && index.column() == EnabledColumn) {
        setting.m_enabled = value.toBool();
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
                                                    [](const LanguageClientSettings & setting){
        return QVariant(setting.toMap());
    }));
    settings->endGroup();
}

void LanguageClientSettingsModel::fromSettings(QSettings *settings)
{
    settings->beginGroup(settingsGroupKey);
    auto variants = settings->value(clientsKey).toList();
    m_settings.reserve(variants.size());
    m_settings = Utils::transform(variants, [](const QVariant& var){
        return LanguageClientSettings::fromMap(var.toMap());
    });
    settings->endGroup();
}

void LanguageClientSettingsModel::applyChanges()
{
    const QVector<BaseClient *> interfaces(LanguageClientManager::clients());
    QVector<BaseClient *> toShutdown;
    QList<LanguageClientSettings> toStart = m_settings;
    // check currently registered interfaces
    for (auto interface : interfaces) {
        auto setting = Utils::findOr(m_settings, LanguageClientSettings(), [interface](const LanguageClientSettings &setting){
            return interface->matches(setting);
        });
        if (setting.isValid() && setting.m_enabled) {
            toStart.removeAll(setting);
            if (!interface->isSupportedLanguage(setting.m_language))
                interface->setSupportedLanguages({setting.m_language});
        } else {
            toShutdown << interface;
        }
    }
    for (auto interface : toShutdown)
        interface->shutdown();
    for (auto setting : toStart) {
        if (setting.isValid() && setting.m_enabled) {
            auto client = new StdIOClient(setting.m_executable, setting.m_arguments);
            client->setName(setting.m_name);
            if (setting.m_language != noLanguageFilter)
                client->setSupportedLanguages({setting.m_language});
            LanguageClientManager::startClient(client);
        }
    }
}

bool LanguageClientSettings::isValid()
{
    return !m_name.isEmpty() && !m_executable.isEmpty() && QFile::exists(m_executable);
}

bool LanguageClientSettings::operator==(const LanguageClientSettings &other) const
{
    return m_name == other.m_name
            && m_enabled == other.m_enabled
            && m_language == other.m_language
            && m_executable == other.m_executable
            && m_arguments == other.m_arguments;
}

QVariantMap LanguageClientSettings::toMap() const
{
    QVariantMap map;
    map.insert(nameKey, m_name);
    map.insert(enabledKey, m_enabled);
    map.insert(languageKey, m_language);
    map.insert(executableKey, m_executable);
    map.insert(argumentsKey, m_arguments);
    return map;
}

LanguageClientSettings LanguageClientSettings::fromMap(const QVariantMap &map)
{
    return { map[nameKey].toString(),
                map[enabledKey].toBool(),
                map[languageKey].toString(),
                map[executableKey].toString(),
                map[argumentsKey].toStringList() };
}

void LanguageClientSettings::init()
{
    static LanguageClientSettingsPage settingsPage;
    settingsPage.init();
}

} // namespace LanguageClient
