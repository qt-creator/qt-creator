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
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/

#ifndef DOCSETTINGSPAGE_H
#define DOCSETTINGSPAGE_H

#include <QtGui/QWidget>
#include <coreplugin/dialogs/ioptionspage.h>

#include "ui_docsettingspage.h"

QT_FORWARD_DECLARE_CLASS(QHelpEngine)

namespace Help {
namespace Internal {

class DocSettingsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    DocSettingsPage(QHelpEngine *helpEngine);

    QString name() const;
    QString category() const;
    QString trCategory() const;

    QWidget *createPage(QWidget *parent);
    void finished(bool accepted);

    bool applyChanges();

signals:
    void documentationAdded();
    void dialogAccepted();

private slots:
    void addDocumentation();
    void removeDocumentation();

private:
    QHelpEngine *m_helpEngine;
    bool m_registeredDocs;
    QStringList m_removeDocs;
    Ui::DocSettingsPage m_ui;
};

} // namespace Help
} // namespace Internal

#endif // DOCSETTINGSPAGE_H
