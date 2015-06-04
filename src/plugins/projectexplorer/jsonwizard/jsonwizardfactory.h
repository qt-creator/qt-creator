/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef JSONWIZARDFACTORY_H
#define JSONWIZARDFACTORY_H

#include "../projectexplorer_export.h"

#include <coreplugin/iwizardfactory.h>

#include <utils/fileutils.h>

#include <QMap>
#include <QVariant>

QT_BEGIN_NAMESPACE
class QDir;
QT_END_NAMESPACE

namespace ProjectExplorer {

class JsonWizardFactory;
class JsonWizardPageFactory;
class JsonWizardGeneratorFactory;
class ProjectExplorerPlugin;
class ProjectExplorerPluginPrivate;

// Documentation inside.
class PROJECTEXPLORER_EXPORT JsonWizardFactory : public Core::IWizardFactory
{
    Q_OBJECT

public:
    // Add search paths for wizard.json files. All subdirs are going to be checked.
    static void addWizardPath(const Utils::FileName &path);

    // actual interface of the wizard factory:
    class Generator {
    public:
        bool isValid() const { return typeId.isValid(); }

        Core::Id typeId;
        QVariant data;
    };

    class Page {
    public:
        bool isValid() const { return typeId.isValid(); }
        QString title;
        QString subTitle;
        QString shortTitle;
        int index; // page index in the wizard
        Core::Id typeId;
        QVariant enabled;
        QVariant data;
    };

    static void registerPageFactory(JsonWizardPageFactory *factory);
    static void registerGeneratorFactory(JsonWizardGeneratorFactory *factory);

    ~JsonWizardFactory();

    static QList<QVariant> objectOrList(const QVariant &data, QString *errorMessage);

    static QString localizedString(const QVariant &value);

    bool isAvailable(const QString &platformName) const override;

private:
    Utils::Wizard *runWizardImpl(const QString &path, QWidget *parent, const QString &platform,
                                 const QVariantMap &variables) override;

    // Create all wizards. As other plugins might register factories for derived
    // classes. Called when the new file dialog is shown for the first time.
    static QList<IWizardFactory *> createWizardFactories();
    static JsonWizardFactory *createWizardFactory(const QVariantMap &data, const QDir &baseDir,
                                                  QString *errorMessage);
    static QList<Utils::FileName> &searchPaths();

    static void setVerbose(int level);
    static int verbose();

    static void destroyAllFactories();
    bool initialize(const QVariantMap &data, const QDir &baseDir, QString *errorMessage);

    QVariant m_enabledExpression;
    QString m_wizardDir;
    QList<Generator> m_generators;

    QList<Page> m_pages;
    QMap<QString, QString> m_options;

    Core::FeatureSet m_preferredFeatures;

    static int m_verbose;

    friend class ProjectExplorerPlugin;
    friend class ProjectExplorerPluginPrivate;
};

} //namespace ProjectExplorer

#endif // JSONWIZARDFACTORY_H
