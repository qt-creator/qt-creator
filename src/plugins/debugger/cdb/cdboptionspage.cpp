/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "cdboptionspage.h"
#include "cdboptions.h"
#include "debuggerconstants.h"
#include "cdbengine.h"

#ifdef Q_OS_WIN
#    include <utils/winutils.h>
#endif
#include <utils/synchronousprocess.h>

#include <coreplugin/icore.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QUrl>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QDateTime>
#include <QtCore/QTextStream>
#include <QtCore/QTimer>
#include <QtCore/QProcess>
#include <QtGui/QMessageBox>
#include <QtGui/QLineEdit>
#include <QtGui/QDesktopServices>

static const char *dgbToolsDownloadLink32C = "http://www.microsoft.com/whdc/devtools/debugging/installx86.Mspx";
static const char *dgbToolsDownloadLink64C = "http://www.microsoft.com/whdc/devtools/debugging/install64bit.Mspx";

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
    {"eh", false, QT_TRANSLATE_NOOP("Debugger::Cdb::CdbBreakEventWidget",
                                    "C++ exception")},
    {"ct", false, QT_TRANSLATE_NOOP("Debugger::Cdb::CdbBreakEventWidget",
                                    "Thread creation")},
    {"et", false, QT_TRANSLATE_NOOP("Debugger::Cdb::CdbBreakEventWidget",
                                    "Thread exit")},
    {"ld", true,  QT_TRANSLATE_NOOP("Debugger::Cdb::CdbBreakEventWidget",
                                    "Load Module:")},
    {"ud", true,  QT_TRANSLATE_NOOP("Debugger::Cdb::CdbBreakEventWidget",
                                    "Unload Module:")},
    {"out", true, QT_TRANSLATE_NOOP("Debugger::Cdb::CdbBreakEventWidget",
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
            if (parameterLayout->count() >= 4) // New column
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

static inline QString msgPathConfigNote()
{
#ifdef Q_OS_WIN
    const bool is64bit = Utils::winIs64BitSystem();
#else
    const bool is64bit = false;
#endif
    const QString link = is64bit ? QLatin1String(dgbToolsDownloadLink64C) : QLatin1String(dgbToolsDownloadLink32C);
    //: Label text for path configuration. %2 is "x-bit version".
    return CdbOptionsPageWidget::tr(
    "<html><body><p>Specify the path to the "
    "<a href=\"%1\">Windows Console Debugger executable</a>"
    " (%2) here.</p>"
    "</body></html>").arg(link, (is64bit ? CdbOptionsPageWidget::tr("64-bit version")
                                         : CdbOptionsPageWidget::tr("32-bit version")));
}

CdbOptionsPageWidget::CdbOptionsPageWidget(QWidget *parent) :
    QWidget(parent), m_breakEventWidget(new CdbBreakEventWidget),
    m_reportTimer(0)
{
    m_ui.setupUi(this);
    m_ui.noteLabel->setText(msgPathConfigNote());
    m_ui.noteLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    connect(m_ui.noteLabel, SIGNAL(linkActivated(QString)), this, SLOT(downLoadLinkActivated(QString)));

    m_ui.pathChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_ui.pathChooser->addButton(tr("Autodetect"), this, SLOT(autoDetect()));
    m_ui.cdbPathGroupBox->installEventFilter(this);
    QVBoxLayout *eventLayout = new QVBoxLayout;
    eventLayout->addWidget(m_breakEventWidget);
    m_ui.eventGroupBox->setLayout(eventLayout);
}

void CdbOptionsPageWidget::setOptions(CdbOptions &o)
{
    m_ui.pathChooser->setPath(o.executable);
    m_ui.additionalArgumentsLineEdit->setText(o.additionalArguments);
    m_ui.is64BitCheckBox->setChecked(o.is64bit);
    m_ui.cdbPathGroupBox->setChecked(o.enabled);
    setSymbolPaths(o.symbolPaths);
    m_ui.sourcePathListEditor->setPathList(o.sourcePaths);
    m_breakEventWidget->setBreakEvents(o.breakEvents);
}

bool CdbOptionsPageWidget::is64Bit() const
{
    return m_ui.is64BitCheckBox->isChecked();
}

QString CdbOptionsPageWidget::path() const
{
    return m_ui.pathChooser->path();
}

CdbOptions CdbOptionsPageWidget::options() const
{
    CdbOptions  rc;
    rc.executable = path();
    rc.additionalArguments = m_ui.additionalArgumentsLineEdit->text().trimmed();
    rc.enabled = m_ui.cdbPathGroupBox->isChecked();
    rc.is64bit = is64Bit();
    rc.symbolPaths = symbolPaths();
    rc.sourcePaths = m_ui.sourcePathListEditor->pathList();
    rc.breakEvents = m_breakEventWidget->breakEvents();
    return rc;
}

QStringList CdbOptionsPageWidget::symbolPaths() const
{
    return m_ui.symbolPathListEditor->pathList();
}

void CdbOptionsPageWidget::setSymbolPaths(const QStringList &s)
{
    m_ui.symbolPathListEditor->setPathList(s);
}

void CdbOptionsPageWidget::hideReportLabel()
{
    m_ui.reportLabel->clear();
    m_ui.reportLabel->setVisible(false);
}

void CdbOptionsPageWidget::autoDetect()
{
    QString executable;
    QStringList checkedDirectories;
    bool is64bit;
    const bool ok = CdbOptions::autoDetectExecutable(&executable, &is64bit, &checkedDirectories);
    m_ui.cdbPathGroupBox->setChecked(ok);
    if (ok) {
        m_ui.is64BitCheckBox->setChecked(is64bit);
        m_ui.pathChooser->setPath(executable);
        QString report;
        // Now check for the extension library as well.
        const bool allOk = checkInstallation(executable, is64Bit(), &report);
        setReport(report, allOk);
        // On this occasion, if no symbol paths are specified, check for an
        // old CDB installation
        if (symbolPaths().isEmpty())
            setSymbolPaths(CdbOptions::oldEngineSymbolPaths(Core::ICore::instance()->settings()));
    } else {
        const QString msg = tr("\"Debugging Tools for Windows\" could not be found.");
        const QString details = tr("Checked:\n%1").arg(checkedDirectories.join(QString(QLatin1Char('\n'))));
        QMessageBox msbBox(QMessageBox::Information, tr("Autodetection"), msg, QMessageBox::Ok, this);
        msbBox.setDetailedText(details);
        msbBox.exec();
    }
}

void CdbOptionsPageWidget::setReport(const QString &msg, bool success)
{
    // Hide label after some interval
    if (!m_reportTimer) {
        m_reportTimer = new QTimer(this);
        m_reportTimer->setSingleShot(true);
        connect(m_reportTimer, SIGNAL(timeout()), this, SLOT(hideReportLabel()));
    } else {
        if (m_reportTimer->isActive())
            m_reportTimer->stop();
    }
    m_reportTimer->setInterval(success ? 10000 : 20000);
    m_reportTimer->start();

    m_ui.reportLabel->setText(msg);
    m_ui.reportLabel->setStyleSheet(success ? QString() : QString::fromAscii("background-color : 'red'"));
    m_ui.reportLabel->setVisible(true);
}

void CdbOptionsPageWidget::downLoadLinkActivated(const QString &link)
{
    QDesktopServices::openUrl(QUrl(link));
}

QString CdbOptionsPageWidget::searchKeywords() const
{
    QString rc;
    QTextStream(&rc) << m_ui.pathLabel->text() << ' ' << m_ui.symbolPathLabel->text()
            << ' ' << m_ui.sourcePathLabel->text();
    rc.remove(QLatin1Char('&'));
    return rc;
}

static QString cdbVersion(const QString &executable)
{
    QProcess cdb;
    cdb.start(executable, QStringList(QLatin1String("-version")));
    cdb.closeWriteChannel();
    if (!cdb.waitForStarted())
        return QString();
    if (!cdb.waitForFinished()) {
        Utils::SynchronousProcess::stopProcess(cdb);
        return QString();
    }
    return QString::fromLocal8Bit(cdb.readAllStandardOutput());
}

bool CdbOptionsPageWidget::checkInstallation(const QString &executable,
                                             bool is64Bit, QString *message)
{
    // 1) Check on executable
    unsigned checkedItems = 0;
    QString rc;
    if (executable.isEmpty()) {
        message->append(tr("No cdb executable specified.\n"));
    } else {
        const QString version = cdbVersion(executable);
        if (version.isEmpty()) {
            message->append(tr("Unable to determine version of %1.\n").
                            arg(executable));
        } else {
            message->append(tr("Version: %1").arg(version));
            checkedItems++;
        }
    }

    // 2) Check on extension library
    const QFileInfo extensionFi(CdbEngine::extensionLibraryName(is64Bit));
    if (extensionFi.isFile()) {
        message->append(tr("Extension library: %1, built: %3.\n").
                        arg(QDir::toNativeSeparators(extensionFi.absoluteFilePath())).
                        arg(extensionFi.lastModified().toString(Qt::SystemLocaleShortDate)));
        checkedItems++;
    } else {
        message->append("Extension library not found.\n");
    }
    return checkedItems == 2u;
}

bool CdbOptionsPageWidget::eventFilter(QObject *o, QEvent *e)
{
    if (o != m_ui.cdbPathGroupBox || e->type() != QEvent::ToolTip)
        return QWidget::eventFilter(o, e);
    QString message;
    checkInstallation(path(), is64Bit(), &message);
    m_ui.cdbPathGroupBox->setToolTip(message);
    return false;
}

// ---------- CdbOptionsPage

CdbOptionsPage *CdbOptionsPage::m_instance = 0;

CdbOptionsPage::CdbOptionsPage() :
        m_options(new CdbOptions)
{
    CdbOptionsPage::m_instance = this;
    m_options->fromSettings(Core::ICore::instance()->settings());
}

CdbOptionsPage::~CdbOptionsPage()
{
    CdbOptionsPage::m_instance = 0;
}

QString CdbOptionsPage::settingsId()
{
    return QLatin1String("F.Cda"); // before old CDB
}

QString CdbOptionsPage::displayName() const
{
    return tr("CDB");
}

QString CdbOptionsPage::category() const
{
    return QLatin1String(Debugger::Constants::DEBUGGER_SETTINGS_CATEGORY);
}

QString CdbOptionsPage::displayCategory() const
{
    return QCoreApplication::translate("Debugger", Debugger::Constants::DEBUGGER_SETTINGS_TR_CATEGORY);
}

QIcon CdbOptionsPage::categoryIcon() const
{
    return QIcon(QLatin1String(Debugger::Constants::DEBUGGER_COMMON_SETTINGS_CATEGORY_ICON));
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
        m_options->toSettings(Core::ICore::instance()->settings());
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
