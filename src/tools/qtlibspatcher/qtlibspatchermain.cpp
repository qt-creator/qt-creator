/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "binpatch.h"
#include <cstdio>
#include <iostream>

#include <QFile>
#include <QRegExp>
#include <QTextStream>
#include <QString>
#include <QtCore/QDebug>

#ifdef Q_OS_WIN
#   define QT_INSTALL_DIR "C:/qt-greenhouse/Trolltech/Code_less_create_more/Trolltech/Code_less_create_more/Troll/4.5.0/qt";

    const char * const oldInstallBase = QT_INSTALL_DIR;
    const char * const oldSourceBase = QT_INSTALL_DIR;
#else
    const char * const oldSourceBase = "/home/berlin/dev/qt-4.5.0-opensource-temp/this_path_is_supposed_to_be_very_long_hopefully_this_is_long_enough/qt-x11-opensource-src-4.5.0";
    const char * const oldInstallBase = "/home/berlin/dev/qt-4.5.0-opensource-shipping/this_path_is_supposed_to_be_very_long_hopefully_this_is_long_enough/qt";
#endif


char * allocFileNameCopyAppend(const char * textToCopy,
        const char * textToAppend, const char * textToAppend2 = NULL);

void logFileName(const char *fileName) {
    std::cout << "Patching file " << fileName << std::endl;
}

void logDiff(const char *oldText, const char *newText) {
    std::cout << "  --- " << oldText << std::endl;
    std::cout << "  +++ " << newText << std::endl;
}

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

    logFileName(fileName);
    for (int i = 0; i < (int)(sizeof(variables) / sizeof(variables[0])); i++) {
        const char * const newStr = allocFileNameCopyAppend(
            variables[i].variable, baseQtPath, variables[i].subDirectory);
        BinPatch binFile(fileName);
        const bool success = binFile.patch(variables[i].variable, newStr);
        delete[] newStr;
        if (!success) {
            result = false;
        }
    }

    return result;
}

bool patchBinariesWithQtPathes(const char *baseQtPath)
{
    bool result = true;

    static const char *filesToPatch[] = {
    #ifdef Q_OS_WIN
        "/bin/qmake.exe",
        "/bin/QtCore4.dll",
        "/bin/QtCored4.dll",
        "/lib/QtCored4.dll"
    #else
        "/bin/qmake",
        "/lib/libQtCore.so",
    #endif
    };

    for (int i = 0; i < (int)(sizeof(filesToPatch) / sizeof(filesToPatch[0])); i++) {
        const char * const fileName = allocFileNameCopyAppend(baseQtPath, filesToPatch[i]);
        const bool success = patchBinaryWithQtPathes(fileName, baseQtPath);
        delete[] fileName;
        if (!success) {
            result = false;
        }
    }

    return result;
}

char * allocFileNameCopyAppend(const char * textToCopy,
        const char * textToAppend, const char * textToAppend2)
{
    const unsigned int bytesToAllocate = FILENAME_MAX;
    Q_ASSERT(bytesToAllocate > 0);
    Q_ASSERT(textToCopy != NULL);
    Q_ASSERT(textToAppend != NULL);
    if (textToAppend2 == NULL)
        textToAppend2 = "";

    char * const res = new char[bytesToAllocate];
    const size_t textToCopyLen = strlen(textToCopy);
    const size_t textToAppendLen = strlen(textToAppend);
    const size_t textToAppend2Len = strlen(textToAppend2);

    // Array too small?
    if (textToCopyLen + textToAppendLen + textToAppend2Len >= bytesToAllocate) {
        res[0] = '\0';
        return res;
    }

    strncpy(res, textToCopy, bytesToAllocate - 1);
    if (textToAppendLen > 0)
        strncpy(res + textToCopyLen, textToAppend, bytesToAllocate - textToCopyLen - 1);
    if (textToAppend2Len > 0)
        strncpy(res + textToCopyLen + textToAppendLen, textToAppend2, bytesToAllocate - textToCopyLen - textToAppendLen - 1);
    res[textToCopyLen + textToAppendLen + textToAppend2Len] = '\0';
    res[bytesToAllocate - 1] = '\0'; // Safe is safe
    return res;
}


