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

#include "cdboptionspage.h"
#include "cdbengine.h"
#include <debugger/commonoptionspage.h>
#include <debugger/debuggeractions.h>
#include <debugger/debuggercore.h>
#include <debugger/debuggerinternalconstants.h>
#include <debugger/shared/cdbsymbolpathlisteditor.h>

#include <coreplugin/icore.h>

#include <QDialogButtonBox>
#include <QTextStream>

namespace Debugger {
namespace Internal {

const char *CdbOptionsPage::crtDbgReport = "CrtDbgReport";

struct EventsDescription {
    const char *abbreviation;
    bool hasParameter;
    const char *description;
};

// Parameters of the "sxe" command
const EventsDescription eventDescriptions[] =
{
    {"eh", false, QT_TRANSLATE_NOOP("Debugger::Internal::CdbBreakEventWidget",
                                    "C++ exception")},
    {"ct", false, QT_TRANSLATE_NOOP("Debugger::Internal::CdbBreakEventWidget",
                                    "Thread creation")},
    {"et", false, QT_TRANSLATE_NOOP("Debugger::Internal::CdbBreakEventWidget",
                                    "Thread exit")},
    {"ld", true,  QT_TRANSLATE_NOOP("Debugger::Internal::CdbBreakEventWidget",
                                    "Load module:")},
    {"ud", true,  QT_TRANSLATE_NOOP("Debugger::Internal::CdbBreakEventWidget",
                                    "Unload module:")},
    {"out", true, QT_TRANSLATE_NOOP("Debugger::Internal::CdbBreakEventWidget",
                                    "Output:")}
};

static inline int indexOfEvent(const QString &abbrev)
{
    const size_t eventCount = sizeof(eventDescriptions) / sizeof(EventsDescription);
    for (size_t e = 0; e < eventCount; e++)
        if (abbrev == QLatin1String(eventDescriptions[e].abbreviation))
                return int(e);
    return -1;
}

CdbBreakEventWidget::CdbBreakEventWidget(QWidget *parent) : QWidget(parent)
{
    // 1 column with checkboxes only,
    // further columns with checkbox + parameter
    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->setMargin(0);
    QVBoxLayout *leftLayout = new QVBoxLayout;
    QFormLayout *parameterLayout = 0;
    mainLayout->addLayout(leftLayout);
    const size_t eventCount = sizeof(eventDescriptions) / sizeof(EventsDescription);
    for (size_t e = 0; e < eventCount; e++) {
        QCheckBox *cb = new QCheckBox(tr(eventDescriptions[e].description));
        QLineEdit *le = 0;
        if (eventDescriptions[e].hasParameter) {
            if (!parameterLayout) {
                parameterLayout = new QFormLayout;
                mainLayout->addSpacerItem(new QSpacerItem(20, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Ignored));
                mainLayout->addLayout(parameterLayout);
            }
            le = new QLineEdit;
            parameterLayout->addRow(cb, le);
            if (parameterLayout->count() >= 6) // New column
                parameterLayout = 0;
        } else {
            leftLayout->addWidget(cb);
        }
        m_checkBoxes.push_back(cb);
        m_lineEdits.push_back(le);
    }
    setLayout(mainLayout);
}

void CdbBreakEventWidget::clear()
{
    foreach (QLineEdit *l, m_lineEdits) {
        if (l)
            l->clear();
    }
    foreach (QCheckBox *c, m_checkBoxes)
        c->setChecked(false);
}

void CdbBreakEventWidget::setBreakEvents(const QStringList &l)
{
    clear();
    // Split the list of ("eh", "out:MyOutput")
    foreach (const QString &evt, l) {
        const int colonPos = evt.indexOf(QLatin1Char(':'));
        const QString abbrev = colonPos != -1 ? evt.mid(0, colonPos) : evt;
        const int index = indexOfEvent(abbrev);
        if (index != -1)
            m_checkBoxes.at(index)->setChecked(true);
        if (colonPos != -1 && m_lineEdits.at(index))
            m_lineEdits.at(index)->setText(evt.mid(colonPos + 1));
    }
}

QString CdbBreakEventWidget::filterText(int i) const
{
    return m_lineEdits.at(i) ? m_lineEdits.at(i)->text() : QString();
}

QStringList CdbBreakEventWidget::breakEvents() const
{
    // Compile a list of ("eh", "out:MyOutput")
    QStringList rc;
    const int eventCount = sizeof(eventDescriptions) / sizeof(EventsDescription);
    for (int e = 0; e < eventCount; e++) {
        if (m_checkBoxes.at(e)->isChecked()) {
            const QString filter = filterText(e);
            QString s = QLatin1String(eventDescriptions[e].abbreviation);
            if (!filter.isEmpty()) {
                s += QLatin1Char(':');
                s += filter;
            }
            rc.push_back(s);
        }
    }
    return rc;
}

CdbOptionsPageWidget::CdbOptionsPageWidget(QWidget *parent)
    : QWidget(parent)
    , m_breakEventWidget(new CdbBreakEventWidget)
{
    m_ui.setupUi(this);
    // Squeeze the groupbox layouts vertically to
    // accommodate all options. This page only shows on
    // Windows, which has large margins by default.

    const int margin = layout()->margin();
    const QMargins margins(margin, margin / 3, margin, margin / 3);

    m_ui.startupFormLayout->setContentsMargins(margins);

    QVBoxLayout *eventLayout = new QVBoxLayout;
    eventLayout->setContentsMargins(margins);
    eventLayout->addWidget(m_breakEventWidget);
    m_ui.eventGroupBox->setLayout(eventLayout);
    m_ui.breakCrtDbgReportCheckBox
        ->setText(CommonOptionsPage::msgSetBreakpointAtFunction(CdbOptionsPage::crtDbgReport));
    const QString hint = tr("This is useful to catch runtime error messages, for example caused by assert().");
    m_ui.breakCrtDbgReportCheckBox
        ->setToolTip(CommonOptionsPage::msgSetBreakpointAtFunctionToolTip(CdbOptionsPage::crtDbgReport, hint));

    group.insert(action(CdbAdditionalArguments), m_ui.additionalArgumentsLineEdit);
    group.insert(action(CdbBreakOnCrtDbgReport), m_ui.breakCrtDbgReportCheckBox);
    group.insert(action(UseCdbConsole), m_ui.consoleCheckBox);
    group.insert(action(CdbBreakPointCorrection), m_ui.breakpointCorrectionCheckBox);
    group.insert(action(CdbUsePythonDumper), m_ui.usePythonDumper);
    group.insert(action(IgnoreFirstChanceAccessViolation),
                 m_ui.ignoreFirstChanceAccessViolationCheckBox);

    m_breakEventWidget->setBreakEvents(stringListSetting(CdbBreakEvents));
}

QStringList CdbOptionsPageWidget::breakEvents() const
{
    return m_breakEventWidget->breakEvents();
}

// ---------- CdbOptionsPage

CdbOptionsPage::CdbOptionsPage()
{
    setId("F.Debugger.Cda");
    setDisplayName(tr("CDB"));
    setCategory(Debugger::Constants::DEBUGGER_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("Debugger",
        Constants::DEBUGGER_SETTINGS_TR_CATEGORY));
    setCategoryIcon(Utils::Icon(Constants::DEBUGGER_COMMON_SETTINGS_CATEGORY_ICON));
}

CdbOptionsPage::~CdbOptionsPage()
{
}

QWidget *CdbOptionsPage::widget()
{
    if (!m_widget)
        m_widget = new CdbOptionsPageWidget;
    return m_widget;
}

void CdbOptionsPage::apply()
{
    if (!m_widget)
        return;
    m_widget->group.apply(Core::ICore::settings());
    action(CdbBreakEvents)->setValue(m_widget->breakEvents());
}

void CdbOptionsPage::finish()
{
    if (m_widget) {
        m_widget->group.finish();
        delete m_widget;
    }
}

// ---------- CdbPathsPage

class CdbPathsPageWidget : public QWidget
{
    Q_OBJECT
public:
    Utils::SavedActionSet group;

//    CdbPaths m_paths;
    CdbSymbolPathListEditor *m_symbolPathListEditor;
    Utils::PathListEditor *m_sourcePathListEditor;

