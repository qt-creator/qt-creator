// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljsbundleprovider.h"

#include <coreplugin/icore.h>
#include <qmljs/qmljsbundle.h>
#include <qmljs/qmljsconstants.h>
#include <qtsupport/qtkitaspect.h>
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

QmlBundle BasicBundleProvider::defaultBundle(const QString &bundleInfoName,
                                             QtSupport::QtVersion *qtVersion)
{
    static bool wroteErrors = false;
    QmlBundle res;
    const Utils::FilePath defaultBundlePath = Core::ICore::resourcePath("qml-type-descriptions")
                                              / bundleInfoName;
    if (!defaultBundlePath.exists()) {
        qWarning() << "BasicBundleProvider: ERROR " << defaultBundlePath
                   << " not found";
        return res;
    }
    QStringList errors;
    bool stripVersions = qtVersion && qtVersion->qtVersion().majorVersion() > 5;
    if (!res.readFrom(defaultBundlePath.toString(), stripVersions, &errors) && !wroteErrors) {
        qWarning() << "BasicBundleProvider: ERROR reading " << defaultBundlePath
                   << " : " << errors;
        wroteErrors = true;
    }
    return res;
}

QmlBundle BasicBundleProvider::defaultQt5QtQuick2Bundle(QtSupport::QtVersion *qtVersion)
{
    QmlBundle result = defaultBundle(QLatin1String("qt5QtQuick2-bundle.json"), qtVersion);
    if (!qtVersion || qtVersion->qtVersion().majorVersion() < 6)
        return result;
    if (Utils::HostOsInfo::isMacHost())
        result.merge(defaultBundle(QLatin1String("qt5QtQuick2ext-macos-bundle.json"), qtVersion));
    if (Utils::HostOsInfo::isWindowsHost())
        result.merge(defaultBundle(QLatin1String("qt5QtQuick2ext-win-bundle.json"), qtVersion));
    return result;
}

QmlBundle BasicBundleProvider::defaultQbsBundle()
{
    return defaultBundle(QLatin1String("qbs-bundle.json"), nullptr);
}

QmlBundle BasicBundleProvider::defaultQmltypesBundle()
{
    return defaultBundle(QLatin1String("qmltypes-bundle.json"), nullptr);
}

QmlBundle BasicBundleProvider::defaultQmlprojectBundle()
{
    return defaultBundle(QLatin1String("qmlproject-bundle.json"), nullptr);
}

void BasicBundleProvider::mergeBundlesForKit(ProjectExplorer::Kit *kit
                                             , QmlLanguageBundles &bundles
                                             , const QHash<QString,QString> &replacements)
{
    QHash<QString,QString> myReplacements = replacements;

    bundles.mergeBundleForLanguage(Dialect::QmlQbs, defaultQbsBundle());
    bundles.mergeBundleForLanguage(Dialect::QmlTypeInfo, defaultQmltypesBundle());
    bundles.mergeBundleForLanguage(Dialect::QmlProject, defaultQmlprojectBundle());

    QtSupport::QtVersion *qtVersion = QtSupport::QtKitAspect::qtVersion(kit);
    if (!qtVersion) {
        QmlBundle b2(defaultQt5QtQuick2Bundle(qtVersion));
        bundles.mergeBundleForLanguage(Dialect::Qml, b2);
        bundles.mergeBundleForLanguage(Dialect::QmlQtQuick2, b2);
        bundles.mergeBundleForLanguage(Dialect::QmlQtQuick2Ui, b2);
        return;
    }
    QString qtQmlPath = qtVersion->qmlPath().toString();

    myReplacements.insert(QLatin1String("$(CURRENT_DIRECTORY)"), qtQmlPath);
    QDir qtQuick2Bundles(qtQmlPath);
    qtQuick2Bundles.setNameFilters(QStringList(QLatin1String("*-bundle.json")));
    QmlBundle qtQuick2Bundle;
    QFileInfoList list = qtQuick2Bundles.entryInfoList();
    bool stripVersions = qtVersion->qtVersion().majorVersion() > 5;
    for (int i = 0; i < list.size(); ++i) {
        QmlBundle bAtt;
        QStringList errors;
        if (!bAtt.readFrom(list.value(i).filePath(), stripVersions, &errors))
            qWarning() << "BasicBundleProvider: ERROR reading " << list[i].filePath() << " : "
                       << errors;
        qtQuick2Bundle.merge(bAtt);
    }
    if (!qtQuick2Bundle.supportedImports().contains(QLatin1String("QtQuick 2."),
                                                    PersistentTrie::Partial)) {
        qtQuick2Bundle.merge(defaultQt5QtQuick2Bundle(qtVersion));
    }
    qtQuick2Bundle.replaceVars(myReplacements);
    bundles.mergeBundleForLanguage(Dialect::Qml, qtQuick2Bundle);
    bundles.mergeBundleForLanguage(Dialect::QmlQtQuick2, qtQuick2Bundle);
    bundles.mergeBundleForLanguage(Dialect::QmlQtQuick2Ui, qtQuick2Bundle);

}

static QList<IBundleProvider *> g_bundleProviders;

IBundleProvider::IBundleProvider(QObject *parent)
    : QObject(parent)
{
    g_bundleProviders.append(this);
}

IBundleProvider::~IBundleProvider()
{
    g_bundleProviders.removeOne(this);
}

const QList<IBundleProvider *> IBundleProvider::allBundleProviders()
{
    return g_bundleProviders;
}

} // end namespace QmlJSTools
