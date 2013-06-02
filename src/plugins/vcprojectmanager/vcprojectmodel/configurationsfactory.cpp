/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkvoic
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
#include "configurationsfactory.h"
#include "../widgets/configurationswidgets.h"
#include "vcprojectdocument.h"

namespace VcProjectManager {
namespace Internal {

ConfigurationsFactory::ConfigurationsFactory()
{
}

ConfigurationsFactory::~ConfigurationsFactory()
{
}

Configuration::Ptr ConfigurationsFactory::createConfiguration(VcDocConstants::DocumentVersion version)
{
    Configuration::Ptr config;

    switch (version) {
    case VcDocConstants::DV_MSVC_2003:
        config = Configuration::Ptr(new Configuration2003);
        break;
    case VcDocConstants::DV_MSVC_2005:
        config = Configuration::Ptr(new Configuration2005);
        break;
    case VcDocConstants::DV_MSVC_2008:
        config = Configuration::Ptr(new Configuration2008);
        break;
    }

    if (config)
        config->init();
    return config;
}

ConfigurationsBaseWidget *ConfigurationsFactory::createSettingsWidget(VcProjectDocument *vcProjDoc, Configurations *config)
{
    switch (vcProjDoc->documentVersion()) {
    case VcDocConstants::DV_MSVC_2003:
        return new Configurations2003Widget(config, vcProjDoc);
        break;
    case VcDocConstants::DV_MSVC_2005:
        return new Configurations2005Widget(config, vcProjDoc);
        break;
    case VcDocConstants::DV_MSVC_2008:
        return new Configurations2008Widget(config, vcProjDoc);
        break;
    }

    return 0;
}

} // namespace Internal
} // namespace VcProjectManager
