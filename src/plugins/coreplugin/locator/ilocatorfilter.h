// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../core_global.h"

#include <solutions/tasking/tasktree.h>

#include <utils/filepath.h>
#include <utils/id.h>
#include <utils/link.h>

#include <QIcon>
#include <QKeySequence>

#include <optional>

QT_BEGIN_NAMESPACE
template <typename T>
class QFuture;
QT_END_NAMESPACE

namespace Core {

namespace Internal {
class Locator;
class LocatorWidget;
}

class ILocatorFilter;
class LocatorStoragePrivate;
class LocatorFileCachePrivate;

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

    using Acceptor = std::function<AcceptResult()>;
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

using LocatorFilterEntries = QList<LocatorFilterEntry>;

class CORE_EXPORT LocatorStorage final
{
public:
    LocatorStorage() = default;
    QString input() const;
    void reportOutput(const LocatorFilterEntries &outputData) const;

private:
    friend class LocatorMatcher;
    LocatorStorage(const std::shared_ptr<LocatorStoragePrivate> &priv) { d = priv; }
    void finalize() const;
    std::shared_ptr<LocatorStoragePrivate> d;
};

class CORE_EXPORT LocatorMatcherTask final
{
public:
    // The main task. Initial data (searchTerm) should be taken from storage.input().
    // Results reporting is done via the storage.reportOutput().
    Tasking::GroupItem task = Tasking::Group{};

    // When constructing the task, don't place the storage inside the task above.
    Tasking::TreeStorage<LocatorStorage> storage;
};

using LocatorMatcherTasks = QList<LocatorMatcherTask>;
using LocatorMatcherTaskCreator = std::function<LocatorMatcherTasks()>;
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
    void setTasks(const LocatorMatcherTasks &tasks);
    void setInputData(const QString &inputData);
    void setParallelLimit(int limit); // by default 0 = parallel
    void start();
    void stop();

    bool isRunning() const;
    // Total data collected so far, even when running.
    LocatorFilterEntries outputData() const;

    // Note: Starts internal event loop.
    static LocatorFilterEntries runBlocking(const LocatorMatcherTasks &tasks,
                                            const QString &input, int parallelLimit = 0);

    static void addMatcherCreator(MatcherType type, const LocatorMatcherTaskCreator &creator);
    static LocatorMatcherTasks matchers(MatcherType type);

signals:
    void serialOutputDataReady(const LocatorFilterEntries &serialOutputData);
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

    void setRefreshRecipe(const std::optional<Tasking::GroupItem> &recipe);
    std::optional<Tasking::GroupItem> refreshRecipe() const;

    static bool isOldSetting(const QByteArray &state);

private:
    virtual LocatorMatcherTasks matchers() = 0;

    friend class Internal::Locator;
    friend class Internal::LocatorWidget;
    static const QList<ILocatorFilter *> allLocatorFilters();

    Utils::Id m_id;
    QString m_shortcut;
    Priority m_priority = Medium;
    QString m_displayName;
    QString m_description;
    QString m_defaultShortcut;
    std::optional<QString> m_defaultSearchText;
    std::optional<Tasking::GroupItem> m_refreshRecipe;
    QKeySequence m_defaultKeySequence;
    bool m_defaultIncludedByDefault = false;
    bool m_includedByDefault = m_defaultIncludedByDefault;
    bool m_hidden = false;
    bool m_enabled = true;
    bool m_isConfigurable = true;
};

class CORE_EXPORT LocatorFileCache final
{
    Q_DISABLE_COPY_MOVE(LocatorFileCache)

public:
    // Always called from non-main thread.
    using FilePathsGenerator = std::function<Utils::FilePaths(const QFuture<void> &)>;
    // Always called from main thread.
    using GeneratorProvider = std::function<FilePathsGenerator()>;

    LocatorFileCache();

    void invalidate();
    void setFilePathsGenerator(const FilePathsGenerator &generator);
    void setFilePaths(const Utils::FilePaths &filePaths);
    void setGeneratorProvider(const GeneratorProvider &provider);

    std::optional<Utils::FilePaths> filePaths() const;

    static FilePathsGenerator filePathsGenerator(const Utils::FilePaths &filePaths);
    LocatorMatcherTask matcher() const;

    using MatchedEntries = std::array<LocatorFilterEntries, int(ILocatorFilter::MatchLevel::Count)>;
    static Utils::FilePaths processFilePaths(const QFuture<void> &future,
                                             const Utils::FilePaths &filePaths,
                                             bool hasPathSeparator,
                                             const QRegularExpression &regExp,
                                             const Utils::Link &inputLink,
                                             LocatorFileCache::MatchedEntries *entries);
private:
    std::shared_ptr<LocatorFileCachePrivate> d;
};

} // namespace Core
