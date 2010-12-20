/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nicolas Arnaud-Cormos.
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

#ifndef MACROSPLUGIN_MACROTEXTFIND_H
#define MACROSPLUGIN_MACROTEXTFIND_H

#include <find/ifindsupport.h>

#include <QPointer>

namespace Macros {
namespace Internal {

class MacroTextFind : public Find::IFindSupport
{
    Q_OBJECT

public:
    MacroTextFind(Find::IFindSupport *currentFind);

    bool supportsReplace() const;
    Find::FindFlags supportedFindFlags() const;
    void resetIncrementalSearch();
    void clearResults();
    QString currentFindString() const;
    QString completedFindString() const;

    void highlightAll(const QString &txt, Find::FindFlags findFlags);
    Find::IFindSupport::Result findIncremental(const QString &txt, Find::FindFlags findFlags);
    Find::IFindSupport::Result findStep(const QString &txt, Find::FindFlags findFlags);
    void replace(const QString &before, const QString &after, Find::FindFlags findFlags);
    bool replaceStep(const QString &before, const QString &after, Find::FindFlags findFlags);
    int replaceAll(const QString &before, const QString &after, Find::FindFlags findFlags);

    void defineFindScope();
    void clearFindScope();

signals:
    void incrementalSearchReseted();
    void incrementalFound(const QString &txt, Find::FindFlags findFlags);
    void stepFound(const QString &txt, Find::FindFlags findFlags);
    void replaced(const QString &before, const QString &after,
        Find::FindFlags findFlags);
    void stepReplaced(const QString &before, const QString &after,
        Find::FindFlags findFlags);
    void allReplaced(const QString &before, const QString &after,
        Find::FindFlags findFlags);

private:
    QPointer<Find::IFindSupport> m_currentFind;
};

} // namespace Internal
} // namespace Macros

#endif // MACROSPLUGIN_MACROTEXTFIND_H
