/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkovic
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
#ifndef VCPROJECTMANAGER_INTERNAL_IVISUALSTUDIOPROJECT_H
#define VCPROJECTMANAGER_INTERNAL_IVISUALSTUDIOPROJECT_H

#include <QString>
#include "ivcprojectnodemodel.h"
#include "../vcprojectmodel/vcprojectdocument_constants.h"

namespace VcProjectManager {
namespace Internal {

class IConfigurations;
class IPlatforms;
class IGlobals;
class IReferences;
class IFiles;
class ISettingsWidget;
class IToolFiles;
class IPublishingData;
class IAttributeContainer;
class IConfiguration;

class IVisualStudioProject : public IVcProjectXMLNode
{
public:
    virtual ~IVisualStudioProject() {}

    virtual IAttributeContainer *attributeContainer() const = 0;
    virtual IConfigurations *configurations() const = 0;
    virtual IFiles *files() const = 0;
    virtual IGlobals *globals() const = 0;
    virtual IPlatforms *platforms() const = 0;
    virtual IReferences *referencess() const = 0;
    virtual IToolFiles *toolFiles() const = 0;
    virtual IPublishingData *publishingData() const = 0;
    virtual QString filePath() const = 0;
    virtual bool saveToFile(const QString &filePath) const = 0;
    virtual VcDocConstants::DocumentVersion documentVersion() const = 0;

    virtual IConfiguration *createDefaultBuildConfiguration(const QString &fullConfigName) const = 0;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_IVISUALSTUDIOPROJECT_H
