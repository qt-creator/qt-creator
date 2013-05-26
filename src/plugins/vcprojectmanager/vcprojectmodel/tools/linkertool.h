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
#ifndef VCPROJECTMANAGER_INTERNAL_LINKERTOOL_H
#define VCPROJECTMANAGER_INTERNAL_LINKERTOOL_H

#include "linker_constants.h"
#include "tool.h"
#include "../../widgets/vcnodewidget.h"

class QLineEdit;
class QComboBox;

namespace VcProjectManager {
namespace Internal {

using namespace LinkerConstants;

class LineEdit;

class LinkerTool : public Tool
{
public:
    typedef QSharedPointer<LinkerTool>  Ptr;

    LinkerTool();
    LinkerTool(const LinkerTool &tool);
    ~LinkerTool();

    QString nodeWidgetName() const;
    VcNodeWidget* createSettingsWidget();
    Tool::Ptr clone() const;

    // General
    QString outputFile() const;
    QString outputFileDefault() const;
    void setOutputFile(const QString &file);
    ShowProgressEnum showProgress() const;
    ShowProgressEnum showProgressDefault() const;
    void setShowProgress(ShowProgressEnum show);
    QString version() const;
    QString versionDefault() const;
    void setVersion(const QString &version);
    LinkIncrementalEnum linkIncremental() const;
    LinkIncrementalEnum linkIncrementalDefault() const;
    void setLinkIncremental(LinkIncrementalEnum value);
    bool suppressStartupBanner() const;
    bool suppressStartupBannerDefault() const;
    void setSuppressStartupBanner(bool suppress);
    bool ignoreImportLibrary() const;
    bool ignoreImportLibraryDefault() const;
    void setIgnoreImportLibrary(bool ignore);
    bool registerOutput() const;
    bool registerOutputDefault() const;
    void setRegisterOutput(bool regis);
    bool perUserRedirection() const;
    bool perUserRedirectionDefault() const;
    void setPerUserRedirection(bool enable);
    QStringList additionalLibraryDirectories() const;
    QStringList additionalLibraryDirectoriesDefault() const;
    void setAdditionalLibraryDirectories(const QStringList &dirs);
    bool linkLibraryDependencies() const;
    bool linkLibraryDependenciesDefault() const;
    void setLinkLibraryDependencies(bool enable);
    bool useLibraryDependencyInputs() const;
    bool useLibraryDependencyInputsDefault() const;
    void setUseLibraryDependencyInputs(bool use);
    bool useUnicodeResponseFiles() const;
    bool useUnicodeResponseFilesDefault() const;
    void setUseUnicodeResponseFiles(bool use);

    // Input
    QStringList additionalDependencies() const;
    QStringList additionalDependenciesDefault() const;
    void setAdditionalDependencies(const QStringList &dependencies);
    bool ignoreAllDefaultLibraries() const;
    bool ignoreAllDefaultLibrariesDefault() const;
    void setIgnoreAllDefaultLibraries(bool ignore);
    QStringList ignoreDefaultLibraryNames() const;
    QStringList ignoreDefaultLibraryNamesDefault() const;
    void setIgnoreDefaultLibraryNames(const QStringList &libNames);
    QString moduleDefinitionFile() const;
    QString moduleDefinitionFileDefault() const;
    void setModuleDefinitionFile(const QString &file);
    QStringList addModuleNamesToAssembly() const;
    QStringList addModuleNamesToAssemblyDefault() const;
    void setAddModuleNamesToAssembly(const QStringList &moduleNames);
    QStringList embedManagedResourceFile() const;
    QStringList embedManagedResourceFileDefault() const;
    void setEmbedManagedResourceFile(const QStringList &resFiles);
    QStringList forceSymbolReferences() const;
    QStringList forceSymbolReferencesDefault() const;
    void setForceSymbolReferences(const QStringList &refs);
    QStringList delayLoadDLLs() const;
    QStringList delayLoadDLLsDefault() const;
    void setDelayLoadDLLs(const QStringList &dlls);
    QStringList assemblyLinkResource() const;
    QStringList assemblyLinkResourceDefault() const;
    void setAssemblyLinkResource(const QStringList &res);

