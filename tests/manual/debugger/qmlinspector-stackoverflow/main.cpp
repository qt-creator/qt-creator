// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// Reproducer for QTCREATORBUG-33722 / QTCREATORBUG-33740:
// Stack overflow in QmlInspectorAgent::updateObjectTree when debugging.
//
// Run this project under Qt Creator's debugger with
// Debugger → General → "Show QML Object Tree" enabled (the default).
//
// The bug: updateObjectTree() and verifyAndInsertObjectInTree() in
// qmlinspectoragent.cpp use unbounded mutual recursion to walk the
// QML debug protocol's context/object tree.  When a QObject whose
// debug-ID matches the engine's debug-ID appears in the context tree,
// verifyAndInsertObjectInTree (line 462) calls updateObjectTree to
// re-traverse the entire root context, creating infinite recursion:
//
//   verifyAndInsertObjectInTree:462
//     → updateObjectTree:388 → updateObjectTree:391 (×N context levels)
//       → verifyAndInsertObjectInTree:462 → ...  (infinite)
//
// The engine is injected into rootContext.instances() BEFORE loading
// QML, so it is already present when the debug service responds to
// LIST_OBJECTS.  The engine is also exposed as a QML property on
// the root object, making it reachable from the inspector's object
// tree — as happens in complex frameworks (e.g. Application Manager).
//
// Workaround: uncheck Debugger → General → "Show QML Object Tree".

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include <private/qqmlcontext_p.h>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    // Inject the engine into its own root context's instances list
    // BEFORE loading QML.  This ensures the engine is present when the
    // debug service processes LIST_OBJECTS (which may happen on a
    // separate thread as soon as the debugger connects).
    //
    // setContextForObject gives the engine QQmlData with context
    // pointing to rootContext.  buildObjectList() filters instances by
    // QQmlData::context, so the engine will appear at the rootContext
    // level in the context tree sent to Qt Creator.
    QQmlEngine::setContextForObject(&engine, engine.rootContext());
    QQmlContextPrivate::get(engine.rootContext())->appendInstance(&engine);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("DeepNesting", "Main");

    // Also expose the engine as a property on the root QML object so
    // the inspector can reach it through property expansion — matching
    // the pattern used by frameworks that expose the engine to QML.
    if (!engine.rootObjects().isEmpty()) {
        engine.rootObjects().first()->setProperty(
            "engineRef", QVariant::fromValue<QObject *>(&engine));
    }

    return app.exec();
}
