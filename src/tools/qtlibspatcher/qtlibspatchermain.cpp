/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "binpatch.h"
#include <stdio.h>

#include <QFile>
#include <QRegExp>
#include <QTextStream>
#include <QString>
#include <QtCore/QDebug>

bool patchBinaryWithQtPathes(const char *fileName, const char *baseQtPath)
{
    bool result = true;

    static const struct
    {
        const char *variable;
        const char *subDirectory;
    } variables[] = {
        {"qt_prfxpath=", ""},
        {"qt_docspath=", "/doc"},
        {"qt_hdrspath=", "/include"},
        {"qt_libspath=", "/lib"},
        {"qt_binspath=", "/bin"},
        {"qt_plugpath=", "/plugins"},
        {"qt_datapath=", ""},
        {"qt_trnspath=", "/translations"},
        {"qt_xmplpath=", "/examples"},
        {"qt_demopath=", "/demos"}
    };

    for (int i = 0; i < sizeof(variables)/sizeof(variables[0]); i++) {
        char newStr[256] = "";
        strncpy(newStr, variables[i].variable, sizeof(newStr));
        newStr[sizeof(newStr) - 1] = 0;
        strncat(newStr, baseQtPath, sizeof(newStr) - strlen(newStr) - 1);
        newStr[sizeof(newStr) - 1] = 0;
        strncat(newStr, variables[i].subDirectory, sizeof(newStr) - strlen(newStr) - 1);
        newStr[sizeof(newStr) - 1] = 0;
        BinPatch binFile(fileName);
        if (!binFile.patch(variables[i].variable, newStr)) {
            result = false;
            break;
        }
    }

    return result;
}

bool patchBinariesWithQtPathes(const char *baseQtPath)
{
    bool result = true;

    static const char *filesToPatch[] = {
        "/bin/qmake.exe",
        "/bin/QtCore4.dll",
        "/bin/QtCored4.dll"
    };

    for (int i = 0; i < sizeof(filesToPatch)/sizeof(filesToPatch[0]); i++) {
        char fileName[FILENAME_MAX] = "";
        strncpy(fileName, baseQtPath, sizeof(fileName));
        fileName[sizeof(fileName)-1] = '\0';
        strncat(fileName, filesToPatch[i], sizeof(fileName) - strlen(fileName) - 1);
        fileName[sizeof(fileName)-1] = '\0';
        if (!patchBinaryWithQtPathes(fileName, baseQtPath)) {
            result = false;
            break;
        }
    }

    return result;
}

