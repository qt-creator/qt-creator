/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "loggingviewer.h"

#include "actionmanager/actionmanager.h"
#include "coreicons.h"
#include "icore.h"
#include "loggingmanager.h"

#include <utils/algorithm.h>
#include <utils/basetreeview.h>
#include <utils/executeondestruction.h>
#include <utils/listmodel.h>
#include <utils/qtcassert.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QClipboard>
#include <QColorDialog>
#include <QComboBox>
#include <QDialog>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>

namespace Core {
namespace Internal {

class LoggingCategoryItem
{
public:
    QString name;
    LoggingCategoryEntry entry;

    static LoggingCategoryItem fromJson(const QJsonObject &object, bool *ok);
};

LoggingCategoryItem LoggingCategoryItem::fromJson(const QJsonObject &object, bool *ok)
{
    if (!object.contains("name")) {
        *ok = false;
        return {};
    }
    const QJsonValue entryVal = object.value("entry");
    if (entryVal.isUndefined()) {
        *ok = false;
        return {};
    }
    const QJsonObject entryObj = entryVal.toObject();
    if (!entryObj.contains("level")) {
        *ok = false;
        return {};
    }

    LoggingCategoryEntry entry;
    entry.level = QtMsgType(entryObj.value("level").toInt());
    entry.enabled = true;
    if (entryObj.contains("color"))
        entry.color = QColor(entryObj.value("color").toString());
    LoggingCategoryItem item {object.value("name").toString(), entry};
    *ok = true;
    return item;
}

class LoggingCategoryModel : public QAbstractListModel
{
    Q_OBJECT
public:
    LoggingCategoryModel() = default;
    ~LoggingCategoryModel() override;

    bool append(const QString &category, const LoggingCategoryEntry &entry = {});
    bool update(const QString &category, const LoggingCategoryEntry &entry);
    int columnCount(const QModelIndex &) const final { return 3; }
    int rowCount(const QModelIndex & = QModelIndex()) const final { return m_categories.count(); }
    QVariant data(const QModelIndex &index, int role) const final;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) final;
    Qt::ItemFlags flags(const QModelIndex &index) const final;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const final;
    void reset();
    void setFromManager(LoggingViewManager *manager);
    QList<LoggingCategoryItem> enabledCategories() const;
    void disableAll();

signals:
    void categoryChanged(const QString &category, bool enabled);
    void colorChanged(const QString &category, const QColor &color);
    void logLevelChanged(const QString &category, QtMsgType logLevel);

private:
    QList<LoggingCategoryItem *> m_categories;
};

LoggingCategoryModel::~LoggingCategoryModel()
{
    reset();
}

bool LoggingCategoryModel::append(const QString &category, const LoggingCategoryEntry &entry)
{
    // no check?
    beginInsertRows(QModelIndex(), m_categories.size(), m_categories.size());
    m_categories.append(new LoggingCategoryItem{category, entry});
    endInsertRows();
    return true;
}

bool LoggingCategoryModel::update(const QString &category, const LoggingCategoryEntry &entry)
{
    if (m_categories.size() == 0) // should not happen
        return false;

    int row = 0;
    for (int end = m_categories.size(); row < end; ++row) {
        if (m_categories.at(row)->name == category)
            break;
    }
    if (row == m_categories.size()) // should not happen
        return false;

    setData(index(row, 0), Qt::Checked, Qt::CheckStateRole);
    setData(index(row, 1), LoggingViewManager::messageTypeToString(entry.level), Qt::EditRole);
    setData(index(row, 2), entry.color, Qt::DecorationRole);
    return true;
}

QVariant LoggingCategoryModel::data(const QModelIndex &index, int role) const
{
    static const QColor defaultColor = Utils::creatorTheme()->palette().text().color();
    if (!index.isValid())
        return {};
    if (role == Qt::DisplayRole) {
        if (index.column() == 0)
            return m_categories.at(index.row())->name;
        if (index.column() == 1) {
            return LoggingViewManager::messageTypeToString(
                        m_categories.at(index.row())->entry.level);
        }
    }
    if (role == Qt::DecorationRole && index.column() == 2) {
        const QColor color = m_categories.at(index.row())->entry.color;
        if (color.isValid())
            return color;
        return defaultColor;
    }
    if (role == Qt::CheckStateRole && index.column() == 0) {
        const LoggingCategoryEntry entry = m_categories.at(index.row())->entry;
        return entry.enabled ? Qt::Checked : Qt::Unchecked;
    }
    return {};
}

bool LoggingCategoryModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    if (role == Qt::CheckStateRole && index.column() == 0) {
        LoggingCategoryItem *item = m_categories.at(index.row());
        const Qt::CheckState current = item->entry.enabled ? Qt::Checked : Qt::Unchecked;
        if (current != value.toInt()) {
            item->entry.enabled = !item->entry.enabled;
            emit categoryChanged(item->name, item->entry.enabled);
            return true;
        }
    } else if (role == Qt::DecorationRole && index.column() == 2) {
        LoggingCategoryItem *item = m_categories.at(index.row());
        QColor color = value.value<QColor>();
        if (color.isValid() && color != item->entry.color) {
            item->entry.color = color;
            emit colorChanged(item->name, color);
            return true;
        }
    } else if (role == Qt::EditRole && index.column() == 1) {
        LoggingCategoryItem *item = m_categories.at(index.row());
        item->entry.level = LoggingViewManager::messageTypeFromString(value.toString());
        emit logLevelChanged(item->name, item->entry.level);
        return true;
    }