bool patchDebugLibrariesWithQtPath(const char *baseQtPath)
{
    bool result = true;

    static const struct
    {
        const char *fileName;
    } libraries[] = {
#ifdef Q_OS_WIN
        { "/bin/Qt3Supportd4.dll" },
        { "/bin/QtCored4.dll" },
        { "/bin/QtGuid4.dll" },
        { "/bin/QtHelpd4.dll" },
        { "/bin/QtNetworkd4.dll" },
        { "/bin/QtOpenGLd4.dll" },
        { "/bin/QtScriptd4.dll" },
        { "/bin/QtScriptToolsd4.dll" },
        { "/bin/QtSqld4.dll" },
        { "/bin/QtSvgd4.dll" },
        { "/bin/QtTestd4.dll" },
        { "/bin/QtWebKitd4.dll" },
        { "/bin/QtXmld4.dll" },
        { "/bin/QtXmlPatternsd4.dll" },
        { "/lib/Qt3Supportd4.dll" },
        { "/lib/QtCored4.dll" },
        { "/lib/QtGuid4.dll" },
        { "/lib/QtHelpd4.dll" },
        { "/lib/QtNetworkd4.dll" },
        { "/lib/QtOpenGLd4.dll" },
        { "/lib/QtScriptd4.dll" },
        { "/lib/QtScriptToolsd4.dll" },
        { "/lib/QtSqld4.dll" },
        { "/lib/QtSvgd4.dll" },
        { "/lib/QtTestd4.dll" },
        { "/lib/QtWebKitd4.dll" },
        { "/lib/QtXmld4.dll" },
        { "/lib/QtXmlPatternsd4.dll" },
        { "/plugins/accessible/qtaccessiblecompatwidgetsd4.dll" },
        { "/plugins/accessible/qtaccessiblewidgetsd4.dll" },
        { "/plugins/codecs/qcncodecsd4.dll" },
        { "/plugins/codecs/qjpcodecsd4.dll" },
        { "/plugins/codecs/qkrcodecsd4.dll" },
        { "/plugins/codecs/qtwcodecsd4.dll" },
        { "/plugins/iconengines/qsvgicond4.dll" },
        { "/plugins/imageformats/qgifd4.dll" },
        { "/plugins/imageformats/qjpegd4.dll" },
        { "/plugins/imageformats/qmngd4.dll" },
        { "/plugins/imageformats/qsvgd4.dll" },
        { "/plugins/imageformats/qtiffd4.dll" },
        { "/plugins/sqldrivers/qsqlited4.dll" },
//        { "/plugins/sqldrivers/qsqlodbcd4.dll" }
#else
        { "/bin/assistant" },
        { "/bin/assistant_adp" },
        { "/bin/designer" },
        { "/bin/linguist" },
        { "/bin/lrelease" },
        { "/bin/lupdate" },
        { "/bin/moc" },
        { "/bin/pixeltool" },
        { "/bin/qcollectiongenerator" },
        { "/bin/qdbus" },
        { "/bin/qdbuscpp2xml" },
        { "/bin/qdbuscpp2xml" },
        { "/bin/qdbusviewer" },
        { "/bin/qdbusxml2cpp" },
        { "/bin/qhelpconverter" },
        { "/bin/qhelpgenerator" },
        { "/bin/qmake" },
        { "/bin/qt3to4" },
        { "/bin/qtconfig" },
        { "/bin/qtdemo" },
        { "/bin/rcc" },
        { "/bin/uic" },
        { "/bin/uic3" },
        { "/bin/xmlpatterns" },
        { "/demos/affine/affine" },
        { "/demos/books/books" },
        { "/demos/browser/browser" },
        { "/demos/chip/chip" },
        { "/demos/composition/composition" },
        { "/demos/deform/deform" },
        { "/demos/embeddeddialogs/embeddeddialogs" },
        { "/demos/gradients/gradients" },
        { "/demos/interview/interview" },
        { "/demos/mainwindow/mainwindow" },
        { "/demos/pathstroke/pathstroke" },
        { "/demos/shared/libdemo_shared.a" },
        { "/demos/spreadsheet/spreadsheet" },
        { "/demos/sqlbrowser/sqlbrowser" },
        { "/demos/textedit/textedit" },
        { "/demos/undo/undo" },
        { "/examples/assistant/simpletextviewer/simpletextviewer" },
        { "/examples/dbus/chat/dbus-chat" },
        { "/examples/dbus/complexpingpong/complexping" },
        { "/examples/dbus/complexpingpong/complexpong" },
        { "/examples/dbus/listnames/listnames" },
        { "/examples/dbus/pingpong/ping" },
        { "/examples/dbus/pingpong/pong" },
        { "/examples/dbus/remotecontrolledcar/car/car" },
        { "/examples/dbus/remotecontrolledcar/controller/controller" },
        { "/examples/designer/calculatorbuilder/calculatorbuilder" },
        { "/examples/designer/calculatorform/calculatorform" },
        { "/examples/designer/worldtimeclockbuilder/worldtimeclockbuilder" },
        { "/examples/desktop/screenshot/screenshot" },
        { "/examples/desktop/systray/systray" },
        { "/examples/dialogs/classwizard/classwizard" },
        { "/examples/dialogs/configdialog/configdialog" },
        { "/examples/dialogs/extension/extension" },
        { "/examples/dialogs/findfiles/findfiles" },
        { "/examples/dialogs/licensewizard/licensewizard" },
        { "/examples/dialogs/standarddialogs/standarddialogs" },
        { "/examples/dialogs/tabdialog/tabdialog" },
        { "/examples/dialogs/trivialwizard/trivialwizard" },
        { "/examples/draganddrop/draggableicons/draggableicons" },
        { "/examples/draganddrop/draggabletext/draggabletext" },
        { "/examples/draganddrop/dropsite/dropsite" },
        { "/examples/draganddrop/fridgemagnets/fridgemagnets" },
        { "/examples/draganddrop/puzzle/puzzle" },
        { "/examples/graphicsview/collidingmice/collidingmice" },
        { "/examples/graphicsview/diagramscene/diagramscene" },
        { "/examples/graphicsview/dragdroprobot/dragdroprobot" },
        { "/examples/graphicsview/elasticnodes/elasticnodes" },
        { "/examples/graphicsview/padnavigator/padnavigator" },
        { "/examples/graphicsview/portedasteroids/portedasteroids" },
        { "/examples/graphicsview/portedcanvas/portedcanvas" },
        { "/examples/help/contextsensitivehelp/contextsensitivehelp" },
        { "/examples/help/remotecontrol/remotecontrol" },
        { "/examples/help/simpletextviewer/simpletextviewer" },
        { "/examples/ipc/localfortuneclient/localfortuneclient" },
        { "/examples/ipc/localfortuneserver/localfortuneserver" },
        { "/examples/ipc/sharedmemory/sharedmemory" },
        { "/examples/itemviews/addressbook/addressbook" },
        { "/examples/itemviews/basicsortfiltermodel/basicsortfiltermodel" },
        { "/examples/itemviews/chart/chart" },
        { "/examples/itemviews/coloreditorfactory/coloreditorfactory" },
        { "/examples/itemviews/customsortfiltermodel/customsortfiltermodel" },
        { "/examples/itemviews/dirview/dirview" },
        { "/examples/itemviews/editabletreemodel/editabletreemodel" },
        { "/examples/itemviews/pixelator/pixelator" },
        { "/examples/itemviews/puzzle/puzzle" },
        { "/examples/itemviews/simpledommodel/simpledommodel" },
        { "/examples/itemviews/simpletreemodel/simpletreemodel" },
        { "/examples/itemviews/simplewidgetmapper/simplewidgetmapper" },
        { "/examples/itemviews/spinboxdelegate/spinboxdelegate" },
        { "/examples/itemviews/stardelegate/stardelegate" },
        { "/examples/layouts/basiclayouts/basiclayouts" },
        { "/examples/layouts/borderlayout/borderlayout" },
        { "/examples/layouts/dynamiclayouts/dynamiclayouts" },
        { "/examples/layouts/flowlayout/flowlayout" },
        { "/examples/linguist/arrowpad/arrowpad" },
        { "/examples/linguist/hellotr/hellotr" },
        { "/examples/linguist/trollprint/trollprint" },
        { "/examples/mainwindows/application/application" },
        { "/examples/mainwindows/dockwidgets/dockwidgets" },
        { "/examples/mainwindows/mdi/mdi" },
        { "/examples/mainwindows/menus/menus" },
        { "/examples/mainwindows/recentfiles/recentfiles" },
        { "/examples/mainwindows/sdi/sdi" },
        { "/examples/network/blockingfortuneclient/blockingfortuneclient" },
        { "/examples/network/broadcastreceiver/broadcastreceiver" },
        { "/examples/network/broadcastsender/broadcastsender" },
        { "/examples/network/chat/network-chat" },
        { "/examples/network/download/download" },
        { "/examples/network/downloadmanager/downloadmanager" },
        { "/examples/network/fortuneclient/fortuneclient" },
        { "/examples/network/fortuneserver/fortuneserver" },
        { "/examples/network/ftp/ftp" },
        { "/examples/network/http/http" },
        { "/examples/network/loopback/loopback" },
        { "/examples/network/securesocketclient/securesocketclient" },
        { "/examples/network/threadedfortuneserver/threadedfortuneserver" },
        { "/examples/network/torrent/torrent" },
        { "/examples/opengl/2dpainting/2dpainting" },
        { "/examples/opengl/framebufferobject/framebufferobject" },
        { "/examples/opengl/framebufferobject2/framebufferobject2" },
        { "/examples/opengl/grabber/grabber" },
        { "/examples/opengl/hellogl/hellogl" },
        { "/examples/opengl/overpainting/overpainting" },
        { "/examples/opengl/pbuffers/pbuffers" },
        { "/examples/opengl/pbuffers2/pbuffers2" },
        { "/examples/opengl/samplebuffers/samplebuffers" },
        { "/examples/opengl/textures/textures" },
        { "/examples/painting/basicdrawing/basicdrawing" },
        { "/examples/painting/concentriccircles/concentriccircles" },
        { "/examples/painting/fontsampler/fontsampler" },
        { "/examples/painting/imagecomposition/imagecomposition" },
        { "/examples/painting/painterpaths/painterpaths" },
        { "/examples/painting/svgviewer/svgviewer" },
        { "/examples/painting/transformations/transformations" },
        { "/examples/qtconcurrent/imagescaling/imagescaling" },
        { "/examples/qtconcurrent/map/mapdemo" },
        { "/examples/qtconcurrent/progressdialog/progressdialog" },
        { "/examples/qtconcurrent/runfunction/runfunction" },
        { "/examples/qtconcurrent/wordcount/wordcount" },
        { "/examples/qtestlib/tutorial1/tutorial1" },
        { "/examples/qtestlib/tutorial2/tutorial2" },
        { "/examples/qtestlib/tutorial3/tutorial3" },
        { "/examples/qtestlib/tutorial4/tutorial4" },
        { "/examples/richtext/calendar/calendar" },
        { "/examples/richtext/orderform/orderform" },
        { "/examples/richtext/syntaxhighlighter/syntaxhighlighter" },
        { "/examples/script/calculator/calculator" },
        { "/examples/script/context2d/context2d" },
        { "/examples/script/customclass/customclass" },
        { "/examples/script/defaultprototypes/defaultprototypes" },
        { "/examples/script/helloscript/helloscript" },
        { "/examples/script/marshal/marshal" },
        { "/examples/script/qscript/qscript" },
        { "/examples/script/tetrix/tetrix" },
        { "/examples/sql/cachedtable/cachedtable" },
        { "/examples/sql/drilldown/drilldown" },
        { "/examples/sql/masterdetail/masterdetail" },
        { "/examples/sql/querymodel/querymodel" },
        { "/examples/sql/relationaltablemodel/relationaltablemodel" },
        { "/examples/sql/tablemodel/tablemodel" },
        { "/examples/threads/mandelbrot/mandelbrot" },
        { "/examples/threads/semaphores/semaphores" },
        { "/examples/threads/waitconditions/waitconditions" },
        { "/examples/tools/codecs/codecs" },
        { "/examples/tools/completer/completer" },
        { "/examples/tools/customcompleter/customcompleter" },
        { "/examples/tools/echoplugin/echoplugin" },
        { "/examples/tools/echoplugin/plugin/libechoplugin.so" },
        { "/examples/tools/i18n/i18n" },
        { "/examples/tools/plugandpaint/plugandpaint" },
        { "/examples/tools/plugandpaint/plugins/libpnp_basictools.a" },
        { "/examples/tools/plugandpaint/plugins/libpnp_extrafilters.so" },
        { "/examples/tools/regexp/regexp" },
        { "/examples/tools/settingseditor/settingseditor" },
        { "/examples/tools/styleplugin/styleplugin" },
        { "/examples/tools/styleplugin/styles/libsimplestyleplugin.so" },
        { "/examples/tools/treemodelcompleter/treemodelcompleter" },
        { "/examples/tools/undoframework/undoframework" },
        { "/examples/tutorials/addressbook/part1/part1" },
        { "/examples/tutorials/addressbook/part2/part2" },
        { "/examples/tutorials/addressbook/part3/part3" },
        { "/examples/tutorials/addressbook/part4/part4" },
        { "/examples/tutorials/addressbook/part5/part5" },
        { "/examples/tutorials/addressbook/part6/part6" },
        { "/examples/tutorials/addressbook/part7/part7" },
        { "/examples/tutorials/tutorial/t1/t1" },
        { "/examples/tutorials/tutorial/t10/t10" },
        { "/examples/tutorials/tutorial/t11/t11" },
        { "/examples/tutorials/tutorial/t12/t12" },
        { "/examples/tutorials/tutorial/t13/t13" },
        { "/examples/tutorials/tutorial/t14/t14" },
        { "/examples/tutorials/tutorial/t2/t2" },
        { "/examples/tutorials/tutorial/t3/t3" },
        { "/examples/tutorials/tutorial/t4/t4" },
        { "/examples/tutorials/tutorial/t5/t5" },
        { "/examples/tutorials/tutorial/t6/t6" },
        { "/examples/tutorials/tutorial/t7/t7" },
        { "/examples/tutorials/tutorial/t8/t8" },
        { "/examples/tutorials/tutorial/t9/t9" },
        { "/examples/uitools/multipleinheritance/multipleinheritance" },
        { "/examples/uitools/textfinder/textfinder" },
        { "/examples/webkit/formextractor/formExtractor" },
        { "/examples/webkit/previewer/previewer" },
        { "/examples/widgets/analogclock/analogclock" },
        { "/examples/widgets/calculator/calculator" },
        { "/examples/widgets/calendarwidget/calendarwidget" },
        { "/examples/widgets/charactermap/charactermap" },
        { "/examples/widgets/digitalclock/digitalclock" },
        { "/examples/widgets/groupbox/groupbox" },
        { "/examples/widgets/icons/icons" },
        { "/examples/widgets/imageviewer/imageviewer" },
        { "/examples/widgets/lineedits/lineedits" },
        { "/examples/widgets/movie/movie" },
        { "/examples/widgets/scribble/scribble" },
        { "/examples/widgets/shapedclock/shapedclock" },
        { "/examples/widgets/sliders/sliders" },
        { "/examples/widgets/spinboxes/spinboxes" },
        { "/examples/widgets/styles/styles" },
        { "/examples/widgets/stylesheet/stylesheet" },
        { "/examples/widgets/tablet/tablet" },
        { "/examples/widgets/tetrix/tetrix" },
        { "/examples/widgets/tooltips/tooltips" },
        { "/examples/widgets/validators/validators" },
        { "/examples/widgets/wiggly/wiggly" },
        { "/examples/widgets/windowflags/windowflags" },
        { "/examples/xml/dombookmarks/dombookmarks" },
        { "/examples/xml/rsslisting/rsslisting" },
        { "/examples/xml/saxbookmarks/saxbookmarks" },
        { "/examples/xml/streambookmarks/streambookmarks" },
        { "/examples/xml/xmlstreamlint/xmlstreamlint" },
        { "/examples/xmlpatterns/filetree/filetree" },
        { "/examples/xmlpatterns/qobjectxmlmodel/qobjectxmlmodel" },
        { "/examples/xmlpatterns/recipes/recipes" },
        { "/lib/libQt3Support.so.4.5.0" },
        { "/lib/libQtAssistantClient.so.4.5.0" },
        { "/lib/libQtCLucene.so.4.5.0" },
        { "/lib/libQtCore.so.4.5.0" },
        { "/lib/libQtDBus.so.4.5.0" },
        { "/lib/libQtDesigner.so.4.5.0" },
        { "/lib/libQtDesignerComponents.so.4.5.0" },
        { "/lib/libQtGui.so.4.5.0" },
        { "/lib/libQtHelp.so.4.5.0" },
        { "/lib/libQtNetwork.so.4.5.0" },
        { "/lib/libQtOpenGL.so.4.5.0" },
        { "/lib/libQtScript.so.4.5.0" },
        { "/lib/libQtScriptTools.so.4.5.0" },
        { "/lib/libQtSql.so.4.5.0" },
        { "/lib/libQtSvg.so.4.5.0" },
        { "/lib/libQtTest.so.4.5.0" },
        { "/lib/libQtUiTools.a" },
        { "/lib/libQtWebKit.so.4.5.0" },
        { "/lib/libQtXml.so.4.5.0" },
        { "/lib/libQtXmlPatterns.so.4.5.0" },
        { "/plugins/accessible/libqtaccessiblecompatwidgets.so" },
        { "/plugins/accessible/libqtaccessiblewidgets.so" },
        { "/plugins/codecs/libqcncodecs.so" },
        { "/plugins/codecs/libqjpcodecs.so" },
        { "/plugins/codecs/libqkrcodecs.so" },
        { "/plugins/codecs/libqtwcodecs.so" },
        { "/plugins/designer/libarthurplugin.so" },
        { "/plugins/designer/libcontainerextension.so" },
        { "/plugins/designer/libcustomwidgetplugin.so" },
        { "/plugins/designer/libqt3supportwidgets.so" },
        { "/plugins/designer/libqwebview.so" },
        { "/plugins/designer/libtaskmenuextension.so" },
        { "/plugins/designer/libworldtimeclockplugin.so" },
        { "/plugins/iconengines/libqsvgicon.so" },
        { "/plugins/imageformats/libqgif.so" },
        { "/plugins/imageformats/libqico.so" },
        { "/plugins/imageformats/libqjpeg.so" },
        { "/plugins/imageformats/libqmng.so" },
        { "/plugins/imageformats/libqsvg.so" },
        { "/plugins/imageformats/libqtiff.so" },
        { "/plugins/inputmethods/libqimsw-multi.so" },
        { "/plugins/script/libqtscriptdbus.so" },
        { "/plugins/sqldrivers/libqsqlite.so" },
        { "/plugins/sqldrivers/libqsqlite2.so" },
        { "/plugins/sqldrivers/libqsqlmysql.so" },
        { "/plugins/sqldrivers/libqsqlpsql.so" }
#endif
    };


    for (int i = 0; i < (int)(sizeof(libraries) / sizeof(libraries[0])); i++) {
        // USAGE NOTE: Don't use FILENAME_MAX as the size of an array in which to store a file name!
        // [In some cases] you [may not be able to statically] make an array that big!
        // Use dynamic allocation [..] instead.
        // From http://www.gnu.org/software/libc/manual/html_mono/libc.html#Limits-for-Files

        // Make filename
        char * const fileName = allocFileNameCopyAppend(baseQtPath, libraries[i].fileName);
        logFileName(fileName);

        logDiff(oldSourceBase, baseQtPath);

        // Patch
        BinPatch binFile(fileName);
        binFile.enableInsertReplace(true);
        if (!binFile.patch(oldSourceBase, baseQtPath))
            result = false;

        delete[] fileName;
    }

    return result;
}

