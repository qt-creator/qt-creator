/****************************************************************************
**
** Copyright (C) 2013 Tim Sander <tim@krieglstein.org>
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

#ifndef BAREMETALDEPLOYCONFIGURATION_H
#define BAREMETALDEPLOYCONFIGURATION_H

#include "baremetaldeployconfiguration.h"
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/deployconfiguration.h>

namespace BareMetal {
namespace Internal {
class BareMetalDeployConfigurationFactory;
} // namespace Internal

class BareMetalDeployConfiguration : public ProjectExplorer::DeployConfiguration
{
    Q_OBJECT
public:
    BareMetalDeployConfiguration(ProjectExplorer::Target *target, const Core::Id id,
                                 const QString &defaultDisplayName);
    BareMetalDeployConfiguration(ProjectExplorer::Target *target,
                                 BareMetalDeployConfiguration *source);

    ProjectExplorer::NamedWidget *createConfigWidget();

    template<class T> T *earlierBuildStep(const ProjectExplorer::BuildStep *laterBuildStep) const
    {
        foreach (const ProjectExplorer::BuildStep *step,  stepList()->steps()) {
            if (step == laterBuildStep)
                return 0;
            if (T * const step = dynamic_cast<T *>(step))
                return step;
        }
        return 0;
    }

private:
    friend class Internal::BareMetalDeployConfigurationFactory;

};

} // namespace BareMetal

#endif // BAREMETALDEPLOYCONFIGURATION_H
