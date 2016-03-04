/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "valgrindplugin.h"

#include "callgrindtool.h"
#include "memchecktool.h"
#include "valgrindruncontrolfactory.h"
#include "valgrindsettings.h"
#include "valgrindconfigwidget.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>

#include <projectexplorer/projectexplorer.h>

#include <QtPlugin>
#include <QCoreApplication>
#include <QPointer>

using namespace Core;
using namespace ProjectExplorer;

namespace Valgrind {
namespace Internal {

static ValgrindGlobalSettings *theGlobalSettings = 0;

class ValgrindOptionsPage : public IOptionsPage
{
public:
    explicit ValgrindOptionsPage()
    {
        setId(ANALYZER_VALGRIND_SETTINGS);
        setDisplayName(QCoreApplication::translate("Valgrind::Internal::ValgrindOptionsPage", "Valgrind"));
        setCategory("T.Analyzer");
        setDisplayCategory(QCoreApplication::translate("Analyzer", "Analyzer"));
        setCategoryIcon(QLatin1String(":/images/analyzer_category.png"));
    }

    QWidget *widget()
    {
        if (!m_widget)
            m_widget = new ValgrindConfigWidget(theGlobalSettings, 0, true);
        return m_widget;
    }

    void apply()
    {
        theGlobalSettings->writeSettings();
    }

    void finish()
    {
        delete m_widget;
    }

private:
    QPointer<QWidget> m_widget;
};

ValgrindPlugin::~ValgrindPlugin()
{
    delete theGlobalSettings;
    theGlobalSettings = 0;
}

bool ValgrindPlugin::initialize(const QStringList &, QString *)
{
    theGlobalSettings = new ValgrindGlobalSettings;
    theGlobalSettings->readSettings();

    addAutoReleasedObject(new ValgrindOptionsPage);
    addAutoReleasedObject(new ValgrindRunControlFactory);

    return true;
}

void ValgrindPlugin::extensionsInitialized()
{
    initMemcheckTool(this);
    initCallgrindTool(this);
}

ExtensionSystem::IPlugin::ShutdownFlag ValgrindPlugin::aboutToShutdown()
{
    destroyCallgrindTool();
    destroyMemcheckTool();
    return SynchronousShutdown;
}

ValgrindGlobalSettings *ValgrindPlugin::globalSettings()
{
    return theGlobalSettings;
}

} // namespace Internal
} // namespace Valgrind
