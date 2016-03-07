/****************************************************************************
**
** Copyright (C) 2016 Kläralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
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

#include "valgrindruncontrolfactory.h"

#include "valgrindengine.h"
#include "valgrindsettings.h"
#include "valgrindplugin.h"
#include "callgrindtool.h"
#include "memchecktool.h"

#include <debugger/analyzer/analyzermanager.h>
#include <debugger/analyzer/analyzerrunconfigwidget.h>

#include <utils/qtcassert.h>

using namespace Debugger;
using namespace ProjectExplorer;

namespace Valgrind {
namespace Internal {

class ValgrindRunConfigurationAspect : public IRunConfigurationAspect
{
public:
    ValgrindRunConfigurationAspect(RunConfiguration *parent)
        : IRunConfigurationAspect(parent)
    {
        setProjectSettings(new ValgrindProjectSettings());
        setGlobalSettings(ValgrindPlugin::globalSettings());
        setId(ANALYZER_VALGRIND_SETTINGS);
        setDisplayName(QCoreApplication::translate("Valgrind::Internal::ValgrindRunConfigurationAspect", "Valgrind Settings"));
        setUsingGlobalSettings(true);
        resetProjectToGlobalSettings();
        setRunConfigWidgetCreator([this] { return new AnalyzerRunConfigWidget(this); });
    }

    ValgrindRunConfigurationAspect *create(RunConfiguration *parent) const override
    {
        return new ValgrindRunConfigurationAspect(parent);
    }
};

IRunConfigurationAspect *createValgrindRunConfigurationAspect(RunConfiguration *rc)
{
    return new ValgrindRunConfigurationAspect(rc);
}

} // namespace Internal
} // namespace Valgrind
