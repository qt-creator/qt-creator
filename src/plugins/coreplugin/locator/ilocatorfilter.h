// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/core_global.h>

#include <utils/filepath.h>
#include <utils/id.h>
#include <utils/link.h>
#include <utils/tasktree.h>

#include <QFutureInterface>
#include <QIcon>
#include <QMetaType>
#include <QVariant>
#include <QKeySequence>

#include <optional>

namespace Utils::Tasking { class TaskItem; }

namespace Core {

namespace Internal { class Locator; }

class ILocatorFilter;

class AcceptResult
{
public:
    QString newText;
    int selectionStart = -1;
    int selectionLength = 0;
};

class LocatorFilterEntry
{
public:
    struct HighlightInfo {
        enum DataType {
            DisplayName,
            ExtraInfo
        };

        HighlightInfo() = default;

        HighlightInfo(QVector<int> startIndex,
                      QVector<int> length,
                      DataType type = DataType::DisplayName)
        {
            if (type == DataType::DisplayName) {
                startsDisplay = startIndex;
                lengthsDisplay = length;
            } else {
                startsExtraInfo = startIndex;
                lengthsExtraInfo = length;
            }
        }

        HighlightInfo(int startIndex, int length, DataType type = DataType::DisplayName)
            : HighlightInfo(QVector<int>{startIndex}, QVector<int>{length}, type)
        { }

        QVector<int> starts(DataType type = DataType::DisplayName) const
        {
            return type == DataType::DisplayName ? startsDisplay : startsExtraInfo;
        };

        QVector<int> lengths(DataType type = DataType::DisplayName) const
        {
            return type == DataType::DisplayName ? lengthsDisplay : lengthsExtraInfo;
        };

        QVector<int> startsDisplay;
        QVector<int> lengthsDisplay;
        QVector<int> startsExtraInfo;
        QVector<int> lengthsExtraInfo;
    };

    LocatorFilterEntry() = default;

    LocatorFilterEntry(ILocatorFilter *fromFilter, const QString &name)
        : filter(fromFilter)
        , displayName(name)
    {}

    using Acceptor = std::function<AcceptResult()>;
    /* backpointer to creating filter */
    ILocatorFilter *filter = nullptr;
    /* displayed string */
    QString displayName;
    /* extra information displayed in parentheses and light-gray next to display name (optional)*/
    QString displayExtra;
    /* extra information displayed in light-gray in a second column (optional) */
    QString extraInfo;
    /* additional tooltip */
    QString toolTip;
    /* called by locator widget on accept. By default, when acceptor is empty,
       EditorManager::openEditor(LocatorFilterEntry) will be used instead. */
    Acceptor acceptor;
    /* can be used by the filter to save more information about the entry */
    /* Replaced by acceptor - DON'T USE IN NEW CODE, IT'S GOING TO BE REMOVED, SOON... */
    QVariant internalData;
    /* icon to display along with the entry */
    std::optional<QIcon> displayIcon;
    /* file path, if the entry is related to a file, is used e.g. for resolving a file icon */
    Utils::FilePath filePath;
    /* highlighting support */
    HighlightInfo highlightInfo;
    // Should be used only when accept() calls BaseFileFilter::openEditorAt()
    std::optional<Utils::Link> linkForEditor;
    static bool compareLexigraphically(const Core::LocatorFilterEntry &lhs,
                                       const Core::LocatorFilterEntry &rhs)
    {
        const int cmp = lhs.displayName.compare(rhs.displayName);
        if (cmp < 0)
            return true;
        if (cmp > 0)
            return false;
        return lhs.extraInfo < rhs.extraInfo;
    }
};

class CORE_EXPORT LocatorMatcherTask final
{
public:
    using InputData = QString;
    using OutputData = QList<LocatorFilterEntry>;
    class Storage
    {
    public:
        InputData input;
        OutputData output;
    };
    // The main task. Initial data taken from storage.input field.
    // Results reporting is done through the storage.output field.
    Utils::Tasking::TaskItem task = Utils::Tasking::Group{};
    // When setting up the task, take the input data from storage.input field.
    // When task is done, report results by updating the storage.output field.
    // When constructing the task, don't place the storage inside the task above.
    Utils::Tasking::TreeStorage<Storage> storage;
};

using LocatorMatcherTaskCreator = std::function<QList<LocatorMatcherTask>()>;
class LocatorMatcherPrivate;

enum class MatcherType {
    AllSymbols,
    Classes,
    Functions,
    CurrentDocumentSymbols
};

class CORE_EXPORT LocatorMatcher final : public QObject
{
    Q_OBJECT

public:
    LocatorMatcher();
    ~LocatorMatcher();
    void setTasks(const QList<LocatorMatcherTask> &tasks);
    void setInputData(const LocatorMatcherTask::InputData &inputData);
    void setParallelLimit(int limit); // by default 0 = parallel
    void start();
    void stop();

