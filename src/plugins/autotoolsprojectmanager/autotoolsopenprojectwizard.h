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

#ifndef AUTOTOOLSOPENPROJECTWIZARD_H
#define AUTOTOOLSOPENPROJECTWIZARD_H

#include <utils/wizard.h>

namespace Utils { class PathChooser; }

namespace AutotoolsProjectManager {
namespace Internal {

class AutotoolsManager;

class AutotoolsOpenProjectWizard : public Utils::Wizard
{
    Q_OBJECT

public:
    enum PageId
    {
        BuildPathPageId
    };

    AutotoolsOpenProjectWizard(AutotoolsManager *manager,
                               const QString &sourceDirectory,
                               QWidget *parent = 0);

    QString buildDirectory() const;
    QString sourceDirectory() const;
    void setBuildDirectory(const QString &directory);
    AutotoolsManager *autotoolsManager() const;

private:
   AutotoolsManager *m_manager;
   QString m_buildDirectory;
   QString m_sourceDirectory;
};

class BuildPathPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit BuildPathPage(AutotoolsOpenProjectWizard *wizard);

private slots:
    void buildDirectoryChanged();

private:
    AutotoolsOpenProjectWizard *m_wizard;
    Utils::PathChooser *m_pc;
};

} // namespace Internal
} // namespace AutotoolsProjectManager
#endif //AUTOTOOLSOPENPROJECTWIZARD_H
