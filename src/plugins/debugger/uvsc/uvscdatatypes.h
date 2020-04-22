/****************************************************************************
**
** Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <qglobal.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UV3_SOCKIF_VERS 229
#define SOCK_NDATA 32768

// Socket commands.
enum UV_OPERATION {
    // General commands:
    UV_NULL_CMD = 0x0000,
    UV_GEN_GET_VERSION = 0x0001,
    UV_GEN_UI_UNLOCK = 0x0002,
    UV_GEN_UI_LOCK = 0x0003,
    UV_GEN_HIDE = 0x0004,
    UV_GEN_SHOW = 0x0005,
    UV_GEN_RESTORE = 0x0006,
    UV_GEN_MINIMIZE = 0x0007,
    UV_GEN_MAXIMIZE = 0x0008,
    UV_GEN_EXIT = 0x0009,
    UV_GEN_GET_EXTVERSION = 0x000A,
    UV_GEN_CHECK_LICENSE = 0x000B,
    UV_GEN_CPLX_COMPLETE = 0x000C,
    UV_GEN_SET_OPTIONS = 0x000D,
    // Project commands:
    UV_PRJ_LOAD = 0x1000,
    UV_PRJ_CLOSE = 0x1001,
    UV_PRJ_ADD_GROUP = 0x1002,
    UV_PRJ_DEL_GROUP = 0x1003,
    UV_PRJ_ADD_FILE = 0x1004,
    UV_PRJ_DEL_FILE = 0x1005,
    UV_PRJ_BUILD = 0x1006,
    UV_PRJ_REBUILD = 0x1007,
    UV_PRJ_CLEAN = 0x1008,
    UV_PRJ_BUILD_CANCEL = 0x1009,
    UV_PRJ_FLASH_DOWNLOAD = 0x100A,
    UV_PRJ_GET_DEBUG_TARGET = 0x100B,
    UV_PRJ_SET_DEBUG_TARGET = 0x100C,
    UV_PRJ_GET_OPTITEM = 0x100D,
    UV_PRJ_SET_OPTITEM = 0x100E,
    UV_PRJ_ENUM_GROUPS = 0x100F,
    UV_PRJ_ENUM_FILES = 0x1010,
    UV_PRJ_CMD_PROGRESS = 0x1011,
    UV_PRJ_ACTIVE_FILES = 0x1012,
    UV_PRJ_FLASH_ERASE = 0x1013,
    UV_PRJ_GET_OUTPUTNAME = 0x1014,
    UV_PRJ_ENUM_TARGETS = 0x1015,
    UV_PRJ_SET_TARGET = 0x1016,
    UV_PRJ_GET_CUR_TARGET = 0x1017,
    UV_PRJ_SET_OUTPUTNAME = 0x1018,
    // Debug commands:
    UV_DBG_ENTER = 0x2000,
    UV_DBG_EXIT = 0x2001,
    UV_DBG_START_EXECUTION = 0x2002,
    UV_DBG_RUN_TO_ADDRESS = 0x2102,
    UV_DBG_STOP_EXECUTION = 0x2003,
    UV_DBG_STATUS = 0x2004,
    UV_DBG_RESET = 0x2005,
    UV_DBG_STEP_HLL = 0x2006,
    UV_DBG_STEP_HLL_N = 0x2106,
    UV_DBG_STEP_INTO = 0x2007,
    UV_DBG_STEP_INTO_N = 0x2107,
    UV_DBG_STEP_INSTRUCTION = 0x2008,
    UV_DBG_STEP_INSTRUCTION_N = 0x2108,
    UV_DBG_STEP_OUT = 0x2009,
    UV_DBG_CALC_EXPRESSION = 0x200A,
    UV_DBG_MEM_READ = 0x200B,
    UV_DBG_MEM_WRITE = 0x200C,
    UV_DBG_TIME_INFO = 0x200D,
    UV_DBG_SET_CALLBACK = 0x200E,
    UV_DBG_VTR_GET = 0x200F,
    UV_DBG_VTR_SET = 0x2010,
    UV_DBG_SERIAL_GET = 0x2011,
    UV_DBG_SERIAL_PUT = 0x2012,
    UV_DBG_VERIFY_CODE = 0x2013,
    UV_DBG_CREATE_BP = 0x2014,
    UV_DBG_ENUMERATE_BP = 0x2015,
    UV_DBG_CHANGE_BP = 0x2016,
    UV_DBG_ENUM_SYMTP = 0x2017,
    UV_DBG_ADR_TOFILELINE = 0x2018,
    UV_DBG_ENUM_STACK = 0x2019,
    UV_DBG_ENUM_VTR = 0x201A,
    UV_DBG_UNUSED = 0x201B,
    UV_DBG_ADR_SHOWCODE = 0x201C,
    UV_DBG_WAKE = 0x201D,
    UV_DBG_SLEEP = 0x201E,
    UV_MSGBOX_MSG = 0x201F,
    UV_DBG_EXEC_CMD = 0x2020,
    UV_DBG_POWERSCALE_SHOWCODE = 0x2021,
    UV_DBG_POWERSCALE_SHOWPOWER = 0x2022,
    POWERSCALE_OPEN = 0x2023,
    UV_DBG_EVAL_EXPRESSION_TO_STR = 0x2024,
    UV_DBG_FILELINE_TO_ADR = 0x2025,
    // Registers commands:
    UV_DBG_ENUM_REGISTER_GROUPS = 0x2026,
    UV_DBG_ENUM_REGISTERS = 0x2027,
    UV_DBG_READ_REGISTERS = 0x2028,
    UV_DBG_REGISTER_SET = 0x2029,
    UV_DBG_DSM_READ = 0x202A,
    UV_DBG_EVAL_WATCH_EXPRESSION = 0x202B,
    UV_DBG_REMOVE_WATCH_EXPRESSION = 0x202D,
    UV_DBG_ENUM_VARIABLES = 0x202E,
    UV_DBG_VARIABLE_SET = 0x202F,
    UV_DBG_ENUM_TASKS = 0x2030,
    UV_DBG_ENUM_MENUS = 0x2031,
    UV_DBG_MENU_EXEC = 0x2032,
    UV_DBG_ITM_REGISTER = 0x2033,
    UV_DBG_ITM_UNREGISTER = 0x2034,
    UV_DBG_EVTR_REGISTER = 0x2035,
    UV_DBG_EVTR_UNREGISTER = 0x2036,
    UV_DBG_EVTR_GETSTATUS = 0x2037,
    UV_DBG_EVTR_ENUMSCVDFILES = 0x2038,
    // Responses/errors from uVision to client:
    UV_CMD_RESPONSE = 0x3000,
    // Asynchronous messages:
    UV_ASYNC_MSG = 0x4000,
    // Project asynchronous messages:
    UV_PRJ_BUILD_COMPLETE = 0x5000,
    UV_PRJ_BUILD_OUTPUT = 0x5001,
    // Debug asynchronous messages:
    UV_DBG_CALLBACK = 0x5002,
    // Response to UV_DBG_ENUMERATE_BP:
    UV_DBG_BP_ENUM_START = 0x5004,
    UV_DBG_BP_ENUMERATED = 0x5005,
    UV_DBG_BP_ENUM_END = 0x5006,
    // Response to UV_PRJ_ENUM_GROUPS:
    UV_PRJ_ENUM_GROUPS_START= 0x5007,
    UV_PRJ_ENUM_GROUPS_ENU = 0x5008,
    UV_PRJ_ENUM_GROUPS_END = 0x5009,
    // Response to UV_PRJ_ENUM_FILES:
    UV_PRJ_ENUM_FILES_START = 0x500A,
    UV_PRJ_ENUM_FILES_ENU = 0x500B,
    UV_PRJ_ENUM_FILES_END = 0x500C,
    // Progress bar functions:
    UV_PRJ_PBAR_INIT = 0x500D,
    UV_PRJ_PBAR_STOP = 0x500E,
    UV_PRJ_PBAR_SET = 0x500F,
    UV_PRJ_PBAR_TEXT = 0x5010,
    // Response to UV_DBG_ENUM_SYMTP:
    UV_DBG_ENUM_SYMTP_START = 0x5011,
    UV_DBG_ENUM_SYMTP_ENU = 0x5012,
    UV_DBG_ENUM_SYMTP_END = 0x5013,
    // Response to UV_DBG_ENUM_STACK:
    UV_DBG_ENUM_STACK_START = 0x5014,
    UV_DBG_ENUM_STACK_ENU = 0x5015,
    UV_DBG_ENUM_STACK_END = 0x5016,
    // Response to UV_DBG_ENUM_VTR:
    UV_DBG_ENUM_VTR_START = 0x5017,
    UV_DBG_ENUM_VTR_ENU = 0x5018,
    UV_DBG_ENUM_VTR_END = 0x5019,
    // Command window output:
    UV_DBG_CMD_OUTPUT = 0x5020,
    // Serial output:
    UV_DBG_SERIAL_OUTPUT = 0x5120,
    // Response to UV_PRJ_ENUM_TARGETS:
    UV_PRJ_ENUM_TARGETS_START= 0x5021,
    UV_PRJ_ENUM_TARGETS_ENU = 0x5022,
    UV_PRJ_ENUM_TARGETS_END = 0x5023,
    // Response to UV_DBG_ENUM_REGISTER_GROUPS:
    UV_DBG_ENUM_REGISTER_GROUPS_START = 0x5024,
    UV_DBG_ENUM_REGISTER_GROUPS_ENU = 0x5025,
    UV_DBG_ENUM_REGISTER_GROUPS_END = 0x5026,
    // Response to UV_DBG_ENUM_REGISTERS:
    UV_DBG_ENUM_REGISTERS_START = 0x5027,
    UV_DBG_ENUM_REGISTERS_ENU = 0x5028,
    UV_DBG_ENUM_REGISTERS_END = 0x5029,
    // Response to UV_DBG_ITM_REGISTER:
    UV_DBG_ITM_OUTPUT = 0x5030,
    // Response to UV_DBG_ENUM_VARIABLES:
    UV_DBG_ENUM_VARIABLES_START = 0x5040,
    UV_DBG_ENUM_VARIABLES_ENU = 0x5041,
    UV_DBG_ENUM_VARIABLES_END = 0x5042,
    // Response to UV_DBG_ENUM_TASKS:
    UV_DBG_ENUM_TASKS_START = 0x5050,
    UV_DBG_ENUM_TASKS_ENU = 0x5051,
    UV_DBG_ENUM_TASKS_END = 0x5052,
    // Response to UV_DBG_ENUM_MENUS:
    UV_DBG_ENUM_MENUS_START = 0x5060,
    UV_DBG_ENUM_MENUS_ENU = 0x5061,
    UV_DBG_ENUM_MENUS_END = 0x5062,
    // Response to UV_DBG_EVTR_REGISTER:
    UV_DBG_EVTR_OUTPUT = 0x5063,
    // Response to UV_DBG_EVTR_ENUMSCVDFILES:
    UV_DBG_EVTR_ENUMSCVDFILES_START = 0x5064,
    UV_DBG_EVTR_ENUMSCVDFILES_ENU = 0x5065,
    UV_DBG_EVTR_ENUMSCVDFILES_END = 0x5066,
    // Real-Time agent:
    UV_RTA_MESSAGE = 0x6000,
    UV_RTA_INCOMPATIBLE = 0x6001,
    // Test definititions (for testing only):
    UV_TST_1 = 0xFF00,
    UV_TST_2 = 0xFF01,
    UV_TST_3 = 0xFF02,
    UV_TST_4 = 0xFF03,
    UV_TST_5 = 0xFF04,
    UV_TST_6 = 0xFF05,
    UV_TST_7 = 0xFF06,
    UV_TST_8 = 0xFF07,
    UV_TST_9 = 0xFF08,
    UV_TST_10 = 0xFF09
};

// Socket status codes.
enum UV_STATUS {
    UV_STATUS_SUCCESS = 0,
    UV_STATUS_FAILED = 1,
    UV_STATUS_NO_PROJECT = 2,
    UV_STATUS_WRITE_PROTECTED = 3,
    UV_STATUS_NO_TARGET = 4,
    UV_STATUS_NO_TOOLSET = 5,
    UV_STATUS_NOT_DEBUGGING = 6,
    UV_STATUS_ALREADY_PRESENT = 7,
    UV_STATUS_INVALID_NAME = 8,
    UV_STATUS_NOT_FOUND = 9,
    UV_STATUS_DEBUGGING = 10,
    UV_STATUS_TARGET_EXECUTING = 11,
    UV_STATUS_TARGET_STOPPED = 12,
    UV_STATUS_PARSE_ERROR = 13,
    UV_STATUS_OUT_OF_RANGE = 14,
    UV_STATUS_BP_CANCELLED = 15,
    UV_STATUS_BP_BADADDRESS = 16,
    UV_STATUS_BP_NOTSUPPORTED = 17,
    UV_STATUS_BP_FAILED = 18,
    UV_STATUS_BP_REDEFINED = 19,
    UV_STATUS_BP_DISABLED = 20,
    UV_STATUS_BP_ENABLED = 21,
    UV_STATUS_BP_CREATED = 22,
    UV_STATUS_BP_DELETED = 23,
    UV_STATUS_BP_NOTFOUND = 24,
    UV_STATUS_BUILD_OK_WARNINGS = 25,
    UV_STATUS_BUILD_FAILED = 26,
    UV_STATUS_BUILD_CANCELLED = 27,
    UV_STATUS_NOT_SUPPORTED = 28,
    UV_STATUS_TIMEOUT = 29,
    UV_STATUS_UNEXPECTED_MSG = 30,
    UV_STATUS_VERIFY_FAILED = 31,
    UV_STATUS_NO_ADRMAP = 32,
    UV_STATUS_INFO = 33,
    UV_STATUS_NO_MEM_ACCESS = 34,
    UV_STATUS_FLASH_DOWNLOAD = 35,
    UV_STATUS_BUILDING = 36,
    UV_STATUS_HARDWARE = 37,
    UV_STATUS_SIMULATOR = 38,
    UV_STATUS_BUFFER_TOO_SMALL = 39,
    UV_STATUS_EVTR_FAILED = 40,
    UV_STATUS_END
};

// Variant-types used in #TVAL.
enum VTT_TYPE {
    VTT_void = 0,
    VTT_bit = 1,
    VTT_int8 = 2,
    VTT_uint8 = 3,
    VTT_int = 4,
    VTT_uint = 5,
    VTT_short = 6,
    VTT_ushort = 7,
    VTT_long = 8,
    VTT_ulong = 9,
    VTT_float = 10,
    VTT_double = 11,
    VTT_ptr = 12,
    VTT_union = 13,
    VTT_struct = 14,
    VTT_func = 15,
    VTT_string = 16,
    VTT_enum = 17,
    VTT_field = 18,
    VTT_int64 = 19,
    VTT_uint64 = 20,
    VTT_end
};

// View types.
enum UVMENUTYPES {
    UVMENU_DEBUG = 0,
    UVMENU_SYS_VIEW = 1,
    UVMENU_PERI = 2,
    UVMENU_RTX = 3,
    UVMENU_CAN = 4,
    UVMENU_AGDI = 5,
    UVMENU_TOOLBOX = 6,
    UVMENU_END
};

// Stop reason.
enum STOPREASON {
    STOPREASON_UNDEFINED = 0x0000,
    STOPREASON_EXEC = 0x0001,
    STOPREASON_READ = 0x0002,
    STOPREASON_HIT_WRITE = 0x0004,
    STOPREASON_HIT_COND = 0x0008,
    STOPREASON_HIT_ESC = 0x0010,
    STOPREASON_HIT_VIOLA = 0x0020,
    STOPREASON_TIME_OVER = 0x0040,
    STOPREASON_UNDEFINS = 0x0080,
    STOPREASON_PABT = 0x0100,
    STOPREASON_DABT = 0x0200,
    STOPREASON_NONALIGN = 0x0400,
    STOPREASON_END
};

// View types.
enum MENUENUMTYPES {
    MENU_INFO_ITEM = 1,
    MENU_INFO_GROUP = 2,
    MENU_INFO_GROUP_END = -2,
    MENU_INFO_LIST_END = -1,
    MENU_INFO_RESERVED = 3,
    MENU_INFO_END = 4,
};

// Build completion codes.
enum UVBUILDCODES {
    UVBUILD_OK = 1,
    UVBUILD_OK_WARNINGS = 2,
    UVBUILD_ERRORS = 3,
    UVBUILD_CANCELLED = 4,
    UVBUILD_CLEANED = 5,
    UVBUILD_CODES_END
};

// Breakpoint type.
enum BKTYPE {
    BRKTYPE_EXEC = 1,
    BRKTYPE_READ = 2,
    BRKTYPE_WRITE = 3,
    BRKTYPE_READWRITE = 4,
    BRKTYPE_COMPLEX = 5,
    BRKTYPE_END
};

// Breakpoint change operation.
enum CHG_TYPE {
    CHG_KILLBP = 1,
    CHG_ENABLEBP = 2,
    CHG_DISABLEBP = 3,
    CHG_END
};

// Memory range type.
enum UV_MR {
    UV_MR_NONE = 0,
    UV_MR_ROM = 1,
    UV_MR_RAM = 2,
    UV_MR_END
};

// Project option type.
enum OPTSEL {
    OPT_LMISC = 1,
    OPT_CMISC = 2,
    OPT_AMISC = 3,
    OPT_CINCL = 4,
    OPT_AINCL = 5,
    OPT_CDEF = 6,
    OPT_ADEF = 7,
    OPT_CUNDEF = 8,
    OPT_AUNDEF = 9,
    OPT_COPTIMIZE = 10,
    OPT_CODEGEN = 11,
    OPT_MEMRANGES = 12,
    OPT_ASNMEMRANGES = 13,
    OPT_UBCOMP1 = 14,
    OPT_UBCOMP2 = 15,
    OPT_UBBUILD1 = 16,
    OPT_UBBUILD2 = 17,
    OPT_UABUILD1 = 18,
    OPT_UABUILD2 = 19,
    OPT_UBEEP = 20,
    OPT_USTARTDEB = 21,
    OPT_END
};

// Debug target type.
enum UV_TARGET {
    UV_TARGET_HW = 0,
    UV_TARGET_SIM = 1,
    UV_TARGET_END
};

// Progress bar control type.
enum PGCMD {
    UV_PROGRESS_INIT = 1,
    UV_PROGRESS_SETPOS = 2,
    UV_PROGRESS_CLOSE = 3,
    UV_PROGRESS_INITTXT = 4,
    UV_PROGRESS_SETTEXT = 5,
    UV_PROGRESS_END
};

// Symbol enumeration type.
enum ENTPJOB {
    UV_TPENUM_MEMBERS = 1,
    UV_TPENUM_END
};

// Event format.
enum EVFMT {
    EVFMT_RAW = 1,
    EVFMT_DEC = 2
};

#pragma pack(1)

// Socket options.
struct UVSOCK_OPTIONS {
    quint32 extendedStack : 1;
    quint32 : 31;
};
static_assert(sizeof(UVSOCK_OPTIONS) == 4, "UVSOCK_OPTIONS size is not 4 bytes");

// Cycles and time data.
struct CYCTS {
    quint64 timeCycles;
    double timeStamp;
};
static_assert(sizeof(CYCTS) == 16, "CYCTS size is not 16 bytes");

// String data.
struct SSTR {
    qint32 length; // Including NULL terminator.
    qint8 data[256]; // NULL terminated string.
};
static_assert(sizeof(SSTR) == 260, "SSTR size is not 260 bytes");

// Virtual register (VTR) value.
struct TVAL {
    VTT_TYPE type;
    union {
        qint8 sc;
        quint8 uc;
        qint16 s16;
        quint16 u16;
        qint32 s32;
        quint32 u32;
        qint64 s64;
        quint64 u64;
        int i;
        float f;
        double d;
    } v;
};
static_assert(sizeof(TVAL) == 12, "TVAL size is not 12 bytes");

// Virtual register (VTR) data or register data or expression
// to calculate or watch exprerssion to set/change.
struct VSET {
    TVAL value;
    SSTR name;
};
static_assert(sizeof(VSET) == 272, "VSET size is not 272 bytes");

// Variable enumeration or variable data request.
struct IVARENUM {
    qint32 id;
    qint32 frame;
    qint32 task;
    quint32 count : 16;
    quint32 changed : 1;
    quint32 reseved : 15;
};
static_assert(sizeof(IVARENUM) == 16, "IVARENUM size is not 16 bytes");

// Variable set request.
struct VARVAL {
    IVARENUM variable;
    SSTR value;
};
static_assert(sizeof(VARVAL) == 276, "VARVAL size is not 276 bytes");

// Information about watch expression, variable or its member.
struct VARINFO {
    qint32 id;
    qint32 index;
    qint32 count;
    qint32 typeSize;
    quint32 isEditable : 1;
    quint32 isPointer : 1;
    quint32 isFunction : 1;
    quint32 isStruct : 1;
    quint32 isUnion : 1;
    quint32 isClass : 1;
    quint32 isArray : 1;
    quint32 isEnum : 1;
    quint32 isAuto : 1;
    quint32 isParam : 1;
    quint32 isStatic : 1;
    quint32 isGlobal : 1;
    quint32 hasValue : 1;
    quint32 reserved1 : 12;
    quint32 hasType : 1;
    quint32 hasName : 1;
    quint32 hasQualified : 1;
    quint32 reserved2 : 4;
    SSTR value;
    SSTR type;
    SSTR name;
    SSTR qualified;
};
static_assert(sizeof(VARINFO) == 1060, "VARINFO size is not 1060 bytes");

// Menu enumeration data request.
struct MENUID {
    qint32 menuType;
    qint32 menuId;
    quint32 reserved : 32;
};
static_assert(sizeof(MENUID) == 12, "MENUID size is not 12 bytes");

// Menu enumeration information.
struct MENUENUM {
    qint32 menuId;
    qint32 menuType;
    qint32 infoType;
    quint32 isEnabled : 1;
    quint32 reserved : 31;
    SSTR menuLabel;
};
static_assert(sizeof(MENUENUM) == 276, "MENUENUM size is not 276 bytes");

// Memory read or write data.
struct AMEM {
    quint64 address;
    quint32 bytesCount;
    quint64 errorAddress;
    quint32 errorCode;
    quint8 bytes[1];
};
static_assert(sizeof(AMEM) == 25, "AMEM size is not 25 bytes");

// Terminal I/O data.
struct SERIO {
    quint16 channel;
    quint16 itemMode;
    quint32 itemsCount;
    union {
        quint8 bytes[1];
        quint16 words[1];
    } s;
};
static_assert(sizeof(SERIO) == 10, "SERIO size is not 10 bytes");

// ITM channel booking or output data.
struct ITMOUT {
    quint16 channel;
    quint16 itemMode;
    quint32 itemsCount;
    union {
        quint8 bytes[1];
        quint16 words[1];
    } s;
};
static_assert(sizeof(ITMOUT) == 10, "ITMOUT size is not 10 bytes");

// Multiple string data.
struct PRJDATA {
    quint32 length;
    quint32 code;
    qint8 names[1];
};
static_assert(sizeof(PRJDATA) == 9, "PRJDATA size is not 9 bytes");

// Breakpoint parameter data (new).
struct BKPARM {
    BKTYPE type;
    quint32 count;
    quint32 accessSize;
    quint32 expressionLength;
    quint32 commandLength;
    qint8 expressionBuffer[1];
};
static_assert(sizeof(BKPARM) == 21, "BKPARM size is not 21 bytes");

// Breakpoint parameter data (existing).
struct BKRSP {
    BKTYPE type;
    quint32 count;
    quint32 isEnabled;
    quint32 tickMark;
    quint64 address;
    quint32 expressionLength;
    qint8 expressionBuffer[512];
};
static_assert(sizeof(BKRSP) == 540, "BKRSP size is not 540 bytes");

// Breakpoint change data.
struct BKCHG {
    CHG_TYPE type;
    quint32 tickMark;
    quint32 reserved[8];
};
static_assert(sizeof(BKCHG) == 40, "BKCHG size is not 40 bytes");

// Project option data.
struct TRNOPT {
    OPTSEL job;
    quint32 targetIndex;
    quint32 groupIndex;
    quint32 fileIndex;
    quint32 itemIndex;
    qint8 buffer[3];
};
static_assert(sizeof(TRNOPT) == 23, "TRNOPT size is not 23 bytes");

// Individual memory range data.
struct UV_MRANGE {
    quint32 memoryType : 8;
    quint32 defaultRamRange : 1;
    quint32 defaultRomRange : 1;
    quint32 zeroRamRange : 1;
    quint32 reserved1 : 21;
    quint32 reserved2[3];
    quint64 rangeStartAddress;
    quint64 rangeSize;
};
static_assert(sizeof(UV_MRANGE) == 32, "UV_MRANGE size is not 32 bytes");

// Memory ranges data.
struct UV_MEMINFO {
    quint32 rangesCount;
    quint32 reserved[8];
    UV_MRANGE range;
};
static_assert(sizeof(UV_MEMINFO) == 68, "UV_MEMINFO size is not 68 bytes");

// Licensing data.
struct UVLICINFO {
    quint32 license : 1;
    quint32 reserved1 : 31;
    quint32 reserved2[10];
};
static_assert(sizeof(UVLICINFO) == 44, "UVLICINFO size is not 44 bytes");

// Target debugging data.
struct DBGTGTOPT {
    quint32 targetType : 1;
    quint32 reserved1 : 31;
    quint32 reserved2[10];
};
static_assert(sizeof(DBGTGTOPT) == 44, "DBGTGTOPT size is not 44 bytes");

// Progress bar control data.
struct PGRESS {
    PGCMD job;
    quint32 completionPercentage;
    quint32 reserved[8];
    qint8 label[1];
};
static_assert(sizeof(PGRESS) == 41, "PGRESS size is not 41 bytes");

// Extended version data.
struct EXTVERS {
    quint32 uvisionVersionIndex;
    quint32 uvsockVersionIndex;
    quint32 reserved[30];
    qint8 buffer[3];
};
static_assert(sizeof(EXTVERS) == 131, "EXTVERS size is not 131 bytes");

// Symbol enumeration data.
struct ENUMTPM {
    ENTPJOB job;
    quint32 memberOffset;
    quint32 memberSize;
    quint32 reserved[8];
    qint8 memberName[512];
};
static_assert(sizeof(ENUMTPM) == 556, "ENUMTPM size is not 556 bytes");

// Map address to file or line request data.
struct ADRMTFL {
    quint32 isFullPath :  1;
    quint32 reserved1 : 31;
    quint64 address;
    quint32 reserved2[7];
};
static_assert(sizeof(ADRMTFL) == 40, "ADRMTFL size is not 40 bytes");

// Map address to file or line return data.
struct AFLMAP {
    quint32 codeLineNumber;
    quint64 codeAddress;
    quint32 fileNameIndex;
    quint32 functionNameIndex;
    qint32 reserved[5];
    qint8 fileName[1];
};
static_assert(sizeof(AFLMAP) == 41, "AFLMAP size is not 41 bytes");

// Breakpoint reason data.
struct BPREASON {
    quint32 reserved1;
    quint32 reserved2;
    quint32 length;
    STOPREASON stopReason;
    quint64 programCounterAddress;
    quint64 reasonAddress;
    qint32 breakpointNumber;
    quint32 tickMark;
    quint32 reserved3[4];
};
static_assert(sizeof(BPREASON) == 56, "BPREASON size is not 56 bytes");

// Stack enumeration request data.
struct iSTKENUM {
    quint32 isFull : 1;
    quint32 hasExtended : 1;
    quint32 onlyModified : 1;
    quint32 reserved1 : 29;
    quint32 task;
    quint32 reserved2[6];
};
static_assert(sizeof(iSTKENUM) == 32, "iSTKENUM size is not 32 bytes");

// Stack enumeration return data.
struct STACKENUM {
    quint32 number;
    quint64 currentAddress;
    quint64 returnAddress;
    quint32 variablesCount;
    quint32 equalFramesCount;
    quint32 totalFramesCount;
    quint32 task;
    quint32 reserved[3];
};
static_assert(sizeof(STACKENUM) == 48, "STACKENUM size is not 48 bytes");

// Task list enumeration return data.
struct TASKENUM {
    qint32 id;
    quint64 entryAddress;
    quint32 state : 8;
    quint32 reserved : 24;
    SSTR name;
};
static_assert(sizeof(TASKENUM) == 276, "TASKENUM size is not 276 bytes");

// Register enumeration return data.
struct REGENUM {
    quint16 groupIndex;
    quint16 item;
    qint8 name[16];
    quint8 isPc : 1;
    quint8 isChangable : 1;
    quint8 reserved : 6;
    qint8 value[32];
};
static_assert(sizeof(REGENUM) == 53, "REGENUM size is not 53 bytes");

// Virtual register (VTR) enumeration request data.
struct iVTRENUM {
    quint32 value : 1;
    quint32 reserved : 31;
    quint32 reserved2[7];
};
static_assert(sizeof(iVTRENUM) == 32, "iVTRENUM size is not 32 bytes");

// Virtual register (VTR) enumeration return data.
struct AVTR {
    quint32 useValue : 1;
    quint32 type : 8;
    quint32 isXtal : 1;
    quint32 isClock : 1;
    quint32 internalNumber : 16;
    quint32 reserved1 : 5;
    quint32 reserved2[7];
    TVAL value;
    qint8 name[1];
};
static_assert(sizeof(AVTR) == 45, "AVTR size is not 45 bytes");

// Wake interval data.
struct iINTERVAL {
    quint32 autoStart : 1;
    quint32 useCycles : 1;
    quint32 useIntervalCallback : 1;
    quint32 reserved1 : 29;
    float secondsCount;
    qint64 cyclesCount;
    quint32 reserved2[7];
};
static_assert(sizeof(iINTERVAL) == 44, "iINTERVAL size is not 44 bytes");

// Show code in uVision request data.
struct iSHOWSYNC {
    quint64 displayedCodeAddress;
    quint32 displayDisassembly : 1;
    quint32 displayCode : 1;
    quint32 reserved1 : 1;
    quint32 reserved2 : 1;
    quint32 reserved3 : 28;
    quint32 reserved4[7];
};
static_assert(sizeof(iSHOWSYNC) == 40, "iSHOWSYNC size is not 40 bytes");

// Timestamp for power scale.
struct UVSC_PSTAMP {
    quint64 codeAddress;
    qint64 ticksCount;
    double timeDelta;
    double absoluteTime;
    quint32 displaySyncError : 1;
    quint32 reserved1 : 31;
    quint32 reserved2[6];
};
static_assert(sizeof(UVSC_PSTAMP) == 60, "UVSC_PSTAMP size is not 60 bytes");

// RAW event recorder.
struct RAW_EVENT {
    quint32 type : 8;
    quint32 reserved1 : 24;
    quint64 eventNumber;
    quint64 timeStamp;
    quint32 restartId;
    quint16 eventId;
    quint16 payloadValuesCount;
    quint8 multipleRecordsIndex;
    quint8 eventContext;
    quint8 recorderId;
    quint8 hasOverflowL : 1;
    quint8 hasReset : 1;
    quint8 isValidId : 1;
    quint8 lastEventRecord : 1;
    quint8 isCompleteRecord : 1;
    quint8 hasOverflowH : 1;
    quint8 irqFlag : 1;
    quint8 hasResume : 1;
    quint32 payload[2];
};
static_assert(sizeof(RAW_EVENT) == 40, "RAW_EVENT size is not 40 bytes");

// DEC event recorder.
struct DEC_EVENT {
    quint32 type : 8;
    quint32 reserved : 24;
    quint64 eventNumber;
    quint64 timeStamp;
    qint8 text[1];
};
static_assert(sizeof(DEC_EVENT) == 21, "DEC_EVENT size is not 21 bytes");

// Event package item.
struct EVTR_PACK {
    quint16 many;
    union {
        qint8 data;
        qint8 type;
        RAW_EVENT raw;
        DEC_EVENT dec;
    };
};
static_assert(sizeof(EVTR_PACK) == 42, "EVTR_PACK size is not 42 bytes");

// Event out item.
struct EVTROUT {
    quint32 many;
    quint32 reserved;
    union {
        qint8 data;
        EVTR_PACK pack;
    };
};
static_assert(sizeof(EVTROUT) == 50, "EVTROUT size is not 50 bytes");

// Command execution request or response data.
struct EXECCMD {
    quint32 useEcho : 1;
    quint32 reserved1 : 31;
    quint32 reserved2[7];
    SSTR command;
};
static_assert(sizeof(EXECCMD) == 292, "EXECCMD size is not 292 bytes");

// Path request data structure.
struct iPATHREQ {
    quint32 useFullPath : 1;
    quint32 reserved1 : 31;
    quint32 reserved2[7];
};
static_assert(sizeof(iPATHREQ) == 32, "iPATHREQ size is not 32 bytes");

//#pragma pack(1)  // ()

// UVSOCK error response data.
struct UVSOCK_ERROR_RESPONSE {
    quint32 reserved1;
    quint32 reserved2;
    quint32 length;
    quint8 data[SOCK_NDATA - 20];
};
static_assert(sizeof(UVSOCK_ERROR_RESPONSE) == 32760, "UVSOCK_ERROR_RESPONSE size is not 32760 bytes");

// UVSOCK command response or async message data format.
struct UVSOCK_CMD_RESPONSE {
    UV_OPERATION command;
    UV_STATUS status;
    union {
        UVSOCK_ERROR_RESPONSE error;
        quint32 value;
        CYCTS cycts;
        AMEM amem;
        SERIO serio;
        ITMOUT itmout;
        EVTROUT evtrout;
        VSET vset;
        BKRSP bkrsp;
        TRNOPT trnopt;
        SSTR sstr;
        EXTVERS extvers;
        ENUMTPM enumtpm;
        AFLMAP aflmap;
        BPREASON bpreason;
        STACKENUM stackenum;
        TASKENUM taskenum;
        AVTR avtr;
        UVLICINFO uvlicinfo;
        DBGTGTOPT dbgtgtopt;
        UVSC_PSTAMP uvpstamp;
        REGENUM regenum;
        VARINFO varinfo;
        MENUENUM viewinfo;
        qint8 strbuf[1];
    };
};
static_assert(sizeof(UVSOCK_CMD_RESPONSE) == 32768, "UVSOCK_CMD_RESPONSE size is not 32768 bytes");

// UVSOCK message data format.
union UVSOCK_CMD_DATA {
    quint8 raw[SOCK_NDATA];
    PRJDATA prjdata;
    AMEM amem;
    SERIO serio;
    ITMOUT itmout;
    EVTROUT evtrout;
    VSET vset;
    TRNOPT trnopt;
    SSTR sstr;
    BKPARM bkparm;
    BKCHG bkchg;
    DBGTGTOPT dbgtgtopt;
    ADRMTFL adrmtfl;
    iSHOWSYNC ishowsync;
    iVTRENUM ivtrenum;
    EXECCMD execcmd;
    iPATHREQ ipathreq;
    UVSC_PSTAMP uvpstamp;
    iSTKENUM istkenum;
    PGRESS pgress;
    ENUMTPM enumtpm;
    iINTERVAL iinterval;
    quint32 value;
    quint64 address;
    UVSOCK_OPTIONS uvsockopt;
    UVSOCK_CMD_RESPONSE cmdrsp;
};
static_assert(sizeof(UVSOCK_CMD_DATA) == 32768, "UVSOCK_CMD_DATA size is not 32768 bytes");

// UVSOCK message format.
struct UVSOCK_CMD {
    quint32 totalLength;
    UV_OPERATION command;
    quint32 dataLength;
    quint64 cyclesCount;
    double timeStamp;
    quint32 id;
    UVSOCK_CMD_DATA data;
};
static_assert(sizeof(UVSOCK_CMD) == 32800, "UVSOCK_CMD size is not 32800 bytes");

#pragma pack()

#ifdef __cplusplus
}
#endif
