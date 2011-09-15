/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef BASEFILEWIZARD_H
#define BASEFILEWIZARD_H

#include "core_global.h"
#include "generatedfile.h"

#include <coreplugin/dialogs/iwizard.h>

#include <QtCore/QSharedDataPointer>
#include <QtCore/QList>

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

private:
    QSharedDataPointer<BaseFileWizardParameterData> m_d;
};

CORE_EXPORT QDebug operator<<(QDebug d, const BaseFileWizardParameters &);

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

    virtual void runWizard(const QString &path, QWidget *parent);

    static QString buildFileName(const QString &path, const QString &baseName, const QString &extension);
    static void setupWizard(QWizard *);
    static void applyExtensionPageShortTitle(Utils::Wizard *wizard, int pageId);

protected:
    typedef QList<QWizardPage *> WizardPageList;

    explicit BaseFileWizard(const BaseFileWizardParameters &parameters, QObject *parent = 0);

    virtual QWizard *createWizardDialog(QWidget *parent,
                                        const QString &defaultPath,
                                        const WizardPageList &extensionPages) const = 0;
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
    virtual QWizard *createWizardDialog(QWidget *parent,
                                        const QString &defaultPath,
                                        const WizardPageList &extensionPages) const;
    virtual GeneratedFiles generateFiles(const QWizard *w, QString *errorMessage) const;
    virtual GeneratedFiles generateFilesFromPath(const QString &path, const QString &name,
                                                 QString *errorMessage) const = 0;
};

} // namespace Core

Q_DECLARE_OPERATORS_FOR_FLAGS(Core::GeneratedFile::Attributes)

#endif // BASEFILEWIZARD_H
