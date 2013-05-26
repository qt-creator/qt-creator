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
#include "linkertool.h"

#include <QTabWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QVBoxLayout>

#include "../../widgets/basicconfigurationwidget.h"
#include "../../widgets/lineedit.h"

namespace VcProjectManager {
namespace Internal {

LinkerTool::LinkerTool()
{
}

LinkerTool::LinkerTool(const LinkerTool &tool)
    : Tool(tool)
{
}

LinkerTool::~LinkerTool()
{
}

QString LinkerTool::nodeWidgetName() const
{
    return QObject::tr("Linker");
}

VcNodeWidget *LinkerTool::createSettingsWidget()
{
    return new LinkerToolWidget(this);
}

Tool::Ptr LinkerTool::clone() const
{
    return Tool::Ptr(new LinkerTool(*this));
}

QString LinkerTool::outputFile() const
{
    return readStringAttribute(QLatin1String("OutputFile"), outputFileDefault());
}

QString LinkerTool::outputFileDefault() const
{
    return QLatin1String("$(OutDir)\\$(ProjectName).dll");
}

void LinkerTool::setOutputFile(const QString &file)
{
    setStringAttribute(QLatin1String("OutputFile"), file, outputFileDefault());
}

ShowProgressEnum LinkerTool::showProgress() const
{
    return readIntegerEnumAttribute<ShowProgressEnum>(QLatin1String("ShowProgress"),
                                                      SP_Displays_Some_Progress_Messages,
                                                      showProgressDefault());
}

ShowProgressEnum LinkerTool::showProgressDefault() const
{
    return SP_Not_Set;
}

void LinkerTool::setShowProgress(ShowProgressEnum show)
{
    setIntegerEnumAttribute(QLatin1String("ShowProgress"), show, showProgressDefault());
}

QString LinkerTool::version() const
{
    return readStringAttribute(QLatin1String("Version"), versionDefault());
}

QString LinkerTool::versionDefault() const
{
    return QString();
}

void LinkerTool::setVersion(const QString &version)
{
    setStringAttribute(QLatin1String("Version"), version, versionDefault());
}

LinkIncrementalEnum LinkerTool::linkIncremental() const
{
    return readIntegerEnumAttribute<LinkIncrementalEnum>(QLatin1String("LinkIncremental"),
                                                         LI_Yes,
                                                         linkIncrementalDefault());
}

LinkIncrementalEnum LinkerTool::linkIncrementalDefault() const
{
    return LI_Default;
}

void LinkerTool::setLinkIncremental(LinkIncrementalEnum value)
{
    setIntegerEnumAttribute(QLatin1String("LinkIncremental"), value, linkIncrementalDefault());
}

bool LinkerTool::suppressStartupBanner() const
{
    return readBooleanAttribute(QLatin1String("SuppressStartupBanner"), suppressStartupBannerDefault());
}

bool LinkerTool::suppressStartupBannerDefault() const
{
    return true;
}

void LinkerTool::setSuppressStartupBanner(bool suppress)
{
    setBooleanAttribute(QLatin1String("SuppressStartupBanner"), suppress, suppressStartupBannerDefault());
}

bool LinkerTool::ignoreImportLibrary() const
{
    return readBooleanAttribute(QLatin1String("IgnoreImportLibrary"), ignoreImportLibraryDefault());
}

bool LinkerTool::ignoreImportLibraryDefault() const
{
    return false;
}

void LinkerTool::setIgnoreImportLibrary(bool ignore)
{
    setBooleanAttribute(QLatin1String("IgnoreImportLibrary"), ignore, ignoreImportLibraryDefault());
}

bool LinkerTool::registerOutput() const
{
    return readBooleanAttribute(QLatin1String("RegisterOutput"), registerOutputDefault());
}

bool LinkerTool::registerOutputDefault() const
{
    return false;
}

void LinkerTool::setRegisterOutput(bool regis)
{
    setBooleanAttribute(QLatin1String("RegisterOutput"), regis, registerOutputDefault());
}

bool LinkerTool::perUserRedirection() const
{
    return readBooleanAttribute(QLatin1String("PerUserRedirection"), perUserRedirectionDefault());
}

bool LinkerTool::perUserRedirectionDefault() const
{
    return false;
}

void LinkerTool::setPerUserRedirection(bool enable)
{
    setBooleanAttribute(QLatin1String("PerUserRedirection"), enable, perUserRedirectionDefault());
}

QStringList LinkerTool::additionalLibraryDirectories() const
{
    return readStringListAttribute(QLatin1String("AdditionalLibraryDirectories"), QLatin1String(";"));
}

QStringList LinkerTool::additionalLibraryDirectoriesDefault() const
{
    return QStringList();
}

void LinkerTool::setAdditionalLibraryDirectories(const QStringList &dirs)
{
    setStringListAttribute(QLatin1String("AdditionalLibraryDirectories"), dirs, QLatin1String(";"));
}

bool LinkerTool::linkLibraryDependencies() const
{
    return readBooleanAttribute(QLatin1String("LinkLibraryDependencies"), linkLibraryDependenciesDefault());
}

bool LinkerTool::linkLibraryDependenciesDefault() const
{
    return true;
}

void LinkerTool::setLinkLibraryDependencies(bool enable)
{
    setBooleanAttribute(QLatin1String("LinkLibraryDependencies"), enable, linkLibraryDependenciesDefault());
}

bool LinkerTool::useLibraryDependencyInputs() const
{
    return readBooleanAttribute(QLatin1String("UseLibraryDependencyInputs"), useLibraryDependencyInputsDefault());
}

bool LinkerTool::useLibraryDependencyInputsDefault() const
{
    return false;
}

void LinkerTool::setUseLibraryDependencyInputs(bool use)
{
    setBooleanAttribute(QLatin1String("UseLibraryDependencyInputs"), use, useLibraryDependencyInputsDefault());
}

bool LinkerTool::useUnicodeResponseFiles() const
{
    return readBooleanAttribute(QLatin1String("UseUnicodeResponseFiles"), useUnicodeResponseFilesDefault());
}

bool LinkerTool::useUnicodeResponseFilesDefault() const
{
    return true;
}

void LinkerTool::setUseUnicodeResponseFiles(bool use)
{
    setBooleanAttribute(QLatin1String("UseUnicodeResponseFiles"), use, useUnicodeResponseFilesDefault());
}

QStringList LinkerTool::additionalDependencies() const
{
    return readStringListAttribute(QLatin1String("AdditionalDependencies"), QLatin1String(" "));
}

QStringList LinkerTool::additionalDependenciesDefault() const
{
    return QStringList();
}

void LinkerTool::setAdditionalDependencies(const QStringList &dependencies)
{
    setStringListAttribute(QLatin1String("AdditionalDependencies"), dependencies, QLatin1String(" "));
}

bool LinkerTool::ignoreAllDefaultLibraries() const
{
    return readBooleanAttribute(QLatin1String("IgnoreAllDefaultLibraries"), ignoreAllDefaultLibrariesDefault());
}

bool LinkerTool::ignoreAllDefaultLibrariesDefault() const
{
    return false;
}

void LinkerTool::setIgnoreAllDefaultLibraries(bool ignore)
{
    setBooleanAttribute(QLatin1String("IgnoreAllDefaultLibraries"), ignore, ignoreAllDefaultLibrariesDefault());
}

QStringList LinkerTool::ignoreDefaultLibraryNames() const
{
    return readStringListAttribute(QLatin1String("IgnoreDefaultLibraryNames"), QLatin1String(";"));
}

QStringList LinkerTool::ignoreDefaultLibraryNamesDefault() const
{
    return QStringList();
}

void LinkerTool::setIgnoreDefaultLibraryNames(const QStringList &libNames)
{
    setStringListAttribute(QLatin1String("IgnoreDefaultLibraryNames"), libNames, QLatin1String(";"));
}

QString LinkerTool::moduleDefinitionFile() const
{
    return readStringAttribute(QLatin1String("ModuleDefinitionFile"), moduleDefinitionFileDefault());
}

QString LinkerTool::moduleDefinitionFileDefault() const
{
    return QString();
}

void LinkerTool::setModuleDefinitionFile(const QString &file)
{
    setStringAttribute(QLatin1String("ModuleDefinitionFile"), file, moduleDefinitionFileDefault());
}

QStringList LinkerTool::addModuleNamesToAssembly() const
{
    return readStringListAttribute(QLatin1String("AddModuleNamesToAssembly"), QLatin1String(";"));
}

QStringList LinkerTool::addModuleNamesToAssemblyDefault() const
{
    return QStringList();
}

void LinkerTool::setAddModuleNamesToAssembly(const QStringList &moduleNames)
{
    setStringListAttribute(QLatin1String("AddModuleNamesToAssembly"), moduleNames, QLatin1String(";"));
}

QStringList LinkerTool::embedManagedResourceFile() const
{
    return readStringListAttribute(QLatin1String("EmbedManagedResourceFile"), QLatin1String(";"));
}

QStringList LinkerTool::embedManagedResourceFileDefault() const
{
    return QStringList();
}

void LinkerTool::setEmbedManagedResourceFile(const QStringList &resFiles)
{
    setStringListAttribute(QLatin1String("EmbedManagedResourceFile"), resFiles, QLatin1String(";"));
}

QStringList LinkerTool::forceSymbolReferences() const
{
    return readStringListAttribute(QLatin1String("ForceSymbolReferences"), QLatin1String(";"));
}

QStringList LinkerTool::forceSymbolReferencesDefault() const
{
    return QStringList();
}

void LinkerTool::setForceSymbolReferences(const QStringList &refs)
{
    setStringListAttribute(QLatin1String("ForceSymbolReferences"), refs, QLatin1String(";"));
}

QStringList LinkerTool::delayLoadDLLs() const
{
    return readStringListAttribute(QLatin1String("DelayLoadDLLs"), QLatin1String(";"));
}

QStringList LinkerTool::delayLoadDLLsDefault() const
{
    return QStringList();
}

void LinkerTool::setDelayLoadDLLs(const QStringList &dlls)
{
    setStringListAttribute(QLatin1String("DelayLoadDLLs"), dlls, QLatin1String(";"));
}

QStringList LinkerTool::assemblyLinkResource() const
{
    return readStringListAttribute(QLatin1String("AssemblyLinkResource"), QLatin1String(";"));
}

QStringList LinkerTool::assemblyLinkResourceDefault() const
{
    return QStringList();
}

void LinkerTool::setAssemblyLinkResource(const QStringList &res)
{
    setStringListAttribute(QLatin1String("AssemblyLinkResource"), res, QLatin1String(";"));
}

bool LinkerTool::generateManifest() const
{
    return readBooleanAttribute(QLatin1String("GenerateManifest"), generateManifestDefault());
}

bool LinkerTool::generateManifestDefault() const
{
    return true;
}

void LinkerTool::setGenerateManifest(bool generate)
{
    setBooleanAttribute(QLatin1String("GenerateManifest"), generate, generateManifestDefault());
}

QString LinkerTool::manifestFile() const
{
    return readStringAttribute(QLatin1String("ManifestFile"), manifestFileDefault());
}

QString LinkerTool::manifestFileDefault() const
{
    return QLatin1String("$(IntDir)\\$(TargetFileName).intermediate.manifest");
}

void LinkerTool::setManifestFile(const QString &file)
{
    setStringAttribute(QLatin1String("ManifestFile"), file, manifestFileDefault());
}

QStringList LinkerTool::additionalManifestDependencies() const
{
    return readStringListAttribute(QLatin1String("AdditionalManifestDependencies"), QLatin1String(";"));
}

QStringList LinkerTool::additionalManifestDependenciesDefault() const
{
    return QStringList();
}

void LinkerTool::setAdditionalManifestDependencies(const QStringList &dependencies)
{
    setStringListAttribute(QLatin1String("AdditionalManifestDependencies"), dependencies, QLatin1String(";"));
}

bool LinkerTool::allowIsolation() const
{
    return readBooleanAttribute(QLatin1String("AllowIsolation"), allowIsolationDefault());
}

bool LinkerTool::allowIsolationDefault() const
{
    return true;
}

void LinkerTool::setAllowIsolation(bool allow)
{
    setBooleanAttribute(QLatin1String("AllowIsolation"), allow, allowIsolationDefault());
}

bool LinkerTool::enableUAC() const
{
    return readBooleanAttribute(QLatin1String("EnableUAC"), enableUACDefault());
}

bool LinkerTool::enableUACDefault() const
{
    return true;
}

void LinkerTool::setEnableUAC(bool enable)
{
    setBooleanAttribute(QLatin1String("EnableUAC"), enable, enableUACDefault());
}

UACExecutionLevelEnum LinkerTool::uacExecutionLevel() const
{
    return readIntegerEnumAttribute<UACExecutionLevelEnum>(QLatin1String("UACExecutionLevel"),
                                                           UACEL_RequireAdministrator,
                                                           uacExecutionLevelDefault());
}

UACExecutionLevelEnum LinkerTool::uacExecutionLevelDefault() const
{
    return UACEL_AsInvoker;
}

void LinkerTool::setUACExecutionLevel(UACExecutionLevelEnum value)
{
    setIntegerEnumAttribute(QLatin1String("UACExecutionLevel"), value, uacExecutionLevelDefault());
}

bool LinkerTool::uacUIAAccess() const
{
    return readBooleanAttribute(QLatin1String("UACUIAccess"), uacUIAAccessDefault());
}

bool LinkerTool::uacUIAAccessDefault() const
{
    return false;
}

void LinkerTool::setUACUIAccess(bool enable)
{
    setBooleanAttribute(QLatin1String("UACUIAccess"), enable, uacUIAAccessDefault());
}

bool LinkerTool::generateDebugInformation() const
{
    return readBooleanAttribute(QLatin1String("GenerateDebugInformation"), generateDebugInformationDefault());
}

bool LinkerTool::generateDebugInformationDefault() const
{
    return true;
}

void LinkerTool::setGenerateDebugInformation(bool generate)
{
    setBooleanAttribute(QLatin1String("GenerateDebugInformation"), generate, generateDebugInformationDefault());
}

QString LinkerTool::programDatabaseFile() const
{
    return readStringAttribute(QLatin1String("ProgramDatabaseFile"), programDatabaseFileDefault());
}

QString LinkerTool::programDatabaseFileDefault() const
{
    return QLatin1String("$(TargetDir)$(TargetName).pdb");
}

void LinkerTool::setProgramDatabaseFile(const QString &file)
{
    setStringAttribute(QLatin1String("ProgramDatabaseFile"), file, programDatabaseFileDefault());
}

QString LinkerTool::stripPrivateSymbols() const
{
    return readStringAttribute(QLatin1String("StripPrivateSymbols"), QString());
}

QString LinkerTool::stripPrivateSymbolsDefault() const
{
    return QString();
}

void LinkerTool::setStripPrivateSymbols(const QString &symbols)
{
    setStringAttribute(QLatin1String("StripPrivateSymbols"), symbols, stripPrivateSymbolsDefault());
}

bool LinkerTool::generateMapFile() const
{
    return readBooleanAttribute(QLatin1String("GenerateMapFile"), generateMapFileDefault());
}

bool LinkerTool::generateMapFileDefault() const
{
    return false;
}

void LinkerTool::setGenerateMapFile(bool generate)
{
    setBooleanAttribute(QLatin1String("GenerateMapFile"), generate, generateMapFileDefault());
}

QString LinkerTool::mapFileName() const
{
    return readStringAttribute(QLatin1String("MapFileName"), mapFileNameDefault());
}

QString LinkerTool::mapFileNameDefault() const
{
    return QString();
}

void LinkerTool::setMapFileName(const QString &fileName)
{
    setStringAttribute(QLatin1String("MapFileName"), fileName, mapFileNameDefault());
}

bool LinkerTool::mapExports() const
{
    return readBooleanAttribute(QLatin1String("MapExports"), mapExportsDefault());
}

bool LinkerTool::mapExportsDefault() const
{
    return false;
}

void LinkerTool::setMapExports(bool enable)
{
    setBooleanAttribute(QLatin1String("MapExports"), enable, mapExportsDefault());
}

AssemblyDebugEnum LinkerTool::assemblyDebug() const
{
    return readIntegerEnumAttribute<AssemblyDebugEnum>(QLatin1String("AssemblyDebug"),
                                                       AD_No_Runtime_Tracking_And_Enable_Optimizations,
                                                       assemblyDebugDefault());
}

AssemblyDebugEnum LinkerTool::assemblyDebugDefault() const
{
    return AD_No_Debuggable_Attribute_Emitted;
}

void LinkerTool::setAssemblyDebug(AssemblyDebugEnum value)
{
    setIntegerEnumAttribute(QLatin1String("AssemblyDebug"), value, assemblyDebugDefault());
}

SubSystemEnum LinkerTool::subSystem() const
{
    return readIntegerEnumAttribute<SubSystemEnum>(QLatin1String("SubSystem"),
                                                   SS_WindowsCE,
                                                   subSystemDefault());
}

SubSystemEnum LinkerTool::subSystemDefault() const
{
    return SS_Not_Set;
}

void LinkerTool::setSubSystem(SubSystemEnum value)
{
    setIntegerEnumAttribute(QLatin1String("SubSystem"), value, subSystemDefault());
}

int LinkerTool::heapReserveSize() const
{
    bool ok;
    int size = QVariant(attributeValue(QLatin1String("HeapReserveSize"))).toInt(&ok);

    if (ok)
        return size;
    return heapReserveSizeDefault();
}

int LinkerTool::heapReserveSizeDefault() const
{
    return 0;
}

void LinkerTool::setHeapReserveSize(int size)
{
    setIntegerEnumAttribute(QLatin1String("HeapReserveSize"), size, heapReserveSizeDefault());
}

int LinkerTool::heapCommitSize() const
{
    bool ok;
    int size = QVariant(attributeValue(QLatin1String("HeapCommitSize"))).toInt(&ok);

    if (ok)
        return size;
    return heapCommitSizeDefault();
}

int LinkerTool::heapCommitSizeDefault() const
{
    return 0;
}

void LinkerTool::setHeapCommitSize(int size)
{
    setIntegerEnumAttribute(QLatin1String("HeapCommitSize"), size, heapCommitSizeDefault());
}

int LinkerTool::stackReserveSize() const
{
    bool ok;
    int size = QVariant(attributeValue(QLatin1String("StackReserveSize"))).toInt(&ok);

    if (ok)
        return size;
    return stackReserveSizeDefault();
}

int LinkerTool::stackReserveSizeDefault() const
{
    return 0;
}

void LinkerTool::setStackReserveSize(int size)
{
    setIntegerEnumAttribute(QLatin1String("StackReserveSize"), size, stackReserveSizeDefault());
}

int LinkerTool::stackCommitSize() const
{
    bool ok;
    int size = QVariant(attributeValue(QLatin1String("StackCommitSize"))).toInt(&ok);

    if (ok)
        return size;
    return stackCommitSizeDefault();
}

int LinkerTool::stackCommitSizeDefault() const
{
    return 0;
}

void LinkerTool::setStackCommitSize(int size)
{
    setIntegerEnumAttribute(QLatin1String("StackCommitSize"), size, stackCommitSizeDefault());
}

LargeAddressAwareEnum LinkerTool::largeAddressAware() const
{
    return readIntegerEnumAttribute<LargeAddressAwareEnum>(QLatin1String("LargeAddressAware"),
                                                           LAA_Support_Addresses_Larger_Than_Two_Gigabytes,
                                                           largeAddressAwareDefault());
}

LargeAddressAwareEnum LinkerTool::largeAddressAwareDefault() const
{
    return LAA_Default;
}

void LinkerTool::setLargeAddressAware(LargeAddressAwareEnum value)
{
    setIntegerEnumAttribute(QLatin1String("LargeAddressAware"), value, largeAddressAwareDefault());
}

TerminalServerAwareEnum LinkerTool::terminalServerAware() const
{
    return readIntegerEnumAttribute<TerminalServerAwareEnum>(QLatin1String("TerminalServerAware"),
                                                             TSA_Application_Terminal_Server_Aware,
                                                             terminalServerAwareDefault());
}

TerminalServerAwareEnum LinkerTool::terminalServerAwareDefault() const
{
    return TSA_Default;
}

void LinkerTool::setTerminalServerAware(TerminalServerAwareEnum value)
{
    setIntegerEnumAttribute(QLatin1String("TerminalServerAware"), value, terminalServerAwareDefault());
}

bool LinkerTool::swapRunFromCD() const
{
    return readBooleanAttribute(QLatin1String("SwapRunFromCD"), swapRunFromCDDefault());
}

bool LinkerTool::swapRunFromCDDefault() const
{
    return false;
}

void LinkerTool::setSwapRunFromCD(bool enable)
{
    setBooleanAttribute(QLatin1String("SwapRunFromCD"), enable, swapRunFromCDDefault());
}

bool LinkerTool::swapRunFromNet() const
{
    return readBooleanAttribute(QLatin1String("SwapRunFromNet"), swapRunFromNetDefault());
}

bool LinkerTool::swapRunFromNetDefault() const
{
    return false;
}

void LinkerTool::setSwapRunFromNet(bool swap)
{
    setBooleanAttribute(QLatin1String("SwapRunFromNet"), swap, swapRunFromNetDefault());
}

DriverEnum LinkerTool::driver() const
{
    return readIntegerEnumAttribute<DriverEnum>(QLatin1String("Driver"),
                                                DR_WDM,
                                                driverDefault());
}

DriverEnum LinkerTool::driverDefault() const
{
    return DR_Not_Set;
}

void LinkerTool::setDriver(DriverEnum value)
{
    setIntegerEnumAttribute(QLatin1String("Driver"), value, driverDefault());
}

OptimizeReferencesEnum LinkerTool::optimizeReferences() const
{
    return readIntegerEnumAttribute<OptimizeReferencesEnum>(QLatin1String("OptimizeReferences"),
                                                            OR_Eliminate_Unreferenced_Data,
                                                            optimizeReferencesDefault());
}

OptimizeReferencesEnum LinkerTool::optimizeReferencesDefault() const
{
    return OR_Default;
}

void LinkerTool::setOptimizeReferences(OptimizeReferencesEnum value)
{
    setIntegerEnumAttribute(QLatin1String("OptimizeReferences"), value, optimizeReferencesDefault());
}

EnableCOMDATFoldingEnum LinkerTool::enableCOMDATFolding() const
{
    return readIntegerEnumAttribute<EnableCOMDATFoldingEnum>(QLatin1String("EnableCOMDATFolding"),
                                                             ECF_Remove_Redundant_COMDATs,
                                                             enableCOMDATFoldingDefault());
}

EnableCOMDATFoldingEnum LinkerTool::enableCOMDATFoldingDefault() const
{
    return ECF_Default;
}

void LinkerTool::setEnableCOMDATFolding(EnableCOMDATFoldingEnum value)
{
    setIntegerEnumAttribute(QLatin1String("EnableCOMDATFolding"), value, enableCOMDATFoldingDefault());
}

OptimizeForWindows98Enum LinkerTool::optimizeForWindows98() const
{
    return readIntegerEnumAttribute<OptimizeForWindows98Enum>(QLatin1String("OptimizeForWindows98"),
                                                              OFW98_Yes,
                                                              optimizeForWindows98Default());
}

OptimizeForWindows98Enum LinkerTool::optimizeForWindows98Default() const
{
    return OFW98_No;
}

void LinkerTool::setOptimizeForWindows98(OptimizeForWindows98Enum value)
{
    setIntegerEnumAttribute(QLatin1String("OptimizeForWindows98"), value, optimizeForWindows98Default());
}

QString LinkerTool::functionOrder() const
{
    return readStringAttribute(QLatin1String("FunctionOrder"), functionOrderDefault());
}

QString LinkerTool::functionOrderDefault() const
{
    return QString();
}

void LinkerTool::setFunctionOrder(const QString &order)
{
    setStringAttribute(QLatin1String("FunctionOrder"), order, functionOrderDefault());
}

QString LinkerTool::profileGuidedDatabase() const
{
    return readStringAttribute(QLatin1String("ProfileGuidedDatabase"), profileGuidedDatabaseDefault());
}

QString LinkerTool::profileGuidedDatabaseDefault() const
{
    return QLatin1String("$(TargetDir)$(TargetName).pg");
}

void LinkerTool::setProfileGuidedDatabase(const QString &database)
{
    setStringAttribute(QLatin1String("ProfileGuidedDatabase"), database, profileGuidedDatabaseDefault());
}

LinkTimeCodeGenerationEnum LinkerTool::linkTimeCodeGeneration() const
{
    return readIntegerEnumAttribute<LinkTimeCodeGenerationEnum>(QLatin1String("LinkTimeCodeGeneration"),
                                                                LTCG_Profile_Guided_Optimization_Update,
                                                                linkTimeCodeGenerationDefault());
}

LinkTimeCodeGenerationEnum LinkerTool::linkTimeCodeGenerationDefault() const
{
    return LTCG_Default;
}

void LinkerTool::setLinkTimeCodeGeneration(LinkTimeCodeGenerationEnum value)
{
    setIntegerEnumAttribute(QLatin1String("LinkTimeCodeGeneration"), value, linkTimeCodeGenerationDefault());
}

QStringList LinkerTool::midlCommandFile() const
{
    return readStringListAttribute(QLatin1String("MidlCommandFile"), QLatin1String("\r\n"));
}

QStringList LinkerTool::midlCommandFileDefault() const
{
    return QStringList();
}

void LinkerTool::setMidlCommandFile(const QStringList &values)
{
    setStringListAttribute(QLatin1String("MidlCommandFile"), values, QLatin1String("\r\n"));
}

bool LinkerTool::ignoreEmbeddedIDL() const
{
    return readBooleanAttribute(QLatin1String("IgnoreEmbeddedIDL"), ignoreEmbeddedIDLDefault());
}

bool LinkerTool::ignoreEmbeddedIDLDefault() const
{
    return false;
}

void LinkerTool::setIgnoreEmbeddedIDL(bool ignore)
{
    setBooleanAttribute(QLatin1String("IgnoreEmbeddedIDL"), ignore, ignoreEmbeddedIDLDefault());
}

QString LinkerTool::mergedIDLBaseFileName() const
{
    return readStringAttribute(QLatin1String("MergedIDLBaseFileName"), mergedIDLBaseFileNameDefault());
}

QString LinkerTool::mergedIDLBaseFileNameDefault() const
{
    return QString();
}

void LinkerTool::setMergedIDLBaseFileName(const QString &fileName)
{
    setStringAttribute(QLatin1String("MergedIDLBaseFileName"), fileName, mergedIDLBaseFileNameDefault());
}

QString LinkerTool::typeLibraryFile() const
{
    return readStringAttribute(QLatin1String("TypeLibraryFile"), typeLibraryFileDefault());
}

QString LinkerTool::typeLibraryFileDefault() const
{
    return QString();
}

void LinkerTool::setTypeLibraryFile(const QString &libFile)
{
    setStringAttribute(QLatin1String("TypeLibraryFile"), libFile, typeLibraryFileDefault());
}

int LinkerTool::typeLibraryResourceID() const
{
    bool ok;
    int size = QVariant(attributeValue(QLatin1String("TypeLibraryResourceID"))).toInt(&ok);

    if (ok)
        return size;
    return typeLibraryResourceIDDefault();
}

int LinkerTool::typeLibraryResourceIDDefault() const
{
    return 0;
}

void LinkerTool::setTypeLibraryResourceID(int id)
{
    setAttribute(QLatin1String("TypeLibraryResourceID"), QVariant(id).toString());
}

QString LinkerTool::entryPointSymbol() const
{
    return readStringAttribute(QLatin1String("EntryPointSymbol"), entryPointSymbolDefault());
}

QString LinkerTool::entryPointSymbolDefault() const
{
    return QString();
}

void LinkerTool::setEntryPointSymbol(const QString &symbol)
{
    setStringAttribute(QLatin1String("EntryPointSymbol"), symbol, entryPointSymbolDefault());
}

bool LinkerTool::resourceOnlyDLL() const
{
    return readBooleanAttribute(QLatin1String("ResourceOnlyDLL"), resourceOnlyDLLDefault());
}

bool LinkerTool::resourceOnlyDLLDefault() const
{
    return false;
}

void LinkerTool::setResourceOnlyDLL(bool enable)
{
    setBooleanAttribute(QLatin1String("ResourceOnlyDLL"), enable, resourceOnlyDLLDefault());
}

bool LinkerTool::checksum() const
{
    return readBooleanAttribute(QLatin1String("SetChecksum"), checksumDefault());
}

bool LinkerTool::checksumDefault() const
{
    return false;
}

void LinkerTool::setChecksum(bool enable)
{
    setBooleanAttribute(QLatin1String("SetChecksum"), enable, checksumDefault());
}

QString LinkerTool::baseAddress() const
{
    return readStringAttribute(QLatin1String("BaseAddress"), baseAddressDefault());
}

QString LinkerTool::baseAddressDefault() const
{
    return QString();
}

void LinkerTool::setBaseAddress(const QString &address)
{
    setStringAttribute(QLatin1String("BaseAddress"), address, baseAddressDefault());
}

RandomizedBaseAddressEnum LinkerTool::randomizedBaseAddress() const
{
    return readIntegerEnumAttribute<RandomizedBaseAddressEnum>(QLatin1String("RandomizedBaseAddress"),
                                                               RBA_Enable_Image_Randomization,
                                                               randomizedBaseAddressDefault());
}

RandomizedBaseAddressEnum LinkerTool::randomizedBaseAddressDefault() const
{
    return RBA_Enable_Image_Randomization;
}

void LinkerTool::setRandomizedBaseAddress(RandomizedBaseAddressEnum value)
{
    setIntegerEnumAttribute(QLatin1String("RandomizedBaseAddress"), value, randomizedBaseAddressDefault());
}

FixedBaseAddressEnum LinkerTool::fixedBaseAddress() const
{
    return readIntegerEnumAttribute<FixedBaseAddressEnum>(QLatin1String("FixedBaseAddress"),
                                                          FBA_Image_Must_Be_Loaded_At_A_Fixed_Address,
                                                          fixedBaseAddressDefault());
}

FixedBaseAddressEnum LinkerTool::fixedBaseAddressDefault() const
{
    return FBA_Default;
}

void LinkerTool::setFixedBaseAddress(FixedBaseAddressEnum value)
{
    setIntegerEnumAttribute(QLatin1String("FixedBaseAddress"), value, fixedBaseAddressDefault());
}

DataExecutionPreventionEnum LinkerTool::dataExecutionPrevention() const
{
    return readIntegerEnumAttribute<DataExecutionPreventionEnum>(QLatin1String("DataExecutionPrevention"),
                                                                 DEP_Image_Is_Compatible_With_DEP,
                                                                 dataExecutionPreventionDefault());
}

DataExecutionPreventionEnum LinkerTool::dataExecutionPreventionDefault() const
{
    return DEP_Image_Is_Compatible_With_DEP;
}

void LinkerTool::setDataExecutionPrevention(DataExecutionPreventionEnum value)
{
    setIntegerEnumAttribute(QLatin1String("DataExecutionPrevention"), value, dataExecutionPreventionDefault());
}

bool LinkerTool::turnOffAssemblyGeneration() const
{
    return readBooleanAttribute(QLatin1String("TurnOffAssemblyGeneration"), turnOffAssemblyGenerationDefault());
}

bool LinkerTool::turnOffAssemblyGenerationDefault() const
{
    return false;
}

void LinkerTool::setTurnOffAssemblyGeneration(bool turnOff)
{
    setBooleanAttribute(QLatin1String("TurnOffAssemblyGeneration"), turnOff, turnOffAssemblyGenerationDefault());
}

bool LinkerTool::supportUnloadOfDelayLoadedDLL() const
{
    return readBooleanAttribute(QLatin1String("SupportUnloadOfDelayLoadedDLL"), supportUnloadOfDelayLoadedDLLDefault());
}

bool LinkerTool::supportUnloadOfDelayLoadedDLLDefault() const
{
    return false;
}

void LinkerTool::setSupportUnloadOfDelayLoadedDLL(bool support)
{
    setBooleanAttribute(QLatin1String("SupportUnloadOfDelayLoadedDLL"), support, supportUnloadOfDelayLoadedDLLDefault());
}

QString LinkerTool::importLibrary() const
{
    return readStringAttribute(QLatin1String("ImportLibrary"), importLibraryDefault());
}

QString LinkerTool::importLibraryDefault() const
{
    return QString();
}

void LinkerTool::setImportLibrary(const QString &lib)
{
    setStringAttribute(QLatin1String("ImportLibrary"), lib, importLibraryDefault());
}

QString LinkerTool::mergeSections() const
{
    return readStringAttribute(QLatin1String("MergeSections"), mergeSectionsDefault());
}

QString LinkerTool::mergeSectionsDefault() const
{
    return QString();
}

void LinkerTool::setMergeSections(const QString &sections)
{
    setStringAttribute(QLatin1String("MergeSections"), sections, mergeSectionsDefault());
}

TargetMachineEnum LinkerTool::targetMachine() const
{
    return readIntegerEnumAttribute<TargetMachineEnum>(QLatin1String("TargetMachine"),
                                                       TM_MachineX64,
                                                       targetMachineDefault());
}

TargetMachineEnum LinkerTool::targetMachineDefault() const
{
    return TM_MachineX86;
}

void LinkerTool::setTargetMachine(TargetMachineEnum value)
{
    setIntegerEnumAttribute(QLatin1String("TargetMachine"), value, targetMachineDefault());
}

bool LinkerTool::profile() const
{
    return readBooleanAttribute(QLatin1String("Profile"), profileDefault());
}

bool LinkerTool::profileDefault() const
{
    return false;
}

void LinkerTool::setProfile(bool enable)
{
    setBooleanAttribute(QLatin1String("Profile"), enable, profileDefault());
}

CLRThreadAttributeEnum LinkerTool::clrThreadAttribute() const
{
    return readIntegerEnumAttribute<CLRThreadAttributeEnum>(QLatin1String("CLRThreadAttribute"),
                                                            CLRTA_STA_Threading_Attribute,
                                                            clrThreadAttributeDefault());
}

CLRThreadAttributeEnum LinkerTool::clrThreadAttributeDefault() const
{
    return CLRTA_No_Threading_Attribute_Set;
}

void LinkerTool::setCLRThreadAttribute(CLRThreadAttributeEnum value)
{
    setIntegerEnumAttribute(QLatin1String("CLRThreadAttribute"), value, clrThreadAttributeDefault());
}

CLRImageTypeEnum LinkerTool::clrImageType() const
{
    return readIntegerEnumAttribute<CLRImageTypeEnum>(QLatin1String("CLRImageType"),
                                                      CLRIT_Force_Safe_IL_Image,
                                                      clrImageTypeDefault());
}

CLRImageTypeEnum LinkerTool::clrImageTypeDefault() const
{
    return CLRIT_Default_Image_Type;
}

void LinkerTool::setCLRImageType(CLRImageTypeEnum value)
{
    setIntegerEnumAttribute(QLatin1String("CLRImageType"), value, clrImageTypeDefault());
}

QString LinkerTool::keyFile() const
{
    return readStringAttribute(QLatin1String("KeyFile"), keyFileDefault());
}

QString LinkerTool::keyFileDefault() const
{
    return QString();
}

void LinkerTool::setKeyFile(const QString &file)
{
    setStringAttribute(QLatin1String("KeyFile"), file, keyFileDefault());
}

QString LinkerTool::keyContainer() const
{
    return readStringAttribute(QLatin1String("KeyContainer"), keyContainerDefault());
}

QString LinkerTool::keyContainerDefault() const
{
    return QString();
}

void LinkerTool::setKeyContainer(const QString &keyContainer)
{
    setStringAttribute(QLatin1String("KeyContainer"), keyContainer, keyContainerDefault());
}

bool LinkerTool::delaySign() const
{
    return readBooleanAttribute(QLatin1String("DelaySign"), delaySignDefault());
}

bool LinkerTool::delaySignDefault() const
{
    return false;
}

void LinkerTool::setDelaySign(bool delay)
{
    setBooleanAttribute(QLatin1String("DelaySign"), delay, delaySignDefault());
}

ErrorReportingLinkingEnum LinkerTool::errorReporting() const
{
    return readIntegerEnumAttribute<ErrorReportingLinkingEnum>(QLatin1String("ErrorReporting"),
                                                               ERL_Queue_For_Next_Login,
                                                               errorReportingDefault());
}

ErrorReportingLinkingEnum LinkerTool::errorReportingDefault() const
{
    return ERL_Prompt_Immediately;
}

void LinkerTool::setErrorReporting(ErrorReportingLinkingEnum value)
{
    setIntegerEnumAttribute(QLatin1String("ErrorReporting"), value, errorReportingDefault());
}

bool LinkerTool::clrUnmanagedCodeCheck() const
{
    return readBooleanAttribute(QLatin1String("CLRUnmanagedCodeCheck"), clrUnmanagedCodeCheckDefault());
}

bool LinkerTool::clrUnmanagedCodeCheckDefault() const
{
    return false;
}

void LinkerTool::setCLRUnmanagedCodeCheck(bool unmanaged)
{
    setBooleanAttribute(QLatin1String("CLRUnmanagedCodeCheck"), unmanaged, clrUnmanagedCodeCheckDefault());
}


LinkerToolWidget::LinkerToolWidget(LinkerTool* tool)
    : m_tool(tool)
{
    QTabWidget *mainTabWidget = new QTabWidget(this);
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->addWidget(mainTabWidget);
    setLayout(layout);

    mainTabWidget->addTab(createGeneralWidget(), tr("General"));
    mainTabWidget->addTab(createInputWidget(), tr("Input"));
    mainTabWidget->addTab(createManifestFileWidget(), tr("Manifest File"));
    mainTabWidget->addTab(createDebuggingWidget(), tr("Debugging"));
    mainTabWidget->addTab(createSystemWidget(), tr("System"));
    mainTabWidget->addTab(createOptimizationWidget(), tr("Optimization"));
    mainTabWidget->addTab(createEmbeddedIDLWidget(), tr("Embedded IDL"));
    mainTabWidget->addTab(createAdvancedWidget(), tr("Advanced"));
}

LinkerToolWidget::~LinkerToolWidget()
{
}

void LinkerToolWidget::saveData()
{
    saveGeneralData();
    saveInputData();
    saveManifestFileData();
    saveDebuggingData();
    saveSystemData();
    saveOptimizationData();
    saveEnbeddedIDLData();
    saveAdvancedData();
}

QWidget *LinkerToolWidget::createGeneralWidget()
{
    BasicConfigurationWidget *basicWidget = new BasicConfigurationWidget;

    m_outputFileLineEdit = new QLineEdit;
    m_outputFileLineEdit->setText(m_tool->outputFile());
    basicWidget->insertTableRow(tr("Output File"),
                                m_outputFileLineEdit,
                                tr("Override the default file name."));

    m_showProgComboBox = new QComboBox;
    m_showProgComboBox->addItem(tr("Not Set"), QVariant(0));
    m_showProgComboBox->addItem(tr("Display All Progress Messages (/VERBOSE)"), QVariant(1));
    m_showProgComboBox->addItem(tr("Displays Some Progress Messages (/VERBOSE:LIB)"), QVariant(2));

    m_showProgComboBox->setCurrentIndex(m_tool->showProgress());

    basicWidget->insertTableRow(tr("Show Progress"),
                                m_showProgComboBox,
                                tr("Enables detailed display progress."));

    m_versionLineEdit = new QLineEdit;
    m_versionLineEdit->setText(m_tool->version());
    basicWidget->insertTableRow(tr("Version"),
                                m_versionLineEdit,
                                tr("Use this value as the version number in created image"));

    m_linkIncComboBox = new QComboBox;
    m_linkIncComboBox->addItem(tr("Default"), QVariant(0));
    m_linkIncComboBox->addItem(tr("No (/INCREMENTAL:NO)"), QVariant(1));
    m_linkIncComboBox->addItem(tr("Yes (/INCREMENTAL)"), QVariant(2));

    m_linkIncComboBox->setCurrentIndex(m_tool->linkIncremental());

    basicWidget->insertTableRow(tr("Enable Incremental Linking"),
                                m_linkIncComboBox,
                                tr("Enables incremental linking."));

    m_suppStartBannerComboBox = new QComboBox;
    m_suppStartBannerComboBox->addItem(tr("No"), QVariant(false));
    m_suppStartBannerComboBox->addItem(tr("Yes (/NOLOGO)"), QVariant(true));

    if (m_tool->suppressStartupBanner())
        m_suppStartBannerComboBox->setCurrentIndex(1);
    else
        m_suppStartBannerComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Suppress Startup Banner"),
                                m_suppStartBannerComboBox,
                                tr("Suppress the display of the startup banner and information messages."));

