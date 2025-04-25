// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../projectexplorer_export.h"

#include <coreplugin/basefilewizardfactory.h>

#include <QList>
#include <QMap>

QT_BEGIN_NAMESPACE
class QDir;
QT_END_NAMESPACE

namespace Utils { class Wizard; }

namespace ProjectExplorer {
class CustomWizard;
class BaseProjectWizardDialog;

namespace Internal {
class CustomWizardPrivate;
class CustomWizardContext;
class CustomWizardParameters;
}

// Documentation inside.
class PROJECTEXPLORER_EXPORT ICustomWizardMetaFactory
{
public:
    ICustomWizardMetaFactory(const QString &klass, Core::IWizardFactory::WizardKind kind);
    virtual ~ICustomWizardMetaFactory();

    virtual CustomWizard *create() const = 0;
    QString klass() const { return m_klass; }
    int kind() const { return m_kind; }

private:
    QString m_klass;
    Core::IWizardFactory::WizardKind m_kind;
};

// Convenience template to create wizard factory classes.
template <class Wizard> class CustomWizardMetaFactory : public ICustomWizardMetaFactory
{
public:
    CustomWizardMetaFactory(const QString &klass, Core::IWizardFactory::WizardKind kind) : ICustomWizardMetaFactory(klass, kind) { }
    CustomWizardMetaFactory(Core::IWizardFactory::WizardKind kind) : ICustomWizardMetaFactory(QString(), kind) { }
    CustomWizard *create() const override { return new Wizard; }
};

// Documentation inside.
class PROJECTEXPLORER_EXPORT CustomWizard : public Core::BaseFileWizardFactory
{
public:
    using FieldReplacementMap = QMap<QString, QString>;

    CustomWizard();
    ~CustomWizard() override;

    // Can be reimplemented to create custom wizards. initWizardDialog() needs to be
    // called.
    Core::BaseFileWizard *create(const Core::WizardDialogParameters &parameters) const override;

    Core::GeneratedFiles generateFiles(const QWizard *w, QString *errorMessage) const override;

    // Create all wizards. As other plugins might register factories for derived
    // classes, call it in extensionsInitialized().
    static void createWizards();

    static void setVerbose(int);
    static int verbose();

protected:
    using CustomWizardParametersPtr = std::shared_ptr<Internal::CustomWizardParameters>;
    using CustomWizardContextPtr = std::shared_ptr<Internal::CustomWizardContext>;

    // generate files in path
    Utils::Result<Core::GeneratedFiles> generateWizardFiles() const;
    // Create replacement map as static base fields + QWizard fields
    FieldReplacementMap replacementMap(const QWizard *w) const;
    Utils::Result<> writeFiles(const Core::GeneratedFiles &files) const override;

    CustomWizardParametersPtr parameters() const;
    CustomWizardContextPtr context() const;

    static CustomWizard *createWizard(const CustomWizardParametersPtr &p);

private:
    void setParameters(const CustomWizardParametersPtr &p);

    Internal::CustomWizardPrivate *d;
};

// Documentation inside.
class PROJECTEXPLORER_EXPORT CustomProjectWizard : public CustomWizard
{
public:
    CustomProjectWizard();

    static Utils::Result<> postGenerateOpen(const Core::GeneratedFiles &l);

protected:
    Core::BaseFileWizard *create(const Core::WizardDialogParameters &parameters) const override;

    Core::GeneratedFiles generateFiles(const QWizard *w, QString *errorMessage) const override;

    Utils::Result<> postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l) const override;

    void initProjectWizardDialog(BaseProjectWizardDialog *w, const Utils::FilePath &defaultPath,
                                 const QList<QWizardPage *> &extensionPages) const;

private:
    void handleProjectParametersChanged(const QString &project, const Utils::FilePath &path);
};

} // namespace ProjectExplorer
