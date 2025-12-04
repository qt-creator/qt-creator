// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fontresourcesmodel.h"
#include "fileresourcesmodel.h"
#include "propertyeditortracing.h"

#include <cmakeprojectmanager/cmakekitaspect.h>
#include <designermcumanager.h>
#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/taskhub.h>
#include <qmlprojectmanager/qmlproject.h>
#include <qmlprojectmanager/qmlprojectconstants.h>
#include <utils/algorithm.h>
#include <utils/expected.h>
#include <utils/filepath.h>
#include <utils/id.h>

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

using QmlDesigner::PropertyEditorTracing::category;

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

std::optional<QString> fontFamily(const QString &fontPath)
{
    const int fontId = QFontDatabase::addApplicationFont(fontPath);
    const QStringList fontFamilies = QFontDatabase::applicationFontFamilies(fontId);
    QFontDatabase::removeApplicationFont(fontId);

    if (fontFamilies.isEmpty()) {
        const auto issueType = ProjectExplorer::Task::TaskType::Warning;
        const auto issueDesc = "Cannot determine font family for font file '%1'"_L1.arg(fontPath);
        const Utils::Id issueCategory = ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM;
        ProjectExplorer::TaskHub::addTask(issueType, issueDesc, issueCategory);
        return {};
    }

    return fontFamilies.front();
}

Utils::Result<QSet<QString>> mcuFonts()
{
    Utils::Result<Utils::FilePath> mcuFontsDir = QmlProjectManager::mcuFontsDir();
    if (!mcuFontsDir) {
        return Utils::make_unexpected(mcuFontsDir.error());
    }

    QSet<QString> fonts;
    const Utils::FilePaths fontFiles = mcuFontsDir->dirEntries({fontFilesFilterList(), QDir::Files});
    for (const auto &file : fontFiles) {
        auto family = fontFamily(file.absoluteFilePath().toFSPathString());
        if (family) {
            fonts.insert(*std::move(family));
        }
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
    NanotraceHR::Tracer tracer{"font resources model register declarative type", category()};

    qmlRegisterType<FontResourcesModel>("HelperWidgets", 2, 0, "FontResourcesModel");
}

QVariant FontResourcesModel::modelNodeBackend()
{
    NanotraceHR::Tracer tracer{"font resources model model node backend", category()};

    return {};
}

FontResourcesModel::FontResourcesModel(QObject *parent)
    : QObject{parent}
    , m_resourceModel{std::make_unique<FileResourcesModel>()}
{
    NanotraceHR::Tracer tracer{"font resources model constructor", category()};

    m_resourceModel->setFilter(fontFilesFilterList().join(' '));
}

void FontResourcesModel::setModelNodeBackend(const QVariant &modelNodeBackend)
{
    NanotraceHR::Tracer tracer{"file resources model set model node backend", category()};

    m_resourceModel->setModelNodeBackend(modelNodeBackend);
}

QStringList FontResourcesModel::model() const
{
    NanotraceHR::Tracer tracer{"file resources model model", category()};

    QSet<QString> fonts;
    for (const auto &item : m_resourceModel->model()) {
        auto family = fontFamily(item.absoluteFilePath());
        if (family) {
            fonts.insert(*std::move(family));
        }
    }

    fonts.unite(systemFonts());

    auto ret = QStringList{fonts.cbegin(), fonts.cend()};
    ret.sort();

    return ret;
}

void FontResourcesModel::refreshModel()
{
    NanotraceHR::Tracer tracer{"file resources model refresh model", category()};

    m_resourceModel->refreshModel();
}

void FontResourcesModel::openFileDialog(const QString &customPath)
{
    NanotraceHR::Tracer tracer{"file resources model open file dialog", category()};

    m_resourceModel->openFileDialog(customPath);
}

QString FontResourcesModel::resolve(const QString &relative) const
{
    NanotraceHR::Tracer tracer{"file resources model resolve", category()};

    return m_resourceModel->resolve(relative);
}

bool FontResourcesModel::isLocal(const QString &path) const
{
    NanotraceHR::Tracer tracer{"file resources model is local", category()};

    return m_resourceModel->isLocal(path);
}
