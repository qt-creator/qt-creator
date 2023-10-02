// Copyright (C) 2016 Canonical Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmaketoolmanager.h"

#include "cmakekitaspect.h"
#include "cmakeprojectmanagertr.h"
#include "cmakespecificsettings.h"
#include "cmaketoolsettingsaccessor.h"

#include "3rdparty/rstparser/rstparser.h"

#include <extensionsystem/pluginmanager.h>

#include <coreplugin/helpmanager.h>
#include <coreplugin/icore.h>

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/target.h>
#include <stack>
#include <utils/environment.h>
#include <utils/pointeralgorithm.h>
#include <utils/qtcassert.h>

#include <nanotrace/nanotrace.h>

using namespace Core;
using namespace Utils;

namespace CMakeProjectManager {

class CMakeToolManagerPrivate
{
public:
    Id m_defaultCMake;
    std::vector<std::unique_ptr<CMakeTool>> m_cmakeTools;
    Internal::CMakeToolSettingsAccessor m_accessor;
};

class HtmlHandler : public rst::ContentHandler
{
private:
    std::stack<QString> m_tags;

    QStringList m_p;
    QStringList m_h3;
    QStringList m_cmake_code;

    QString m_last_directive_type;
    QString m_last_directive_class;

    void StartBlock(rst::BlockType type) final
    {
        QString tag;
        switch (type) {
        case rst::REFERENCE_LINK:
            // not used, HandleReferenceLink is used instead
            break;
        case rst::H1:
            tag = "h1";
            break;
        case rst::H2:
            tag = "h2";
            break;
        case rst::H3:
            tag = "h3";
            break;
        case rst::H4:
            tag = "h4";
            break;
        case rst::H5:
            tag = "h5";
            break;
        case rst::CODE:
            tag = "code";
            break;
        case rst::PARAGRAPH:
            tag = "p";
            break;
        case rst::LINE_BLOCK:
            tag = "pre";
            break;
        case rst::BLOCK_QUOTE:
            if (m_last_directive_type == "code-block" && m_last_directive_class == "cmake")
                tag = "cmake-code";
            else
                tag = "blockquote";
            break;
        case rst::BULLET_LIST:
            tag = "ul";
            break;
        case rst::LIST_ITEM:
            tag = "li";
            break;
        case rst::LITERAL_BLOCK:
            tag = "pre";
            break;
        }

        if (tag == "p")
            m_p.push_back(QString());
        if (tag == "h3")
            m_h3.push_back(QString());
        if (tag == "cmake-code")
            m_cmake_code.push_back(QString());

        if (tag == "code" && m_tags.top() == "p")
            m_p.last().append("`");

        m_tags.push(tag);
    }

    void EndBlock() final
    {
        // Add a new "p" collector for any `code` markup that comes afterwads
        // since we are insterested only in the first paragraph.
        if (m_tags.top() == "p")
            m_p.push_back(QString());

        if (m_tags.top() == "code" && !m_p.isEmpty()) {
            m_tags.pop();

            if (m_tags.size() > 0 && m_tags.top() == "p")
                m_p.last().append("`");
        } else {
            m_tags.pop();
        }
    }

    void HandleText(const char *text, std::size_t size) final
    {
        if (m_last_directive_type.endsWith("replace"))
            return;

        QString str = QString::fromUtf8(text, size);

        if (m_tags.top() == "h3")
            m_h3.last().append(str);
        if (m_tags.top() == "p")
            m_p.last().append(str);
        if (m_tags.top() == "cmake-code")
            m_cmake_code.last().append(str);
        if (m_tags.top() == "code" && !m_p.isEmpty())
            m_p.last().append(str);
    }

    void HandleDirective(const std::string &type, const std::string &name) final
    {
        m_last_directive_type = QString::fromStdString(type);
        m_last_directive_class = QString::fromStdString(name);
    }

