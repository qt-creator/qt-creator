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

#ifndef SYMBOLPATHLISTEDITOR_H
#define SYMBOLPATHLISTEDITOR_H

#include <utils/pathlisteditor.h>

#include <QDialog>

namespace Utils {
    class PathChooser;
}

QT_BEGIN_NAMESPACE
class QDialogButtonBox;
QT_END_NAMESPACE

namespace Debugger {
namespace Internal {

// Internal helper dialog prompting for a cache directory
// using a PathChooser.
// Note that QFileDialog does not offer a way of suggesting
// a non-existent folder, which is in turn automatically
// created. This is done here (suggest $TEMP\symbolcache
// regardless of its existence).

class CacheDirectoryDialog    : public QDialog {
    Q_OBJECT
public:
   explicit CacheDirectoryDialog(QWidget *parent = 0);

   void setPath(const QString &p);
   QString path() const;

   virtual void accept();

private:
   Utils::PathChooser *m_chooser;
   QDialogButtonBox *m_buttonBox;
};

class CdbSymbolPathListEditor : public Utils::PathListEditor
{
    Q_OBJECT
public:
    explicit CdbSymbolPathListEditor(QWidget *parent = 0);

    static QString promptCacheDirectory(QWidget *parent);

    // Pre- and Postfix used to build a symbol server path specification
    static const char *symbolServerPrefixC;
    static const char *symbolServerPostfixC;

    // Format a symbol server specification with a local cache directory
    static QString symbolServerPath(const QString &cacheDir);
    // Check for a symbol server path and extract local cache directory
    static bool isSymbolServerPath(const QString &path, QString *cacheDir = 0);
    // Check for symbol server in list of paths.
    static int indexOfSymbolServerPath(const QStringList &paths, QString *cacheDir = 0);

    // Nag user to add a symbol server to the path list on debugger startup.
    static bool promptToAddSymbolServer(const QString &settingsGroup, QStringList *symbolPaths);

private slots:
    void addSymbolServer();
};

} // namespace Internal
} // namespace Debugger

#endif // SYMBOLPATHLISTEDITOR_H
