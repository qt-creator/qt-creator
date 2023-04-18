// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "modulescanner.h"

#ifdef QDS_HAS_QMLPRIVATE
#include <private/qqmldirparser_p.h>
#endif

#include <QFile>

#include <filesystem>

namespace QmlDesigner {

namespace {

std::optional<QString> contentAsQString(const QString &filePath)
{
    QFile file{filePath};
    if (file.open(QIODevice::ReadOnly))
        return {QString::fromUtf8(file.readAll())};

    return {};
}

#ifdef QDS_HAS_QMLPRIVATE
QString createVersion(const QMultiHash<QString, QQmlDirParser::Component> &components)
{
    auto found = std::max_element(components.begin(), components.end(), [](auto &&first, auto &&second) {
        return first.version < second.version;
    });

    if (found != components.end() && found->version.isValid())
        return QString{"%1.%2"}.arg(found->version.majorVersion()).arg(found->version.minorVersion());

    return {};
}
#endif

} // namespace

void ModuleScanner::scan(const QStringList &modulePaths)
{
    for (const QString &modulePath : modulePaths)
        scan(modulePath.toStdString());
}

void ModuleScanner::scan(std::string_view modulePath)
{
#ifdef QDS_HAS_QMLPRIVATE
    try {
        const std::filesystem::path installDirectoryPath{modulePath};

        auto current = std::filesystem::recursive_directory_iterator{installDirectoryPath};
        auto end = std::filesystem::end(current);

        for (; current != end; ++current) {
            const auto &entry = *current;
            auto path = entry.path();

            if (path.filename() == "qmldir") {
                QQmlDirParser parser;

                auto content = contentAsQString(QString::fromStdU16String(path.u16string()));
                if (!content)
                    continue;

                bool hasError = parser.parse(*content);
                if (hasError)
                    continue;

                auto moduleName = parser.typeNamespace();

                if (moduleName.isEmpty() || m_skip(moduleName))
                    continue;

                m_modules.push_back(
                    Import::createLibraryImport(moduleName, createVersion(parser.components())));
            }
        }
    } catch (const std::filesystem::filesystem_error &) {
        return;
    }
#endif
}

} // namespace QmlDesigner
