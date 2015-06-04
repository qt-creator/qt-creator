/**************************************************************************
**
** Copyright (C) 2015 Openismus GmbH.
** Authors: Peter Penz (ppenz@openismus.com)
**          Patricia Santana Cruz (patriciasantanacruz@gmail.com)
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef AUTOTOOLSPROJECTFILE_H
#define AUTOTOOLSPROJECTFILE_H

#include <coreplugin/idocument.h>

namespace AutotoolsProjectManager {
namespace Internal {

class AutotoolsProject;

/**
 * @brief Implementation of the Core::IDocument interface.
 *
 * Is used in AutotoolsProject and describes the root
 * of a project. In the context of autotools the implementation
 * is mostly empty, as the modification of a project is
 * done by several Makefile.am files and the configure.ac file.
 *
 * @see AutotoolsProject
 */
class AutotoolsProjectFile : public Core::IDocument
{
    Q_OBJECT

public:
    AutotoolsProjectFile(AutotoolsProject *project, const QString &fileName);

    bool save(QString *errorString, const QString &fileName, bool autoSave) override;
    QString defaultPath() const override;
    QString suggestedFileName() const override;
    bool isModified() const override;
    bool isSaveAsAllowed() const override;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) override;

private:
    AutotoolsProject *m_project;
};

} // namespace Internal
} // namespace AutotoolsProjectManager

#endif // AUTOTOOLSPROJECTFILE_H