    return false;
}

Qt::ItemFlags LoggingCategoryModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    // ItemIsEnabled should depend on availability (Qt logging enabled?)
    if (index.column() == 0)
        return Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (index.column() == 1)
        return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant LoggingCategoryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal && section >= 0 && section < 3) {
        switch (section) {
        case 0: return tr("Category");
        case 1: return tr("Type");
        case 2: return tr("Color");
        }
    }
    return {};
}

void LoggingCategoryModel::reset()
{
    beginResetModel();
    qDeleteAll(m_categories);
    m_categories.clear();
    endResetModel();
}

void LoggingCategoryModel::setFromManager(LoggingViewManager *manager)
{
    beginResetModel();
    qDeleteAll(m_categories);
    m_categories.clear();
    const QMap<QString, LoggingCategoryEntry> categories = manager->categories();
    auto it = categories.begin();
    for (auto end = categories.end() ; it != end; ++it)
        m_categories.append(new LoggingCategoryItem{it.key(), it.value()});
    endResetModel();
}

QList<LoggingCategoryItem> LoggingCategoryModel::enabledCategories() const
{
    QList<LoggingCategoryItem> result;
    for (auto item : m_categories) {
        if (item->entry.enabled)
            result.append({item->name, item->entry});
    }
    return result;
}

void LoggingCategoryModel::disableAll()
{
    for (int row = 0, end = m_categories.count(); row < end; ++row)
        setData(index(row, 0), Qt::Unchecked, Qt::CheckStateRole);
}

class LoggingLevelDelegate : public QStyledItemDelegate
{
public:
    explicit LoggingLevelDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}
    ~LoggingLevelDelegate() = default;

protected:
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;
};

QWidget *LoggingLevelDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/*option*/,
                                            const QModelIndex &index) const
{
    if (!index.isValid() || index.column() != 1)
        return nullptr;
    QComboBox *combo = new QComboBox(parent);
    combo->addItems({ {"Critical"}, {"Warning"}, {"Debug"}, {"Info"} });
    return combo;
}

void LoggingLevelDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QComboBox *combo = qobject_cast<QComboBox *>(editor);
    if (!combo)
        return;

    const int i = combo->findText(index.data().toString());
    if (i >= 0)
        combo->setCurrentIndex(i);
}

void LoggingLevelDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                        const QModelIndex &index) const
{
    QComboBox *combo = qobject_cast<QComboBox *>(editor);
    if (combo)
        model->setData(index, combo->currentText());
}

class LogEntry
{
public:
    QString timestamp;
    QString category;
    QString type;
    QString message;

    QString outputLine(bool printTimestamp, bool printType) const
    {
        QString line;
        if (printTimestamp)
            line.append(timestamp + ' ');
        line.append(category);
        if (printType)
            line.append('.' + type.toLower());
        line.append(": ");
        line.append(message);
        line.append('\n');
        return line;
    }
};

