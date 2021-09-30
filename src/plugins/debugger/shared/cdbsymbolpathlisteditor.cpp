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

#include "cdbsymbolpathlisteditor.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>

#include <utils/pathchooser.h>
#include <utils/checkablemessagebox.h>
#include <utils/temporarydirectory.h>

#include "symbolpathsdialog.h"

#include <QCheckBox>
#include <QDebug>
#include <QAction>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>

using namespace Utils;

namespace Debugger {
namespace Internal {

// Internal helper dialog prompting for a cache directory using a PathChooser.
//
// Note that QFileDialog does not offer a way of suggesting
// a non-existent folder, which is in turn automatically
// created. This is done here (suggest $TEMP\symbolcache
// regardless of its existence).

class CacheDirectoryDialog : public QDialog
{
    Q_DECLARE_TR_FUNCTIONS(Debugger::Internal::CaheDirectoryDialog)

public:
    explicit CacheDirectoryDialog(QWidget *parent);

    void setPath(const FilePath &p) { m_chooser->setFilePath(p); }
    FilePath path() const { return m_chooser->filePath(); }

    void accept() override;

private:
    PathChooser *m_chooser;
    QDialogButtonBox *m_buttonBox;
};

CacheDirectoryDialog::CacheDirectoryDialog(QWidget *parent) :
    QDialog(parent), m_chooser(new PathChooser),
    m_buttonBox(new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel))
{
    setWindowTitle(tr("Select Local Cache Folder"));
    setModal(true);

    auto formLayout = new QFormLayout;
    m_chooser->setExpectedKind(PathChooser::ExistingDirectory);
    m_chooser->setHistoryCompleter("Debugger.CdbCacheDir.History");
    m_chooser->setMinimumWidth(400);
    formLayout->addRow(tr("Path:"), m_chooser);

    auto mainLayout = new QVBoxLayout;
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(m_buttonBox);

    setLayout(mainLayout);

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &CacheDirectoryDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &CacheDirectoryDialog::reject);
}

void CacheDirectoryDialog::accept()
{
    FilePath cache = path();
    // if cache is empty a default is used by the cdb
    if (cache.isEmpty()) {
        QDialog::accept();
        return;
    }
    // Ensure path exists
    // Folder exists - all happy.
    if (cache.isDir()) {
        QDialog::accept();
        return;
    }
    // Does a file of the same name exist?
    if (cache.exists()) {
        Core::AsynchronousMessageBox::warning(tr("Already Exists"),
                                              tr("A file named \"%1\" already exists.").arg(cache.toUserOutput()));
        return;
    }
    // Create
    if (!cache.ensureWritableDir()) {
        Core::AsynchronousMessageBox::warning(tr("Cannot Create"),
                                              tr("The folder \"%1\" could not be created.").arg(cache.toUserOutput()));
        return;
    }
    QDialog::accept();
}

// ---------------- CdbSymbolPathListEditor

// Pre- and Postfix used to build a symbol server path specification
const char symbolServerPrefixC[] = "srv*";
const char symbolServerPostfixC[] = "http://msdl.microsoft.com/download/symbols";
const char symbolCachePrefixC[] = "cache*";

CdbSymbolPathListEditor::CdbSymbolPathListEditor(QWidget *parent) :
    PathListEditor(parent)
{
    QPushButton *button = insertButton(lastInsertButtonIndex + 1,
                                       tr("Insert Symbol Server..."), this, [this](){
        addSymbolPath(SymbolServerPath);
    });
    button->setToolTip(tr("Adds the Microsoft symbol server providing symbols for operating system "
                          "libraries. Requires specifying a local cache directory."));

    button = insertButton(lastInsertButtonIndex + 1, tr("Insert Symbol Cache..."), this, [this]() {
        addSymbolPath(SymbolCachePath);
    });
    button->setToolTip(tr("Uses a directory to cache symbols used by the debugger."));

    button = insertButton(lastInsertButtonIndex + 1, tr("Set up Symbol Paths..."), this, [this](){
        setupSymbolPaths();
    });
    button->setToolTip(tr("Configure Symbol paths that are used to locate debug symbol files."));
}

bool CdbSymbolPathListEditor::promptCacheDirectory(QWidget *parent, FilePath *cacheDirectory)
{
    CacheDirectoryDialog dialog(parent);
    dialog.setPath(FilePath::fromString(TemporaryDirectory::masterDirectoryPath()) + "/symbolcache");
    if (dialog.exec() != QDialog::Accepted)
        return false;
    *cacheDirectory = dialog.path();
    return true;
}

void CdbSymbolPathListEditor::addSymbolPath(CdbSymbolPathListEditor::SymbolPathMode mode)
{
    FilePath cacheDir;
    if (promptCacheDirectory(this, &cacheDir))
        insertPathAtCursor(CdbSymbolPathListEditor::symbolPath(cacheDir, mode));
}

void CdbSymbolPathListEditor::setupSymbolPaths()
{
    const QStringList &currentPaths = pathList();
    const int indexOfSymbolServer = indexOfSymbolPath(currentPaths, SymbolServerPath);
    const int indexOfSymbolCache = indexOfSymbolPath(currentPaths, SymbolCachePath);

    FilePath path;
    if (indexOfSymbolServer != -1)
        path = FilePath::fromString(currentPaths.at(indexOfSymbolServer));
    if (path.isEmpty() && indexOfSymbolCache != -1)
        path = FilePath::fromString(currentPaths.at(indexOfSymbolCache));
    if (path.isEmpty())
        path = FilePath::fromString(TemporaryDirectory::masterDirectoryPath() + "/symbolcache");

    bool useSymbolServer = true;
    bool useSymbolCache = true;
    bool addSymbolPaths = SymbolPathsDialog::useCommonSymbolPaths(useSymbolCache,
                                                                  useSymbolServer,
                                                                  path);
    if (!addSymbolPaths)
        return;

    if (useSymbolCache) {
        insertPathAtCursor(CdbSymbolPathListEditor::symbolPath(path, SymbolCachePath));
        if (useSymbolServer)
            insertPathAtCursor(CdbSymbolPathListEditor::symbolPath({}, SymbolServerPath));
    } else if (useSymbolServer) {
        insertPathAtCursor(CdbSymbolPathListEditor::symbolPath(path, SymbolServerPath));
    }
}

QString CdbSymbolPathListEditor::symbolPath(const FilePath &cacheDir,
                                            CdbSymbolPathListEditor::SymbolPathMode mode)
{
    if (mode == SymbolCachePath)
        return symbolCachePrefixC + cacheDir.toUserOutput();
    QString s = QLatin1String(symbolServerPrefixC);
    if (!cacheDir.isEmpty())
        s += cacheDir.toUserOutput() + '*';
    s += QLatin1String(symbolServerPostfixC);
    return s;
}

bool CdbSymbolPathListEditor::isSymbolServerPath(const QString &path, QString *cacheDir /*  = nullptr */)
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
                                               QString *cacheDir /*  = nullptr */)
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

} // namespace Internal
} // namespace Debugger
