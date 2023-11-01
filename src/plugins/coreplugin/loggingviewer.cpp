// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "loggingviewer.h"

#include "coreicons.h"
#include "coreplugintr.h"
#include "icore.h"

#include <utils/async.h>
#include <utils/basetreeview.h>
#include <utils/fancylineedit.h>
#include <utils/fileutils.h>
#include <utils/layoutbuilder.h>
#include <utils/listmodel.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QColorDialog>
#include <QDialog>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QToolButton>
#include <QVBoxLayout>
namespace Core::Internal {

static QColor colorForCategory(const QString &category);
void setCategoryColor(const QString &category, const QColor &color);

QHash<QString, QColor> s_categoryColor;

static inline QString messageTypeToString(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg:
        return {"Debug"};
    case QtInfoMsg:
        return {"Info"};
    case QtCriticalMsg:
        return {"Critical"};
    case QtWarningMsg:
        return {"Warning"};
    case QtFatalMsg:
        return {"Fatal"};
    default:
        return {"Unknown"};
    }
}

class LogCategoryRegistry : public QObject
{
    Q_OBJECT
public:
    static LogCategoryRegistry &instance()
    {
        static LogCategoryRegistry s_instance;
        return s_instance;
    }

    static void filter(QLoggingCategory *category)
    {
        if (s_oldFilter)
            s_oldFilter(category);

        LogCategoryRegistry::instance().onFilter(category);
    }

    void start()
    {
        if (m_started)
            return;

        m_started = true;
        s_oldFilter = QLoggingCategory::installFilter(&LogCategoryRegistry::filter);
    }

    QList<QLoggingCategory *> categories() { return m_categories; }

signals:
    void newLogCategory(QLoggingCategory *category);

private:
    LogCategoryRegistry() = default;

    void onFilter(QLoggingCategory *category)
    {
        if (QThread::currentThread() != thread()) {
            QMetaObject::invokeMethod(
                this, [category, this] { onFilter(category); }, Qt::QueuedConnection);
            return;
        }

        if (!m_categories.contains(category)) {
            m_categories.append(category);
            emit newLogCategory(category);
        }
    }

private:
    static QLoggingCategory::CategoryFilter s_oldFilter;

    QList<QLoggingCategory *> m_categories;
    bool m_started{false};
};

QLoggingCategory::CategoryFilter LogCategoryRegistry::s_oldFilter = nullptr;

struct SavedEntry
{
    QColor color;
    QString name;
    QtMsgType level{QtFatalMsg};
    std::optional<std::array<bool, 5>> levels;

    static Utils::expected_str<SavedEntry> fromJson(const QJsonObject &obj)
    {
        if (!obj.contains("name"))
            return Utils::make_unexpected(Tr::tr("Entry is missing a logging category name."));

        SavedEntry result;
        result.name = obj.value("name").toString();

        if (!obj.contains("entry"))
            return Utils::make_unexpected(Tr::tr("Entry is missing data."));

        auto entry = obj.value("entry").toObject();
        if (entry.contains("color"))
            result.color = QColor(entry.value("color").toString());

        if (entry.contains("level")) {
            int lvl = entry.value("level").toInt(0);
            if (lvl < QtDebugMsg || lvl > QtInfoMsg)
                return Utils::make_unexpected(Tr::tr("Invalid level: %1").arg(lvl));
            result.level = static_cast<QtMsgType>(lvl);
        }

        if (entry.contains("levels")) {
            QVariantMap map = entry.value("levels").toVariant().toMap();
            std::array<bool, 5> levels{
                map.contains("Debug") && map["Debug"].toBool(),
                map.contains("Warning") && map["Warning"].toBool(),
                map.contains("Critical") && map["Critical"].toBool(),
                true,
                map.contains("Info") && map["Info"].toBool(),
            };
            result.levels = levels;
        }

        return result;
    }
};

class LoggingCategoryEntry
{
public:
    LoggingCategoryEntry(const QString &name)
        : m_name(name)
    {}
    LoggingCategoryEntry(QLoggingCategory *category)
        : m_name(QString::fromUtf8(category->categoryName()))
    {
        setLogCategory(category);
    }

    LoggingCategoryEntry(const SavedEntry &savedEntry)
        : m_name(savedEntry.name)
    {
        m_saved = savedEntry.levels;
        m_color = savedEntry.color;
        if (!m_saved) {
            m_saved = std::array<bool, 5>();
            for (int i = QtDebugMsg; i <= QtInfoMsg; ++i) {
                (*m_saved)[i] = savedEntry.level <= i;
            }
        }
    }

