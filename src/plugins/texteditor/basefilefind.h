/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef BASEFILEFIND_H
#define BASEFILEFIND_H

#include "texteditor_global.h"

#include <find/ifindfilter.h>
#include <find/searchresultwindow.h>

QT_BEGIN_NAMESPACE
class QLabel;
class QComboBox;
QT_END_NAMESPACE

namespace Utils {
class FileIterator;
}
namespace Find {
class SearchResult;
struct SearchResultItem;
class IFindSupport;
}

namespace TextEditor {
namespace Internal {
class BaseFileFindPrivate;
} // namespace Internal

class TEXTEDITOR_EXPORT BaseFileFind : public Find::IFindFilter
{
    Q_OBJECT

public:
    BaseFileFind();
    ~BaseFileFind();

    bool isEnabled() const;
    bool isReplaceSupported() const { return true; }
    void findAll(const QString &txt, Find::FindFlags findFlags);
    void replaceAll(const QString &txt, Find::FindFlags findFlags);

    /* returns the list of unique files that were passed in items */
    static QStringList replaceAll(const QString &txt,
                                  const QList<Find::SearchResultItem> &items,
                                  bool preserveCase = false);

protected:
    virtual Utils::FileIterator *files(const QStringList &nameFilters,
                                       const QVariant &additionalParameters) const = 0;
    virtual QVariant additionalParameters() const = 0;
    QVariant getAdditionalParameters(Find::SearchResult *search);
    virtual QString label() const = 0; // see Find::SearchResultWindow::startNewSearch
    virtual QString toolTip() const = 0; // see Find::SearchResultWindow::startNewSearch,
                                         // add %1 placeholder where the find flags should be put

    void writeCommonSettings(QSettings *settings);
    void readCommonSettings(QSettings *settings, const QString &defaultFilter);
    QWidget *createPatternWidget();
    void syncComboWithSettings(QComboBox *combo, const QString &setting);
    void updateComboEntries(QComboBox *combo, bool onTop);
    QStringList fileNameFilters() const;

private slots:
    void displayResult(int index);
    void searchFinished();
    void cancel();
    void setPaused(bool paused);
    void openEditor(const Find::SearchResultItem &item);
    void doReplace(const QString &txt,
                   const QList<Find::SearchResultItem> &items,
                   bool preserveCase);
    void hideHighlightAll(bool visible);
    void searchAgain();
    void recheckEnabled();

private:
    void runNewSearch(const QString &txt, Find::FindFlags findFlags,
                      Find::SearchResultWindow::SearchMode searchMode);
    void runSearch(Find::SearchResult *search);

    Internal::BaseFileFindPrivate *d;
};

} // namespace TextEditor

#endif // BASEFILEFIND_H
