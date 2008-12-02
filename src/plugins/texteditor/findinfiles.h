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

#ifndef FINDINFILES_H
#define FINDINFILES_H

#include "basefilefind.h"

#include <coreplugin/icore.h>
#include <find/ifindfilter.h>
#include <find/searchresultwindow.h>

#include <QtCore/QPointer>
#include <QtGui/QLabel>
#include <QtGui/QComboBox>
#include <QtGui/QStringListModel>


namespace TextEditor {
namespace Internal {

class FindInFiles : public BaseFileFind
{
    Q_OBJECT

public:
    FindInFiles(Core::ICore *core, Find::SearchResultWindow *resultWindow);

    QString name() const;

    QKeySequence defaultShortcut() const;

    void findAll(const QString &txt, QTextDocument::FindFlags findFlags);
    QWidget *createConfigWidget();
    void writeSettings(QSettings *settings);
    void readSettings(QSettings *settings);

protected:
    QStringList files();

private slots:
    void openFileBrowser();

private:
    QStringListModel m_directoryStrings;
    QString m_directorySetting;
    QPointer<QWidget> m_configWidget;
    QPointer<QComboBox> m_directory;
};

} // namespace Internal
} // namespace TextEditor

#endif // FINDINFILES_H