class LoggingViewManagerWidget : public QDialog
{
    Q_DECLARE_TR_FUNCTIONS(LoggingViewManagerWidget)
public:
    explicit LoggingViewManagerWidget(QWidget *parent);
    ~LoggingViewManagerWidget()
    {
        setEnabled(false);
        delete m_manager;
    }

    static QColor colorForCategory(const QString &category);
private:
    void showLogViewContextMenu(const QPoint &pos) const;
    void showLogCategoryContextMenu(const QPoint &pos) const;
    void saveLoggingsToFile() const;
    void saveEnabledCategoryPreset() const;
    void loadAndUpdateFromPreset();
    LoggingViewManager *m_manager = nullptr;
    void setCategoryColor(const QString &category, const QColor &color);
    // should category model be owned directly by the manager? or is this duplication of
    // categories in manager and widget beneficial?
    LoggingCategoryModel *m_categoryModel = nullptr;
    Utils::BaseTreeView *m_logView = nullptr;
    Utils::BaseTreeView *m_categoryView = nullptr;
    Utils::ListModel<LogEntry> *m_logModel = nullptr;
    QToolButton *m_timestamps = nullptr;
    QToolButton *m_messageTypes = nullptr;
    static QHash<QString, QColor> m_categoryColor;
};

QHash<QString, QColor> LoggingViewManagerWidget::m_categoryColor;

static QVariant logEntryDataAccessor(const LogEntry &entry, int column, int role)
{
    if (column >= 0 && column <= 3 && (role == Qt::DisplayRole || role == Qt::ToolTipRole)) {
        switch (column) {
        case 0: return entry.timestamp;
        case 1: return entry.category;
        case 2: return entry.type;
        case 3: return entry.message;
        }
    }
    if (role == Qt::TextAlignmentRole)
        return Qt::AlignTop;
    if (column == 1 && role == Qt::ForegroundRole)
        return LoggingViewManagerWidget::colorForCategory(entry.category);
    return {};
}