    // Manifest File
    bool generateManifest() const;
    bool generateManifestDefault() const;
    void setGenerateManifest(bool generate);
    QString manifestFile() const;
    QString manifestFileDefault() const;
    void setManifestFile(const QString &file);
    QStringList additionalManifestDependencies() const;
    QStringList additionalManifestDependenciesDefault() const;
    void setAdditionalManifestDependencies(const QStringList &dependencies);
    bool allowIsolation() const;
    bool allowIsolationDefault() const;
    void setAllowIsolation(bool allow);
    bool enableUAC() const;
    bool enableUACDefault() const;
    void setEnableUAC(bool enable);
    UACExecutionLevelEnum uacExecutionLevel() const;
    UACExecutionLevelEnum uacExecutionLevelDefault() const;
    void setUACExecutionLevel(UACExecutionLevelEnum value);
    bool uacUIAAccess() const;
    bool uacUIAAccessDefault() const;
    void setUACUIAccess(bool enable);

    // Debugging
    bool generateDebugInformation() const;
    bool generateDebugInformationDefault() const;
    void setGenerateDebugInformation(bool generate);
    QString programDatabaseFile() const;
    QString programDatabaseFileDefault() const;
    void setProgramDatabaseFile(const QString &file);
    QString stripPrivateSymbols() const;
    QString stripPrivateSymbolsDefault() const;
    void setStripPrivateSymbols(const QString &symbols);
    bool generateMapFile() const;
    bool generateMapFileDefault() const;
    void setGenerateMapFile(bool generate);
    QString mapFileName() const;
    QString mapFileNameDefault() const;
    void setMapFileName(const QString &fileName);
    bool mapExports() const;
    bool mapExportsDefault() const;
    void setMapExports(bool enable);
    AssemblyDebugEnum assemblyDebug() const;
    AssemblyDebugEnum assemblyDebugDefault() const;
    void setAssemblyDebug(AssemblyDebugEnum value);

    // System
    SubSystemEnum subSystem() const;
    SubSystemEnum subSystemDefault() const;
    void setSubSystem(SubSystemEnum value);
    int heapReserveSize() const;
    int heapReserveSizeDefault() const;
    void setHeapReserveSize(int size);
    int heapCommitSize() const;
    int heapCommitSizeDefault() const;
    void setHeapCommitSize(int size);
    int stackReserveSize() const;
    int stackReserveSizeDefault() const;
    void setStackReserveSize(int size);
    int stackCommitSize() const;
    int stackCommitSizeDefault() const;
    void setStackCommitSize(int size);
    LargeAddressAwareEnum largeAddressAware() const;
    LargeAddressAwareEnum largeAddressAwareDefault() const;
    void setLargeAddressAware(LargeAddressAwareEnum value);
    TerminalServerAwareEnum terminalServerAware() const;
    TerminalServerAwareEnum terminalServerAwareDefault() const;
    void setTerminalServerAware(TerminalServerAwareEnum value);
    bool swapRunFromCD() const;
    bool swapRunFromCDDefault() const;
    void setSwapRunFromCD(bool enable);
    bool swapRunFromNet() const;
    bool swapRunFromNetDefault() const;
    void setSwapRunFromNet(bool swap);
    DriverEnum driver() const;
    DriverEnum driverDefault() const;
    void setDriver(DriverEnum value);

    // Optimization
    OptimizeReferencesEnum optimizeReferences() const;
    OptimizeReferencesEnum optimizeReferencesDefault() const;
    void setOptimizeReferences(OptimizeReferencesEnum value);
    EnableCOMDATFoldingEnum enableCOMDATFolding() const;
    EnableCOMDATFoldingEnum enableCOMDATFoldingDefault() const;
    void setEnableCOMDATFolding(EnableCOMDATFoldingEnum value);
    OptimizeForWindows98Enum optimizeForWindows98() const;
    OptimizeForWindows98Enum optimizeForWindows98Default() const;
    void setOptimizeForWindows98(OptimizeForWindows98Enum value);
    QString functionOrder() const;
    QString functionOrderDefault() const;
    void setFunctionOrder(const QString &order);
    QString profileGuidedDatabase() const;
    QString profileGuidedDatabaseDefault() const;
    void setProfileGuidedDatabase(const QString &database);
    LinkTimeCodeGenerationEnum linkTimeCodeGeneration() const;
    LinkTimeCodeGenerationEnum linkTimeCodeGenerationDefault() const;
    void setLinkTimeCodeGeneration(LinkTimeCodeGenerationEnum value);

