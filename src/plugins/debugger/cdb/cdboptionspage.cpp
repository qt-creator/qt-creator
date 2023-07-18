// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cdboptionspage.h"

#include <debugger/commonoptionspage.h>
#include <debugger/debuggeractions.h>
#include <debugger/debuggercore.h>
#include <debugger/debuggerinternalconstants.h>
#include <debugger/debuggertr.h>
#include <debugger/shared/cdbsymbolpathlisteditor.h>

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
    {"eh", false, QT_TRANSLATE_NOOP("QtC::Debugger", "C++ exception")},
    {"ct", false, QT_TRANSLATE_NOOP("QtC::Debugger", "Thread creation")},
    {"et", false, QT_TRANSLATE_NOOP("QtC::Debugger", "Thread exit")},
    {"ld", true,  QT_TRANSLATE_NOOP("QtC::Debugger", "Load module:")},
    {"ud", true,  QT_TRANSLATE_NOOP("QtC::Debugger", "Unload module:")},
    {"out", true, QT_TRANSLATE_NOOP("QtC::Debugger", "Output:")}
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
        auto cb = new QCheckBox(Tr::tr(eventDescriptions[e].description));
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
    for (QLineEdit *l : std::as_const(m_lineEdits)) {
        if (l)
            l->clear();
    }
    for (QCheckBox *c : std::as_const(m_checkBoxes))
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
public:
    CdbOptionsPageWidget();

private:
    void apply() final;
    void finish() final;

    Utils::AspectContainer &m_group = settings().page5;
    CdbBreakEventWidget *m_breakEventWidget;
};

CdbOptionsPageWidget::CdbOptionsPageWidget()
    : m_breakEventWidget(new CdbBreakEventWidget)
{
    using namespace Layouting;
    DebuggerSettings &s = settings();

    m_breakEventWidget->setBreakEvents(settings().cdbBreakEvents());

    Column {
        Row {
            Group {
                title(Tr::tr("Startup")),
                Column {
                    s.cdbAdditionalArguments,
                    s.useCdbConsole,
                    st
                 }
            },

            Group {
                title(Tr::tr("Various")),
                Column {
                    s.ignoreFirstChanceAccessViolation,
                    s.cdbBreakOnCrtDbgReport,
                    s.cdbBreakPointCorrection,
                    s.cdbUsePythonDumper
                }
            }
        },

        Group {
            title(Tr::tr("Break On")),
            Column { m_breakEventWidget }
        },

        Group {
            title(Tr::tr("Add Exceptions to Issues View")),
            Column {
                s.firstChanceExceptionTaskEntry,
                s.secondChanceExceptionTaskEntry
            }
        },

        st

    }.attachTo(this);
}

void CdbOptionsPageWidget::apply()
{
    m_group.apply();
    m_group.writeSettings();
    settings().cdbBreakEvents.setValue(m_breakEventWidget->breakEvents());
}

void CdbOptionsPageWidget::finish()
{
    m_breakEventWidget->setBreakEvents(settings().cdbBreakEvents());
    m_group.finish();
}

CdbOptionsPage::CdbOptionsPage()
{
    setId("F.Debugger.Cda");
    setDisplayName(Tr::tr("CDB"));
    setCategory(Debugger::Constants::DEBUGGER_SETTINGS_CATEGORY);
    setWidgetCreator([] { return new CdbOptionsPageWidget; });
}


// ---------- CdbPathsPage

class CdbPathsPageWidget : public Core::IOptionsPageWidget
{
public:
    CdbPathsPageWidget();

    void apply() final;
    void finish() final;

    AspectContainer &m_group = settings().page6;

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
        Group { title(Tr::tr("Symbol Paths")), Column { m_symbolPaths } },
        Group { title(Tr::tr("Source Paths")), Column { m_sourcePaths } },
        st
    }.attachTo(this);
}

void CdbPathsPageWidget::apply()
{
    settings().cdbSymbolPaths.setValue(m_symbolPaths->pathList());
    settings().cdbSourcePaths.setValue(m_sourcePaths->pathList());
    m_group.writeSettings();
}

void CdbPathsPageWidget::finish()
{
    m_symbolPaths->setPathList(settings().cdbSymbolPaths());
    m_sourcePaths->setPathList(settings().cdbSourcePaths());
}

CdbPathsPage::CdbPathsPage()
{
    setId("F.Debugger.Cdb");
    setDisplayName(Tr::tr("CDB Paths"));
    setCategory(Debugger::Constants::DEBUGGER_SETTINGS_CATEGORY);
    setWidgetCreator([] { return new CdbPathsPageWidget; });
}

} // namespace Internal
} // namespace Debugger