    m_ignoreImpLibComboBox = new QComboBox;
    m_ignoreImpLibComboBox->addItem(tr("No"), QVariant(false));
    m_ignoreImpLibComboBox->addItem(tr("Yes"), QVariant(true));

    if (m_tool->ignoreImportLibrary())
        m_ignoreImpLibComboBox->setCurrentIndex(1);
    else
        m_ignoreImpLibComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Ignore Import Library"),
                                m_ignoreImpLibComboBox,
                                tr("Specifies that the import library generated by this configuration should not be imported into dependent projects."));

    m_regOutputComboBox = new QComboBox;
    m_regOutputComboBox->addItem(tr("No"), QVariant(false));
    m_regOutputComboBox->addItem(tr("Yes"), QVariant(true));

    if (m_tool->registerOutput())
        m_regOutputComboBox->setCurrentIndex(1);
    else
        m_regOutputComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Register Output"),
                                m_regOutputComboBox,
                                tr("Specifies whether to register the primary output of this build."));

    m_perUserRedComboBox = new QComboBox;
    m_perUserRedComboBox->addItem(tr("No"), QVariant(false));
    m_perUserRedComboBox->addItem(tr("Yes"), QVariant(true));

    if (m_tool->perUserRedirection())
        m_perUserRedComboBox->setCurrentIndex(1);
    else
        m_perUserRedComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Per User Redirection"),
                                m_perUserRedComboBox,
                                tr("When Register Output is enabled, Per-user redirection forces registry writes to HKEY_CLASSES_ROOT to be redirected to HKEY_CURRENT_USER."));

    m_additLibDirLineEdit = new LineEdit;
    m_additLibDirLineEdit->setTextList(m_tool->additionalLibraryDirectories(), QLatin1String(";"));
    basicWidget->insertTableRow(tr("Additional Library Directories"),
                                m_additLibDirLineEdit,
                                tr("Specifies one or more additional paths to search for libraries; configuration specific; use semi-colon delimited list if more than one."));

    m_linkLibDepComboBox = new QComboBox;
    m_linkLibDepComboBox->addItem(tr("No"), QVariant(false));
    m_linkLibDepComboBox->addItem(tr("Yes"), QVariant(true));

    if (m_tool->linkLibraryDependencies())
        m_linkLibDepComboBox->setCurrentIndex(1);
    else
        m_linkLibDepComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Link Library Dependencies"),
                                m_linkLibDepComboBox,
                                tr("Specifies whether or not library outputs from project dependencies are automatically linked in."));

    m_useLibDepInputsComboBox = new QComboBox;
    m_useLibDepInputsComboBox->addItem(tr("No"), QVariant(false));
    m_useLibDepInputsComboBox->addItem(tr("Yes"), QVariant(true));

    if (m_tool->useLibraryDependencyInputs())
        m_useLibDepInputsComboBox->setCurrentIndex(1);
    else
        m_useLibDepInputsComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Use Library Dependency Inputs"),
                                m_useLibDepInputsComboBox,
                                tr("Specifies whether or not the inputs to the librarian tool are used rather than the library file itself when linking in outputs of project dependencies."));

    m_useUnicodeRespFilesComboBox = new QComboBox;
    m_useUnicodeRespFilesComboBox->addItem(tr("No"), QVariant(false));
    m_useUnicodeRespFilesComboBox->addItem(tr("Yes"), QVariant(true));

    if (m_tool->useUnicodeResponseFiles())
        m_useUnicodeRespFilesComboBox->setCurrentIndex(1);
    else
        m_useUnicodeRespFilesComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Use Unicode Response Files"),
                                m_useUnicodeRespFilesComboBox,
                                tr("Instructs the project system to generate UNICODE response files when spawning the linker. Set this property to “Yes” when files in the project have UNICODE paths."));
    return basicWidget;
}

