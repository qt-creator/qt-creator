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
#ifndef VCPROJECTMANAGER_INTERNAL_VCPROJECTDOCUMENT_CONSTANTS_H
#define VCPROJECTMANAGER_INTERNAL_VCPROJECTDOCUMENT_CONSTANTS_H

namespace VcProjectManager {
namespace Internal {
namespace VcDocConstants {

enum DocumentVersion
{
    DV_UNRECOGNIZED,
    DV_MSVC_2003,
    DV_MSVC_2005,
    DV_MSVC_2008
};

const char ACTIVEX_REFERENCE [] = "VC_Doc_Model.ActiveXReference";
const char ACTIVEX_REFERENCE_CONTROL_GUID [] = "ControlGUID";
const char ACTIVEX_REFERENCE_CONTROL_VERSION [] = "ControlVersion";
const char ACTIVEX_REFERENCE_WRAPPER_TOOL [] = "WrapperTool";
const char ACTIVEX_REFERENCE_LOCAL_ID [] ="LocaleID";
const char ACTIVEX_REFERENCE_COPY_LOCAL [] = "CopyLocal";
const char ACTIVEX_REFERENCE_USE_IN_BUILD [] = "UseInBuild";
const char ACTIVEX_REFERENCE_COPY_LOCAL_DEPENDENCIES [] = "CopyLocalDependencies";
const char ACTIVEX_REFERENCE_COPY_LOCAL_SATELITE_ASSEMBLIES [] = "CopyLocalSatelliteAssemblies";
const char ACTIVEX_REFERENCE_USE_DEPENDENCIES_IN_BUILD [] = "UseDependenciesInBuild";

} // VcDocConstants
} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_VCPROJECTDOCUMENT_CONSTANTS_H
