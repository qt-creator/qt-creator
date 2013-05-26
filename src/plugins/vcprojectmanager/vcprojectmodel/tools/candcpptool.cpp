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
#include "candcpptool.h"

#include <QLineEdit>
#include <QTabWidget>
#include <QComboBox>
#include <QVBoxLayout>

#include "../../widgets/basicconfigurationwidget.h"
#include "../../widgets/lineedit.h"

namespace VcProjectManager {
namespace Internal {

CAndCppTool::CAndCppTool()
{
}

CAndCppTool::CAndCppTool(const CAndCppTool &tool)
    : Tool(tool)
{
}

CAndCppTool::~CAndCppTool()
{
}

QString CAndCppTool::nodeWidgetName() const
{
    return QObject::tr("C/C++");
}

VcNodeWidget *CAndCppTool::createSettingsWidget()
{
    return new CAndCppToolWidget(this);
}

Tool::Ptr CAndCppTool::clone() const
{
    return Tool::Ptr(new CAndCppTool(*this));
}

QStringList CAndCppTool::additionalIncludeDirectories() const
{
    return readStringListAttribute(QLatin1String("AdditionalIncludeDirectories"), QLatin1String(";"));
}

QStringList CAndCppTool::additionalIncludeDirectoriesDefault() const
{
    return QStringList();
}

void CAndCppTool::setAdditionalIncludeDirectories(const QStringList &additIncDirs)
{
    setStringListAttribute(QLatin1String("AdditionalIncludeDirectories"), additIncDirs, QLatin1String(";"));
}

QStringList CAndCppTool::additionalUsingDirectories() const
{
    return readStringListAttribute(QLatin1String("AdditionalUsingDirectories"), QLatin1String(";"));
}

QStringList CAndCppTool::additionalUsingDirectoriesDefault() const
{
    return QStringList();
}

void CAndCppTool::setAdditionalUsingDirectories(const QStringList &dirs)
{
    setStringListAttribute(QLatin1String("AdditionalUsingDirectories"), dirs, QLatin1String(";"));
}

DebugInformationFormatEnum CAndCppTool::debugInformationFormat() const
{
    return readIntegerEnumAttribute<DebugInformationFormatEnum>(QLatin1String("DebugInformationFormat"),
                                                                DIF_Program_Database_For_Edit_And_Continue,
                                                                debugInformationFormatDefault());
}

DebugInformationFormatEnum CAndCppTool::debugInformationFormatDefault() const
{
    return DIF_Program_Database;
}

void CAndCppTool::setDebugInformationFormat(DebugInformationFormatEnum value)
{
    setIntegerEnumAttribute(QLatin1String("DebugInformationFormat"), value, debugInformationFormatDefault());
}

bool CAndCppTool::suppressStartupBanner() const
{
    return readBooleanAttribute(QLatin1String("SuppressStartupBanner"), suppressStartupBannerDefault());
}

bool CAndCppTool::suppressStartupBannerDefault() const
{
    return true;
}

void CAndCppTool::setSuppressStartupBanner(bool suppress)
{
    setBooleanAttribute(QLatin1String("SuppressStartupBanner"), suppress, suppressStartupBannerDefault());
}

WarningLevelEnum CAndCppTool::warningLevel() const
{
    return readIntegerEnumAttribute<WarningLevelEnum>(QLatin1String("WarningLevel"),
                                                      WL_Level_4,
                                                      warningLevelDefault());
}

WarningLevelEnum CAndCppTool::warningLevelDefault() const
{
    return WL_Level_3;
}

void CAndCppTool::setWarningLevel(WarningLevelEnum level)
{
    setIntegerEnumAttribute(QLatin1String("WarningLevel"), level, warningLevelDefault());
}

bool CAndCppTool::detect64BitPortabilityProblems() const
{
    return readBooleanAttribute(QLatin1String("Detect64BitPortabilityProblems"), detect64BitPortabilityProblemsDefault());
}

bool CAndCppTool::detect64BitPortabilityProblemsDefault() const
{
    return false;
}

void CAndCppTool::setDetect64BitPortabilityProblems(bool detect)
{
    setBooleanAttribute(QLatin1String("Detect64BitPortabilityProblems"), detect, detect64BitPortabilityProblemsDefault());
}

bool CAndCppTool::warnAsError() const
{
    return readBooleanAttribute(QLatin1String("WarnAsError"), warnAsErrorDefault());
}

bool CAndCppTool::warnAsErrorDefault() const
{
    return true;
}

void CAndCppTool::setWarnAsError(bool warn)
{
    setBooleanAttribute(QLatin1String("WarnAsError"), warn, warnAsErrorDefault());
}

bool CAndCppTool::useUnicodeResponseFiles() const
{
    return readBooleanAttribute(QLatin1String("UseUnicodeResponseFiles"), useUnicodeResponseFilesDefault());
}

bool CAndCppTool::useUnicodeResponseFilesDefault() const
{
    return true;
}

void CAndCppTool::setUseUnicodeResponseFiles(bool use)
{
    setBooleanAttribute(QLatin1String("UseUnicodeResponseFiles"), use, useUnicodeResponseFilesDefault());
}

OptimizationEnum CAndCppTool::optimization() const
{
    return readIntegerEnumAttribute<OptimizationEnum>(QLatin1String("Optimization"),
                                                      OP_Custom,
                                                      optimizationDefault());
}

OptimizationEnum CAndCppTool::optimizationDefault() const
{
    return OP_Disabled;
}

void CAndCppTool::setOptimization(OptimizationEnum optim)
{
    setIntegerEnumAttribute(QLatin1String("Optimization"), optim, optimizationDefault());
}

InlineFunctionExpansionEnum CAndCppTool::inlineFunctionExpansion() const
{
    return readIntegerEnumAttribute<InlineFunctionExpansionEnum>(QLatin1String("InlineFunctionExpansion"),
                                                                 IFE_Any_Suitable,
                                                                 inlineFunctionExpansionDefault());
}

InlineFunctionExpansionEnum CAndCppTool::inlineFunctionExpansionDefault() const
{
    return IFE_Default;
}

void CAndCppTool::setInlineFunctionExpansion(InlineFunctionExpansionEnum value)
{
    setIntegerEnumAttribute(QLatin1String("InlineFunctionExpansion"), value, inlineFunctionExpansionDefault());
}

bool CAndCppTool::enableIntrinsicFunctions() const
{
    return readBooleanAttribute(QLatin1String("EnableIntrinsicFunctions"), enableIntrinsicFunctionsDefault());
}

bool CAndCppTool::enableIntrinsicFunctionsDefault() const
{
    return false;
}

void CAndCppTool::setEnableIntrinsicFunctions(bool enable)
{
    setBooleanAttribute(QLatin1String("EnableIntrinsicFunctions"), enable, enableIntrinsicFunctionsDefault());
}

FavorSizeOrSpeedEnum CAndCppTool::favorSizeOrSpeed() const
{
    return readIntegerEnumAttribute<FavorSizeOrSpeedEnum>(QLatin1String("FavorSizeOrSpeed"),
                                                          FSS_Favor_Small_Code,
                                                          favorSizeOrSpeedDefault());
}

FavorSizeOrSpeedEnum CAndCppTool::favorSizeOrSpeedDefault() const
{
    return FSS_Neither;
}

void CAndCppTool::setFavorSizeOrSpeed(FavorSizeOrSpeedEnum value)
{
    setIntegerEnumAttribute(QLatin1String("FavorSizeOrSpeed"), value, favorSizeOrSpeedDefault());
}

bool CAndCppTool::omitFramePointers() const
{
    return readBooleanAttribute(QLatin1String("OmitFramePointers"), omitFramePointersDefault());
}

bool CAndCppTool::omitFramePointersDefault() const
{
    return false;
}

void CAndCppTool::setOmitFramePointers(bool omit)
{
    setBooleanAttribute(QLatin1String("OmitFramePointers"), omit, omitFramePointersDefault());
}

bool CAndCppTool::enableFiberSafeOptimizations() const
{
    return readBooleanAttribute(QLatin1String("EnableFiberSafeOptimizations"), enableFiberSafeOptimizationsDefault());
}

bool CAndCppTool::enableFiberSafeOptimizationsDefault() const
{
    return false;
}

void CAndCppTool::setEnableFiberSafeOptimizations(bool enable)
{
    setBooleanAttribute(QLatin1String("EnableFiberSafeOptimizations"), enable, enableFiberSafeOptimizationsDefault());
}

bool CAndCppTool::wholeProgramOptimization() const
{
    return readBooleanAttribute(QLatin1String("WholeProgramOptimization"), wholeProgramOptimizationDefault());
}

bool CAndCppTool::wholeProgramOptimizationDefault() const
{
    return false;
}

void CAndCppTool::setWholeProgramOptimization(bool optimize)
{
    setBooleanAttribute(QLatin1String("WholeProgramOptimization"), optimize, wholeProgramOptimizationDefault());
}

QStringList CAndCppTool::preprocessorDefinitions() const
{
    return readStringListAttribute(QLatin1String("PreprocessorDefinitions"), QLatin1String(";"));
}

QStringList CAndCppTool::preprocessorDefinitionsDefault() const
{
    return QStringList();
}

void CAndCppTool::setPreprocessorDefinitions(const QStringList &defs)
{
    setStringListAttribute(QLatin1String("PreprocessorDefinitions"), defs, QLatin1String(";"));
}

bool CAndCppTool::ignoreStandardIncludePath() const
{
    return readBooleanAttribute(QLatin1String("IgnoreStandardIncludePath"), ignoreStandardIncludePathDefault());
}

bool CAndCppTool::ignoreStandardIncludePathDefault() const
{
    return false;
}

void CAndCppTool::setIgnoreStandardIncludePath(bool ignore)
{
    setBooleanAttribute(QLatin1String("IgnoreStandardIncludePath"), ignore,ignoreStandardIncludePathDefault());
}

GeneratePreprocessedFileEnum CAndCppTool::generatePreprocessedFile() const
{
    return readIntegerEnumAttribute<GeneratePreprocessedFileEnum>(QLatin1String("GeneratePreprocessedFile"),
                                                                  GPF_Without_Line_Numbers,
                                                                  generatePreprocessedFileDefault()); // default
}

GeneratePreprocessedFileEnum CAndCppTool::generatePreprocessedFileDefault() const
{
    return GPF_Dont_Generate;
}

void CAndCppTool::setGeneratePreprocessedFile(GeneratePreprocessedFileEnum value)
{
    setIntegerEnumAttribute(QLatin1String("GeneratePreprocessedFile"), value, generatePreprocessedFileDefault());
}

bool CAndCppTool::keepComments() const
{
    return readBooleanAttribute(QLatin1String("KeepComments"), keepCommentsDefault());
}

bool CAndCppTool::keepCommentsDefault() const
{
    return false;
}

void CAndCppTool::setKeepComments(bool keep)
{
    setBooleanAttribute(QLatin1String("KeepComments"), keep, keepCommentsDefault());
}

bool CAndCppTool::stringPooling() const
{
    return readBooleanAttribute(QLatin1String("StringPooling"), stringPoolingDefault());
}

bool CAndCppTool::stringPoolingDefault() const
{
    return false;
}

void CAndCppTool::setStringPooling(bool pool)
{
    setBooleanAttribute(QLatin1String("StringPooling"), pool, stringPoolingDefault());
}

bool CAndCppTool::minimalRebuild() const
{
    return readBooleanAttribute(QLatin1String("MinimalRebuild"), minimalRebuildDefault());
}

bool CAndCppTool::minimalRebuildDefault() const
{
    return false;
}

void CAndCppTool::setMinimalRebuild(bool enable)
{
    setBooleanAttribute(QLatin1String("MinimalRebuild"), enable, minimalRebuildDefault());
}

ExceptionHandlingEnum CAndCppTool::exceptionHandling() const
{
    return readIntegerEnumAttribute<ExceptionHandlingEnum>(QLatin1String("ExceptionHandling"),
                                                           EH_Handle_Exception_With_SEH_Exceptions,
                                                           exceptionHandlingDefault()); // default
}

ExceptionHandlingEnum CAndCppTool::exceptionHandlingDefault() const
{
    return EH_Handle_Exception;
}

void CAndCppTool::setExceptionHandling(ExceptionHandlingEnum value)
{
    setIntegerEnumAttribute(QLatin1String("ExceptionHandling"), value, exceptionHandlingDefault());
}

bool CAndCppTool::smallerTypeCheck() const
{
    return readBooleanAttribute(QLatin1String("SmallerTypeCheck"), smallerTypeCheckDefault());
}

bool CAndCppTool::smallerTypeCheckDefault() const
{
    return false;
}

void CAndCppTool::setSmallerTypeCheck(bool check)
{
    setBooleanAttribute(QLatin1String("SmallerTypeCheck"), check, smallerTypeCheckDefault());
}

BasicRuntimeChecksEnum CAndCppTool::basicRuntimeChecks() const
{
    return readIntegerEnumAttribute<BasicRuntimeChecksEnum>(QLatin1String("BasicRuntimeChecks"),
                                                            BRC_Both,
                                                            basicRuntimeChecksDefault()); // default
}

BasicRuntimeChecksEnum CAndCppTool::basicRuntimeChecksDefault() const
{
    return BRC_Default;
}

void CAndCppTool::setBasicRuntimeChecks(BasicRuntimeChecksEnum value)
{
    setIntegerEnumAttribute(QLatin1String("BasicRuntimeChecks"), value, basicRuntimeChecksDefault());
}

RuntimeLibraryEnum CAndCppTool::runtimeLibrary() const
{
    return readIntegerEnumAttribute<RuntimeLibraryEnum>(QLatin1String("RuntimeLibrary"),
                                                        RL_Multi_threaded_Debug_DLL,
                                                        runtimeLibraryDefault()); // default
}

RuntimeLibraryEnum CAndCppTool::runtimeLibraryDefault() const
{
    return RL_Multi_threaded_Debug_DLL;
}

void CAndCppTool::setRuntimeLibrary(RuntimeLibraryEnum value)
{
    setIntegerEnumAttribute(QLatin1String("RuntimeLibrary"), value, runtimeLibraryDefault());
}

StructMemberAlignmentEnum CAndCppTool::structMemberAlignment() const
{
    return readIntegerEnumAttribute<StructMemberAlignmentEnum>(QLatin1String("StructMemberAlignment"),
                                                               SMA_Sixteen_Bytes,
                                                               structMemberAlignmentDefault()); // default
}

StructMemberAlignmentEnum CAndCppTool::structMemberAlignmentDefault() const
{
    return SMA_Default;
}

void CAndCppTool::setStructMemberAlignment(StructMemberAlignmentEnum value)
{
    setIntegerEnumAttribute(QLatin1String("StructMemberAlignment"), value, structMemberAlignmentDefault());
}

bool CAndCppTool::bufferSecurityCheck() const
{
    return readBooleanAttribute(QLatin1String("BufferSecurityCheck"), bufferSecurityCheckDefault());
}

bool CAndCppTool::bufferSecurityCheckDefault() const
{
    return true;
}

void CAndCppTool::setBufferSecurityCheck(bool check)
{
    setBooleanAttribute(QLatin1String("BufferSecurityCheck"), check, bufferSecurityCheckDefault());
}

bool CAndCppTool::enableFunctionLevelLinking() const
{
    return readBooleanAttribute(QLatin1String("EnableFunctionLevelLinking"), enableFunctionLevelLinkingDefault());
}

bool CAndCppTool::enableFunctionLevelLinkingDefault() const
{
    return false;
}

void CAndCppTool::setEnableFunctionLevelLinking(bool enable)
{
    setBooleanAttribute(QLatin1String("EnableFunctionLevelLinking"), enable, enableFunctionLevelLinkingDefault());
}

EnableEnhancedInstructionSetEnum CAndCppTool::enableEnhancedInstructionSet() const
{
    return readIntegerEnumAttribute<EnableEnhancedInstructionSetEnum>(QLatin1String("EnableEnhancedInstructionSet"),
                                                                      EEI_Streaming_SIMD_Extensions_2,
                                                                      enableEnhancedInstructionSetDefault()); // default
}

EnableEnhancedInstructionSetEnum CAndCppTool::enableEnhancedInstructionSetDefault() const
{
    return EEI_Not_Set;
}

void CAndCppTool::setEnableEnhancedInstructionSet(EnableEnhancedInstructionSetEnum value)
{
    setIntegerEnumAttribute(QLatin1String("EnableEnhancedInstructionSet"), value, enableEnhancedInstructionSetDefault());
}

FloatingPointModelEnum CAndCppTool::floatingPointModel() const
{
    return readIntegerEnumAttribute<FloatingPointModelEnum>(QLatin1String("FloatingPointModel"),
                                                            FPM_Fast,
                                                            floatingPointModelDefault()); // default
}

FloatingPointModelEnum CAndCppTool::floatingPointModelDefault() const
{
    return FPM_Precise;
}

void CAndCppTool::setFloatingPointModel(FloatingPointModelEnum value)
{
    setIntegerEnumAttribute(QLatin1String("FloatingPointModel"), value, floatingPointModelDefault());
}

bool CAndCppTool::floatingPointExceptions() const
{
    return readBooleanAttribute(QLatin1String("FloatingPointExceptions"), floatingPointExceptionsDefault());
}

bool CAndCppTool::floatingPointExceptionsDefault() const
{
    return false;
}

void CAndCppTool::setFloatingPointExceptions(bool enable)
{
    setBooleanAttribute(QLatin1String("FloatingPointExceptions"), enable, floatingPointExceptionsDefault());
}

bool CAndCppTool::disableLanguageExtensions() const
{
    return readBooleanAttribute(QLatin1String("DisableLanguageExtensions"), disableLanguageExtensionsDefault());
}

bool CAndCppTool::disableLanguageExtensionsDefault() const
{
    return false;
}

void CAndCppTool::setDisableLanguageExtensions(bool disable)
{
    setBooleanAttribute(QLatin1String("DisableLanguageExtensions"), disable, disableLanguageExtensionsDefault());
}

bool CAndCppTool::defaultCharIsUnsigned() const
{
    return readBooleanAttribute(QLatin1String("DefaultCharIsUnsigned"), defaultCharIsUnsignedDefault());
}

bool CAndCppTool::defaultCharIsUnsignedDefault() const
{
    return false;
}

void CAndCppTool::setDefaultCharIsUnsigned(bool isUnsigned)
{
    setBooleanAttribute(QLatin1String("DefaultCharIsUnsigned"), isUnsigned, defaultCharIsUnsignedDefault());
}

bool CAndCppTool::treatWChartAsBuiltInType() const
{
    return readBooleanAttribute(QLatin1String("TreatWChar_tAsBuiltInType"), treatWChartAsBuiltInTypeDefault());
}

bool CAndCppTool::treatWChartAsBuiltInTypeDefault() const
{
    return true;
}

void CAndCppTool::setTreatWChartAsBuiltInType(bool enable)
{
    setBooleanAttribute(QLatin1String("TreatWChar_tAsBuiltInType"), enable, treatWChartAsBuiltInTypeDefault());
}

bool CAndCppTool::forceConformanceInForLoopScope() const
{
    return readBooleanAttribute(QLatin1String("ForceConformanceInForLoopScope"), forceConformanceInForLoopScopeDefault());
}

bool CAndCppTool::forceConformanceInForLoopScopeDefault() const
{
    return true;
}

void CAndCppTool::setForceConformanceInForLoopScope(bool force)
{
    setBooleanAttribute(QLatin1String("ForceConformanceInForLoopScope"), force, forceConformanceInForLoopScopeDefault());
}

bool CAndCppTool::runtimeTypeInfo() const
{
    return readBooleanAttribute(QLatin1String("RuntimeTypeInfo"), runtimeTypeInfoDefault());
}

bool CAndCppTool::runtimeTypeInfoDefault() const
{
    return true;
}

void CAndCppTool::setRuntimeTypeInfo(bool enable)
{
    setBooleanAttribute(QLatin1String("RuntimeTypeInfo"), enable, runtimeTypeInfoDefault());
}

bool CAndCppTool::openMP() const
{
    return readBooleanAttribute(QLatin1String("OpenMP"), openMPDefault());
}

bool CAndCppTool::openMPDefault() const
{
    return false;
}

void CAndCppTool::setOpenMP(bool enable)
{
    setBooleanAttribute(QLatin1String("OpenMP"), enable, openMPDefault());
}

UsePrecompiledHeaderEnum CAndCppTool::usePrecompiledHeader() const
{
    return readIntegerEnumAttribute<UsePrecompiledHeaderEnum>(QLatin1String("UsePrecompiledHeader"),
                                                              UPH_Use_Precompiled_Header,
                                                              usePrecompiledHeaderDefault()); // default
}

UsePrecompiledHeaderEnum CAndCppTool::usePrecompiledHeaderDefault() const
{
    return UPH_Not_Using_Precompiled_Headers;
}

void CAndCppTool::setUsePrecompiledHeader(UsePrecompiledHeaderEnum value)
{
    setIntegerEnumAttribute(QLatin1String("UsePrecompiledHeader"), value, usePrecompiledHeaderDefault());
}

QString CAndCppTool::precompiledHeaderThrough() const
{
    return readStringAttribute(QLatin1String("PrecompiledHeaderThrough"), precompiledHeaderThroughDefault());
}

QString CAndCppTool::precompiledHeaderThroughDefault() const
{
    return QString();
}

void CAndCppTool::setPrecompiledHeaderThrough(const QString &headerFile)
{
    setStringAttribute(QLatin1String("PrecompiledHeaderThrough"), headerFile, precompiledHeaderThroughDefault());
}

QString CAndCppTool::precompiledHeaderFile() const
{
    return readStringAttribute(QLatin1String("PrecompiledHeaderFile"), precompiledHeaderFileDefault());
}

QString CAndCppTool::precompiledHeaderFileDefault() const
{
    return QString();
}

void CAndCppTool::setPrecompiledHeaderFile(const QString &headerFile)
{
    setStringAttribute(QLatin1String("PrecompiledHeaderFile"), headerFile, precompiledHeaderFileDefault());
}

bool CAndCppTool::expandAttributedSource() const
{
    return readBooleanAttribute(QLatin1String("ExpandAttributedSource"), expandAttributedSourceDefault());
}

bool CAndCppTool::expandAttributedSourceDefault() const
{
    return false;
}

void CAndCppTool::setExpandAttributedSource(bool expand)
{
    setBooleanAttribute(QLatin1String("ExpandAttributedSource"), expand, expandAttributedSourceDefault());
}

AssemblerOutputEnum CAndCppTool::assemblerOutput() const
{
    return readIntegerEnumAttribute<AssemblerOutputEnum>(QLatin1String("AssemblerOutput"),
                                                         AO_Assembly_With_Source_Code,
                                                         assemblerOutputDefault()); // default
}

AssemblerOutputEnum CAndCppTool::assemblerOutputDefault() const
{
    return AO_No_Listing;
}

void CAndCppTool::setAssemblerOutput(AssemblerOutputEnum value)
{
    setIntegerEnumAttribute(QLatin1String("AssemblerOutput"), value, assemblerOutputDefault());
}

QString CAndCppTool::assemblerListingLocation() const
{
    return readStringAttribute(QLatin1String("AssemblerListingLocation") , assemblerListingLocationDefault());
}

QString CAndCppTool::assemblerListingLocationDefault() const
{
    return QLatin1String("$(IntDir)\\");
}

void CAndCppTool::setAssemblerListingLocation(const QString &location)
{
    setStringAttribute(QLatin1String("AssemblerListingLocation"), location, assemblerListingLocationDefault());
}

QString CAndCppTool::objectFile() const
{
    return readStringAttribute(QLatin1String("ObjectFile"), objectFileDefault());
}

QString CAndCppTool::objectFileDefault() const
{
    return QLatin1String("$(IntDir)\\");
}

void CAndCppTool::setObjectFile(const QString &file)
{
    setStringAttribute(QLatin1String("ObjectFile"), file, objectFileDefault());
}

QString CAndCppTool::programDataBaseFileName() const
{
    return readStringAttribute(QLatin1String("ProgramDataBaseFileName"), programDataBaseFileNameDefault());
}

QString CAndCppTool::programDataBaseFileNameDefault() const
{
    return QLatin1String("$(IntDir)\\vc.pdb");
}

void CAndCppTool::setProgramDataBaseFileName(const QString &fileName)
{
    setStringAttribute(QLatin1String("ProgramDataBaseFileName"), fileName, programDataBaseFileNameDefault());
}

bool CAndCppTool::generateXMLDocumentationFiles() const
{
    return readBooleanAttribute(QLatin1String("GenerateXMLDocumentationFiles"), generateXMLDocumentationFilesDefault());
}

bool CAndCppTool::generateXMLDocumentationFilesDefault() const
{
    return false;
}

void CAndCppTool::setGenerateXMLDocumentationFiles(bool generate)
{
    setBooleanAttribute(QLatin1String("GenerateXMLDocumentationFiles"), generate, generateXMLDocumentationFilesDefault());
}

QString CAndCppTool::xmlDocumentationFileName() const
{
    return readStringAttribute(QLatin1String("XMLDocumentationFileName"), xmlDocumentationFileNameDefault());
}

QString CAndCppTool::xmlDocumentationFileNameDefault() const
{
    return QLatin1String("$(IntDir)\\");
}

void CAndCppTool::setXMLDocumentationFileName(const QString fileName)
{
    setStringAttribute(QLatin1String("XMLDocumentationFileName"), fileName, xmlDocumentationFileNameDefault());
}

BrowseInformationEnum CAndCppTool::browseInformation() const
{
    return readIntegerEnumAttribute<BrowseInformationEnum>(QLatin1String("BrowseInformation"),
                                                           BI_No_Local_Symbols,
                                                           browseInformationDefault()); // default
}

BrowseInformationEnum CAndCppTool::browseInformationDefault() const
{
    return BI_None;
}

void CAndCppTool::setBrowseInformation(BrowseInformationEnum value)
{
    setIntegerEnumAttribute(QLatin1String("BrowseInformation"), value, browseInformationDefault());
}

QString CAndCppTool::browseInformationFile() const
{
    return readStringAttribute(QLatin1String("BrowseInformationFile"), browseInformationFileDefault());
}

QString CAndCppTool::browseInformationFileDefault() const
{
    return QLatin1String("$(IntDir)\\");
}

void CAndCppTool::setBrowseInformationFile(const QString &file)
{
    setStringAttribute(QLatin1String("BrowseInformationFile"), file, browseInformationFileDefault());
}

CallingConventionEnum CAndCppTool::callingConvention() const
{
    return readIntegerEnumAttribute<CallingConventionEnum>(QLatin1String("CallingConvention"),
                                                           CC_Stdcall,
                                                           callingConventionDefault()); // default
}

CallingConventionEnum CAndCppTool::callingConventionDefault() const
{
    return CC_Cdecl;
}

void CAndCppTool::setCallingConvention(CallingConventionEnum value)
{
    setIntegerEnumAttribute(QLatin1String("CallingConvention"), value, callingConventionDefault());
}

CompileAsEnum CAndCppTool::compileAs() const
{
    return readIntegerEnumAttribute<CompileAsEnum>(QLatin1String("CompileAs"),
                                                   CA_Compile_as_Cpp_Code,
                                                   compileAsDefault()); // default
}

CompileAsEnum CAndCppTool::compileAsDefault() const
{
    return CA_Compile_as_Cpp_Code;
}

void CAndCppTool::setCompileAs(CompileAsEnum value)
{
    setIntegerEnumAttribute(QLatin1String("CompileAs"), value, compileAsDefault());
}

QStringList CAndCppTool::disableSpecificWarnings() const
{
    return readStringListAttribute(QLatin1String("DisableSpecificWarnings"), QLatin1String(";"));
}

QStringList CAndCppTool::disableSpecificWarningsDefault() const
{
    return QStringList();
}

void CAndCppTool::setDisableSpecificWarnings(const QStringList &warnings)
{
    setStringListAttribute(QLatin1String("DisableSpecificWarnings"), warnings, QLatin1String(";"));
}

QStringList CAndCppTool::forcedIncludeFiles() const
{
    return readStringListAttribute(QLatin1String("ForcedIncludeFiles"), QLatin1String(";"));
}

QStringList CAndCppTool::forcedIncludeFilesDefault() const
{
    return QStringList();
}

void CAndCppTool::setForcedIncludeFiles(const QStringList &files)
{
    setStringListAttribute(QLatin1String("ForcedIncludeFiles"), files, QLatin1String(";"));
}

QStringList CAndCppTool::forcedUsingFiles() const
{
    return readStringListAttribute(QLatin1String("ForcedUsingFiles"), QLatin1String(";"));
}

QStringList CAndCppTool::forcedUsingFilesDefault() const
{
    return QStringList();
}

void CAndCppTool::setForcedUsingFiles(const QStringList &files)
{
    setStringListAttribute(QLatin1String("ForcedUsingFiles"), files, QLatin1String(";"));
}

bool CAndCppTool::showIncludes() const
{
    return readBooleanAttribute(QLatin1String("ShowIncludes"), showIncludesDefault());
}

bool CAndCppTool::showIncludesDefault() const
{
    return false;
}

void CAndCppTool::setShowIncludes(bool show)
{
    setBooleanAttribute(QLatin1String("ShowIncludes"), show, showIncludesDefault());
}

QStringList CAndCppTool::undefinePreprocessorDefinitions() const
{
    return readStringListAttribute(QLatin1String("UndefinePreprocessorDefinitions"), QLatin1String(";"));
}

QStringList CAndCppTool::undefinePreprocessorDefinitionsDefault() const
{
    return QStringList();
}

void CAndCppTool::setUndefinePreprocessorDefinitions(const QStringList &definitions)
{
    setStringListAttribute(QLatin1String("UndefinePreprocessorDefinitions"), definitions, QLatin1String(";"));
}

bool CAndCppTool::undefineAllPreprocessorDefinitions() const
{
    return readBooleanAttribute(QLatin1String("UndefineAllPreprocessorDefinitions"), undefineAllPreprocessorDefinitionsDefault());
}

bool CAndCppTool::undefineAllPreprocessorDefinitionsDefault() const
{
    return false;
}

void CAndCppTool::setUndefineAllPreprocessorDefinitions(bool undefine)
{
    setBooleanAttribute(QLatin1String("UndefineAllPreprocessorDefinitions"), undefine, undefineAllPreprocessorDefinitionsDefault());
}

bool CAndCppTool::useFullPaths() const
{
    return readBooleanAttribute(QLatin1String("UseFullPaths"), useFullPathsDefault());
}

bool CAndCppTool::useFullPathsDefault() const
{
    return false;
}

void CAndCppTool::setUseFullPaths(bool use)
{
    setBooleanAttribute(QLatin1String("UseFullPaths"), use, useFullPathsDefault());
}

bool CAndCppTool::omitDefaultLibName() const
{
    return readBooleanAttribute(QLatin1String("OmitDefaultLibName"), omitDefaultLibNameDefault());
}

bool CAndCppTool::omitDefaultLibNameDefault() const
{
    return false;
}

void CAndCppTool::setOmitDefaultLibName(bool omit)
{
    setBooleanAttribute(QLatin1String("OmitDefaultLibName"), omit, omitDefaultLibNameDefault());
}

ErrorReportingCCppEnum CAndCppTool::errorReporting() const
{
    return readIntegerEnumAttribute<ErrorReportingCCppEnum>(QLatin1String("ErrorReporting"),
                                                            ERCC_Queue_For_Next_Login,
                                                            errorReportingDefault()); // default
}

ErrorReportingCCppEnum CAndCppTool::errorReportingDefault() const
{
    return ERCC_Prompt_Immediately;
}

void CAndCppTool::setErrorReporting(ErrorReportingCCppEnum value)
{
    setIntegerEnumAttribute(QLatin1String("ErrorReporting"), value, errorReportingDefault());
}


CAndCppToolWidget::CAndCppToolWidget(CAndCppTool *tool)
    : m_tool(tool)
{
    QTabWidget *mainTabWidget = new QTabWidget(this);
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->addWidget(mainTabWidget);
    setLayout(layout);

    mainTabWidget->addTab(createGeneralWidget(), tr("General"));
    mainTabWidget->addTab(createOptimizationWidget(), tr("Optimization"));
    mainTabWidget->addTab(createPreprocessorWidget(), tr("Preprocessor"));
    mainTabWidget->addTab(createCodeGenerationWidget(), tr("Code Generation"));
    mainTabWidget->addTab(createLanguageWidget(), tr("Language"));
    mainTabWidget->addTab(createPrecompiledHeadersWidget(), tr("Precompiled Headers"));
    mainTabWidget->addTab(createOutputFilesWidget(), tr("Output Files"));
    mainTabWidget->addTab(createBrowseInformationWidget(), tr("Browse Information"));
    mainTabWidget->addTab(createAdvancedWidget(), tr("Advanced"));
}

CAndCppToolWidget::~CAndCppToolWidget()
{
}

void CAndCppToolWidget::saveData()
{
    saveGeneralData();
    saveOptimizationData();
    savePreprocessorData();
    saveCodeGenerationData();
    saveLanguageData();
    savePrecompiledHeadersData();
    saveOutputFilesData();
    saveBrowseInformationData();
    saveAdvancedData();
}

QWidget *CAndCppToolWidget::createGeneralWidget()
{
    BasicConfigurationWidget *basicWidget = new BasicConfigurationWidget;

    m_additIncDirsLineEdit = new LineEdit;
    m_additIncDirsLineEdit->setTextList(m_tool->additionalIncludeDirectories(), QLatin1String(";"));
    basicWidget->insertTableRow(tr("Additional Include Directories"),
                                m_additIncDirsLineEdit,
                                tr("Specifies one or more directories to add to the include path; use semi-colon delimited list if more than one. (/I[path])"));

    m_resolveRefsLineEdit = new LineEdit;
    m_resolveRefsLineEdit->setTextList(m_tool->additionalUsingDirectories(), QLatin1String(";"));
    basicWidget->insertTableRow(tr("Resolve #using References"),
                                m_resolveRefsLineEdit,
                                tr("Specifies one or more directories (separate directory names with a semicolon) to be searched to resolve names passed to a #using directive. (/AI[path])"));

    m_dbgInfoFormatComboBox = new QComboBox;
    m_dbgInfoFormatComboBox->addItem(tr("Disabled"), QVariant(0));
    m_dbgInfoFormatComboBox->addItem(tr("C7 Compatible (/Z7)"), QVariant(1));
    m_dbgInfoFormatComboBox->addItem(tr("Program Database (/Zi)"), QVariant(3));
    m_dbgInfoFormatComboBox->addItem(tr("Program Database for Edit & Continue (/ZI)"), QVariant(4));

    if (m_tool->debugInformationFormat() == 0)
        m_dbgInfoFormatComboBox->setCurrentIndex(0);
    if (m_tool->debugInformationFormat() == 1)
        m_dbgInfoFormatComboBox->setCurrentIndex(1);
    if (m_tool->debugInformationFormat() == 3)
        m_dbgInfoFormatComboBox->setCurrentIndex(2);
    if (m_tool->debugInformationFormat() == 4)
        m_dbgInfoFormatComboBox->setCurrentIndex(3);

    basicWidget->insertTableRow(tr("Debug Information Format"),
                                m_dbgInfoFormatComboBox,
                                tr("Specifies the type of debugging information generated by the compiler. You must also change linker settings appropriately to match. (/Z7, Zd, /Zi, /ZI)"));

    m_suppresStBannerComboBox = new QComboBox;
    m_suppresStBannerComboBox->addItem(tr("No"), QVariant(false));
    m_suppresStBannerComboBox->addItem(tr("Yes (/nologo)"), QVariant(true));

    if (m_tool->suppressStartupBanner())
        m_suppresStBannerComboBox->setCurrentIndex(1);
    else
        m_suppresStBannerComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Suppress Startup Banner"),
                                m_suppresStBannerComboBox,
                                tr("Suppress the display of the startup banner and information messages. (/nologo)"));