QWidget *LinkerToolWidget::createInputWidget()
{
    BasicConfigurationWidget *basicWidget = new BasicConfigurationWidget;

    m_addDepLineEdit = new LineEdit;
    m_addDepLineEdit->setTextList(m_tool->additionalDependencies(), QLatin1String(";"));
    basicWidget->insertTableRow(tr("Additional Dependencies"),
                                m_addDepLineEdit,
                                tr("Specifies additional items to add to the link line (ex:kernel32.lib); configuration specific."));

    m_ignoreAllDefLibComboBox = new QComboBox;
    m_ignoreAllDefLibComboBox->addItem(tr("No"), QVariant(false));
    m_ignoreAllDefLibComboBox->addItem(tr("Yes (/NODEFAULTLIB)"), QVariant(true));

    if (m_tool->ignoreAllDefaultLibraries())
        m_ignoreAllDefLibComboBox->setCurrentIndex(1);
    else
        m_ignoreAllDefLibComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Ignore All Default Libraries"),
                                m_ignoreAllDefLibComboBox,
                                tr("Ignore all default libraries during linking."));

    m_ignoreDefLibNamesLineEdit = new LineEdit;
    m_ignoreDefLibNamesLineEdit->setTextList(m_tool->ignoreDefaultLibraryNames(), QLatin1String(";"));
    basicWidget->insertTableRow(tr("Ignore Specific Library"),
                                m_ignoreDefLibNamesLineEdit,
                                tr("Specifies one or more names of default libraries to ignore; separate multiple libraries with semi-colon."));

    m_moduleDefFileLineEdit = new QLineEdit;
    m_moduleDefFileLineEdit->setText(m_tool->moduleDefinitionFile());
    basicWidget->insertTableRow(tr("Module Definition File"),
                                m_moduleDefFileLineEdit,
                                tr("Use specified module definition file during executable creation."));

    m_addModuleNamesToASMLineEdit = new LineEdit;
    m_addModuleNamesToASMLineEdit->setTextList(m_tool->addModuleNamesToAssembly(), QLatin1String(";"));
    basicWidget->insertTableRow(tr("Add Module to Assembly"),
                                m_addModuleNamesToASMLineEdit,
                                tr("Import the specific non-assembly file into the final output."));

    m_embedManagedResFileLineEdit = new LineEdit;
    m_embedManagedResFileLineEdit->setTextList(m_tool->embedManagedResourceFile(), QLatin1String(";"));
    basicWidget->insertTableRow(tr("Embed Managed Resource File"),
                                m_embedManagedResFileLineEdit,
                                tr("Embed the specified .NET (or .NET Framework) resource file."));

    m_forceSymRefLineEdit = new LineEdit;
    m_forceSymRefLineEdit->setTextList(m_tool->forceSymbolReferences(), QString());
    basicWidget->insertTableRow(tr("Force Symbol References"),
                                m_forceSymRefLineEdit,
                                tr("Force the linker to include a reference to this symbol."));

    m_delayLoadDLLsLineEdit = new LineEdit;
    m_delayLoadDLLsLineEdit->setTextList(m_tool->delayLoadDLLs(), QLatin1String(";"));
    basicWidget->insertTableRow(tr("Delay Load DLLs"),
                                m_delayLoadDLLsLineEdit,
                                tr("Specifies one or more DLLs for delayed loading; use semi-colon delimited list if more than once."));

    m_asmLinkResLineEdit = new LineEdit;
    m_asmLinkResLineEdit->setTextList(m_tool->assemblyLinkResource(), QString());
    basicWidget->insertTableRow(tr("Assembly Link Resource"),
                                m_asmLinkResLineEdit,
                                tr("Links a resource file to the output assembly. The resource file is not placed in the output assembly, unlike the /ASSEMBLYRESOURCE switch."));

    return basicWidget;
}

