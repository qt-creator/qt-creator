/****************************************************************************
**
** Copyright (C) 2018 Sergey Morozov
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

#include "cppcheckconstants.h"
#include "cppcheckoptions.h"
#include "cppchecktool.h"
#include "cppchecktrigger.h"

#include <utils/flowlayout.h>
#include <utils/hostosinfo.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>

#include <coreplugin/icore.h>
#include <coreplugin/variablechooser.h>

#include <debugger/analyzer/analyzericons.h>

#include <QCheckBox>
#include <QDir>
#include <QFormLayout>

namespace Cppcheck {
namespace Internal {

class OptionsWidget final : public QWidget
{
    Q_DECLARE_TR_FUNCTIONS(CppcheckOptionsPage)
public:
    explicit OptionsWidget(QWidget *parent = nullptr)
        : QWidget(parent),
          m_binary(new Utils::PathChooser(this)),
          m_customArguments(new QLineEdit(this)),
          m_ignorePatterns(new QLineEdit(this)),
          m_warning(new QCheckBox(tr("Warnings"), this)),
          m_style(new QCheckBox(tr("Style"), this)),
          m_performance(new QCheckBox(tr("Performance"), this)),
          m_portability(new QCheckBox(tr("Portability"), this)),
          m_information(new QCheckBox(tr("Information"), this)),
          m_unusedFunction(new QCheckBox(tr("Unused functions"), this)),
          m_missingInclude(new QCheckBox(tr("Missing include"), this)),
          m_inconclusive(new QCheckBox(tr("Inconclusive errors"), this)),
          m_forceDefines(new QCheckBox(tr("Check all define combinations"), this)),
          m_showOutput(new QCheckBox(tr("Show raw output"), this)),
          m_addIncludePaths(new QCheckBox(tr("Add include paths"), this)),
          m_guessArguments(new QCheckBox(tr("Calculate additional arguments"), this))
    {
        m_binary->setExpectedKind(Utils::PathChooser::ExistingCommand);
        m_binary->setCommandVersionArguments({"--version"});

        auto variableChooser = new Core::VariableChooser(this);
        variableChooser->addSupportedWidget (m_customArguments);

        m_unusedFunction->setToolTip(tr("Disables multithreaded check."));
        m_ignorePatterns->setToolTip(tr("Comma-separated wildcards of full file paths. "
                                        "Files still can be checked if others include them."));
        m_addIncludePaths->setToolTip(tr("Can find missing includes but makes "
                                         "checking slower. Use only when needed."));
        m_guessArguments->setToolTip(tr("Like C++ standard and language."));

        auto layout = new QFormLayout(this);
        layout->addRow(tr("Binary:"), m_binary);

        auto checks = new Utils::FlowLayout;
        layout->addRow(tr("Checks:"), checks);
        checks->addWidget(m_warning);
        checks->addWidget(m_style);
        checks->addWidget(m_performance);
        checks->addWidget(m_portability);
        checks->addWidget(m_information);
        checks->addWidget(m_unusedFunction);
        checks->addWidget(m_missingInclude);

        layout->addRow(tr("Custom arguments:"), m_customArguments);
        layout->addRow(tr("Ignored file patterns:"), m_ignorePatterns);
        auto flags = new Utils::FlowLayout;
        layout->addRow(flags);
        flags->addWidget(m_inconclusive);
        flags->addWidget(m_forceDefines);
        flags->addWidget(m_showOutput);
        flags->addWidget(m_addIncludePaths);
        flags->addWidget(m_guessArguments);
    }

    void load(const CppcheckOptions &options)
    {
        m_binary->setPath(options.binary);
        m_customArguments->setText(options.customArguments);
        m_ignorePatterns->setText(options.ignoredPatterns);
        m_warning->setChecked(options.warning);
        m_style->setChecked(options.style);
        m_performance->setChecked(options.performance);
        m_portability->setChecked(options.portability);
        m_information->setChecked(options.information);
        m_unusedFunction->setChecked(options.unusedFunction);
        m_missingInclude->setChecked(options.missingInclude);
        m_inconclusive->setChecked(options.inconclusive);
        m_forceDefines->setChecked(options.forceDefines);
        m_showOutput->setChecked(options.showOutput);
        m_addIncludePaths->setChecked(options.addIncludePaths);
        m_guessArguments->setChecked(options.guessArguments);
    }

    void save(CppcheckOptions &options) const
    {
        options.binary = m_binary->path();
        options.customArguments = m_customArguments->text();
        options.ignoredPatterns = m_ignorePatterns->text();
        options.warning = m_warning->isChecked();
        options.style = m_style->isChecked();
        options.performance = m_performance->isChecked();
        options.portability = m_portability->isChecked();
        options.information = m_information->isChecked();
        options.unusedFunction = m_unusedFunction->isChecked();
        options.missingInclude = m_missingInclude->isChecked();
        options.inconclusive = m_inconclusive->isChecked();
        options.forceDefines = m_forceDefines->isChecked();
        options.showOutput = m_showOutput->isChecked();
        options.addIncludePaths = m_addIncludePaths->isChecked();
        options.guessArguments = m_guessArguments->isChecked();
    }

private:
    Utils::PathChooser *m_binary = nullptr;
    QLineEdit *m_customArguments = nullptr;
    QLineEdit *m_ignorePatterns = nullptr;
    QCheckBox *m_warning = nullptr;
    QCheckBox *m_style = nullptr;
    QCheckBox *m_performance = nullptr;
    QCheckBox *m_portability = nullptr;
    QCheckBox *m_information = nullptr;
    QCheckBox *m_unusedFunction = nullptr;
    QCheckBox *m_missingInclude = nullptr;
    QCheckBox *m_inconclusive = nullptr;
    QCheckBox *m_forceDefines = nullptr;
    QCheckBox *m_showOutput = nullptr;
    QCheckBox *m_addIncludePaths = nullptr;
    QCheckBox *m_guessArguments = nullptr;
};

CppcheckOptionsPage::CppcheckOptionsPage(CppcheckTool &tool, CppcheckTrigger &trigger):
    m_tool(tool),
    m_trigger(trigger)
{
    setId(Constants::OPTIONS_PAGE_ID);
    setDisplayName(tr("Cppcheck"));
    setCategory("T.Analyzer");
    setDisplayCategory(QCoreApplication::translate("Analyzer", "Analyzer"));
    setCategoryIcon(Analyzer::Icons::SETTINGSCATEGORY_ANALYZER);

    CppcheckOptions options;
    if (Utils::HostOsInfo::isAnyUnixHost()) {
        options.binary = "cppcheck";
    } else {
        QString programFiles = QDir::fromNativeSeparators(
                    QString::fromLocal8Bit(qgetenv("PROGRAMFILES")));
        if (programFiles.isEmpty())
            programFiles = "C:/Program Files";
        options.binary = programFiles + "/Cppcheck/cppcheck.exe";
    }

    load(options);

    m_tool.updateOptions(options);
}

QWidget *CppcheckOptionsPage::widget()
{
    if (!m_widget)
        m_widget = new OptionsWidget;
    m_widget->load(m_tool.options());
    return m_widget.data();
}

void CppcheckOptionsPage::apply()
{
    CppcheckOptions options;
    m_widget->save(options);
    save(options);
    m_tool.updateOptions(options);
    m_trigger.recheck();
}

void CppcheckOptionsPage::finish()
{
}

void CppcheckOptionsPage::save(const CppcheckOptions &options) const
{
    QSettings *s = Core::ICore::settings();
    QTC_ASSERT(s, return);
    s->beginGroup(Constants::SETTINGS_ID);
    s->setValue(Constants::SETTINGS_BINARY, options.binary);
    s->setValue(Constants::SETTINGS_CUSTOM_ARGUMENTS, options.customArguments);
    s->setValue(Constants::SETTINGS_IGNORE_PATTERNS, options.ignoredPatterns);
    s->setValue(Constants::SETTINGS_WARNING, options.warning);
    s->setValue(Constants::SETTINGS_STYLE, options.style);
    s->setValue(Constants::SETTINGS_PERFORMANCE, options.performance);
    s->setValue(Constants::SETTINGS_PORTABILITY, options.portability);
    s->setValue(Constants::SETTINGS_INFORMATION, options.information);
    s->setValue(Constants::SETTINGS_UNUSED_FUNCTION, options.unusedFunction);
    s->setValue(Constants::SETTINGS_MISSING_INCLUDE, options.missingInclude);
    s->setValue(Constants::SETTINGS_INCONCLUSIVE, options.inconclusive);
    s->setValue(Constants::SETTINGS_FORCE_DEFINES, options.forceDefines);
    s->setValue(Constants::SETTINGS_SHOW_OUTPUT, options.showOutput);
    s->setValue(Constants::SETTINGS_ADD_INCLUDE_PATHS, options.addIncludePaths);
    s->setValue(Constants::SETTINGS_GUESS_ARGUMENTS, options.guessArguments);
    s->endGroup();
}

void CppcheckOptionsPage::load(CppcheckOptions &options) const
{
    QSettings *s = Core::ICore::settings();
    QTC_ASSERT(s, return);
    s->beginGroup(Constants::SETTINGS_ID);
    options.binary = s->value(Constants::SETTINGS_BINARY,
                              options.binary).toString();
    options.customArguments = s->value(Constants::SETTINGS_CUSTOM_ARGUMENTS,
                                       options.customArguments).toString();
    options.ignoredPatterns = s->value(Constants::SETTINGS_IGNORE_PATTERNS,
                                       options.ignoredPatterns).toString();
    options.warning = s->value(Constants::SETTINGS_WARNING,
                               options.warning).toBool();
    options.style = s->value(Constants::SETTINGS_STYLE,
                             options.style).toBool();
    options.performance = s->value(Constants::SETTINGS_PERFORMANCE,
                                   options.performance).toBool();
    options.portability = s->value(Constants::SETTINGS_PORTABILITY,
                                   options.portability).toBool();
    options.information = s->value(Constants::SETTINGS_INFORMATION,
                                   options.information).toBool();
    options.unusedFunction = s->value(Constants::SETTINGS_UNUSED_FUNCTION,
                                      options.unusedFunction).toBool();
    options.missingInclude = s->value(Constants::SETTINGS_MISSING_INCLUDE,
                                      options.missingInclude).toBool();
    options.inconclusive = s->value(Constants::SETTINGS_INCONCLUSIVE,
                                    options.inconclusive).toBool();
    options.forceDefines = s->value(Constants::SETTINGS_FORCE_DEFINES,
                                    options.forceDefines).toBool();
    options.showOutput = s->value(Constants::SETTINGS_SHOW_OUTPUT,
                                  options.showOutput).toBool();
    options.addIncludePaths = s->value(Constants::SETTINGS_ADD_INCLUDE_PATHS,
                                       options.addIncludePaths).toBool();
    options.guessArguments = s->value(Constants::SETTINGS_GUESS_ARGUMENTS,
                                      options.guessArguments).toBool();
    s->endGroup();
}

} // namespace Internal
} // namespace Cppcheck
