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

#include <coreplugin/basefilewizardfactory.h>

#include <QSharedPointer>
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
class PROJECTEXPLORER_EXPORT ICustomWizardMetaFactory : public QObject
{
    Q_OBJECT

public:
    ICustomWizardMetaFactory(const QString &klass, Core::IWizardFactory::WizardKind kind) :
        m_klass(klass), m_kind(kind)
    { }

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
    Q_OBJECT

public:
    typedef QMap<QString, QString> FieldReplacementMap;

    CustomWizard();
    ~CustomWizard() override;

    // Can be reimplemented to create custom wizards. initWizardDialog() needs to be
    // called.
    Core::BaseFileWizard *create(QWidget *parent, const Core::WizardDialogParameters &parameters) const override;

    Core::GeneratedFiles generateFiles(const QWizard *w, QString *errorMessage) const override;

    // Create all wizards. As other plugins might register factories for derived
    // classes, call it in extensionsInitialized().
    static QList<IWizardFactory *> createWizards();

    static void setVerbose(int);
    static int verbose();

protected:
    typedef QSharedPointer<Internal::CustomWizardParameters> CustomWizardParametersPtr;
    typedef QSharedPointer<Internal::CustomWizardContext> CustomWizardContextPtr;

    // generate files in path
    Core::GeneratedFiles generateWizardFiles(QString *errorMessage) const;
    // Create replacement map as static base fields + QWizard fields
    FieldReplacementMap replacementMap(const QWizard *w) const;
    bool writeFiles(const Core::GeneratedFiles &files, QString *errorMessage) const override;

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
    Q_OBJECT

public:
    CustomProjectWizard();

    static bool postGenerateOpen(const Core::GeneratedFiles &l, QString *errorMessage = nullptr);

signals:
    void projectLocationChanged(const QString &path);

protected:
    Core::BaseFileWizard *create(QWidget *parent, const Core::WizardDialogParameters &parameters) const override;

    Core::GeneratedFiles generateFiles(const QWizard *w, QString *errorMessage) const override;

    bool postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l, QString *errorMessage) const override;

    void initProjectWizardDialog(BaseProjectWizardDialog *w, const QString &defaultPath,
                                 const QList<QWizardPage *> &extensionPages) const;

private:
    void projectParametersChanged(const QString &project, const QString &path);
};

} // namespace ProjectExplorer
