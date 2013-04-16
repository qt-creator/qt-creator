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
#ifndef VCPROJECTMANAGER_INTERNAL_CANDCPPTOOL_CONSTANTS_H
#define VCPROJECTMANAGER_INTERNAL_CANDCPPTOOL_CONSTANTS_H

namespace VcProjectManager {
namespace Internal {
namespace CAndCppConstants {

enum DebugInformationFormatEnum
{
    DIF_Disabled = 0,
    DIF_C7_Compatible = 1,
    DIF_Program_Database = 3,
    DIF_Program_Database_For_Edit_And_Continue = 4
};

enum WarningLevelEnum
{
    WL_Turn_Off_All = 0,
    WL_Level_1 = 1,
    WL_Level_2 = 2,
    WL_Level_3 = 3,
    WL_Level_4 = 4
};

enum OptimizationEnum
{
    OP_Disabled  = 0,
    OP_Minimize_Size = 1,
    OP_Maximize_Speed = 2,
    OP_Full_Optimization = 3,
    OP_Custom = 4
};

enum InlineFunctionExpansionEnum
{
    IFE_Default = 0,
    IFE_Only__inline = 1,
    IFE_Any_Suitable = 2
};

enum FavorSizeOrSpeedEnum
{
    FSS_Neither = 0,
    FSS_Favor_Fast_Code = 1,
    FSS_Favor_Small_Code = 2
};

enum GeneratePreprocessedFileEnum
{
    GPF_Dont_Generate = 0,
    GPF_With_Line_Numbers = 1,
    GPF_Without_Line_Numbers = 2
};

enum ExceptionHandlingEnum
{
    EH_No = 0,
    EH_Handle_Exception = 1,
    EH_Handle_Exception_With_SEH_Exceptions = 2
};

enum BasicRuntimeChecksEnum
{
    BRC_Default = 0,
    BRC_Stack_Frames = 1,
    BRC_Uninitialized_Variables = 2,
    BRC_Both = 3
};

enum RuntimeLibraryEnum
{
    RL_Multi_threaded = 0,
    RL_Multi_threaded_Debug = 1,
    RL_Multi_threaded_DLL = 2,
    RL_Multi_threaded_Debug_DLL = 3
};

enum StructMemberAlignmentEnum
{
    SMA_Default = 0,
    SMA_One_Byte = 1,
    SMA_Two_Bytes = 2,
    SMA_Four_Bytes = 3,
    SMA_Eight_Bytes = 4,
    SMA_Sixteen_Bytes = 5
};

enum EnableEnhancedInstructionSetEnum
{
    EEI_Not_Set = 0,
    EEI_Streaming_SIMD_Extensions = 1,
    EEI_Streaming_SIMD_Extensions_2 = 2
};

enum FloatingPointModelEnum
{
    FPM_Precise = 0,
    FPM_Strict = 1,
    FPM_Fast = 2
};

enum UsePrecompiledHeaderEnum
{
    UPH_Not_Using_Precompiled_Headers = 0,
    UPH_Create_Precompiled_Header = 1,
    UPH_Use_Precompiled_Header  = 2
};

enum AssemblerOutputEnum
{
   AO_No_Listing = 0,
   AO_Assembly_Only_Listing = 1,
   AO_Assembly_Machine_Code_And_Source = 2,
   AO_Assembly_With_Machine_Code = 3,
   AO_Assembly_With_Source_Code = 4

};

enum BrowseInformationEnum
{
    BI_None = 0,
    BI_Include_All_Browse_Information = 1,
    BI_No_Local_Symbols = 2
};

enum CallingConventionEnum
{
    CC_Cdecl = 0,
    CC_Fastcall = 1,
    CC_Stdcall = 2
};

enum CompileAsEnum
{
    CA_Default = 0,
    CA_Compile_as_C_Code = 1,
    CA_Compile_as_Cpp_Code = 2
};

enum ErrorReportingCCppEnum
{
    ERCC_Default = 0,
    ERCC_Prompt_Immediately = 1,
    ERCC_Queue_For_Next_Login = 2
};

} // namespace CAndCppConstants
} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_CANDCPPTOOL_CONSTANTS_H
