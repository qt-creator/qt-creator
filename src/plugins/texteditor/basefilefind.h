/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef BASEFILEFIND_H
#define BASEFILEFIND_H

#include "texteditor_global.h"

#include <coreplugin/icore.h>
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
    BaseFileFind(Core::ICore *core, Find::SearchResultWindow *resultWindow);

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

    Core::ICore *m_core;
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
