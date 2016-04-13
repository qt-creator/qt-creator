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

#include "jsonwizardgeneratorfactory.h"

#include <QVariant>

namespace ProjectExplorer {
namespace Internal {

// Documentation inside.
class JsonWizardFileGenerator : public JsonWizardGenerator
{
public:
    bool setup(const QVariant &data, QString *errorMessage);

    Core::GeneratedFiles fileList(Utils::MacroExpander *expander,
                                  const QString &wizardDir, const QString &projectDir,
                                  QString *errorMessage) override;

    bool writeFile(const JsonWizard *wizard, Core::GeneratedFile *file, QString *errorMessage) override;

private:
    class File {
    public:
        bool keepExisting = false;
        QString source;
        QString target;
        QVariant condition = true;
        QVariant isBinary = false;
        QVariant overwrite = false;
        QVariant openInEditor = false;
        QVariant openAsProject = false;

        QList<JsonWizard::OptionDefinition> options;
    };

    Core::GeneratedFile generateFile(const File &file, Utils::MacroExpander *expander,
                                     QString *errorMessage);

    QList<File> m_fileList;
};

} // namespace Internal
} // namespace ProjectExplorer
