/****************************************************************************
**
** Copyright (C) 2016 Nicolas Arnaud-Cormos
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