QWidget *LinkerToolWidget::createManifestFileWidget()
{
    BasicConfigurationWidget *basicWidget = new BasicConfigurationWidget;

    m_generManifestComboBox = new QComboBox;
    m_generManifestComboBox->addItem(tr("No"), QVariant(false));
    m_generManifestComboBox->addItem(tr("Yes"), QVariant(true));

    if (m_tool->generateManifest())
        m_generManifestComboBox->setCurrentIndex(1);
    else
        m_generManifestComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Generate Manifest"),
                                m_generManifestComboBox,
                                tr("Specifies if the linker should always generate a manifest file."));

    m_manifestFileLineEdit = new QLineEdit;
    m_manifestFileLineEdit->setText(m_tool->manifestFile());
    basicWidget->insertTableRow(tr("Manifest File"),
                                m_manifestFileLineEdit,
                                tr("Specifies the name of the manifest file to generate. Only used if Generate Manifest is true."));

    m_additManifestDepLineEdit = new LineEdit;
    m_additManifestDepLineEdit->setTextList(m_tool->additionalManifestDependencies(), QLatin1String(";"));
    basicWidget->insertTableRow(tr("Additional Manifest Dependencies"),
                                m_additManifestDepLineEdit,
                                tr("Specifies the additional XML manifest fragments the linker will put in the manifest file."));

    m_allowIsolationComboBox = new QComboBox;
    m_allowIsolationComboBox->addItem(tr("Don't allow side-by-side isolation (/ALLOWISOLATION:NO)"), QVariant(false));
    m_allowIsolationComboBox->addItem(tr("Yes"), QVariant(true));

    if (m_tool->allowIsolation())
        m_allowIsolationComboBox->setCurrentIndex(1);
    else
        m_allowIsolationComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Allow Isolation"),
                                m_allowIsolationComboBox,
                                tr("Specifies manifest file lookup behavior for side-by-side asemblies."));

    m_enableUacComboBox = new QComboBox;
    m_enableUacComboBox->addItem(tr("No"), QVariant(false));
    m_enableUacComboBox->addItem(tr("Yes"), QVariant(true));

    if (m_tool->enableUAC())
        m_enableUacComboBox->setCurrentIndex(1);
    else
        m_enableUacComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Enable User AccountControll (UAC)"),
                                m_enableUacComboBox,
                                tr("Specifies whether or not User Account Control is enabled."));

    m_uacExecLvlComboBox = new QComboBox;
    m_uacExecLvlComboBox->addItem(tr("asInvoker"), QVariant(0));
    m_uacExecLvlComboBox->addItem(tr("highestAvailable"), QVariant(1));
    m_uacExecLvlComboBox->addItem(tr("requireAdministrator"), QVariant(2));

    m_uacExecLvlComboBox->setCurrentIndex(m_tool->uacExecutionLevel());

    basicWidget->insertTableRow(tr("UAC Execution Level"),
                                m_uacExecLvlComboBox,
                                tr("Specifies the requested execution level for the application when running with User Account Control."));

    m_uacUIAccComboBox = new QComboBox;
    m_uacUIAccComboBox->addItem(tr("No"), QVariant(false));
    m_uacUIAccComboBox->addItem(tr("Yes"), QVariant(true));

    if (m_tool->uacUIAAccess())
        m_uacUIAccComboBox->setCurrentIndex(1);
    else
        m_uacUIAccComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("UAC Bypass UI Protection"),
                                m_uacUIAccComboBox,
                                tr("Specifies whether or not to bypass user interface protection levels for other windows on the desktop. Set this property to 'Yes' only for accessibility applications."));

    return basicWidget;
}

