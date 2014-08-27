/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef JSONWIZARDGENERATORFACTORY_H
#define JSONWIZARDGENERATORFACTORY_H

#include "../projectexplorer_export.h"

#include "jsonwizard.h"

#include <coreplugin/id.h>

#include <QList>
#include <QObject>

namespace Utils { class AbstractMacroExpander; }

namespace ProjectExplorer {

class JsonWizardGenerator
{
public:
    virtual ~JsonWizardGenerator() { }

    virtual Core::GeneratedFiles fileList(Utils::AbstractMacroExpander *expander,
                                          const QString &baseDir, const QString &projectDir,
                                          QString *errorMessage) = 0;
    virtual bool formatFile(const JsonWizard *wizard, Core::GeneratedFile *file, QString *errorMessage) = 0;
    virtual bool writeFile(const JsonWizard *wizard, Core::GeneratedFile *file, QString *errorMessage) = 0;
    virtual bool postWrite(const JsonWizard *wizard, Core::GeneratedFile *file, QString *errorMessage) = 0;

    virtual bool canKeepExistingFiles() const { return true; }

    enum OverwriteResult { OverwriteOk,  OverwriteError,  OverwriteCanceled };
    static OverwriteResult promptForOverwrite(JsonWizard::GeneratorFiles *files, QString *errorMessage);

    static bool formatFiles(const JsonWizard *wizard, QList<JsonWizard::GeneratorFile> *files, QString *errorMessage);
    static bool writeFiles(const JsonWizard *wizard, JsonWizard::GeneratorFiles *files, QString *errorMessage);
    static bool postWrite(const JsonWizard *wizard, JsonWizard::GeneratorFiles *files, QString *errorMessage);
};

class JsonWizardGeneratorFactory : public QObject
{
    Q_OBJECT

public:
    bool canCreate(Core::Id typeId) const { return m_typeIds.contains(typeId); }
    QList<Core::Id> supportedIds() const { return m_typeIds; }

    virtual JsonWizardGenerator *create(Core::Id typeId, const QVariant &data,
                                        const QString &path, const QString &platform,
                                        const QVariantMap &variables) = 0;

    // Basic syntax check for the data taken from the wizard.json file:
    virtual bool validateData(Core::Id typeId, const QVariant &data, QString *errorMessage) = 0;

protected:
    // This will add "PE.Wizard.Generator." in front of the suffixes and set those as supported typeIds
    void setTypeIdsSuffixes(const QStringList &suffixes);
    void setTypeIdsSuffix(const QString &suffix);

private:
    QList<Core::Id> m_typeIds;
};

} // namespace ProjectExplorer

#endif // JSONWIZARDGENERATORFACTORY_H
