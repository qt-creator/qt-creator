/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "peutils.h"

#include <utils/winutils.h>
#include <QStringList>
#include <QDebug>
#include <climits>
#include <windows.h>

using Utils::winErrorMessage;

// Create a pointer from base and offset when rummaging around in
// a memory mapped file

template <class Ptr>
inline Ptr *makePtr(void *base, ptrdiff_t offset)
{
    return reinterpret_cast<Ptr*>(static_cast<char*>(base) + offset);
}

// CodeView header
struct CV_HEADER
{
    DWORD CvSignature; // NBxx
    LONG  Offset;      // Always 0 for NB10
};

// CodeView NB10 debug information of a PDB 2.00 file (VS 6)
struct CV_INFO_PDB20
{
    CV_HEADER  Header;
    DWORD      Signature;
    DWORD      Age;
    BYTE       PdbFileName[1];
};

// CodeView RSDS debug information of a PDB 7.00 file
struct CV_INFO_PDB70
{
    DWORD      CvSignature;
    GUID       Signature;
    DWORD      Age;
    BYTE       PdbFileName[1];
};

// Retrieve the NT image header of an executable via the legacy DOS header.
static IMAGE_NT_HEADERS *getNtHeader(void *fileMemory, QString *errorMessage)
{
    IMAGE_DOS_HEADER *dosHeader = static_cast<PIMAGE_DOS_HEADER>(fileMemory);
    // Check DOS header consistency
    if (IsBadReadPtr(dosHeader, sizeof(IMAGE_DOS_HEADER))
        || dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        *errorMessage = QString::fromLatin1("DOS header check failed.");
        return 0;
    }
    // Retrieve NT header
    IMAGE_NT_HEADERS *ntHeaders = makePtr<IMAGE_NT_HEADERS>(dosHeader, dosHeader->e_lfanew);
    // check NT header consistency
    if (IsBadReadPtr(ntHeaders, sizeof(ntHeaders->Signature))
        || ntHeaders->Signature != IMAGE_NT_SIGNATURE
        || IsBadReadPtr(&ntHeaders->FileHeader, sizeof(IMAGE_FILE_HEADER))) {
        *errorMessage = QString::fromLatin1("NT header check failed.");
        return 0;
    }
    // Check magic
    const WORD magic = ntHeaders->OptionalHeader.Magic;
#ifdef  __GNUC__ // MinGW does not have complete 64bit definitions.
    if (magic != 0x10b) {
        *errorMessage = QString::fromLatin1("NT header check failed; magic %1 is not that of a 32-bit executable.").
                        arg(magic);
        return 0;
    }
#else
    if (magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC && magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        *errorMessage = QString::fromLatin1("NT header check failed; magic %1 is none of %2, %3.").
                        arg(magic).arg(IMAGE_NT_OPTIONAL_HDR32_MAGIC).arg(IMAGE_NT_OPTIONAL_HDR64_MAGIC);
        return 0;
    }
#endif
    // Check section headers
    IMAGE_SECTION_HEADER *sectionHeaders = IMAGE_FIRST_SECTION(ntHeaders);
    if (IsBadReadPtr(sectionHeaders, ntHeaders->FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER))) {
        *errorMessage = QString::fromLatin1("NT header section header check failed.");
        return 0;
    }
    return ntHeaders;
}

// Find the COFF section an RVA belongs to and convert to file offset
static bool getFileOffsetFromRVA(IMAGE_NT_HEADERS *ntHeaders, DWORD rva, DWORD* fileOffset)
{
    IMAGE_SECTION_HEADER *sectionHeader = IMAGE_FIRST_SECTION(ntHeaders);
    for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++, sectionHeader++) {
        const DWORD sectionSize = sectionHeader->Misc.VirtualSize ?
                                  sectionHeader->Misc.VirtualSize : sectionHeader->SizeOfRawData;
        if ((rva >= sectionHeader->VirtualAddress) && (rva < sectionHeader->VirtualAddress + sectionSize)) {
            const DWORD diff = sectionHeader->VirtualAddress - sectionHeader->PointerToRawData;
            *fileOffset = rva - diff;
            return true;
        }
    }
    return false;
}

// Retrieve debug directory and number of entries
static bool getDebugDirectory(IMAGE_NT_HEADERS *ntHeaders, void *fileMemory,
    IMAGE_DEBUG_DIRECTORY **debugDir, int *count, QString *errorMessage)
{
    DWORD debugDirRva = 0;
    DWORD debugDirSize;
    *debugDir = 0;
    *count = 0;

#ifdef  __GNUC__ // MinGW does not have complete 64bit definitions.
    IMAGE_OPTIONAL_HEADER *optionalHeader = reinterpret_cast<IMAGE_OPTIONAL_HEADER*>(&(ntHeaders->OptionalHeader));
    debugDirRva = optionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
    debugDirSize = optionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size;
#else
    // Find the virtual address
    const bool is64Bit = ntHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC;
    if (is64Bit) {
        IMAGE_OPTIONAL_HEADER64 *optionalHeader64 = reinterpret_cast<IMAGE_OPTIONAL_HEADER64*>(&(ntHeaders->OptionalHeader));
        debugDirRva = optionalHeader64->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
        debugDirSize = optionalHeader64->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size;
    } else {
        IMAGE_OPTIONAL_HEADER32 *optionalHeader32 = reinterpret_cast<IMAGE_OPTIONAL_HEADER32*>(&(ntHeaders->OptionalHeader));
        debugDirRva = optionalHeader32->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
        debugDirSize = optionalHeader32->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size;
    }
#endif

    // Empty. This is the case for MinGW binaries
    if (debugDirSize == 0)
        return true;
    // Look up in  file
    DWORD debugDirOffset;
    if (!getFileOffsetFromRVA(ntHeaders, debugDirRva, &debugDirOffset)) {
        *errorMessage = QString::fromLatin1("Unable to locate debug dir RVA %1/%2.").arg(debugDirRva).arg(debugDirSize);
        return false;
    }
    *debugDir = makePtr<IMAGE_DEBUG_DIRECTORY>(fileMemory, debugDirOffset);
    // Check
    if (IsBadReadPtr(*debugDir, debugDirSize) || debugDirSize < sizeof(IMAGE_DEBUG_DIRECTORY)) {
        *errorMessage = QString::fromLatin1("Debug directory corrupted.");
        return 0;
    }

    *count = debugDirSize / sizeof(IMAGE_DEBUG_DIRECTORY);
    return debugDir;
}