QWidget *LinkerToolWidget::createDebuggingWidget()
{
    BasicConfigurationWidget *basicWidget = new BasicConfigurationWidget;

    m_genDebugInfoComboBox = new QComboBox;
    m_genDebugInfoComboBox->addItem(tr("No"), QVariant(false));
    m_genDebugInfoComboBox->addItem(tr("Yes (/DEBUG)"), QVariant(true));

    if (m_tool->generateDebugInformation())
        m_genDebugInfoComboBox->setCurrentIndex(1);
    else
        m_genDebugInfoComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Generate Debug Info"),
                                m_genDebugInfoComboBox,
                                tr("Enables generation of debug information."));

    m_progDatabFileLineEdit = new QLineEdit;
    m_progDatabFileLineEdit->setText(m_tool->programDatabaseFile());
    basicWidget->insertTableRow(tr("Generate Program Database File"),
                                m_progDatabFileLineEdit,
                                tr("Enables generation of a program database “.pdb” file; requires 'Generate Debug Info' option to be enabled."));

    m_stripPrivSymbLineEdit = new QLineEdit;
    m_stripPrivSymbLineEdit->setText(m_tool->stripPrivateSymbols());
    basicWidget->insertTableRow(tr("Strip Private Symbols"),
                                m_stripPrivSymbLineEdit,
                                tr("Do not put private symbols into the generated PDB file specified."));

    m_genMapFileComboBox = new QComboBox;
    m_genMapFileComboBox->addItem(tr("No"), QVariant(false));
    m_genMapFileComboBox->addItem(tr("Yes (/MAP)"), QVariant(true));

    if (m_tool->generateMapFile())
        m_genMapFileComboBox->setCurrentIndex(1);
    else
        m_genMapFileComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("GenerateMapFile"),
                                m_genMapFileComboBox,
                                tr("Enables generation of map file during linking."));

    m_mapFileNameLineEdit = new QLineEdit;
    m_mapFileNameLineEdit->setText(m_tool->mapFileName());
    basicWidget->insertTableRow(tr("Map File Name"),
                                m_mapFileNameLineEdit,
                                tr("Specifies a name for the mapfile."));

    m_mapExportsComboBox = new QComboBox;
    m_mapExportsComboBox->addItem(tr("No"), QVariant(false));
    m_mapExportsComboBox->addItem(tr("Yes (/MAPINFO:EXPORTS)"), QVariant(true));

    if (m_tool->mapExports())
        m_mapExportsComboBox->setCurrentIndex(1);
    else
        m_mapExportsComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Map Exports"),
                                m_mapExportsComboBox,
                                tr("Includes exported functions in map file information."));

    m_asmDebugComboBox = new QComboBox;
    m_asmDebugComboBox->addItem(tr("No Debuggable attribute emitted"), QVariant(0));
    m_asmDebugComboBox->addItem(tr("Runtime tracking and disable optimizations (/ASSEMBLYDEBUG)"), QVariant(1));
    m_asmDebugComboBox->addItem(tr("No runtime tracking and enable optimizations (/ASSEMBLYDEBUG:DISABLE)"), QVariant(2));

    m_asmDebugComboBox->setCurrentIndex(m_tool->assemblyDebug());

    basicWidget->insertTableRow(tr("Debuggable Assembly"),
                                m_asmDebugComboBox,
                                tr("Emits the Debuggable attributes to the assembly."));

    return basicWidget;
}

QWidget *LinkerToolWidget::createSystemWidget()
{
    BasicConfigurationWidget *basicWidget = new BasicConfigurationWidget;

    m_subSystemComboBox = new QComboBox;
    m_subSystemComboBox->addItem(tr("Not Set"), QVariant(0));
    m_subSystemComboBox->addItem(tr("Console (/SUBSYSTEM:CONSOLE)"), QVariant(1));
    m_subSystemComboBox->addItem(tr("Windows (/SUBSYSTEM:WINDOWS)"), QVariant(2));
    m_subSystemComboBox->addItem(tr("Native (/SUBSYSTEM:NATIVE)"), QVariant(3));
    m_subSystemComboBox->addItem(tr("EFI Application (/SUBSYSTEM:EFI_APPLICATION)"), QVariant(4));
    m_subSystemComboBox->addItem(tr("EFI Boot Service Driver (/SUBSYSTEM:EFI_BOOT_SERVICE_DRIVER)"), QVariant(5));
    m_subSystemComboBox->addItem(tr("EFI ROM (/SUBSYSTEM:EFI_ROM)"), QVariant(6));
    m_subSystemComboBox->addItem(tr("EFI Runtime (/SUBSYSTEM:EFI_RUNTIME_DRIVER)"), QVariant(7));
    m_subSystemComboBox->addItem(tr("WindowsCE (/SUBSYSTEM:WINDOWSCE)"), QVariant(8));

    m_subSystemComboBox->setCurrentIndex(m_tool->subSystem());

    basicWidget->insertTableRow(tr("Sub System"),
                                m_subSystemComboBox,
                                tr("Specifies subsystem for the linker."));

    m_heapResSizeLineEdit = new QLineEdit;
    m_heapResSizeLineEdit->setValidator(new QIntValidator(m_heapResSizeLineEdit));
    m_heapResSizeLineEdit->setText(QVariant(m_tool->heapReserveSize()).toString());
    basicWidget->insertTableRow(tr("Heap Reserve Size"),
                                m_heapResSizeLineEdit,
                                tr("Specifies total heap allocation size in virtual memory."));

    m_heapCommitSizeLineEdit = new QLineEdit;
    m_heapCommitSizeLineEdit->setValidator(new QIntValidator(m_heapCommitSizeLineEdit));
    m_heapCommitSizeLineEdit->setText(QVariant(m_tool->heapCommitSize()).toString());
    basicWidget->insertTableRow(tr("Heap Commit Size"),
                                m_heapCommitSizeLineEdit,
                                tr("Specifies total heap size in physical memory."));

    m_stackReserveSizeLineEdit = new QLineEdit;
    m_stackReserveSizeLineEdit->setValidator(new QIntValidator(m_stackReserveSizeLineEdit));
    m_stackReserveSizeLineEdit->setText(QVariant(m_tool->stackReserveSize()).toString());
    basicWidget->insertTableRow(tr("Stack Reserve Size"),
                                m_stackReserveSizeLineEdit,
                                tr("Specifies the total stack allocation size in virtual memory."));

    m_stackCommitSize = new QLineEdit;
    m_stackCommitSize->setValidator(new QIntValidator(m_stackCommitSize));
    m_stackCommitSize->setText(QVariant(m_tool->stackCommitSize()).toString());
    basicWidget->insertTableRow(tr("Stack Commit Size"),
                                m_stackCommitSize,
                                tr("Specifies the total stack allocation size in physical memory."));

    m_largeAddressAwareComboBox = new QComboBox;
    m_largeAddressAwareComboBox->addItem(tr("Default"), QVariant(0));
    m_largeAddressAwareComboBox->addItem(tr("Do Not Support Addresses Larger Than 2 Gigabytes (/LARGEADDRESSAWARE:NO)"), QVariant(1));
    m_largeAddressAwareComboBox->addItem(tr("Support Addresses Larger Than 2 Gigabytes (/LARGEADDRESSAWARE)"), QVariant(2));

    m_largeAddressAwareComboBox->setCurrentIndex(m_tool->largeAddressAware());

    basicWidget->insertTableRow(tr("Enable Large Addresses"),
                                m_largeAddressAwareComboBox,
                                tr("Enables handling addresses larger than 2 GB."));

    m_terminalServerAwareComboBox = new QComboBox;
    m_terminalServerAwareComboBox->addItem(tr("Default"), QVariant(0));
    m_terminalServerAwareComboBox->addItem(tr("Not Terminal Server Aware (/TSAWARE:NO)"), QVariant(1));
    m_terminalServerAwareComboBox->addItem(tr("Application is Terminal Server Aware (/TSAWARE)"), QVariant(2));

    m_terminalServerAwareComboBox->setCurrentIndex(m_tool->terminalServerAware());

    basicWidget->insertTableRow(tr("Terminal Server"),
                                m_terminalServerAwareComboBox,
                                tr("Enables terminal server awerness."));

    m_swapFromCDComboBox = new QComboBox;
    m_swapFromCDComboBox->addItem(tr("No"), QVariant(false));
    m_swapFromCDComboBox->addItem(tr("Yes (/SWAPRUN:CD)"), QVariant(true));

    if (m_tool->swapRunFromCD())
        m_swapFromCDComboBox->setCurrentIndex(1);
    else
        m_swapFromCDComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Swap Run From CD"),
                                m_swapFromCDComboBox,
                                tr("Run application from the swap location of the CD."));

    m_swapFromNetComboBox = new QComboBox;
    m_swapFromNetComboBox->addItem(tr("No"), QVariant(false));
    m_swapFromNetComboBox->addItem(tr("Yes (/SWAPRUN:NET)"), QVariant(true));

    if (m_tool->swapRunFromNet())
        m_swapFromNetComboBox->setCurrentIndex(1);
    else
        m_swapFromNetComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Swap Run From Network"),
                                m_swapFromNetComboBox,
                                tr("Run application from the swap location of the Net."));

    m_driverComboBox = new QComboBox;
    m_driverComboBox->addItem(tr("Not Set"), QVariant(0));
    m_driverComboBox->addItem(tr("Driver (/DRIVER)"), QVariant(1));
    m_driverComboBox->addItem(tr("Up Only (/DRIVER:UPONLY)"), QVariant(2));
    m_driverComboBox->addItem(tr("WDM (/DRIVER:WDM)"), QVariant(3));

    m_driverComboBox->setCurrentIndex(m_tool->driver());

    basicWidget->insertTableRow(tr("Driver"),
                                m_driverComboBox,
                                tr("Specifies the driver for the linker."));
    return basicWidget;
}