    QString name() const { return m_name; }
    QColor color() const { return m_color; }
    void setColor(const QColor &c) { m_color = c; }

    void setUseOriginal(bool useOriginal)
    {
        if (!m_useOriginal && m_category && m_originalSettings) {
            m_saved = std::array<bool, 5>{};

            for (int i = QtDebugMsg; i < QtInfoMsg; i++) {
                (*m_saved)[i] = m_category->isEnabled(static_cast<QtMsgType>(i));
                m_category->setEnabled(static_cast<QtMsgType>(i), (*m_originalSettings)[i]);
            }

        } else if (!useOriginal && m_useOriginal && m_saved && m_category) {
            for (int i = QtDebugMsg; i < QtInfoMsg; i++)
                m_category->setEnabled(static_cast<QtMsgType>(i), (*m_saved)[i]);
        }
        m_useOriginal = useOriginal;
    }

    bool isEnabled(QtMsgType msgType) const
    {
        if (m_category)
            return m_category->isEnabled(msgType);
        if (m_saved)
            return (*m_saved)[msgType];
        return false;
    }

    bool isEnabledOriginally(QtMsgType msgType) const
    {
        if (m_originalSettings)
            return (*m_originalSettings)[msgType];
        return isEnabled(msgType);
    }

    void setEnabled(QtMsgType msgType, bool isEnabled)
    {
        QTC_ASSERT(!m_useOriginal, return);

        if (m_category)
            m_category->setEnabled(msgType, isEnabled);

        if (m_saved)
            (*m_saved)[msgType] = isEnabled;
    }

    void setSaved(const SavedEntry &entry)
    {
        QTC_ASSERT(entry.name == name(), return);

        m_saved = entry.levels;
        m_color = entry.color;
        if (!m_saved) {
            m_saved = std::array<bool, 5>();
            for (int i = QtDebugMsg; i <= QtInfoMsg; ++i) {
                (*m_saved)[i] = entry.level <= i;
            }
        }
        if (m_category)
            setLogCategory(m_category);
    }

    void setLogCategory(QLoggingCategory *category)
    {
        QTC_ASSERT(QString::fromUtf8(category->categoryName()) == m_name, return);

        m_category = category;
        if (!m_originalSettings) {
            m_originalSettings = {
                category->isDebugEnabled(),
                category->isWarningEnabled(),
                category->isCriticalEnabled(),
                true, // always enable fatal
                category->isInfoEnabled(),
            };
        }

        if (m_saved && !m_useOriginal) {
            m_category->setEnabled(QtDebugMsg, m_saved->at(0));
            m_category->setEnabled(QtWarningMsg, m_saved->at(1));
            m_category->setEnabled(QtCriticalMsg, m_saved->at(2));
            m_category->setEnabled(QtInfoMsg, m_saved->at(4));
        }
    }

    bool isDebugEnabled() const { return isEnabled(QtDebugMsg); }
    bool isWarningEnabled() const { return isEnabled(QtWarningMsg); }
    bool isCriticalEnabled() const { return isEnabled(QtCriticalMsg); }
    bool isInfoEnabled() const { return isEnabled(QtInfoMsg); }

private:
    QString m_name;
    QLoggingCategory *m_category{nullptr};
    std::optional<std::array<bool, 5>> m_originalSettings;
    std::optional<std::array<bool, 5>> m_saved;
    QColor m_color;
    bool m_useOriginal{false};
};

class LoggingCategoryModel : public QAbstractListModel
{
    Q_OBJECT
public:
    LoggingCategoryModel(QObject *parent)
        : QAbstractListModel(parent)
    {
        auto newCategory = [this](QLoggingCategory *category) {
            QString name = QString::fromUtf8(category->categoryName());
            auto itExists = std::find_if(m_categories.begin(),
                                         m_categories.end(),
                                         [name](const auto &cat) { return name == cat.name(); });

            if (itExists != m_categories.end()) {
                itExists->setLogCategory(category);
            } else {
                LoggingCategoryEntry entry(category);
                append(entry);
            }
        };

        for (QLoggingCategory *cat : LogCategoryRegistry::instance().categories())
            newCategory(cat);

        connect(&LogCategoryRegistry::instance(),
                &LogCategoryRegistry::newLogCategory,
                this,
                newCategory);

        LogCategoryRegistry::instance().start();
    };

    ~LoggingCategoryModel() override;
    enum Column { Color, Name, Debug, Warning, Critical, Fatal, Info };

    enum Role { OriginalStateRole = Qt::UserRole + 1 };

