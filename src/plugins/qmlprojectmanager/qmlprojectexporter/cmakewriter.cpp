// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "cmakewriter.h"
#include "cmakegenerator.h"
#include "cmakewriterv0.h"
#include "cmakewriterv1.h"

#include "qmlprojectmanager/buildsystem/qmlbuildsystem.h"
#include "qmlprojectmanager/qmlproject.h"
#include "qmlprojectmanager/qmlprojectmanagertr.h"

#include "utils/namevalueitem.h"

#include <QFile>
#include <QTextStream>

namespace QmlProjectManager {

namespace QmlProjectExporter {

const char TEMPLATE_RESOURCES[] = R"(
qt6_add_resources(%1 %2
    PREFIX "%3"
    VERSION 1.0
    FILES %4
))";

const char TEMPLATE_BIG_RESOURCES[] = R"(
qt6_add_resources(%1 %2
    BIG_RESOURCES
    PREFIX "%3"
    VERSION 1.0
    FILES %4
)
)";

CMakeWriter::Ptr CMakeWriter::create(CMakeGenerator *parent)
{
    const QmlProject *project = parent->qmlProject();
    QTC_ASSERT(project, return {});

    const QmlBuildSystem *buildSystem = parent->buildSystem();
    QTC_ASSERT(buildSystem, return {});

    auto [major, minor, patch] = versionFromString(buildSystem->versionDesignStudio());

    bool useV1 = false;
    if (major.has_value())
        useV1 = minor.has_value() ? *major >= 4 && *minor >= 5 : *major >= 5;

    if (useV1)
        return std::make_unique<CMakeWriterV1>(parent);

    CMakeGenerator::logIssue(
        ProjectExplorer::Task::Warning,
        Tr::tr(
            "The project was created with a Qt Design Studio version earlier than Qt Design Studio "
            "4.5. Due to limitations of the project structure in earlier Qt Design Studio "
            "versions, "
            "the resulting application might not display all the assets. Referring to "
            "assets between different QML modules does not work in the compiled application.<br>"
            "<a "
            "href=\"https://doc.qt.io/qtdesignstudio/studio-designer-developer-workflow.html\">See "
            "the documentation for details.</a>"),
        buildSystem->projectFilePath());

    return std::make_unique<CMakeWriterV0>(parent);
}

CMakeWriter::Version CMakeWriter::versionFromString(const QString &versionString)
{
    const QStringList versions = versionString.split('.', Qt::SkipEmptyParts);
    auto checkComponent = [&versions](qsizetype idx) -> std::optional<int> {
        if (versions.size() >= idx+1) {
            bool ok = false;
            if (int version = versions[idx].toInt(&ok); ok)
                return version;
        }
        return std::nullopt;
    };

    return {checkComponent(0), checkComponent(1), checkComponent(2)};
}

CMakeWriter::Version CMakeWriter::versionFromIgnoreFile(const Utils::FilePath &path)
{
    QString versionString;
    QFile ignoreFile(path.toFSPathString());
    if (ignoreFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&ignoreFile);
        QString firstLine = stream.readLine();
        ignoreFile.close();

        QStringList parts = firstLine.split(' ');
        QTC_ASSERT(parts.size()==3, return {});
        return versionFromString(parts[2]);
    }
    return {};
}