QWidget *LinkerToolWidget::createOptimizationWidget()
{
    BasicConfigurationWidget *basicWidget = new BasicConfigurationWidget;

    m_optmRefComboBox = new QComboBox;
    m_optmRefComboBox->addItem(tr("Default"), QVariant(0));
    m_optmRefComboBox->addItem(tr("Keep Unreferenced Data (/OPT:NOREF)"), QVariant(1));
    m_optmRefComboBox->addItem(tr("Eliminate Unreferenced Data (/OPT:REF)"), QVariant(2));

    m_optmRefComboBox->setCurrentIndex(m_tool->optimizeReferences());

    basicWidget->insertTableRow(tr("References"),
                                m_optmRefComboBox,
                                tr("Enables elimination of functions and/or data that are never referenced."));

    m_enableCOMDATFoldComboBox = new QComboBox;
    m_enableCOMDATFoldComboBox->addItem(tr("Default"), QVariant(0));
    m_enableCOMDATFoldComboBox->addItem(tr("Do Not Remove Redundant COMDATs (/OPT:NOICF)"), QVariant(1));
    m_enableCOMDATFoldComboBox->addItem(tr("Remove Redundant COMDATs (/OPT:ICF)"), QVariant(2));

    m_enableCOMDATFoldComboBox->setCurrentIndex(m_tool->enableCOMDATFolding());

    basicWidget->insertTableRow(tr("Enable COMDAT Folding"),
                                m_enableCOMDATFoldComboBox,
                                tr("Removes redundant COMDAT symbols from the linker output."));

    m_optmForWin98ComboBox = new QComboBox;
    m_optmForWin98ComboBox->addItem(tr("No"), QVariant(0));
    m_optmForWin98ComboBox->addItem(tr("No (/OPT:NOWIN98)"), QVariant(1));
    m_optmForWin98ComboBox->addItem(tr("Yes (/OPT:WIN98)"), QVariant(2));

    m_optmForWin98ComboBox->setCurrentIndex(m_tool->optimizeForWindows98());

    basicWidget->insertTableRow(tr("Optimize For Windows 98"),
                                m_optmForWin98ComboBox,
                                tr("Align code on 4KB boundaries. This improves performance on Windows 98 systems."));

    m_funcOrderLineEdit = new QLineEdit;
    m_funcOrderLineEdit->setText(m_tool->functionOrder());
    basicWidget->insertTableRow(tr("Function Order"),
                                m_funcOrderLineEdit,
                                tr("Places COMDATs into the image in a predetermined order; specify file name containing the order; no incremental linking."));

    m_profGuidedDatabLineEdit = new QLineEdit;
    m_profGuidedDatabLineEdit->setText(m_tool->profileGuidedDatabase());
    basicWidget->insertTableRow(tr("Profile Guided Database"),
                                m_profGuidedDatabLineEdit,
                                tr("Specifies the database file to use when using profile guided optimizations (/PGD:database). This property should only be used when using Profile Guided Optimizations."));

    m_linkTimeCodeGenComboBox = new QComboBox;
    m_linkTimeCodeGenComboBox->addItem(tr("Default"), QVariant(0));
    m_linkTimeCodeGenComboBox->addItem(tr("Use Link Time Code Generation (/ltcg)"), QVariant(1));
    m_linkTimeCodeGenComboBox->addItem(tr("Profile Guided Optimization - Instrument (/ltcg:pginstrument)"), QVariant(2));
    m_linkTimeCodeGenComboBox->addItem(tr("Profile Guided Optimization - Optimize (/ltcg:pgoptimize)"), QVariant(3));
    m_linkTimeCodeGenComboBox->addItem(tr("Profile Guided Optimization - Update (/ltcg:pgupdate)"), QVariant(4));

    m_linkTimeCodeGenComboBox->setCurrentIndex(m_tool->linkTimeCodeGeneration());

    basicWidget->insertTableRow(tr("Link Time Code Generation"),
                                m_linkTimeCodeGenComboBox,
                                tr("Enables link time code generation of objects compile with Whole Program Optimization set."));
    return basicWidget;
}

QWidget *LinkerToolWidget::createEmbeddedIDLWidget()
{
    BasicConfigurationWidget *basicWidget = new BasicConfigurationWidget;

    m_midlCommFileLineEdit = new LineEdit;
    m_midlCommFileLineEdit->setTextList(m_tool->midlCommandFile(), QLatin1String(";"));
    basicWidget->insertTableRow(tr("MIDL Commands"),
                                m_midlCommFileLineEdit,
                                tr("Specifies a response file or its desired contents for MIDL commands to use."));

    m_ignoreEmbedIDLComboBox = new QComboBox;
    m_ignoreEmbedIDLComboBox->addItem(tr("No"), QVariant(false));
    m_ignoreEmbedIDLComboBox->addItem(tr("Yes (/IGNOREIDL)"), QVariant(true));

    if (m_tool->ignoreEmbeddedIDL())
        m_ignoreEmbedIDLComboBox->setCurrentIndex(1);
    else
        m_ignoreEmbedIDLComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("IgnoreEmbeddedIDL"),
                                m_ignoreEmbedIDLComboBox,
                                tr("Ignore embedded .idlsym sections of object files."));

    m_mergedIDLLineEdit = new QLineEdit;
    m_mergedIDLLineEdit->setText(m_tool->mergedIDLBaseFileName());
    basicWidget->insertTableRow(tr("Merged IDL Base File Name"),
                                m_mergedIDLLineEdit,
                                tr("Specifies the basename of the .IDL file that contains the contents of the merged IDLSYM sections."));

    m_typeLibFileLineEdit = new QLineEdit;
    m_typeLibFileLineEdit->setText(m_tool->typeLibraryFile());
    basicWidget->insertTableRow(tr("Type Library"),
                                m_typeLibFileLineEdit,
                                tr("Specifies the name of the type library file."));

    m_typeLibResIDLineEdit = new QLineEdit;
    m_typeLibResIDLineEdit->setValidator(new QIntValidator(m_typeLibResIDLineEdit));
    m_typeLibResIDLineEdit->setText(QVariant(m_tool->typeLibraryResourceID()).toString());
    basicWidget->insertTableRow(tr("TypeLib Resource ID"),
                                m_typeLibResIDLineEdit,
                                tr("Specifies the ID number to assign to the .tlb file in the compiled resources."));
    return basicWidget;
}

