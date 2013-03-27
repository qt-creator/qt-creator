/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef BASEFILEWIZARD_H
#define BASEFILEWIZARD_H

#include "core_global.h"
#include "generatedfile.h"
#include "featureprovider.h"

#include <coreplugin/dialogs/iwizard.h>

#include <extensionsystem/iplugin.h>

#include <QSharedDataPointer>
#include <QList>

QT_BEGIN_NAMESPACE
class QIcon;
class QWizard;
class QWizardPage;
class QDebug;
QT_END_NAMESPACE

namespace Utils {
    class Wizard;
}

namespace Core {

class IEditor;
class IFileWizardExtension;

class BaseFileWizardParameterData;
struct BaseFileWizardPrivate;

class CORE_EXPORT BaseFileWizardParameters
{
public:
    explicit BaseFileWizardParameters(IWizard::WizardKind kind = IWizard::FileWizard);
    BaseFileWizardParameters(const BaseFileWizardParameters &);
    BaseFileWizardParameters &operator=(const BaseFileWizardParameters&);
   ~BaseFileWizardParameters();

    void clear();

    IWizard::WizardKind kind() const;
    void setKind(IWizard::WizardKind k);

    QIcon icon() const;
    void setIcon(const QIcon &icon);

    QString description() const;
    void setDescription(const QString &description);

    QString displayName() const;
    void setDisplayName(const QString &name);

    QString id() const;
    void setId(const QString &id);

    QString category() const;
    void setCategory(const QString &category);

    QString displayCategory() const;
    void setDisplayCategory(const QString &trCategory);

    Core::FeatureSet requiredFeatures() const;
    void setRequiredFeatures(Core::FeatureSet features);

    Core::IWizard::WizardFlags flags() const;
    void setFlags(Core::IWizard::WizardFlags flags);

    QString descriptionImage() const;
    void setDescriptionImage(const QString &path);

private:
    QSharedDataPointer<BaseFileWizardParameterData> m_d;
};

CORE_EXPORT QDebug operator<<(QDebug d, const BaseFileWizardParameters &);

class CORE_EXPORT WizardDialogParameters
{
public:
    typedef QList<QWizardPage *> WizardPageList;

    enum DialogParameterEnum {
        ForceCapitalLetterForFileName = 0x01
    };
    Q_DECLARE_FLAGS(DialogParameterFlags, DialogParameterEnum)

    explicit WizardDialogParameters(const QString &defaultPath, const WizardPageList &extensionPages,
                                    const QString &platform, const Core::FeatureSet &requiredFeatures,
                                    DialogParameterFlags flags,
                                    QVariantMap extraValues)
        : m_defaultPath(defaultPath),
          m_extensionPages(extensionPages),
          m_selectedPlatform(platform),
          m_requiredFeatures(requiredFeatures),
          m_parameterFlags(flags),
          m_extraValues(extraValues)
    {}

    QString defaultPath() const
    { return m_defaultPath; }

    WizardPageList extensionPages() const
    { return m_extensionPages; }

    QString selectedPlatform() const
    { return m_selectedPlatform; }

    Core::FeatureSet requiredFeatures() const
    { return m_requiredFeatures; }

    DialogParameterFlags flags() const
    { return m_parameterFlags; }

    QVariantMap extraValues() const
    { return m_extraValues; }

private:
    QString m_defaultPath;
    WizardPageList m_extensionPages;
    QString m_selectedPlatform;
    Core::FeatureSet m_requiredFeatures;
    DialogParameterFlags m_parameterFlags;
    QVariantMap m_extraValues;
};

class CORE_EXPORT BaseFileWizard : public IWizard
{
    Q_OBJECT

public:
    virtual ~BaseFileWizard();

    // IWizard
    virtual WizardKind kind() const;
    virtual QIcon icon() const;
    virtual QString description() const;
    virtual QString displayName() const;
    virtual QString id() const;

    virtual QString category() const;
    virtual QString displayCategory() const;

    virtual QString descriptionImage() const;

    virtual void runWizard(const QString &path, QWidget *parent, const QString &platform, const QVariantMap &extraValues);
    virtual Core::FeatureSet requiredFeatures() const;
    virtual WizardFlags flags() const;

    static QString buildFileName(const QString &path, const QString &baseName, const QString &extension);
    static void setupWizard(QWizard *);
    static void applyExtensionPageShortTitle(Utils::Wizard *wizard, int pageId);

protected:
    typedef QList<QWizardPage *> WizardPageList;

    explicit BaseFileWizard(const BaseFileWizardParameters &parameters, QObject *parent = 0);

    BaseFileWizardParameters baseFileWizardParameters() const;

    virtual QWizard *createWizardDialog(QWidget *parent,
                                        const WizardDialogParameters &wizardDialogParameters) const = 0;

    virtual GeneratedFiles generateFiles(const QWizard *w,
                                         QString *errorMessage) const = 0;

    virtual bool writeFiles(const GeneratedFiles &files, QString *errorMessage);

    virtual bool postGenerateFiles(const QWizard *w, const GeneratedFiles &l, QString *errorMessage);

    static QString preferredSuffix(const QString &mimeType);

    enum OverwriteResult { OverwriteOk,  OverwriteError,  OverwriteCanceled };
    OverwriteResult promptOverwrite(GeneratedFiles *files,
                                    QString *errorMessage) const;
    static bool postGenerateOpenEditors(const GeneratedFiles &l, QString *errorMessage = 0);

private:
    BaseFileWizardPrivate *d;
};

class CORE_EXPORT StandardFileWizard : public BaseFileWizard
{
    Q_OBJECT

protected:
    explicit StandardFileWizard(const BaseFileWizardParameters &parameters, QObject *parent = 0);
    virtual QWizard *createWizardDialog(QWidget *parent, const WizardDialogParameters &wizardDialogParameters) const;
    virtual GeneratedFiles generateFiles(const QWizard *w, QString *errorMessage) const;
    virtual GeneratedFiles generateFilesFromPath(const QString &path, const QString &name,
                                                 QString *errorMessage) const = 0;
};

template <class WizardClass>
QList<WizardClass*> createMultipleBaseFileWizardInstances(const QList<BaseFileWizardParameters> &parametersList, ExtensionSystem::IPlugin *plugin)
{
    QList<WizardClass*> list;
    foreach (const BaseFileWizardParameters &parameters, parametersList) {
        WizardClass *wc = new WizardClass(parameters, 0);
        plugin->addAutoReleasedObject(wc);
        list << wc;
    }
    return list;
}

} // namespace Core

Q_DECLARE_OPERATORS_FOR_FLAGS(Core::GeneratedFile::Attributes)
Q_DECLARE_OPERATORS_FOR_FLAGS(Core::WizardDialogParameters::DialogParameterFlags)

#endif // BASEFILEWIZARD_H
