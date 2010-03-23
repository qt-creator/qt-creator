/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef CUSTOMPROJECTWIZARD_H
#define CUSTOMPROJECTWIZARD_H

#include "../projectexplorer_export.h"

#include <coreplugin/basefilewizard.h>

#include <QtCore/QSharedPointer>
#include <QtCore/QList>
#include <QtCore/QMap>

QT_BEGIN_NAMESPACE
class QDir;
QT_END_NAMESPACE

namespace ProjectExplorer {
class CustomWizard;
struct CustomWizardPrivate;
class BaseProjectWizardDialog;

namespace Internal {
    struct CustomWizardParameters;
    struct CustomWizardContext;
}

// Factory for creating wizard. Can be registered under a name
// in CustomWizard.
class ICustomWizardFactory {
public:
    virtual CustomWizard *create(const Core::BaseFileWizardParameters& baseFileParameters,
                                 QObject *parent = 0) const = 0;
    virtual ~ICustomWizardFactory() {}
};

// Convenience template to create wizard classes.
template <class Wizard> class CustomWizardFactory : public ICustomWizardFactory {
    virtual CustomWizard *create(const Core::BaseFileWizardParameters& baseFileParameters,
                                 QObject *parent = 0) const
    { return new Wizard(baseFileParameters, parent); }
};

// A custom wizard presenting CustomWizardDialog (fields page containing
// path control) for wizards of type "class" or "file". Serves as base class
// for project wizards.

class PROJECTEXPLORER_EXPORT CustomWizard : public Core::BaseFileWizard
{
    Q_OBJECT
public:
    typedef QMap<QString, QString> FieldReplacementMap;
    typedef QSharedPointer<ICustomWizardFactory> ICustomWizardFactoryPtr;

    explicit CustomWizard(const Core::BaseFileWizardParameters& baseFileParameters,
                          QObject *parent = 0);
    virtual ~CustomWizard();

    // Can be reimplemented to create custom wizards. initWizardDialog() needs to be
    // called.
    virtual QWizard *createWizardDialog(QWidget *parent,
                                        const QString &defaultPath,
                                        const WizardPageList &extensionPages) const;

    virtual Core::GeneratedFiles generateFiles(const QWizard *w, QString *errorMessage) const;


    // Register a factory for a derived custom widget
    static void registerFactory(const QString &name, const ICustomWizardFactoryPtr &f);
    template <class Wizard> static void registerFactory(const QString &name)
        { registerFactory(name, ICustomWizardFactoryPtr(new CustomWizardFactory<Wizard>)); }

    // Create all wizards. As other plugins might register factories for derived
    // classes, call it in extensionsInitialized().
    static QList<CustomWizard*> createWizards();

    static void setVerbose(int);
    static int verbose();

protected:
    typedef QSharedPointer<Internal::CustomWizardParameters> CustomWizardParametersPtr;
    typedef QSharedPointer<Internal::CustomWizardContext> CustomWizardContextPtr;

    void initWizardDialog(QWizard *w, const QString &defaultPath,
                          const WizardPageList &extensionPages) const;

    // generate files in path
    Core::GeneratedFiles generateWizardFiles(const QString &path,
                                             const FieldReplacementMap &defaultFields,
                                             QString *errorMessage) const;
    // Create replacement map as static base fields + QWizard fields
    FieldReplacementMap replacementMap(const QWizard *w) const;

    CustomWizardParametersPtr parameters() const;
    CustomWizardContextPtr context() const;

private:
    void setParameters(const CustomWizardParametersPtr &p);

    static CustomWizard *createWizard(const CustomWizardParametersPtr &p, const Core::BaseFileWizardParameters &b);
    CustomWizardPrivate *d;
};

// A custom project wizard presenting CustomProjectWizardDialog
// (Project intro page and fields page) for wizards of type "project".
// Overwrites postGenerateFiles() to open the project file which is the
// last one by convention. Also inserts '%ProjectName%' into the base
// replacement map once the intro page is left to have it available
// for QLineEdit-type fields' default text.

class PROJECTEXPLORER_EXPORT CustomProjectWizard : public CustomWizard
{
    Q_OBJECT
public:
    explicit CustomProjectWizard(const Core::BaseFileWizardParameters& baseFileParameters,
                                 QObject *parent = 0);

    // Can be reimplemented to create custom project wizards.
    // initProjectWizardDialog() needs to be called.
    virtual QWizard *createWizardDialog(QWidget *parent,
                                        const QString &defaultPath,
                                        const WizardPageList &extensionPages) const;

    virtual Core::GeneratedFiles generateFiles(const QWizard *w, QString *errorMessage) const;

protected:
    virtual bool postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l, QString *errorMessage);

    void initProjectWizardDialog(BaseProjectWizardDialog *w, const QString &defaultPath,
                                 const WizardPageList &extensionPages) const;

private slots:
    void introPageLeft(const QString &project, const QString &path);
};

} // namespace ProjectExplorer

#endif // CUSTOMPROJECTWIZARD_H
