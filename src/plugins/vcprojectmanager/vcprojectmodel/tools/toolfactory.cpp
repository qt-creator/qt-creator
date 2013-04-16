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
#include "toolfactory.h"

#include "browseinformationtool.h"
#include "candcpptool.h"
#include "custombuildsteptool.h"
#include "generaltool.h"
#include "linkertool.h"
#include "manifesttool.h"
#include "postbuildeventtool.h"
#include "prebuildeventtool.h"
#include "prelinkeventtool.h"
#include "xmldocgeneratortool.h"

namespace VcProjectManager {
namespace Internal {

Tool::Ptr ToolFactory::createTool(const QString &toolName)
{
    if (toolName == QLatin1String("VCPreBuildEventTool"))
        return Tool::Ptr(new PreBuildEventTool);
    else if (toolName == QLatin1String("VCCustomBuildTool"))
        return Tool::Ptr(new CustomBuildStepTool);
    else if (toolName == QLatin1String("VCXMLDataGeneratorTool"))
        return Tool::Ptr();
    else if (toolName == QLatin1String("VCWebServiceProxyGeneratorTool"))
        return Tool::Ptr();
    else if (toolName == QLatin1String("VCMIDLTool"))
        return Tool::Ptr();
    else if (toolName == QLatin1String("VCCLCompilerTool"))
        return Tool::Ptr(new CAndCppTool);
    else if (toolName == QLatin1String("VCManagedResourceCompilerTool"))
        return Tool::Ptr();
    else if (toolName == QLatin1String("VCResourceCompilerTool"))
        return Tool::Ptr();
    else if (toolName == QLatin1String("VCPreLinkEventTool"))
        return Tool::Ptr(new PreLinkEventTool);
    else if (toolName == QLatin1String("VCLinkerTool"))
        return Tool::Ptr(new LinkerTool);
    else if (toolName == QLatin1String("VCALinkTool"))
        return Tool::Ptr();
    else if (toolName == QLatin1String("VCManifestTool"))
        return Tool::Ptr();
    else if (toolName == QLatin1String("VCXDCMakeTool"))
        return Tool::Ptr(new XMLDocGeneratorTool);
    else if (toolName == QLatin1String("VCBscMakeTool"))
        return Tool::Ptr(new BrowseInformationTool);
    else if (toolName == QLatin1String("VCFxCopTool"))
        return Tool::Ptr();
    else if (toolName == QLatin1String("VCAppVerifierTool"))
        return Tool::Ptr();
    else if (toolName == QLatin1String("VCPostBuildEventTool"))
        return Tool::Ptr(new PostBuildEventTool);

    return Tool::Ptr(new GeneralTool);
}

ToolFactory::ToolFactory()
{
}

ToolFactory::~ToolFactory()
{
}

} // namespace Internal
} // namespace VcProjectManager
