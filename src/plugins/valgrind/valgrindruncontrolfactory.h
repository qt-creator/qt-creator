/****************************************************************************
**
** Copyright (C) 2015 Kläralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
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

#ifndef VALGRINDRUNCONTROLFACTORY_H
#define VALGRINDRUNCONTROLFACTORY_H

#include <projectexplorer/runconfiguration.h>

namespace Valgrind {
namespace Internal {

class ValgrindRunControlFactory : public ProjectExplorer::IRunControlFactory
{
    Q_OBJECT
public:
    typedef ProjectExplorer::RunConfiguration RunConfiguration;

    explicit ValgrindRunControlFactory(QObject *parent = 0);

    // IRunControlFactory implementation
    bool canRun(RunConfiguration *runConfiguration, Core::Id mode) const;

    ProjectExplorer::RunControl *create(RunConfiguration *runConfiguration,
                                        Core::Id mode,
                                        QString *errorMessage);
    ProjectExplorer::IRunConfigurationAspect *createRunConfigurationAspect(ProjectExplorer::RunConfiguration *rc);
};

} // namespace Internal
} // namespace Valgrind

#endif // VALGRINDRUNCONTROLFACTORY_H
