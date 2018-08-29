/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "clangformatplugin.h"

#include "clangformatconfigwidget.h"
#include "clangformatindenter.h"

#include <utils/qtcassert.h>

#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/dialogs/ioptionspage.h>

#include <cpptools/cpptoolsconstants.h>
#include <cpptools/cppmodelmanager.h>

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectpanelfactory.h>
#include <projectexplorer/target.h>

#include <QAction>
#include <QDebug>
#include <QMainWindow>
#include <QMessageBox>
#include <QMenu>

#include <QtPlugin>

using namespace ProjectExplorer;

namespace ClangFormat {
namespace Internal {

class ClangFormatOptionsPage : public Core::IOptionsPage
{
public:
    explicit ClangFormatOptionsPage()
    {
        setId("Cpp.CodeStyle.ClangFormat");
        setDisplayName(QCoreApplication::translate(
                           "ClangFormat::Internal::ClangFormatOptionsPage",
                           "Clang Format"));
        setCategory(CppTools::Constants::CPP_SETTINGS_CATEGORY);
    }

    QWidget *widget()
    {
        if (!m_widget)
            m_widget = new ClangFormatConfigWidget;
        return m_widget;
    }

    void apply()
    {
        m_widget->apply();
    }

    void finish()
    {
        delete m_widget;
    }

private:
    QPointer<ClangFormatConfigWidget> m_widget;
};

ClangFormatPlugin::ClangFormatPlugin() = default;
ClangFormatPlugin::~ClangFormatPlugin() = default;

bool ClangFormatPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorString);

    m_optionsPage = std::make_unique<ClangFormatOptionsPage>();

    auto panelFactory = new ProjectPanelFactory();
    panelFactory->setPriority(120);
    panelFactory->setDisplayName(tr("Clang Format"));
    panelFactory->setCreateWidgetFunction([](Project *project) {
        return new ClangFormatConfigWidget(project);
    });
    ProjectPanelFactory::registerFactory(panelFactory);

    CppTools::CppModelManager::instance()->setCppIndenterCreator([]() {
        return new ClangFormatIndenter();
    });

    return true;
}

} // namespace Internal
} // namespace ClangFormat