    m_warningLevelComboBox = new QComboBox;
    m_warningLevelComboBox->addItem(tr("Off: Turn Off All Warnings (/W0)"),
                                    QVariant(0));
    m_warningLevelComboBox->addItem(tr("Level 1 (/W1)"), QVariant(1));
    m_warningLevelComboBox->addItem(tr("Level 2 (/W2)"), QVariant(2));
    m_warningLevelComboBox->addItem(tr("Level 3 (/W3)"), QVariant(3));
    m_warningLevelComboBox->addItem(tr("Level 4 (/W4)"), QVariant(4));

    m_warningLevelComboBox->setCurrentIndex(m_tool->warningLevel());

    basicWidget->insertTableRow(tr("Warning Level"),
                                m_warningLevelComboBox,
                                tr("Select how strict you want the compiler to be about checking for potentially suspect constructs. (/W0 - /W4)"));

    m_detect64BitPortIssuesComboBox = new QComboBox;
    m_detect64BitPortIssuesComboBox->addItem(tr("No"), QVariant(false));
    m_detect64BitPortIssuesComboBox->addItem(tr("Yes (/Wp64)"), QVariant(true));

    if (m_tool->detect64BitPortabilityProblems())
        m_detect64BitPortIssuesComboBox->setCurrentIndex(1);
    else
        m_detect64BitPortIssuesComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Detect 64Bit Portability Problems"),
                                m_detect64BitPortIssuesComboBox,
                                tr("Tells the compiler to check for 64-bit portability issues. (/Wp64)"));

