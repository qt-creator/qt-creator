/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com
**
****************************************************************************/

#ifndef AUTOTESTPLUGIN_H
#define AUTOTESTPLUGIN_H

#include "autotest_global.h"

#include <extensionsystem/iplugin.h>

namespace Autotest {
namespace Internal {

struct TestSettings;

class AutotestPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "AutoTest.json")

public:
    AutotestPlugin();
    ~AutotestPlugin();

    static AutotestPlugin *instance();

    QSharedPointer<TestSettings> settings() const;

    bool initialize(const QStringList &arguments, QString *errorString);
    void extensionsInitialized();
    ShutdownFlag aboutToShutdown();

private:
    bool checkLicense();
    void initializeMenuEntries();
    QList<QObject *> createTestObjects() const;
    const QSharedPointer<TestSettings> m_settings;
};

} // namespace Internal
} // namespace Autotest

#endif // AUTOTESTPLUGIN_H