bool patchDebugLibrariesWithQtPath(const char *baseQtPath)
{
    bool result = true;

    static const struct
    {
        const char *fileName;
        const char *sourceLocation;
    } libraries[] = {
        {"/bin/Qt3Supportd4.dll", "/src/qt3support/"},
        {"/bin/QtCored4.dll", "/src/corelib/"},
        {"/bin/QtGuid4.dll", "/src/gui/"},
        {"/bin/QtHelpd4.dll", "/tools/assistant/lib/"},
        {"/bin/QtNetworkd4.dll", "/src/network/"},
        {"/bin/QtOpenGLd4.dll", "/src/opengl/"},
        {"/bin/QtScriptd4.dll", "/src/script/"},
        {"/bin/QtSqld4.dll", "/src/sql/"},
        {"/bin/QtSvgd4.dll", "/src/svg/"},
        {"/bin/QtTestd4.dll", "/src/testlib/"},
        {"/bin/QtWebKitd4.dll", "/src/3rdparty/webkit/WebCore/"},
        {"/bin/QtXmld4.dll", "/src/xml/"},
        {"/bin/QtXmlPatternsd4.dll", "/src/xmlpatterns/"},
        {"/plugins/accessible/qtaccessiblecompatwidgetsd4.dll", "/src/plugins/accessible/compat/"},
        {"/plugins/accessible/qtaccessiblewidgetsd4.dll", "/src/plugins/accessible/widgets/"},
        {"/plugins/codecs/qcncodecsd4.dll", "/src/plugins/codecs/cn/"},
        {"/plugins/codecs/qjpcodecsd4.dll", "/src/plugins/codecs/jp/"},
        {"/plugins/codecs/qkrcodecsd4.dll", "/src/plugins/codecs/kr/"},
        {"/plugins/codecs/qtwcodecsd4.dll", "/src/plugins/codecs/tw/"},
        {"/plugins/iconengines/qsvgicond4.dll", "/src/plugins/iconengines/svgiconengine/"},
        {"/plugins/imageformats/qgifd4.dll", "/src/plugins/imageformats/gif/"},
        {"/plugins/imageformats/qjpegd4.dll", "/src/plugins/imageformats/jpeg/"},
        {"/plugins/imageformats/qmngd4.dll", "/src/plugins/imageformats/mng/"},
        {"/plugins/imageformats/qsvgd4.dll", "/src/plugins/imageformats/svg/"},
        {"/plugins/imageformats/qtiffd4.dll", "/src/plugins/imageformats/tiff/"},
        {"/plugins/sqldrivers/qsqlited4.dll", "/src/plugins/sqldrivers/sqlite/"},
//        {"/plugins/sqldrivers/qsqlodbcd4.dll", "/src/plugins/sqldrivers/odbc/"}
    };

    for (int i = 0; i < sizeof(libraries)/sizeof(libraries[0]); i++) {
        char fileName[FILENAME_MAX] = "";
        strncpy(fileName, baseQtPath, sizeof(fileName));
        fileName[sizeof(fileName)-1] = '\0';
        strncat(fileName, libraries[i].fileName, sizeof(fileName) - strlen(fileName) - 1);
        fileName[sizeof(fileName)-1] = '\0';

        char oldSourcePath[FILENAME_MAX] =
            "C:/qt-greenhouse/Trolltech/Code_less_create_more/Trolltech/Code_less_create_more/Troll/4.4.3";
        strncat(oldSourcePath, libraries[i].sourceLocation, sizeof(oldSourcePath) - strlen(oldSourcePath) - 1);
        oldSourcePath[sizeof(oldSourcePath)-1] = '\0';

        char newSourcePath[FILENAME_MAX] = "";
        strncpy(newSourcePath, baseQtPath, sizeof(newSourcePath));
        newSourcePath[sizeof(newSourcePath)-1] = '\0';
        strncat(newSourcePath, libraries[i].sourceLocation, sizeof(newSourcePath) - strlen(newSourcePath) - 1);
        newSourcePath[sizeof(newSourcePath)-1] = '\0';

        BinPatch binFile(fileName);
        if (!binFile.patch(oldSourcePath, newSourcePath)) {
            result = false;
            break;
        }
    }

    return result;
}

void patchQMakeSpec(const char *path)
{
    QString baseQtPath(path);
    QFile f(baseQtPath + "/mkspecs/default/qmake.conf");
    f.open(QIODevice::ReadOnly);
    QTextStream in(&f);
    QString all = in.readAll();
    f.close();
    // Old value is QMAKESPEC_ORIGINAL=C:/somepath/mkspecs/amakespce
    // New value should be QMAKESPEC_ORIGINAL=baseQtPath/mkspec/amakespec
    // We don't care to match the line end we simply match to the end of the file
    QRegExp regExp("(QMAKESPEC_ORIGINAL=).*(/mkspecs/.*)");
    all.replace(regExp, "\\1"+baseQtPath+"\\2");
    
    f.open(QIODevice::WriteOnly);
    QTextStream out(&f);
    out << all;
}

int main(int argc, char *args[])
{
    if (argc != 2) {
        printf("Please provide a QTDIR value as parameter.\n"
               "This is also the location where the binaries are expected\n"
               "in the \"/bin\" and \"/plugins\" subdirectories.\n");
        return 1;
    }

    char baseQtPath[FILENAME_MAX] = "";
    strncpy(baseQtPath, args[1], sizeof(baseQtPath));
    baseQtPath[sizeof(baseQtPath)-1] = '\0';

    // Convert backslash to slash
    for (char *p = baseQtPath; *p != '\0'; p++)
        if (*p == '\\')
            *p = '/';

    // Remove trailing slash(es)
    for (char *p = baseQtPath + strlen(baseQtPath) - 1; p != baseQtPath; p--)
        if (*p == '/')
            *p = '\0';
        else
            break;

    patchQMakeSpec(baseQtPath);
    return patchBinariesWithQtPathes(baseQtPath) && patchDebugLibrariesWithQtPath(baseQtPath)?0:1;
}
