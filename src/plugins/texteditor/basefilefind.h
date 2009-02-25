/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef BASEFILEFIND_H
#define BASEFILEFIND_H

#include "texteditor_global.h"

#include <find/ifindfilter.h>
#include <find/searchresultwindow.h>
#include <utils/filesearch.h>

#include <QtCore/QFutureWatcher>
#include <QtCore/QPointer>
#include <QtGui/QLabel>
#include <QtGui/QComboBox>
#include <QtGui/QStringListModel>
#include <QtGui/QCheckBox>

namespace TextEditor {

class TEXTEDITOR_EXPORT BaseFileFind : public Find::IFindFilter
{
    Q_OBJECT

public:
    explicit BaseFileFind(Find::SearchResultWindow *resultWindow);

    bool isEnabled() const;
    void findAll(const QString &txt, QTextDocument::FindFlags findFlags);

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
    void openEditor(const QString &fileName, int line, int column);
    void syncRegExpSetting(bool useRegExp);

private:
    QWidget *createProgressWidget();

    Find::SearchResultWindow *m_resultWindow;
    QFutureWatcher<Core::Utils::FileSearchResult> m_watcher;
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