    m_threatWarnAsErrComboBox = new QComboBox;
    m_threatWarnAsErrComboBox->addItem(tr("No"), QVariant(false));
    m_threatWarnAsErrComboBox->addItem(tr("Yes (/WX)"), QVariant(true));

    if (m_tool->warnAsError())
        m_threatWarnAsErrComboBox->setCurrentIndex(1);
    else
        m_threatWarnAsErrComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Threat Warnings as Errors"),
                                m_threatWarnAsErrComboBox,
                                tr("Enables the compiler to treat all warnings as errors. (/WX)"));

    m_useUnicodeRespFilesComboBox = new QComboBox;
    m_useUnicodeRespFilesComboBox->addItem(tr("No"), QVariant(false));
    m_useUnicodeRespFilesComboBox->addItem(tr("Yes"), QVariant(true));

    if (m_tool->useUnicodeResponseFiles())
        m_useUnicodeRespFilesComboBox->setCurrentIndex(1);
    else
        m_useUnicodeRespFilesComboBox->setCurrentIndex(0);

    return basicWidget;
}

QWidget *CAndCppToolWidget::createOptimizationWidget()
{
    BasicConfigurationWidget *basicWidget = new BasicConfigurationWidget;

    m_optimizationComboBox = new QComboBox;
    m_optimizationComboBox->addItem(tr("Disabled (/Od)"), QVariant(0));
    m_optimizationComboBox->addItem(tr("Minimize Size (/O1)"), QVariant(1));
    m_optimizationComboBox->addItem(tr("Maximize Speed (/O2)"), QVariant(2));
    m_optimizationComboBox->addItem(tr("Full Optimization (/Ox)"), QVariant(3));
    m_optimizationComboBox->addItem(tr("Custom"), QVariant(4));

    m_optimizationComboBox->setCurrentIndex(m_tool->optimization());

    basicWidget->insertTableRow(tr("Optimization"),
                                m_optimizationComboBox,
                                tr("Select option for code optimization; choose Custom to use specific optimization options. (/Od, /O1, /O2 /O3, /Ox)"));

    m_inlineFuncExpComboBox = new QComboBox;
    m_inlineFuncExpComboBox->addItem(tr("Default"), QVariant(0));
    m_inlineFuncExpComboBox->addItem(tr("Only __inline (/Ob1)"), QVariant(1));
    m_inlineFuncExpComboBox->addItem(tr("Any Suitable (/Ob2)"), QVariant(2));

    m_inlineFuncExpComboBox->setCurrentIndex(m_tool->inlineFunctionExpansion());

    basicWidget->insertTableRow(tr("Inline Function Expansion"),
                                m_inlineFuncExpComboBox,
                                tr("Select the level of inline function expansion for the build. (/Ob1, /Ob2)"));

    m_enableInstrFuncComboBox = new QComboBox;
    m_enableInstrFuncComboBox->addItem(tr("No"), QVariant(false));
    m_enableInstrFuncComboBox->addItem(tr("Yes"), QVariant(true));

    if (m_tool->enableIntrinsicFunctions())
        m_enableInstrFuncComboBox->setCurrentIndex(1);
    else
        m_enableInstrFuncComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Enable Intrinsic Functions"),
                                m_enableInstrFuncComboBox,
                                tr("Enables instrinsic functions. Using instrinsic functions generates faster, but possibly larger, code. (/Oi)"));

    m_favorSizeorSpeedComboBox = new QComboBox;
    m_favorSizeorSpeedComboBox->addItem(tr("Neither"), QVariant(0));
    m_favorSizeorSpeedComboBox->addItem(tr("Favor Fast Code (/Ot)"), QVariant(1));
    m_favorSizeorSpeedComboBox->addItem(tr("Favor Small Code (/Os)"), QVariant(2));

    m_favorSizeorSpeedComboBox->setCurrentIndex(m_tool->favorSizeOrSpeed());

    basicWidget->insertTableRow(tr("Favor Size Or Speed"),
                                m_favorSizeorSpeedComboBox,
                                tr("Choose whether to favor code size or code speed; 'Global Optimization' must be turned on. (/Ot, /Os)"));

    m_omitFramePtrComboBox = new QComboBox;
    m_omitFramePtrComboBox->addItem(tr("No"), QVariant(false));
    m_omitFramePtrComboBox->addItem(tr("Yes (/Oy)"), QVariant(true));

    if (m_tool->omitFramePointers())
        m_omitFramePtrComboBox->setCurrentIndex(1);
    else
        m_omitFramePtrComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Omit Frame Pointers"),
                                m_omitFramePtrComboBox,
                                tr("Suppresses frame pointers. (/Oy)"));

    m_enableFiberSafeOptmComboBox = new QComboBox;
    m_enableFiberSafeOptmComboBox->addItem(tr("No"), QVariant(false));
    m_enableFiberSafeOptmComboBox->addItem(tr("Yes (/GT)"), QVariant(true));

    if (m_tool->enableFiberSafeOptimizations())
        m_enableFiberSafeOptmComboBox->setCurrentIndex(1);
    else
        m_enableFiberSafeOptmComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Enable Fiber Safe Optimizations"),
                                m_enableFiberSafeOptmComboBox,
                                tr("Enables memory space optimization when using fibers and thread local storage access. (/GT)"));

    m_wholeProgOptmComboBox = new QComboBox;
    m_wholeProgOptmComboBox->addItem(tr("No"), QVariant(false));
    m_wholeProgOptmComboBox->addItem(tr("Enable link-time code generation (/GL)"), QVariant(true));

    if (m_tool->wholeProgramOptimization())
        m_wholeProgOptmComboBox->setCurrentIndex(1);
    else
        m_wholeProgOptmComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Whole Program Optimization"),
                                m_wholeProgOptmComboBox,
                                tr("Enables cross-module optimizations by delaying code generation to link time; requires that linker option 'Link Time Code  Generation' be turned on. (/GL)"));

    return basicWidget;
}