QString CMakeWriter::readTemplate(const QString &templatePath)
{
    QFile templatefile(templatePath);
    if (!templatefile.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    QTextStream stream(&templatefile);
    QString content = stream.readAll();
    templatefile.close();
    return content;
}

void CMakeWriter::writeFile(const Utils::FilePath &path, const QString &content)
{
    QFile fileHandle(path.toUrlishString());
    if (fileHandle.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QString cpy = content;
        cpy.replace("\t", "    ");

        QTextStream stream(&fileHandle);
        stream << cpy;
    } else {
        QString text("Failed to write");
        CMakeGenerator::logIssue(ProjectExplorer::Task::Error, text, path);
    }
    fileHandle.close();
}

CMakeWriter::CMakeWriter(CMakeGenerator *parent)
    : m_parent(parent)
{}

const CMakeGenerator *CMakeWriter::parent() const
{
    return m_parent;
}

bool CMakeWriter::isPlugin(const NodePtr &node) const
{
    if (node->type == Node::Type::Module)
        return true;
    return false;
}

QString CMakeWriter::sourceDirName() const
{
    return "src";
}

void CMakeWriter::transformNode(NodePtr &) const
{}

std::vector<Utils::FilePath> CMakeWriter::files(const NodePtr &node, const FileGetter &getter) const
{
    std::vector<Utils::FilePath> out = getter(node);
    for (const NodePtr &child : node->subdirs) {
        if (child->type == Node::Type::Module)
            continue;

        auto childFiles = files(child, getter);
        out.insert(out.end(), childFiles.begin(), childFiles.end());
    }
    return out;
}

std::vector<Utils::FilePath> CMakeWriter::qmlFiles(const NodePtr &node) const
{
    return files(node, [](const NodePtr &n) { return n->files; });
}

std::vector<Utils::FilePath> CMakeWriter::singletons(const NodePtr &node) const
{
    return files(node, [](const NodePtr &n) { return n->singletons; });
}

std::vector<Utils::FilePath> CMakeWriter::assets(const NodePtr &node) const
{
    return files(node, [](const NodePtr &n) { return n->assets; });
}

std::vector<Utils::FilePath> CMakeWriter::sources(const NodePtr &node) const
{
    return files(node, [](const NodePtr &n) { return n->sources; });
}

std::vector<QString> CMakeWriter::plugins(const NodePtr &node) const
{
    QTC_ASSERT(parent(), return {});
    std::vector<QString> out;
    collectPlugins(node, out);
    return out;
}

QString CMakeWriter::getEnvironmentVariable(const QString &key) const
{
    QTC_ASSERT(parent(), return {});

    QString value;
    if (m_parent->buildSystem()) {
        auto envItems = m_parent->buildSystem()->environment();
        auto confEnv = std::find_if(
            envItems.begin(), envItems.end(), [key](const Utils::EnvironmentItem &item) {
                return item.name == key;
            });
        if (confEnv != envItems.end())
            value = confEnv->value;
    }
    return value;
}

QString CMakeWriter::makeFindPackageBlock(const NodePtr &node, const QmlBuildSystem *buildSystem) const
{
    QString head = "find_package(Qt" + buildSystem->versionQt();
    QString tail = " REQUIRED COMPONENTS Core Gui Widgets Qml Quick QuickTimeline ShaderTools";

    if (hasMesh(node) || hasQuick3dImport(buildSystem->mainUiFilePath()))
        tail.append(" Quick3D");
    tail.append(")\n");

    auto [major, minor, patch] = versionFromString(buildSystem->versionQtQuick());
    if (!major.has_value() || !minor.has_value())
        return head + tail;

    const QString from = QString::number(*major) + "." + QString::number(*minor);
    QString out = head + " " + from + tail;

    if (major >= 6 && minor >= 3)
        out += "qt_standard_project_setup()\n";

    return out;
}

QString CMakeWriter::makeRelative(const NodePtr &node, const Utils::FilePath &path) const
{
    return "\"" + path.relativePathFromDir(node->dir).path() + "\"";
}

QString CMakeWriter::makeQmlFilesBlock(const NodePtr &node) const
{
    QTC_ASSERT(parent(), return {});

    QString qmlFileContent;
    for (const Utils::FilePath &path : qmlFiles(node))
        qmlFileContent.append(QString("\t\t%1\n").arg(makeRelative(node, path)));

    QString str;
    if (!qmlFileContent.isEmpty())
        str.append(QString("\tQML_FILES\n%1").arg(qmlFileContent));

    return str;
}

QString CMakeWriter::makeSingletonBlock(const NodePtr &node) const
{
    QString str;
    const QString setProperties("set_source_files_properties(%1\n\tPROPERTIES\n\t\t%2 %3\n)\n\n");
    for (const Utils::FilePath &path : node->singletons)
        str.append(setProperties.arg(path.fileName()).arg("QT_QML_SINGLETON_TYPE").arg("true"));
    return str;
}

QString CMakeWriter::makeSubdirectoriesBlock(const NodePtr &node, const QStringList &others) const
{
    QTC_ASSERT(parent(), return {});

    QString str;
    for (const NodePtr &n : node->subdirs) {
        if (n->type == Node::Type::Module || n->type == Node::Type::Library
            || n->type == Node::Type::App || parent()->hasChildModule(n))
            str.append(QString("add_subdirectory(%1)\n").arg(n->dir.fileName()));
    }

    for (const QString &other : others)
        str.append(QString("add_subdirectory(%1)\n").arg(other));

    return str;
}

QString CMakeWriter::makeSetEnvironmentFn() const
{
    QTC_ASSERT(parent(), return {});
    QTC_ASSERT(parent()->buildSystem(), return {});

    const QmlBuildSystem *buildSystem = parent()->buildSystem();
    const QString configFile = getEnvironmentVariable(ENV_VARIABLE_CONTROLCONF);

    QString out("inline void set_qt_environment() {\n");
    for (Utils::EnvironmentItem &envItem : buildSystem->environment()) {
        QString key = envItem.name;
        QString value = envItem.value;
        if (value == configFile)
            value.prepend(":/");
        out.append(QString("\tqputenv(\"%1\", \"%2\");\n").arg(key).arg(value));
    }
    out.append("}");

    return out;
}

std::tuple<QString, QString> CMakeWriter::makeResourcesBlocksRoot(const NodePtr &node) const
{
    QString resourcesOut;
    QString bigResourcesOut;

    QStringList res;
    QStringList bigRes;
    collectResources(node, res, bigRes);

    if (!res.isEmpty()) {
        QString resourceContent;
        for (const QString &r : res)
            resourceContent.append(QString("\n\t\t%1").arg(r));

        const QString resourceName = node->name + "Resource";
        resourcesOut = QString::fromUtf8(TEMPLATE_RESOURCES, -1)
            .arg("${CMAKE_PROJECT_NAME}", resourceName, "/qt/qml", resourceContent);
    }

    if (!bigRes.isEmpty()) {
        QString bigResourceContent;
        for (const QString &r : bigRes)
            bigResourceContent.append(QString("\n\t\t%1").arg(r));

        const QString resourceName = node->name + "BigResource";
        bigResourcesOut = QString::fromUtf8(TEMPLATE_BIG_RESOURCES, -1)
            .arg("${CMAKE_PROJECT_NAME}", resourceName, "/qt/qml", bigResourceContent);
    }

    return {resourcesOut, bigResourcesOut};
}

std::tuple<QString, QString> CMakeWriter::makeResourcesBlocksModule(const NodePtr &node) const
{
    QString resourcesOut;
    QString bigResourcesOut;

    QStringList res;
    QStringList bigRes;
    collectResources(node, res, bigRes);

    if (!res.isEmpty()) {
        resourcesOut = "\tRESOURCES\n";
        for (const QString &r : res)
            resourcesOut.append(QString("\t\t%1\n").arg(r));
    }

    if (!bigRes.isEmpty()) {
        QString resourceContent;
        for (const QString &res : bigRes)
            resourceContent.append(QString("\n\t\t%1").arg(res));

        const QString prefixPath = QString(node->uri).replace('.', '/');
        const QString prefix = "/qt/qml/" + prefixPath;
        const QString resourceName = node->name + "BigResource";

        bigResourcesOut = QString::fromUtf8(TEMPLATE_BIG_RESOURCES, -1)
            .arg(node->name, resourceName, prefix, resourceContent);
    }

    return {resourcesOut, bigResourcesOut};
}

void CMakeWriter::collectPlugins(const NodePtr &node, std::vector<QString> &out) const
{
    if (isPlugin(node))
        out.push_back(node->name);
    for (const auto &child : node->subdirs)
        collectPlugins(child, out);
}

void CMakeWriter::collectResources(const NodePtr &node, QStringList &res, QStringList &bigRes) const
{
    for (const Utils::FilePath &path : assets(node)) {
        if (path.fileSize() > 5000000) {
            bigRes.push_back(makeRelative(node, path));
        } else {
            res.append(makeRelative(node, path));
        }
    }
}

bool CMakeWriter::hasMesh(const NodePtr &node) const
{
    for (const auto &path : node->assets) {
        if (path.suffix()=="mesh")
            return true;
    }

    for (const auto &child : node->subdirs) {
        if (hasMesh(child))
            return true;
    }

    return false;
}

bool CMakeWriter::hasQuick3dImport(const Utils::FilePath &filePath) const
{
    QFile f(filePath.toUrlishString());
    if (!f.open(QIODevice::ReadOnly))
        return false;

    QTextStream stream(&f);
    while (!stream.atEnd()) {
        QString line = stream.readLine();
        if (line.contains("{"))
            break;
        if (line.contains("import") && line.contains("QtQuick3D"))
            return true;
    }
    return false;
}

} // End namespace QmlProjectExporter.

} // End namespace QmlProjectManager.