    void append(const LoggingCategoryEntry &entry);
    int columnCount(const QModelIndex &) const final { return 7; }
    int rowCount(const QModelIndex & = QModelIndex()) const final { return m_categories.size(); }
    QVariant data(const QModelIndex &index, int role) const final;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) final;
    Qt::ItemFlags flags(const QModelIndex &index) const final;
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const final;

    void saveEnabledCategoryPreset() const;
    void loadAndUpdateFromPreset();

    void setUseOriginal(bool useOriginal)
    {
        if (useOriginal != m_useOriginal) {
            beginResetModel();
            for (auto &entry : m_categories)
                entry.setUseOriginal(useOriginal);

            m_useOriginal = useOriginal;
            endResetModel();
        }
    }

    const QList<LoggingCategoryEntry> &categories() const { return m_categories; }

private:
    QList<LoggingCategoryEntry> m_categories;
    bool m_useOriginal{false};
};

LoggingCategoryModel::~LoggingCategoryModel() {}

void LoggingCategoryModel::append(const LoggingCategoryEntry &entry)
{
    beginInsertRows(QModelIndex(), m_categories.size(), m_categories.size() + 1);
    m_categories.push_back(entry);
    endInsertRows();
}

QVariant LoggingCategoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    if (index.column() == Column::Name && role == Qt::DisplayRole) {
        return m_categories.at(index.row()).name();
    } else if (role == Qt::DecorationRole && index.column() == Column::Color) {
        const QColor color = m_categories.at(index.row()).color();
        if (color.isValid())
            return color;

        static const QColor defaultColor = Utils::creatorTheme()->palette().text().color();
        return defaultColor;
    } else if (index.column() >= Column::Debug && index.column() <= Column::Info) {
        if (role == Qt::CheckStateRole) {
            const LoggingCategoryEntry &entry = m_categories.at(index.row());
            const bool isEnabled = entry.isEnabled(
                static_cast<QtMsgType>(index.column() - Column::Debug));
            return isEnabled ? Qt::Checked : Qt::Unchecked;
        } else if (role == OriginalStateRole) {
            const LoggingCategoryEntry &entry = m_categories.at(index.row());
            return entry.isEnabledOriginally(static_cast<QtMsgType>(index.column() - Column::Debug))
                       ? Qt::Checked
                       : Qt::Unchecked;
        }
    }
    return {};
}

bool LoggingCategoryModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    if (role == Qt::CheckStateRole && index.column() >= Column::Debug
        && index.column() <= Column::Info) {
        QtMsgType msgType = static_cast<QtMsgType>(index.column() - Column::Debug);
        auto &entry = m_categories[index.row()];
        bool isEnabled = entry.isEnabled(msgType);

        const Qt::CheckState current = isEnabled ? Qt::Checked : Qt::Unchecked;

        if (current != value.toInt()) {
            entry.setEnabled(msgType, value.toInt() == Qt::Checked);
            return true;
        }
    } else if (role == Qt::DecorationRole && index.column() == Column::Color) {
        auto &category = m_categories[index.row()];
        QColor currentColor = category.color();
        QColor color = value.value<QColor>();
        if (color.isValid() && color != currentColor) {
            category.setColor(color);
            setCategoryColor(category.name(), color);
            emit dataChanged(index, index, {Qt::DisplayRole});
            return true;
        }
    }

    return false;
}

Qt::ItemFlags LoggingCategoryModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    if (index.column() == LoggingCategoryModel::Column::Fatal)
        return Qt::NoItemFlags;

    if (index.column() == Column::Name || index.column() == Column::Color)
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    if (m_useOriginal)
        return Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
}

QVariant LoggingCategoryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal && section >= 0 && section < 8) {
        switch (section) {
        case Column::Name:
            return Tr::tr("Category");
        case Column::Color:
            return Tr::tr("Color");
        case Column::Debug:
            return Tr::tr("Debug");
        case Column::Warning:
            return Tr::tr("Warning");
        case Column::Critical:
            return Tr::tr("Critical");
        case Column::Fatal:
            return Tr::tr("Fatal");
        case Column::Info:
            return Tr::tr("Info");
        }
    }
    return {};
}

class LogEntry
{
public:
    QString timestamp;
    QString type;
    QString category;
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

class LoggingEntryModel : public Utils::ListModel<LogEntry>
{
public:
    ~LoggingEntryModel() { qInstallMessageHandler(m_originalMessageHandler); }

    static void logMessageHandler(QtMsgType type,
                                  const QMessageLogContext &context,
                                  const QString &mssg)
    {
        instance().msgHandler(type, context, mssg);
    }

