/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
****************************************************************************/

#ifndef QMLPROFILEREXTENDED_H
#define QMLPROFILEREXTENDED_H

#include "qmlprofilerextended_global.h"

#include <extensionsystem/iplugin.h>

namespace QmlProfilerExtended {
namespace Internal {

class QmlProfilerExtendedPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "QmlProfilerExtended.json")

public:
    QmlProfilerExtendedPlugin();
    ~QmlProfilerExtendedPlugin();

    bool initialize(const QStringList &arguments, QString *errorString);
    void extensionsInitialized();
    ShutdownFlag aboutToShutdown();

private slots:
    void triggerAction();
};

} // namespace Internal
} // namespace QmlProfilerExtended

#endif // QMLPROFILEREXTENDED_H

