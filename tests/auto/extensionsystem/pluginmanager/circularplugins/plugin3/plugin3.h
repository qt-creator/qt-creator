// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

#include <QObject>
#include <QtGlobal>

#if defined(PLUGIN3_LIBRARY)
#  define PLUGIN3_EXPORT Q_DECL_EXPORT
#elif defined(PLUGIN3_STATIC_LIBRARY)
#  define PLUGIN3_EXPORT
#else
#  define PLUGIN3_EXPORT Q_DECL_IMPORT
#endif

namespace Plugin3 {

class PLUGIN3_EXPORT MyPlugin3 : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "plugin" FILE "plugin3.json")

public:
    MyPlugin3();

    bool initialize(const QStringList &arguments, QString *errorString);
};

} // namespace Plugin3