QWidget *CAndCppToolWidget::createPreprocessorWidget()
{
    BasicConfigurationWidget *basicWidget = new BasicConfigurationWidget;

    m_preProcDefLineEdit = new LineEdit;
    m_preProcDefLineEdit->setTextList(m_tool->preprocessorDefinitions(), QLatin1String(";"));
    basicWidget->insertTableRow(tr("Preprocessor Definitions"),
                                m_preProcDefLineEdit,
                                tr("Specifies one or more preprocessor defines."));

    m_ignoreStdIncPathComboBox = new QComboBox;
    m_ignoreStdIncPathComboBox->addItem(tr("No"), QVariant(false));
    m_ignoreStdIncPathComboBox->addItem(tr("Yes (/X)"), QVariant(true));

    if (m_tool->ignoreStandardIncludePath())
        m_ignoreStdIncPathComboBox->setCurrentIndex(1);
    else
        m_ignoreStdIncPathComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Ignore Standard Include Path"),
                                m_ignoreStdIncPathComboBox,
                                tr("Ignore standard include path. (/X)"));

    m_generPreProcFileComboBox = new QComboBox;
    m_generPreProcFileComboBox->addItem(tr("No"), QVariant(0));
    m_generPreProcFileComboBox->addItem(tr("With Line Numbers (/P)"), QVariant(1));
    m_generPreProcFileComboBox->addItem(tr("Without Line Numbers (/EP /P)"), QVariant(2));

    m_generPreProcFileComboBox->setCurrentIndex(m_tool->generatePreprocessedFile());

    basicWidget->insertTableRow(tr("Generate Preprocessed File"),
                                m_generPreProcFileComboBox,
                                tr("Specifies the preprocessor option for this configurations. (/P, /EP /P)"));

    m_keepCommentsComboBox = new QComboBox;
    m_keepCommentsComboBox->addItem(tr("No"), QVariant(false));
    m_keepCommentsComboBox->addItem(tr("Yes"), QVariant(true));

    if (m_tool->keepComments())
        m_keepCommentsComboBox->setCurrentIndex(1);
    else
        m_keepCommentsComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Keep Comments"),
                                m_keepCommentsComboBox,
                                tr("Suppresses comment strip from source code; requires that one of the 'Preprocessing' options be set. (/C)"));

    return basicWidget;
}

