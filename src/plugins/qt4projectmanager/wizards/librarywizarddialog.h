/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef LIBRARYWIZARDDIALOG_H
#define LIBRARYWIZARDDIALOG_H

#include "qtwizard.h"
#include "qtprojectparameters.h"

namespace Qt4ProjectManager {
namespace Internal {

struct QtProjectParameters;
class FilesPage;
class MobileLibraryWizardOptionPage;
struct LibraryParameters;
struct MobileLibraryParameters;

// Library wizard dialog.
class LibraryWizardDialog : public BaseQt4ProjectWizardDialog
{
    Q_OBJECT

public:
    LibraryWizardDialog(const QString &templateName,
                        const QIcon &icon,
                        bool showModulesPage,
                        QWidget *parent,
                        const Core::WizardDialogParameters &parameters);

    void setSuffixes(const QString &header, const QString &source,  const QString &form= QString());
    void setLowerCaseFiles(bool);

    QtProjectParameters parameters() const;
    LibraryParameters libraryParameters() const;
    MobileLibraryParameters mobileLibraryParameters() const;

    virtual int nextId() const;

protected:
    void initializePage(int id);
    void cleanupPage(int id);

private slots:
    void slotCurrentIdChanged(int);

private:
    QtProjectParameters::Type type() const;
    void setupFilesPage();
    void setupMobilePage();
    bool isModulesPageSkipped() const;
    int skipModulesPageIfNeeded() const;

    FilesPage *m_filesPage;
    MobileLibraryWizardOptionPage *m_mobilePage;
    bool m_pluginBaseClassesInitialized;
    int m_filesPageId;
    int m_modulesPageId;
    int m_targetPageId;
    int m_mobilePageId;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // LIBRARYWIZARDDIALOG_H
