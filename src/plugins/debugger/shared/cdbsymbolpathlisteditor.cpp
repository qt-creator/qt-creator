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

#include <symbolpathsdialog.h>

#include <QCheckBox>
#include <QDir>
#include <QDebug>
#include <QAction>
#include <QFormLayout>
#include <QLabel>

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
    QString cache = path();
    // if cache is empty a default is used by the cdb
    if (cache.isEmpty()) {
        QDialog::accept();
        return;
    }
    // Ensure path exists
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

const char *CdbSymbolPathListEditor::symbolServerPrefixC = "srv*";
const char *CdbSymbolPathListEditor::symbolServerPostfixC = "http://msdl.microsoft.com/download/symbols";
const char *CdbSymbolPathListEditor::symbolCachePrefixC = "cache*";

CdbSymbolPathListEditor::CdbSymbolPathListEditor(QWidget *parent) :
    Utils::PathListEditor(parent)
{
    //! Add Microsoft Symbol server connection
    QAction *action = insertAction(lastAddActionIndex() + 1, tr("Symbol Server..."), this, SLOT(addSymbolServer()));
    action->setToolTip(tr("Adds the Microsoft symbol server providing symbols for operating system libraries."
                      "Requires specifying a local cache directory."));
    action = insertAction(lastAddActionIndex() + 1, tr("Symbol Cache..."), this, SLOT(addSymbolCache()));
    action->setToolTip(tr("Uses a directory to cache symbols used by the debugger."));
}

bool CdbSymbolPathListEditor::promptCacheDirectory(QWidget *parent, QString *cacheDirectory)
{
    CacheDirectoryDialog dialog(parent);
    dialog.setPath(QDir::tempPath() + QDir::separator() + QLatin1String("symbolcache"));
    if (dialog.exec() != QDialog::Accepted)
        return false;
    *cacheDirectory = dialog.path();
    return true;
}

void CdbSymbolPathListEditor::addSymbolServer()
{
    addSymbolPath(SymbolServerPath);
}

void CdbSymbolPathListEditor::addSymbolCache()
{
    addSymbolPath(SymbolCachePath);
}

void CdbSymbolPathListEditor::addSymbolPath(CdbSymbolPathListEditor::SymbolPathMode mode)
{
    QString cacheDir;
    if (promptCacheDirectory(this, &cacheDir))
        insertPathAtCursor(CdbSymbolPathListEditor::symbolPath(cacheDir, mode));
}

QString CdbSymbolPathListEditor::symbolPath(const QString &cacheDir,
                                            CdbSymbolPathListEditor::SymbolPathMode mode)
{
    if (mode == SymbolCachePath)
        return QLatin1String(symbolCachePrefixC) + QDir::toNativeSeparators(cacheDir);
    QString s = QLatin1String(symbolServerPrefixC);
    if (!cacheDir.isEmpty())
        s += QDir::toNativeSeparators(cacheDir) + QLatin1String("*");
    s += QLatin1String(symbolServerPostfixC);
    return s;
}

bool CdbSymbolPathListEditor::isSymbolServerPath(const QString &path, QString *cacheDir /*  = 0 */)
{
    if (!path.startsWith(QLatin1String(symbolServerPrefixC)) || !path.endsWith(QLatin1String(symbolServerPostfixC)))
        return false;
    if (cacheDir) {
        static const unsigned prefixLength = qstrlen(symbolServerPrefixC);
        static const unsigned postfixLength = qstrlen(symbolServerPostfixC);
        if (path.length() == int(prefixLength + postfixLength))
            return true;
        // Split apart symbol server post/prefixes
        *cacheDir = path.mid(prefixLength, path.size() - prefixLength - qstrlen(symbolServerPostfixC) + 1);
    }
    return true;
}

bool CdbSymbolPathListEditor::isSymbolCachePath(const QString &path, QString *cacheDir)
{
    if (!path.startsWith(QLatin1String(symbolCachePrefixC)))
        return false;
    if (cacheDir) {
        static const unsigned prefixLength = qstrlen(symbolCachePrefixC);
        // Split apart symbol cach prefixes
        *cacheDir = path.mid(prefixLength);
    }
    return true;
}

int CdbSymbolPathListEditor::indexOfSymbolPath(const QStringList &paths,
                                               CdbSymbolPathListEditor::SymbolPathMode mode,
                                               QString *cacheDir /*  = 0 */)
{
    const int count = paths.size();
    for (int i = 0; i < count; i++) {
        if (mode == SymbolServerPath
                ? CdbSymbolPathListEditor::isSymbolServerPath(paths.at(i), cacheDir)
                : CdbSymbolPathListEditor::isSymbolCachePath(paths.at(i), cacheDir)) {
                return i;
        }
    }
    return -1;
}

bool CdbSymbolPathListEditor::promptToAddSymbolPaths(QStringList *symbolPaths)
{
    const int indexOfSymbolServer =
            CdbSymbolPathListEditor::indexOfSymbolPath(*symbolPaths, SymbolServerPath);
    const int indexOfSymbolCache =
            CdbSymbolPathListEditor::indexOfSymbolPath(*symbolPaths, SymbolCachePath);

    if (!qgetenv("_NT_SYMBOL_PATH").isEmpty()
            || (indexOfSymbolServer != -1 && indexOfSymbolCache != -1))
        return false;

    const QString nagSymbolServerKey = QLatin1String("CDB2/NoPromptSymbolCache");
    bool noFurtherNagging = Core::ICore::settings()->value(nagSymbolServerKey, false).toBool();
    if (noFurtherNagging)
        return false;

    QString path;
    if (indexOfSymbolServer != -1)
        path = symbolPaths->at(indexOfSymbolServer);
    if (path.isEmpty() && indexOfSymbolCache != -1)
        path = symbolPaths->at(indexOfSymbolCache);
    if (path.isEmpty())
        path = QDir::tempPath() + QDir::separator() + QLatin1String("symbolcache");

    bool useSymbolServer = true;
    bool useSymbolCache = true;
    bool addSymbolPaths = SymbolPathsDialog::useCommonSymbolPaths(useSymbolCache,
                                                                  useSymbolServer,
                                                                  path, noFurtherNagging);
    Core::ICore::settings()->setValue(nagSymbolServerKey, noFurtherNagging);
    if (!addSymbolPaths)
        return false;

    // remove old entries
    if (indexOfSymbolServer > indexOfSymbolCache) {
        symbolPaths->removeAt(indexOfSymbolServer);
        if (indexOfSymbolCache != -1)
            symbolPaths->removeAt(indexOfSymbolCache);
    } else if (indexOfSymbolCache > indexOfSymbolServer) {
        symbolPaths->removeAt(indexOfSymbolCache);
        if (indexOfSymbolServer != -1)
            symbolPaths->removeAt(indexOfSymbolServer);
    }

    if (useSymbolCache) {
        symbolPaths->push_back(CdbSymbolPathListEditor::symbolPath(path, SymbolCachePath));
        if (useSymbolServer)
            symbolPaths->push_back(CdbSymbolPathListEditor::symbolPath(QString(), SymbolServerPath));
        return true;
    } else if (useSymbolServer) {
        symbolPaths->push_back(CdbSymbolPathListEditor::symbolPath(path, SymbolServerPath));
        return true;
    }
    return false;
}

} // namespace Internal
} // namespace Debugger
