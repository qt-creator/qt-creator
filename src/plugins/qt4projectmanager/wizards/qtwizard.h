/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef QTWIZARD_H
#define QTWIZARD_H

#include "qtprojectparameters.h"

#include <coreplugin/basefilewizard.h>

QT_BEGIN_NAMESPACE
class QTextStream;
class QDir;
QT_END_NAMESPACE

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
    Q_DISABLE_COPY(QtWizard)
    Q_OBJECT

public:

protected:
    explicit QtWizard(Core::ICore *core, const QString &name,
                      const QString &description, const QIcon &icon);

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
