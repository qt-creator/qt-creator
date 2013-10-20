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

// ActiveX Reference
const char ACTIVEX_REFERENCE [] = "ActiveXReference";
const char ACTIVEX_REFERENCE_CONTROL_GUID [] = "ControlGUID";
const char ACTIVEX_REFERENCE_CONTROL_VERSION [] = "ControlVersion";
const char ACTIVEX_REFERENCE_WRAPPER_TOOL [] = "WrapperTool";
const char ACTIVEX_REFERENCE_LOCAL_ID [] ="LocaleID";
const char ACTIVEX_REFERENCE_COPY_LOCAL [] = "CopyLocal";
const char ACTIVEX_REFERENCE_USE_IN_BUILD [] = "UseInBuild";
const char ACTIVEX_REFERENCE_COPY_LOCAL_DEPENDENCIES [] = "CopyLocalDependencies";
const char ACTIVEX_REFERENCE_COPY_LOCAL_SATELITE_ASSEMBLIES [] = "CopyLocalSatelliteAssemblies";
const char ACTIVEX_REFERENCE_USE_DEPENDENCIES_IN_BUILD [] = "UseDependenciesInBuild";

// Assembly Reference
const char ASSEMBLY_REFERENCE [] = "AssemblyReference";
const char ASSEMBLY_REFERENCE_RELATIVE_PATH [] = "RelativePath";
const char ASSEMBLY_REFERENCE_ASSEMBLY_NAME [] = "AssemblyName";
const char ASSEMBLY_REFERENCE_COPY_LOCAL [] = "CopyLocal";
const char ASSEMBLY_REFERENCE_USE_IN_BUILD [] = "UseInBuild";
const char ASSEMBLY_REFERENCE_COPY_LOCAL_DEPENDENCIES [] = "CopyLocalDependencies";
const char ASSEMBLY_REFERENCE_COPY_LOCAL_SATELITE_ASSEMBLIES [] = "CopyLocalSatelliteAssemblies";
const char ASSEMBLY_REFERENCE_USE_DEPENDENCIES_IN_BUILD [] = "UseDependenciesInBuild";
const char ASSEMBLY_REFERENCE_SUB_TYPE [] = "SubType";
const char ASSEMBLY_REFERENCE_MIN_FRAMEWORK_VERSION [] = "MinFrameworkVersion";

//Project Reference
const char PROJECT_REFERENCE [] = "ProjectReference";
const char PROJECT_REFERENCE_NAME [] = "Name";
const char PROJECT_REFERENCE_REFERENCED_PROJECT_IDENTIFIER [] = "ReferencedProjectIdentifier";
const char PROJECT_REFERENCE_COPY_LOCAL [] = "CopyLocal";
const char PROJECT_REFERENCE_USE_IN_BUILD [] = "UseInBuild";
const char PROJECT_REFERENCE_RELATIVE_PATH_FROM_SOLUTION [] = "RelativePathFromSolution";
const char PROJECT_REFERENCE_RELATIVE_PATH_TO_PROJECT [] = "RelativePathToProject";
const char PROJECT_REFERENCE_USE_DEPENDENCIES_IN_BUILD [] = "UseDependenciesInBuild";
const char PROJECT_REFERENCE_COPY_LOCAL_SATELITE_ASSEMBLIES [] = "CopyLocalSatelliteAssemblies";
const char PROJECT_REFERENCE_COPY_LOCAL_DEPENDENCIES [] = "CopyLocalDependencies";

// ToolFile
const char TOOL_FILE [] = "ToolFile";
const char TOOL_FILE_RELATIVE_PATH [] = "RelativePath";
const char DEFAULT_TOOL_FILE [] = "DefaultToolFile";
const char DEFAULT_TOOL_FILE_FILE_NAME [] = "FileName";

// Vs Project Constants
const char VS_PROJECT_PROJECT_TYPE [] = "ProjectType";
const char VS_PROJECT_VERSION [] = "Version";
const char VS_PROJECT_PROJECT_GUID [] = "ProjectGUID";
const char VS_PROJECT_ROOT_NAMESPACE [] = "RootNamespace";
const char VS_PROJECT_KEYWORD [] = "Keyword";
const char VS_PROJECT_NAME [] = "Name";
const char VS_PROJECT_ASSEMBLY_REFERENCE_SEARCH_PATH [] = "AssemblyReferenceSearchPaths";
const char VS_PROJECT_MANIFEST_KEY_FILE [] = "ManifestKeyFile";
const char VS_PROJECT_MANIFEST_CERTIFICATE_THUMBPRINT [] = "ManifestCertificateThumbprint";
const char VS_PROJECT_MANIFEST_TIMESTAMP_URL [] = "ManifestTimestampURL";
const char VS_PROJECT_SIGN_MANIFEST [] = "SignManifests";
const char VS_PROJECT_SIGN_ASSEMBLY [] = "SignAssembly";
const char VS_PROJECT_ASSEMBLY_ORIGINATOR_KEY_FILE [] = "AssemblyOriginatorKeyFile";
const char VS_PROJECT_DELAY_SIGN [] = "DelaySign";
const char VS_PROJECT_GENERATE_MANIFESTS [] = "GenerateManifests";
const char VS_PROJECT_TARGET_ZONE [] = "TargetZone";
const char VS_PROJECT_EXCLUDED_PERMISSIONS [] = "ExcludedPermissions";
const char VS_PROJECT_TARGET_FRAMEWORK_VERSION [] = "TargetFrameworkVersion";

// Configuration Attributes
const char VS_PROJECT_CONFIG_EXCLUDED [] = "ExcludedFromBuild";

// Tools
const char TOOL_CPP_C_COMPILER [] = "VCCLCompilerTool";
const char TOOL_CUSTOM [] = "VCCustomBuildTool";
const char TOOL_MIDL [] = "VCMIDLTool";
const char TOOL_RESOURCE_COMPILER [] = "VCResourceCompilerTool";
const char TOOL_MANAGED_RESOURCE_COMPILER [] = "VCManagedResourceCompilerTool";
const char TOOL_WEB_SERVICE_PROXY_GENERATOR [] = "VCWebServiceProxyGeneratorTool";
const char TOOL_XML_DATA_PROXY_GENERATOR [] = "VCXMLDataGeneratorTool";

} // VcDocConstants
} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_VCPROJECTDOCUMENT_CONSTANTS_H
