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
#include "cdboptions.h"
#include "commonoptionspage.h"
#include "debuggerinternalconstants.h"
#include "cdbengine.h"
#include "cdbsymbolpathlisteditor.h"

#include <utils/synchronousprocess.h>

#include <coreplugin/icore.h>

#include <QTextStream>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QCheckBox>
#include <QVBoxLayout>

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

CdbPathDialog::CdbPathDialog(QWidget *parent, Mode mode)
    : QDialog(parent)
    , m_pathListEditor(0)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setMinimumWidth(700);

    switch (mode) {
    case SymbolPaths:
        setWindowTitle(tr("CDB Symbol Paths"));
        m_pathListEditor = new CdbSymbolPathListEditor(this);
        break;
    case SourcePaths:
        setWindowTitle(tr("CDB Source Paths"));
        m_pathListEditor = new Utils::PathListEditor(this);
        break;
    }

    QVBoxLayout *layout = new QVBoxLayout(this);
    QGroupBox *groupBox = new QGroupBox(this);
    (new QVBoxLayout(groupBox))->addWidget(m_pathListEditor);
    layout->addWidget(groupBox);
    QDialogButtonBox *buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                             Qt::Horizontal, this);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    layout->addWidget(buttonBox);
}

QStringList CdbPathDialog::paths() const
{
    return m_pathListEditor->pathList();
}

void CdbPathDialog::setPaths(const QStringList &paths)
{
    m_pathListEditor->setPathList(paths);
}

CdbOptionsPageWidget::CdbOptionsPageWidget(QWidget *parent) :
    QWidget(parent), m_breakEventWidget(new CdbBreakEventWidget)
{
    m_ui.setupUi(this);
    // Squeeze the groupbox layouts vertically to
    // accommodate all options. This page only shows on
    // Windows, which has large margins by default.

    const int margin = layout()->margin();
    const QMargins margins(margin, margin / 3, margin, margin / 3);

    m_ui.startupFormLayout->setContentsMargins(margins);
    m_ui.pathGroupBox->layout()->setContentsMargins(margins);
    m_ui.breakpointLayout->setContentsMargins(margins);

    QVBoxLayout *eventLayout = new QVBoxLayout;
    eventLayout->setContentsMargins(margins);
    eventLayout->addWidget(m_breakEventWidget);
    m_ui.eventGroupBox->setLayout(eventLayout);
    m_ui.breakCrtDbgReportCheckBox
        ->setText(CommonOptionsPage::msgSetBreakpointAtFunction(CdbOptions::crtDbgReport));
    const QString hint = tr("This is useful to catch runtime error messages, for example caused by assert().");
    m_ui.breakCrtDbgReportCheckBox
        ->setToolTip(CommonOptionsPage::msgSetBreakpointAtFunctionToolTip(CdbOptions::crtDbgReport, hint));

    connect(m_ui.symbolPathButton, SIGNAL(clicked()), this, SLOT(showSymbolPathDialog()));
    connect(m_ui.sourcePathButton, SIGNAL(clicked()), this, SLOT(showSourcePathDialog()));
}

void CdbOptionsPageWidget::setSymbolPaths(const QStringList &s)
{
    m_symbolPaths = s;
    const QString summary =
        tr("Symbol paths: %1").arg(m_symbolPaths.isEmpty() ?
            tr("<none>") : QString::number(m_symbolPaths.size()));
    m_ui.symbolPathLabel->setText(summary);
}

void CdbOptionsPageWidget::setSourcePaths(const QStringList &s)
{
    m_sourcePaths = s;
    const QString summary =
        tr("Source paths: %1").arg(m_sourcePaths.isEmpty() ?
            tr("<none>") : QString::number(m_sourcePaths.size()));
    m_ui.sourcePathLabel->setText(summary);
}

