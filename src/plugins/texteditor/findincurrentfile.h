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

#ifndef FINDINCURRENTFILE_H
#define FINDINCURRENTFILE_H

#include "basefilefind.h"

#include <coreplugin/ifile.h>
#include <coreplugin/editormanager/ieditor.h>
#include <find/ifindfilter.h>
#include <find/searchresultwindow.h>

#include <QtCore/QPointer>
#include <QtGui/QLabel>
#include <QtGui/QComboBox>
#include <QtGui/QStringListModel>


namespace TextEditor {
namespace Internal {

class FindInCurrentFile : public BaseFileFind
{
    Q_OBJECT

public:
    explicit FindInCurrentFile(Find::SearchResultWindow *resultWindow);

    QString id() const;
    QString name() const;
    QKeySequence defaultShortcut() const;
    bool isEnabled() const;
    QWidget *createConfigWidget();
    void writeSettings(QSettings *settings);
    void readSettings(QSettings *settings);

protected:
    QStringList files();

private slots:
    void handleFileChange(Core::IEditor *editor);

private:
    QPointer<QWidget> m_configWidget;
    Core::IFile *m_currentFile;
};

} // namespace Internal
} // namespace TextEditor

#endif // FINDINCURRENTFILE_H
