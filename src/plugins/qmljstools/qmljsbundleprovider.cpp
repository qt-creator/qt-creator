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

#include "qmljsbundleprovider.h"

#include <coreplugin/icore.h>
#include <qmljs/qmljsbundle.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>

#include <QDir>

namespace QmlJSTools {

namespace {
typedef QmlJS::Document::Language Language;
typedef QmlJS::QmlBundle QmlBundle;
typedef QmlJS::QmlLanguageBundles QmlLanguageBundles;
}

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
    if (!QFileInfo(defaultBundlePath).exists()) {
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
                                             , QmlJS::QmlLanguageBundles &bundles
                                             , const QHash<QString,QString> &replacements)
{
    typedef QmlJS::Document Doc;
    QHash<QString,QString> myReplacements = replacements;

    bundles.mergeBundleForLanguage(Doc::QmlQbsLanguage, defaultQbsBundle());
    bundles.mergeBundleForLanguage(Doc::QmlTypeInfoLanguage, defaultQmltypesBundle());
    bundles.mergeBundleForLanguage(Doc::QmlProjectLanguage, defaultQmlprojectBundle());

    QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitInformation::qtVersion(kit);
    if (!qtVersion) {
        QmlBundle b1(defaultQt4QtQuick1Bundle());
        bundles.mergeBundleForLanguage(Doc::QmlLanguage, b1);
        bundles.mergeBundleForLanguage(Doc::QmlQtQuick1Language, b1);
        QmlBundle b11(defaultQt5QtQuick1Bundle());
        bundles.mergeBundleForLanguage(Doc::QmlLanguage, b11);
        bundles.mergeBundleForLanguage(Doc::QmlQtQuick1Language, b11);
        QmlBundle b2(defaultQt5QtQuick2Bundle());
        bundles.mergeBundleForLanguage(Doc::QmlLanguage, b2);
        bundles.mergeBundleForLanguage(Doc::QmlQtQuick2Language, b2);
        return;
    }
    QString qtImportsPath = qtVersion->qmakeProperty("QT_INSTALL_IMPORTS");
    QString qtQmlPath = qtVersion->qmakeProperty("QT_INSTALL_QML");

    Core::FeatureSet features = qtVersion->availableFeatures();
    if (features.contains(Core::Feature(QtSupport::Constants::FEATURE_QT_QUICK))
            || features.contains(Core::Feature(QtSupport::Constants::FEATURE_QT_QUICK_1))
            || features.contains(Core::Feature(QtSupport::Constants::FEATURE_QT_QUICK_1_1))) {
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
                                                        QmlJS::PersistentTrie::Partial)) {
            if (qtVersion->qtVersion().majorVersion == 4)
                qtQuick1Bundle.merge(defaultQt4QtQuick1Bundle());
            else if (qtVersion->qtVersion().majorVersion > 4)
                qtQuick1Bundle.merge(defaultQt5QtQuick1Bundle());
        }
        qtQuick1Bundle.replaceVars(myReplacements);
        bundles.mergeBundleForLanguage(Doc::QmlLanguage, qtQuick1Bundle);
        bundles.mergeBundleForLanguage(Doc::QmlQtQuick1Language, qtQuick1Bundle);
    }
    if (features.contains(Core::Feature(QtSupport::Constants::FEATURE_QT_QUICK_2))) {
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
                                                        QmlJS::PersistentTrie::Partial)) {
            qtQuick2Bundle.merge(defaultQt5QtQuick2Bundle());
        }
        qtQuick2Bundle.replaceVars(myReplacements);
        bundles.mergeBundleForLanguage(Doc::QmlLanguage, qtQuick2Bundle);
        bundles.mergeBundleForLanguage(Doc::QmlQtQuick2Language, qtQuick2Bundle);
    }
}

} // end namespace QmlJSTools
