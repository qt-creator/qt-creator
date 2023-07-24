// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "examplesparser.h"

#include "qtsupporttr.h"

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/stylehelper.h>

#include <QPixmapCache>
#include <QXmlStreamReader>

using namespace Utils;

namespace QtSupport::Internal {

static FilePath relativeOrInstallPath(const FilePath &path,
                                      const FilePath &manifestPath,
                                      const FilePath &installPath)
{
    const FilePath relativeResolvedPath = manifestPath.resolvePath(path);
    const FilePath installResolvedPath = installPath.resolvePath(path);
    if (relativeResolvedPath.exists())
        return relativeResolvedPath;
    if (installResolvedPath.exists())
        return installResolvedPath;
    // doesn't exist, return the preferred resolved install path
    return installResolvedPath;
}

static QString fixStringForTags(const QString &string)
{
    QString returnString = string;
    returnString.remove(QLatin1String("<i>"));
    returnString.remove(QLatin1String("</i>"));
    returnString.remove(QLatin1String("<tt>"));
    returnString.remove(QLatin1String("</tt>"));
    return returnString;
}

static QStringList trimStringList(const QStringList &stringlist)
{
    return Utils::transform(stringlist, [](const QString &str) { return str.trimmed(); });
}

static QHash<QString, QStringList> parseMeta(QXmlStreamReader *reader)
{
    QHash<QString, QStringList> result;
    while (!reader->atEnd()) {
        switch (reader->readNext()) {
        case QXmlStreamReader::StartElement:
            if (reader->name() == QLatin1String("entry")) {
                const QString key = reader->attributes().value("name").toString();
                if (key.isEmpty()) {
                    reader->raiseError("Tag \"entry\" requires \"name\" attribute");
                    break;
                }
                const QString value = reader->readElementText();
                if (!value.isEmpty())
                    result[key].append(value);
            }
            break;
        case QXmlStreamReader::EndElement:
            if (reader->name() == QLatin1String("meta"))
                return result;
            break;
        default:
            break;
        }
    }
    return result;
}

static QStringList parseCategories(QXmlStreamReader *reader)
{
    QStringList categoryOrder;
    while (!reader->atEnd()) {
        switch (reader->readNext()) {
        case QXmlStreamReader::StartElement:
            if (reader->name() == QLatin1String("category"))
                categoryOrder.append(reader->readElementText());
            break;
        case QXmlStreamReader::EndElement:
            if (reader->name() == QLatin1String("categories"))
                return categoryOrder;
            break;
        default:
            break;
        }
    }
    return categoryOrder;
}

static QList<ExampleItem *> parseExamples(QXmlStreamReader *reader,
                                          const FilePath &projectsOffset,
                                          const FilePath &examplesInstallPath)
{
    QList<ExampleItem *> result;
    std::unique_ptr<ExampleItem> item;
    while (!reader->atEnd()) {
        switch (reader->readNext()) {
        case QXmlStreamReader::StartElement:
            if (reader->name() == QLatin1String("example")) {
                item = std::make_unique<ExampleItem>();
                item->type = Example;
                QXmlStreamAttributes attributes = reader->attributes();
                item->name = attributes.value(QLatin1String("name")).toString();
                item->projectPath = FilePath::fromUserInput(
                    attributes.value(QLatin1String("projectPath")).toString());
                item->hasSourceCode = !item->projectPath.isEmpty();
                item->projectPath = relativeOrInstallPath(item->projectPath,
                                                          projectsOffset,
                                                          examplesInstallPath);
                item->imageUrl = attributes.value(QLatin1String("imageUrl")).toString();
                QPixmapCache::remove(item->imageUrl);
                item->docUrl = attributes.value(QLatin1String("docUrl")).toString();
                item->isHighlighted = attributes.value(QLatin1String("isHighlighted")).toString()
                                      == QLatin1String("true");

            } else if (reader->name() == QLatin1String("fileToOpen")) {
                const QString mainFileAttribute
                    = reader->attributes().value(QLatin1String("mainFile")).toString();
                const FilePath filePath
                    = relativeOrInstallPath(FilePath::fromUserInput(reader->readElementText()),
                                            projectsOffset,
                                            examplesInstallPath);
                item->filesToOpen.append(filePath);
                if (mainFileAttribute.compare(QLatin1String("true"), Qt::CaseInsensitive) == 0)
                    item->mainFile = filePath;
            } else if (reader->name() == QLatin1String("description")) {
                item->description = fixStringForTags(reader->readElementText());
            } else if (reader->name() == QLatin1String("dependency")) {
                item->dependencies.append(projectsOffset / reader->readElementText());
            } else if (reader->name() == QLatin1String("tags")) {
                item->tags = trimStringList(
                    reader->readElementText().split(QLatin1Char(','), Qt::SkipEmptyParts));
            } else if (reader->name() == QLatin1String("platforms")) {
                item->platforms = trimStringList(
                    reader->readElementText().split(QLatin1Char(','), Qt::SkipEmptyParts));
            } else if (reader->name() == QLatin1String("meta")) {
                item->metaData = parseMeta(reader);
            }
            break;
        case QXmlStreamReader::EndElement:
            if (reader->name() == QLatin1String("example")) {
                result.push_back(item.release());
            } else if (reader->name() == QLatin1String("examples")) {
                return result;
            }
            break;
        default: // nothing
            break;
        }
    }
    return result;
}

static QList<ExampleItem *> parseDemos(QXmlStreamReader *reader,
                                       const FilePath &projectsOffset,
                                       const FilePath &demosInstallPath)
{
    QList<ExampleItem *> result;
    std::unique_ptr<ExampleItem> item;
    while (!reader->atEnd()) {
        switch (reader->readNext()) {
        case QXmlStreamReader::StartElement:
            if (reader->name() == QLatin1String("demo")) {
                item = std::make_unique<ExampleItem>();
                item->type = Demo;
                QXmlStreamAttributes attributes = reader->attributes();
                item->name = attributes.value(QLatin1String("name")).toString();
                item->projectPath = FilePath::fromUserInput(
                    attributes.value(QLatin1String("projectPath")).toString());
                item->hasSourceCode = !item->projectPath.isEmpty();
                item->projectPath = relativeOrInstallPath(item->projectPath,
                                                          projectsOffset,
                                                          demosInstallPath);
                item->imageUrl = attributes.value(QLatin1String("imageUrl")).toString();
                QPixmapCache::remove(item->imageUrl);
                item->docUrl = attributes.value(QLatin1String("docUrl")).toString();
                item->isHighlighted = attributes.value(QLatin1String("isHighlighted")).toString()
                                      == QLatin1String("true");
            } else if (reader->name() == QLatin1String("fileToOpen")) {
                item->filesToOpen.append(
                    relativeOrInstallPath(FilePath::fromUserInput(reader->readElementText()),
                                          projectsOffset,
                                          demosInstallPath));
            } else if (reader->name() == QLatin1String("description")) {
                item->description = fixStringForTags(
                    reader->readElementText());
            } else if (reader->name() == QLatin1String("dependency")) {
                item->dependencies.append(
                    projectsOffset
                    / reader->readElementText());
            } else if (reader->name() == QLatin1String("tags")) {
                item->tags = reader->readElementText().split(QLatin1Char(','));
            }
            break;
        case QXmlStreamReader::EndElement:
            if (reader->name() == QLatin1String("demo")) {
                result.push_back(item.release());
            } else if (reader->name() == QLatin1String("demos")) {
                return result;
            }
            break;
        default: // nothing
            break;
        }
    }
    return result;
}

static QList<ExampleItem *> parseTutorials(QXmlStreamReader *reader, const FilePath &projectsOffset)
{
    QList<ExampleItem *> result;
    std::unique_ptr<ExampleItem> item = std::make_unique<ExampleItem>();
    while (!reader->atEnd()) {
        switch (reader->readNext()) {
        case QXmlStreamReader::StartElement:
            if (reader->name() == QLatin1String("tutorial")) {
                item = std::make_unique<ExampleItem>();
                item->type = Tutorial;
                QXmlStreamAttributes attributes = reader->attributes();
                item->name = attributes.value(QLatin1String("name")).toString();
                const QString projectPath = attributes.value(QLatin1String("projectPath")).toString();
                item->projectPath = projectPath.isEmpty() ? FilePath()
                                                          : projectsOffset / projectPath;
                item->hasSourceCode = !projectPath.isEmpty();
                item->imageUrl = Utils::StyleHelper::dpiSpecificImageFile(
                    attributes.value(QLatin1String("imageUrl")).toString());
                QPixmapCache::remove(item->imageUrl);
                item->docUrl = attributes.value(QLatin1String("docUrl")).toString();
                item->isVideo = attributes.value(QLatin1String("isVideo")).toString()
                                == QLatin1String("true");
                item->videoUrl = attributes.value(QLatin1String("videoUrl")).toString();
                item->videoLength = attributes.value(QLatin1String("videoLength")).toString();
            } else if (reader->name() == QLatin1String("fileToOpen")) {
                item->filesToOpen.append(
                    projectsOffset
                    / reader->readElementText());
            } else if (reader->name() == QLatin1String("description")) {
                item->description = fixStringForTags(reader->readElementText());
            } else if (reader->name() == QLatin1String("dependency")) {
                item->dependencies.append(projectsOffset / reader->readElementText());
            } else if (reader->name() == QLatin1String("tags")) {
                item->tags = reader->readElementText().split(QLatin1Char(','));
            } else if (reader->name() == QLatin1String("meta")) {
                item->metaData = parseMeta(reader);
            }
            break;
        case QXmlStreamReader::EndElement:
            if (reader->name() == QLatin1String("tutorial"))
                result.push_back(item.release());
            else if (reader->name() == QLatin1String("tutorials"))
                return result;
            break;
        default: // nothing
            break;
        }
    }
    return result;
}

expected_str<ParsedExamples> parseExamples(const FilePath &manifest,
                                           const FilePath &examplesInstallPath,
                                           const FilePath &demosInstallPath,
                                           const bool examples)
{
    const expected_str<QByteArray> contents = manifest.fileContents();
    if (!contents)
        return make_unexpected(contents.error());

    return parseExamples(*contents, manifest, examplesInstallPath, demosInstallPath, examples);
}

expected_str<ParsedExamples> parseExamples(const QByteArray &manifestData,
                                           const Utils::FilePath &manifestPath,
                                           const FilePath &examplesInstallPath,
                                           const FilePath &demosInstallPath,
                                           const bool examples)
{
    const FilePath path = manifestPath.parentDir();
    QStringList categoryOrder;
    QList<ExampleItem *> items;
    QXmlStreamReader reader(manifestData);
    while (!reader.atEnd()) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement:
            if (categoryOrder.isEmpty() && reader.name() == QLatin1String("categories"))
                categoryOrder = parseCategories(&reader);
            else if (examples && reader.name() == QLatin1String("examples"))
                items += parseExamples(&reader, path, examplesInstallPath);
            else if (examples && reader.name() == QLatin1String("demos"))
                items += parseDemos(&reader, path, demosInstallPath);
            else if (!examples && reader.name() == QLatin1String("tutorials"))
                items += parseTutorials(&reader, path);
            break;
        default: // nothing
            break;
        }
    }

    if (reader.hasError()) {
        qDeleteAll(items);
        return make_unexpected(QString("Could not parse file \"%1\" as XML document: %2:%3: %4")
                                   .arg(manifestPath.toUserOutput())
                                   .arg(reader.lineNumber())
                                   .arg(reader.columnNumber())
                                   .arg(reader.errorString()));
    }
    return {{items, categoryOrder}};
}

