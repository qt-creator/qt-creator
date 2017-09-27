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

#include "qmljsbundleprovider.h"

#include <coreplugin/icore.h>
#include <qmljs/qmljsbundle.h>
#include <qmljs/qmljsconstants.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>

#include <QDir>

namespace QmlJSTools {

using namespace QmlJS;

/*!
  \class QmlJSEditor::BasicBundleProvider

    \brief The BasicBundleProvider class sets up the default bundles for Qt and
    various QML states.
  */
BasicBundleProvider::BasicBundleProvider(QObject *parent) :
    IBundleProvider(parent)
{ }

QmlBundle BasicBundleProvider::defaultBundle(const QString &bundleInfoName)
{
    static bool wroteErrors = false;
    QmlBundle res;
    QString defaultBundlePath = Core::ICore::resourcePath()
            + QLatin1String("/qml-type-descriptions/")
            + bundleInfoName;
    if (!QFileInfo::exists(defaultBundlePath)) {
        qWarning() << "BasicBundleProvider: ERROR " << defaultBundlePath
                   << " not found";
        return res;
    }
    QStringList errors;
    if (!res.readFrom(defaultBundlePath, &errors) && ! wroteErrors) {
        qWarning() << "BasicBundleProvider: ERROR reading " << defaultBundlePath
                   << " : " << errors;
        wroteErrors = true;
    }
    return res;
}

QmlBundle BasicBundleProvider::defaultQt4QtQuick1Bundle()
{
    return defaultBundle(QLatin1String("qt4QtQuick1-bundle.json"));
}

QmlBundle BasicBundleProvider::defaultQt5QtQuick1Bundle()
{
    return defaultBundle(QLatin1String("qt5QtQuick1-bundle.json"));
}

QmlBundle BasicBundleProvider::defaultQt5QtQuick2Bundle()
{
    return defaultBundle(QLatin1String("qt5QtQuick2-bundle.json"));
}

QmlBundle BasicBundleProvider::defaultQbsBundle()
{
    return defaultBundle(QLatin1String("qbs-bundle.json"));
}

QmlBundle BasicBundleProvider::defaultQmltypesBundle()
{
    return defaultBundle(QLatin1String("qmltypes-bundle.json"));
}

QmlBundle BasicBundleProvider::defaultQmlprojectBundle()
{
    return defaultBundle(QLatin1String("qmlproject-bundle.json"));
}

void BasicBundleProvider::mergeBundlesForKit(ProjectExplorer::Kit *kit
                                             , QmlLanguageBundles &bundles
                                             , const QHash<QString,QString> &replacements)
{
    QHash<QString,QString> myReplacements = replacements;

    bundles.mergeBundleForLanguage(Dialect::QmlQbs, defaultQbsBundle());
    bundles.mergeBundleForLanguage(Dialect::QmlTypeInfo, defaultQmltypesBundle());
    bundles.mergeBundleForLanguage(Dialect::QmlProject, defaultQmlprojectBundle());

    QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitInformation::qtVersion(kit);
    if (!qtVersion) {
        QmlBundle b1(defaultQt4QtQuick1Bundle());
        bundles.mergeBundleForLanguage(Dialect::Qml, b1);
        bundles.mergeBundleForLanguage(Dialect::QmlQtQuick1, b1);
        QmlBundle b11(defaultQt5QtQuick1Bundle());
        bundles.mergeBundleForLanguage(Dialect::Qml, b11);
        bundles.mergeBundleForLanguage(Dialect::QmlQtQuick1, b11);
        QmlBundle b2(defaultQt5QtQuick2Bundle());
        bundles.mergeBundleForLanguage(Dialect::Qml, b2);
        bundles.mergeBundleForLanguage(Dialect::QmlQtQuick2, b2);
        bundles.mergeBundleForLanguage(Dialect::QmlQtQuick2Ui, b2);
        return;
    }
    QString qtImportsPath = qtVersion->qmakeProperty("QT_INSTALL_IMPORTS");
    QString qtQmlPath = qtVersion->qmlPath().toString();

    QSet<Core::Id> features = qtVersion->availableFeatures();
    if (features.contains(QtSupport::Constants::FEATURE_QT_QUICK_PREFIX)) {
        myReplacements.insert(QLatin1String("$(CURRENT_DIRECTORY)"), qtImportsPath);
        QDir qtQuick1Bundles(qtImportsPath);
        qtQuick1Bundles.setNameFilters(QStringList(QLatin1String("*-bundle.json")));
        QmlBundle qtQuick1Bundle;
        QFileInfoList list = qtQuick1Bundles.entryInfoList();
        for (int i = 0; i < list.size(); ++i) {
            QmlBundle bAtt;
            QStringList errors;
            if (!bAtt.readFrom(list.value(i).filePath(), &errors))
                qWarning() << "BasicBundleProvider: ERROR reading " << list[i].filePath() << " : "
                           << errors;
            qtQuick1Bundle.merge(bAtt);
        }
        if (!qtQuick1Bundle.supportedImports().contains(QLatin1String("QtQuick 1."),
                                                        PersistentTrie::Partial)) {
            if (qtVersion->qtVersion().majorVersion == 4)
                qtQuick1Bundle.merge(defaultQt4QtQuick1Bundle());
            else if (qtVersion->qtVersion().majorVersion > 4)
                qtQuick1Bundle.merge(defaultQt5QtQuick1Bundle());
        }
        qtQuick1Bundle.replaceVars(myReplacements);
        bundles.mergeBundleForLanguage(Dialect::Qml, qtQuick1Bundle);
        bundles.mergeBundleForLanguage(Dialect::QmlQtQuick1, qtQuick1Bundle);
    }
    if (features.contains(Core::Id::versionedId(QtSupport::Constants::FEATURE_QT_QUICK_PREFIX, 2))) {
        myReplacements.insert(QLatin1String("$(CURRENT_DIRECTORY)"), qtQmlPath);
        QDir qtQuick2Bundles(qtQmlPath);
        qtQuick2Bundles.setNameFilters(QStringList(QLatin1String("*-bundle.json")));
        QmlBundle qtQuick2Bundle;
        QFileInfoList list = qtQuick2Bundles.entryInfoList();
        for (int i = 0; i < list.size(); ++i) {
            QmlBundle bAtt;
            QStringList errors;
            if (!bAtt.readFrom(list.value(i).filePath(), &errors))
                qWarning() << "BasicBundleProvider: ERROR reading " << list[i].filePath() << " : "
                           << errors;
            qtQuick2Bundle.merge(bAtt);
        }
        if (!qtQuick2Bundle.supportedImports().contains(QLatin1String("QtQuick 2."),
                                                        PersistentTrie::Partial)) {
            qtQuick2Bundle.merge(defaultQt5QtQuick2Bundle());
        }
        qtQuick2Bundle.replaceVars(myReplacements);
        bundles.mergeBundleForLanguage(Dialect::Qml, qtQuick2Bundle);
        bundles.mergeBundleForLanguage(Dialect::QmlQtQuick2, qtQuick2Bundle);
        bundles.mergeBundleForLanguage(Dialect::QmlQtQuick2Ui, qtQuick2Bundle);
    }
}

} // end namespace QmlJSTools
