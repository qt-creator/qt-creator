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
#	define QT_INSTALL_DIR "C:/qt-greenhouse/Trolltech/Code_less_create_more/Trolltech/Code_less_create_more/Troll/4.4.3/qt";

	const char * const oldInstallBase = QT_INSTALL_DIR;
	const char * const oldSourceBase = QT_INSTALL_DIR;
#else
    const char * const oldSourceBase = "/home/berlin/dev/qt-4.4.3-temp/qt-x11-opensource-src-4.4.3";
    const char * const oldInstallBase = "/home/berlin/dev/qt-4.4.3-shipping/qt";
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

        {"/plugins/codecs/libqkrcodecs.so.debug", "/plugins/codecs"},
        {"/plugins/codecs/libqtwcodecs.so.debug", "/plugins/codecs"},
        {"/plugins/codecs/libqcncodecs.so.debug", "/plugins/codecs"},
        {"/plugins/codecs/libqjpcodecs.so.debug", "/plugins/codecs"},
        {"/plugins/iconengines/libqsvgicon.so.debug", "/plugins/iconengines"},
        {"/plugins/sqldrivers/libqsqlmysql.so.debug", "/plugins/sqldrivers"},
        {"/plugins/sqldrivers/libqsqlite.so.debug", "/plugins/sqldrivers"},
        {"/plugins/sqldrivers/libqsqlite2.so.debug", "/plugins/sqldrivers"},
        {"/plugins/sqldrivers/libqsqlpsql.so.debug", "/plugins/sqldrivers"},
        {"/plugins/imageformats/libqgif.so.debug", "/plugins/imageformats"},
        {"/plugins/imageformats/libqtiff.so.debug", "/plugins/imageformats"},
        {"/plugins/imageformats/libqsvg.so.debug", "/plugins/imageformats"},
        {"/plugins/imageformats/libqjpeg.so.debug", "/plugins/imageformats"},
        {"/plugins/imageformats/libqico.so.debug", "/plugins/imageformats"},
        {"/plugins/imageformats/libqmng.so.debug", "/plugins/imageformats"},
        {"/plugins/accessible/libqtaccessiblewidgets.so.debug", "/plugins/accessible"},
        {"/plugins/accessible/libqtaccessiblecompatwidgets.so.debug", "/plugins/accessible"},
        {"/plugins/designer/libcontainerextension.so.debug", "/plugins/designer"},
        {"/plugins/designer/libtaskmenuextension.so.debug", "/plugins/designer"},
        {"/plugins/designer/libqwebview.so.debug", "/plugins/designer"},
        {"/plugins/designer/libcustomwidgetplugin.so.debug", "/plugins/designer"},
        {"/plugins/designer/libarthurplugin.so.debug", "/plugins/designer"},
        {"/plugins/designer/libqt3supportwidgets.so.debug", "/plugins/designer"},
        {"/plugins/designer/libworldtimeclockplugin.so.debug", "/plugins/designer"},
        {"/plugins/inputmethods/libqimsw-multi.so.debug", "/plugins/inputmethods"},
        {"/plugins/script/libqtscriptdbus.so.debug", "/plugins/script"},
        {"/examples/draganddrop/puzzle/puzzle.debug", "/examples/draganddrop/puzzle"},
        {"/examples/draganddrop/dropsite/dropsite.debug", "/examples/draganddrop/dropsite"},
        {"/examples/draganddrop/draggabletext/draggabletext.debug", "/examples/draganddrop/draggabletext"},
        {"/examples/draganddrop/draggableicons/draggableicons.debug", "/examples/draganddrop/draggableicons"},
        {"/examples/draganddrop/fridgemagnets/fridgemagnets.debug", "/examples/draganddrop/fridgemagnets"},
        {"/examples/webkit/formextractor/formExtractor.debug", "/examples/webkit/formextractor"},
        {"/examples/webkit/previewer/previewer.debug", "/examples/webkit/previewer"},
        {"/examples/richtext/orderform/orderform.debug", "/examples/richtext/orderform"},
        {"/examples/richtext/calendar/calendar.debug", "/examples/richtext/calendar"},
        {"/examples/richtext/syntaxhighlighter/syntaxhighlighter.debug", "/examples/richtext/syntaxhighlighter"},
        {"/examples/desktop/systray/systray.debug", "/examples/desktop/systray"},
        {"/examples/desktop/screenshot/screenshot.debug", "/examples/desktop/screenshot"},
        {"/examples/linguist/arrowpad/arrowpad.debug", "/examples/linguist/arrowpad"},
        {"/examples/linguist/trollprint/trollprint.debug", "/examples/linguist/trollprint"},
        {"/examples/linguist/hellotr/hellotr.debug", "/examples/linguist/hellotr"},
        {"/examples/ipc/sharedmemory/sharedmemory.debug", "/examples/ipc/sharedmemory"},
        {"/examples/ipc/localfortuneclient/localfortuneclient.debug", "/examples/ipc/localfortuneclient"},
        {"/examples/ipc/localfortuneserver/localfortuneserver.debug", "/examples/ipc/localfortuneserver"},
        {"/examples/threads/waitconditions/waitconditions.debug", "/examples/threads/waitconditions"},
        {"/examples/threads/semaphores/semaphores.debug", "/examples/threads/semaphores"},
        {"/examples/threads/mandelbrot/mandelbrot.debug", "/examples/threads/mandelbrot"},
        {"/examples/dbus/listnames/listnames.debug", "/examples/dbus/listnames"},
        {"/examples/dbus/pingpong/ping.debug", "/examples/dbus/pingpong"},
        {"/examples/dbus/pingpong/pong.debug", "/examples/dbus/pingpong"},
        {"/examples/dbus/complexpingpong/complexping.debug", "/examples/dbus/complexpingpong"},
        {"/examples/dbus/complexpingpong/complexpong.debug", "/examples/dbus/complexpingpong"},
        {"/examples/dbus/chat/dbus-chat.debug", "/examples/dbus/chat"},
        {"/examples/dbus/remotecontrolledcar/car/car.debug", "/examples/dbus/remotecontrolledcar/car"},
        {"/examples/dbus/remotecontrolledcar/controller/controller.debug", "/examples/dbus/remotecontrolledcar/controller"},
        {"/examples/qtconcurrent/wordcount/wordcount.debug", "/examples/qtconcurrent/wordcount"},
        {"/examples/qtconcurrent/runfunction/runfunction.debug", "/examples/qtconcurrent/runfunction"},
        {"/examples/qtconcurrent/progressdialog/progressdialog.debug", "/examples/qtconcurrent/progressdialog"},
        {"/examples/qtconcurrent/map/mapdemo.debug", "/examples/qtconcurrent/map"},
        {"/examples/qtconcurrent/imagescaling/imagescaling.debug", "/examples/qtconcurrent/imagescaling"},
        {"/examples/designer/calculatorform/calculatorform.debug", "/examples/designer/calculatorform"},
        {"/examples/designer/worldtimeclockbuilder/worldtimeclockbuilder.debug", "/examples/designer/worldtimeclockbuilder"},
        {"/examples/designer/calculatorbuilder/calculatorbuilder.debug", "/examples/designer/calculatorbuilder"},
        {"/examples/sql/drilldown/drilldown.debug", "/examples/sql/drilldown"},
        {"/examples/sql/masterdetail/masterdetail.debug", "/examples/sql/masterdetail"},
        {"/examples/sql/tablemodel/tablemodel.debug", "/examples/sql/tablemodel"},
        {"/examples/sql/relationaltablemodel/relationaltablemodel.debug", "/examples/sql/relationaltablemodel"},
        {"/examples/sql/querymodel/querymodel.debug", "/examples/sql/querymodel"},
        {"/examples/sql/cachedtable/cachedtable.debug", "/examples/sql/cachedtable"},
        {"/examples/xmlpatterns/qobjectxmlmodel/qobjectxmlmodel.debug", "/examples/xmlpatterns/qobjectxmlmodel"},
        {"/examples/xmlpatterns/recipes/recipes.debug", "/examples/xmlpatterns/recipes"},
        {"/examples/xmlpatterns/filetree/filetree.debug", "/examples/xmlpatterns/filetree"},
        {"/examples/assistant/simpletextviewer/simpletextviewer.debug", "/examples/assistant/simpletextviewer"},
        {"/examples/help/simpletextviewer/simpletextviewer.debug", "/examples/help/simpletextviewer"},
        {"/examples/help/contextsensitivehelp/contextsensitivehelp.debug", "/examples/help/contextsensitivehelp"},
        {"/examples/help/remotecontrol/remotecontrol.debug", "/examples/help/remotecontrol"},
        {"/examples/opengl/grabber/grabber.debug", "/examples/opengl/grabber"},
        {"/examples/opengl/framebufferobject2/framebufferobject2.debug", "/examples/opengl/framebufferobject2"},
        {"/examples/opengl/hellogl/hellogl.debug", "/examples/opengl/hellogl"},
        {"/examples/opengl/framebufferobject/framebufferobject.debug", "/examples/opengl/framebufferobject"},
        {"/examples/opengl/overpainting/overpainting.debug", "/examples/opengl/overpainting"},
        {"/examples/opengl/pbuffers2/pbuffers2.debug", "/examples/opengl/pbuffers2"},
        {"/examples/opengl/2dpainting/2dpainting.debug", "/examples/opengl/2dpainting"},
        {"/examples/opengl/pbuffers/pbuffers.debug", "/examples/opengl/pbuffers"},
        {"/examples/opengl/samplebuffers/samplebuffers.debug", "/examples/opengl/samplebuffers"},
        {"/examples/opengl/textures/textures.debug", "/examples/opengl/textures"},
        {"/examples/graphicsview/elasticnodes/elasticnodes.debug", "/examples/graphicsview/elasticnodes"},
        {"/examples/graphicsview/collidingmice/collidingmice.debug", "/examples/graphicsview/collidingmice"},
        {"/examples/graphicsview/portedasteroids/portedasteroids.debug", "/examples/graphicsview/portedasteroids"},
        {"/examples/graphicsview/padnavigator/padnavigator.debug", "/examples/graphicsview/padnavigator"},
        {"/examples/graphicsview/portedcanvas/portedcanvas.debug", "/examples/graphicsview/portedcanvas"},
        {"/examples/graphicsview/diagramscene/diagramscene.debug", "/examples/graphicsview/diagramscene"},
        {"/examples/graphicsview/dragdroprobot/dragdroprobot.debug", "/examples/graphicsview/dragdroprobot"},
        {"/examples/mainwindows/menus/menus.debug", "/examples/mainwindows/menus"},
        {"/examples/mainwindows/mdi/mdi.debug", "/examples/mainwindows/mdi"},
        {"/examples/mainwindows/sdi/sdi.debug", "/examples/mainwindows/sdi"},
        {"/examples/mainwindows/recentfiles/recentfiles.debug", "/examples/mainwindows/recentfiles"},
        {"/examples/mainwindows/application/application.debug", "/examples/mainwindows/application"},
        {"/examples/mainwindows/dockwidgets/dockwidgets.debug", "/examples/mainwindows/dockwidgets"},
        {"/examples/widgets/tablet/tablet.debug", "/examples/widgets/tablet"},
        {"/examples/widgets/shapedclock/shapedclock.debug", "/examples/widgets/shapedclock"},
        {"/examples/widgets/styles/styles.debug", "/examples/widgets/styles"},
        {"/examples/widgets/icons/icons.debug", "/examples/widgets/icons"},
        {"/examples/widgets/charactermap/charactermap.debug", "/examples/widgets/charactermap"},
        {"/examples/widgets/sliders/sliders.debug", "/examples/widgets/sliders"},
        {"/examples/widgets/tooltips/tooltips.debug", "/examples/widgets/tooltips"},
        {"/examples/widgets/windowflags/windowflags.debug", "/examples/widgets/windowflags"},
        {"/examples/widgets/stylesheet/stylesheet.debug", "/examples/widgets/stylesheet"},
        {"/examples/widgets/spinboxes/spinboxes.debug", "/examples/widgets/spinboxes"},
        {"/examples/widgets/validators/validators.debug", "/examples/widgets/validators"},
        {"/examples/widgets/calculator/calculator.debug", "/examples/widgets/calculator"},
        {"/examples/widgets/groupbox/groupbox.debug", "/examples/widgets/groupbox"},
        {"/examples/widgets/scribble/scribble.debug", "/examples/widgets/scribble"},
        {"/examples/widgets/imageviewer/imageviewer.debug", "/examples/widgets/imageviewer"},
        {"/examples/widgets/digitalclock/digitalclock.debug", "/examples/widgets/digitalclock"},
        {"/examples/widgets/lineedits/lineedits.debug", "/examples/widgets/lineedits"},
        {"/examples/widgets/movie/movie.debug", "/examples/widgets/movie"},
        {"/examples/widgets/calendarwidget/calendarwidget.debug", "/examples/widgets/calendarwidget"},
        {"/examples/widgets/wiggly/wiggly.debug", "/examples/widgets/wiggly"},
        {"/examples/widgets/analogclock/analogclock.debug", "/examples/widgets/analogclock"},
        {"/examples/widgets/tetrix/tetrix.debug", "/examples/widgets/tetrix"},
        {"/examples/painting/basicdrawing/basicdrawing.debug", "/examples/painting/basicdrawing"},
        {"/examples/painting/svgviewer/svgviewer.debug", "/examples/painting/svgviewer"},
        {"/examples/painting/fontsampler/fontsampler.debug", "/examples/painting/fontsampler"},
        {"/examples/painting/concentriccircles/concentriccircles.debug", "/examples/painting/concentriccircles"},
        {"/examples/painting/painterpaths/painterpaths.debug", "/examples/painting/painterpaths"},
        {"/examples/painting/imagecomposition/imagecomposition.debug", "/examples/painting/imagecomposition"},
        {"/examples/painting/transformations/transformations.debug", "/examples/painting/transformations"},
        {"/examples/tools/customcompleter/customcompleter.debug", "/examples/tools/customcompleter"},
        {"/examples/tools/codecs/codecs.debug", "/examples/tools/codecs"},
        {"/examples/tools/plugandpaint/plugins/libpnp_extrafilters.so.debug", "/examples/tools/plugandpaint/plugins"},
        {"/examples/tools/plugandpaint/plugandpaint.debug", "/examples/tools/plugandpaint"},
        {"/examples/tools/regexp/regexp.debug", "/examples/tools/regexp"},
        {"/examples/tools/undoframework/undoframework.debug", "/examples/tools/undoframework"},
        {"/examples/tools/i18n/i18n.debug", "/examples/tools/i18n"},
        {"/examples/tools/completer/completer.debug", "/examples/tools/completer"},
        {"/examples/tools/echoplugin/plugin/libechoplugin.so.debug", "/examples/tools/echoplugin/plugin"},
        {"/examples/tools/echoplugin/echoplugin.debug", "/examples/tools/echoplugin"},
        {"/examples/tools/styleplugin/styles/libsimplestyleplugin.so.debug", "/examples/tools/styleplugin/styles"},
        {"/examples/tools/styleplugin/styleplugin.debug", "/examples/tools/styleplugin"},
        {"/examples/tools/treemodelcompleter/treemodelcompleter.debug", "/examples/tools/treemodelcompleter"},
        {"/examples/tools/settingseditor/settingseditor.debug", "/examples/tools/settingseditor"},
        {"/examples/network/securesocketclient/securesocketclient.debug", "/examples/network/securesocketclient"},
        {"/examples/network/broadcastreceiver/broadcastreceiver.debug", "/examples/network/broadcastreceiver"},
        {"/examples/network/downloadmanager/downloadmanager.debug", "/examples/network/downloadmanager"},
        {"/examples/network/fortuneserver/fortuneserver.debug", "/examples/network/fortuneserver"},
        {"/examples/network/loopback/loopback.debug", "/examples/network/loopback"},
        {"/examples/network/http/http.debug", "/examples/network/http"},
        {"/examples/network/ftp/ftp.debug", "/examples/network/ftp"},
        {"/examples/network/download/download.debug", "/examples/network/download"},
        {"/examples/network/fortuneclient/fortuneclient.debug", "/examples/network/fortuneclient"},
        {"/examples/network/blockingfortuneclient/blockingfortuneclient.debug", "/examples/network/blockingfortuneclient"},
        {"/examples/network/broadcastsender/broadcastsender.debug", "/examples/network/broadcastsender"},
        {"/examples/network/threadedfortuneserver/threadedfortuneserver.debug", "/examples/network/threadedfortuneserver"},
        {"/examples/network/chat/network-chat.debug", "/examples/network/chat"},
        {"/examples/network/torrent/torrent.debug", "/examples/network/torrent"},
        {"/examples/qtestlib/tutorial4/tutorial4.debug", "/examples/qtestlib/tutorial4"},
        {"/examples/qtestlib/tutorial1/tutorial1.debug", "/examples/qtestlib/tutorial1"},
        {"/examples/qtestlib/tutorial2/tutorial2.debug", "/examples/qtestlib/tutorial2"},
        {"/examples/qtestlib/tutorial3/tutorial3.debug", "/examples/qtestlib/tutorial3"},
        {"/examples/tutorials/tutorial/t3/t3.debug", "/examples/tutorials/tutorial/t3"},
        {"/examples/tutorials/tutorial/t5/t5.debug", "/examples/tutorials/tutorial/t5"},
        {"/examples/tutorials/tutorial/t2/t2.debug", "/examples/tutorials/tutorial/t2"},
        {"/examples/tutorials/tutorial/t11/t11.debug", "/examples/tutorials/tutorial/t11"},
        {"/examples/tutorials/tutorial/t6/t6.debug", "/examples/tutorials/tutorial/t6"},
        {"/examples/tutorials/tutorial/t13/t13.debug", "/examples/tutorials/tutorial/t13"},
        {"/examples/tutorials/tutorial/t12/t12.debug", "/examples/tutorials/tutorial/t12"},
        {"/examples/tutorials/tutorial/t9/t9.debug", "/examples/tutorials/tutorial/t9"},
        {"/examples/tutorials/tutorial/t1/t1.debug", "/examples/tutorials/tutorial/t1"},
        {"/examples/tutorials/tutorial/t4/t4.debug", "/examples/tutorials/tutorial/t4"},
        {"/examples/tutorials/tutorial/t14/t14.debug", "/examples/tutorials/tutorial/t14"},
        {"/examples/tutorials/tutorial/t8/t8.debug", "/examples/tutorials/tutorial/t8"},
        {"/examples/tutorials/tutorial/t7/t7.debug", "/examples/tutorials/tutorial/t7"},
        {"/examples/tutorials/tutorial/t10/t10.debug", "/examples/tutorials/tutorial/t10"},
        {"/examples/tutorials/addressbook/part2/part2.debug", "/examples/tutorials/addressbook/part2"},
        {"/examples/tutorials/addressbook/part5/part5.debug", "/examples/tutorials/addressbook/part5"},
        {"/examples/tutorials/addressbook/part3/part3.debug", "/examples/tutorials/addressbook/part3"},
        {"/examples/tutorials/addressbook/part4/part4.debug", "/examples/tutorials/addressbook/part4"},
        {"/examples/tutorials/addressbook/part7/part7.debug", "/examples/tutorials/addressbook/part7"},
        {"/examples/tutorials/addressbook/part1/part1.debug", "/examples/tutorials/addressbook/part1"},
        {"/examples/tutorials/addressbook/part6/part6.debug", "/examples/tutorials/addressbook/part6"},
        {"/examples/xml/streambookmarks/streambookmarks.debug", "/examples/xml/streambookmarks"},
        {"/examples/xml/saxbookmarks/saxbookmarks.debug", "/examples/xml/saxbookmarks"},
        {"/examples/xml/xmlstreamlint/xmlstreamlint.debug", "/examples/xml/xmlstreamlint"},
        {"/examples/xml/dombookmarks/dombookmarks.debug", "/examples/xml/dombookmarks"},
        {"/examples/xml/rsslisting/rsslisting.debug", "/examples/xml/rsslisting"},
        {"/examples/layouts/dynamiclayouts/dynamiclayouts.debug", "/examples/layouts/dynamiclayouts"},
        {"/examples/layouts/flowlayout/flowlayout.debug", "/examples/layouts/flowlayout"},
        {"/examples/layouts/borderlayout/borderlayout.debug", "/examples/layouts/borderlayout"},
        {"/examples/layouts/basiclayouts/basiclayouts.debug", "/examples/layouts/basiclayouts"},
        {"/examples/dialogs/trivialwizard/trivialwizard.debug", "/examples/dialogs/trivialwizard"},
        {"/examples/dialogs/extension/extension.debug", "/examples/dialogs/extension"},
        {"/examples/dialogs/standarddialogs/standarddialogs.debug", "/examples/dialogs/standarddialogs"},
        {"/examples/dialogs/tabdialog/tabdialog.debug", "/examples/dialogs/tabdialog"},
        {"/examples/dialogs/classwizard/classwizard.debug", "/examples/dialogs/classwizard"},
        {"/examples/dialogs/findfiles/findfiles.debug", "/examples/dialogs/findfiles"},
        {"/examples/dialogs/licensewizard/licensewizard.debug", "/examples/dialogs/licensewizard"},
        {"/examples/dialogs/configdialog/configdialog.debug", "/examples/dialogs/configdialog"},
        {"/examples/itemviews/coloreditorfactory/coloreditorfactory.debug", "/examples/itemviews/coloreditorfactory"},
        {"/examples/itemviews/pixelator/pixelator.debug", "/examples/itemviews/pixelator"},
        {"/examples/itemviews/simplewidgetmapper/simplewidgetmapper.debug", "/examples/itemviews/simplewidgetmapper"},
        {"/examples/itemviews/puzzle/puzzle.debug", "/examples/itemviews/puzzle"},
        {"/examples/itemviews/dirview/dirview.debug", "/examples/itemviews/dirview"},
        {"/examples/itemviews/addressbook/addressbook.debug", "/examples/itemviews/addressbook"},
        {"/examples/itemviews/spinboxdelegate/spinboxdelegate.debug", "/examples/itemviews/spinboxdelegate"},
        {"/examples/itemviews/simpletreemodel/simpletreemodel.debug", "/examples/itemviews/simpletreemodel"},
        {"/examples/itemviews/chart/chart.debug", "/examples/itemviews/chart"},
        {"/examples/itemviews/basicsortfiltermodel/basicsortfiltermodel.debug", "/examples/itemviews/basicsortfiltermodel"},
        {"/examples/itemviews/customsortfiltermodel/customsortfiltermodel.debug", "/examples/itemviews/customsortfiltermodel"},
        {"/examples/itemviews/stardelegate/stardelegate.debug", "/examples/itemviews/stardelegate"},
        {"/examples/itemviews/editabletreemodel/editabletreemodel.debug", "/examples/itemviews/editabletreemodel"},
        {"/examples/itemviews/simpledommodel/simpledommodel.debug", "/examples/itemviews/simpledommodel"},
        {"/examples/uitools/multipleinheritance/multipleinheritance.debug", "/examples/uitools/multipleinheritance"},
        {"/examples/uitools/textfinder/textfinder.debug", "/examples/uitools/textfinder"},
        {"/examples/script/helloscript/helloscript.debug", "/examples/script/helloscript"},
        {"/examples/script/marshal/marshal.debug", "/examples/script/marshal"},
        {"/examples/script/customclass/customclass.debug", "/examples/script/customclass"},
        {"/examples/script/calculator/calculator.debug", "/examples/script/calculator"},
        {"/examples/script/context2d/context2d.debug", "/examples/script/context2d"},
        {"/examples/script/defaultprototypes/defaultprototypes.debug", "/examples/script/defaultprototypes"},
        {"/examples/script/qscript/qscript.debug", "/examples/script/qscript"},
        {"/examples/script/tetrix/tetrix.debug", "/examples/script/tetrix"},
        {"/lib/libQtTest.so.4.4.3.debug", "/lib"},
        {"/lib/libQtDesignerComponents.so.4.4.3.debug", "/lib"},
        {"/lib/libQtScript.so.4.4.3.debug", "/lib"},
        {"/lib/libQtDesigner.so.4.4.3.debug", "/lib"},
        {"/lib/libQtGui.so.4.4.3.debug", "/lib"},
        {"/lib/libQtSvg.so.4.4.3.debug", "/lib"},
        {"/lib/libQtXml.so.4.4.3.debug", "/lib"},
        {"/lib/libQtCLucene.so.4.4.3.debug", "/lib"},
        {"/lib/libQtCore.so.4.4.3.debug", "/lib"},
        {"/lib/libQtDBus.so.4.4.3.debug", "/lib"},
        {"/lib/libQtXmlPatterns.so.4.4.3.debug", "/lib"},
        {"/lib/libQtHelp.so.4.4.3.debug", "/lib"},
        {"/lib/libQtSql.so.4.4.3.debug", "/lib"},
        {"/lib/libQtNetwork.so.4.4.3.debug", "/lib"},
        {"/lib/libQtOpenGL.so.4.4.3.debug", "/lib"},
        {"/lib/libQt3Support.so.4.4.3.debug", "/lib"},
        {"/lib/libQtAssistantClient.so.4.4.3.debug", "/lib"},
        {"/lib/libQtWebKit.so.4.4.3.debug", "/lib"},
        {"/demos/spreadsheet/spreadsheet.debug", "/demos/spreadsheet"},
        {"/demos/composition/composition.debug", "/demos/composition"},
        {"/demos/gradients/gradients.debug", "/demos/gradients"},
        {"/demos/deform/deform.debug", "/demos/deform"},
        {"/demos/embeddeddialogs/embeddeddialogs.debug", "/demos/embeddeddialogs"},
        {"/demos/textedit/textedit.debug", "/demos/textedit"},
        {"/demos/browser/browser.debug", "/demos/browser"},
        {"/demos/interview/interview.debug", "/demos/interview"},
        {"/demos/affine/affine.debug", "/demos/affine"},
        {"/demos/books/books.debug", "/demos/books"},
        {"/demos/chip/chip.debug", "/demos/chip"},
        {"/demos/pathstroke/pathstroke.debug", "/demos/pathstroke"},
        {"/demos/undo/undo.debug", "/demos/undo"},
        {"/demos/sqlbrowser/sqlbrowser.debug", "/demos/sqlbrowser"},
        {"/demos/mainwindow/mainwindow.debug", "/demos/mainwindow"},
        {"/bin/qcollectiongenerator.debug", "/bin"},
        {"/bin/qhelpconverter.debug", "/bin"},
        {"/bin/lupdate.debug", "/bin"},
        {"/bin/moc.debug", "/bin"},
        {"/bin/pixeltool.debug", "/bin"},
        {"/bin/qdbusviewer.debug", "/bin"},
        {"/bin/qtconfig.debug", "/bin"},
        {"/bin/qdbusxml2cpp.debug", "/bin"},
        {"/bin/qdbus.debug", "/bin"},
        {"/bin/uic3.debug", "/bin"},
        {"/bin/qhelpgenerator.debug", "/bin"},
        {"/bin/qt3to4.debug", "/bin"},
        {"/bin/xmlpatterns.debug", "/bin"},
        {"/bin/linguist.debug", "/bin"},
        {"/bin/uic.debug", "/bin"},
        {"/bin/qtdemo.debug", "/bin"},
        {"/bin/lrelease.debug", "/bin"},
        {"/bin/qmake", "/bin"},
        {"/bin/assistant.debug", "/bin"},
        {"/bin/rcc.debug", "/bin"},
        {"/bin/designer.debug", "/bin"},
        {"/bin/assistant_adp.debug", "/bin"},
        {"/bin/qdbuscpp2xml.debug", "/bin"},


        {"/plugins/codecs/libqkrcodecs.so.debug", "/src/plugins/codecs/kr"},
        {"/plugins/codecs/libqtwcodecs.so.debug", "/src/plugins/codecs/tw"},
        {"/plugins/codecs/libqcncodecs.so.debug", "/src/plugins/codecs/cn"},
        {"/plugins/codecs/libqjpcodecs.so.debug", "/src/plugins/codecs/jp"},
        {"/plugins/iconengines/libqsvgicon.so.debug", "/src/plugins/iconengines/svgiconengine"},
        {"/plugins/sqldrivers/libqsqlmysql.so.debug", "/src/plugins/sqldrivers/mysql"},
        {"/plugins/sqldrivers/libqsqlite.so.debug", "/src/plugins/sqldrivers/sqlite"},
        {"/plugins/sqldrivers/libqsqlite2.so.debug", "/src/plugins/sqldrivers/sqlite2"},
        {"/plugins/sqldrivers/libqsqlpsql.so.debug", "/src/plugins/sqldrivers/psql"},
        {"/plugins/imageformats/libqgif.so.debug", "/src/plugins/imageformats/gif"},
        {"/plugins/imageformats/libqtiff.so.debug", "/src/plugins/imageformats/tiff"},
        {"/plugins/imageformats/libqsvg.so.debug", "/src/plugins/imageformats/svg"},
        {"/plugins/imageformats/libqjpeg.so.debug", "/src/plugins/imageformats/jpeg"},
        {"/plugins/imageformats/libqico.so.debug", "/src/plugins/imageformats/ico"},
        {"/plugins/imageformats/libqmng.so.debug", "/src/plugins/imageformats/mng"},
        {"/plugins/accessible/libqtaccessiblewidgets.so.debug", "/src/plugins/accessible/widgets"},
        {"/plugins/accessible/libqtaccessiblecompatwidgets.so.debug", "/src/plugins/accessible/compat"},
        {"/plugins/designer/libcontainerextension.so.debug", "/examples/designer/containerextension"},
        {"/plugins/designer/libtaskmenuextension.so.debug", "/examples/designer/taskmenuextension"},
        {"/plugins/designer/libqwebview.so.debug", "/tools/designer/src/plugins/qwebview"},
        {"/plugins/designer/libcustomwidgetplugin.so.debug", "/examples/designer/customwidgetplugin"},
        {"/plugins/designer/libarthurplugin.so.debug", "/demos/arthurplugin"},
        {"/plugins/designer/libarthurplugin.so.debug", "/demos/shared"},
        {"/plugins/designer/libqt3supportwidgets.so.debug", "/tools/designer/src/plugins/widgets"},
        {"/plugins/designer/libworldtimeclockplugin.so.debug", "/examples/designer/worldtimeclockplugin"},
        {"/plugins/inputmethods/libqimsw-multi.so.debug", "/src/plugins/inputmethods/imsw-multi"},
        {"/plugins/script/libqtscriptdbus.so.debug", "/src/plugins/script/qtdbus"},

        {"/examples/dbus/chat/dbus-chat.debug", "/examples/dbus/dbus-chat"},
        {"/examples/designer/worldtimeclockbuilder/worldtimeclockbuilder.debug", "/tools/designer/src/uitools"},
        {"/examples/designer/calculatorbuilder/calculatorbuilder.debug", "/tools/designer/src/uitools"},
        {"/examples/tools/plugandpaint/plugins/libpnp_extrafilters.so.debug", "/examples/tools/plugandpaintplugins/extrafilters"},
        {"/examples/tools/styleplugin/styles/libsimplestyleplugin.so.debug", "/examples/tools/styleplugin/plugin"},
        {"/examples/network/chat/network-chat.debug", "/examples/network/network-chat"},
        {"/examples/uitools/textfinder/textfinder.debug", "/tools/designer/src/uitools"},
        {"/examples/script/calculator/calculator.debug", "/tools/designer/src/uitools"},
        {"/examples/script/tetrix/tetrix.debug", "/tools/designer/src/uitools"},

        {"/lib/libQtTest.so.4.4.3.debug", "/src/testlib"},
        {"/lib/libQtDesignerComponents.so.4.4.3.debug", "/tools/designer/src/components"},
        {"/lib/libQtScript.so.4.4.3.debug", "/src/script"},
        {"/lib/libQtDesigner.so.4.4.3.debug", "/tools/designer/src/lib"},
        {"/lib/libQtGui.so.4.4.3.debug", "/src/gui"},
        {"/lib/libQtSvg.so.4.4.3.debug", "/src/svg"},
        {"/lib/libQtXml.so.4.4.3.debug", "/src/xml"},
        {"/lib/libQtCLucene.so.4.4.3.debug", "/tools/assistant/lib/fulltextsearch"},
        {"/lib/libQtCore.so.4.4.3.debug", "/src/corelib"},
        {"/lib/libQtDBus.so.4.4.3.debug", "/src/dbus"},
        {"/lib/libQtXmlPatterns.so.4.4.3.debug", "/src/xmlpatterns"},
        {"/lib/libQtHelp.so.4.4.3.debug", "/tools/assistant/lib"},
        {"/lib/libQtSql.so.4.4.3.debug", "/src/sql"},
        {"/lib/libQtNetwork.so.4.4.3.debug", "/src/network"},
        {"/lib/libQtOpenGL.so.4.4.3.debug", "/src/opengl"},
        {"/lib/libQt3Support.so.4.4.3.debug", "/src/qt3support"},
        {"/lib/libQtAssistantClient.so.4.4.3.debug", "/tools/assistant/compat/lib"},
        {"/lib/libQtWebKit.so.4.4.3.debug", "/src/3rdparty/webkit/WebCore"},

        {"/demos/composition/composition.debug", "/demos/shared"},
        {"/demos/gradients/gradients.debug", "/demos/shared"},
        {"/demos/deform/deform.debug", "/demos/shared"},
        {"/demos/browser/browser.debug", "/tools/designer/src/uitools"},
        {"/demos/affine/affine.debug", "/demos/shared"},
        {"/demos/pathstroke/pathstroke.debug", "/demos/shared"},

        {"/bin/qcollectiongenerator.debug", "/tools/assistant/tools/qcollectiongenerator"},
        {"/bin/qhelpconverter.debug", "/tools/assistant/tools/qhelpconverter"},
        {"/bin/lupdate.debug", "/tools/linguist/lupdate"},
        {"/bin/moc.debug", "/src/tools/moc"},
        {"/bin/pixeltool.debug", "/tools/pixeltool"},
        {"/bin/qdbusviewer.debug", "/tools/qdbus/qdbusviewer"},
        {"/bin/qtconfig.debug", "/tools/qtconfig"},
        {"/bin/qdbusxml2cpp.debug", "/tools/qdbus/qdbusxml2cpp"},
        {"/bin/qdbus.debug", "/tools/qdbus/qdbus"},
        {"/bin/uic3.debug", "/src/tools/uic3"},
        {"/bin/qhelpgenerator.debug", "/tools/assistant/tools/qhelpgenerator"},
        {"/bin/qt3to4.debug", "/tools/porting/src"},
        {"/bin/xmlpatterns.debug", "/tools/xmlpatterns"},
        {"/bin/linguist.debug", "/tools/designer/src/uitools"},
        {"/bin/linguist.debug", "/tools/linguist/linguist"},
        {"/bin/uic.debug", "/src/tools/uic"},
        {"/bin/qtdemo.debug", "/demos/qtdemo"},
        {"/bin/lrelease.debug", "/tools/linguist/lrelease"},
        {"/bin/qmake", "/qmake"},
        {"/bin/assistant.debug", "/tools/assistant/tools/assistant"},
        {"/bin/rcc.debug", "/src/tools/rcc"},
        {"/bin/designer.debug", "/tools/designer/src/designer"},
        {"/bin/assistant_adp.debug", "/tools/assistant/compat"},
        {"/bin/qdbuscpp2xml.debug", "/tools/qdbus/qdbuscpp2xml"}
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
