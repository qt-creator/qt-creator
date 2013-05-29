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
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cdboptionspage.h"
#include "commonoptionspage.h"
#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerinternalconstants.h"
#include "cdbengine.h"
#include "cdbsymbolpathlisteditor.h"

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

    DebuggerCore *dc = debuggerCore();
    group.insert(dc->action(CdbAdditionalArguments), m_ui.additionalArgumentsLineEdit);
    group.insert(dc->action(CdbBreakOnCrtDbgReport), m_ui.breakCrtDbgReportCheckBox);
    group.insert(dc->action(UseCdbConsole), m_ui.consoleCheckBox);
    group.insert(dc->action(CdbBreakPointCorrection), m_ui.breakpointCorrectionCheckBox);
    group.insert(dc->action(IgnoreFirstChanceAccessViolation),
                 m_ui.ignoreFirstChanceAccessViolationCheckBox);

    m_breakEventWidget->setBreakEvents(dc->stringListSetting(CdbBreakEvents));
}

QStringList CdbOptionsPageWidget::breakEvents() const
{
    return m_breakEventWidget->breakEvents();
}

static QString stripColon(QString s)
{
    const int lastColon = s.lastIndexOf(QLatin1Char(':'));
    if (lastColon != -1)
        s.truncate(lastColon);
    return s;
}

QString CdbOptionsPageWidget::searchKeywords() const
{
    QString rc;
    QTextStream(&rc) << stripColon(m_ui.additionalArgumentsLabel->text());
    rc.remove(QLatin1Char('&'));
    return rc;
}

// ---------- CdbOptionsPage

CdbOptionsPage::CdbOptionsPage()
{
    setId("F.Cda");
    setDisplayName(tr("CDB"));
    setCategory(Debugger::Constants::DEBUGGER_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("Debugger",
        Constants::DEBUGGER_SETTINGS_TR_CATEGORY));
    setCategoryIcon(QLatin1String(Constants::DEBUGGER_COMMON_SETTINGS_CATEGORY_ICON));
}

CdbOptionsPage::~CdbOptionsPage()
{
}

QWidget *CdbOptionsPage::createPage(QWidget *parent)
{
    m_widget = new CdbOptionsPageWidget(parent);
    if (m_searchKeywords.isEmpty())
        m_searchKeywords = m_widget->searchKeywords();
    return m_widget;
}

void CdbOptionsPage::apply()
{
    if (!m_widget)
        return;
    m_widget->group.apply(Core::ICore::settings());
    debuggerCore()->action(CdbBreakEvents)->setValue(m_widget->breakEvents());
}

void CdbOptionsPage::finish()
{
    if (m_widget)
        m_widget->group.finish();
}

bool CdbOptionsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

// ---------- CdbPathsPage

class CdbPathsPageWidget : public QWidget
{
    Q_OBJECT
public:
    Utils::SavedActionSet group;

//    CdbPaths m_paths;
    QString m_searchKeywords;

    CdbSymbolPathListEditor *m_symbolPathListEditor;
    Utils::PathListEditor *m_sourcePathListEditor;

    CdbPathsPageWidget(QWidget *parent = 0);
};

CdbPathsPageWidget::CdbPathsPageWidget(QWidget *parent) :
    QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    QString title = tr("Symbol Paths");
    m_searchKeywords.append(title);
    QGroupBox* gbSymbolPath = new QGroupBox(this);
    gbSymbolPath->setTitle(title);
    QVBoxLayout *gbSymbolPathLayout = new QVBoxLayout(gbSymbolPath);
    m_symbolPathListEditor = new CdbSymbolPathListEditor(gbSymbolPath);
    gbSymbolPathLayout->addWidget(m_symbolPathListEditor);

    title = tr("Source Paths");
    m_searchKeywords.append(title);
    QGroupBox* gbSourcePath = new QGroupBox(this);
    gbSourcePath->setTitle(title);
    QVBoxLayout *gbSourcePathLayout = new QVBoxLayout(gbSourcePath);
    m_sourcePathListEditor = new Utils::PathListEditor(gbSourcePath);
    gbSourcePathLayout->addWidget(m_sourcePathListEditor);

    layout->addWidget(gbSymbolPath);
    layout->addWidget(gbSourcePath);

    DebuggerCore *dc = debuggerCore();
    group.insert(dc->action(CdbSymbolPaths), m_symbolPathListEditor);
    group.insert(dc->action(CdbSourcePaths), m_sourcePathListEditor);
}

CdbPathsPage::CdbPathsPage()
    : m_widget(0)
{
    setId("F.Cdb");
    setDisplayName(tr("CDB Paths"));
    setCategory(Debugger::Constants::DEBUGGER_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("Debugger",
        Constants::DEBUGGER_SETTINGS_TR_CATEGORY));
    setCategoryIcon(QLatin1String(Constants::DEBUGGER_COMMON_SETTINGS_CATEGORY_ICON));
}

CdbPathsPage::~CdbPathsPage()
{
}

QWidget *CdbPathsPage::createPage(QWidget *parent)
{
    if (!m_widget)
        m_widget = new CdbPathsPageWidget(parent);
    else
        m_widget->setParent(parent);
    return m_widget;
}

void CdbPathsPage::apply()
{
    if (m_widget)
        m_widget->group.apply(Core::ICore::settings());
}

void CdbPathsPage::finish()
{
    if (m_widget)
        m_widget->group.finish();
}

bool CdbPathsPage::matches(const QString &searchKeyWord) const
{
    return m_widget &&
            m_widget->m_searchKeywords.contains(searchKeyWord, Qt::CaseInsensitive);
}

} // namespace Internal
} // namespace Debugger

#include "cdboptionspage.moc"
