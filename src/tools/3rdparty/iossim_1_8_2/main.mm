/* Author: Landon Fuller <landonf@plausiblelabs.com>
 * Copyright (c) 2008-2011 Plausible Labs Cooperative, Inc.
 * All rights reserved.
 *
 * See the IOSSIM_LICENSE file in this directory for the license on the source code in this file.
 */
/* derived from https://github.com/phonegap/ios-sim */
#import <AppKit/AppKit.h>
#import "iphonesimulator.h"
#include <QLibrary>
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include <QApplication>
#else
#include <QGuiApplication>
#endif
#include <QString>
#include <QStringList>

/* to do:
 * - try to stop inferior when killed (or communicate with creator to allow killing the inferior)
 * - remove unneeded functionality and streamline a bit
 */

/*
 * Runs the iPhoneSimulator backed by a main runloop.
 */
int main (int argc, char *argv[]) {
    int qtargc = 1;
    char *qtarg = 0;
    if (argc)
        qtarg = argv[0];
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    QApplication a(qtargc, &qtarg);
#else
    QGuiApplication a(qtargc, &qtarg);
#endif


    //NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    QString xcodePath = QLatin1String("/Applications/Xcode.app/Contents/Developer/");
    for (int i = 0; i + 1 < argc; ++i) {
        if (strcmp(argv[i], "--developer-path") == 0)
            xcodePath = QString::fromLocal8Bit(argv[i + 1]);
    }
    if (!xcodePath.endsWith(QLatin1Char('/')))
        xcodePath.append(QLatin1Char('/'));

    /* manual loading of the private deps */
    QStringList deps = QStringList()
        << QLatin1String("/System/Library/PrivateFrameworks/DebugSymbols.framework/Versions/A/DebugSymbols")
        << QLatin1String("/System/Library/PrivateFrameworks/CoreSymbolication.framework/CoreSymbolication")
        << (xcodePath + QLatin1String("../OtherFrameworks/DevToolsFoundation.framework/DevToolsFoundation"));
    foreach (const QString &libPath, deps) {
        QLibrary *lib = new QLibrary(libPath);
        //lib->setLoadHints(QLibrary::ExportExternalSymbolsHint);
        if (!lib->load())
            printf("<msg>error loading %s</msg>", libPath.toUtf8().constData());
    }
    QLibrary *libIPhoneSimulatorRemoteClient = new QLibrary(xcodePath
        + QLatin1String("Platforms/iPhoneSimulator.platform/Developer/Library/PrivateFrameworks/iPhoneSimulatorRemoteClient.framework/iPhoneSimulatorRemoteClient"));
    //libIPhoneSimulatorRemoteClient->setLoadHints(QLibrary::ResolveAllSymbolsHint|QLibrary::ExportExternalSymbolsHint);
    if (!libIPhoneSimulatorRemoteClient->load())
        printf("<msg>error loading  iPhoneSimulatorRemoteClient</msg>");

    iPhoneSimulator *sim = [[iPhoneSimulator alloc] init];

    /* Execute command line handler */
    [sim runWithArgc: argc argv: argv];

    /* Run the loop to handle added input sources, if any */

    int res = a.exec();
    exit(res);
    // [pool release];
    return 0;
}
