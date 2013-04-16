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
#ifndef VCPROJECTMANAGER_INTERNAL_CANDCPPTOOL_H
#define VCPROJECTMANAGER_INTERNAL_CANDCPPTOOL_H

#include "tool.h"
#include "candcpptool_constants.h"
#include "../../widgets/vcnodewidget.h"

class QComboBox;
class QLineEdit;

namespace VcProjectManager {
namespace Internal {

using namespace CAndCppConstants;

class LineEdit;

class CAndCppTool : public Tool
{
public:
    typedef QSharedPointer<CAndCppTool> Ptr;

    CAndCppTool();
    CAndCppTool(const CAndCppTool &tool);
    ~CAndCppTool();

    QString nodeWidgetName() const;
    VcNodeWidget *createSettingsWidget();
    Tool::Ptr clone() const;

    // General
    QStringList additionalIncludeDirectories() const;
    QStringList additionalIncludeDirectoriesDefault() const;
    void setAdditionalIncludeDirectories(const QStringList &additIncDirs);
    QStringList additionalUsingDirectories() const;
    QStringList additionalUsingDirectoriesDefault() const;
    void setAdditionalUsingDirectories(const QStringList &dirs);
    DebugInformationFormatEnum debugInformationFormat() const;
    DebugInformationFormatEnum debugInformationFormatDefault() const;
    void setDebugInformationFormat(DebugInformationFormatEnum value);
    bool suppressStartupBanner() const;
    bool suppressStartupBannerDefault() const;
    void setSuppressStartupBanner(bool suppress);
    WarningLevelEnum warningLevel() const;
    WarningLevelEnum warningLevelDefault() const;
    void setWarningLevel(WarningLevelEnum level);
    bool detect64BitPortabilityProblems() const;
    bool detect64BitPortabilityProblemsDefault() const;
    void setDetect64BitPortabilityProblems(bool detect);
    bool warnAsError() const;
    bool warnAsErrorDefault() const;
    void setWarnAsError(bool warn);
    bool useUnicodeResponseFiles() const;
    bool useUnicodeResponseFilesDefault() const;
    void setUseUnicodeResponseFiles(bool use);

    // Optimization
    OptimizationEnum optimization() const;
    OptimizationEnum optimizationDefault() const;
    void setOptimization(OptimizationEnum optim);
    InlineFunctionExpansionEnum inlineFunctionExpansion() const;
    InlineFunctionExpansionEnum inlineFunctionExpansionDefault() const;
    void setInlineFunctionExpansion(InlineFunctionExpansionEnum value);
    bool enableIntrinsicFunctions() const;
    bool enableIntrinsicFunctionsDefault() const;
    void setEnableIntrinsicFunctions(bool enable);
    FavorSizeOrSpeedEnum favorSizeOrSpeed() const;
    FavorSizeOrSpeedEnum favorSizeOrSpeedDefault() const;
    void setFavorSizeOrSpeed(FavorSizeOrSpeedEnum value);
    bool omitFramePointers() const;
    bool omitFramePointersDefault() const;
    void setOmitFramePointers(bool omit);
    bool enableFiberSafeOptimizations() const;
    bool enableFiberSafeOptimizationsDefault() const;
    void setEnableFiberSafeOptimizations(bool enable);
    bool wholeProgramOptimization() const;
    bool wholeProgramOptimizationDefault() const;
    void setWholeProgramOptimization(bool optimize);

    // Preprocessor
    QStringList preprocessorDefinitions() const;
    QStringList preprocessorDefinitionsDefault() const;
    void setPreprocessorDefinitions(const QStringList &defs);
    bool ignoreStandardIncludePath() const;
    bool ignoreStandardIncludePathDefault() const;
    void setIgnoreStandardIncludePath(bool ignore);
    GeneratePreprocessedFileEnum generatePreprocessedFile() const;
    GeneratePreprocessedFileEnum generatePreprocessedFileDefault() const;
    void setGeneratePreprocessedFile(GeneratePreprocessedFileEnum value);
    bool keepComments() const;
    bool keepCommentsDefault() const;
    void setKeepComments(bool keep);

