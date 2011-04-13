/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef BASEFILEFIND_H
#define BASEFILEFIND_H

#include "texteditor_global.h"

#include <find/ifindfilter.h>
#include <utils/filesearch.h>

#include <QtCore/QFutureWatcher>
#include <QtCore/QPointer>

#include <QtGui/QStringListModel>

QT_BEGIN_NAMESPACE
class QLabel;
class QComboBox;
QT_END_NAMESPACE

namespace Find {
class SearchResultWindow;
struct SearchResultItem;
}

namespace TextEditor {

class TEXTEDITOR_EXPORT BaseFileFind : public Find::IFindFilter
{
    Q_OBJECT

public:
    explicit BaseFileFind(Find::SearchResultWindow *resultWindow);

    bool isEnabled() const;
    bool canCancel() const;
    void cancel();
    bool isReplaceSupported() const { return true; }
    void findAll(const QString &txt, Find::FindFlags findFlags);
    void replaceAll(const QString &txt, Find::FindFlags findFlags);

    /* returns the list of unique files that were passed in items */
    static QStringList replaceAll(const QString &txt,
                                  const QList<Find::SearchResultItem> &items);

protected:
    virtual Utils::FileIterator *files() const = 0;
    void writeCommonSettings(QSettings *settings);
    void readCommonSettings(QSettings *settings, const QString &defaultFilter);
    QWidget *createPatternWidget();
    void syncComboWithSettings(QComboBox *combo, const QString &setting);
    void updateComboEntries(QComboBox *combo, bool onTop);
    QStringList fileNameFilters() const;

private slots:
    void displayResult(int index);
    void searchFinished();
    void openEditor(const Find::SearchResultItem &item);
    void doReplace(const QString &txt,
                    const QList<Find::SearchResultItem> &items);

private:
    QWidget *createProgressWidget();

    Find::SearchResultWindow *m_resultWindow;
    QFutureWatcher<Utils::FileSearchResultList> m_watcher;
    bool m_isSearching;
    QLabel *m_resultLabel;
    QStringListModel m_filterStrings;
    QString m_filterSetting;
    QPointer<QComboBox> m_filterCombo;
};

} // namespace TextEditor

#endif // BASEFILEFIND_H
