// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qdoceditor.h"
#include "qdoclinter.h"

#include <extensionsystem/iplugin.h>

namespace QDoc::Internal {

class QDocPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "QDoc.json")

public:
    QDocPlugin() = default;
    ~QDocPlugin() final = default;

    void initialize() final;
};

void QDocPlugin::initialize()
{
    setupQDocEditor();
    setupQDocLinter();

#ifdef WITH_TESTS
    addTestCreator(createQDocLinterTest);
#endif
}

} // namespace QDoc::Internal

#include "qdocplugin.moc"