LoggingViewManagerWidget::LoggingViewManagerWidget(QWidget *parent)
    : QDialog(parent)
    , m_manager(new LoggingViewManager)
{
    setWindowTitle(tr("Logging Category Viewer"));
    setModal(false);

    auto mainLayout = new QVBoxLayout;

    auto buttonsLayout = new QHBoxLayout;
    buttonsLayout->setSpacing(0);
    // add further buttons..
    auto save = new QToolButton;
    save->setIcon(Utils::Icons::SAVEFILE.icon());
    save->setToolTip(tr("Save Log"));
    buttonsLayout->addWidget(save);
    auto clean = new QToolButton;
    clean->setIcon(Utils::Icons::CLEAN.icon());
    clean->setToolTip(tr("Clear"));
    buttonsLayout->addWidget(clean);
    auto stop = new QToolButton;
    stop->setIcon(Utils::Icons::STOP_SMALL.icon());
    stop->setToolTip(tr("Stop Logging"));
    buttonsLayout->addWidget(stop);
    auto qtInternal = new QToolButton;
    qtInternal->setIcon(Core::Icons::QTLOGO.icon());
    qtInternal->setToolTip(tr("Toggle logging of Qt internal loggings"));
    qtInternal->setCheckable(true);
    qtInternal->setChecked(false);
    buttonsLayout->addWidget(qtInternal);
    auto autoScroll = new QToolButton;
    autoScroll->setIcon(Utils::Icons::ARROW_DOWN.icon());
    autoScroll->setToolTip(tr("Auto Scroll"));
    autoScroll->setCheckable(true);
    autoScroll->setChecked(true);
    buttonsLayout->addWidget(autoScroll);
    m_timestamps = new QToolButton;
    auto icon = Utils::Icon({{":/utils/images/stopwatch.png", Utils::Theme::PanelTextColorMid}},
                            Utils::Icon::Tint);
    m_timestamps->setIcon(icon.icon());
    m_timestamps->setToolTip(tr("Timestamps"));
    m_timestamps->setCheckable(true);
    m_timestamps->setChecked(true);
    buttonsLayout->addWidget(m_timestamps);
    m_messageTypes = new QToolButton;
    icon = Utils::Icon({{":/utils/images/message.png", Utils::Theme::PanelTextColorMid}},
                       Utils::Icon::Tint);
    m_messageTypes->setIcon(icon.icon());
    m_messageTypes->setToolTip(tr("Message Types"));
    m_messageTypes->setCheckable(true);
    m_messageTypes->setChecked(false);
    buttonsLayout->addWidget(m_messageTypes);

    buttonsLayout->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Expanding));
    mainLayout->addLayout(buttonsLayout);

    auto horizontal = new QHBoxLayout;
    m_logView = new Utils::BaseTreeView;
    m_logModel = new Utils::ListModel<LogEntry>;
    m_logModel->setHeader({tr("Timestamp"), tr("Category"), tr("Type"), tr("Message")});
    m_logModel->setDataAccessor(&logEntryDataAccessor);
    m_logView->setModel(m_logModel);
    horizontal->addWidget(m_logView);
    m_logView->setUniformRowHeights(false);
    m_logView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_logView->setFrameStyle(QFrame::Box);
    m_logView->setTextElideMode(Qt::ElideNone);
    m_logView->setAttribute(Qt::WA_MacShowFocusRect, false);
    m_logView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_logView->setColumnHidden(2, true);
    m_logView->setContextMenuPolicy(Qt::CustomContextMenu);

    m_categoryView = new Utils::BaseTreeView;
    m_categoryView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_categoryView->setUniformRowHeights(true);
    m_categoryView->setFrameStyle(QFrame::Box);
    m_categoryView->setTextElideMode(Qt::ElideNone);
    m_categoryView->setAttribute(Qt::WA_MacShowFocusRect, false);
    m_categoryView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_categoryView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_categoryModel = new LoggingCategoryModel;
    m_categoryModel->setFromManager(m_manager);
    auto sortFilterModel = new QSortFilterProxyModel(this);
    sortFilterModel->setSourceModel(m_categoryModel);
    sortFilterModel->sort(0);
    m_categoryView->setModel(sortFilterModel);
    m_categoryView->setItemDelegateForColumn(1, new LoggingLevelDelegate(this));
    horizontal->addWidget(m_categoryView);
    horizontal->setStretch(0, 5);
    horizontal->setStretch(1, 3);

    mainLayout->addLayout(horizontal);
    setLayout(mainLayout);
    resize(800, 300);

    connect(m_manager, &LoggingViewManager::receivedLog,
            this, [this, autoScroll](const QString &timestamp, const QString &type,
                                     const QString &category, const QString &msg) {
        if (m_logModel->rowCount() >= 1000000) // limit log to 1000000 items
            m_logModel->destroyItem(m_logModel->itemForIndex(m_logModel->index(0, 0)));
        m_logModel->appendItem(LogEntry{timestamp, type, category, msg});
        if (autoScroll->isChecked())
            m_logView->scrollToBottom();
    }, Qt::QueuedConnection);
    connect(m_manager, &LoggingViewManager::foundNewCategory,
            m_categoryModel, &LoggingCategoryModel::append, Qt::QueuedConnection);
    connect(m_manager, &LoggingViewManager::updatedCategory,
            m_categoryModel, &LoggingCategoryModel::update, Qt::QueuedConnection);
    connect(m_categoryModel, &LoggingCategoryModel::categoryChanged,
            m_manager, &LoggingViewManager::setCategoryEnabled);
    connect(m_categoryModel, &LoggingCategoryModel::colorChanged,
            this, &LoggingViewManagerWidget::setCategoryColor);
    connect(m_categoryModel, &LoggingCategoryModel::logLevelChanged,
            m_manager, &LoggingViewManager::setLogLevel);
    connect(m_categoryView, &Utils::BaseTreeView::activated,
            this, [this, sortFilterModel](const QModelIndex &index) {
        const QModelIndex modelIndex = sortFilterModel->mapToSource(index);
        const QVariant value = m_categoryModel->data(modelIndex, Qt::DecorationRole);
        if (!value.isValid())
            return;
        const QColor original = value.value<QColor>();
        if (!original.isValid())
            return;
        QColor changed = QColorDialog::getColor(original, this);
        if (!changed.isValid())
            return;
        if (original != changed)
            m_categoryModel->setData(modelIndex, changed, Qt::DecorationRole);
    });
    connect(save, &QToolButton::clicked,
            this, &LoggingViewManagerWidget::saveLoggingsToFile);
    connect(m_logView, &Utils::BaseTreeView::customContextMenuRequested,
            this, &LoggingViewManagerWidget::showLogViewContextMenu);
    connect(m_categoryView, &Utils::BaseTreeView::customContextMenuRequested,
            this, &LoggingViewManagerWidget::showLogCategoryContextMenu);
    connect(clean, &QToolButton::clicked, m_logModel, &Utils::ListModel<LogEntry>::clear);
    connect(stop, &QToolButton::clicked, this, [this, stop]() {
        if (m_manager->isEnabled()) {
            m_manager->setEnabled(false);
            stop->setIcon(Utils::Icons::RUN_SMALL.icon());
            stop->setToolTip(tr("Start Logging"));
        } else {
            m_manager->setEnabled(true);
            stop->setIcon(Utils::Icons::STOP_SMALL.icon());
            stop->setToolTip(tr("Stop Logging"));
        }
    });
    connect(qtInternal, &QToolButton::toggled, m_manager, &LoggingViewManager::setListQtInternal);
    connect(m_timestamps, &QToolButton::toggled, this, [this](bool checked){
        m_logView->setColumnHidden(0, !checked);
    });
    connect(m_messageTypes, &QToolButton::toggled, this, [this](bool checked){
        m_logView->setColumnHidden(2, !checked);
    });
}