QWidget *LinkerToolWidget::createAdvancedWidget()
{
    BasicConfigurationWidget *basicWidget = new BasicConfigurationWidget;

    m_entryPointSym = new QLineEdit;
    m_entryPointSym->setText(m_tool->entryPointSymbol());
    basicWidget->insertTableRow(tr("Entry Point"),
                                m_entryPointSym,
                                tr("Sets the starting address for an .exe file or DLL."));

    m_resourceOnlyDLLComboBox = new QComboBox;
    m_resourceOnlyDLLComboBox->addItem(tr("No"), QVariant(false));
    m_resourceOnlyDLLComboBox->addItem(tr("Yes (/NOENTRY)"), QVariant(true));

    if (m_tool->resourceOnlyDLL())
        m_resourceOnlyDLLComboBox->setCurrentIndex(1);
    else
        m_resourceOnlyDLLComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("No Entry Point"),
                                m_resourceOnlyDLLComboBox,
                                tr("Create DLL with no entry point; incompatible with setting the 'Entry Point' option."));

    m_setchecksumComboBox = new QComboBox;
    m_setchecksumComboBox->addItem(tr("No"), QVariant(false));
    m_setchecksumComboBox->addItem(tr("Yes (/RELEASE)"), QVariant(true));

    if (m_tool->checksum())
        m_setchecksumComboBox->setCurrentIndex(1);
    else
        m_setchecksumComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Set Checksum"),
                                m_setchecksumComboBox,
                                tr("Enables setting the checksum in the header of a .exe."));

    m_baseAddressLineEdit = new QLineEdit;
    m_baseAddressLineEdit->setText(m_tool->baseAddress());
    basicWidget->insertTableRow(tr("Base Address"),
                                m_baseAddressLineEdit,
                                tr("Specifies base address for program; numeric or string."));

    m_randBaseAddressComboBox = new QComboBox;
    m_randBaseAddressComboBox->addItem(tr("Default"), QVariant(0));
    m_randBaseAddressComboBox->addItem(tr("Disable Image Randomization (/DYNAMICBASE:NO)"), QVariant(1));
    m_randBaseAddressComboBox->addItem(tr("Enable Image Randomization (/DYNAMICBASE)"), QVariant(2));

    m_randBaseAddressComboBox->setCurrentIndex(m_tool->randomizedBaseAddress());

    basicWidget->insertTableRow(tr("Randomized Base Address"),
                                m_randBaseAddressComboBox,
                                tr("Generates an executable image that can be randomly rebased at load-time by using the address space layout randomization (ASLR) feature."));

    m_fixedBaseAddressComboBox = new QComboBox;
    m_fixedBaseAddressComboBox->addItem(tr("Default"), QVariant(0));
    m_fixedBaseAddressComboBox->addItem(tr("Generate a relocation section (/FIXED:NO)"), QVariant(1));
    m_fixedBaseAddressComboBox->addItem(tr("Image must be loaded at a fixed address (/FIXED)"), QVariant(2));

    m_fixedBaseAddressComboBox->setCurrentIndex(m_tool->fixedBaseAddress());

    basicWidget->insertTableRow(tr("Fixed Base Address"),
                                m_fixedBaseAddressComboBox,
                                tr("Specifies if image must be loaded at fixed address."));

    m_dataExecPrevComboBox = new QComboBox;
    m_dataExecPrevComboBox->addItem(tr("Default"), QVariant(0));
    m_dataExecPrevComboBox->addItem(tr("Image is not compatible with DEP (/NXCOMPAT:NO)"), QVariant(1));
    m_dataExecPrevComboBox->addItem(tr("Image is compatible with DEP (/NXCOMPAT)"), QVariant(2));

    m_dataExecPrevComboBox->setCurrentIndex(m_tool->dataExecutionPrevention());

    basicWidget->insertTableRow(tr("Data Execution Prevention (DEP)"),
                                m_dataExecPrevComboBox,
                                tr("Indicates that an executable was tested to be compatible with the Windows Data Execution Prevention feature."));

    m_turnOffAsmGenComboBox = new QComboBox;
    m_turnOffAsmGenComboBox->addItem(tr("No"), QVariant(false));
    m_turnOffAsmGenComboBox->addItem(tr("Yes (/NOASSEMBLY)"), QVariant(true));

    if (m_tool->turnOffAssemblyGeneration())
        m_turnOffAsmGenComboBox->setCurrentIndex(1);
    else
        m_turnOffAsmGenComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Turn Of fAssembly Generation"),
                                m_turnOffAsmGenComboBox,
                                tr("Specifies that no assembly will be generated even though common language runtime information is present in the object file."));

    m_suppUnloadOfDelayDLLComboBox = new QComboBox;
    m_suppUnloadOfDelayDLLComboBox->addItem(tr("Don't Support Unload"), QVariant(false));
    m_suppUnloadOfDelayDLLComboBox->addItem(tr("Support Unload (/DELAY:UNLOAD)"), QVariant(true));

    if (m_tool->supportUnloadOfDelayLoadedDLL())
        m_suppUnloadOfDelayDLLComboBox->setCurrentIndex(1);
    else
        m_suppUnloadOfDelayDLLComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Delay Loaded DLL"),
                                m_suppUnloadOfDelayDLLComboBox,
                                tr("Specifies to allow explicit unloading of the delayed load DLLs."));

    m_importLibLineEdit = new QLineEdit;
    m_importLibLineEdit->setText(m_tool->importLibrary());
    basicWidget->insertTableRow(tr("Import Library"),
                                m_importLibLineEdit,
                                tr("Specifies which import library to generate."));

    m_mergeSectionsLineEdit = new QLineEdit;
    m_mergeSectionsLineEdit->setText(m_tool->mergeSections());
    basicWidget->insertTableRow(tr("Merge Sections"),
                                m_mergeSectionsLineEdit,
                                tr("Causes the linker to merge section 'from' into section 'to' does not exist, section 'from' is renamed as 'to'."));

    m_targetMachineComboBox = new QComboBox;
    m_targetMachineComboBox->addItem(tr("Not Set"), QVariant(0));
    m_targetMachineComboBox->addItem(tr("MachineX86 (/MACHINE:X86)"), QVariant(1));
    m_targetMachineComboBox->addItem(tr("MachineAM33 (/MACHINE:AM33)"), QVariant(2));
    m_targetMachineComboBox->addItem(tr("MachineARM (/MACHINE:ARM)"), QVariant(3));
    m_targetMachineComboBox->addItem(tr("MachineEBC (/MACHINE:EBC)"), QVariant(4));
    m_targetMachineComboBox->addItem(tr("MachineIA64 (/MACHINE:IA64)"), QVariant(5));
    m_targetMachineComboBox->addItem(tr("MachineM32R (/MACHINE:M32R)"), QVariant(6));
    m_targetMachineComboBox->addItem(tr("MachineMIPS (/MACHINE:MIPS)"), QVariant(7));
    m_targetMachineComboBox->addItem(tr("MachineMIPS16 (/MACHINE:MIPS16)"), QVariant(8));
    m_targetMachineComboBox->addItem(tr("MachineMIPSFPU (/MACHINE:MIPSFPU)"), QVariant(9));
    m_targetMachineComboBox->addItem(tr("MachineMIPSFPU16 (/MACHINE:MIPSFPU16)"), QVariant(10));
    m_targetMachineComboBox->addItem(tr("MachineMIPSR41XX (/MACHINE:MIPSR41XX)"), QVariant(11));
    m_targetMachineComboBox->addItem(tr("MachineSH3 (/MACHINE:SH3)"), QVariant(12));
    m_targetMachineComboBox->addItem(tr("MachineSH3DSP (/MACHINE:SH3DSP)"), QVariant(13));
    m_targetMachineComboBox->addItem(tr("MachineSH4 (/MACHINE:SH4)"), QVariant(14));
    m_targetMachineComboBox->addItem(tr("MachineSH5 (/MACHINE:SH5)"), QVariant(15));
    m_targetMachineComboBox->addItem(tr("MachineTHUMB (/MACHINE:THUMB)"), QVariant(16));
    m_targetMachineComboBox->addItem(tr("MachineX64 (/MACHINE:X64)"), QVariant(17));

    m_targetMachineComboBox->setCurrentIndex(m_tool->targetMachine());

    basicWidget->insertTableRow(tr("Target Machine"),
                                m_targetMachineComboBox,
                                tr("Specifies the subsystem for the linker."));

    m_profileComboBox = new QComboBox;
    m_profileComboBox->addItem(tr("No"), QVariant(false));
    m_profileComboBox->addItem(tr("Enable Profiling information (/PROFILE)"), QVariant(true));

    if (m_tool->profile())
        m_profileComboBox->setCurrentIndex(1);
    else
        m_profileComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Profile"),
                                m_profileComboBox,
                                tr("Produces an output file that can be used with the Enterprise Developer performance profiler."));

    m_clrThreadAttrComboBox = new QComboBox;
    m_clrThreadAttrComboBox->addItem(tr("No threading attribute set"), QVariant(0));
    m_clrThreadAttrComboBox->addItem(tr("MTA threading attribute (/CLRTHREADATTRIBUTE:MTA)"), QVariant(1));
    m_clrThreadAttrComboBox->addItem(tr("STA threading attribute (/CLRTHREADATTRIBUTE:STA)"), QVariant(2));

    m_clrThreadAttrComboBox->setCurrentIndex(m_tool->clrThreadAttribute());

    basicWidget->insertTableRow(tr("CLR Thread Attribute"),
                                m_clrThreadAttrComboBox,
                                tr("Specifies the threading attribute for the entry point of your CLR program."));

    m_clrImageTypeComboBox = new QComboBox;
    m_clrImageTypeComboBox->addItem(tr("Default image type"), QVariant(0));
    m_clrImageTypeComboBox->addItem(tr("Force IJW image (/CLRIMAGETYPE:IJW)"), QVariant(1));
    m_clrImageTypeComboBox->addItem(tr("Force pure IL image (/CLRIMAGETYPE:PURE)"), QVariant(2));
    m_clrImageTypeComboBox->addItem(tr("Force safe IL image (/CLRIMAGETYPE:SAFE)"), QVariant(3));

    m_clrImageTypeComboBox->setCurrentIndex(m_tool->clrImageType());

    basicWidget->insertTableRow(tr("CLR Image Type"),
                                m_clrImageTypeComboBox,
                                tr("Specifies the type of a CLR image. Useful when linking object files of different types."));

    m_keyFileLineEdit = new QLineEdit;
    m_keyFileLineEdit->setText(m_tool->keyFile());
    basicWidget->insertTableRow(tr("Key File"),
                                m_keyFileLineEdit,
                                tr("Specifies the file that contains the key for strongly naming the output assembly."));

    m_keyContainerLineEdit = new QLineEdit;
    m_keyContainerLineEdit->setText(m_tool->keyContainer());
    basicWidget->insertTableRow(tr("Key Container"),
                                m_keyContainerLineEdit,
                                tr("Specifies the named container of the key for strongly naming the output assembly."));

    m_delaySignComboBox = new QComboBox;
    m_delaySignComboBox->addItem(tr("No"), QVariant(false));
    m_delaySignComboBox->addItem(tr("Yes (/DELAYSIGN)"), QVariant(true));

    if (m_tool->delaySign())
        m_delaySignComboBox->setCurrentIndex(1);
    else
        m_delaySignComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Delay Sign"),
                                m_delaySignComboBox,
                                tr("Indicates whether the output assembly should be delayed signed."));

    m_errReportComboBox = new QComboBox;
    m_errReportComboBox->addItem(tr("Default"), QVariant(0));
    m_errReportComboBox->addItem(tr("Prompt Immediately (/ERRORREPORT:PROMPT)"), QVariant(1));
    m_errReportComboBox->addItem(tr("Queue For Next Login (/ERRORREPORT:QUEUE)"), QVariant(2));

    m_errReportComboBox->setCurrentIndex(m_tool->errorReporting());

    basicWidget->insertTableRow(tr("Error Reporting"),
                                m_errReportComboBox,
                                tr("Specifies how internal tool errors should be reported back to Microsoft. The default in the IDE is prompt. The default from command line builds is queue."));

    m_clrUnmanagedCodeCheckComboBox = new QComboBox;
    m_clrUnmanagedCodeCheckComboBox->addItem(tr("NO"), QVariant(false));
    m_clrUnmanagedCodeCheckComboBox->addItem(tr("Apply Unmanaged Code Check"), QVariant(true));

    if (m_tool->clrUnmanagedCodeCheck())
        m_clrUnmanagedCodeCheckComboBox->setCurrentIndex(1);
    else
        m_clrUnmanagedCodeCheckComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("CLR Unmanaged Code Check"),
                                m_clrUnmanagedCodeCheckComboBox,
                                tr("Specifies whether the linker wil apply T:System.Security.SuppressUnmanagedCodeSecurityAttribute to linker -generated Pinvoke calls from managed code into native DLLs."));

    return basicWidget;
}

void LinkerToolWidget::saveGeneralData()
{
    m_tool->setOutputFile(m_outputFileLineEdit->text());

    if (m_showProgComboBox->currentIndex() >= 0) {
        QVariant data = m_showProgComboBox->itemData(m_showProgComboBox->currentIndex(),
                                                     Qt::UserRole);
        m_tool->setShowProgress(static_cast<ShowProgressEnum>(data.toInt()));
    }

    m_tool->setVersion(m_versionLineEdit->text());

    if (m_linkIncComboBox->currentIndex() >= 0) {
        QVariant data = m_linkIncComboBox->itemData(m_linkIncComboBox->currentIndex(),
                                                    Qt::UserRole);
        m_tool->setLinkIncremental(static_cast<LinkIncrementalEnum>(data.toInt()));
    }

    if (m_suppStartBannerComboBox->currentIndex() >= 0) {
        QVariant data = m_suppStartBannerComboBox->itemData(m_suppStartBannerComboBox->currentIndex(),
                                                            Qt::UserRole);
        m_tool->setSuppressStartupBanner(data.toBool());
    }

    if (m_ignoreImpLibComboBox->currentIndex() >= 0) {
        QVariant data = m_ignoreImpLibComboBox->itemData(m_ignoreImpLibComboBox->currentIndex(),
                                                         Qt::UserRole);
        m_tool->setIgnoreImportLibrary(data.toBool());
    }

    if (m_regOutputComboBox->currentIndex() >= 0) {
        QVariant data = m_regOutputComboBox->itemData(m_regOutputComboBox->currentIndex(),
                                                      Qt::UserRole);
        m_tool->setRegisterOutput(data.toBool());
    }

    if (m_perUserRedComboBox->currentIndex() >= 0) {
        QVariant data = m_perUserRedComboBox->itemData(m_perUserRedComboBox->currentIndex(),
                                                       Qt::UserRole);
        m_tool->setPerUserRedirection(data.toBool());
    }

    m_tool->setAdditionalLibraryDirectories(m_additLibDirLineEdit->textToList(QLatin1String(";")));

    if (m_linkLibDepComboBox->currentIndex() >= 0) {
        QVariant data = m_linkLibDepComboBox->itemData(m_linkLibDepComboBox->currentIndex(),
                                                       Qt::UserRole);
        m_tool->setLinkLibraryDependencies(data.toBool());
    }

    if (m_useLibDepInputsComboBox->currentIndex() >= 0) {
        QVariant data = m_useLibDepInputsComboBox->itemData(m_useLibDepInputsComboBox->currentIndex(),
                                                            Qt::UserRole);
        m_tool->setUseLibraryDependencyInputs(data.toBool());
    }

    if (m_useUnicodeRespFilesComboBox->currentIndex() >= 0) {
        QVariant data = m_useUnicodeRespFilesComboBox->itemData(m_useUnicodeRespFilesComboBox->currentIndex(),
                                                                Qt::UserRole);
        m_tool->setUseUnicodeResponseFiles(data.toBool());
    }
}

void LinkerToolWidget::saveInputData()
{
    m_tool->setAdditionalDependencies(m_addDepLineEdit->textToList(QLatin1String(";")));

    if (m_ignoreAllDefLibComboBox->currentIndex() >= 0) {
        QVariant data = m_ignoreAllDefLibComboBox->itemData(m_ignoreAllDefLibComboBox->currentIndex(),
                                                            Qt::UserRole);
        m_tool->setIgnoreAllDefaultLibraries(data.toBool());
    }

    m_tool->setIgnoreDefaultLibraryNames(m_ignoreDefLibNamesLineEdit->textToList(QLatin1String(";")));
    m_tool->setModuleDefinitionFile(m_moduleDefFileLineEdit->text());
    m_tool->setAddModuleNamesToAssembly(m_addModuleNamesToASMLineEdit->textToList(QLatin1String(";")));
    m_tool->setEmbedManagedResourceFile(m_embedManagedResFileLineEdit->textToList(QLatin1String(";")));
    m_tool->setForceSymbolReferences(m_forceSymRefLineEdit->textToList(QLatin1String(";")));
    m_tool->setDelayLoadDLLs(m_delayLoadDLLsLineEdit->textToList(QLatin1String(";")));
    m_tool->setAssemblyLinkResource(m_asmLinkResLineEdit->textToList(QLatin1String(";")));
}

