/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef LIBRARYWIZARDDIALOG_H
#define LIBRARYWIZARDDIALOG_H

#include "qtwizard.h"

namespace QmakeProjectManager {
namespace Internal {

struct QtProjectParameters;
class FilesPage;
struct LibraryParameters;

// Library wizard dialog.
class LibraryWizardDialog : public BaseQmakeProjectWizardDialog
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

    static QString pluginInterface(const QString &baseClass);

    virtual int nextId() const;

protected:
    void initializePage(int id);
    void cleanupPage(int id);

private slots:
    void slotCurrentIdChanged(int);

private:
    QtProjectParameters::Type type() const;
    void setupFilesPage();
    bool isModulesPageSkipped() const;
    int skipModulesPageIfNeeded() const;

    FilesPage *m_filesPage;
    bool m_pluginBaseClassesInitialized;
    int m_filesPageId;
    int m_modulesPageId;
    int m_targetPageId;
};

} // namespace Internal
} // namespace QmakeProjectManager

#endif // LIBRARYWIZARDDIALOG_H