    // Embedded IDL
    QStringList midlCommandFile() const;
    QStringList midlCommandFileDefault() const;
    void setMidlCommandFile(const QStringList &values);
    bool ignoreEmbeddedIDL() const;
    bool ignoreEmbeddedIDLDefault() const;
    void setIgnoreEmbeddedIDL(bool ignore);
    QString mergedIDLBaseFileName() const;
    QString mergedIDLBaseFileNameDefault() const;
    void setMergedIDLBaseFileName(const QString &fileName);
    QString typeLibraryFile() const;
    QString typeLibraryFileDefault() const;
    void setTypeLibraryFile(const QString &libFile);
    int typeLibraryResourceID() const;
    int typeLibraryResourceIDDefault() const;
    void setTypeLibraryResourceID(int id);

    // Advanced
    QString entryPointSymbol() const;
    QString entryPointSymbolDefault() const;
    void setEntryPointSymbol(const QString &symbol);
    bool resourceOnlyDLL() const;
    bool resourceOnlyDLLDefault() const;
    void setResourceOnlyDLL(bool enable);
    // ----------------- Only in 2008 -------------------- //
    bool checksum() const;
    bool checksumDefault() const;
    void setChecksum(bool enable);
    // --------------------------------------------------- //
    QString baseAddress() const;
    QString baseAddressDefault() const;
    void setBaseAddress(const QString &address);
    RandomizedBaseAddressEnum randomizedBaseAddress() const;
    RandomizedBaseAddressEnum randomizedBaseAddressDefault() const;
    void setRandomizedBaseAddress(RandomizedBaseAddressEnum value);
    FixedBaseAddressEnum fixedBaseAddress() const;
    FixedBaseAddressEnum fixedBaseAddressDefault() const;
    void setFixedBaseAddress(FixedBaseAddressEnum value);
    DataExecutionPreventionEnum dataExecutionPrevention() const;
    DataExecutionPreventionEnum dataExecutionPreventionDefault() const;
    void setDataExecutionPrevention(DataExecutionPreventionEnum value);
    bool turnOffAssemblyGeneration() const;
    bool turnOffAssemblyGenerationDefault() const;
    void setTurnOffAssemblyGeneration(bool turnOff);
    bool supportUnloadOfDelayLoadedDLL() const;
    bool supportUnloadOfDelayLoadedDLLDefault() const;
    void setSupportUnloadOfDelayLoadedDLL(bool support);
    QString importLibrary() const;
    QString importLibraryDefault() const;
    void setImportLibrary(const QString &lib);
    QString mergeSections() const;
    QString mergeSectionsDefault() const;
    void setMergeSections(const QString &sections);
    TargetMachineEnum targetMachine() const;
    TargetMachineEnum targetMachineDefault() const;
    void setTargetMachine(TargetMachineEnum value);
    bool profile() const;
    bool profileDefault() const;
    void setProfile(bool enable);
    CLRThreadAttributeEnum clrThreadAttribute() const;
    CLRThreadAttributeEnum clrThreadAttributeDefault() const;
    void setCLRThreadAttribute(CLRThreadAttributeEnum value);
    CLRImageTypeEnum clrImageType() const;
    CLRImageTypeEnum clrImageTypeDefault() const;
    void setCLRImageType(CLRImageTypeEnum value);
    QString keyFile() const;
    QString keyFileDefault() const;
    void setKeyFile(const QString &file);
    QString keyContainer() const;
    QString keyContainerDefault() const;
    void setKeyContainer(const QString &keyContainer);
    bool delaySign() const;
    bool delaySignDefault() const;
    void setDelaySign(bool delay);
    ErrorReportingLinkingEnum errorReporting() const;
    ErrorReportingLinkingEnum errorReportingDefault() const;
    void setErrorReporting(ErrorReportingLinkingEnum value);
    bool clrUnmanagedCodeCheck() const;
    bool clrUnmanagedCodeCheckDefault() const;
    void setCLRUnmanagedCodeCheck(bool unmanaged);
};

class LinkerToolWidget : public VcNodeWidget
{
public:
    explicit LinkerToolWidget(LinkerTool* tool);
    ~LinkerToolWidget();
    void saveData();

private:
    QWidget* createGeneralWidget();
    QWidget* createInputWidget();
    QWidget* createManifestFileWidget();
    QWidget* createDebuggingWidget();
    QWidget* createSystemWidget();
    QWidget* createOptimizationWidget();
    QWidget* createEmbeddedIDLWidget();
    QWidget* createAdvancedWidget();

    void saveGeneralData();
    void saveInputData();
    void saveManifestFileData();
    void saveDebuggingData();
    void saveSystemData();
    void saveOptimizationData();
    void saveEnbeddedIDLData();
    void saveAdvancedData();