    bool isRunning() const;
    // Total data collected so far, even when running.
    LocatorMatcherTask::OutputData outputData() const;

    // Note: Starts internal event loop.
    static LocatorMatcherTask::OutputData runBlocking(const QList<LocatorMatcherTask> &tasks,
                                                      const LocatorMatcherTask::InputData &input,
                                                      int parallelLimit = 0);

    static void addMatcherCreator(MatcherType type, const LocatorMatcherTaskCreator &creator);
    static QList<LocatorMatcherTask> matchers(MatcherType type);

signals:
    void serialOutputDataReady(const LocatorMatcherTask::OutputData &serialOutputData);
    void done(bool success);

private:
    std::unique_ptr<LocatorMatcherPrivate> d;
};

class CORE_EXPORT ILocatorFilter : public QObject
{
    Q_OBJECT

public:
    enum class MatchLevel {
        Best = 0,
        Better,
        Good,
        Normal,
        Count
    };

    enum Priority {Highest = 0, High = 1, Medium = 2, Low = 3};

    ILocatorFilter(QObject *parent = nullptr);
    ~ILocatorFilter() override;

    Utils::Id id() const;
    Utils::Id actionId() const;

    QString displayName() const;
    void setDisplayName(const QString &displayString);

    QString description() const;
    void setDescription(const QString &description);

    Priority priority() const;

    QString shortcutString() const;
    void setDefaultShortcutString(const QString &shortcut);
    void setShortcutString(const QString &shortcut);

    QKeySequence defaultKeySequence() const;
    void setDefaultKeySequence(const QKeySequence &sequence);

    std::optional<QString> defaultSearchText() const;
    void setDefaultSearchText(const QString &defaultSearchText);

    virtual void prepareSearch(const QString &entry);

    virtual QList<LocatorFilterEntry> matchesFor(QFutureInterface<LocatorFilterEntry> &future, const QString &entry) = 0;

    virtual void accept(const LocatorFilterEntry &selection, QString *newText,
                        int *selectionStart, int *selectionLength) const;

    virtual QByteArray saveState() const;
    virtual void restoreState(const QByteArray &state);

    virtual bool openConfigDialog(QWidget *parent, bool &needsRefresh);
    bool isConfigurable() const;

    bool isIncludedByDefault() const;
    void setDefaultIncludedByDefault(bool includedByDefault);
    void setIncludedByDefault(bool includedByDefault);

    bool isHidden() const;

    bool isEnabled() const;

    static Qt::CaseSensitivity caseSensitivity(const QString &str);
    static QRegularExpression createRegExp(const QString &text,
                                           Qt::CaseSensitivity caseSensitivity = Qt::CaseInsensitive,
                                           bool multiWord = false);
    static LocatorFilterEntry::HighlightInfo highlightInfo(const QRegularExpressionMatch &match,
        LocatorFilterEntry::HighlightInfo::DataType dataType = LocatorFilterEntry::HighlightInfo::DisplayName);

    static QString msgConfigureDialogTitle();
    static QString msgPrefixLabel();
    static QString msgPrefixToolTip();
    static QString msgIncludeByDefault();
    static QString msgIncludeByDefaultToolTip();

public slots:
    void setEnabled(bool enabled);

signals:
    void enabledChanged(bool enabled);

protected:
    void setHidden(bool hidden);
    void setId(Utils::Id id);
    void setPriority(Priority priority);
    void setConfigurable(bool configurable);
    bool openConfigDialog(QWidget *parent, QWidget *additionalWidget);

    virtual void saveState(QJsonObject &object) const;
    virtual void restoreState(const QJsonObject &object);

    void setRefreshRecipe(const std::optional<Utils::Tasking::TaskItem> &recipe);
    std::optional<Utils::Tasking::TaskItem> refreshRecipe() const;

    static bool isOldSetting(const QByteArray &state);

private:
    friend class Internal::Locator;
    static const QList<ILocatorFilter *> allLocatorFilters();

    Utils::Id m_id;
    QString m_shortcut;
    Priority m_priority = Medium;
    QString m_displayName;
    QString m_description;
    QString m_defaultShortcut;
    std::optional<QString> m_defaultSearchText;
    std::optional<Utils::Tasking::TaskItem> m_refreshRecipe;
    QKeySequence m_defaultKeySequence;
    bool m_defaultIncludedByDefault = false;
    bool m_includedByDefault = m_defaultIncludedByDefault;
    bool m_hidden = false;
    bool m_enabled = true;
    bool m_isConfigurable = true;
};

} // namespace Core