QWidget *CAndCppToolWidget::createCodeGenerationWidget()
{
    BasicConfigurationWidget *basicWidget = new BasicConfigurationWidget;

    m_stringPoolingComboBox = new QComboBox;
    m_stringPoolingComboBox->addItem(tr("No"), QVariant(false));
    m_stringPoolingComboBox->addItem(tr("Yes (/GF)"), QVariant(true));

    if (m_tool->stringPooling())
        m_stringPoolingComboBox->setCurrentIndex(1);
    else
        m_stringPoolingComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Enable String Pool"),
                                m_stringPoolingComboBox,
                                tr("Enables read-only string pooling for generating smaller compiled code. (/GF)"));

    m_minimalRebuildComboBox = new QComboBox;
    m_minimalRebuildComboBox->addItem(tr("No"), QVariant(false));
    m_minimalRebuildComboBox->addItem(tr("Yes"), QVariant(true));

    if (m_tool->minimalRebuild())
        m_minimalRebuildComboBox->setCurrentIndex(1);
    else
        m_minimalRebuildComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Enable Minimal Rebuild"),
                                m_minimalRebuildComboBox,
                                tr("Detect changes to C++ class definitions and recompile only affected source files. (/Gm)"));

    m_exceptionHandlComboBox = new QComboBox;
    m_exceptionHandlComboBox->addItem(tr("No"), QVariant(0));
    m_exceptionHandlComboBox->addItem(tr("Yes (/EHsc)"), QVariant(1));
    m_exceptionHandlComboBox->addItem(tr("Yes With SEH Exceptions (/EHa)"), QVariant(2));

    m_exceptionHandlComboBox->setCurrentIndex(m_tool->exceptionHandling());

    basicWidget->insertTableRow(tr("Enable C++ Exceptions"),
                                m_exceptionHandlComboBox,
                                tr("Calls destructirs for automatic objects during a stack unwind caused by an exception being thrown. (/EHsc, /EHa)"));


    m_smallerTypeCheckComboBox = new QComboBox;
    m_smallerTypeCheckComboBox->addItem(tr("No"), QVariant(false));
    m_smallerTypeCheckComboBox->addItem(tr("Yes (/RTCc)"), QVariant(true));

    if (m_tool->smallerTypeCheck())
        m_smallerTypeCheckComboBox->setCurrentIndex(1);
    else
        m_smallerTypeCheckComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Smaller Type Check"),
                                m_smallerTypeCheckComboBox,
                                tr("Enables checking for conversion to smaller types, incompatible with any optimization type other than debug. (/RTCc)"));

    m_basicRuntimeChecksComboBox = new QComboBox;
    m_basicRuntimeChecksComboBox->addItem(tr("Default"), QVariant(0));
    m_basicRuntimeChecksComboBox->addItem(tr("Stack Frames (/RTCs)"), QVariant(1));
    m_basicRuntimeChecksComboBox->addItem(tr("Uninitialized Variables (/RTCu)"), QVariant(2));
    m_basicRuntimeChecksComboBox->addItem(tr("Both (/RTC1, equiv. to /RTCsu)"), QVariant(3));

    m_basicRuntimeChecksComboBox->setCurrentIndex(m_tool->basicRuntimeChecks());

    basicWidget->insertTableRow(tr("Basic Runtime Checks"),
                                m_basicRuntimeChecksComboBox,
                                tr("Perform basic runtime error checks, incompatible with any optimization type other than debug. (/RTCs. /RTCu, /RTC1)"));

    m_runtimeLibComboBox = new QComboBox;
    m_runtimeLibComboBox->addItem(tr("Multi-threaded (/MT)"), QVariant(0));
    m_runtimeLibComboBox->addItem(tr("Multi-threaded Debug (/MTd)"), QVariant(1));
    m_runtimeLibComboBox->addItem(tr("Multi-threaded DLL (/MD)"), QVariant(2));
    m_runtimeLibComboBox->addItem(tr("Multi-threaded Debug DLL (/Mdd)"), QVariant(3));

    m_runtimeLibComboBox->setCurrentIndex(m_tool->runtimeLibrary());

    basicWidget->insertTableRow(tr("Runtime Library"),
                                m_runtimeLibComboBox,
                                tr("Specify runtime library for linking. (/MT, /MTd, /MD, /MDd)"));

    m_structMemberAlignComboBox = new QComboBox;
    m_structMemberAlignComboBox->addItem(tr("Default"), QVariant(0));
    m_structMemberAlignComboBox->addItem(tr("1 Byte (/Zp1)"), QVariant(1));
    m_structMemberAlignComboBox->addItem(tr("2 Bytes (/Zp2)"), QVariant(2));
    m_structMemberAlignComboBox->addItem(tr("4 Bytes (/Zp4)"), QVariant(3));
    m_structMemberAlignComboBox->addItem(tr("8 Bytes (/Zp8)"), QVariant(4));
    m_structMemberAlignComboBox->addItem(tr("16 Bytes (/Zp16)"), QVariant(5));

    m_structMemberAlignComboBox->setCurrentIndex(m_tool->structMemberAlignment());

    basicWidget->insertTableRow(tr("Struct Member Alignment"),
                                m_structMemberAlignComboBox,
                                tr("Specify 1, 2, 4, 8, or 16-byte boundaries for struct member alignment. (/Zp[num])"));

    m_bufferSecCheckComboBox = new QComboBox;
    m_bufferSecCheckComboBox->addItem(tr("No (/GS-)"), QVariant(false));
    m_bufferSecCheckComboBox->addItem(tr("Yes"), QVariant(true));

    if (m_tool->bufferSecurityCheck())
        m_bufferSecCheckComboBox->setCurrentIndex(1);
    else
        m_bufferSecCheckComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Buffer Security Check"),
                                m_bufferSecCheckComboBox,
                                tr("Check for buffer overruns; useful for closing hackable loopholes on internet servers. The default is enabled. (/GS-)"));

    m_enableFuncLvlLinkComboBox = new QComboBox;
    m_enableFuncLvlLinkComboBox->addItem(tr("No"), QVariant(false));
    m_enableFuncLvlLinkComboBox->addItem(tr("Yes (/Gy)"), QVariant(true));

    if (m_tool->enableFunctionLevelLinking())
        m_enableFuncLvlLinkComboBox->setCurrentIndex(1);
    else
        m_enableFuncLvlLinkComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Enable Function Level Linking"),
                                m_enableFuncLvlLinkComboBox,
                                tr("Enables function-level linking; required for Edit and Continue to work. (/Gy)"));

    m_enableEnhInstrSetComboBox = new QComboBox;
    m_enableEnhInstrSetComboBox->addItem(tr("Not Set"), QVariant(0));
    m_enableEnhInstrSetComboBox->addItem(tr("Streaming SIMD Extensions (/arch:SSE)"), QVariant(1));
    m_enableEnhInstrSetComboBox->addItem(tr("Streaming SIMD Extensions 2 (/arch:SSE2)"), QVariant(2));

    m_enableEnhInstrSetComboBox->setCurrentIndex(m_tool->enableEnhancedInstructionSet());

    basicWidget->insertTableRow(tr("Enable Enhanced Instruction Set"),
                                m_enableEnhInstrSetComboBox,
                                tr("Enable use of instructions found on processors that support enhanced instruction sets, e.g., the SSE and SSE2 enhancements to the IA-32. Currently only available when building for the x86 architecture. (/arch:SSE, /arch:SSE2)"));

    m_floatPointModelComboBox = new QComboBox;
    m_floatPointModelComboBox->addItem(tr("Precise (/fp:precise)"), QVariant(0));
    m_floatPointModelComboBox->addItem(tr("Strict (/fp:strict)"), QVariant(1));
    m_floatPointModelComboBox->addItem(tr("Fast (/fp:fast)"), QVariant(2));

    m_floatPointModelComboBox->setCurrentIndex(m_tool->floatingPointModel());

    basicWidget->insertTableRow(tr("Floating Point Model"),
                                m_floatPointModelComboBox,
                                tr("Sets the floating point model."));

    m_floatPointExcpComboBox = new QComboBox;
    m_floatPointExcpComboBox->addItem(tr("No"), QVariant(false));
    m_floatPointExcpComboBox->addItem(tr("Yes (/fp:except)"), QVariant(true));

    if (m_tool->floatingPointExceptions())
        m_floatPointExcpComboBox->setCurrentIndex(1);
    else
        m_floatPointExcpComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Floating Point Exceptions"),
                                m_floatPointExcpComboBox,
                                tr("Enables floating point exceptions when generating code. (/fp:except)"));

    return basicWidget;
}

