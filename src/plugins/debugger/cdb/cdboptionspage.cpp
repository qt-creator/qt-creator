/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "cdboptionspage.h"
#include "cdboptions.h"
#include "debuggerconstants.h"

#include <coreplugin/icore.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QUrl>
#include <QtCore/QTextStream>
#include <QtGui/QMessageBox>
#include <QtGui/QDesktopServices>

static const char *dgbToolsDownloadLink32C = "http://www.microsoft.com/whdc/devtools/debugging/installx86.Mspx";
static const char *dgbToolsDownloadLink64C = "http://www.microsoft.com/whdc/devtools/debugging/install64bit.Mspx";

namespace Debugger {
namespace Internal {

static inline QString msgPathConfigNote()
{
#ifdef Q_OS_WIN64
    const bool is64bit = true;
#else
    const bool is64bit = false;
#endif
    const QString link = is64bit ? QLatin1String(dgbToolsDownloadLink64C) : QLatin1String(dgbToolsDownloadLink32C);
    //: Label text for path configuration. %2 is "x-bit version".
    return CdbOptionsPageWidget::tr(
    "<html><body><p>Specify the path to the "
    "<a href=\"%1\">Debugging Tools for Windows</a>"
    " (%2) here.</p>"
    "<p><b>Note:</b> Restarting Qt Creator is required for these settings to take effect.</p></p>"
    "</body></html>").arg(link, (is64bit ? CdbOptionsPageWidget::tr("64-bit version")
                                         : CdbOptionsPageWidget::tr("32-bit version")));
}

CdbOptionsPageWidget::CdbOptionsPageWidget(QWidget *parent) :
    QWidget(parent)
{
    m_ui.setupUi(this);
    m_ui.noteLabel->setText(msgPathConfigNote());
    m_ui.noteLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    connect(m_ui.noteLabel, SIGNAL(linkActivated(QString)), this, SLOT(downLoadLinkActivated(QString)));

    m_ui.pathChooser->setExpectedKind(Utils::PathChooser::Directory);
    m_ui.pathChooser->addButton(tr("Autodetect"), this, SLOT(autoDetect()));
    m_ui.failureLabel->setVisible(false);
}


void CdbOptionsPageWidget::setOptions(CdbOptions &o)
{
    m_ui.pathChooser->setPath(o.path);
    m_ui.cdbPathGroupBox->setChecked(o.enabled);
    m_ui.symbolPathListEditor->setPathList(o.symbolPaths);
    m_ui.sourcePathListEditor->setPathList(o.sourcePaths);
    m_ui.verboseSymbolLoadingCheckBox->setChecked(o.verboseSymbolLoading);

}

CdbOptions CdbOptionsPageWidget::options() const
{
    CdbOptions  rc;
    rc.path = m_ui.pathChooser->path();
    rc.enabled = m_ui.cdbPathGroupBox->isChecked();
    rc.symbolPaths = m_ui.symbolPathListEditor->pathList();
    rc.sourcePaths = m_ui.sourcePathListEditor->pathList();
    rc.verboseSymbolLoading = m_ui.verboseSymbolLoadingCheckBox->isChecked();
    return rc;
}

void CdbOptionsPageWidget::autoDetect()
{
    QString path;
    QStringList checkedDirectories;
    const bool ok = CdbOptions::autoDetectPath(&path, &checkedDirectories);
    m_ui.cdbPathGroupBox->setChecked(ok);
    if (ok) {
        m_ui.pathChooser->setPath(path);
    } else {
        const QString msg = tr("\"Debugging Tools for Windows\" could not be found.");
        const QString details = tr("Checked:\n%1").arg(checkedDirectories.join(QString(QLatin1Char('\n'))));
        QMessageBox msbBox(QMessageBox::Information, tr("Autodetection"), msg, QMessageBox::Ok, this);
        msbBox.setDetailedText(details);
        msbBox.exec();
    }
}

void CdbOptionsPageWidget::setFailureMessage(const QString &msg)
{
    m_ui.failureLabel->setText(msg);
    m_ui.failureLabel->setVisible(!msg.isEmpty());
}

void CdbOptionsPageWidget::downLoadLinkActivated(const QString &link)
{
    QDesktopServices::openUrl(QUrl(link));
}

QString CdbOptionsPageWidget::searchKeywords() const
{
    QString rc;
    QTextStream(&rc) << m_ui.pathLabel->text() << ' ' << m_ui.symbolPathLabel->text()
            << ' ' << m_ui.sourcePathLabel->text()
            << ' ' << m_ui.verboseSymbolLoadingCheckBox->text();
    rc.remove(QLatin1Char('&'));
    return rc;
}

// ---------- CdbOptionsPage
CdbOptionsPage::CdbOptionsPage(const QSharedPointer<CdbOptions> &options) :
        m_options(options)
{
}

CdbOptionsPage::~CdbOptionsPage()
{
}

QString CdbOptionsPage::settingsId()
{
    return QLatin1String("F.Cdb");
}

QString CdbOptionsPage::trName() const
{
    return tr("Cdb");
}

QString CdbOptionsPage::category() const
{
    return QLatin1String(Debugger::Constants::DEBUGGER_SETTINGS_CATEGORY);
}

QString CdbOptionsPage::trCategory() const
{
    return QCoreApplication::translate("Debugger", Debugger::Constants::DEBUGGER_SETTINGS_TR_CATEGORY);
}

QWidget *CdbOptionsPage::createPage(QWidget *parent)
{
    m_widget = new CdbOptionsPageWidget(parent);
    m_widget->setOptions(*m_options);
    m_widget->setFailureMessage(m_failureMessage);
    if (m_searchKeywords.isEmpty())
        m_searchKeywords = m_widget->searchKeywords();
    return m_widget;
}

void CdbOptionsPage::apply()
{
    if (!m_widget)
        return;
    const CdbOptions newOptions = m_widget->options();
    if (unsigned changedMask = m_options->compare(newOptions)) {
        *m_options = newOptions;
        m_options->toSettings(Core::ICore::instance()->settings());
        // Paths changed?
        if (changedMask & CdbOptions::DebuggerPathsChanged) {
            emit debuggerPathsChanged();
            changedMask &= ~CdbOptions::DebuggerPathsChanged;
        }
        // Remaining options?
        if (changedMask)
            emit optionsChanged();
    }
}

void CdbOptionsPage::finish()
{
}

bool CdbOptionsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

} // namespace Internal
} // namespace Debugger
