// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "examplecheckout.h"

#include "studiowelcomeplugin.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>

#include <designerpaths.h>
#include <studiosettingspage.h>
#include <qmldesignerbase/qmldesignerbaseplugin.h>

#include <utils/algorithm.h>
#include <utils/networkaccessmanager.h>
#include <utils/qtcassert.h>
#include <utils/unarchiver.h>

#include <private/qqmldata_p.h>

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <projectexplorer/projectexplorer.h>

#include <qmldesigner/qmldesignerplugin.h>
#include <qmldesigner/utils/fileextractor.h>

#include <QDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QQuickItem>
#include <QQuickWidget>

#include <algorithm>

using namespace Utils;

void ExampleCheckout::registerTypes()
{
    static bool once = []() {
        qmlRegisterType<QmlDesigner::FileDownloader>("ExampleCheckout", 1, 0, "FileDownloader");
        qmlRegisterType<QmlDesigner::FileExtractor>("ExampleCheckout", 1, 0, "FileExtractor");
        return true;
    }();

    QTC_ASSERT(once, ;);
}

void DataModelDownloader::usageStatisticsDownloadExample(const QString &name)
{
    QmlDesigner::QmlDesignerPlugin::emitUsageStatistics("exampleDownload:" + name);
}

bool DataModelDownloader::downloadEnabled() const
{
    const Key lastQDSVersionEntry = "QML/Designer/EnableWelcomePageDownload";
    return Core::ICore::settings()->value(lastQDSVersionEntry, false).toBool();
}

QString DataModelDownloader::targetPath() const
{
    return QmlDesigner::Paths::examplesPathSetting();
}

static FilePath tempFilePath()
{
    QStandardPaths::StandardLocation location = QStandardPaths::CacheLocation;

    return FilePath::fromString(QStandardPaths::writableLocation(location))
        .pathAppended("QtDesignStudio");
}

DataModelDownloader::DataModelDownloader(QObject * /* parent */)
{
    auto fileInfo = targetFolder().toFileInfo();
    m_birthTime = fileInfo.lastModified();
    m_exists = fileInfo.exists();
    m_fileDownloader.setProbeUrl(true);

    connect(&m_fileDownloader,
            &QmlDesigner::FileDownloader::progressChanged,
            this,
            &DataModelDownloader::progressChanged);

    connect(&m_fileDownloader,
            &QmlDesigner::FileDownloader::downloadFailed,
            this,
            &DataModelDownloader::downloadFailed);

    const ExtensionSystem::PluginSpec *pluginSpec
        = Utils::findOrDefault(ExtensionSystem::PluginManager::plugins(),
                               Utils::equal(&ExtensionSystem::PluginSpec::name,
                                            QString("StudioWelcome")));

    if (!pluginSpec)
        return;

    ExtensionSystem::IPlugin *plugin = pluginSpec->plugin();

    if (!plugin)
        return;

    auto studioWelcomePlugin = qobject_cast<StudioWelcome::Internal::StudioWelcomePlugin *>(plugin);
    QmlDesigner::StudioConfigSettingsPage *settingsPage
        = QmlDesigner::QmlDesignerBasePlugin::studioConfigSettingsPage();
    if (studioWelcomePlugin && settingsPage) {
        QObject::connect(settingsPage,
                         &QmlDesigner::StudioConfigSettingsPage::examplesDownloadPathChanged,
                         this,
                         &DataModelDownloader::targetPathMustChange);
    }

    connect(&m_fileDownloader, &QmlDesigner::FileDownloader::finishedChanged, this, [this]() {
        m_started = false;

        if (m_fileDownloader.finished()) {
            const FilePath archiveFile = FilePath::fromString(m_fileDownloader.outputFile());
            const auto sourceAndCommand = Unarchiver::sourceAndCommand(archiveFile);
            QTC_ASSERT(sourceAndCommand, return);
            auto unarchiver = new Unarchiver;
            unarchiver->setSourceAndCommand(*sourceAndCommand);
            unarchiver->setDestDir(tempFilePath());
            QObject::connect(unarchiver, &Unarchiver::done, this, [this, unarchiver](bool success) {
                QTC_CHECK(success);
                unarchiver->deleteLater();
                emit finished();
            });
            unarchiver->start();
        }
    });
}

void DataModelDownloader::onAvailableChanged()
{
    m_available = m_fileDownloader.available();

    emit availableChanged();

    if (!m_available) {
        qWarning() << m_fileDownloader.url() << "failed to download";
        return;
    }

    if (!m_forceDownload && (m_fileDownloader.lastModified() <= m_birthTime))
        return;

    m_started = true;

    m_fileDownloader.start();
}

bool DataModelDownloader::start()
{
    if (!downloadEnabled()) {
        m_available = false;
        emit availableChanged();
        return false;
    }

    m_fileDownloader.setDownloadEnabled(true);
    m_fileDownloader.setUrl(QUrl::fromUserInput(
        "https://download.qt.io/learning/examples/qtdesignstudio/dataImports.zip"));

    m_started = false;

    connect(&m_fileDownloader, &QmlDesigner::FileDownloader::availableChanged, this, &DataModelDownloader::onAvailableChanged);
    return m_started;
}

bool DataModelDownloader::exists() const
{
    return m_exists;
}

bool DataModelDownloader::available() const
{
    return m_available;
}

FilePath DataModelDownloader::targetFolder() const
{
    return FilePath::fromUserInput(tempFilePath().toString() + "/" + "dataImports");
}

void DataModelDownloader::setForceDownload(bool b)
{
    m_forceDownload = b;
}

int DataModelDownloader::progress() const
{
    return m_fileDownloader.progress();
}
