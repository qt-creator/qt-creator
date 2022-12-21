// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"
#include <utils/filesearch.h>

#include <coreplugin/find/ifindfilter.h>
#include <coreplugin/find/searchresultwindow.h>

#include <QFuture>

namespace Utils { class FileIterator; }
namespace Core {
class IEditor;
class SearchResult;
class SearchResultItem;
} // namespace Core

namespace TextEditor {

namespace Internal {
class BaseFileFindPrivate;
class SearchEnginePrivate;
} // Internal

class TEXTEDITOR_EXPORT FileFindParameters
{
public:
    QString text;
    QStringList nameFilters;
    QStringList exclusionFilters;
    QVariant additionalParameters;
    QVariant searchEngineParameters;
    int searchEngineIndex;
    Core::FindFlags flags;
};

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
    virtual QVariant parameters() const = 0;
    virtual void readSettings(QSettings *settings) = 0;
    virtual void writeSettings(QSettings *settings) const = 0;
    virtual QFuture<Utils::FileSearchResultList> executeSearch(
            const FileFindParameters &parameters, BaseFileFind *baseFileFind) = 0;
    virtual Core::IEditor *openEditor(const Core::SearchResultItem &item,
                                      const FileFindParameters &parameters) = 0;
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
    void findAll(const QString &txt, Core::FindFlags findFlags) override;
    void replaceAll(const QString &txt, Core::FindFlags findFlags) override;
    void addSearchEngine(SearchEngine *searchEngine);

    /* returns the list of unique files that were passed in items */
    static Utils::FilePaths replaceAll(const QString &txt,
                                       const QList<Core::SearchResultItem> &items,
                                       bool preserveCase = false);
    virtual Utils::FileIterator *files(const QStringList &nameFilters,
                                       const QStringList &exclusionFilters,
                                       const QVariant &additionalParameters) const = 0;

protected:
    virtual QVariant additionalParameters() const = 0;
    static QVariant getAdditionalParameters(Core::SearchResult *search);
    virtual QString label() const = 0; // see Core::SearchResultWindow::startNewSearch
    virtual QString toolTip() const = 0; // see Core::SearchResultWindow::startNewSearch,
                                         // add %1 placeholder where the find flags should be put
    QFuture<Utils::FileSearchResultList> executeSearch(const FileFindParameters &parameters);

    void writeCommonSettings(QSettings *settings);
    void readCommonSettings(QSettings *settings, const QString &defaultFilter, const QString &defaultExclusionFilter);
    QList<QPair<QWidget *, QWidget *>> createPatternWidgets();
    QStringList fileNameFilters() const;
    QStringList fileExclusionFilters() const;

    SearchEngine *currentSearchEngine() const;
    QVector<SearchEngine *> searchEngines() const;
    void setCurrentSearchEngine(int index);
    virtual void syncSearchEngineCombo(int /*selectedSearchEngineIndex*/) {}

signals:
    void currentSearchEngineChanged();

private:
    void openEditor(Core::SearchResult *result, const Core::SearchResultItem &item);
    void doReplace(const QString &txt,
                   const QList<Core::SearchResultItem> &items,
                   bool preserveCase);
    void hideHighlightAll(bool visible);
    void searchAgain(Core::SearchResult *search);
    virtual void recheckEnabled(Core::SearchResult *search);

    void runNewSearch(const QString &txt, Core::FindFlags findFlags,
                      Core::SearchResultWindow::SearchMode searchMode);
    void runSearch(Core::SearchResult *search);

    Internal::BaseFileFindPrivate *d;
};

} // namespace TextEditor

Q_DECLARE_METATYPE(TextEditor::FileFindParameters)