QWidget *CAndCppToolWidget::createLanguageWidget()
{
    BasicConfigurationWidget *basicWidget = new BasicConfigurationWidget;

    m_disLangExtComboBox = new QComboBox;
    m_disLangExtComboBox->addItem(tr("No"), QVariant(false));
    m_disLangExtComboBox->addItem(tr("Yes (/Za)"), QVariant(true));

    if (m_tool->disableLanguageExtensions())
        m_disLangExtComboBox->setCurrentIndex(1);
    else
        m_disLangExtComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Disable Language Extensions"),
                                m_disLangExtComboBox,
                                tr("Suppresses or enables language extensions."));

    m_defCharUnsignedComboBox = new QComboBox;
    m_defCharUnsignedComboBox->addItem(tr("No"), QVariant(false));
    m_defCharUnsignedComboBox->addItem(tr("Yes (/J)"), QVariant(true));

    if (m_tool->defaultCharIsUnsigned())
        m_defCharUnsignedComboBox->setCurrentIndex(1);
    else
        m_defCharUnsignedComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Default Char Is Unsigned"),
                                m_defCharUnsignedComboBox,
                                tr("Sets default char type to unsigned."));

    m_treatWcharasBuiltInComboBox = new QComboBox;
    m_treatWcharasBuiltInComboBox->addItem(tr("No (/Zc:wchar_t-)"), QVariant(false));
    m_treatWcharasBuiltInComboBox->addItem(tr("Yes"), QVariant(true));

    if (m_tool->treatWChartAsBuiltInType())
        m_treatWcharasBuiltInComboBox->setCurrentIndex(1);
    else
        m_treatWcharasBuiltInComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Treat WChar_t As Built In Type"),
                                m_treatWcharasBuiltInComboBox,
                                tr("Treats wchar_t as a built-in type."));

    m_forceConformanceComboBox = new QComboBox;
    m_forceConformanceComboBox->addItem(tr("No (/Zc:forScope-)"), QVariant(false));
    m_forceConformanceComboBox->addItem(tr("Yes"), QVariant(true));

    if (m_tool->forceConformanceInForLoopScope())
        m_forceConformanceComboBox->setCurrentIndex(1);
    else
        m_forceConformanceComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Force Conformance In For Loop Scope"),
                                m_forceConformanceComboBox,
                                tr("Forces the compiler to conform to the local scope in a for loop."));

    m_runTimeTypeComboBox = new QComboBox;
    m_runTimeTypeComboBox->addItem(tr("No (/GR)"), QVariant(false));
    m_runTimeTypeComboBox->addItem(tr("Yes"), QVariant(true));

    if (m_tool->runtimeTypeInfo())
        m_runTimeTypeComboBox->setCurrentIndex(1);
    else
        m_runTimeTypeComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Enable Run-Time Type Info"),
                                m_runTimeTypeComboBox,
                                tr("Adds code for checking C++ object types at run time (runtime type information)."));

    m_openMPComboBox = new QComboBox;
    m_openMPComboBox->addItem(tr("No"), QVariant(false));
    m_openMPComboBox->addItem(tr("Yes (/openmp)"), QVariant(true));

    if (m_tool->openMP())
        m_openMPComboBox->setCurrentIndex(1);
    else
        m_openMPComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("OpenMP Support"),
                                m_openMPComboBox,
                                tr("Enables OpenMP 2.0 language extensions."));

    return basicWidget;
}

QWidget *CAndCppToolWidget::createPrecompiledHeadersWidget()
{
    BasicConfigurationWidget *basicWidget = new BasicConfigurationWidget;

    m_usePrecomHeadComboBox = new QComboBox;
    m_usePrecomHeadComboBox->addItem(tr("Not Using Precompiled Headers"), QVariant(0));
    m_usePrecomHeadComboBox->addItem(tr("Create Precompiled Header (/Yc)"), QVariant(1));
    m_usePrecomHeadComboBox->addItem(tr("Use Precompiled Header (/Yu)"), QVariant(2));

    m_usePrecomHeadComboBox->setCurrentIndex(m_tool->usePrecompiledHeader());

    basicWidget->insertTableRow(tr("Create/Use Precompiled Header"),
                                m_usePrecomHeadComboBox,
                                tr("Enables creation or use of a precompiled header during the build."));

    m_precomHeadThroughLineEdit = new QLineEdit;
    m_precomHeadThroughLineEdit->setText(m_tool->precompiledHeaderThrough());
    basicWidget->insertTableRow(tr("Create/Use PCH Through File"),
                                m_precomHeadThroughLineEdit,
                                tr("Specifies header file name to use when creating or using a precompiled header file."));

    m_precomHeaderFileLineEdit = new QLineEdit;
    m_precomHeaderFileLineEdit->setText(m_tool->precompiledHeaderFile());
    basicWidget->insertTableRow(tr("Precompiled Header File"),
                                m_precomHeaderFileLineEdit,
                                tr("Specifies the path and/or name of the generated precompiled header file."));

    return basicWidget;
}

QWidget *CAndCppToolWidget::createOutputFilesWidget()
{
    BasicConfigurationWidget *basicWidget = new BasicConfigurationWidget;

    m_expandAttrSrcComboBox = new QComboBox;
    m_expandAttrSrcComboBox->addItem(tr("No"), QVariant(false));
    m_expandAttrSrcComboBox->addItem(tr("Yes (/Fx)"), QVariant(true));

    if (m_tool->expandAttributedSource())
        m_expandAttrSrcComboBox->setCurrentIndex(1);
    else
        m_expandAttrSrcComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Expand Attributed Source"),
                                m_expandAttrSrcComboBox,
                                tr("Create listing file with expanded attributes injected into source file."));

    m_assemblerOutputComboBox = new QComboBox;
    m_assemblerOutputComboBox->addItem(tr("No Listing"), QVariant(0));
    m_assemblerOutputComboBox->addItem(tr("Assembly-Only Listing (/FA)"), QVariant(1));
    m_assemblerOutputComboBox->addItem(tr("Assembly, Machine Code and Source (/FAcs)"), QVariant(2));
    m_assemblerOutputComboBox->addItem(tr("Assembly With Machine Code (/Fac)"), QVariant(3));
    m_assemblerOutputComboBox->addItem(tr("Assembly With Source Code (/FAs)"), QVariant(4));

    m_assemblerOutputComboBox->setCurrentIndex(m_tool->assemblerOutput());

    basicWidget->insertTableRow(tr("Assembler Output"),
                                m_assemblerOutputComboBox,
                                tr("Specifies the contents of assembly language output file."));

    m_asmListLocLineEdit = new QLineEdit;
    m_asmListLocLineEdit->setText(m_tool->assemblerListingLocation());
    basicWidget->insertTableRow(tr("ASM List Location"),
                                m_asmListLocLineEdit,
                                tr("Specifies relative path and/or name for ASM listing file; can be file or directory name."));

    m_objectFileLineEdit = new QLineEdit;
    m_objectFileLineEdit->setText(m_tool->objectFile());
    basicWidget->insertTableRow(tr("Object File Name"),
                                m_objectFileLineEdit,
                                tr("Specifies a name to override the default object file name; can be file or directory name."));

    m_progDataLineEdit = new QLineEdit;
    m_progDataLineEdit->setText(m_tool->programDataBaseFileName());
    basicWidget->insertTableRow(tr("Program Data Base File Name"),
                                m_progDataLineEdit,
                                tr("Specifies a name for a compiler-generated PDB file; also specifies base name for the required compiler-generated IDB file; can be file or directory name."));

    m_genXMLDocComboBox = new QComboBox;
    m_genXMLDocComboBox->addItem(tr("No"), QVariant(false));
    m_genXMLDocComboBox->addItem(tr("Yes (/doc)"), QVariant(true));

    if (m_tool->generateXMLDocumentationFiles())
        m_genXMLDocComboBox->setCurrentIndex(1);
    else
        m_genXMLDocComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Generate XML Documentation Files"),
                                m_genXMLDocComboBox,
                                tr("Specifies that the compiler should generate XML documentation comment file (.XDC)."));

    m_xmlDocFileNameLineEdit = new QLineEdit;
    m_xmlDocFileNameLineEdit->setText(m_tool->xmlDocumentationFileName());
    basicWidget->insertTableRow(tr("XML Documentation File Name"),
                                m_xmlDocFileNameLineEdit,
                                tr("Specifies the name of the generated XML documentation files; can be file or directory name."));

    return basicWidget;
}

QWidget *CAndCppToolWidget::createBrowseInformationWidget()
{
    BasicConfigurationWidget *basicWidget = new BasicConfigurationWidget;

    m_browseInfoComboBox = new QComboBox;
    m_browseInfoComboBox->addItem(tr("None"), QVariant(0));
    m_browseInfoComboBox->addItem(tr("Include All Browse Information (/FR)"), QVariant(1));
    m_browseInfoComboBox->addItem(tr("No Local Symbols (/Fr)"), QVariant(2));

    m_browseInfoComboBox->setCurrentIndex(m_tool->browseInformation());

    basicWidget->insertTableRow(tr("Enable Browse Information"),
                                m_browseInfoComboBox,
                                tr("Specifies level of Browse information in .bsc file."));

    m_browseInfoFileLineEdit = new QLineEdit;
    m_browseInfoFileLineEdit->setText(m_tool->browseInformationFile());
    basicWidget->insertTableRow(tr("Browse File"),
                                m_browseInfoFileLineEdit,
                                tr("Specifies optional name for browser information file."));

    return basicWidget;
}

