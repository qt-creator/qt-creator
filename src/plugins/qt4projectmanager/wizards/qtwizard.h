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

#ifndef QTWIZARD_H
#define QTWIZARD_H

#include "qtprojectparameters.h"

#include <coreplugin/basefilewizard.h>

namespace ProjectExplorer {
class ProjectExplorerPlugin;
}

namespace Qt4ProjectManager {
namespace Internal {

/* Base class for wizard creating Qt projects using QtProjectParameters.
 * To implement a project wizard, overwrite:
 * - createWizardDialog() to create up the dialog
 * - generateFiles() to set their contents
 * The base implementation provides the wizard parameters and opens
 * and opens the finished project in postGenerateFiles().
 * The pro-file must be the last one of the generated files. */

class QtWizard : public Core::BaseFileWizard
{
    Q_OBJECT
    Q_DISABLE_COPY(QtWizard)

protected:
    QtWizard(const QString &name, const QString &description, const QIcon &icon);

    QString templateDir() const;

    QString sourceSuffix()  const;
    QString headerSuffix()  const;
    QString formSuffix()    const;
    QString profileSuffix() const;

private:
    bool postGenerateFiles(const Core::GeneratedFiles &l, QString *errorMessage);

    ProjectExplorer::ProjectExplorerPlugin *m_projectExplorer;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // QTWIZARD_H
