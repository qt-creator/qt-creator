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
#ifndef VCPROJECTMANAGER_INTERNAL_LINKER_CONSTANTS_H
#define VCPROJECTMANAGER_INTERNAL_LINKER_CONSTANTS_H

namespace VcProjectManager {
namespace Internal {
namespace LinkerConstants {

enum ShowProgressEnum
{
    SP_Not_Set = 0,
    SP_Display_All_Progress_Messages = 1,
    SP_Displays_Some_Progress_Messages = 2
};

enum LinkIncrementalEnum
{
    LI_Default = 0,
    LI_No = 1,
    LI_Yes = 2
};

enum UACExecutionLevelEnum
{
    UACEL_AsInvoker = 0,
    UACEL_HighestAvailable = 1,
    UACEL_RequireAdministrator = 2
};

enum AssemblyDebugEnum
{
    AD_No_Debuggable_Attribute_Emitted = 0,
    AD_Runtime_Tracking_And_Disable_Optimizations = 1,
    AD_No_Runtime_Tracking_And_Enable_Optimizations = 2
};

enum SubSystemEnum
{
    SS_Not_Set = 0,
    SS_Console = 1,
    SS_Windows = 2,
    SS_Native = 3,
    SS_EFI_Application = 4,
    SS_EFI_Boot_Service_Driver = 5,
    SS_EFI_ROM = 6,
    SS_EFI_Runtime = 7,
    SS_WindowsCE = 8
};

enum LargeAddressAwareEnum
{
    LAA_Default = 0,
    LAA_Do_Not_Support_Addresses_Larger_Than_Two_Gigabytes = 1,
    LAA_Support_Addresses_Larger_Than_Two_Gigabytes = 2
};

enum TerminalServerAwareEnum
{
    TSA_Default = 0,
    TSA_Not_Terminal_Server_Aware = 1,
    TSA_Application_Terminal_Server_Aware = 2
};

enum DriverEnum
{
    DR_Not_Set = 0,
    DR_Driver  = 1,
    DR_Up_Only = 2,
    DR_WDM = 3
};

enum OptimizeReferencesEnum
{
    OR_Default = 0,
    OR_Keep_Unreferenced_Data = 1,
    OR_Eliminate_Unreferenced_Data = 2

};

enum EnableCOMDATFoldingEnum
{
    ECF_Default = 0,
    ECF_Do_Not_Remove_Redundant_COMDATs = 1,
    ECF_Remove_Redundant_COMDATs = 2
};

enum OptimizeForWindows98Enum
{
    OFW98_No = 0,
    OFW98_No_OPT_WIN98 = 1,
    OFW98_Yes = 2

};

enum LinkTimeCodeGenerationEnum
{
    LTCG_Default = 0,
    LTCG_Use_Link_Time_Code_Generation = 1,
    LTCG_Profile_Guided_Optimization_Optimize = 2,
    LTCG_Profile_Guided_Optimization_Update = 3
};

enum RandomizedBaseAddressEnum
{
    RBA_Default = 0,
    RBA_Disable_Image_Randomization = 1,
    RBA_Enable_Image_Randomization = 2
};

enum FixedBaseAddressEnum
{
    FBA_Default = 0,
    FBA_Generate_A_Relocation_Section = 1,
    FBA_Image_Must_Be_Loaded_At_A_Fixed_Address = 2
};

enum DataExecutionPreventionEnum
{
    DEP_Default = 0,
    DEP_Image_Is_Not_Compatible_With_DEP = 1,
    DEP_Image_Is_Compatible_With_DEP = 2
};

enum TargetMachineEnum
{
    TM_Not_Set = 0,
    TM_MachineX86 = 1,
    TM_MachineAM33 = 2,
    TM_MachineARM = 3,
    TM_MachineEBC = 4,
    TM_MachineIA64 = 5,
    TM_MachineM32R = 6,
    TM_MachineMIPS = 7,
    TM_MachineMIPS16 = 8,
    TM_MachineMIPSFPU = 9,
    TM_MachineMIPSFPU16 = 10,
    TM_MachineMIPSR41XX = 11,
    TM_MachineSH3 = 12,
    TM_MachineSH3DSP = 13,
    TM_MachineSH4 = 14,
    TM_MachineSH5 = 15,
    TM_MachineTHUMB = 16,
    TM_MachineX64 = 17
};

enum CLRThreadAttributeEnum
{
    CLRTA_No_Threading_Attribute_Set = 0,
    CLRTA_MTA_Threading_Attribute = 1,
    CLRTA_STA_Threading_Attribute = 2
};

enum CLRImageTypeEnum
{
    CLRIT_Default_Image_Type = 0,
    CLRIT_Force_IJW_Image = 1,
    CLRIT_Force_Pure_IL_Image = 2,
    CLRIT_Force_Safe_IL_Image  = 3
};

enum ErrorReportingLinkingEnum
{
    ERL_Default = 0,
    ERL_Prompt_Immediately = 1,
    ERL_Queue_For_Next_Login = 2
};

} // namespace LinkerConstants
} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_LINKER_CONSTANTS_H
