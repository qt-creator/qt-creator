/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#pragma once

#include "texteditor_global.h"
#include "utils/filesearch.h"

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
    SearchEngine();
    ~SearchEngine();
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
    ~BaseFileFind();

    bool isEnabled() const;
    bool isReplaceSupported() const { return true; }
    void findAll(const QString &txt, Core::FindFlags findFlags);
    void replaceAll(const QString &txt, Core::FindFlags findFlags);
    void addSearchEngine(SearchEngine *searchEngine);

    /* returns the list of unique files that were passed in items */
    static QStringList replaceAll(const QString &txt,
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
    void openEditor(const Core::SearchResultItem &item);
    void doReplace(const QString &txt,
                   const QList<Core::SearchResultItem> &items,
                   bool preserveCase);
    void hideHighlightAll(bool visible);
    void searchAgain();
    void recheckEnabled();

    void runNewSearch(const QString &txt, Core::FindFlags findFlags,
                      Core::SearchResultWindow::SearchMode searchMode);
    void runSearch(Core::SearchResult *search);

    Internal::BaseFileFindPrivate *d;
};

} // namespace TextEditor

Q_DECLARE_METATYPE(TextEditor::FileFindParameters)