QWidget *CAndCppToolWidget::createAdvancedWidget()
{
    BasicConfigurationWidget *basicWidget = new BasicConfigurationWidget;

    m_callConvComboBox = new QComboBox;
    m_callConvComboBox->addItem(tr("__cdecl (/Gd)"), QVariant(0));
    m_callConvComboBox->addItem(tr("__fastcall (/Gr)"), QVariant(1));
    m_callConvComboBox->addItem(tr("__stdcall (/Gz)"), QVariant(2));

    m_callConvComboBox->setCurrentIndex(m_tool->callingConvention());

    basicWidget->insertTableRow(tr("Calling Convention"),
                                m_callConvComboBox,
                                tr("Select the default calling convention for your application (can be overridden by function)."));

    m_compileAsComboBox = new QComboBox;
    m_compileAsComboBox->addItem(tr("Default"), QVariant(0));
    m_compileAsComboBox->addItem(tr("Compile as C Code (/TC)"), QVariant(1));
    m_compileAsComboBox->addItem(tr("Compile as C++ Code (/TP)"), QVariant(2));

    m_compileAsComboBox->setCurrentIndex(m_tool->compileAs());

    basicWidget->insertTableRow(tr("Compile As"),
                                m_compileAsComboBox,
                                tr("Select compile language option for .c and .cpp files."));

    m_disableSpecWarningLineEdit = new LineEdit;
    m_disableSpecWarningLineEdit->setTextList(m_tool->disableSpecificWarnings(), QLatin1String(";"));
    basicWidget->insertTableRow(tr("Disable Specific Warnings"),
                                m_disableSpecWarningLineEdit,
                                tr("Disable the desired warning numbers; put numbers in a semi-colon delimited list."));

    m_forcedIncFilesLineEdit = new LineEdit;
    m_forcedIncFilesLineEdit->setTextList(m_tool->forcedIncludeFiles(), QLatin1String(";"));
    basicWidget->insertTableRow(tr("Force Includes"),
                                m_forcedIncFilesLineEdit,
                                tr("Specifies one or more forced include files."));

    m_forceUsingFilesLineEdit = new LineEdit;
    m_forceUsingFilesLineEdit->setTextList(m_tool->forcedUsingFiles(), QLatin1String(";"));
    basicWidget->insertTableRow(tr("Force #using"),
                                m_forceUsingFilesLineEdit,
                                tr("Specifies one or more forced #using files."));

    m_showIncComboBox = new QComboBox;
    m_showIncComboBox->addItem(tr("No"), QVariant(false));
    m_showIncComboBox->addItem(tr("Yes (/showIncludes)"), QVariant(true));

    if (m_tool->showIncludes())
        m_showIncComboBox->setCurrentIndex(1);
    else
        m_showIncComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Show Includes"),
                                m_showIncComboBox,
                                tr("Generate a list of include files with compile output."));

    m_undefPreprocDefLineEdit = new LineEdit;
    m_undefPreprocDefLineEdit->setTextList(m_tool->undefinePreprocessorDefinitions(), QLatin1String(";"));
    basicWidget->insertTableRow(tr("Undefine Preprocessor Definitions"),
                                m_undefPreprocDefLineEdit,
                                tr("Specifies one or more preprocessor undefines."));

    m_undefAllPreprocDefComboBox = new QComboBox;
    m_undefAllPreprocDefComboBox->addItem(tr("No"), QVariant(false));
    m_undefAllPreprocDefComboBox->addItem(tr("Yes (/u)"), QVariant(true));

    basicWidget->insertTableRow(tr("Undefine All Preprocessor Definitions"),
                                m_undefAllPreprocDefComboBox,
                                tr("Undefine all previously defined preprocessor values."));

    m_useFullPathsComboBox = new QComboBox;
    m_useFullPathsComboBox->addItem(tr("No"), QVariant(false));
    m_useFullPathsComboBox->addItem(tr("Yes (/FC)"), QVariant(true));

    if (m_tool->useFullPaths())
        m_useFullPathsComboBox->setCurrentIndex(1);
    else
        m_useFullPathsComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Use Full Paths"),
                                m_useFullPathsComboBox,
                                tr("Use full paths in diagnostic messages."));

    m_omitDefLibNameComboBox = new QComboBox;
    m_omitDefLibNameComboBox->addItem(tr("No"), QVariant(false));
    m_omitDefLibNameComboBox->addItem(tr("Yes (/Zl)"), QVariant(true));

    if (m_tool->omitDefaultLibName())
        m_omitDefLibNameComboBox->setCurrentIndex(1);
    else
        m_omitDefLibNameComboBox->setCurrentIndex(0);

    basicWidget->insertTableRow(tr("Omit Default Library Names"),
                                m_omitDefLibNameComboBox,
                                tr("Do not include default library names in .obj files."));

    m_errReportComboBox = new QComboBox;
    m_errReportComboBox->addItem(tr("Default"), QVariant(0));
    m_errReportComboBox->addItem(tr("Prompt Immediately (/errorReport:prompt)"), QVariant(1));
    m_errReportComboBox->addItem(tr("Queue For Next Login (/errorReport:queue)"), QVariant(2));

    m_errReportComboBox->setCurrentIndex(m_tool->errorReporting());

    basicWidget->insertTableRow(tr("Error Reporting"),
                                m_errReportComboBox,
                                tr("Specifies how internal tool errors should be reported back to Microsoft. The default in the IDE is prompt. The default from command line builds is queue."));

    return basicWidget;
}

void CAndCppToolWidget::saveGeneralData()
{
    m_tool->setAdditionalIncludeDirectories(m_additIncDirsLineEdit->textToList(QLatin1String(";")));
    m_tool->setAdditionalUsingDirectories(m_resolveRefsLineEdit->textToList(QLatin1String(";")));

    if (m_dbgInfoFormatComboBox->currentIndex() >= 0) {
        QVariant data = m_dbgInfoFormatComboBox->itemData(m_dbgInfoFormatComboBox->currentIndex(),
                                                          Qt::UserRole);
        m_tool->setDebugInformationFormat(static_cast<DebugInformationFormatEnum>(data.toInt()));
    }

    if (m_suppresStBannerComboBox->currentIndex() >= 0) {
        QVariant data = m_suppresStBannerComboBox->itemData(m_suppresStBannerComboBox->currentIndex(),
                                                            Qt::UserRole);
        m_tool->setSuppressStartupBanner(data.toBool());
    }

    if (m_warningLevelComboBox->currentIndex() >= 0) {
        QVariant data = m_warningLevelComboBox->itemData(m_warningLevelComboBox->currentIndex(),
                                                         Qt::UserRole);
        m_tool->setWarningLevel(static_cast<WarningLevelEnum>(data.toInt()));
    }

    if (m_detect64BitPortIssuesComboBox->currentIndex() >= 0) {
        QVariant data = m_detect64BitPortIssuesComboBox->itemData(m_detect64BitPortIssuesComboBox->currentIndex(),
                                                                  Qt::UserRole);
        m_tool->setDetect64BitPortabilityProblems(data.toBool());
    }

    if (m_threatWarnAsErrComboBox->currentIndex() >= 0) {
        QVariant data = m_threatWarnAsErrComboBox->itemData(m_threatWarnAsErrComboBox->currentIndex(),
                                                            Qt::UserRole);
        m_tool->setWarnAsError(data.toBool());
    }

    if (m_useUnicodeRespFilesComboBox->currentIndex() >= 0) {
        QVariant data = m_useUnicodeRespFilesComboBox->itemData(m_useUnicodeRespFilesComboBox->currentIndex(),
                                                                Qt::UserRole);
        m_tool->setUseUnicodeResponseFiles(data.toBool());
    }
}

void CAndCppToolWidget::saveOptimizationData()
{
    if (m_optimizationComboBox->currentIndex() >= 0) {
        QVariant data = m_optimizationComboBox->itemData(m_optimizationComboBox->currentIndex(),
                                                         Qt::UserRole);
        m_tool->setOptimization(static_cast<OptimizationEnum>(data.toInt()));
    }

    if (m_inlineFuncExpComboBox->currentIndex() >= 0) {
        QVariant data = m_inlineFuncExpComboBox->itemData(m_inlineFuncExpComboBox->currentIndex(),
                                                          Qt::UserRole);
        m_tool->setInlineFunctionExpansion(static_cast<InlineFunctionExpansionEnum>(data.toInt()));
    }

    if (m_enableInstrFuncComboBox->currentIndex() >= 0) {
        QVariant data = m_enableInstrFuncComboBox->itemData(m_enableInstrFuncComboBox->currentIndex(),
                                                            Qt::UserRole);
        m_tool->setEnableIntrinsicFunctions(data.toBool());
    }

    if (m_favorSizeorSpeedComboBox->currentIndex() >= 0) {
        QVariant data = m_favorSizeorSpeedComboBox->itemData(m_favorSizeorSpeedComboBox->currentIndex(),
                                                             Qt::UserRole);
        m_tool->setFavorSizeOrSpeed(static_cast<FavorSizeOrSpeedEnum>(data.toInt()));
    }

    if (m_omitFramePtrComboBox->currentIndex() >= 0) {
        QVariant data = m_omitFramePtrComboBox->itemData(m_omitFramePtrComboBox->currentIndex(),
                                                         Qt::UserRole);
        m_tool->setOmitFramePointers(data.toBool());
    }

    if (m_enableFiberSafeOptmComboBox->currentIndex() >= 0) {
        QVariant data = m_enableFiberSafeOptmComboBox->itemData(m_enableFiberSafeOptmComboBox->currentIndex(),
                                                                Qt::UserRole);
        m_tool->setEnableFiberSafeOptimizations(data.toBool());
    }

    if (m_wholeProgOptmComboBox->currentIndex() >= 0) {
        QVariant data = m_wholeProgOptmComboBox->itemData(m_wholeProgOptmComboBox->currentIndex(),
                                                          Qt::UserRole);
        m_tool->setWholeProgramOptimization(data.toBool());
    }
}

void CAndCppToolWidget::savePreprocessorData()
{
    m_tool->setPreprocessorDefinitions(m_preProcDefLineEdit->textToList(QLatin1String(";")));

    if (m_ignoreStdIncPathComboBox->currentIndex() >= 0) {
        QVariant data = m_ignoreStdIncPathComboBox->itemData(m_ignoreStdIncPathComboBox->currentIndex(),
                                                             Qt::UserRole);
        m_tool->setIgnoreStandardIncludePath(data.toBool());
    }

    if (m_generPreProcFileComboBox->currentIndex() >= 0) {
        QVariant data = m_generPreProcFileComboBox->itemData(m_generPreProcFileComboBox->currentIndex(),
                                                             Qt::UserRole);
        m_tool->setGeneratePreprocessedFile(static_cast<GeneratePreprocessedFileEnum>(data.toInt()));
    }

    if (m_keepCommentsComboBox->currentIndex() >= 0) {
        QVariant data = m_keepCommentsComboBox->itemData(m_keepCommentsComboBox->currentIndex(),
                                                         Qt::UserRole);
        m_tool->setKeepComments(data.toBool());
    }
}