void LoggingViewManagerWidget::showLogViewContextMenu(const QPoint &pos) const
{
    QMenu m;
    auto copy = new QAction(tr("Copy Selected Logs"), &m);
    m.addAction(copy);
    auto copyAll = new QAction(tr("Copy All"), &m);
    m.addAction(copyAll);
    connect(copy, &QAction::triggered, &m, [this](){
        auto selectionModel = m_logView->selectionModel();
        QString copied;
        const bool useTS = m_timestamps->isChecked();
        const bool useLL = m_messageTypes->isChecked();
        for (int row = 0, end = m_logModel->rowCount(); row < end; ++row) {
            if (selectionModel->isRowSelected(row, QModelIndex()))
                copied.append(m_logModel->dataAt(row).outputLine(useTS, useLL));
        }

        QGuiApplication::clipboard()->setText(copied);
    });
    connect(copyAll, &QAction::triggered, &m, [this](){
        QString copied;
        const bool useTS = m_timestamps->isChecked();
        const bool useLL = m_messageTypes->isChecked();

        for (int row = 0, end = m_logModel->rowCount(); row < end; ++row)
            copied.append(m_logModel->dataAt(row).outputLine(useTS, useLL));

        QGuiApplication::clipboard()->setText(copied);
    });
    m.exec(m_logView->mapToGlobal(pos));
}

void LoggingViewManagerWidget::showLogCategoryContextMenu(const QPoint &pos) const
{
    QMenu m;
    // minimal load/save - plugins could later provide presets on their own?
    auto savePreset = new QAction(tr("Save Enabled as Preset..."), &m);
    m.addAction(savePreset);
    auto loadPreset = new QAction(tr("Update from Preset..."), &m);
    m.addAction(loadPreset);
    auto uncheckAll = new QAction(tr("Uncheck All"), &m);
    m.addAction(uncheckAll);
    connect(savePreset, &QAction::triggered,
            this, &LoggingViewManagerWidget::saveEnabledCategoryPreset);
    connect(loadPreset, &QAction::triggered,
            this, &LoggingViewManagerWidget::loadAndUpdateFromPreset);
    connect(uncheckAll, &QAction::triggered,
            m_categoryModel, &LoggingCategoryModel::disableAll);
    m.exec(m_categoryView->mapToGlobal(pos));
}

void LoggingViewManagerWidget::saveLoggingsToFile() const
{
    // should we just let it continue without temporarily disabling?
    const bool enabled = m_manager->isEnabled();
    Utils::ExecuteOnDestruction exec([this, enabled]() { m_manager->setEnabled(enabled); });
    if (enabled)
        m_manager->setEnabled(false);
    const Utils::FilePath fp = Utils::FileUtils::getSaveFilePath(ICore::dialogParent(),
                                                                 tr("Save logs as"));
    if (fp.isEmpty())
        return;
    const bool useTS = m_timestamps->isChecked();
    const bool useLL = m_messageTypes->isChecked();
    QFile file(fp.path());
    if (file.open(QIODevice::WriteOnly)) {
        for (int row = 0, end = m_logModel->rowCount(); row < end; ++row) {
            qint64 res = file.write( m_logModel->dataAt(row).outputLine(useTS, useLL).toUtf8());
            if (res == -1) {
                QMessageBox::critical(
                            ICore::dialogParent(), tr("Error"),
                            tr("Failed to write logs to '%1'.").arg(fp.toUserOutput()));
                break;
            }
        }
        file.close();
    } else {
        QMessageBox::critical(
                    ICore::dialogParent(), tr("Error"),
                    tr("Failed to open file '%1' for writing logs.").arg(fp.toUserOutput()));
    }
}

