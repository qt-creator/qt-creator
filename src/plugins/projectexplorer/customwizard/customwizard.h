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

#ifndef CUSTOMWIZARD_H
#define CUSTOMWIZARD_H

#include "../projectexplorer_export.h"

#include <coreplugin/basefilewizard.h>

#include <QSharedPointer>
#include <QList>
#include <QMap>

QT_BEGIN_NAMESPACE
class QDir;
QT_END_NAMESPACE

namespace Utils { class Wizard; }

namespace ProjectExplorer {
class CustomWizard;
class CustomWizardPrivate;
class BaseProjectWizardDialog;

namespace Internal {
    class CustomWizardParameters;
    class CustomWizardContext;
}

// Documentation inside.
class PROJECTEXPLORER_EXPORT ICustomWizardFactory : public QObject
{
    Q_OBJECT

public:
    ICustomWizardFactory(const QString &klass, Core::IWizard::WizardKind kind) :
        m_klass(klass), m_kind(kind)
    { }

    virtual CustomWizard *create() const = 0;
    QString klass() const { return m_klass; }
    int kind() const { return m_kind; }

private:
    QString m_klass;
    Core::IWizard::WizardKind m_kind;
};

// Convenience template to create wizard factory classes.
template <class Wizard> class CustomWizardFactory : public ICustomWizardFactory
{
public:
    CustomWizardFactory(const QString &klass, Core::IWizard::WizardKind kind) : ICustomWizardFactory(klass, kind) { }
    CustomWizardFactory(Core::IWizard::WizardKind kind) : ICustomWizardFactory(QString(), kind) { }
    CustomWizard *create() const { return new Wizard; }
};

// Documentation inside.
class PROJECTEXPLORER_EXPORT CustomWizard : public Core::BaseFileWizard
{
    Q_OBJECT

public:
    typedef QMap<QString, QString> FieldReplacementMap;
    typedef QSharedPointer<ICustomWizardFactory> ICustomWizardFactoryPtr;

    CustomWizard();
    ~CustomWizard();

    // Can be reimplemented to create custom wizards. initWizardDialog() needs to be
    // called.
    QWizard *createWizardDialog(QWidget *parent,
                                const Core::WizardDialogParameters &wizardDialogParameters) const;

    Core::GeneratedFiles generateFiles(const QWizard *w, QString *errorMessage) const;

    // Create all wizards. As other plugins might register factories for derived
    // classes, call it in extensionsInitialized().
    static QList<CustomWizard*> createWizards();

    static void setVerbose(int);
    static int verbose();

protected:
    typedef QSharedPointer<Internal::CustomWizardParameters> CustomWizardParametersPtr;
    typedef QSharedPointer<Internal::CustomWizardContext> CustomWizardContextPtr;

    void initWizardDialog(Utils::Wizard *w, const QString &defaultPath,
                          const WizardPageList &extensionPages) const;

    // generate files in path
    Core::GeneratedFiles generateWizardFiles(QString *errorMessage) const;
    // Create replacement map as static base fields + QWizard fields
    FieldReplacementMap replacementMap(const QWizard *w) const;
    bool writeFiles(const Core::GeneratedFiles &files, QString *errorMessage);

    CustomWizardParametersPtr parameters() const;
    CustomWizardContextPtr context() const;

    static CustomWizard *createWizard(const CustomWizardParametersPtr &p, const Core::IWizard::Data &b);

private:
    void setParameters(const CustomWizardParametersPtr &p);

    static CustomWizard *createWizard(const CustomWizardParametersPtr &p);
    CustomWizardPrivate *d;
};

// Documentation inside.
class PROJECTEXPLORER_EXPORT CustomProjectWizard : public CustomWizard
{
    Q_OBJECT

public:
    CustomProjectWizard();

    static bool postGenerateOpen(const Core::GeneratedFiles &l, QString *errorMessage = 0);

protected:
    QWizard *createWizardDialog(QWidget *parent,
                                        const Core::WizardDialogParameters &wizardDialogParameters) const;

    Core::GeneratedFiles generateFiles(const QWizard *w, QString *errorMessage) const;

signals:
    void projectLocationChanged(const QString &path);

protected:
    bool postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l, QString *errorMessage);

    void initProjectWizardDialog(BaseProjectWizardDialog *w, const QString &defaultPath,
                                 const WizardPageList &extensionPages) const;

private slots:
    void projectParametersChanged(const QString &project, const QString &path);
};

} // namespace ProjectExplorer

#endif // CUSTOMWIZARD_H
