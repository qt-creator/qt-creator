/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
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
class QCheckBox;
class QStringListModel;
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
    bool isReplaceSupported() const { return true; }
    void findAll(const QString &txt, QTextDocument::FindFlags findFlags);
    void replaceAll(const QString &txt, QTextDocument::FindFlags findFlags);

    /* returns the list of unique files that were passed in items */
    static QStringList replaceAll(const QString &txt,
                                  const QList<Find::SearchResultItem> &items);
protected:
    virtual QStringList files() = 0;
    void writeCommonSettings(QSettings *settings);
    void readCommonSettings(QSettings *settings, const QString &defaultFilter);
    QWidget *createPatternWidget();
    QWidget *createRegExpWidget();
    void syncComboWithSettings(QComboBox *combo, const QString &setting);
    void updateComboEntries(QComboBox *combo, bool onTop);
    QStringList fileNameFilters() const;

private slots:
    void displayResult(int index);
    void searchFinished();
    void openEditor(const Find::SearchResultItem &item);
    void syncRegExpSetting(bool useRegExp);
    void doReplace(const QString &txt,
                    const QList<Find::SearchResultItem> &items);

private:
    QWidget *createProgressWidget();

    Find::SearchResultWindow *m_resultWindow;
    QFutureWatcher<Utils::FileSearchResult> m_watcher;
    bool m_isSearching;
    QLabel *m_resultLabel;
    QStringListModel m_filterStrings;
    QString m_filterSetting;
    QPointer<QComboBox> m_filterCombo;
    bool m_useRegExp;
    QCheckBox *m_useRegExpCheckBox;
};

} // namespace TextEditor

#endif // BASEFILEFIND_H
