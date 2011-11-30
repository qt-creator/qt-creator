/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010-2011 Openismus GmbH.
**   Authors: Peter Penz (ppenz@openismus.com)
**            Patricia Santana Cruz (patriciasantanacruz@gmail.com)
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef AUTOTOOLSPROJECTFILE_H
#define AUTOTOOLSPROJECTFILE_H

#include <coreplugin/ifile.h>

namespace AutotoolsProjectManager {
namespace Internal {

class AutotoolsProject;

/**
 * @brief Implementation of the Core::IFile interface.
 *
 * Is used in AutotoolsProject and describes the root
 * of a project. In the context of autotools the implementation
 * is mostly empty, as the modification of a project is
 * done by several Makefile.am files and the configure.ac file.
 *
 * @see AutotoolsProject
 */
class AutotoolsProjectFile : public Core::IFile
{
    Q_OBJECT

public:
    AutotoolsProjectFile(AutotoolsProject *project, const QString &fileName);

    bool save(QString *errorString, const QString &fileName, bool autoSave);
    QString fileName() const;
    QString defaultPath() const;
    QString suggestedFileName() const;
    QString mimeType() const;
    bool isModified() const;
    bool isReadOnly() const;
    bool isSaveAsAllowed() const;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type);
    void rename(const QString &newName);

private:
    AutotoolsProject *m_project;
    QString m_fileName;
};

} // namespace Internal
} // namespace AutotoolsProjectManager

#endif // AUTOTOOLSPROJECTFILE_H
