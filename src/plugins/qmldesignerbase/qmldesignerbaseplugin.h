// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldesignerbase_global.h"

#include <extensionsystem/iplugin.h>

#include <memory>

QT_BEGIN_NAMESPACE
class QStyle;
QT_END_NAMESPACE

namespace QmlDesigner {

class QMLDESIGNERBASE_EXPORT QmlDesignerBasePlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "QmlDesignerBase.json")

public:
    QmlDesignerBasePlugin();
    ~QmlDesignerBasePlugin();

    static class DesignerSettings &settings();
    static QStyle *style();
    static class StudioConfigSettingsPage *studioConfigSettingsPage();

private:
    bool initialize(const QStringList &arguments, QString *errorMessage) override;

private:
    class Data;
    std::unique_ptr<Data> d;
};

} // namespace QmlDesigner