void patchQMakeSpec(const char *path)
{
    QString baseQtPath(path);
    const QString fileName(baseQtPath + "/mkspecs/default/qmake.conf");
    QFile f(fileName);
    logFileName(qPrintable(fileName));
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

#ifndef Q_OS_WIN
const char * const textFileFileNames[] =
{
    // *.la
    "/lib/libQtCore.la",
    "/lib/libQt3Support.la",
    "/lib/libQtCLucene.la",
    "/lib/libQtDBus.la",
    "/lib/libQtGui.la",
    "/lib/libQtHelp.la",
    "/lib/libQtNetwork.la",
    "/lib/libQtOpenGL.la",
    "/lib/libQtScript.la",
    "/lib/libQtScriptTools.la",
    "/lib/libQtSql.la",
    "/lib/libQtSvg.la",
    "/lib/libQtTest.la",
    "/lib/libQtWebKit.la",
    "/lib/libQtXml.la",
    "/lib/libQtXmlPatterns.la",

    // *.prl
    "/demos/shared/libdemo_shared.prl",
    "/lib/libQt3Support.prl",
    "/lib/libQtAssistantClient.prl",
    "/lib/libQtCLucene.prl",
    "/lib/libQtCore.prl",
    "/lib/libQtDBus.prl",
    "/lib/libQtDesignerComponents.prl",
    "/lib/libQtDesigner.prl",
    "/lib/libQtGui.prl",
    "/lib/libQtHelp.prl",
    "/lib/libQtNetwork.prl",
    "/lib/libQtOpenGL.prl",
    "/lib/libQtScript.prl",
    "/lib/libQtScriptTools.prl",
    "/lib/libQtSql.prl",
    "/lib/libQtSvg.prl",
    "/lib/libQtTest.prl",
    "/lib/libQtUiTools.prl",
    "/lib/libQtWebKit.prl",
    "/lib/libQtXmlPatterns.prl",
    "/lib/libQtXml.prl",

    // *.pc
    "/lib/pkgconfig/Qt3Support.pc",
    "/lib/pkgconfig/QtAssistantClient.pc",
    "/lib/pkgconfig/QtCLucene.pc",
    "/lib/pkgconfig/QtCore.pc",
    "/lib/pkgconfig/QtDBus.pc",
    "/lib/pkgconfig/QtDesignerComponents.pc",
    "/lib/pkgconfig/QtDesigner.pc",
    "/lib/pkgconfig/QtGui.pc",
    "/lib/pkgconfig/QtHelp.pc",
    "/lib/pkgconfig/QtNetwork.pc",
    "/lib/pkgconfig/QtOpenGL.pc",
    "/lib/pkgconfig/QtScript.pc",
    "/lib/pkgconfig/QtScriptTools.pc",
    "/lib/pkgconfig/QtSql.pc",
    "/lib/pkgconfig/QtSvg.pc",
    "/lib/pkgconfig/QtTest.pc",
    "/lib/pkgconfig/QtUiTools.pc",
    "/lib/pkgconfig/QtWebKit.pc",
    "/lib/pkgconfig/QtXmlPatterns.pc",
    "/lib/pkgconfig/QtXml.pc",

    // misc
    "/mkspecs/qconfig.pri"
};
#endif

void replaceInTextFile(const char * fileName,
        const char * oldText, const char * newText,
        const char * oldText2 = NULL, const char * newText2 = NULL)
{
    const QString errorMessage = QString("Warning: Could not patch file ") + fileName;

    QFile f(fileName);
    if (!f.open(QIODevice::ReadOnly)) {
        std::cerr << qPrintable(errorMessage) << std::endl;
        return;
    }
    QTextStream in(&f);
    QString all = in.readAll();
    f.close();

    all.replace(QString(oldText), QString(newText), Qt::CaseSensitive);
    if (oldText2 && newText2) {
        all = all.replace(QString(oldText2), QString(newText2), Qt::CaseSensitive);
    }

    if (!f.open(QIODevice::WriteOnly)) {
        std::cerr << qPrintable(errorMessage) << std::endl;
        return;
    }
    QTextStream out(&f);
    out << all;
    f.close();
}

void patchTextFiles(const char *newInstallBase)
{
#ifndef Q_OS_WIN
    const char * const baseQtPath = newInstallBase;
    const char * const newSourceBase = newInstallBase;
    const int fileCount = sizeof(textFileFileNames) / sizeof(const char *);
    for (int i = 0; i < fileCount; i++) {
        char * const fileName = allocFileNameCopyAppend(baseQtPath, textFileFileNames[i]);
        logFileName(fileName);
        logDiff(oldSourceBase, newSourceBase);
        logDiff(oldInstallBase, newInstallBase);
        replaceInTextFile(fileName,
                oldSourceBase, newSourceBase,
                oldInstallBase, newInstallBase);
        delete[] fileName;
    }
#endif

    patchQMakeSpec(newInstallBase);
}

int main(int argc, char *args[])
{
    if (argc != 2) {
        printf("Please provide a QTDIR value as parameter.\n"
               "This is also the location where the binaries are expected\n"
               "in the \"/bin\" and \"/plugins\" subdirectories.\n");
        return 1;
    }

    char * const baseQtPath = allocFileNameCopyAppend(args[1], "");

#ifdef Q_OS_WIN
    // Convert backslash to slash
    for (char *p = baseQtPath; *p != '\0'; p++)
        if (*p == '\\')
            *p = '/';
#endif

    // Remove trailing slash(es)
    for (char *p = baseQtPath + strlen(baseQtPath) - 1; p != baseQtPath; p--)
        if (*p == '/')
            *p = '\0';
        else
            break;

    patchTextFiles(baseQtPath);
    patchBinariesWithQtPathes(baseQtPath);
    patchDebugLibrariesWithQtPath(baseQtPath);
    delete[] baseQtPath;
    return 0;
}