    LinkerTool* m_tool;

    // General
    QLineEdit *m_outputFileLineEdit;
    QComboBox *m_showProgComboBox;
    QLineEdit *m_versionLineEdit;
    QComboBox *m_linkIncComboBox;
    QComboBox *m_suppStartBannerComboBox;
    QComboBox *m_ignoreImpLibComboBox;
    QComboBox *m_regOutputComboBox;
    QComboBox *m_perUserRedComboBox;
    LineEdit *m_additLibDirLineEdit;
    QComboBox *m_linkLibDepComboBox;
    QComboBox *m_useLibDepInputsComboBox;
    QComboBox *m_useUnicodeRespFilesComboBox;

    // Input
    LineEdit *m_addDepLineEdit;
    QComboBox *m_ignoreAllDefLibComboBox;
    LineEdit *m_ignoreDefLibNamesLineEdit;
    QLineEdit *m_moduleDefFileLineEdit;
    LineEdit *m_addModuleNamesToASMLineEdit;
    LineEdit *m_embedManagedResFileLineEdit;
    LineEdit *m_forceSymRefLineEdit;
    LineEdit *m_delayLoadDLLsLineEdit;
    LineEdit *m_asmLinkResLineEdit;

    // Manifest
    QComboBox *m_generManifestComboBox;
    QLineEdit *m_manifestFileLineEdit;
    LineEdit *m_additManifestDepLineEdit;
    QComboBox *m_allowIsolationComboBox;
    QComboBox *m_enableUacComboBox;
    QComboBox *m_uacExecLvlComboBox;
    QComboBox *m_uacUIAccComboBox;

    // Debugging
    QComboBox *m_genDebugInfoComboBox;
    QLineEdit *m_progDatabFileLineEdit;
    QLineEdit *m_stripPrivSymbLineEdit;
    QComboBox *m_genMapFileComboBox;
    QLineEdit *m_mapFileNameLineEdit;
    QComboBox *m_mapExportsComboBox;
    QComboBox *m_asmDebugComboBox;

    // System
    QComboBox *m_subSystemComboBox;
    QLineEdit *m_heapResSizeLineEdit;
    QLineEdit *m_heapCommitSizeLineEdit;
    QLineEdit *m_stackReserveSizeLineEdit;
    QLineEdit *m_stackCommitSize;
    QComboBox *m_largeAddressAwareComboBox;
    QComboBox *m_terminalServerAwareComboBox;
    QComboBox *m_swapFromCDComboBox;
    QComboBox *m_swapFromNetComboBox;
    QComboBox *m_driverComboBox;

    // Optimization
    QComboBox *m_optmRefComboBox;
    QComboBox *m_enableCOMDATFoldComboBox;
    QComboBox *m_optmForWin98ComboBox;
    QLineEdit *m_funcOrderLineEdit;
    QLineEdit *m_profGuidedDatabLineEdit;
    QComboBox *m_linkTimeCodeGenComboBox;

    // Embedded IDL
    LineEdit *m_midlCommFileLineEdit;
    QComboBox *m_ignoreEmbedIDLComboBox;
    QLineEdit *m_mergedIDLLineEdit;
    QLineEdit *m_typeLibFileLineEdit;
    QLineEdit *m_typeLibResIDLineEdit;

    // Advanced
    QLineEdit *m_entryPointSym;
    QComboBox *m_resourceOnlyDLLComboBox;
    QComboBox *m_setchecksumComboBox;
    QLineEdit *m_baseAddressLineEdit;
    QComboBox *m_randBaseAddressComboBox;
    QComboBox *m_fixedBaseAddressComboBox;
    QComboBox *m_dataExecPrevComboBox;
    QComboBox *m_turnOffAsmGenComboBox;
    QComboBox *m_suppUnloadOfDelayDLLComboBox;
    QLineEdit *m_importLibLineEdit;
    QLineEdit *m_mergeSectionsLineEdit;
    QComboBox *m_targetMachineComboBox;
    QComboBox *m_profileComboBox;
    QComboBox *m_clrThreadAttrComboBox;
    QComboBox *m_clrImageTypeComboBox;
    QLineEdit *m_keyFileLineEdit;
    QLineEdit *m_keyContainerLineEdit;
    QComboBox *m_delaySignComboBox;
    QComboBox *m_errReportComboBox;
    QComboBox *m_clrUnmanagedCodeCheckComboBox;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_LINKERTOOL_H
