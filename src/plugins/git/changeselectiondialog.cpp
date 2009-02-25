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

#include "changeselectiondialog.h"

#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>

using namespace Git::Internal;

ChangeSelectionDialog::ChangeSelectionDialog(QWidget *parent)
    : QDialog(parent)
{
    m_ui.setupUi(this);
    connect(m_ui.repositoryButton, SIGNAL(clicked()), this, SLOT(selectWorkingDirectory()));
    setWindowTitle(tr("Select a Git commit"));
}

void ChangeSelectionDialog::selectWorkingDirectory()
{
    static QString location = QString();
    location = QFileDialog::getExistingDirectory(this,
                                      QLatin1String("Select Git repository"),
                                      location);
    if (location.isEmpty())
        return;

    // Verify that the location is a repository
    // We are polite, we also allow to specify a directory, which is not
    // the head directory of the repository.
    QDir repository(location);
    do {
        if (repository.entryList(QDir::AllDirs|QDir::Hidden).contains(QLatin1String(".git"))) {
            m_ui.repositoryEdit->setText(repository.absolutePath());
            return;
        }
    } while (repository.cdUp());

    // Did not find a repo
    QMessageBox::critical(this, QLatin1String("Error"),
                          QLatin1String("Selected directory is not a Git repository"));

}
