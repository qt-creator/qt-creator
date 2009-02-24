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
#include <cstdio>
#include <iostream>

#include <QFile>
#include <QRegExp>
#include <QTextStream>
#include <QString>
#include <QtCore/QDebug>

#ifdef Q_OS_WIN
#	define QT_INSTALL_DIR "C:/qt-greenhouse/Trolltech/Code_less_create_more/Trolltech/Code_less_create_more/Troll/4.5.0/qt";

	const char * const oldInstallBase = QT_INSTALL_DIR;
	const char * const oldSourceBase = QT_INSTALL_DIR;
#else
    const char * const oldSourceBase = "/home/berlin/dev/qt-4.5.0-temp/this_path_is_supposed/to_be_very_long/hopefully_this/is_long_enough/qt-x11-opensource-src-4.5.0";
    const char * const oldInstallBase = "/home/berlin/dev/qt-4.5.0-shipping/this_path_is_supposed/to_be_very_long/hopefully_this/is_long_enough/qt";
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
        const char *sourceLocation;
    } libraries[] = {
#ifdef Q_OS_WIN
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
        {"/lib/Qt3Supportd4.dll", "/src/qt3support/"},
        {"/lib/QtCored4.dll", "/src/corelib/"},
        {"/lib/QtGuid4.dll", "/src/gui/"},
        {"/lib/QtHelpd4.dll", "/tools/assistant/lib/"},
        {"/lib/QtNetworkd4.dll", "/src/network/"},
        {"/lib/QtOpenGLd4.dll", "/src/opengl/"},
        {"/lib/QtScriptd4.dll", "/src/script/"},
        {"/lib/QtSqld4.dll", "/src/sql/"},
        {"/lib/QtSvgd4.dll", "/src/svg/"},
        {"/lib/QtTestd4.dll", "/src/testlib/"},
        {"/lib/QtWebKitd4.dll", "/src/3rdparty/webkit/WebCore/"},
        {"/lib/QtXmld4.dll", "/src/xml/"},
        {"/lib/QtXmlPatternsd4.dll", "/src/xmlpatterns/"},
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
#else
        {"/examples/tools/plugandpaint/plugins/libpnp_basictools.a", "/examples/tools/plugandpaintplugins/basictools"},
        {"/lib/libQtUiTools.a", "/tools/designer/src/uitools"},
        {"/demos/shared/libdemo_shared.a", "/demos/shared"},

        {"/plugins/codecs/libqkrcodecs.so", "/plugins/codecs"},
        {"/plugins/codecs/libqtwcodecs.so", "/plugins/codecs"},
        {"/plugins/codecs/libqcncodecs.so", "/plugins/codecs"},
        {"/plugins/codecs/libqjpcodecs.so", "/plugins/codecs"},
        {"/plugins/iconengines/libqsvgicon.so", "/plugins/iconengines"},
        {"/plugins/sqldrivers/libqsqlmysql.so", "/plugins/sqldrivers"},
        {"/plugins/sqldrivers/libqsqlite.so", "/plugins/sqldrivers"},
        {"/plugins/sqldrivers/libqsqlite2.so", "/plugins/sqldrivers"},
        {"/plugins/sqldrivers/libqsqlpsql.so", "/plugins/sqldrivers"},
        {"/plugins/imageformats/libqgif.so", "/plugins/imageformats"},
        {"/plugins/imageformats/libqtiff.so", "/plugins/imageformats"},
        {"/plugins/imageformats/libqsvg.so", "/plugins/imageformats"},
        {"/plugins/imageformats/libqjpeg.so", "/plugins/imageformats"},
        {"/plugins/imageformats/libqico.so", "/plugins/imageformats"},
        {"/plugins/imageformats/libqmng.so", "/plugins/imageformats"},
        {"/plugins/accessible/libqtaccessiblewidgets.so", "/plugins/accessible"},
        {"/plugins/accessible/libqtaccessiblecompatwidgets.so", "/plugins/accessible"},
        {"/plugins/designer/libcontainerextension.so", "/plugins/designer"},
        {"/plugins/designer/libtaskmenuextension.so", "/plugins/designer"},
        {"/plugins/designer/libqwebview.so", "/plugins/designer"},
        {"/plugins/designer/libcustomwidgetplugin.so", "/plugins/designer"},
        {"/plugins/designer/libarthurplugin.so", "/plugins/designer"},
        {"/plugins/designer/libqt3supportwidgets.so", "/plugins/designer"},
        {"/plugins/designer/libworldtimeclockplugin.so", "/plugins/designer"},
        {"/plugins/inputmethods/libqimsw-multi.so", "/plugins/inputmethods"},
        {"/plugins/script/libqtscriptdbus.so", "/plugins/script"},
        {"/examples/draganddrop/puzzle/puzzle", "/examples/draganddrop/puzzle"},
        {"/examples/draganddrop/dropsite/dropsite", "/examples/draganddrop/dropsite"},
        {"/examples/draganddrop/draggabletext/draggabletext", "/examples/draganddrop/draggabletext"},
        {"/examples/draganddrop/draggableicons/draggableicons", "/examples/draganddrop/draggableicons"},
        {"/examples/draganddrop/fridgemagnets/fridgemagnets", "/examples/draganddrop/fridgemagnets"},
        {"/examples/webkit/formextractor/formExtractor", "/examples/webkit/formextractor"},
        {"/examples/webkit/previewer/previewer", "/examples/webkit/previewer"},
        {"/examples/richtext/orderform/orderform", "/examples/richtext/orderform"},
        {"/examples/richtext/calendar/calendar", "/examples/richtext/calendar"},
        {"/examples/richtext/syntaxhighlighter/syntaxhighlighter", "/examples/richtext/syntaxhighlighter"},
        {"/examples/desktop/systray/systray", "/examples/desktop/systray"},
        {"/examples/desktop/screenshot/screenshot", "/examples/desktop/screenshot"},
        {"/examples/linguist/arrowpad/arrowpad", "/examples/linguist/arrowpad"},
        {"/examples/linguist/trollprint/trollprint", "/examples/linguist/trollprint"},
        {"/examples/linguist/hellotr/hellotr", "/examples/linguist/hellotr"},
        {"/examples/ipc/sharedmemory/sharedmemory", "/examples/ipc/sharedmemory"},
        {"/examples/ipc/localfortuneclient/localfortuneclient", "/examples/ipc/localfortuneclient"},
        {"/examples/ipc/localfortuneserver/localfortuneserver", "/examples/ipc/localfortuneserver"},
        {"/examples/threads/waitconditions/waitconditions", "/examples/threads/waitconditions"},
        {"/examples/threads/semaphores/semaphores", "/examples/threads/semaphores"},
        {"/examples/threads/mandelbrot/mandelbrot", "/examples/threads/mandelbrot"},
        {"/examples/dbus/listnames/listnames", "/examples/dbus/listnames"},
        {"/examples/dbus/pingpong/ping", "/examples/dbus/pingpong"},
        {"/examples/dbus/pingpong/pong", "/examples/dbus/pingpong"},
        {"/examples/dbus/complexpingpong/complexping", "/examples/dbus/complexpingpong"},
        {"/examples/dbus/complexpingpong/complexpong", "/examples/dbus/complexpingpong"},
        {"/examples/dbus/chat/dbus-chat", "/examples/dbus/chat"},
        {"/examples/dbus/remotecontrolledcar/car/car", "/examples/dbus/remotecontrolledcar/car"},
        {"/examples/dbus/remotecontrolledcar/controller/controller", "/examples/dbus/remotecontrolledcar/controller"},
        {"/examples/qtconcurrent/wordcount/wordcount", "/examples/qtconcurrent/wordcount"},
        {"/examples/qtconcurrent/runfunction/runfunction", "/examples/qtconcurrent/runfunction"},
        {"/examples/qtconcurrent/progressdialog/progressdialog", "/examples/qtconcurrent/progressdialog"},
        {"/examples/qtconcurrent/map/mapdemo", "/examples/qtconcurrent/map"},
        {"/examples/qtconcurrent/imagescaling/imagescaling", "/examples/qtconcurrent/imagescaling"},
        {"/examples/designer/calculatorform/calculatorform", "/examples/designer/calculatorform"},
        {"/examples/designer/worldtimeclockbuilder/worldtimeclockbuilder", "/examples/designer/worldtimeclockbuilder"},
        {"/examples/designer/calculatorbuilder/calculatorbuilder", "/examples/designer/calculatorbuilder"},
        {"/examples/sql/drilldown/drilldown", "/examples/sql/drilldown"},
        {"/examples/sql/masterdetail/masterdetail", "/examples/sql/masterdetail"},
        {"/examples/sql/tablemodel/tablemodel", "/examples/sql/tablemodel"},
        {"/examples/sql/relationaltablemodel/relationaltablemodel", "/examples/sql/relationaltablemodel"},
        {"/examples/sql/querymodel/querymodel", "/examples/sql/querymodel"},
        {"/examples/sql/cachedtable/cachedtable", "/examples/sql/cachedtable"},
        {"/examples/xmlpatterns/qobjectxmlmodel/qobjectxmlmodel", "/examples/xmlpatterns/qobjectxmlmodel"},
        {"/examples/xmlpatterns/recipes/recipes", "/examples/xmlpatterns/recipes"},
        {"/examples/xmlpatterns/filetree/filetree", "/examples/xmlpatterns/filetree"},
        {"/examples/assistant/simpletextviewer/simpletextviewer", "/examples/assistant/simpletextviewer"},
        {"/examples/help/simpletextviewer/simpletextviewer", "/examples/help/simpletextviewer"},
        {"/examples/help/contextsensitivehelp/contextsensitivehelp", "/examples/help/contextsensitivehelp"},
        {"/examples/help/remotecontrol/remotecontrol", "/examples/help/remotecontrol"},
        {"/examples/opengl/grabber/grabber", "/examples/opengl/grabber"},
        {"/examples/opengl/framebufferobject2/framebufferobject2", "/examples/opengl/framebufferobject2"},
        {"/examples/opengl/hellogl/hellogl", "/examples/opengl/hellogl"},
        {"/examples/opengl/framebufferobject/framebufferobject", "/examples/opengl/framebufferobject"},
        {"/examples/opengl/overpainting/overpainting", "/examples/opengl/overpainting"},
        {"/examples/opengl/pbuffers2/pbuffers2", "/examples/opengl/pbuffers2"},
        {"/examples/opengl/2dpainting/2dpainting", "/examples/opengl/2dpainting"},
        {"/examples/opengl/pbuffers/pbuffers", "/examples/opengl/pbuffers"},
        {"/examples/opengl/samplebuffers/samplebuffers", "/examples/opengl/samplebuffers"},
        {"/examples/opengl/textures/textures", "/examples/opengl/textures"},
        {"/examples/graphicsview/elasticnodes/elasticnodes", "/examples/graphicsview/elasticnodes"},
        {"/examples/graphicsview/collidingmice/collidingmice", "/examples/graphicsview/collidingmice"},
        {"/examples/graphicsview/portedasteroids/portedasteroids", "/examples/graphicsview/portedasteroids"},
        {"/examples/graphicsview/padnavigator/padnavigator", "/examples/graphicsview/padnavigator"},
        {"/examples/graphicsview/portedcanvas/portedcanvas", "/examples/graphicsview/portedcanvas"},
        {"/examples/graphicsview/diagramscene/diagramscene", "/examples/graphicsview/diagramscene"},
        {"/examples/graphicsview/dragdroprobot/dragdroprobot", "/examples/graphicsview/dragdroprobot"},
        {"/examples/mainwindows/menus/menus", "/examples/mainwindows/menus"},
        {"/examples/mainwindows/mdi/mdi", "/examples/mainwindows/mdi"},
        {"/examples/mainwindows/sdi/sdi", "/examples/mainwindows/sdi"},
        {"/examples/mainwindows/recentfiles/recentfiles", "/examples/mainwindows/recentfiles"},
        {"/examples/mainwindows/application/application", "/examples/mainwindows/application"},
        {"/examples/mainwindows/dockwidgets/dockwidgets", "/examples/mainwindows/dockwidgets"},
        {"/examples/widgets/tablet/tablet", "/examples/widgets/tablet"},
        {"/examples/widgets/shapedclock/shapedclock", "/examples/widgets/shapedclock"},
        {"/examples/widgets/styles/styles", "/examples/widgets/styles"},
        {"/examples/widgets/icons/icons", "/examples/widgets/icons"},
        {"/examples/widgets/charactermap/charactermap", "/examples/widgets/charactermap"},
        {"/examples/widgets/sliders/sliders", "/examples/widgets/sliders"},
        {"/examples/widgets/tooltips/tooltips", "/examples/widgets/tooltips"},
        {"/examples/widgets/windowflags/windowflags", "/examples/widgets/windowflags"},
        {"/examples/widgets/stylesheet/stylesheet", "/examples/widgets/stylesheet"},
        {"/examples/widgets/spinboxes/spinboxes", "/examples/widgets/spinboxes"},
        {"/examples/widgets/validators/validators", "/examples/widgets/validators"},
        {"/examples/widgets/calculator/calculator", "/examples/widgets/calculator"},
        {"/examples/widgets/groupbox/groupbox", "/examples/widgets/groupbox"},
        {"/examples/widgets/scribble/scribble", "/examples/widgets/scribble"},
        {"/examples/widgets/imageviewer/imageviewer", "/examples/widgets/imageviewer"},
        {"/examples/widgets/digitalclock/digitalclock", "/examples/widgets/digitalclock"},
        {"/examples/widgets/lineedits/lineedits", "/examples/widgets/lineedits"},
        {"/examples/widgets/movie/movie", "/examples/widgets/movie"},
        {"/examples/widgets/calendarwidget/calendarwidget", "/examples/widgets/calendarwidget"},
        {"/examples/widgets/wiggly/wiggly", "/examples/widgets/wiggly"},
        {"/examples/widgets/analogclock/analogclock", "/examples/widgets/analogclock"},
        {"/examples/widgets/tetrix/tetrix", "/examples/widgets/tetrix"},
        {"/examples/painting/basicdrawing/basicdrawing", "/examples/painting/basicdrawing"},
        {"/examples/painting/svgviewer/svgviewer", "/examples/painting/svgviewer"},
        {"/examples/painting/fontsampler/fontsampler", "/examples/painting/fontsampler"},
        {"/examples/painting/concentriccircles/concentriccircles", "/examples/painting/concentriccircles"},
        {"/examples/painting/painterpaths/painterpaths", "/examples/painting/painterpaths"},
        {"/examples/painting/imagecomposition/imagecomposition", "/examples/painting/imagecomposition"},
        {"/examples/painting/transformations/transformations", "/examples/painting/transformations"},
        {"/examples/tools/customcompleter/customcompleter", "/examples/tools/customcompleter"},
        {"/examples/tools/codecs/codecs", "/examples/tools/codecs"},
        {"/examples/tools/plugandpaint/plugins/libpnp_extrafilters.so", "/examples/tools/plugandpaint/plugins"},
        {"/examples/tools/plugandpaint/plugandpaint", "/examples/tools/plugandpaint"},
        {"/examples/tools/regexp/regexp", "/examples/tools/regexp"},
        {"/examples/tools/undoframework/undoframework", "/examples/tools/undoframework"},
        {"/examples/tools/i18n/i18n", "/examples/tools/i18n"},
        {"/examples/tools/completer/completer", "/examples/tools/completer"},
        {"/examples/tools/echoplugin/plugin/libechoplugin.so", "/examples/tools/echoplugin/plugin"},
        {"/examples/tools/echoplugin/echoplugin", "/examples/tools/echoplugin"},
        {"/examples/tools/styleplugin/styles/libsimplestyleplugin.so", "/examples/tools/styleplugin/styles"},
        {"/examples/tools/styleplugin/styleplugin", "/examples/tools/styleplugin"},
        {"/examples/tools/treemodelcompleter/treemodelcompleter", "/examples/tools/treemodelcompleter"},
        {"/examples/tools/settingseditor/settingseditor", "/examples/tools/settingseditor"},
        {"/examples/network/securesocketclient/securesocketclient", "/examples/network/securesocketclient"},
        {"/examples/network/broadcastreceiver/broadcastreceiver", "/examples/network/broadcastreceiver"},
        {"/examples/network/downloadmanager/downloadmanager", "/examples/network/downloadmanager"},
        {"/examples/network/fortuneserver/fortuneserver", "/examples/network/fortuneserver"},
        {"/examples/network/loopback/loopback", "/examples/network/loopback"},
        {"/examples/network/http/http", "/examples/network/http"},
        {"/examples/network/ftp/ftp", "/examples/network/ftp"},
        {"/examples/network/download/download", "/examples/network/download"},
        {"/examples/network/fortuneclient/fortuneclient", "/examples/network/fortuneclient"},
        {"/examples/network/blockingfortuneclient/blockingfortuneclient", "/examples/network/blockingfortuneclient"},
        {"/examples/network/broadcastsender/broadcastsender", "/examples/network/broadcastsender"},
        {"/examples/network/threadedfortuneserver/threadedfortuneserver", "/examples/network/threadedfortuneserver"},
        {"/examples/network/chat/network-chat", "/examples/network/chat"},
        {"/examples/network/torrent/torrent", "/examples/network/torrent"},
        {"/examples/qtestlib/tutorial4/tutorial4", "/examples/qtestlib/tutorial4"},
        {"/examples/qtestlib/tutorial1/tutorial1", "/examples/qtestlib/tutorial1"},
        {"/examples/qtestlib/tutorial2/tutorial2", "/examples/qtestlib/tutorial2"},
        {"/examples/qtestlib/tutorial3/tutorial3", "/examples/qtestlib/tutorial3"},
        {"/examples/tutorials/tutorial/t3/t3", "/examples/tutorials/tutorial/t3"},
        {"/examples/tutorials/tutorial/t5/t5", "/examples/tutorials/tutorial/t5"},
        {"/examples/tutorials/tutorial/t2/t2", "/examples/tutorials/tutorial/t2"},
        {"/examples/tutorials/tutorial/t11/t11", "/examples/tutorials/tutorial/t11"},
        {"/examples/tutorials/tutorial/t6/t6", "/examples/tutorials/tutorial/t6"},
        {"/examples/tutorials/tutorial/t13/t13", "/examples/tutorials/tutorial/t13"},
        {"/examples/tutorials/tutorial/t12/t12", "/examples/tutorials/tutorial/t12"},
        {"/examples/tutorials/tutorial/t9/t9", "/examples/tutorials/tutorial/t9"},
        {"/examples/tutorials/tutorial/t1/t1", "/examples/tutorials/tutorial/t1"},
        {"/examples/tutorials/tutorial/t4/t4", "/examples/tutorials/tutorial/t4"},
        {"/examples/tutorials/tutorial/t14/t14", "/examples/tutorials/tutorial/t14"},
        {"/examples/tutorials/tutorial/t8/t8", "/examples/tutorials/tutorial/t8"},
        {"/examples/tutorials/tutorial/t7/t7", "/examples/tutorials/tutorial/t7"},
        {"/examples/tutorials/tutorial/t10/t10", "/examples/tutorials/tutorial/t10"},
        {"/examples/tutorials/addressbook/part2/part2", "/examples/tutorials/addressbook/part2"},
        {"/examples/tutorials/addressbook/part5/part5", "/examples/tutorials/addressbook/part5"},
        {"/examples/tutorials/addressbook/part3/part3", "/examples/tutorials/addressbook/part3"},
        {"/examples/tutorials/addressbook/part4/part4", "/examples/tutorials/addressbook/part4"},
        {"/examples/tutorials/addressbook/part7/part7", "/examples/tutorials/addressbook/part7"},
        {"/examples/tutorials/addressbook/part1/part1", "/examples/tutorials/addressbook/part1"},
        {"/examples/tutorials/addressbook/part6/part6", "/examples/tutorials/addressbook/part6"},
        {"/examples/xml/streambookmarks/streambookmarks", "/examples/xml/streambookmarks"},
        {"/examples/xml/saxbookmarks/saxbookmarks", "/examples/xml/saxbookmarks"},
        {"/examples/xml/xmlstreamlint/xmlstreamlint", "/examples/xml/xmlstreamlint"},
        {"/examples/xml/dombookmarks/dombookmarks", "/examples/xml/dombookmarks"},
        {"/examples/xml/rsslisting/rsslisting", "/examples/xml/rsslisting"},
        {"/examples/layouts/dynamiclayouts/dynamiclayouts", "/examples/layouts/dynamiclayouts"},
        {"/examples/layouts/flowlayout/flowlayout", "/examples/layouts/flowlayout"},
        {"/examples/layouts/borderlayout/borderlayout", "/examples/layouts/borderlayout"},
        {"/examples/layouts/basiclayouts/basiclayouts", "/examples/layouts/basiclayouts"},
        {"/examples/dialogs/trivialwizard/trivialwizard", "/examples/dialogs/trivialwizard"},
        {"/examples/dialogs/extension/extension", "/examples/dialogs/extension"},
        {"/examples/dialogs/standarddialogs/standarddialogs", "/examples/dialogs/standarddialogs"},
        {"/examples/dialogs/tabdialog/tabdialog", "/examples/dialogs/tabdialog"},
        {"/examples/dialogs/classwizard/classwizard", "/examples/dialogs/classwizard"},
        {"/examples/dialogs/findfiles/findfiles", "/examples/dialogs/findfiles"},
        {"/examples/dialogs/licensewizard/licensewizard", "/examples/dialogs/licensewizard"},
        {"/examples/dialogs/configdialog/configdialog", "/examples/dialogs/configdialog"},
        {"/examples/itemviews/coloreditorfactory/coloreditorfactory", "/examples/itemviews/coloreditorfactory"},
        {"/examples/itemviews/pixelator/pixelator", "/examples/itemviews/pixelator"},
        {"/examples/itemviews/simplewidgetmapper/simplewidgetmapper", "/examples/itemviews/simplewidgetmapper"},
        {"/examples/itemviews/puzzle/puzzle", "/examples/itemviews/puzzle"},
        {"/examples/itemviews/dirview/dirview", "/examples/itemviews/dirview"},
        {"/examples/itemviews/addressbook/addressbook", "/examples/itemviews/addressbook"},
        {"/examples/itemviews/spinboxdelegate/spinboxdelegate", "/examples/itemviews/spinboxdelegate"},
        {"/examples/itemviews/simpletreemodel/simpletreemodel", "/examples/itemviews/simpletreemodel"},
        {"/examples/itemviews/chart/chart", "/examples/itemviews/chart"},
        {"/examples/itemviews/basicsortfiltermodel/basicsortfiltermodel", "/examples/itemviews/basicsortfiltermodel"},
        {"/examples/itemviews/customsortfiltermodel/customsortfiltermodel", "/examples/itemviews/customsortfiltermodel"},
        {"/examples/itemviews/stardelegate/stardelegate", "/examples/itemviews/stardelegate"},
        {"/examples/itemviews/editabletreemodel/editabletreemodel", "/examples/itemviews/editabletreemodel"},
        {"/examples/itemviews/simpledommodel/simpledommodel", "/examples/itemviews/simpledommodel"},
        {"/examples/uitools/multipleinheritance/multipleinheritance", "/examples/uitools/multipleinheritance"},
        {"/examples/uitools/textfinder/textfinder", "/examples/uitools/textfinder"},
        {"/examples/script/helloscript/helloscript", "/examples/script/helloscript"},
        {"/examples/script/marshal/marshal", "/examples/script/marshal"},
        {"/examples/script/customclass/customclass", "/examples/script/customclass"},
        {"/examples/script/calculator/calculator", "/examples/script/calculator"},
        {"/examples/script/context2d/context2d", "/examples/script/context2d"},
        {"/examples/script/defaultprototypes/defaultprototypes", "/examples/script/defaultprototypes"},
        {"/examples/script/qscript/qscript", "/examples/script/qscript"},
        {"/examples/script/tetrix/tetrix", "/examples/script/tetrix"},
        {"/lib/libQtTest.so.4.5.0", "/lib"},
        {"/lib/libQtDesignerComponents.so.4.5.0", "/lib"},
        {"/lib/libQtScript.so.4.5.0", "/lib"},
        {"/lib/libQtDesigner.so.4.5.0", "/lib"},
        {"/lib/libQtGui.so.4.5.0", "/lib"},
        {"/lib/libQtSvg.so.4.5.0", "/lib"},
        {"/lib/libQtXml.so.4.5.0", "/lib"},
        {"/lib/libQtCLucene.so.4.5.0", "/lib"},
        {"/lib/libQtCore.so.4.5.0", "/lib"},
        {"/lib/libQtDBus.so.4.5.0", "/lib"},
        {"/lib/libQtXmlPatterns.so.4.5.0", "/lib"},
        {"/lib/libQtHelp.so.4.5.0", "/lib"},
        {"/lib/libQtSql.so.4.5.0", "/lib"},
        {"/lib/libQtNetwork.so.4.5.0", "/lib"},
        {"/lib/libQtOpenGL.so.4.5.0", "/lib"},
        {"/lib/libQt3Support.so.4.5.0", "/lib"},
        {"/lib/libQtAssistantClient.so.4.5.0", "/lib"},
        {"/lib/libQtWebKit.so.4.5.0", "/lib"},
        {"/demos/spreadsheet/spreadsheet", "/demos/spreadsheet"},
        {"/demos/composition/composition", "/demos/composition"},
        {"/demos/gradients/gradients", "/demos/gradients"},
        {"/demos/deform/deform", "/demos/deform"},
        {"/demos/embeddeddialogs/embeddeddialogs", "/demos/embeddeddialogs"},
        {"/demos/textedit/textedit", "/demos/textedit"},
        {"/demos/browser/browser", "/demos/browser"},
        {"/demos/interview/interview", "/demos/interview"},
        {"/demos/affine/affine", "/demos/affine"},
        {"/demos/books/books", "/demos/books"},
        {"/demos/chip/chip", "/demos/chip"},
        {"/demos/pathstroke/pathstroke", "/demos/pathstroke"},
        {"/demos/undo/undo", "/demos/undo"},
        {"/demos/sqlbrowser/sqlbrowser", "/demos/sqlbrowser"},
        {"/demos/mainwindow/mainwindow", "/demos/mainwindow"},
        {"/bin/qcollectiongenerator", "/bin"},
        {"/bin/qhelpconverter", "/bin"},
        {"/bin/lupdate", "/bin"},
        {"/bin/moc", "/bin"},
        {"/bin/pixeltool", "/bin"},
        {"/bin/qdbusviewer", "/bin"},
        {"/bin/qtconfig", "/bin"},
        {"/bin/qdbusxml2cpp", "/bin"},
        {"/bin/qdbus", "/bin"},
        {"/bin/uic3", "/bin"},
        {"/bin/qhelpgenerator", "/bin"},
        {"/bin/qt3to4", "/bin"},
        {"/bin/xmlpatterns", "/bin"},
        {"/bin/linguist", "/bin"},
        {"/bin/uic", "/bin"},
        {"/bin/qtdemo", "/bin"},
        {"/bin/lrelease", "/bin"},
        {"/bin/qmake", "/bin"},
        {"/bin/assistant", "/bin"},
        {"/bin/rcc", "/bin"},
        {"/bin/designer", "/bin"},
        {"/bin/assistant_adp", "/bin"},
        {"/bin/qdbuscpp2xml", "/bin"},


        {"/plugins/codecs/libqkrcodecs.so", "/src/plugins/codecs/kr"},
        {"/plugins/codecs/libqtwcodecs.so", "/src/plugins/codecs/tw"},
        {"/plugins/codecs/libqcncodecs.so", "/src/plugins/codecs/cn"},
        {"/plugins/codecs/libqjpcodecs.so", "/src/plugins/codecs/jp"},
        {"/plugins/iconengines/libqsvgicon.so", "/src/plugins/iconengines/svgiconengine"},
        {"/plugins/sqldrivers/libqsqlmysql.so", "/src/plugins/sqldrivers/mysql"},
        {"/plugins/sqldrivers/libqsqlite.so", "/src/plugins/sqldrivers/sqlite"},
        {"/plugins/sqldrivers/libqsqlite2.so", "/src/plugins/sqldrivers/sqlite2"},
        {"/plugins/sqldrivers/libqsqlpsql.so", "/src/plugins/sqldrivers/psql"},
        {"/plugins/imageformats/libqgif.so", "/src/plugins/imageformats/gif"},
        {"/plugins/imageformats/libqtiff.so", "/src/plugins/imageformats/tiff"},
        {"/plugins/imageformats/libqsvg.so", "/src/plugins/imageformats/svg"},
        {"/plugins/imageformats/libqjpeg.so", "/src/plugins/imageformats/jpeg"},
        {"/plugins/imageformats/libqico.so", "/src/plugins/imageformats/ico"},
        {"/plugins/imageformats/libqmng.so", "/src/plugins/imageformats/mng"},
        {"/plugins/accessible/libqtaccessiblewidgets.so", "/src/plugins/accessible/widgets"},
        {"/plugins/accessible/libqtaccessiblecompatwidgets.so", "/src/plugins/accessible/compat"},
        {"/plugins/designer/libcontainerextension.so", "/examples/designer/containerextension"},
        {"/plugins/designer/libtaskmenuextension.so", "/examples/designer/taskmenuextension"},
        {"/plugins/designer/libqwebview.so", "/tools/designer/src/plugins/qwebview"},
        {"/plugins/designer/libcustomwidgetplugin.so", "/examples/designer/customwidgetplugin"},
        {"/plugins/designer/libarthurplugin.so", "/demos/arthurplugin"},
        {"/plugins/designer/libarthurplugin.so", "/demos/shared"},
        {"/plugins/designer/libqt3supportwidgets.so", "/tools/designer/src/plugins/widgets"},
        {"/plugins/designer/libworldtimeclockplugin.so", "/examples/designer/worldtimeclockplugin"},
        {"/plugins/inputmethods/libqimsw-multi.so", "/src/plugins/inputmethods/imsw-multi"},
        {"/plugins/script/libqtscriptdbus.so", "/src/plugins/script/qtdbus"},

        {"/examples/dbus/chat/dbus-chat", "/examples/dbus/dbus-chat"},
        {"/examples/designer/worldtimeclockbuilder/worldtimeclockbuilder", "/tools/designer/src/uitools"},
        {"/examples/designer/calculatorbuilder/calculatorbuilder", "/tools/designer/src/uitools"},
        {"/examples/tools/plugandpaint/plugins/libpnp_extrafilters.so", "/examples/tools/plugandpaintplugins/extrafilters"},
        {"/examples/tools/styleplugin/styles/libsimplestyleplugin.so", "/examples/tools/styleplugin/plugin"},
        {"/examples/network/chat/network-chat", "/examples/network/network-chat"},
        {"/examples/uitools/textfinder/textfinder", "/tools/designer/src/uitools"},
        {"/examples/script/calculator/calculator", "/tools/designer/src/uitools"},
        {"/examples/script/tetrix/tetrix", "/tools/designer/src/uitools"},

        {"/lib/libQtTest.so.4.5.0", "/src/testlib"},
        {"/lib/libQtDesignerComponents.so.4.5.0", "/tools/designer/src/components"},
        {"/lib/libQtScript.so.4.5.0", "/src/script"},
        {"/lib/libQtDesigner.so.4.5.0", "/tools/designer/src/lib"},
        {"/lib/libQtGui.so.4.5.0", "/src/gui"},
        {"/lib/libQtSvg.so.4.5.0", "/src/svg"},
        {"/lib/libQtXml.so.4.5.0", "/src/xml"},
        {"/lib/libQtCLucene.so.4.5.0", "/tools/assistant/lib/fulltextsearch"},
        {"/lib/libQtCore.so.4.5.0", "/src/corelib"},
        {"/lib/libQtDBus.so.4.5.0", "/src/dbus"},
        {"/lib/libQtXmlPatterns.so.4.5.0", "/src/xmlpatterns"},
        {"/lib/libQtHelp.so.4.5.0", "/tools/assistant/lib"},
        {"/lib/libQtSql.so.4.5.0", "/src/sql"},
        {"/lib/libQtNetwork.so.4.5.0", "/src/network"},
        {"/lib/libQtOpenGL.so.4.5.0", "/src/opengl"},
        {"/lib/libQt3Support.so.4.5.0", "/src/qt3support"},
        {"/lib/libQtAssistantClient.so.4.5.0", "/tools/assistant/compat/lib"},
        {"/lib/libQtWebKit.so.4.5.0", "/src/3rdparty/webkit/WebCore"},

        {"/demos/composition/composition", "/demos/shared"},
        {"/demos/gradients/gradients", "/demos/shared"},
        {"/demos/deform/deform", "/demos/shared"},
        {"/demos/browser/browser", "/tools/designer/src/uitools"},
        {"/demos/affine/affine", "/demos/shared"},
        {"/demos/pathstroke/pathstroke", "/demos/shared"},

        {"/bin/qcollectiongenerator", "/tools/assistant/tools/qcollectiongenerator"},
        {"/bin/qhelpconverter", "/tools/assistant/tools/qhelpconverter"},
        {"/bin/lupdate", "/tools/linguist/lupdate"},
        {"/bin/moc", "/src/tools/moc"},
        {"/bin/pixeltool", "/tools/pixeltool"},
        {"/bin/qdbusviewer", "/tools/qdbus/qdbusviewer"},
        {"/bin/qtconfig", "/tools/qtconfig"},
        {"/bin/qdbusxml2cpp", "/tools/qdbus/qdbusxml2cpp"},
        {"/bin/qdbus", "/tools/qdbus/qdbus"},
        {"/bin/uic3", "/src/tools/uic3"},
        {"/bin/qhelpgenerator", "/tools/assistant/tools/qhelpgenerator"},
        {"/bin/qt3to4", "/tools/porting/src"},
        {"/bin/xmlpatterns", "/tools/xmlpatterns"},
        {"/bin/linguist", "/tools/designer/src/uitools"},
        {"/bin/linguist", "/tools/linguist/linguist"},
        {"/bin/uic", "/src/tools/uic"},
        {"/bin/qtdemo", "/demos/qtdemo"},
        {"/bin/lrelease", "/tools/linguist/lrelease"},
        {"/bin/qmake", "/qmake"},
        {"/bin/assistant", "/tools/assistant/tools/assistant"},
        {"/bin/rcc", "/src/tools/rcc"},
        {"/bin/designer", "/tools/designer/src/designer"},
        {"/bin/assistant_adp", "/tools/assistant/compat"},
        {"/bin/qdbuscpp2xml", "/tools/qdbus/qdbuscpp2xml"}
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

        // Make old source path array
        char * const oldSourcePath = allocFileNameCopyAppend(oldSourceBase,
                libraries[i].sourceLocation);

        // Make new source path array
        char * const newSourcePath = allocFileNameCopyAppend(baseQtPath,
                libraries[i].sourceLocation);

        logDiff(oldSourcePath, newSourcePath);

        // Patch
        BinPatch binFile(fileName);
        if (!binFile.patch(oldSourcePath, newSourcePath)) {
            result = false;
        }

        delete[] fileName;
        delete[] oldSourcePath;
        delete[] newSourcePath;
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
    if ((oldText2 != NULL) && (newText2 != NULL)) {
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