void CAndCppToolWidget::saveCodeGenerationData()
{
    if (m_stringPoolingComboBox->currentIndex() >= 0) {
        QVariant data = m_stringPoolingComboBox->itemData(m_stringPoolingComboBox->currentIndex(),
                                                           Qt::UserRole);
        m_tool->setStringPooling(data.toBool());
    }

    if (m_minimalRebuildComboBox->currentIndex() >= 0) {
        QVariant data = m_minimalRebuildComboBox->itemData(m_minimalRebuildComboBox->currentIndex(),
                                                           Qt::UserRole);
        m_tool->setMinimalRebuild(data.toBool());
    }

    if (m_exceptionHandlComboBox->currentIndex() >= 0) {
        QVariant data = m_exceptionHandlComboBox->itemData(m_exceptionHandlComboBox->currentIndex(),
                                                           Qt::UserRole);
        m_tool->setExceptionHandling(static_cast<ExceptionHandlingEnum>(data.toInt()));
    }

    if (m_smallerTypeCheckComboBox->currentIndex() >= 0) {
        QVariant data = m_smallerTypeCheckComboBox->itemData(m_smallerTypeCheckComboBox->currentIndex(),
                                                             Qt::UserRole);
        m_tool->setSmallerTypeCheck(data.toBool());
    }

    if (m_basicRuntimeChecksComboBox->currentIndex() >= 0) {
        QVariant data = m_basicRuntimeChecksComboBox->itemData(m_basicRuntimeChecksComboBox->currentIndex(),
                                                               Qt::UserRole);
        m_tool->setBasicRuntimeChecks(static_cast<BasicRuntimeChecksEnum>(data.toInt()));
    }

    if (m_runtimeLibComboBox->currentIndex() >= 0) {
        QVariant data = m_runtimeLibComboBox->itemData(m_runtimeLibComboBox->currentIndex(),
                                                       Qt::UserRole);
        m_tool->setRuntimeLibrary(static_cast<RuntimeLibraryEnum>(data.toInt()));
    }

    if (m_structMemberAlignComboBox->currentIndex() >= 0) {
        QVariant data = m_structMemberAlignComboBox->itemData(m_structMemberAlignComboBox->currentIndex(),
                                                              Qt::UserRole);
        m_tool->setStructMemberAlignment(static_cast<StructMemberAlignmentEnum>(data.toInt()));
    }

    if (m_bufferSecCheckComboBox->currentIndex() >= 0) {
        QVariant data = m_bufferSecCheckComboBox->itemData(m_bufferSecCheckComboBox->currentIndex(),
                                                           Qt::UserRole);
        m_tool->setBufferSecurityCheck(data.toBool());
    }

    if (m_enableFuncLvlLinkComboBox->currentIndex() >= 0) {
        QVariant data = m_enableFuncLvlLinkComboBox->itemData(m_enableFuncLvlLinkComboBox->currentIndex(),
                                                              Qt::UserRole);
        m_tool->setEnableFunctionLevelLinking(data.toBool());
    }

    if (m_enableEnhInstrSetComboBox->currentIndex() >= 0) {
        QVariant data = m_enableEnhInstrSetComboBox->itemData(m_enableEnhInstrSetComboBox->currentIndex(),
                                                              Qt::UserRole);
        m_tool->setEnableEnhancedInstructionSet(static_cast<EnableEnhancedInstructionSetEnum>(data.toInt()));
    }

    if (m_floatPointModelComboBox->currentIndex() >= 0) {
        QVariant data = m_floatPointModelComboBox->itemData(m_floatPointModelComboBox->currentIndex(),
                                                            Qt::UserRole);
        m_tool->setFloatingPointModel(static_cast<FloatingPointModelEnum>(data.toInt()));
    }

    if (m_floatPointExcpComboBox->currentIndex() >= 0) {
        QVariant data = m_floatPointExcpComboBox->itemData(m_floatPointExcpComboBox->currentIndex(),
                                                           Qt::UserRole);
        m_tool->setFloatingPointExceptions(data.toBool());
    }
}

void CAndCppToolWidget::saveLanguageData()
{
    if (m_disLangExtComboBox->currentIndex() >= 0) {
        QVariant data = m_disLangExtComboBox->itemData(m_disLangExtComboBox->currentIndex(),
                                                           Qt::UserRole);
        m_tool->setDisableLanguageExtensions(data.toBool());
    }

    if (m_defCharUnsignedComboBox->currentIndex() >= 0) {
        QVariant data = m_defCharUnsignedComboBox->itemData(m_defCharUnsignedComboBox->currentIndex(),
                                                           Qt::UserRole);
        m_tool->setDefaultCharIsUnsigned(data.toBool());
    }

    if (m_treatWcharasBuiltInComboBox->currentIndex() >= 0) {
        QVariant data = m_treatWcharasBuiltInComboBox->itemData(m_treatWcharasBuiltInComboBox->currentIndex(),
                                                           Qt::UserRole);
        m_tool->setTreatWChartAsBuiltInType(data.toBool());
    }

    if (m_forceConformanceComboBox->currentIndex() >= 0) {
        QVariant data = m_forceConformanceComboBox->itemData(m_forceConformanceComboBox->currentIndex(),
                                                           Qt::UserRole);
        m_tool->setForceConformanceInForLoopScope(data.toBool());
    }

    if (m_runTimeTypeComboBox->currentIndex() >= 0) {
        QVariant data = m_runTimeTypeComboBox->itemData(m_runTimeTypeComboBox->currentIndex(),
                                                           Qt::UserRole);
        m_tool->setRuntimeTypeInfo(data.toBool());
    }

    if (m_openMPComboBox->currentIndex() >= 0) {
        QVariant data = m_openMPComboBox->itemData(m_openMPComboBox->currentIndex(),
                                                           Qt::UserRole);
        m_tool->setOpenMP(data.toBool());
    }
}

void CAndCppToolWidget::savePrecompiledHeadersData()
{
    if (m_usePrecomHeadComboBox->currentIndex() >= 0) {
        QVariant data = m_usePrecomHeadComboBox->itemData(m_usePrecomHeadComboBox->currentIndex(),
                                                            Qt::UserRole);
        m_tool->setUsePrecompiledHeader(static_cast<UsePrecompiledHeaderEnum>(data.toInt()));
    }

    m_tool->setPrecompiledHeaderThrough(m_precomHeadThroughLineEdit->text());
    m_tool->setPrecompiledHeaderFile(m_precomHeaderFileLineEdit->text());
}

void CAndCppToolWidget::saveOutputFilesData()
{
    if (m_expandAttrSrcComboBox->currentIndex() >= 0) {
        QVariant data = m_expandAttrSrcComboBox->itemData(m_expandAttrSrcComboBox->currentIndex(),
                                                           Qt::UserRole);
        m_tool->setExpandAttributedSource(data.toBool());
    }

    if (m_assemblerOutputComboBox->currentIndex() >= 0) {
        QVariant data = m_assemblerOutputComboBox->itemData(m_assemblerOutputComboBox->currentIndex(),
                                                            Qt::UserRole);
        m_tool->setAssemblerOutput(static_cast<AssemblerOutputEnum>(data.toInt()));
    }

    m_tool->setAssemblerListingLocation(m_asmListLocLineEdit->text());
    m_tool->setObjectFile(m_objectFileLineEdit->text());
    m_tool->setProgramDataBaseFileName(m_progDataLineEdit->text());

    if (m_genXMLDocComboBox->currentIndex() >= 0) {
        QVariant data = m_genXMLDocComboBox->itemData(m_genXMLDocComboBox->currentIndex(),
                                                           Qt::UserRole);
        m_tool->setGenerateXMLDocumentationFiles(data.toBool());
    }

    m_tool->setXMLDocumentationFileName(m_xmlDocFileNameLineEdit->text());
}

void CAndCppToolWidget::saveBrowseInformationData()
{
    if (m_browseInfoComboBox->currentIndex() >= 0) {
        QVariant data = m_browseInfoComboBox->itemData(m_browseInfoComboBox->currentIndex(),
                                                            Qt::UserRole);
        m_tool->setBrowseInformation(static_cast<BrowseInformationEnum>(data.toInt()));
    }

    m_tool->setBrowseInformationFile(m_browseInfoFileLineEdit->text());
}

void CAndCppToolWidget::saveAdvancedData()
{
    if (m_callConvComboBox->currentIndex() >= 0) {
        QVariant data = m_callConvComboBox->itemData(m_callConvComboBox->currentIndex(),
                                                            Qt::UserRole);
        m_tool->setCallingConvention(static_cast<CallingConventionEnum>(data.toInt()));
    }

    if (m_compileAsComboBox->currentIndex() >= 0) {
        QVariant data = m_compileAsComboBox->itemData(m_compileAsComboBox->currentIndex(),
                                                            Qt::UserRole);
        m_tool->setCompileAs(static_cast<CompileAsEnum>(data.toInt()));
    }

    m_tool->setDisableSpecificWarnings(m_disableSpecWarningLineEdit->textToList(QLatin1String(";")));
    m_tool->setForcedIncludeFiles(m_forcedIncFilesLineEdit->textToList(QLatin1String(";")));
    m_tool->setForcedUsingFiles(m_forceUsingFilesLineEdit->textToList(QLatin1String(";")));

    if (m_showIncComboBox->currentIndex() >= 0) {
        QVariant data = m_showIncComboBox->itemData(m_showIncComboBox->currentIndex(),
                                                           Qt::UserRole);
        m_tool->setShowIncludes(data.toBool());
    }

    m_tool->setUndefinePreprocessorDefinitions(m_undefPreprocDefLineEdit->textToList(QLatin1String(";")));

    if (m_undefAllPreprocDefComboBox->currentIndex() >= 0) {
        QVariant data = m_undefAllPreprocDefComboBox->itemData(m_undefAllPreprocDefComboBox->currentIndex(),
                                                           Qt::UserRole);
        m_tool->setUndefineAllPreprocessorDefinitions(data.toBool());
    }

    if (m_useFullPathsComboBox->currentIndex() >= 0) {
        QVariant data = m_useFullPathsComboBox->itemData(m_useFullPathsComboBox->currentIndex(),
                                                           Qt::UserRole);
        m_tool->setUseFullPaths(data.toBool());
    }

    if (m_omitDefLibNameComboBox->currentIndex() >= 0) {
        QVariant data = m_omitDefLibNameComboBox->itemData(m_omitDefLibNameComboBox->currentIndex(),
                                                           Qt::UserRole);
        m_tool->setOmitDefaultLibName(data.toBool());
    }

    if (m_errReportComboBox->currentIndex() >= 0) {
        QVariant data = m_errReportComboBox->itemData(m_errReportComboBox->currentIndex(),
                                                           Qt::UserRole);
        m_tool->setErrorReporting(static_cast<ErrorReportingCCppEnum>(data.toInt()));
    }
}

} // namespace Internal
} // namespace VcProjectManager
