// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "mytypes.h"

#include <QObject>

namespace ScxmlEditor {

namespace PluginInterface {

class ISCEditor;
class ScxmlDocument;

class ScxmlUiFactory : public QObject
{
    Q_OBJECT

public:
    ScxmlUiFactory(QObject *parent = nullptr);
    ~ScxmlUiFactory() override;

    // Inform document changes
    void documentChanged(DocumentChangeType type, ScxmlDocument *doc);

    // Inform plugins to refresh
    void refresh();

    // Get registered objects
    QObject *object(const QString &type) const;

    // Register objects
    void unregisterObject(const QString &type, QObject *object);
    void registerObject(const QString &type, QObject *object);

    // Check is active
    bool isActive(const QString &type, const QObject *object) const;

private:
    void initPlugins();

    QVector<ISCEditor*> m_plugins;
    QMap<QString, QObject*> m_objects;
};

} // namespace PluginInterface
} // namespace ScxmlEditor