static bool sortByHighlightedAndName(ExampleItem *first, ExampleItem *second)
{
    if (first->isHighlighted && !second->isHighlighted)
        return true;
    if (!first->isHighlighted && second->isHighlighted)
        return false;
    return first->name.compare(second->name, Qt::CaseInsensitive) < 0;
}

QList<std::pair<Core::Section, QList<ExampleItem *>>> getCategories(const QList<ExampleItem *> &items,
                                                                    bool sortIntoCategories,
                                                                    const QStringList &defaultOrder,
                                                                    bool restrictRows)
{
    static const QString otherDisplayName = Tr::tr("Other", "Category for all other examples");
    const bool useCategories = sortIntoCategories
                               || qtcEnvironmentVariableIsSet("QTC_USE_EXAMPLE_CATEGORIES");
    QList<ExampleItem *> other;
    QMap<QString, QList<ExampleItem *>> categoryMap;
    if (useCategories) {
        // Append copies of the items and delete the original ones,
        // because items might be added to multiple categories and that needs individual items
        for (ExampleItem *item : items) {
            const QStringList itemCategories = Utils::filteredUnique(
                item->metaData.value("category"));
            for (const QString &category : itemCategories)
                categoryMap[category].append(new ExampleItem(*item));
            if (itemCategories.isEmpty())
                other.append(new ExampleItem(*item));
        }
    }
    QList<std::pair<Core::Section, QList<ExampleItem *>>> categories;
    if (categoryMap.isEmpty()) {
        // If we tried sorting into categories, but none were defined, we copied the items
        // into "other", which we don't use here. Get rid of them again.
        qDeleteAll(other);
        // The example set doesn't define categories. Consider the "highlighted" ones as "featured"
        QList<ExampleItem *> featured;
        QList<ExampleItem *> allOther;
        std::tie(featured, allOther) = Utils::partition(items, [](ExampleItem *i) {
            return i->isHighlighted;
        });
        if (!featured.isEmpty()) {
            categories.append(
                {{Tr::tr("Featured", "Category for highlighted examples"), 0}, featured});
        }
        if (!allOther.isEmpty())
            categories.append({{otherDisplayName, 1}, allOther});
    } else {
        // All original items have been copied into a category or other, delete.
        qDeleteAll(items);
        const int defaultOrderSize = defaultOrder.size();
        int index = 0;
        const auto end = categoryMap.constKeyValueEnd();
        for (auto it = categoryMap.constKeyValueBegin(); it != end; ++it) {
            // order "known" categories as wanted, others come afterwards
            const int defaultIndex = defaultOrder.indexOf(it->first);
            const int priority = defaultIndex >= 0 ? defaultIndex : (index + defaultOrderSize);
            const std::optional<int> maxRows = restrictRows
                                                   ? std::make_optional<int>(index == 0 ? 2 : 1)
                                                   : std::nullopt;
            categories.append({{it->first, priority, maxRows}, it->second});
            ++index;
        }
        if (!other.isEmpty())
            categories.append({{otherDisplayName, index + defaultOrderSize, /*maxRows=*/1}, other});
    }

    const auto end = categories.end();
    for (auto it = categories.begin(); it != end; ++it)
        sort(it->second, sortByHighlightedAndName);
    return categories;
}

} // namespace QtSupport::Internal
