/*
    SPDX-FileCopyrightText: 2018 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: MIT
*/

#ifndef KQUICKSYNTAXHIGHLIGHTINGPLUGIN_H
#define KQUICKSYNTAXHIGHLIGHTINGPLUGIN_H

#include <QQmlEngine>
#include <QQmlExtensionPlugin>

class KQuickSyntaxHighlightingPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")

public:
    void registerTypes(const char *uri) override;
};

#endif // KQUICKSYNTAXHIGHLIGHTINGPLUGIN_H
