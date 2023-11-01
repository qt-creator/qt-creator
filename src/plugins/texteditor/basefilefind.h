// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <coreplugin/find/ifindfilter.h>
#include <coreplugin/find/searchresultwindow.h>

#include <utils/filesearch.h>
#include <utils/searchresultitem.h>

#include <QFuture>

namespace Core {
class IEditor;
class SearchResult;
} // namespace Core

namespace Utils { class Process; }

namespace TextEditor {

namespace Internal {
class BaseFileFindPrivate;
class SearchEnginePrivate;
} // Internal

class FileFindParameters;

using FileContainerProvider = std::function<Utils::FileContainer()>;
using EditorOpener = std::function<Core::IEditor *(const Utils::SearchResultItem &,
                                                   const FileFindParameters &)>;
using SearchExecutor = std::function<QFuture<Utils::SearchResultItems>(const FileFindParameters &)>;

class TEXTEDITOR_EXPORT FileFindParameters
{
public:
    QString text;
    QStringList nameFilters;
    QStringList exclusionFilters;
    Utils::FilePath searchDir;
    Utils::FindFlags flags;
    FileContainerProvider fileContainerProvider = {};
    EditorOpener editorOpener = {};
    SearchExecutor searchExecutor = {};
};

using ProcessSetupHandler = std::function<void(Utils::Process &)>;
using ProcessOutputParser = std::function<Utils::SearchResultItems(
    const QFuture<void> &, const QString &, const std::optional<QRegularExpression> &)>;

// Call it from a non-main thread only, it's a blocking invocation.
void TEXTEDITOR_EXPORT searchInProcessOutput(QPromise<Utils::SearchResultItems> &promise,
                                             const FileFindParameters &parameters,
                                             const ProcessSetupHandler &processSetupHandler,
                                             const ProcessOutputParser &processOutputParser);

class BaseFileFind;

class TEXTEDITOR_EXPORT SearchEngine : public QObject
{
    Q_OBJECT
public:
    SearchEngine(QObject *parent = nullptr);
    ~SearchEngine() override;

    virtual QString title() const = 0;
    virtual QString toolTip() const = 0; // add %1 placeholder where the find flags should be put
    virtual QWidget *widget() const = 0;
    virtual void readSettings(Utils::QtcSettings *settings) = 0;
    virtual void writeSettings(Utils::QtcSettings *settings) const = 0;
    virtual SearchExecutor searchExecutor() const = 0;
    virtual EditorOpener editorOpener() const { return {}; }
    bool isEnabled() const;
    void setEnabled(bool enabled);

signals:
    void enabledChanged(bool enabled);

private:
    Internal::SearchEnginePrivate *d;
};

class TEXTEDITOR_EXPORT BaseFileFind : public Core::IFindFilter
{
    Q_OBJECT

public:
    BaseFileFind();
    ~BaseFileFind() override;

    bool isEnabled() const override;
    bool isReplaceSupported() const override { return true; }
    void findAll(const QString &txt, Utils::FindFlags findFlags) override;
    void replaceAll(const QString &txt, Utils::FindFlags findFlags) override;
    void addSearchEngine(SearchEngine *searchEngine);

    /* returns the list of unique files that were passed in items */
    static Utils::FilePaths replaceAll(const QString &txt, const Utils::SearchResultItems &items,
                                       bool preserveCase = false);

protected:
    void setSearchDir(const Utils::FilePath &dir);
    Utils::FilePath searchDir() const;
    virtual QString label() const = 0; // see Core::SearchResultWindow::startNewSearch
    virtual QString toolTip() const = 0; // see Core::SearchResultWindow::startNewSearch,
                                         // add %1 placeholder where the find flags should be put

    void writeCommonSettings(Utils::QtcSettings *settings);
    void readCommonSettings(Utils::QtcSettings *settings, const QString &defaultFilter, const QString &defaultExclusionFilter);
    QList<QPair<QWidget *, QWidget *>> createPatternWidgets();
    QStringList fileNameFilters() const;
    QStringList fileExclusionFilters() const;

    SearchEngine *currentSearchEngine() const;
    QVector<SearchEngine *> searchEngines() const;
    void setCurrentSearchEngine(int index);
    virtual void syncSearchEngineCombo(int /*selectedSearchEngineIndex*/) {}

signals:
    void currentSearchEngineChanged();
    void searchDirChanged(const Utils::FilePath &dir);

private:
    virtual FileContainerProvider fileContainerProvider() const = 0;
    void openEditor(Core::SearchResult *result, const Utils::SearchResultItem &item);
    void doReplace(const QString &txt, const Utils::SearchResultItems &items, bool preserveCase);
    void hideHighlightAll(bool visible);
    void searchAgain(Core::SearchResult *search);
    virtual void setupSearch(Core::SearchResult *search);

    void runNewSearch(const QString &txt, Utils::FindFlags findFlags,
                      Core::SearchResultWindow::SearchMode searchMode);
    void runSearch(Core::SearchResult *search);

    Internal::BaseFileFindPrivate *d;
};

} // namespace TextEditor

Q_DECLARE_METATYPE(TextEditor::FileFindParameters)
