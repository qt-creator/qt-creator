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

#ifndef LIBRARYWIZARDDIALOG_H
#define LIBRARYWIZARDDIALOG_H

#include "qtwizard.h"
#include "qtprojectparameters.h"

namespace Qt4ProjectManager {
namespace Internal {

struct QtProjectParameters;
class FilesPage;
struct LibraryParameters;

// Library wizard dialog.
class LibraryWizardDialog : public BaseQt4ProjectWizardDialog
{
    Q_OBJECT

public:
    LibraryWizardDialog(const QString &templateName,
                        const QIcon &icon,
                        const QList<QWizardPage*> &extensionPages,
                        bool showModulesPage,
                        QWidget *parent = 0);

    void setSuffixes(const QString &header, const QString &source,  const QString &form= QString());
    void setLowerCaseFiles(bool);

    QtProjectParameters parameters() const;
    LibraryParameters libraryParameters() const;

private slots:
    void slotCurrentIdChanged(int);

private:
    QtProjectParameters::Type type() const;

    FilesPage *m_filesPage;
    bool m_pluginBaseClassesInitialized;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // LIBRARYWIZARDDIALOG_H