    static QVariant logEntryDataAccessor(const LogEntry &entry, int column, int role)
    {
        if (column >= 0 && column <= 3 && (role == Qt::DisplayRole || role == Qt::ToolTipRole)) {
            switch (column) {
            case 0:
                return entry.timestamp;
            case 1:
                return entry.category;
            case 2:
                return entry.type;
            case 3: {
                if (role == Qt::ToolTipRole)
                    return entry.message;
                return entry.message.left(1000);
            }
            }
        }
        if (role == Qt::TextAlignmentRole)
            return Qt::AlignTop;
        if (column == 1 && role == Qt::ForegroundRole)
            return colorForCategory(entry.category);
        return {};
    }

    static LoggingEntryModel &instance()
    {
        static LoggingEntryModel model;
        return model;
    }

    void setEnabled(bool enabled) { m_enabled = enabled; }

private:
    LoggingEntryModel()
    {
        setHeader({Tr::tr("Timestamp"), Tr::tr("Category"), Tr::tr("Type"), Tr::tr("Message")});
        setDataAccessor(&logEntryDataAccessor);

        m_originalMessageHandler = qInstallMessageHandler(logMessageHandler);
    }

    void msgHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
    {
        if (!m_enabled) {
            m_originalMessageHandler(type, context, msg);
            return;
        }

        if (!context.category) {
            m_originalMessageHandler(type, context, msg);
            return;
        }

        const QString category = QString::fromLocal8Bit(context.category);

        const QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");

        if (rowCount() >= 1000000) // limit log to 1000000 items
            destroyItem(itemForIndex(index(0, 0)));

        appendItem(LogEntry{timestamp, messageTypeToString(type), category, msg});
    }

private:
    QtMessageHandler m_originalMessageHandler{nullptr};
    bool m_enabled{true};
};

class LoggingViewManagerWidget : public QDialog
{
public:
    ~LoggingViewManagerWidget() { LoggingEntryModel::instance().setEnabled(false); }

