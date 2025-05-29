// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fontresourcesmodel.h"
#include "fileresourcesmodel.h"

#include <cmakeprojectmanager/cmakekitaspect.h>
#include <designermcumanager.h>
#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <qmlprojectmanager/qmlproject.h>
#include <qmlprojectmanager/qmlprojectconstants.h>
#include <utils/algorithm.h>
#include <utils/expected.h>
#include <utils/filepath.h>

#include <QFontDatabase>
#include <QLatin1String>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QUrl>

#include <algorithm>
#include <iterator>

using namespace Qt::Literals::StringLiterals;

namespace {

QStringList makeFontFilesFilterList()
{
    QStringList filterList;
    filterList.reserve(std::size(QmlProjectManager::Constants::QDS_FONT_FILES_FILTERS));
    std::ranges::transform(QmlProjectManager::Constants::QDS_FONT_FILES_FILTERS,
                           std::back_inserter(filterList),
                           [](const char *filter) { return QString::fromLatin1(filter); });

    return filterList;
}

const QStringList &fontFilesFilterList()
{
    static const QStringList list = makeFontFilesFilterList();
    return list;
}

QString fontFamily(const QString &fontPath)
{
    const int fontId = QFontDatabase::addApplicationFont(fontPath);
    QString fontFamily = QFontDatabase::applicationFontFamilies(fontId).front();
    QFontDatabase::removeApplicationFont(fontId);
    return fontFamily;
}

Utils::expected_str<QSet<QString>> mcuFonts()
{
    Utils::expected_str<Utils::FilePath> mcuFontsDir = QmlProjectManager::mcuFontsDir();
    if (!mcuFontsDir) {
        return Utils::make_unexpected(mcuFontsDir.error());
    }

    QSet<QString> fonts;
    const Utils::FilePaths fontFiles = mcuFontsDir->dirEntries({fontFilesFilterList(), QDir::Files});
    for (const auto &file : fontFiles) {
        fonts.insert(fontFamily(file.absoluteFilePath().toFSPathString()));
    }

    return fonts;
}

QSet<QString> systemFonts()
{
    QSet<QString> defaultFonts{
        "Arial",
        "Courier",
        "Tahoma",
        "Times New Roman",
        "Verdana",
    };

    if (QmlDesigner::DesignerMcuManager::instance().isMCUProject()) {
        const auto fonts = mcuFonts();
        if (!fonts) {
            qWarning() << "Failed to load MCU fonts." << fonts.error();
            return defaultFonts;
        }

        return *fonts;
    }

    return defaultFonts;
}

} // namespace

void FontResourcesModel::registerDeclarativeType()
{
    qmlRegisterType<FontResourcesModel>("HelperWidgets", 2, 0, "FontResourcesModel");
}

QVariant FontResourcesModel::modelNodeBackend()
{
    return {};
}

FontResourcesModel::FontResourcesModel(QObject *parent)
    : QObject{parent}
    , m_resourceModel{std::make_unique<FileResourcesModel>()}
{
    m_resourceModel->setFilter(fontFilesFilterList().join(' '));
}

void FontResourcesModel::setModelNodeBackend(const QVariant &modelNodeBackend)
{
    m_resourceModel->setModelNodeBackend(modelNodeBackend);
}

QStringList FontResourcesModel::model() const
{
    QSet<QString> fonts;
    for (const auto &item : m_resourceModel->model()) {
        fonts.insert(fontFamily(item.absoluteFilePath()));
    }

    fonts.unite(systemFonts());

    auto ret = QStringList{fonts.cbegin(), fonts.cend()};
    ret.sort();

    return ret;
}

void FontResourcesModel::refreshModel()
{
    m_resourceModel->refreshModel();
}

void FontResourcesModel::openFileDialog(const QString &customPath)
{
    m_resourceModel->openFileDialog(customPath);
}

QString FontResourcesModel::resolve(const QString &relative) const
{
    return m_resourceModel->resolve(relative);
}

bool FontResourcesModel::isLocal(const QString &path) const
{
    return m_resourceModel->isLocal(path);
}
