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

#ifndef DOCSETTINGSPAGE_H
#define DOCSETTINGSPAGE_H

#include "ui_docsettingspage.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <QtGui/QWidget>

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
    void apply();
    void finish() { }

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