    // Code Generation
    bool stringPooling() const;
    bool stringPoolingDefault() const;
    void setStringPooling(bool pool);
    bool minimalRebuild() const;
    bool minimalRebuildDefault() const;
    void setMinimalRebuild(bool enable);
    ExceptionHandlingEnum exceptionHandling() const;
    ExceptionHandlingEnum exceptionHandlingDefault() const;
    void setExceptionHandling(ExceptionHandlingEnum value);
    bool smallerTypeCheck() const;
    bool smallerTypeCheckDefault() const;
    void setSmallerTypeCheck(bool check);
    BasicRuntimeChecksEnum basicRuntimeChecks() const;
    BasicRuntimeChecksEnum basicRuntimeChecksDefault() const;
    void setBasicRuntimeChecks(BasicRuntimeChecksEnum value);
    RuntimeLibraryEnum runtimeLibrary() const;
    RuntimeLibraryEnum runtimeLibraryDefault() const;
    void setRuntimeLibrary(RuntimeLibraryEnum value);
    StructMemberAlignmentEnum structMemberAlignment() const;
    StructMemberAlignmentEnum structMemberAlignmentDefault() const;
    void setStructMemberAlignment(StructMemberAlignmentEnum value);
    bool bufferSecurityCheck() const;
    bool bufferSecurityCheckDefault() const;
    void setBufferSecurityCheck(bool check);
    bool enableFunctionLevelLinking() const;
    bool enableFunctionLevelLinkingDefault() const;
    void setEnableFunctionLevelLinking(bool enable);
    EnableEnhancedInstructionSetEnum enableEnhancedInstructionSet() const;
    EnableEnhancedInstructionSetEnum enableEnhancedInstructionSetDefault() const;
    void setEnableEnhancedInstructionSet(EnableEnhancedInstructionSetEnum value);
    FloatingPointModelEnum floatingPointModel() const;
    FloatingPointModelEnum floatingPointModelDefault() const;
    void setFloatingPointModel(FloatingPointModelEnum value);
    bool floatingPointExceptions() const;
    bool floatingPointExceptionsDefault() const;
    void setFloatingPointExceptions(bool enable);

    // Language
    bool disableLanguageExtensions() const;
    bool disableLanguageExtensionsDefault() const;
    void setDisableLanguageExtensions(bool disable);
    bool defaultCharIsUnsigned() const;
    bool defaultCharIsUnsignedDefault() const;
    void setDefaultCharIsUnsigned(bool isUnsigned);
    bool treatWChartAsBuiltInType() const;
    bool treatWChartAsBuiltInTypeDefault() const;
    void setTreatWChartAsBuiltInType(bool enable);
    bool forceConformanceInForLoopScope() const;
    bool forceConformanceInForLoopScopeDefault() const;
    void setForceConformanceInForLoopScope(bool force);
    bool runtimeTypeInfo() const;
    bool runtimeTypeInfoDefault() const;
    void setRuntimeTypeInfo(bool enable);
    bool openMP() const;
    bool openMPDefault() const;
    void setOpenMP(bool enable);

    // Precompiled Headers
    UsePrecompiledHeaderEnum usePrecompiledHeader() const;
    UsePrecompiledHeaderEnum usePrecompiledHeaderDefault() const;
    void setUsePrecompiledHeader(UsePrecompiledHeaderEnum value);
    QString precompiledHeaderThrough() const;
    QString precompiledHeaderThroughDefault() const;
    void setPrecompiledHeaderThrough(const QString &headerFile);
    QString precompiledHeaderFile() const;
    QString precompiledHeaderFileDefault() const;
    void setPrecompiledHeaderFile(const QString &headerFile);

    // Output Files
    bool expandAttributedSource() const;
    bool expandAttributedSourceDefault() const;
    void setExpandAttributedSource(bool expand);
    AssemblerOutputEnum assemblerOutput() const;
    AssemblerOutputEnum assemblerOutputDefault() const;
    void setAssemblerOutput(AssemblerOutputEnum value);
    QString assemblerListingLocation() const;
    QString assemblerListingLocationDefault() const;
    void setAssemblerListingLocation(const QString &location);
    QString objectFile() const;
    QString objectFileDefault() const;
    void setObjectFile(const QString &file);
    QString programDataBaseFileName() const;
    QString programDataBaseFileNameDefault() const;
    void setProgramDataBaseFileName(const QString &fileName);
    bool generateXMLDocumentationFiles() const;
    bool generateXMLDocumentationFilesDefault() const;
    void setGenerateXMLDocumentationFiles(bool generate);
    QString xmlDocumentationFileName() const;
    QString xmlDocumentationFileNameDefault() const;
    void setXMLDocumentationFileName(const QString fileName);

    // Browse Information
    BrowseInformationEnum browseInformation() const;
    BrowseInformationEnum browseInformationDefault() const;
    void setBrowseInformation(BrowseInformationEnum value);
    QString browseInformationFile() const;
    QString browseInformationFileDefault() const;
    void setBrowseInformationFile(const QString &file);