// Return the PDB file of a Code View debug section
static QString getPDBFileOfCodeViewSection(void *debugInfo, DWORD size)
{
    static const DWORD CV_SIGNATURE_NB10 = 0x3031424e; // '01BN';
    static const DWORD CV_SIGNATURE_RSDS = 0x53445352; // 'SDSR';
    if (IsBadReadPtr(debugInfo, size) || size < sizeof(DWORD))
        return QString();

    const DWORD cvSignature = *static_cast<DWORD*>(debugInfo);
    if (cvSignature == CV_SIGNATURE_NB10) {
        CV_INFO_PDB20* cvInfo = static_cast<CV_INFO_PDB20*>(debugInfo);
        if (IsBadReadPtr(debugInfo, sizeof(CV_INFO_PDB20)))
            return QString();
        CHAR* pdbFileName = reinterpret_cast<CHAR*>(cvInfo->PdbFileName);
        if (IsBadStringPtrA(pdbFileName, UINT_MAX))
            return QString();
        return QString::fromLocal8Bit(pdbFileName);
    }
    if (cvSignature == CV_SIGNATURE_RSDS) {
        CV_INFO_PDB70* cvInfo = static_cast<CV_INFO_PDB70*>(debugInfo);
        if (IsBadReadPtr(debugInfo, sizeof(CV_INFO_PDB70)))
            return QString();
        CHAR* pdbFileName = reinterpret_cast<CHAR*>(cvInfo->PdbFileName);
        if (IsBadStringPtrA(pdbFileName, UINT_MAX))
            return QString();
        return QString::fromLocal8Bit(pdbFileName);
    }
    return QString();
}

// Collect all PDB files of all debug sections
static void collectPDBfiles(void *fileMemory, IMAGE_DEBUG_DIRECTORY *directoryBase, int count, QStringList *pdbFiles)
{
    for (int i = 0; i < count; i++, directoryBase++)
        if (directoryBase->Type == IMAGE_DEBUG_TYPE_CODEVIEW) {
           const QString pdb = getPDBFileOfCodeViewSection(static_cast<char*>(fileMemory) + directoryBase->PointerToRawData, directoryBase->SizeOfData);
           if (!pdb.isEmpty())
               pdbFiles->push_back(pdb);
       }
}

namespace Debugger {
namespace Internal {

bool getPDBFiles(const QString &peExecutableFileName, QStringList *rc, QString *errorMessage)
{
    HANDLE hFile = NULL;
    HANDLE hFileMap = NULL;
    void *fileMemory = 0;
    bool success = false;

    rc->clear();
    do {
        // Create a memory mapping of the file
        hFile = CreateFile(reinterpret_cast<const WCHAR*>(peExecutableFileName.utf16()), GENERIC_READ, FILE_SHARE_READ, NULL,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE || hFile == NULL) {
            *errorMessage = QString::fromLatin1("Cannot open '%1': %2").arg(peExecutableFileName, winErrorMessage(GetLastError()));
            break;
        }

        hFileMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
        if (hFileMap == NULL) {
            *errorMessage = QString::fromLatin1("Cannot create file mapping of '%1': %2").arg(peExecutableFileName, winErrorMessage(GetLastError()));
            break;
        }

        fileMemory = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 0);
        if (!fileMemory) {
            *errorMessage = QString::fromLatin1("Cannot map '%1': %2").arg(peExecutableFileName, winErrorMessage(GetLastError()));
            break;
        }

        IMAGE_NT_HEADERS *ntHeaders = getNtHeader(fileMemory, errorMessage);
        if (!ntHeaders)
            break;

        int debugSectionCount;
        IMAGE_DEBUG_DIRECTORY *debugDir;
        if (!getDebugDirectory(ntHeaders, fileMemory, &debugDir, &debugSectionCount, errorMessage))
            return false;
        if (debugSectionCount)
            collectPDBfiles(fileMemory, debugDir, debugSectionCount, rc);
        success = true;
    } while (false);

    if (fileMemory)
        UnmapViewOfFile(fileMemory);

    if (hFileMap != NULL)
        CloseHandle(hFileMap);

    if (hFile != NULL && hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    return success;
}

} // namespace Internal
} // namespace Debugger