    void HandleReferenceLink(const std::string &type, const std::string &text) final
    {
        Q_UNUSED(type)
        if (!m_p.isEmpty())
            m_p.last().append(QString::fromStdString(text));
    }

public:
    QString content() const
    {
        const QString title = m_h3.isEmpty() ? QString() : m_h3.first();
        const QString description = m_p.isEmpty() ? QString() : m_p.first();
        const QString cmakeCode = m_cmake_code.isEmpty() ? QString() : m_cmake_code.first();

        return QString("### %1\n\n%2\n\n````\n%3\n````").arg(title, description, cmakeCode);
    }
};

static CMakeToolManagerPrivate *d = nullptr;
CMakeToolManager *CMakeToolManager::m_instance = nullptr;

CMakeToolManager::CMakeToolManager()
{
    QTC_ASSERT(!m_instance, return);
    m_instance = this;

    qRegisterMetaType<QString *>();

    d = new CMakeToolManagerPrivate;
    connect(ICore::instance(), &ICore::saveSettingsRequested,
            this, &CMakeToolManager::saveCMakeTools);

    connect(this, &CMakeToolManager::cmakeAdded, this, &CMakeToolManager::cmakeToolsChanged);
    connect(this, &CMakeToolManager::cmakeRemoved, this, &CMakeToolManager::cmakeToolsChanged);
    connect(this, &CMakeToolManager::cmakeUpdated, this, &CMakeToolManager::cmakeToolsChanged);

    setObjectName("CMakeToolManager");
    ExtensionSystem::PluginManager::addObject(this);

    CMakeKitAspect::createFactories();
}

CMakeToolManager::~CMakeToolManager()
{
    ExtensionSystem::PluginManager::removeObject(this);
    delete d;
}

CMakeToolManager *CMakeToolManager::instance()
{
    return m_instance;
}

QList<CMakeTool *> CMakeToolManager::cmakeTools()
{
    return Utils::toRawPointer<QList>(d->m_cmakeTools);
}

bool CMakeToolManager::registerCMakeTool(std::unique_ptr<CMakeTool> &&tool)
{
    if (!tool || Utils::contains(d->m_cmakeTools, tool.get()))
        return true;

    const Utils::Id toolId = tool->id();
    QTC_ASSERT(toolId.isValid(),return false);

    //make sure the same id was not used before
    QTC_ASSERT(!Utils::contains(d->m_cmakeTools, [toolId](const std::unique_ptr<CMakeTool> &known) {
        return toolId == known->id();
    }), return false);

    d->m_cmakeTools.emplace_back(std::move(tool));

    emit CMakeToolManager::m_instance->cmakeAdded(toolId);

    ensureDefaultCMakeToolIsValid();

    updateDocumentation();

    return true;
}

void CMakeToolManager::deregisterCMakeTool(const Id &id)
{
    auto toRemove = Utils::take(d->m_cmakeTools, Utils::equal(&CMakeTool::id, id));
    if (toRemove.has_value()) {
        ensureDefaultCMakeToolIsValid();

        updateDocumentation();

        emit m_instance->cmakeRemoved(id);
    }
}

CMakeTool *CMakeToolManager::defaultProjectOrDefaultCMakeTool()
{
    CMakeTool *tool = nullptr;

    if (auto bs = ProjectExplorer::ProjectTree::currentBuildSystem())
        tool = CMakeKitAspect::cmakeTool(bs->target()->kit());
    if (!tool)
        tool = CMakeToolManager::defaultCMakeTool();

    return tool;
}

CMakeTool *CMakeToolManager::defaultCMakeTool()
{
    return findById(d->m_defaultCMake);
}

void CMakeToolManager::setDefaultCMakeTool(const Id &id)
{
    if (d->m_defaultCMake != id && findById(id)) {
        d->m_defaultCMake = id;
        emit m_instance->defaultCMakeChanged();
        return;
    }

    ensureDefaultCMakeToolIsValid();
}

CMakeTool *CMakeToolManager::findByCommand(const Utils::FilePath &command)
{
    return Utils::findOrDefault(d->m_cmakeTools, Utils::equal(&CMakeTool::cmakeExecutable, command));
}

CMakeTool *CMakeToolManager::findById(const Id &id)
{
    return Utils::findOrDefault(d->m_cmakeTools, Utils::equal(&CMakeTool::id, id));
}

void CMakeToolManager::restoreCMakeTools()
{
    NANOTRACE_SCOPE("CMakeProjectManager", "CMakeToolManager::restoreCMakeTools");
    Internal::CMakeToolSettingsAccessor::CMakeTools tools
            = d->m_accessor.restoreCMakeTools(ICore::dialogParent());
    d->m_cmakeTools = std::move(tools.cmakeTools);
    setDefaultCMakeTool(tools.defaultToolId);

    updateDocumentation();

    emit m_instance->cmakeToolsLoaded();

    // Store the default CMake tool "Autorun CMake" value globally
    // TODO: Remove in Qt Creator 13
    Internal::CMakeSpecificSettings &s = Internal::settings();
    if (s.autorunCMake() == s.autorunCMake.defaultValue()) {
        CMakeTool *cmake = defaultCMakeTool();
        s.autorunCMake.setValue(cmake ? cmake->isAutoRun() : true);
        s.writeSettings();
    }
}

void CMakeToolManager::updateDocumentation()
{
    const QList<CMakeTool *> tools = cmakeTools();
    QStringList docs;
    for (const auto tool : tools) {
        if (!tool->qchFilePath().isEmpty())
            docs.append(tool->qchFilePath().toString());
    }
    Core::HelpManager::registerDocumentation(docs);
}

QString CMakeToolManager::toolTipForRstHelpFile(const FilePath &helpFile)
{
    static QHash<FilePath, QString> map;
    static QMutex mutex;
    QMutexLocker locker(&mutex);

    if (map.contains(helpFile))
        return map.value(helpFile);

    auto content = helpFile.fileContents(1024).value_or(QByteArray());
    content.replace("\r\n", "\n");

    HtmlHandler handler;
    rst::Parser parser(&handler);
    parser.Parse(content.left(content.lastIndexOf('\n')));

    const QString tooltip = handler.content();

    map[helpFile] = tooltip;
    return tooltip;
}

QList<Id> CMakeToolManager::autoDetectCMakeForDevice(const FilePaths &searchPaths,
                                                const QString &detectionSource,
                                                QString *logMessage)
{
    QList<Id> result;
    QStringList messages{Tr::tr("Searching CMake binaries...")};
    for (const FilePath &path : searchPaths) {
        const FilePath cmake = path.pathAppended("cmake").withExecutableSuffix();
        if (cmake.isExecutableFile()) {
            const Id currentId = registerCMakeByPath(cmake, detectionSource);
            if (currentId.isValid())
                result.push_back(currentId);
            messages.append(Tr::tr("Found \"%1\"").arg(cmake.toUserOutput()));
        }
    }
    if (logMessage)
        *logMessage = messages.join('\n');

    return result;
}


Id CMakeToolManager::registerCMakeByPath(const FilePath &cmakePath, const QString &detectionSource)
{
    Id id = Id::fromString(cmakePath.toUserOutput());

    CMakeTool *cmakeTool = findById(id);
    if (cmakeTool)
        return cmakeTool->id();

    auto newTool = std::make_unique<CMakeTool>(CMakeTool::ManualDetection, id);
    newTool->setFilePath(cmakePath);
    newTool->setDetectionSource(detectionSource);
    newTool->setDisplayName(cmakePath.toUserOutput());
    id = newTool->id();
    registerCMakeTool(std::move(newTool));

    return id;
}

void CMakeToolManager::removeDetectedCMake(const QString &detectionSource, QString *logMessage)
{
    QStringList logMessages{Tr::tr("Removing CMake entries...")};
    while (true) {
        auto toRemove = Utils::take(d->m_cmakeTools, Utils::equal(&CMakeTool::detectionSource, detectionSource));
        if (!toRemove.has_value())
            break;
        logMessages.append(Tr::tr("Removed \"%1\"").arg((*toRemove)->displayName()));
        emit m_instance->cmakeRemoved((*toRemove)->id());
    }

    ensureDefaultCMakeToolIsValid();
    updateDocumentation();
    if (logMessage)
        *logMessage = logMessages.join('\n');
}

void CMakeToolManager::listDetectedCMake(const QString &detectionSource, QString *logMessage)
{
    QTC_ASSERT(logMessage, return);
    QStringList logMessages{Tr::tr("CMake:")};
    for (const auto &tool : std::as_const(d->m_cmakeTools)) {
        if (tool->detectionSource() == detectionSource)
            logMessages.append(tool->displayName());
    }
    *logMessage = logMessages.join('\n');
}

void CMakeToolManager::notifyAboutUpdate(CMakeTool *tool)
{
    if (!tool || !Utils::contains(d->m_cmakeTools, tool))
        return;
    emit m_instance->cmakeUpdated(tool->id());
}

void CMakeToolManager::saveCMakeTools()
{
    d->m_accessor.saveCMakeTools(cmakeTools(), d->m_defaultCMake, ICore::dialogParent());
}

void CMakeToolManager::ensureDefaultCMakeToolIsValid()
{
    const Utils::Id oldId = d->m_defaultCMake;
    if (d->m_cmakeTools.size() == 0) {
        d->m_defaultCMake = Utils::Id();
    } else {
        if (findById(d->m_defaultCMake))
            return;
        auto cmakeTool = Utils::findOrDefault(
                    cmakeTools(), [](CMakeTool *tool){ return tool->detectionSource().isEmpty(); });
        if (cmakeTool)
            d->m_defaultCMake = cmakeTool->id();
    }

    // signaling:
    if (oldId != d->m_defaultCMake)
        emit m_instance->defaultCMakeChanged();
}

} // CMakeProjectManager
