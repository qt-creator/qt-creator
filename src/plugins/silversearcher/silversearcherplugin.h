// Copyright (C) 2017 Przemyslaw Gorszkowski <pgorszkowski@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace SilverSearcher {
namespace Internal {

class SilverSearcherPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "SilverSearcher.json")

public:
    bool initialize(const QStringList &arguments, QString *errorString) override;

#ifdef WITH_TESTS
private:
    QVector<QObject *> createTestObjects() const override;
#endif
};

} // namespace Internal
} // namespace SilverSearcher
