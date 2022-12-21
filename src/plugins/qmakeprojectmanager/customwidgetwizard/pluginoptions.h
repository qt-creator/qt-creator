// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>
#include <QList>

namespace QmakeProjectManager {
namespace Internal {

struct PluginOptions {
    QString pluginName;
    QString resourceFile;
    QString collectionClassName;
    QString collectionHeaderFile;
    QString collectionSourceFile;

    struct WidgetOptions {
        enum { LinkLibrary, IncludeProject } sourceType;
        QString widgetLibrary;
        QString widgetProjectFile;
        QString widgetClassName;
        QString widgetHeaderFile;
        QString widgetSourceFile;
        QString widgetBaseClassName;
        QString pluginClassName;
        QString pluginHeaderFile;
        QString pluginSourceFile;
        QString iconFile;
        bool createSkeleton;

        QString group;
        QString toolTip;
        QString whatsThis;
        bool isContainer;

        QString domXml;
    };

    QList<WidgetOptions> widgetOptions;
};

}
}
