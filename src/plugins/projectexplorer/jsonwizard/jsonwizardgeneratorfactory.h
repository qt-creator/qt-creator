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

#include "../projectexplorer_export.h"

#include "jsonwizard.h"

#include <coreplugin/id.h>

#include <QList>
#include <QObject>

namespace Utils { class MacroExpander; }

namespace ProjectExplorer {

class JsonWizardGenerator
{
public:
    virtual ~JsonWizardGenerator() { }

    virtual Core::GeneratedFiles fileList(Utils::MacroExpander *expander,
                                          const QString &baseDir, const QString &projectDir,
                                          QString *errorMessage) = 0;
    virtual bool formatFile(const JsonWizard *wizard, Core::GeneratedFile *file, QString *errorMessage);
    virtual bool writeFile(const JsonWizard *wizard, Core::GeneratedFile *file, QString *errorMessage);
    virtual bool postWrite(const JsonWizard *wizard, Core::GeneratedFile *file, QString *errorMessage);
    virtual bool polish(const JsonWizard *wizard, Core::GeneratedFile *file, QString *errorMessage);
    virtual bool allDone(const JsonWizard *wizard, Core::GeneratedFile *file, QString *errorMessage);

    virtual bool canKeepExistingFiles() const { return true; }

    enum OverwriteResult { OverwriteOk,  OverwriteError,  OverwriteCanceled };
    static OverwriteResult promptForOverwrite(JsonWizard::GeneratorFiles *files, QString *errorMessage);

    static bool formatFiles(const JsonWizard *wizard, QList<JsonWizard::GeneratorFile> *files, QString *errorMessage);
    static bool writeFiles(const JsonWizard *wizard, JsonWizard::GeneratorFiles *files, QString *errorMessage);
    static bool postWrite(const JsonWizard *wizard, JsonWizard::GeneratorFiles *files, QString *errorMessage);
    static bool polish(const JsonWizard *wizard, JsonWizard::GeneratorFiles *files, QString *errorMessage);
    static bool allDone(const JsonWizard *wizard, JsonWizard::GeneratorFiles *files, QString *errorMessage);
};

class JsonWizardGeneratorFactory : public QObject
{
    Q_OBJECT

public:
    bool canCreate(Core::Id typeId) const { return m_typeIds.contains(typeId); }
    QList<Core::Id> supportedIds() const { return m_typeIds; }

    virtual JsonWizardGenerator *create(Core::Id typeId, const QVariant &data,
                                        const QString &path, Core::Id platform,
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

namespace Internal {

class FileGeneratorFactory : public JsonWizardGeneratorFactory
{
    Q_OBJECT

public:
    FileGeneratorFactory();

    JsonWizardGenerator *create(Core::Id typeId, const QVariant &data,
                                const QString &path, Core::Id platform,
                                const QVariantMap &variables) override;
    bool validateData(Core::Id typeId, const QVariant &data, QString *errorMessage) override;
};

class ScannerGeneratorFactory : public JsonWizardGeneratorFactory
{
    Q_OBJECT

public:
    ScannerGeneratorFactory();

    JsonWizardGenerator *create(Core::Id typeId, const QVariant &data,
                                const QString &path, Core::Id platform,
                                const QVariantMap &variables) override;
    bool validateData(Core::Id typeId, const QVariant &data, QString *errorMessage) override;
};

} // namespace Internal
} // namespace ProjectExplorer
