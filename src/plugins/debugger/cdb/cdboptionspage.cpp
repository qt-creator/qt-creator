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

#include <utils/aspects.h>
#include <utils/layoutbuilder.h>

#include <QCheckBox>
#include <QFormLayout>
#include <QTextStream>

using namespace Utils;

namespace Debugger {
namespace Internal {

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

// ---------- CdbOptionsPage

// Widget displaying a list of break events for the 'sxe' command
// with a checkbox to enable 'break' and optionally a QLineEdit for
// events with parameters (like 'out:Needle').
class CdbBreakEventWidget : public QWidget
{
    Q_DECLARE_TR_FUNCTIONS(Debugger::Internal::CdbBreakEventWidget)

public:
    explicit CdbBreakEventWidget(QWidget *parent = nullptr);

    void setBreakEvents(const QStringList &l);
    QStringList breakEvents() const;

private:
    QString filterText(int i) const;
    void clear();

    QList<QCheckBox*> m_checkBoxes;
    QList<QLineEdit*> m_lineEdits;
};

CdbBreakEventWidget::CdbBreakEventWidget(QWidget *parent) : QWidget(parent)
{
    // 1 column with checkboxes only,
    // further columns with checkbox + parameter
    auto mainLayout = new QHBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);
    auto leftLayout = new QVBoxLayout;
    QFormLayout *parameterLayout = nullptr;
    mainLayout->addLayout(leftLayout);
    const size_t eventCount = sizeof(eventDescriptions) / sizeof(EventsDescription);
    for (size_t e = 0; e < eventCount; e++) {
        auto cb = new QCheckBox(tr(eventDescriptions[e].description));
        QLineEdit *le = nullptr;
        if (eventDescriptions[e].hasParameter) {
            if (!parameterLayout) {
                parameterLayout = new QFormLayout;
                mainLayout->addSpacerItem(new QSpacerItem(20, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Ignored));
                mainLayout->addLayout(parameterLayout);
            }
            le = new QLineEdit;
            parameterLayout->addRow(cb, le);
            if (parameterLayout->count() >= 6) // New column
                parameterLayout = nullptr;
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
    for (QLineEdit *l : qAsConst(m_lineEdits)) {
        if (l)
            l->clear();
    }
    for (QCheckBox *c : qAsConst(m_checkBoxes))
        c->setChecked(false);
}

void CdbBreakEventWidget::setBreakEvents(const QStringList &l)
{
    clear();
    // Split the list of ("eh", "out:MyOutput")
    for (const QString &evt : l) {
        const int colonPos = evt.indexOf(':');
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
                s += ':';
                s += filter;
            }
            rc.push_back(s);
        }
    }
    return rc;
}

class CdbOptionsPageWidget : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(Debugger::Internal::CdbOptionsPageWidget)

public:
    CdbOptionsPageWidget();

private:
    void apply() final;
    void finish() final;

    Utils::AspectContainer &m_group = debuggerSettings()->page5;
    CdbBreakEventWidget *m_breakEventWidget;
};

CdbOptionsPageWidget::CdbOptionsPageWidget()
    : m_breakEventWidget(new CdbBreakEventWidget)
{
    using namespace Layouting;
    DebuggerSettings &s = *debuggerSettings();

    m_breakEventWidget->setBreakEvents(debuggerSettings()->cdbBreakEvents.value());

    Column {
        Row {
            Group {
                Title(tr("Startup")),
                s.cdbAdditionalArguments,
                s.useCdbConsole,
                Stretch()
            },

            Group {
                Title(tr("Various")),
                s.ignoreFirstChanceAccessViolation,
                s.cdbBreakOnCrtDbgReport,
                s.cdbBreakPointCorrection,
                s.cdbUsePythonDumper
            }
        },

        Group {
            Title(tr("Break On")),
            m_breakEventWidget
        },

        Group {
            Title(tr("Add Exceptions to Issues View")),
            s.firstChanceExceptionTaskEntry,
            s.secondChanceExceptionTaskEntry
        },

        Stretch()

    }.attachTo(this);
}

void CdbOptionsPageWidget::apply()
{
    m_group.apply(); m_group.writeSettings(Core::ICore::settings());
    debuggerSettings()->cdbBreakEvents.setValue(m_breakEventWidget->breakEvents());
}

void CdbOptionsPageWidget::finish()
{
    m_breakEventWidget->setBreakEvents(debuggerSettings()->cdbBreakEvents.value());
    m_group.finish();
}

CdbOptionsPage::CdbOptionsPage()
{
    setId("F.Debugger.Cda");
    setDisplayName(CdbOptionsPageWidget::tr("CDB"));
    setCategory(Debugger::Constants::DEBUGGER_SETTINGS_CATEGORY);
    setWidgetCreator([] { return new CdbOptionsPageWidget; });
}


// ---------- CdbPathsPage

class CdbPathsPageWidget : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(Debugger::Internal::CdbPathsPageWidget)

public:
    CdbPathsPageWidget();

    void apply() final;
    void finish() final;

    AspectContainer &m_group = debuggerSettings()->page6;

private:
    PathListEditor *m_symbolPaths = nullptr;
    PathListEditor *m_sourcePaths = nullptr;
};

CdbPathsPageWidget::CdbPathsPageWidget()
    : m_symbolPaths(new CdbSymbolPathListEditor)
    , m_sourcePaths(new PathListEditor)
{
    using namespace Layouting;

    finish();
    Column {
        Group { Title(tr("Symbol Paths")), m_symbolPaths },
        Group { Title(tr("Source Paths")), m_sourcePaths },
        Stretch()
    }.attachTo(this);
}

void CdbPathsPageWidget::apply()
{
    debuggerSettings()->cdbSymbolPaths.setValue(m_symbolPaths->pathList());
    debuggerSettings()->cdbSourcePaths.setValue(m_sourcePaths->pathList());
    m_group.writeSettings(Core::ICore::settings());
}

void CdbPathsPageWidget::finish()
{
    m_symbolPaths->setPathList(debuggerSettings()->cdbSymbolPaths.value());
    m_sourcePaths->setPathList(debuggerSettings()->cdbSourcePaths.value());
}

CdbPathsPage::CdbPathsPage()
{
    setId("F.Debugger.Cdb");
    setDisplayName(CdbPathsPageWidget::tr("CDB Paths"));
    setCategory(Debugger::Constants::DEBUGGER_SETTINGS_CATEGORY);
    setWidgetCreator([] { return new CdbPathsPageWidget; });
}

} // namespace Internal
} // namespace Debugger