    CdbPathsPageWidget(QWidget *parent = 0);
};

CdbPathsPageWidget::CdbPathsPageWidget(QWidget *parent) :
    QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    QString title = tr("Symbol Paths");
    QGroupBox* gbSymbolPath = new QGroupBox(this);
    gbSymbolPath->setTitle(title);
    QVBoxLayout *gbSymbolPathLayout = new QVBoxLayout(gbSymbolPath);
    m_symbolPathListEditor = new CdbSymbolPathListEditor(gbSymbolPath);
    gbSymbolPathLayout->addWidget(m_symbolPathListEditor);

    title = tr("Source Paths");
    QGroupBox* gbSourcePath = new QGroupBox(this);
    gbSourcePath->setTitle(title);
    QVBoxLayout *gbSourcePathLayout = new QVBoxLayout(gbSourcePath);
    m_sourcePathListEditor = new Utils::PathListEditor(gbSourcePath);
    gbSourcePathLayout->addWidget(m_sourcePathListEditor);

    layout->addWidget(gbSymbolPath);
    layout->addWidget(gbSourcePath);

    group.insert(action(CdbSymbolPaths), m_symbolPathListEditor);
    group.insert(action(CdbSourcePaths), m_sourcePathListEditor);
}

CdbPathsPage::CdbPathsPage()
    : m_widget(0)
{
    setId("F.Debugger.Cdb");
    setDisplayName(tr("CDB Paths"));
    setCategory(Debugger::Constants::DEBUGGER_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("Debugger",
        Constants::DEBUGGER_SETTINGS_TR_CATEGORY));
    setCategoryIcon(Utils::Icon(Constants::DEBUGGER_COMMON_SETTINGS_CATEGORY_ICON));
}

CdbPathsPage::~CdbPathsPage()
{
}

QWidget *CdbPathsPage::widget()
{
    if (!m_widget)
        m_widget = new CdbPathsPageWidget;
    return m_widget;
}

void CdbPathsPage::apply()
{
    if (m_widget)
        m_widget->group.apply(Core::ICore::settings());
}

void CdbPathsPage::finish()
{
    if (m_widget) {
        m_widget->group.finish();
        delete m_widget;
    }
}

} // namespace Internal
} // namespace Debugger

#include "cdboptionspage.moc"
