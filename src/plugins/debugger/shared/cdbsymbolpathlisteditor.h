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

#pragma once

#include <utils/pathlisteditor.h>

#include <QDialog>

namespace Utils { class PathChooser; }

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

class CacheDirectoryDialog : public QDialog {
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
    enum SymbolPathMode{
        SymbolServerPath,
        SymbolCachePath
    };

    explicit CdbSymbolPathListEditor(QWidget *parent = 0);

    static bool promptCacheDirectory(QWidget *parent, QString *cacheDirectory);

    // Pre- and Postfix used to build a symbol server path specification
    static const char *symbolServerPrefixC;
    static const char *symbolServerPostfixC;
    static const char *symbolCachePrefixC;

    // Format a symbol path specification
    static QString symbolPath(const QString &cacheDir, SymbolPathMode mode);
    // Check for a symbol server path and extract local cache directory
    static bool isSymbolServerPath(const QString &path, QString *cacheDir = 0);
    // Check for a symbol cache path and extract local cache directory
    static bool isSymbolCachePath(const QString &path, QString *cacheDir = 0);
    // Check for symbol server in list of paths.
    static int indexOfSymbolPath(const QStringList &paths, SymbolPathMode mode, QString *cacheDir = 0);

private:
    void addSymbolPath(SymbolPathMode mode);
    void setupSymbolPaths();
};

} // namespace Internal
} // namespace Debugger
