// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "plugingenerator.h"
#include "pluginoptions.h"
#include "../qmakeprojectmanagertr.h"

#include <coreplugin/generatedfile.h>

#include <cppeditor/abstracteditorsupport.h>

#include <projectexplorer/projecttree.h>

#include <utils/algorithm.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>
#include <utils/templateengine.h>

#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>

using namespace Utils;

namespace QmakeProjectManager::Internal {

static QString headerGuard(const QString &header)
{
    static const QRegularExpression regexp("[^A-Z0-9]+");
    return header.toUpper().replace(regexp, QString("_"));
}

struct ProjectContents {
    QString tmpl;
    QString library;
    QString headers;
    QString sources;
};

// Create a binary icon file
static Result<Core::GeneratedFile> generateIconFile(const FilePath &sourcePath,
                                                    const FilePath &targetPath)
{
    // Read out source
    const Result<QByteArray> res = sourcePath.fileContents();
    if (!res)
        return ResultError(res.error());

    Core::GeneratedFile rc(targetPath);
    rc.setBinaryContents(*res);
    rc.setBinary(true);
    return rc;
}

static QString qt5PluginMetaData(const QString &interfaceName)
{
    return QLatin1String("    Q_PLUGIN_METADATA(IID \"org.qt-project.Qt.")
        + interfaceName + QLatin1String("\")");
}

using SubstitutionMap = QMap<QString, QString>;

static Result<QString> processTemplate(const QString &tmpl, const SubstitutionMap &substMap)
{
    const Result<QByteArray> res = FilePath::fromString(tmpl).fileContents();
    if (!res)
        return ResultError(res.error());

    QString cont = QString::fromUtf8(*res);

    // Expander needed to handle extra variable "Cpp:PragmaOnce"
    MacroExpander *expander = Utils::globalMacroExpander();
    const Result<QString> processed = TemplateEngine::processText(expander, cont);
    if (!processed)
        return ResultError(QString("Error processing custom plugin file: %1\nFile:\n%2")
                               .arg(processed.error(), cont));

    cont = processed.value_or(QString());

    const QChar atChar = QLatin1Char('@');
    int offset = 0;
    for (;;) {
        const int start = cont.indexOf(atChar, offset);
        if (start < 0)
            break;
        const int end = cont.indexOf(atChar, start + 1);
        QTC_ASSERT(end, continue);
        const QString keyword = cont.mid(start + 1, end - start - 1);
        const QString replacement = substMap.value(keyword);
        cont.replace(start, end - start + 1, replacement);
        offset = start + replacement.size();
    }
    return cont;
}

static QString cStringQuote(QString s)
{
    s.replace(QLatin1Char('\\'), QLatin1String("\\\\"));
    s.replace(QLatin1Char('"'), QLatin1String("\\\""));
    s.replace(QLatin1Char('\t'), QLatin1String("\\t"));
    s.replace(QLatin1Char('\n'), QLatin1String("\\n"));
    return s;
}

Result<Core::GeneratedFiles> generatePlugin(
    const GenerationParameters &p, const PluginOptions &options)
{
    const QChar slash = QLatin1Char('/');
    const QChar blank = QLatin1Char(' ');
    QList<Core::GeneratedFile> rc;

    QString baseDir_ = p.path;
    baseDir_ += slash;
    baseDir_ += p.fileName;
    const QString slashLessBaseDir = baseDir_;
    const FilePath baseDir = FilePath::fromString(baseDir_);

    QSet<QString> widgetLibraries;
    QSet<QString> widgetProjects;
    QMap<QString,ProjectContents> widgetProjectContents;
    QString pluginIncludes;
    QString pluginAdditions;
    QString pluginHeaders;
    QString pluginSources;
    QSet<QString> pluginIcons;

    SubstitutionMap sm;

    // First create the widget wrappers (plugins) and - if requested - skeletons
    // for the widgets.
    const int widgetCount = options.widgetOptions.size();
    ProjectExplorer::Project *project = ProjectExplorer::ProjectTree::currentProject();
    for (int i = 0; i < widgetCount; i++) {
        const PluginOptions::WidgetOptions &wo = options.widgetOptions.at(i);
        sm.clear();
        sm.insert(QLatin1String("SINGLE_INCLUDE_GUARD"), headerGuard(wo.pluginHeaderFile));
        sm.insert(QLatin1String("PLUGIN_CLASS"), wo.pluginClassName);
        sm.insert(QLatin1String("SINGLE_PLUGIN_METADATA"),
            options.widgetOptions.count() == 1 ?
                qt5PluginMetaData(QLatin1String("QDesignerCustomWidgetInterface")) : QString());
        const Result<QString> pluginHeaderContents = processTemplate(p.templatePath + QLatin1String("/tpl_single.h"), sm);
        if (!pluginHeaderContents)
            return ResultError(pluginHeaderContents.error());
        Core::GeneratedFile pluginHeader(baseDir / wo.pluginHeaderFile);
        pluginHeader.setContents(CppEditor::licenseTemplate(
                                     project, FilePath::fromString(wo.pluginHeaderFile),
                                     wo.pluginClassName)
                                 + *pluginHeaderContents);
        rc.push_back(pluginHeader);

        sm.remove(QLatin1String("SINGLE_INCLUDE_GUARD"));
        sm.insert(QLatin1String("PLUGIN_HEADER"), wo.pluginHeaderFile);
        sm.insert(QLatin1String("WIDGET_CLASS"), wo.widgetClassName);
        sm.insert(QLatin1String("WIDGET_HEADER"), wo.widgetHeaderFile);
        sm.insert(QLatin1String("WIDGET_GROUP"), wo.group);
        QString iconResource;
        if (!wo.iconFile.isEmpty()) {
            iconResource = QLatin1String("QLatin1String(\":/");
            iconResource += Utils::FilePath::fromString(wo.iconFile).fileName();
            iconResource += QLatin1String("\")");
        }
        sm.insert(QLatin1String("WIDGET_ICON"),iconResource);
        sm.insert(QLatin1String("WIDGET_TOOLTIP"), cStringQuote(wo.toolTip));
        sm.insert(QLatin1String("WIDGET_WHATSTHIS"), cStringQuote(wo.whatsThis));
        sm.insert(QLatin1String("WIDGET_ISCONTAINER"), wo.isContainer ? QLatin1String("true") : QLatin1String("false"));
        sm.insert(QLatin1String("WIDGET_DOMXML"), QLatin1String("R\"(")
                                + wo.domXml.trimmed() + QLatin1String(")\""));

        const Result<QString> pluginSourceContents = processTemplate(p.templatePath + QLatin1String("/tpl_single.cpp"), sm);
        if (!pluginSourceContents)
            return {};
        Core::GeneratedFile pluginSource(baseDir / wo.pluginSourceFile);
        pluginSource.setContents(CppEditor::licenseTemplate(
                                     project,
                                     FilePath::fromString(wo.pluginSourceFile), wo.pluginClassName)
                                 + *pluginSourceContents);
        if (i == 0 && widgetCount == 1) // Open first widget unless collection
            pluginSource.setAttributes(Core::GeneratedFile::OpenEditorAttribute);
        rc.push_back(pluginSource);

        if (wo.sourceType == PluginOptions::WidgetOptions::LinkLibrary)
            widgetLibraries.insert(QLatin1String("-l") + wo.widgetLibrary);
        else
            widgetProjects.insert(QLatin1String("include(") + wo.widgetProjectFile + QLatin1Char(')'));
        pluginIncludes += QLatin1String("#include \"") + wo.pluginHeaderFile + QLatin1String("\"\n");
        pluginAdditions +=
            QLatin1String("    m_widgets.append(new ") + wo.pluginClassName + QLatin1String("(this));\n");
        pluginHeaders += QLatin1Char(' ') + wo.pluginHeaderFile;
        pluginSources += QLatin1Char(' ') + wo.pluginSourceFile;
        if (!wo.iconFile.isEmpty())
            pluginIcons.insert(wo.iconFile);

        if (wo.createSkeleton) {
            ProjectContents &pc = widgetProjectContents[wo.widgetProjectFile];
            if (pc.headers.isEmpty()) {
                if (wo.sourceType == PluginOptions::WidgetOptions::LinkLibrary) {
                    pc.library = wo.widgetLibrary;
                    pc.tmpl = p.templatePath + QLatin1String("/tpl_widget_lib.pro");
                } else {
                    pc.tmpl = p.templatePath + QLatin1String("/tpl_widget_include.pri");
                }
                widgetProjectContents.insert(wo.widgetProjectFile, pc);
            } else {
                if (pc.library != wo.widgetLibrary) {
                    return ResultError(Tr::tr("Creating multiple widget libraries (%1, %2) in one project (%3) is not supported.")
                                           .arg(pc.library, wo.widgetLibrary, wo.widgetProjectFile));
                }
            }
            pc.headers += blank + wo.widgetHeaderFile;
            pc.sources += blank + wo.widgetSourceFile;

            sm.clear();
            sm.insert(QLatin1String("WIDGET_INCLUDE_GUARD"), headerGuard(wo.widgetHeaderFile));
            sm.insert(QLatin1String("WIDGET_BASE_CLASS"), wo.widgetBaseClassName);
            sm.insert(QLatin1String("WIDGET_CLASS"), wo.widgetClassName);
            const Result<QString> widgetHeaderContents = processTemplate(p.templatePath + QLatin1String("/tpl_widget.h"), sm);
            if (!widgetHeaderContents)
                return ResultError(widgetHeaderContents.error());
            Core::GeneratedFile widgetHeader(baseDir / wo.widgetHeaderFile);
            widgetHeader.setContents(CppEditor::licenseTemplate(
                                         project,
                                         FilePath::fromString(wo.widgetHeaderFile),
                                         wo.widgetClassName)
                                     + *widgetHeaderContents);
            rc.push_back(widgetHeader);

            sm.remove(QLatin1String("WIDGET_INCLUDE_GUARD"));
            sm.insert(QLatin1String("WIDGET_HEADER"), wo.widgetHeaderFile);
            const Result<QString> widgetSourceContents = processTemplate(p.templatePath + QLatin1String("/tpl_widget.cpp"), sm);
            if (!widgetSourceContents)
                return ResultError(widgetSourceContents.error());
            Core::GeneratedFile widgetSource(baseDir / wo.widgetSourceFile);
            widgetSource.setContents(CppEditor::licenseTemplate(
                                         project,
                                         FilePath::fromString(wo.widgetSourceFile),
                                         wo.widgetClassName)
                                     + *widgetSourceContents);
            rc.push_back(widgetSource);
        }
    }

    // Then create the project files for the widget skeletons.
    // These might create widgetLibraries or be included into the plugin's project.
    QMap<QString,ProjectContents>::const_iterator it = widgetProjectContents.constBegin();
    const QMap<QString,ProjectContents>::const_iterator end = widgetProjectContents.constEnd();
    for (; it != end; ++it) {
        const ProjectContents &pc = it.value();
        sm.clear();
        sm.insert(QLatin1String("WIDGET_HEADERS"), pc.headers);
        sm.insert(QLatin1String("WIDGET_SOURCES"), pc.sources);
        if (!pc.library.isEmpty())
            sm.insert(QLatin1String("WIDGET_LIBRARY"), pc.library);
        const Result<QString> widgetPriContents = processTemplate(pc.tmpl, sm);
        if (!widgetPriContents)
            return ResultError(widgetPriContents.error());
        Core::GeneratedFile widgetPri(baseDir / it.key());
        widgetPri.setContents(*widgetPriContents);
        rc.push_back(widgetPri);
    }

    // Create the sources for the collection if necessary.
    if (widgetCount > 1) {
        sm.clear();
        sm.insert(QLatin1String("COLLECTION_INCLUDE_GUARD"), headerGuard(options.collectionHeaderFile));
        sm.insert(QLatin1String("COLLECTION_PLUGIN_CLASS"), options.collectionClassName);
        sm.insert(QLatin1String("COLLECTION_PLUGIN_METADATA"), qt5PluginMetaData(QLatin1String("QDesignerCustomWidgetCollectionInterface")));
        const Result<QString> collectionHeaderContents = processTemplate(p.templatePath + QLatin1String("/tpl_collection.h"), sm);
        if (!collectionHeaderContents)
            return ResultError(collectionHeaderContents.error());
        Core::GeneratedFile collectionHeader(baseDir / options.collectionHeaderFile);
        collectionHeader.setContents(CppEditor::licenseTemplate(
                                         project,
                                         FilePath::fromString(options.collectionHeaderFile),
                                         options.collectionClassName)
                                     + *collectionHeaderContents);
        rc.push_back(collectionHeader);

        sm.remove(QLatin1String("COLLECTION_INCLUDE_GUARD"));
        sm.insert(QLatin1String("PLUGIN_INCLUDES"),
            pluginIncludes +
            QLatin1String("#include \"") +
                options.collectionHeaderFile +
                QLatin1String("\""));
        sm.insert(QLatin1String("PLUGIN_ADDITIONS"), pluginAdditions);
        const Result<QString> collectionSourceFileContents = processTemplate(p.templatePath + QLatin1String("/tpl_collection.cpp"), sm);
        if (!collectionSourceFileContents)
            return ResultError(collectionSourceFileContents.error());
        Core::GeneratedFile collectionSource(baseDir / options.collectionSourceFile);
        collectionSource.setContents(CppEditor::licenseTemplate(
                                         project,
                                         FilePath::fromString(options.collectionSourceFile),
                                         options.collectionClassName)
                                     + *collectionSourceFileContents);
        collectionSource.setAttributes(Core::GeneratedFile::OpenEditorAttribute);
        rc.push_back(collectionSource);

        pluginHeaders += blank + options.collectionHeaderFile;
        pluginSources += blank + options.collectionSourceFile;
    }

    // Copy icons that are not in the plugin source base directory yet (that is,
    // probably all), add them to the resource file
    QString iconFiles;
    for (QString icon : std::as_const(pluginIcons)) {
        const QFileInfo qfi(icon);
        if (qfi.dir() != slashLessBaseDir) {
            const FilePath newIcon = baseDir / qfi.fileName();
            const Result<Core::GeneratedFile> iconFile =
                generateIconFile(FilePath::fromFileInfo(qfi), newIcon);
            if (!iconFile)
                return ResultError(iconFile.error());

            rc.push_back(*iconFile);
            icon = qfi.fileName();
        }
        iconFiles += QLatin1String("        <file>") + icon + QLatin1String("</file>\n");
    }
    // Create the resource file with the icons.
    sm.clear();
    sm.insert(QLatin1String("ICON_FILES"), iconFiles);
    const Result<QString> resourceFileContents = processTemplate(p.templatePath + QLatin1String("/tpl_resources.qrc"), sm);
    if (!resourceFileContents)
        return ResultError(resourceFileContents.error());
    Core::GeneratedFile resourceFile(baseDir / options.resourceFile);
    resourceFile.setContents(*resourceFileContents);
    rc.push_back(resourceFile);

    // Finally create the project for the plugin itself.
    sm.clear();
    sm.insert(QLatin1String("PLUGIN_NAME"), options.pluginName);
    sm.insert(QLatin1String("PLUGIN_HEADERS"), pluginHeaders);
    sm.insert(QLatin1String("PLUGIN_SOURCES"), pluginSources);
    sm.insert(QLatin1String("PLUGIN_RESOURCES"), options.resourceFile);
    sm.insert(QLatin1String("WIDGET_LIBS"), QStringList(Utils::toList(widgetLibraries)).join(blank));
    sm.insert(QLatin1String("INCLUSIONS"), QStringList(Utils::toList(widgetProjects)).join(QLatin1Char('\n')));
    const Result<QString> proFileContents = processTemplate(p.templatePath + QLatin1String("/tpl_plugin.pro"), sm);
    if (!proFileContents)
        return {};
    Core::GeneratedFile proFile(baseDir.pathAppended(p.fileName + ".pro"));
    proFile.setContents(*proFileContents);
    proFile.setAttributes(Core::GeneratedFile::OpenProjectAttribute);
    rc.push_back(proFile);
    return rc;
}

} // QmakeProjectManager::Internal