void LoggingViewManagerWidget::saveEnabledCategoryPreset() const
{
    Utils::FilePath fp = Utils::FileUtils::getSaveFilePath(ICore::dialogParent(),
                                                           tr("Save enabled categories as"));
    if (fp.isEmpty())
        return;
    const QList<LoggingCategoryItem> enabled = m_categoryModel->enabledCategories();
    // write them to file
    QJsonArray array;
    for (const LoggingCategoryItem &item : enabled) {
        QJsonObject itemObj;
        itemObj.insert("name", item.name);
        QJsonObject entryObj;
        entryObj.insert("level", item.entry.level);
        if (item.entry.color.isValid())
            entryObj.insert("color", item.entry.color.name(QColor::HexArgb));
        itemObj.insert("entry", entryObj);
        array.append(itemObj);
    }
    QJsonDocument doc(array);
    if (!fp.writeFileContents(doc.toJson(QJsonDocument::Compact)))
        QMessageBox::critical(
                    ICore::dialogParent(), tr("Error"),
                    tr("Failed to write preset file '%1'.").arg(fp.toUserOutput()));
}

void LoggingViewManagerWidget::loadAndUpdateFromPreset()
{
    Utils::FilePath fp = Utils::FileUtils::getOpenFilePath(ICore::dialogParent(),
                                                           tr("Load enabled categories from"));
    if (fp.isEmpty())
        return;
    // read file, update categories
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(fp.fileContents(), &error);
    if (error.error != QJsonParseError::NoError) {
        QMessageBox::critical(ICore::dialogParent(), tr("Error"),
                              tr("Failed to read preset file '%1': %2").arg(fp.toUserOutput())
                              .arg(error.errorString()));
        return;
    }
    bool formatError = false;
    QList<LoggingCategoryItem> presetItems;
    if (doc.isArray()) {
        const QJsonArray array = doc.array();
        for (const QJsonValue &value : array) {
            if (!value.isObject()) {
                formatError = true;
                break;
            }
            const QJsonObject itemObj = value.toObject();
            bool ok = true;
            LoggingCategoryItem item = LoggingCategoryItem::fromJson(itemObj, &ok);
            if (!ok) {
                formatError = true;
                break;
            }
            presetItems.append(item);
        }
    } else {
        formatError = true;
    }

    if (formatError) {
        QMessageBox::critical(ICore::dialogParent(), tr("Error"),
                              tr("Unexpected preset file format."));
    }
    for (const LoggingCategoryItem &item : presetItems)
        m_manager->appendOrUpdate(item.name, item.entry);
}

QColor LoggingViewManagerWidget::colorForCategory(const QString &category)
{
    auto entry = m_categoryColor.find(category);
    if (entry == m_categoryColor.end())
        return Utils::creatorTheme()->palette().text().color();
    return entry.value();
}

void LoggingViewManagerWidget::setCategoryColor(const QString &category, const QColor &color)
{
    const QColor baseColor = Utils::creatorTheme()->palette().text().color();
    if (color != baseColor)
        m_categoryColor.insert(category, color);
    else
        m_categoryColor.remove(category);
}

void LoggingViewer::showLoggingView()
{
    ActionManager::command(Constants::LOGGER)->action()->setEnabled(false);
    auto widget = new LoggingViewManagerWidget(ICore::mainWindow());
    QObject::connect(widget, &QDialog::finished, widget, [widget] () {
        ActionManager::command(Constants::LOGGER)->action()->setEnabled(true);
        // explicitly disable manager again
        widget->deleteLater();
    });
    widget->show();
}

} // namespace Internal
} // namespace Core

#include "loggingviewer.moc"