    static LoggingViewManagerWidget *instance()
    {
        static QPointer<LoggingViewManagerWidget> instance = new LoggingViewManagerWidget(
            Core::ICore::dialogParent());
        return instance;
    }

protected:
    void showEvent(QShowEvent *) override
    {
        if (!m_stopLog->isChecked())
            m_categoryModel->setUseOriginal(false);

        LoggingEntryModel::instance().setEnabled(!m_stopLog->isChecked());
    }
    void hideEvent(QHideEvent *) override
    {
        m_categoryModel->setUseOriginal(true);
        LoggingEntryModel::instance().setEnabled(false);
    }

private:
    explicit LoggingViewManagerWidget(QWidget *parent);

private:
    void showLogViewContextMenu(const QPoint &pos) const;
    void showLogCategoryContextMenu(const QPoint &pos) const;
    void saveLoggingsToFile() const;
    QSortFilterProxyModel *m_sortFilterModel = nullptr;
    LoggingCategoryModel *m_categoryModel = nullptr;
    Utils::BaseTreeView *m_logView = nullptr;
    Utils::BaseTreeView *m_categoryView = nullptr;
    QToolButton *m_timestamps = nullptr;
    QToolButton *m_messageTypes = nullptr;
    QToolButton *m_stopLog = nullptr;
};

LoggingViewManagerWidget::LoggingViewManagerWidget(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(Tr::tr("Logging Category Viewer"));
    auto save = new QToolButton;
    save->setIcon(Utils::Icons::SAVEFILE.icon());
    save->setToolTip(Tr::tr("Save Log"));

    auto clean = new QToolButton;
    clean->setIcon(Utils::Icons::CLEAN.icon());
    clean->setToolTip(Tr::tr("Clear"));

    m_stopLog = new QToolButton;
    m_stopLog->setIcon(Utils::Icons::STOP_SMALL.icon());
    m_stopLog->setToolTip(Tr::tr("Stop Logging"));
    m_stopLog->setCheckable(true);

    auto qtInternal = new QToolButton;
    qtInternal->setIcon(Core::Icons::QTLOGO.icon());
    qtInternal->setToolTip(Tr::tr("Filter Qt Internal Log Categories"));
    qtInternal->setCheckable(false);

    auto autoScroll = new QToolButton;
    autoScroll->setIcon(Utils::Icons::ARROW_DOWN.icon());
    autoScroll->setToolTip(Tr::tr("Auto Scroll"));
    autoScroll->setCheckable(true);
    autoScroll->setChecked(true);

    m_timestamps = new QToolButton;
    auto icon = Utils::Icon({{":/utils/images/stopwatch.png", Utils::Theme::PanelTextColorMid}},
                            Utils::Icon::Tint);
    m_timestamps->setIcon(icon.icon());
    m_timestamps->setToolTip(Tr::tr("Timestamps"));
    m_timestamps->setCheckable(true);
    m_timestamps->setChecked(true);

    m_messageTypes = new QToolButton;
    icon = Utils::Icon({{":/utils/images/message.png", Utils::Theme::PanelTextColorMid}},
                       Utils::Icon::Tint);
    m_messageTypes->setIcon(icon.icon());
    m_messageTypes->setToolTip(Tr::tr("Message Types"));
    m_messageTypes->setCheckable(true);
    m_messageTypes->setChecked(false);

    m_logView = new Utils::BaseTreeView;
    m_logView->setModel(&LoggingEntryModel::instance());
    m_logView->setUniformRowHeights(true);
    m_logView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_logView->setFrameStyle(QFrame::Box);
    m_logView->setAttribute(Qt::WA_MacShowFocusRect, false);
    m_logView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_logView->setColumnHidden(2, true);
    m_logView->setContextMenuPolicy(Qt::CustomContextMenu);

    m_categoryModel = new LoggingCategoryModel(this);
    m_sortFilterModel = new QSortFilterProxyModel(m_categoryModel);
    m_sortFilterModel->setSourceModel(m_categoryModel);
    m_sortFilterModel->sort(LoggingCategoryModel::Column::Name);
    m_sortFilterModel->setSortRole(Qt::DisplayRole);
    m_sortFilterModel->setFilterKeyColumn(LoggingCategoryModel::Column::Name);

    m_categoryView = new Utils::BaseTreeView;
    m_categoryView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_categoryView->setFrameStyle(QFrame::Box);
    m_categoryView->setAttribute(Qt::WA_MacShowFocusRect, false);
    m_categoryView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_categoryView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_categoryView->setModel(m_sortFilterModel);

    for (int i = LoggingCategoryModel::Column::Color; i < LoggingCategoryModel::Column::Info; i++)
        m_categoryView->resizeColumnToContents(i);

    auto filterEdit = new Utils::FancyLineEdit;
    filterEdit->setHistoryCompleter("LogFilterCompletionHistory");
    filterEdit->setFiltering(true);
    filterEdit->setPlaceholderText(Tr::tr("Filter categories by regular expression"));
    filterEdit->setText("^(?!qt\\.).+");
    filterEdit->setValidationFunction(
        [](const QString &input) {
            return Utils::asyncRun([input]() -> Utils::expected_str<QString> {
                QRegularExpression re(input);
                if (re.isValid())
                    return input;

                return Utils::make_unexpected(
                    Tr::tr("Invalid regular expression: %1").arg(re.errorString()));
            });
        });

    QSplitter *splitter{nullptr};

    using namespace Layouting;
    // clang-format off
    Column {
        Splitter {
            bindTo(&splitter),
            Column {
                noMargin(),
                Row {
                    spacing(0),
                    save,
                    clean,
                    m_stopLog,
                    autoScroll,
                    m_timestamps,
                    m_messageTypes,
                    st,
                },
                m_logView
            },
            Column {
                noMargin(),
                Row {
                    qtInternal,
                    filterEdit,
                },
                m_categoryView,
            }
        }
    }.attachTo(this);
    // clang-format on

    splitter->setOrientation(Qt::Horizontal);

    resize(800, 300);

    connect(
        &LoggingEntryModel::instance(),
        &LoggingEntryModel::rowsInserted,
        this,
        [this, autoScroll] {
            if (autoScroll->isChecked())
                m_logView->scrollToBottom();
        },
        Qt::QueuedConnection);

    connect(m_categoryView,
            &QAbstractItemView::activated,
            m_sortFilterModel,
            [this](const QModelIndex &index) {
                const QVariant value = m_sortFilterModel->data(index, Qt::DecorationRole);
                if (!value.isValid())
                    return;
                const QColor original = value.value<QColor>();
                if (!original.isValid())
                    return;
                QColor changed = QColorDialog::getColor(original, this);
                if (!changed.isValid())
                    return;
                if (original != changed)
                    m_sortFilterModel->setData(index, changed, Qt::DecorationRole);
            });
    connect(save, &QToolButton::clicked, this, &LoggingViewManagerWidget::saveLoggingsToFile);
    connect(m_logView,
            &QAbstractItemView::customContextMenuRequested,
            this,
            &LoggingViewManagerWidget::showLogViewContextMenu);
    connect(m_categoryView,
            &QAbstractItemView::customContextMenuRequested,
            this,
            &LoggingViewManagerWidget::showLogCategoryContextMenu);
    connect(clean,
            &QToolButton::clicked,
            &LoggingEntryModel::instance(),
            &Utils::ListModel<LogEntry>::clear);
    connect(m_stopLog, &QToolButton::toggled, this, [this](bool checked) {
        LoggingEntryModel::instance().setEnabled(!checked);

        if (checked) {
            m_stopLog->setIcon(Utils::Icons::RUN_SMALL.icon());
            m_stopLog->setToolTip(Tr::tr("Start Logging"));
            m_categoryModel->setUseOriginal(true);
        } else {
            m_stopLog->setIcon(Utils::Icons::STOP_SMALL.icon());
            m_stopLog->setToolTip(Tr::tr("Stop Logging"));
            m_categoryModel->setUseOriginal(false);
        }
    });

    m_sortFilterModel->setFilterRegularExpression("^(?!qt\\.).+");

    connect(qtInternal, &QToolButton::clicked, filterEdit, [filterEdit] {
        filterEdit->setText("^(?!qt\\.).+");
    });

    connect(filterEdit,
            &Utils::FancyLineEdit::textChanged,
            m_sortFilterModel,
            [this](const QString &f) {
                QRegularExpression re(f);
                if (re.isValid())
                    m_sortFilterModel->setFilterRegularExpression(f);
            });

    connect(m_timestamps, &QToolButton::toggled, this, [this](bool checked) {
        m_logView->setColumnHidden(0, !checked);
    });
    connect(m_messageTypes, &QToolButton::toggled, this, [this](bool checked) {
        m_logView->setColumnHidden(2, !checked);
    });

    ICore::registerWindow(this, Context("Qtc.LogViewer"));
}

void LoggingViewManagerWidget::showLogViewContextMenu(const QPoint &pos) const
{
    QMenu m;
    auto copy = new QAction(Tr::tr("Copy Selected Logs"), &m);
    m.addAction(copy);
    auto copyAll = new QAction(Tr::tr("Copy All"), &m);
    m.addAction(copyAll);
    connect(copy, &QAction::triggered, &m, [this] {
        auto selectionModel = m_logView->selectionModel();
        QString copied;
        const bool useTS = m_timestamps->isChecked();
        const bool useLL = m_messageTypes->isChecked();
        for (int row = 0, end = LoggingEntryModel::instance().rowCount(); row < end; ++row) {
            if (selectionModel->isRowSelected(row, QModelIndex()))
                copied.append(LoggingEntryModel::instance().dataAt(row).outputLine(useTS, useLL));
        }

        Utils::setClipboardAndSelection(copied);
    });
    connect(copyAll, &QAction::triggered, &m, [this] {
        QString copied;
        const bool useTS = m_timestamps->isChecked();
        const bool useLL = m_messageTypes->isChecked();

        for (int row = 0, end = LoggingEntryModel::instance().rowCount(); row < end; ++row)
            copied.append(LoggingEntryModel::instance().dataAt(row).outputLine(useTS, useLL));

        Utils::setClipboardAndSelection(copied);
    });
    m.exec(m_logView->mapToGlobal(pos));
}

void LoggingViewManagerWidget::showLogCategoryContextMenu(const QPoint &pos) const
{
    QModelIndex idx = m_categoryView->indexAt(pos);

    QMenu m;
    auto uncheckAll = new QAction(Tr::tr("Uncheck All"), &m);
    auto resetAll = new QAction(Tr::tr("Reset All"), &m);

    auto isTypeColumn = [](int column) {
        return column >= LoggingCategoryModel::Column::Debug
               && column <= LoggingCategoryModel::Column::Info;
    };

    auto setChecked = [this](std::initializer_list<LoggingCategoryModel::Column> columns,
                             Qt::CheckState checked) {
        for (int row = 0, count = m_sortFilterModel->rowCount(); row < count; ++row) {
            for (int column : columns) {
                m_sortFilterModel->setData(m_sortFilterModel->index(row, column),
                                           checked,
                                           Qt::CheckStateRole);
            }
        }
    };
    auto resetToOriginal = [this](std::initializer_list<LoggingCategoryModel::Column> columns) {
        for (int row = 0, count = m_sortFilterModel->rowCount(); row < count; ++row) {
            for (int column : columns) {
                const QModelIndex id = m_sortFilterModel->index(row, column);
                m_sortFilterModel->setData(id,
                                           id.data(LoggingCategoryModel::OriginalStateRole),
                                           Qt::CheckStateRole);
            }
        }
    };

    if (idx.isValid() && isTypeColumn(idx.column())) {
        const LoggingCategoryModel::Column column = static_cast<LoggingCategoryModel::Column>(
            idx.column());
        bool isChecked = idx.data(Qt::CheckStateRole).toInt() == Qt::Checked;
        const QString uncheckText = isChecked ? Tr::tr("Uncheck All %1") : Tr::tr("Check All %1");

        uncheckAll->setText(uncheckText.arg(messageTypeToString(
            static_cast<QtMsgType>(column - LoggingCategoryModel::Column::Debug))));
        resetAll->setText(Tr::tr("Reset All %1")
                              .arg(messageTypeToString(static_cast<QtMsgType>(
                                  column - LoggingCategoryModel::Column::Debug))));

        Qt::CheckState newState = isChecked ? Qt::Unchecked : Qt::Checked;

        connect(uncheckAll,
                &QAction::triggered,
                m_sortFilterModel,
                [setChecked, column, newState]() { setChecked({column}, newState); });

        connect(resetAll, &QAction::triggered, m_sortFilterModel, [resetToOriginal, column]() {
            resetToOriginal({column});
        });

    } else {
        // No need to add Fatal here, as it is read-only
        static auto allColumns = {LoggingCategoryModel::Column::Debug,
                                  LoggingCategoryModel::Column::Warning,
                                  LoggingCategoryModel::Column::Critical,
                                  LoggingCategoryModel::Column::Info};

        connect(uncheckAll, &QAction::triggered, m_sortFilterModel, [setChecked]() {
            setChecked(allColumns, Qt::Unchecked);
        });
        connect(resetAll, &QAction::triggered, m_sortFilterModel, [resetToOriginal]() {
            resetToOriginal(allColumns);
        });
    }

    // minimal load/save - plugins could later provide presets on their own?
    auto savePreset = new QAction(Tr::tr("Save Enabled as Preset..."), &m);
    m.addAction(savePreset);
    auto loadPreset = new QAction(Tr::tr("Update from Preset..."), &m);
    m.addAction(loadPreset);
    m.addAction(uncheckAll);
    m.addAction(resetAll);
    connect(savePreset,
            &QAction::triggered,
            m_categoryModel,
            &LoggingCategoryModel::saveEnabledCategoryPreset);
    connect(loadPreset,
            &QAction::triggered,
            m_categoryModel,
            &LoggingCategoryModel::loadAndUpdateFromPreset);
    m.exec(m_categoryView->mapToGlobal(pos));
}

void LoggingViewManagerWidget::saveLoggingsToFile() const
{
    const Utils::FilePath fp = Utils::FileUtils::getSaveFilePath(ICore::dialogParent(),
                                                                 Tr::tr("Save Logs As"),
                                                                 {},
                                                                 "*.log");
    if (fp.isEmpty())
        return;

    const bool useTS = m_timestamps->isChecked();
    const bool useLL = m_messageTypes->isChecked();
    QFile file(fp.path());
    if (file.open(QIODevice::WriteOnly)) {
        for (int row = 0, end = LoggingEntryModel::instance().rowCount(); row < end; ++row) {
            qint64 res = file.write(
                LoggingEntryModel::instance().dataAt(row).outputLine(useTS, useLL).toUtf8());
            if (res == -1) {
                QMessageBox::critical(ICore::dialogParent(),
                                      Tr::tr("Error"),
                                      Tr::tr("Failed to write logs to \"%1\".")
                                          .arg(fp.toUserOutput()));
                break;
            }
        }
        file.close();
    } else {
        QMessageBox::critical(ICore::dialogParent(),
                              Tr::tr("Error"),
                              Tr::tr("Failed to open file \"%1\" for writing logs.")
                                  .arg(fp.toUserOutput()));
    }
}

void LoggingCategoryModel::saveEnabledCategoryPreset() const
{
    Utils::FilePath fp = Utils::FileUtils::getSaveFilePath(ICore::dialogParent(),
                                                           Tr::tr("Save Enabled Categories As..."),
                                                           {},
                                                           "*.json");
    if (fp.isEmpty())
        return;

    auto minLevel = [](const LoggingCategoryEntry &logCategory) {
        for (int i = QtDebugMsg; i <= QtInfoMsg; i++) {
            if (logCategory.isEnabled(static_cast<QtMsgType>(i)))
                return i;
        }
        return QtInfoMsg + 1;
    };

    QJsonArray array;

    for (const auto &item : m_categories) {
        QJsonObject itemObj;
        itemObj.insert("name", item.name());
        QJsonObject entryObj;
        entryObj.insert("level", minLevel(item));
        if (item.color().isValid())
            entryObj.insert("color", item.color().name(QColor::HexArgb));

        QVariantMap levels = {{"Debug", item.isDebugEnabled()},
                              {"Warning", item.isWarningEnabled()},
                              {"Critical", item.isCriticalEnabled()},
                              {"Info", item.isInfoEnabled()}};
        entryObj.insert("levels", QJsonValue::fromVariant(levels));

        itemObj.insert("entry", entryObj);
        array.append(itemObj);
    }
    QJsonDocument doc(array);
    if (!fp.writeFileContents(doc.toJson(QJsonDocument::Compact)))
        QMessageBox::critical(ICore::dialogParent(),
                              Tr::tr("Error"),
                              Tr::tr("Failed to write preset file \"%1\".").arg(fp.toUserOutput()));
}

void LoggingCategoryModel::loadAndUpdateFromPreset()
{
    Utils::FilePath fp = Utils::FileUtils::getOpenFilePath(ICore::dialogParent(),
                                                           Tr::tr("Load Enabled Categories From"));
    if (fp.isEmpty())
        return;
    // read file, update categories
    const Utils::expected_str<QByteArray> contents = fp.fileContents();
    if (!contents) {
        QMessageBox::critical(ICore::dialogParent(),
                              Tr::tr("Error"),
                              Tr::tr("Failed to open preset file \"%1\" for reading.")
                                  .arg(fp.toUserOutput()));
        return;
    }
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(*contents, &error);
    if (error.error != QJsonParseError::NoError) {
        QMessageBox::critical(ICore::dialogParent(),
                              Tr::tr("Error"),
                              Tr::tr("Failed to read preset file \"%1\": %2")
                                  .arg(fp.toUserOutput())
                                  .arg(error.errorString()));
        return;
    }
    bool formatError = false;
    QList<SavedEntry> presetItems;
    if (doc.isArray()) {
        const QJsonArray array = doc.array();
        for (const QJsonValue &value : array) {
            if (!value.isObject()) {
                formatError = true;
                break;
            }
            const QJsonObject itemObj = value.toObject();
            Utils::expected_str<SavedEntry> item = SavedEntry::fromJson(itemObj);
            if (!item) {
                formatError = true;
                break;
            }
            presetItems.append(*item);
        }
    } else {
        formatError = true;
    }

    if (formatError) {
        QMessageBox::critical(ICore::dialogParent(),
                              Tr::tr("Error"),
                              Tr::tr("Unexpected preset file format."));
    }

    int idx = 0;

    for (auto it = presetItems.begin(); it != presetItems.end(); ++it, ++idx) {
        QList<LoggingCategoryEntry>::iterator itExisting
            = std::find_if(m_categories.begin(),
                           m_categories.end(),
                           [e = *it](const LoggingCategoryEntry &cat) {
                               return cat.name() == e.name;
                           });

        if (it->color.isValid())
            setCategoryColor(it->name, it->color);

        if (itExisting != m_categories.end()) {
            itExisting->setSaved(*it);
            emit dataChanged(createIndex(idx, Column::Color), createIndex(idx, Column::Info));
        } else {
            LoggingCategoryEntry newEntry(*it);
            append(newEntry);
        }
    }
}

QColor colorForCategory(const QString &category)
{
    auto entry = s_categoryColor.find(category);
    if (entry == s_categoryColor.end())
        return Utils::creatorTheme()->palette().text().color();
    return entry.value();
}

void setCategoryColor(const QString &category, const QColor &color)
{
    const QColor baseColor = Utils::creatorTheme()->palette().text().color();
    if (color != baseColor)
        s_categoryColor.insert(category, color);
    else
        s_categoryColor.remove(category);
}

static bool wasLogViewerShown = false;

void LoggingViewer::showLoggingView()
{
    LoggingViewManagerWidget *staticLogWidget = LoggingViewManagerWidget::instance();
    QTC_ASSERT(staticLogWidget, return);

    staticLogWidget->show();
    staticLogWidget->raise();
    staticLogWidget->activateWindow();

    wasLogViewerShown = true;
}

void LoggingViewer::hideLoggingView()
{
    if (!wasLogViewerShown)
        return;

    LoggingViewManagerWidget *staticLogWidget = LoggingViewManagerWidget::instance();
    QTC_ASSERT(staticLogWidget, return);
    staticLogWidget->close();
    delete staticLogWidget;
}

} // namespace Core::Internal

#include "loggingviewer.moc"
