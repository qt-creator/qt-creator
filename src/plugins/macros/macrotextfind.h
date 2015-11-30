/**************************************************************************
**
** Copyright (C) 2015 Nicolas Arnaud-Cormos
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef MACROSPLUGIN_MACROTEXTFIND_H
#define MACROSPLUGIN_MACROTEXTFIND_H

#include <coreplugin/find/ifindsupport.h>

#include <QPointer>

namespace Macros {
namespace Internal {

class MacroTextFind : public Core::IFindSupport
{
    Q_OBJECT

public:
    MacroTextFind(Core::IFindSupport *currentFind);

    bool supportsReplace() const;
    Core::FindFlags supportedFindFlags() const;
    void resetIncrementalSearch();
    void clearHighlights();
    QString currentFindString() const;
    QString completedFindString() const;

    void highlightAll(const QString &txt, Core::FindFlags findFlags);
    Core::IFindSupport::Result findIncremental(const QString &txt, Core::FindFlags findFlags);
    Core::IFindSupport::Result findStep(const QString &txt, Core::FindFlags findFlags);
    void replace(const QString &before, const QString &after, Core::FindFlags findFlags);
    bool replaceStep(const QString &before, const QString &after, Core::FindFlags findFlags);
    int replaceAll(const QString &before, const QString &after, Core::FindFlags findFlags);

    void defineFindScope();
    void clearFindScope();

signals:
    void incrementalSearchReseted();
    void incrementalFound(const QString &txt, Core::FindFlags findFlags);
    void stepFound(const QString &txt, Core::FindFlags findFlags);
    void replaced(const QString &before, const QString &after,
        Core::FindFlags findFlags);
    void stepReplaced(const QString &before, const QString &after,
        Core::FindFlags findFlags);
    void allReplaced(const QString &before, const QString &after,
        Core::FindFlags findFlags);

private:
    QPointer<Core::IFindSupport> m_currentFind;
};

} // namespace Internal
} // namespace Macros

#endif // MACROSPLUGIN_MACROTEXTFIND_H
