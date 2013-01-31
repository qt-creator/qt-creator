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

#include "cdbsymbolpathlisteditor.h"

#include <coreplugin/icore.h>

#include <utils/pathchooser.h>
#include <utils/checkablemessagebox.h>

#include <QDir>
#include <QDebug>
#include <QFileDialog>
#include <QAction>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QMessageBox>

namespace Debugger {
namespace Internal {

CacheDirectoryDialog::CacheDirectoryDialog(QWidget *parent) :
    QDialog(parent), m_chooser(new Utils::PathChooser),
    m_buttonBox(new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel))
{
    setWindowTitle(tr("Select Local Cache Folder"));
    setModal(true);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QFormLayout *formLayout = new QFormLayout;
    m_chooser->setExpectedKind(Utils::PathChooser::ExistingDirectory);
    m_chooser->setMinimumWidth(400);
    formLayout->addRow(tr("Path:"), m_chooser);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(m_buttonBox);

    setLayout(mainLayout);

    connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void CacheDirectoryDialog::setPath(const QString &p)
{
    m_chooser->setPath(p);
}

QString CacheDirectoryDialog::path() const
{
    return m_chooser->path();
}

void CacheDirectoryDialog::accept()
{
    // Ensure path exists
    QString cache = path();
    if (cache.isEmpty())
        return;
    QFileInfo fi(cache);
    // Folder exists - all happy.
    if (fi.isDir()) {
        QDialog::accept();
        return;
    }
    // Does a file of the same name exist?
    if (fi.exists()) {
        QMessageBox::warning(this, tr("Already Exists"),
                             tr("A file named '%1' already exists.").arg(cache));
        return;
    }
    // Create
    QDir root(QDir::root());
    if (!root.mkpath(cache)) {
        QMessageBox::warning(this, tr("Cannot Create"),
                             tr("The folder '%1' could not be created.").arg(cache));
        return;
    }
    QDialog::accept();
}

// ---------------- CdbSymbolPathListEditor

const char *CdbSymbolPathListEditor::symbolServerPrefixC = "symsrv*symsrv.dll*";
const char *CdbSymbolPathListEditor::symbolServerPostfixC = "*http://msdl.microsoft.com/download/symbols";

CdbSymbolPathListEditor::CdbSymbolPathListEditor(QWidget *parent) :
    Utils::PathListEditor(parent)
{
    //! Add Microsoft Symbol server connection
    QAction *action = insertAction(lastAddActionIndex() + 1, tr("Symbol Server..."), this, SLOT(addSymbolServer()));
    action->setToolTip(tr("Adds the Microsoft symbol server providing symbols for operating system libraries."
                      "Requires specifying a local cache directory."));
}

QString CdbSymbolPathListEditor::promptCacheDirectory(QWidget *parent)
{
    CacheDirectoryDialog dialog(parent);
    dialog.setPath(QDir::tempPath() + QDir::separator() + QLatin1String("symbolcache"));
    if (dialog.exec() != QDialog::Accepted)
        return QString();
    return dialog.path();
}

void CdbSymbolPathListEditor::addSymbolServer()
{
    const QString cacheDir = promptCacheDirectory(this);
    if (!cacheDir.isEmpty())
        insertPathAtCursor(CdbSymbolPathListEditor::symbolServerPath(cacheDir));
}

QString CdbSymbolPathListEditor::symbolServerPath(const QString &cacheDir)
{
    QString s = QLatin1String(symbolServerPrefixC);
    s +=  QDir::toNativeSeparators(cacheDir);
    s += QLatin1String(symbolServerPostfixC);
    return s;
}

bool CdbSymbolPathListEditor::isSymbolServerPath(const QString &path, QString *cacheDir /*  = 0 */)
{
    // Split apart symbol server post/prefixes
    if (!path.startsWith(QLatin1String(symbolServerPrefixC)) || !path.endsWith(QLatin1String(symbolServerPostfixC)))
        return false;
    if (cacheDir) {
        const unsigned prefixLength = qstrlen(symbolServerPrefixC);
        *cacheDir = path.mid(prefixLength, path.size() - prefixLength - qstrlen(symbolServerPostfixC));
    }
    return true;
}

int CdbSymbolPathListEditor::indexOfSymbolServerPath(const QStringList &paths, QString *cacheDir /*  = 0 */)
{
    const int count = paths.size();
    for (int i = 0; i < count; i++)
        if (CdbSymbolPathListEditor::isSymbolServerPath(paths.at(i), cacheDir))
            return i;
    return -1;
}

bool CdbSymbolPathListEditor::promptToAddSymbolServer(const QString &settingsGroup, QStringList *symbolPaths)
{
    // Check symbol server unless the user has an external/internal setup
    if (!qgetenv("_NT_SYMBOL_PATH").isEmpty()
            || CdbSymbolPathListEditor::indexOfSymbolServerPath(*symbolPaths) != -1)
        return false;
    // Prompt to use Symbol server unless the user checked "No nagging".
    const QString nagSymbolServerKey = settingsGroup + QLatin1String("/NoPromptSymbolServer");
    bool noFurtherNagging = Core::ICore::settings()->value(nagSymbolServerKey, false).toBool();
    if (noFurtherNagging)
        return false;

    const QString symServUrl = QLatin1String("http://support.microsoft.com/kb/311503");
    const QString msg = tr("<html><head/><body><p>The debugger is not configured to use the public "
                           "<a href=\"%1\">Microsoft Symbol Server</a>. This is recommended "
                           "for retrieval of the symbols of the operating system libraries.</p>"
                           "<p><i>Note:</i> A fast internet connection is required for this to work smoothly. Also, a delay "
                           "might occur when connecting for the first time.</p>"
                           "<p>Would you like to set it up?</p>"
                           "</body></html>").arg(symServUrl);
    const QDialogButtonBox::StandardButton answer =
            Utils::CheckableMessageBox::question(Core::ICore::mainWindow(), tr("Symbol Server"), msg,
                                                 tr("Do not ask again"), &noFurtherNagging);
    Core::ICore::settings()->setValue(nagSymbolServerKey, noFurtherNagging);
    if (answer == QDialogButtonBox::No)
        return false;
    // Prompt for path and add it. Synchronize QSetting and debugger.
    const QString cacheDir = CdbSymbolPathListEditor::promptCacheDirectory(Core::ICore::mainWindow());
    if (cacheDir.isEmpty())
        return false;

    symbolPaths->push_back(CdbSymbolPathListEditor::symbolServerPath(cacheDir));
    return true;
}

} // namespace Internal
} // namespace Debugger
