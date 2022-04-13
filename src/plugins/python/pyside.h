/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "pipsupport.h"

#include <utils/filepath.h>

#include <QTextDocument>

namespace TextEditor { class TextDocument; }
namespace ProjectExplorer { class RunConfiguration; }

namespace Python {
namespace Internal {

class PySideInstaller : public QObject
{
    Q_DECLARE_TR_FUNCTIONS(Python::Internal::PySideInstaller)
public:
    static PySideInstaller *instance();
    void checkPySideInstallation(const Utils::FilePath &python, TextEditor::TextDocument *document);

private:
    PySideInstaller();

    void installPyside(const Utils::FilePath &python,
                       const QString &pySide, TextEditor::TextDocument *document);
    void changeInterpreter(const QString &interpreterId, ProjectExplorer::RunConfiguration *runConfig);
    void handlePySideMissing(const Utils::FilePath &python,
                             const QString &pySide,
                             TextEditor::TextDocument *document);

    void runPySideChecker(const Utils::FilePath &python,
                          const QString &pySide,
                          TextEditor::TextDocument *document);
    static bool missingPySideInstallation(const Utils::FilePath &python, const QString &pySide);
    static QString importedPySide(const QString &text);

    QHash<Utils::FilePath, QList<TextEditor::TextDocument *>> m_infoBarEntries;
};

} // namespace Internal
} // namespace Python