void CdbOptionsPageWidget::setOptions(CdbOptions &o)
{
    m_ui.additionalArgumentsLineEdit->setText(o.additionalArguments);
    setSymbolPaths(o.symbolPaths);
    setSourcePaths(o.sourcePaths);
    m_ui.ignoreFirstChanceAccessViolationCheckBox->setChecked(o.ignoreFirstChanceAccessViolation);
    m_breakEventWidget->setBreakEvents(o.breakEvents);
    m_ui.consoleCheckBox->setChecked(o.cdbConsole);
    m_ui.breakpointCorrectionCheckBox->setChecked(o.breakpointCorrection);
    m_ui.breakCrtDbgReportCheckBox->setChecked(o.breakFunctions.contains(QLatin1String(CdbOptions::crtDbgReport)));
}

CdbOptions CdbOptionsPageWidget::options() const
{
    CdbOptions  rc;
    rc.additionalArguments = m_ui.additionalArgumentsLineEdit->text().trimmed();
    rc.symbolPaths  = m_symbolPaths;
    rc.sourcePaths = m_sourcePaths;
    rc.ignoreFirstChanceAccessViolation = m_ui.ignoreFirstChanceAccessViolationCheckBox->isChecked();
    rc.breakEvents = m_breakEventWidget->breakEvents();
    rc.cdbConsole = m_ui.consoleCheckBox->isChecked();
    rc.breakpointCorrection = m_ui.breakpointCorrectionCheckBox->isChecked();
    if (m_ui.breakCrtDbgReportCheckBox->isChecked())
        rc.breakFunctions.push_back(QLatin1String(CdbOptions::crtDbgReport));
    return rc;
}

void CdbOptionsPageWidget::showSymbolPathDialog()
{
    CdbPathDialog pathDialog(this, CdbPathDialog::SymbolPaths);
    pathDialog.setPaths(m_symbolPaths);
    if (pathDialog.exec() == QDialog::Accepted)
        setSymbolPaths(pathDialog.paths());
}

void CdbOptionsPageWidget::showSourcePathDialog()
{
    CdbPathDialog pathDialog(this, CdbPathDialog::SourcePaths);
    pathDialog.setPaths(m_sourcePaths);
    if (pathDialog.exec() == QDialog::Accepted)
        setSourcePaths(pathDialog.paths());
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
    QTextStream(&rc)
        << stripColon(m_ui.additionalArgumentsLabel->text()) << ' '
        << stripColon(m_ui.breakFunctionGroupBox->title()) << ' '
        << m_ui.breakpointsGroupBox->title() << ' '
        << stripColon(m_ui.symbolPathLabel->text()) << ' '
        << stripColon(m_ui.sourcePathLabel->text());
    rc.remove(QLatin1Char('&'));
    return rc;
}

// ---------- CdbOptionsPage

CdbOptionsPage *CdbOptionsPage::m_instance = 0;

CdbOptionsPage::CdbOptionsPage() :
        m_options(new CdbOptions)
{
    CdbOptionsPage::m_instance = this;
    m_options->fromSettings(Core::ICore::settings());

    setId("F.Cda");
    setDisplayName(tr("CDB"));
    setCategory(Debugger::Constants::DEBUGGER_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("Debugger",
        Constants::DEBUGGER_SETTINGS_TR_CATEGORY));
    setCategoryIcon(QLatin1String(Constants::DEBUGGER_COMMON_SETTINGS_CATEGORY_ICON));
}

CdbOptionsPage::~CdbOptionsPage()
{
    CdbOptionsPage::m_instance = 0;
}

QWidget *CdbOptionsPage::createPage(QWidget *parent)
{
    m_widget = new CdbOptionsPageWidget(parent);
    m_widget->setOptions(*m_options);
    if (m_searchKeywords.isEmpty())
        m_searchKeywords = m_widget->searchKeywords();
    return m_widget;
}

void CdbOptionsPage::apply()
{
    if (!m_widget)
        return;
    const CdbOptions newOptions = m_widget->options();
    if (*m_options != newOptions) {
        *m_options = newOptions;
        m_options->toSettings(Core::ICore::settings());
    }
}

void CdbOptionsPage::finish()
{
}

bool CdbOptionsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

CdbOptionsPage *CdbOptionsPage::instance()
{
    return m_instance;
}

} // namespace Internal
} // namespace Debugger
