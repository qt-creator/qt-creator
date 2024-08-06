// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakegenerator.h"
#include "filetypes.h"

#include "qmlprojectmanager/qmlproject.h"
#include "qmlprojectmanager/qmlprojectconstants.h"
#include "qmlprojectmanager/qmlprojectmanagertr.h"

#include "projectexplorer/projectmanager.h"
#include "projectexplorer/projectnodes.h"
#include "projectexplorer/taskhub.h"

#include "utils/filenamevalidatinglineedit.h"

#include "coreplugin/actionmanager/actionmanager.h"
#include "coreplugin/actionmanager/actioncontainer.h"

#include <QDirIterator>
#include <QFileInfo>
#include <QMenu>
#include <QRegularExpression>

#include <set>

namespace QmlProjectManager {

namespace GenerateCmake {

void CMakeGenerator::createMenuAction(QObject *parent)
{
    Core::ActionContainer *fileMenu = Core::ActionManager::actionContainer(
        Core::Constants::M_FILE);
    Core::ActionContainer *exportMenu = Core::ActionManager::createMenu(
        QmlProjectManager::Constants::EXPORT_MENU);

    exportMenu->menu()->setTitle(Tr::tr("Export Project"));
    exportMenu->appendGroup(QmlProjectManager::Constants::G_EXPORT_GENERATE);
    fileMenu->addMenu(exportMenu, Core::Constants::G_FILE_EXPORT);

    auto action = new QAction(Tr::tr("Enable Automatic CMake Generation"), parent);
    action->setEnabled(false);
    action->setCheckable(true);

    Core::Command *cmd = Core::ActionManager::registerAction(action, "QmlProject.EnableCMakeGeneration");
    exportMenu->addAction(cmd, QmlProjectManager::Constants::G_EXPORT_GENERATE);

    QObject::connect(
        ProjectExplorer::ProjectManager::instance(),
        &ProjectExplorer::ProjectManager::startupProjectChanged,
        [action]() {
            if (auto buildSystem = QmlBuildSystem::getStartupBuildSystem()) {
                action->setEnabled(!buildSystem->qtForMCUs());
                action->setChecked(buildSystem->enableCMakeGeneration());
            }
        }
    );

    QObject::connect(action, &QAction::toggled, [](bool checked) {
        if (auto buildSystem = QmlBuildSystem::getStartupBuildSystem())
            buildSystem->setEnableCMakeGeneration(checked);
    });
}

void CMakeGenerator::logIssue(ProjectExplorer::Task::TaskType type, const QString &text, const Utils::FilePath &file)
{
    ProjectExplorer::BuildSystemTask task(type, text, file);
    ProjectExplorer::TaskHub::addTask(task);
    ProjectExplorer::TaskHub::requestPopup();
}

void CMakeGenerator::updateMenuAction()
{
    QTC_ASSERT(buildSystem(), return);

    Core::Command *cmd = Core::ActionManager::command("QmlProject.EnableCMakeGeneration");
    if (!cmd)
        return;

    QAction *action = cmd->action();
    if (!action)
        return;

    bool enabled = buildSystem()->enableCMakeGeneration();
    if (enabled != action->isChecked())
        action->setChecked(enabled);
}

CMakeGenerator::CMakeGenerator(QmlBuildSystem *bs, QObject *parent)
    : QObject(parent)
    , m_buildSystem(bs)
    , m_root(std::make_shared<Node>())
{}

const QmlProject *CMakeGenerator::qmlProject() const
{
    if (m_buildSystem)
        return m_buildSystem->qmlProject();
    return nullptr;
}

const QmlBuildSystem *CMakeGenerator::buildSystem() const
{
    return m_buildSystem;
}

bool CMakeGenerator::findFile(const Utils::FilePath& file) const
{
    return findFile(m_root, file);
}

bool CMakeGenerator::isRootNode(const NodePtr &node) const
{
    return node->name == "Main";
}

bool CMakeGenerator::hasChildModule(const NodePtr &node) const
{
    for (const NodePtr &child : node->subdirs) {
        if (child->type == Node::Type::Module)
            return true;
        if (hasChildModule(child))
            return true;
    }
    return false;
}

QString CMakeGenerator::projectName() const
{
    return m_projectName;
}

void CMakeGenerator::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

void CMakeGenerator::initialize(QmlProject *project)
{
    if (!m_enabled)
        return;

    m_moduleNames.clear();
    m_writer = CMakeWriter::create(this);

    m_root = std::make_shared<Node>();
    m_root->type = Node::Type::App;
    m_root->uri = QString("Main");
    m_root->name = QString("Main");
    m_root->dir = project->rootProjectDirectory();

    m_projectName = project->displayName();

    ProjectExplorer::ProjectNode *rootProjectNode = project->rootProjectNode();
    parseNodeTree(m_root, rootProjectNode);
    parseSourceTree();

    createCMakeFiles(m_root);
    createSourceFiles();

    compareWithFileSystem(m_root);
}

void CMakeGenerator::update(const QSet<QString> &added, const QSet<QString> &removed)
{
    if (!m_enabled)
        return;

    QTC_ASSERT(m_writer, return);

    std::set<NodePtr> dirtyModules;
    for (const QString &add : added) {
        const Utils::FilePath path = Utils::FilePath::fromString(add);
        if (ignore(path.parentDir()))
            continue;

        if (auto node = findOrCreateNode(m_root, path.parentDir())) {
            insertFile(node, path);
            if (auto module = findModuleFor(node))
                dirtyModules.insert(module);
        } else {
            QString text("Failed to find Folder for file");
            logIssue(ProjectExplorer::Task::Error, text, path);
        }
    }

    for (const QString &remove : removed) {
        const Utils::FilePath path = Utils::FilePath::fromString(remove);
        if (auto node = findNode(m_root, path.parentDir())) {
            removeFile(node, path);
            if (auto module = findModuleFor(node))
                dirtyModules.insert(module);
        }
    }

    createCMakeFiles(m_root);
    createSourceFiles();
}

bool CMakeGenerator::ignore(const Utils::FilePath &path) const
{
    if (path.isFile()) {
        static const QStringList suffixes = { "hints" };
        return suffixes.contains(path.suffix(), Qt::CaseInsensitive);
    } else if (path.isDir()) {
        if (!m_root->dir.exists())
            return true;

        static const QStringList fileNames = { "CMakeCache.txt", "build.ninja" };

        Utils::FilePath dir = path;
        while (dir.isChildOf(m_root->dir)) {
            for (const QString& fileName : fileNames) {
                Utils::FilePath checkFile = dir.pathAppended(fileName);
                if (checkFile.exists())
                    return true;
            }
            dir = dir.parentDir();
        }
    }
    return false;
}

bool CMakeGenerator::checkUri(const QString& uri, const Utils::FilePath &path) const
{
    QTC_ASSERT(buildSystem(), return false);

    Utils::FilePath relative = path.relativeChildPath(m_root->dir);
    QList<QStringView> pathComponents = relative.pathView().split('/', Qt::SkipEmptyParts);

    for (const auto& import : buildSystem()->importPaths()) {
        Utils::FilePath importPath = Utils::FilePath::fromUserInput(import);
        for (const auto& component : importPath.pathView().split('/', Qt::SkipEmptyParts)) {
            if (component == pathComponents.first())
                pathComponents.pop_front();
        }
    }

    const QStringList uriComponents = uri.split('.', Qt::SkipEmptyParts);
    if (pathComponents.size() == uriComponents.size()) {
        for (qsizetype i=0; i<pathComponents.size(); ++i) {
            if (pathComponents[i] != uriComponents[i])
                return false;
        }
        return true;
    }
    return false;
}

void CMakeGenerator::createCMakeFiles(const NodePtr &node) const
{
    QTC_ASSERT(m_writer, return);

    if (isRootNode(node))
        m_writer->writeRootCMakeFile(node);

    if (node->type == Node::Type::Module || (hasChildModule(node)))
        m_writer->writeModuleCMakeFile(node, m_root);

    for (const NodePtr &n : node->subdirs)
        createCMakeFiles(n);
}

void CMakeGenerator::createSourceFiles() const
{
    QTC_ASSERT(m_writer, return);

    NodePtr sourceNode = {};
    for (const NodePtr &child : m_root->subdirs) {
        if (child->name == m_writer->sourceDirName())
            sourceNode = child;
    }

    if (sourceNode)
        m_writer->writeSourceFiles(sourceNode, m_root);
}

void CMakeGenerator::readQmlDir(const Utils::FilePath &filePath, NodePtr &node) const
{
    node->type = Node::Type::Module;

    QFile f(filePath.toString());
    f.open(QIODevice::ReadOnly);
    QTextStream stream(&f);

    Utils::FilePath dir = filePath.parentDir();
    static const QRegularExpression whitespaceRegex("\\s+");
    while (!stream.atEnd()) {
        const QString line = stream.readLine();
        const QStringList tokenizedLine = line.split(whitespaceRegex);
        const QString maybeFileName = tokenizedLine.last();
        if (tokenizedLine.first().compare("module", Qt::CaseInsensitive) == 0) {
            node->uri = tokenizedLine.last();
            node->name = QString(node->uri).replace('.', '_');
        } else if (maybeFileName.endsWith(".qml", Qt::CaseInsensitive)) {
            Utils::FilePath tmp = dir.pathAppended(maybeFileName);
            if (tokenizedLine.first() == "singleton")
                node->singletons.push_back(tmp);
        }
    }
    f.close();

    if (!checkUri(node->uri, node->dir)) {
        QString text("Unexpected uri %1");
        logIssue(ProjectExplorer::Task::Warning, text.arg(node->uri), node->dir);
    }
}

NodePtr CMakeGenerator::findModuleFor(const NodePtr &node) const
{
    NodePtr current = node;
    while (current->parent) {
        if (current->type == Node::Type::Module)
            return current;

        current = current->parent;
    }
    return nullptr;
}

NodePtr CMakeGenerator::findNode(NodePtr &node, const Utils::FilePath &path) const
{
    for (NodePtr &child : node->subdirs) {
        if (child->dir == path)
            return child;
        if (path.isChildOf(child->dir))
            return findNode(child, path);
    }
    return nullptr;
}

NodePtr CMakeGenerator::findOrCreateNode(NodePtr &node, const Utils::FilePath &path) const
{
    if (auto found = findNode(node, path))
        return found;

    if (!path.isChildOf(node->dir))
        return nullptr;

    auto findSubDir = [](NodePtr &node, const Utils::FilePath &path) -> NodePtr {
        for (NodePtr child : node->subdirs) {
            if (child->dir == path)
                return child;
        }
        return nullptr;
    };

    const Utils::FilePath relative = path.relativeChildPath(node->dir);
    const QList<QStringView> components = relative.pathView().split('/');

    NodePtr lastNode = node;
    for (const auto &comp : components) {

        Utils::FilePath subPath = lastNode->dir.pathAppended(comp.toString());
        if (NodePtr sub = findSubDir(lastNode, subPath)) {
            lastNode = sub;
            continue;
        }
        NodePtr newNode = std::make_shared<Node>();
        newNode->parent = lastNode;
        newNode->name = comp.toString();
        newNode->dir = subPath;
        lastNode->subdirs.push_back(newNode);
        lastNode = newNode;
    }
    return lastNode;
}

bool findFileWithGetter(const Utils::FilePath &file, const NodePtr &node, const FileGetter &getter)
{
    for (const auto &f : getter(node)) {
        if (f == file)
            return true;
    }
    for (const auto &subdir : node->subdirs) {
        if (findFileWithGetter(file, subdir, getter))
            return true;
    }
    return false;
}

bool CMakeGenerator::findFile(const NodePtr &node, const Utils::FilePath &file) const
{
    if (isAssetFile(file)) {
        return findFileWithGetter(file, node, [](const NodePtr &n) { return n->assets; });
    } else if (isQmlFile(file)) {
        if (findFileWithGetter(file, node, [](const NodePtr &n) { return n->files; }))
            return true;
        else if (findFileWithGetter(file, node, [](const NodePtr &n) { return n->singletons; }))
            return true;
    }
    return false;
}

void CMakeGenerator::insertFile(NodePtr &node, const Utils::FilePath &path) const
{
    QString error;
    if (!Utils::FileNameValidatingLineEdit::validateFileName(path.fileName(), false, &error))
        logIssue(ProjectExplorer::Task::Error, error, path);

    if (path.fileName() == "qmldir") {
        readQmlDir(path, node);
    } else if (path.suffix() == "cpp") {
        node->sources.push_back(path);
    } else if (isQmlFile(path)) {
        node->files.push_back(path);
    } else if (isAssetFile(path)) {
        node->assets.push_back(path);
    }
}

void CMakeGenerator::removeFile(NodePtr &node, const Utils::FilePath &path) const
{
    if (path.fileName() == "qmldir") {
        node->type = Node::Type::Folder;
        node->singletons.clear();
        node->uri = "";
        node->name = path.parentDir().fileName();
    } else if (isQmlFile(path)) {
        auto iter = std::find(node->files.begin(), node->files.end(), path);
        if (iter != node->files.end())
            node->files.erase(iter);
    } else if (isAssetFile(path)) {
        auto iter = std::find(node->assets.begin(), node->assets.end(), path);
        if (iter != node->assets.end())
            node->assets.erase(iter);
    }
}

void CMakeGenerator::printModules(const NodePtr &node) const
{
    if (node->type == Node::Type::Module)
        qDebug() << "Module: " << node->name;

    for (const auto &child : node->subdirs)
        printModules(child);
}

void CMakeGenerator::printNodeTree(const NodePtr &generatorNode, size_t indent) const
{
    auto addIndent = [](size_t level) -> QString {
        QString str;
        for (size_t i = 0; i < level; ++i)
            str += "    ";
        return str;
    };

    QString typeString;
    switch (generatorNode->type)
    {
    case Node::Type::App:
        typeString = "Node::Type::App";
        break;
    case Node::Type::Folder:
        typeString = "Node::Type::Folder";
        break;
    case Node::Type::Module:
        typeString = "Node::Type::Module";
        break;
    case Node::Type::Library:
        typeString = "Node::Type::Library";
        break;
    default:
        typeString = "Node::Type::Undefined";
    }

    qDebug() << addIndent(indent) << "GeneratorNode: " << generatorNode->name;
    qDebug() << addIndent(indent) << "type: " << typeString;
    qDebug() << addIndent(indent) << "directory: " << generatorNode->dir;
    qDebug() << addIndent(indent) << "files: " << generatorNode->files;
    qDebug() << addIndent(indent) << "singletons: " << generatorNode->singletons;
    qDebug() << addIndent(indent) << "assets: " << generatorNode->assets;
    qDebug() << addIndent(indent) << "sources: " << generatorNode->sources;

    for (const auto &child : generatorNode->subdirs)
        printNodeTree(child, indent + 1);
}

void CMakeGenerator::parseNodeTree(NodePtr &generatorNode,
                                   const ProjectExplorer::FolderNode *folderNode)
{
    for (const auto *childNode : folderNode->nodes()) {
        if (const auto *subFolderNode = childNode->asFolderNode()) {
            if (ignore(subFolderNode->filePath()))
                continue;

            NodePtr childGeneratorNode = std::make_shared<Node>();
            childGeneratorNode->parent = generatorNode;
            childGeneratorNode->dir = subFolderNode->filePath();
            childGeneratorNode->name = subFolderNode->displayName();
            childGeneratorNode->uri = childGeneratorNode->name;
            parseNodeTree(childGeneratorNode, subFolderNode);
            generatorNode->subdirs.push_back(childGeneratorNode);
        } else if (auto *fileNode = childNode->asFileNode()) {
            insertFile(generatorNode, fileNode->filePath());
        }
    }

    if (m_writer)
        m_writer->transformNode(generatorNode);

    if (generatorNode->type == Node::Type::Module)
        m_moduleNames.push_back(generatorNode->name);
}

void CMakeGenerator::parseSourceTree()
{
    QTC_ASSERT(m_writer, return);

    const Utils::FilePath srcDir = m_root->dir.pathAppended(m_writer->sourceDirName());
    QDirIterator it(srcDir.path(), {"*.cpp"}, QDir::Files, QDirIterator::Subdirectories);

    NodePtr srcNode = std::make_shared<Node>();
    srcNode->parent = m_root;
    srcNode->type = Node::Type::App;
    srcNode->dir = srcDir;
    srcNode->uri = srcDir.baseName();
    srcNode->name = srcNode->uri;

    while (it.hasNext()) {
        auto next = it.next();
        srcNode->sources.push_back(Utils::FilePath::fromString(next));
    }

    if (srcNode->sources.empty())
        srcNode->sources.push_back(srcDir.pathAppended("main.cpp"));

    if (m_writer)
        m_writer->transformNode(srcNode);

    m_root->subdirs.push_back(srcNode);
}

void CMakeGenerator::compareWithFileSystem(const NodePtr &node) const
{
    std::vector<Utils::FilePath> files;
    QDirIterator iter(node->dir.path(), QDir::Files, QDirIterator::Subdirectories);

    while (iter.hasNext()) {
        auto next = Utils::FilePath::fromString(iter.next());
        if (ignore(next.parentDir()))
            continue;

        if (isAssetFile(next) && !findFile(next) && !ignore(next))
            files.push_back(next);
    }

    const QString text("File is not part of the project");
    for (const auto &file : files)
        logIssue(ProjectExplorer::Task::Warning, text, file);
}

} // namespace GenerateCmake
} // namespace QmlProjectManager