void LinkerToolWidget::saveManifestFileData()
{
    if (m_generManifestComboBox->currentIndex() >= 0) {
        QVariant data = m_generManifestComboBox->itemData(m_generManifestComboBox->currentIndex(),
                                                          Qt::UserRole);
        m_tool->setGenerateManifest(data.toBool());
    }

    m_tool->setManifestFile(m_manifestFileLineEdit->text());
    m_tool->setAdditionalManifestDependencies(m_additManifestDepLineEdit->textToList(QLatin1String(";")));

    if (m_allowIsolationComboBox->currentIndex() >= 0) {
        QVariant data = m_allowIsolationComboBox->itemData(m_allowIsolationComboBox->currentIndex(),
                                                           Qt::UserRole);
        m_tool->setAllowIsolation(data.toBool());
    }

    if (m_enableUacComboBox->currentIndex() >= 0) {
        QVariant data = m_enableUacComboBox->itemData(m_enableUacComboBox->currentIndex(),
                                                      Qt::UserRole);
        m_tool->setEnableUAC(data.toBool());
    }

    if (m_uacExecLvlComboBox->currentIndex() >= 0) {
        QVariant data = m_uacExecLvlComboBox->itemData(m_uacExecLvlComboBox->currentIndex(),
                                                       Qt::UserRole);
        m_tool->setUACExecutionLevel(static_cast<UACExecutionLevelEnum>(data.toInt()));
    }

    if (m_uacUIAccComboBox->currentIndex() >= 0) {
        QVariant data = m_uacUIAccComboBox->itemData(m_uacUIAccComboBox->currentIndex(),
                                                     Qt::UserRole);
        m_tool->setUACUIAccess(data.toBool());
    }
}

void LinkerToolWidget::saveDebuggingData()
{
    if (m_genDebugInfoComboBox->currentIndex() >= 0) {
        QVariant data = m_genDebugInfoComboBox->itemData(m_genDebugInfoComboBox->currentIndex(),
                                                         Qt::UserRole);
        m_tool->setGenerateDebugInformation(data.toBool());
    }

    m_tool->setProgramDatabaseFile(m_progDatabFileLineEdit->text());
    m_tool->setStripPrivateSymbols(m_stripPrivSymbLineEdit->text());

    if (m_genMapFileComboBox->currentIndex() >= 0) {
        QVariant data = m_genMapFileComboBox->itemData(m_genMapFileComboBox->currentIndex(),
                                                       Qt::UserRole);
        m_tool->setGenerateMapFile(data.toBool());
    }

    m_tool->setMapFileName(m_mapFileNameLineEdit->text());

    if (m_mapExportsComboBox->currentIndex() >= 0) {
        QVariant data = m_mapExportsComboBox->itemData(m_mapExportsComboBox->currentIndex(),
                                                       Qt::UserRole);
        m_tool->setMapExports(data.toBool());
    }

    if (m_asmDebugComboBox->currentIndex() >= 0) {
        QVariant data = m_asmDebugComboBox->itemData(m_asmDebugComboBox->currentIndex(),
                                                     Qt::UserRole);
        m_tool->setAssemblyDebug(static_cast<AssemblyDebugEnum>(data.toInt()));
    }
}

void LinkerToolWidget::saveSystemData()
{
    if (m_subSystemComboBox->currentIndex() >= 0) {
        QVariant data = m_subSystemComboBox->itemData(m_subSystemComboBox->currentIndex(),
                                                      Qt::UserRole);
        m_tool->setSubSystem(static_cast<SubSystemEnum>(data.toInt()));
    }

    m_tool->setHeapReserveSize(m_heapResSizeLineEdit->text().toInt());
    m_tool->setHeapCommitSize(m_heapCommitSizeLineEdit->text().toInt());
    m_tool->setStackReserveSize(m_stackReserveSizeLineEdit->text().toInt());
    m_tool->setStackCommitSize(m_stackCommitSize->text().toInt());

    if (m_largeAddressAwareComboBox->currentIndex() >= 0) {
        QVariant data = m_largeAddressAwareComboBox->itemData(m_largeAddressAwareComboBox->currentIndex(),
                                                              Qt::UserRole);
        m_tool->setLargeAddressAware(static_cast<LargeAddressAwareEnum>(data.toInt()));
    }

    if (m_terminalServerAwareComboBox->currentIndex() >= 0) {
        QVariant data = m_terminalServerAwareComboBox->itemData(m_terminalServerAwareComboBox->currentIndex(),
                                                                Qt::UserRole);
        m_tool->setTerminalServerAware(static_cast<TerminalServerAwareEnum>(data.toInt()));
    }

    if (m_swapFromCDComboBox->currentIndex() >= 0) {
        QVariant data = m_swapFromCDComboBox->itemData(m_swapFromCDComboBox->currentIndex(),
                                                       Qt::UserRole);
        m_tool->setSwapRunFromCD(data.toBool());
    }

    if (m_swapFromNetComboBox->currentIndex() >= 0) {
        QVariant data = m_swapFromNetComboBox->itemData(m_swapFromNetComboBox->currentIndex(),
                                                        Qt::UserRole);
        m_tool->setSwapRunFromNet(data.toBool());
    }

    if (m_driverComboBox->currentIndex() >= 0) {
        QVariant data = m_driverComboBox->itemData(m_driverComboBox->currentIndex(),
                                                   Qt::UserRole);
        m_tool->setDriver(static_cast<DriverEnum>(data.toInt()));
    }
}

void LinkerToolWidget::saveOptimizationData()
{
    if (m_optmRefComboBox->currentIndex() >= 0) {
        QVariant data = m_optmRefComboBox->itemData(m_optmRefComboBox->currentIndex(),
                                                    Qt::UserRole);
        m_tool->setOptimizeReferences(static_cast<OptimizeReferencesEnum>(data.toInt()));
    }

    if (m_enableCOMDATFoldComboBox->currentIndex() >= 0) {
        QVariant data = m_enableCOMDATFoldComboBox->itemData(m_enableCOMDATFoldComboBox->currentIndex(),
                                                             Qt::UserRole);
        m_tool->setEnableCOMDATFolding(static_cast<EnableCOMDATFoldingEnum>(data.toInt()));
    }

    if (m_optmForWin98ComboBox->currentIndex() >= 0) {
        QVariant data = m_optmForWin98ComboBox->itemData(m_optmForWin98ComboBox->currentIndex(),
                                                         Qt::UserRole);
        m_tool->setOptimizeForWindows98(static_cast<OptimizeForWindows98Enum>(data.toInt()));
    }

    m_tool->setFunctionOrder(m_funcOrderLineEdit->text());
    m_tool->setProfileGuidedDatabase(m_profGuidedDatabLineEdit->text());

    if (m_linkTimeCodeGenComboBox->currentIndex() >= 0) {
        QVariant data = m_linkTimeCodeGenComboBox->itemData(m_linkTimeCodeGenComboBox->currentIndex(),
                                                            Qt::UserRole);
        m_tool->setLinkTimeCodeGeneration(static_cast<LinkTimeCodeGenerationEnum>(data.toInt()));
    }
}

void LinkerToolWidget::saveEnbeddedIDLData()
{
    m_tool->setMidlCommandFile(m_midlCommFileLineEdit->textToList(QLatin1String(";")));

    if (m_ignoreEmbedIDLComboBox->currentIndex() >= 0) {
        QVariant data = m_ignoreEmbedIDLComboBox->itemData(m_ignoreEmbedIDLComboBox->currentIndex(),
                                                           Qt::UserRole);
        m_tool->setIgnoreEmbeddedIDL(data.toBool());
    }

    m_tool->setMergedIDLBaseFileName(m_mergedIDLLineEdit->text());
    m_tool->setTypeLibraryFile(m_typeLibFileLineEdit->text());
    m_tool->setTypeLibraryResourceID(m_typeLibResIDLineEdit->text().toInt());
}

void LinkerToolWidget::saveAdvancedData()
{
    m_tool->setEntryPointSymbol(m_entryPointSym->text());

    if (m_resourceOnlyDLLComboBox->currentIndex() >= 0) {
        QVariant data = m_resourceOnlyDLLComboBox->itemData(m_resourceOnlyDLLComboBox->currentIndex(),
                                                            Qt::UserRole);
        m_tool->setResourceOnlyDLL(data.toBool());
    }

    if (m_setchecksumComboBox->currentIndex() >= 0) {
        QVariant data = m_setchecksumComboBox->itemData(m_setchecksumComboBox->currentIndex(),
                                                        Qt::UserRole);
        m_tool->setChecksum(data.toBool());
    }

    m_tool->setBaseAddress(m_baseAddressLineEdit->text());

    if (m_randBaseAddressComboBox->currentIndex() >= 0) {
        QVariant data = m_randBaseAddressComboBox->itemData(m_randBaseAddressComboBox->currentIndex(),
                                                            Qt::UserRole);
        m_tool->setRandomizedBaseAddress(static_cast<RandomizedBaseAddressEnum>(data.toInt()));
    }

    if (m_fixedBaseAddressComboBox->currentIndex() >= 0) {
        QVariant data = m_fixedBaseAddressComboBox->itemData(m_fixedBaseAddressComboBox->currentIndex(),
                                                             Qt::UserRole);
        m_tool->setFixedBaseAddress(static_cast<FixedBaseAddressEnum>(data.toInt()));
    }

    if (m_dataExecPrevComboBox->currentIndex() >= 0) {
        QVariant data = m_dataExecPrevComboBox->itemData(m_dataExecPrevComboBox->currentIndex(),
                                                         Qt::UserRole);
        m_tool->setDataExecutionPrevention(static_cast<DataExecutionPreventionEnum>(data.toInt()));
    }

    if (m_turnOffAsmGenComboBox->currentIndex() >= 0) {
        QVariant data = m_turnOffAsmGenComboBox->itemData(m_turnOffAsmGenComboBox->currentIndex(),
                                                          Qt::UserRole);
        m_tool->setTurnOffAssemblyGeneration(data.toBool());
    }

    if (m_suppUnloadOfDelayDLLComboBox->currentIndex() >= 0) {
        QVariant data = m_suppUnloadOfDelayDLLComboBox->itemData(m_suppUnloadOfDelayDLLComboBox->currentIndex(),
                                                                 Qt::UserRole);
        m_tool->setSupportUnloadOfDelayLoadedDLL(data.toBool());
    }

    m_tool->setImportLibrary(m_importLibLineEdit->text());
    m_tool->setMergeSections(m_mergeSectionsLineEdit->text());

    if (m_targetMachineComboBox->currentIndex() >= 0) {
        QVariant data = m_targetMachineComboBox->itemData(m_targetMachineComboBox->currentIndex(),
                                                          Qt::UserRole);
        m_tool->setTargetMachine(static_cast<TargetMachineEnum>(data.toInt()));
    }

    if (m_profileComboBox->currentIndex() >= 0) {
        QVariant data = m_profileComboBox->itemData(m_profileComboBox->currentIndex(),
                                                    Qt::UserRole);
        m_tool->setProfile(data.toBool());
    }

    if (m_clrThreadAttrComboBox->currentIndex() >= 0) {
        QVariant data = m_clrThreadAttrComboBox->itemData(m_clrThreadAttrComboBox->currentIndex(),
                                                          Qt::UserRole);
        m_tool->setCLRThreadAttribute(static_cast<CLRThreadAttributeEnum>(data.toInt()));
    }

    if (m_clrImageTypeComboBox->currentIndex() >= 0) {
        QVariant data = m_clrImageTypeComboBox->itemData(m_clrImageTypeComboBox->currentIndex(),
                                                         Qt::UserRole);
        m_tool->setCLRImageType(static_cast<CLRImageTypeEnum>(data.toInt()));
    }

    m_tool->setKeyFile(m_keyFileLineEdit->text());
    m_tool->setKeyContainer(m_keyContainerLineEdit->text());

    if (m_delaySignComboBox->currentIndex() >= 0) {
        QVariant data = m_delaySignComboBox->itemData(m_delaySignComboBox->currentIndex(),
                                                      Qt::UserRole);
        m_tool->setDelaySign(data.toBool());
    }

    if (m_errReportComboBox->currentIndex() >= 0) {
        QVariant data = m_errReportComboBox->itemData(m_errReportComboBox->currentIndex(),
                                                      Qt::UserRole);
        m_tool->setErrorReporting(static_cast<ErrorReportingLinkingEnum>(data.toInt()));
    }

    if (m_clrUnmanagedCodeCheckComboBox->currentIndex() >= 0) {
        QVariant data = m_clrUnmanagedCodeCheckComboBox->itemData(m_clrUnmanagedCodeCheckComboBox->currentIndex(),
                                                                  Qt::UserRole);
        m_tool->setCLRUnmanagedCodeCheck(data.toBool());
    }
}

} // namespace Internal
} // namespace VcProjectManager