    // Advanced
    CallingConventionEnum callingConvention() const;
    CallingConventionEnum callingConventionDefault() const;
    void setCallingConvention(CallingConventionEnum value);
    CompileAsEnum compileAs() const;
    CompileAsEnum compileAsDefault() const;
    void setCompileAs(CompileAsEnum value);
    QStringList disableSpecificWarnings() const;
    QStringList disableSpecificWarningsDefault() const;
    void setDisableSpecificWarnings(const QStringList &warnings);
    QStringList forcedIncludeFiles() const;
    QStringList forcedIncludeFilesDefault() const;
    void setForcedIncludeFiles(const QStringList &files);
    QStringList forcedUsingFiles() const;
    QStringList forcedUsingFilesDefault() const;
    void setForcedUsingFiles(const QStringList &files);
    bool showIncludes() const;
    bool showIncludesDefault() const;
    void setShowIncludes(bool show);
    QStringList undefinePreprocessorDefinitions() const;
    QStringList undefinePreprocessorDefinitionsDefault() const;
    void setUndefinePreprocessorDefinitions(const QStringList &definitions);
    bool undefineAllPreprocessorDefinitions() const;
    bool undefineAllPreprocessorDefinitionsDefault() const;
    void setUndefineAllPreprocessorDefinitions(bool undefine);
    bool useFullPaths() const;
    bool useFullPathsDefault() const;
    void setUseFullPaths(bool use);
    bool omitDefaultLibName() const;
    bool omitDefaultLibNameDefault() const;
    void setOmitDefaultLibName(bool omit);
    ErrorReportingCCppEnum errorReporting() const;
    ErrorReportingCCppEnum errorReportingDefault() const;
    void setErrorReporting(ErrorReportingCCppEnum value);
};

class CAndCppToolWidget : public VcNodeWidget
{
public:
    explicit CAndCppToolWidget(CAndCppTool::Ptr tool);
    ~CAndCppToolWidget();
    void saveData();

private:
    QWidget* createGeneralWidget();
    QWidget* createOptimizationWidget();
    QWidget* createPreprocessorWidget();
    QWidget* createCodeGenerationWidget();
    QWidget* createLanguageWidget();
    QWidget* createPrecompiledHeadersWidget();
    QWidget* createOutputFilesWidget();
    QWidget* createBrowseInformationWidget();
    QWidget* createAdvancedWidget();

    void saveGeneralData();
    void saveOptimizationData();
    void savePreprocessorData();
    void saveCodeGenerationData();
    void saveLanguageData();
    void savePrecompiledHeadersData();
    void saveOutputFilesData();
    void saveBrowseInformationData();
    void saveAdvancedData();

    CAndCppTool::Ptr m_tool;

    // General
    LineEdit *m_additIncDirsLineEdit;
    LineEdit *m_resolveRefsLineEdit;
    QComboBox *m_dbgInfoFormatComboBox;
    QComboBox *m_suppresStBannerComboBox;
    QComboBox *m_warningLevelComboBox;
    QComboBox *m_detect64BitPortIssuesComboBox;
    QComboBox *m_threatWarnAsErrComboBox;
    QComboBox *m_useUnicodeRespFilesComboBox;

    // Optimization
    QComboBox *m_optimizationComboBox;
    QComboBox *m_inlineFuncExpComboBox;
    QComboBox *m_enableInstrFuncComboBox;
    QComboBox *m_favorSizeorSpeedComboBox;
    QComboBox *m_omitFramePtrComboBox;
    QComboBox *m_enableFiberSafeOptmComboBox;
    QComboBox *m_wholeProgOptmComboBox;

    // Preprocessor
    LineEdit *m_preProcDefLineEdit;
    QComboBox *m_ignoreStdIncPathComboBox;
    QComboBox *m_generPreProcFileComboBox;
    QComboBox *m_keepCommentsComboBox;

    // Code Generation
    QComboBox *m_stringPoolingComboBox;
    QComboBox *m_minimalRebuildComboBox;
    QComboBox *m_exceptionHandlComboBox;
    QComboBox *m_smallerTypeCheckComboBox;
    QComboBox *m_basicRuntimeChecksComboBox;
    QComboBox *m_runtimeLibComboBox;
    QComboBox *m_structMemberAlignComboBox;
    QComboBox *m_bufferSecCheckComboBox;
    QComboBox *m_enableFuncLvlLinkComboBox;
    QComboBox *m_enableEnhInstrSetComboBox;
    QComboBox *m_floatPointModelComboBox;
    QComboBox *m_floatPointExcpComboBox;

    // Language
    QComboBox *m_disLangExtComboBox;
    QComboBox *m_defCharUnsignedComboBox;
    QComboBox *m_treatWcharasBuiltInComboBox;
    QComboBox *m_forceConformanceComboBox;
    QComboBox *m_runTimeTypeComboBox;
    QComboBox *m_openMPComboBox;

    // Precompiled
    QComboBox *m_usePrecomHeadComboBox;
    QLineEdit *m_precomHeadThroughLineEdit;
    QLineEdit *m_precomHeaderFileLineEdit;

    // Output Files
    QComboBox *m_expandAttrSrcComboBox;
    QComboBox *m_assemblerOutputComboBox;
    QLineEdit *m_asmListLocLineEdit;
    QLineEdit *m_objectFileLineEdit;
    QLineEdit *m_progDataLineEdit;
    QComboBox *m_genXMLDocComboBox;
    QLineEdit *m_xmlDocFileNameLineEdit;

    // Browse Information
    QComboBox *m_browseInfoComboBox;
    QLineEdit *m_browseInfoFileLineEdit;

    // Advanced
    QComboBox *m_callConvComboBox;
    QComboBox *m_compileAsComboBox;
    LineEdit *m_disableSpecWarningLineEdit;
    LineEdit *m_forcedIncFilesLineEdit;
    LineEdit *m_forceUsingFilesLineEdit;
    QComboBox *m_showIncComboBox;
    LineEdit *m_undefPreprocDefLineEdit;
    QComboBox *m_undefAllPreprocDefComboBox;
    QComboBox *m_useFullPathsComboBox;
    QComboBox *m_omitDefLibNameComboBox;
    QComboBox *m_errReportComboBox;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_CANDCPPTOOL_H
