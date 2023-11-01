// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "insightmodel.h"
#include "insightview.h"

#include <auxiliarydataproperties.h>
#include <externaldependenciesinterface.h>
#include <plaintexteditmodifier.h>
#include <rewriterview.h>
#include <signalhandlerproperty.h>
#include <qmldesignerplugin.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtkitaspect.h>

#include <utils/filepath.h>
#include <utils/qtcassert.h>

#include <QAbstractListModel>
#include <QApplication>
#include <QDebug>

namespace QmlDesigner {

namespace {

constexpr QStringView insightConfFile{u"qtinsight.conf"};
constexpr QStringView qtdsConfFile{u"qtdsinsight.conf"};
constexpr QStringView dataFolder{u"qtinsight"};

constexpr QStringView insightImport{u"QtInsightTracker"};
constexpr QStringView signalHandler{u"Component.onCompleted"};
constexpr QStringView regExp{u"InsightTracker\\.enabled\\s*=\\s*(true|false)"};

constexpr std::string_view defaultColor{"#000000"};
constexpr std::string_view predefinedStr{"predefined"};
constexpr std::string_view customStr{"custom"};

constexpr QStringView defaultCategoryName{u"New Category"};

// JSON attribut names
constexpr std::string_view categoriesAtt{"categories"};
constexpr std::string_view tokenAtt{"token"};
constexpr std::string_view syncAtt{"sync"};
constexpr std::string_view intervalAtt{"interval"};
constexpr std::string_view secondsAtt{"seconds"};
constexpr std::string_view minutesAtt{"minutes"};

constexpr std::string_view nameAtt{"name"};
constexpr std::string_view typeAtt{"type"};
constexpr std::string_view colorAtt{"color"};

QByteArray fileToByteArray(const QString &filePath)
{
    QFile file(filePath);

    if (!file.exists()) {
        qWarning() << "File does not exist" << filePath;
        return {};
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open" << filePath << file.error() << file.errorString();
        return {};
    }

    return file.readAll();
}

QString fileToString(const QString &filePath)
{
    return QString::fromUtf8(fileToByteArray(filePath));
}

bool isNodeEnabled(const ModelNode &node)
{
    SignalHandlerProperty property = node.signalHandlerProperty(signalHandler.toUtf8());

    QString src = property.source();
    const QRegularExpression re(regExp.toString());
    QRegularExpressionMatch match = re.match(src);

    if (match.hasMatch() && !match.capturedView(1).isEmpty())
        return QVariant(match.captured(1)).toBool();

    return false;
}

void setNodeEnabled(const ModelNode &node, bool value)
{
    const QString valueAsStr = QVariant(value).toString();

    if (node.hasSignalHandlerProperty(signalHandler.toUtf8())) {
        SignalHandlerProperty property = node.signalHandlerProperty(signalHandler.toUtf8());

        QString src = property.source();
        const QRegularExpression re(regExp.toString());
        QRegularExpressionMatch match = re.match(src);

        if (match.hasMatch() && !match.capturedView(1).isEmpty())
            src.replace(match.capturedStart(1), match.capturedLength(1), valueAsStr);

        property.setSource(src);
    } else {
        SignalHandlerProperty property = node.signalHandlerProperty(signalHandler.toUtf8());
        property.setSource("InsightTracker.enabled = " + valueAsStr);
    }
}

json readJSON(const QString &filePath)
{
    const QByteArray data = fileToByteArray(filePath);
    if (data.isEmpty()) {
        qWarning() << "File is empty" << filePath;
        return {};
    }

    json document;

    try {
        document = json::parse(data.data());
    } catch (json::parse_error &e) {
        qWarning() << "JSON parse error" << e.what();
        return {};
    }

    return document;
}

bool writeJSON(const QString &filePath, const json &document)
{
    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Could not open file" << filePath << file.error() << file.errorString();
        return false;
    }

    auto result = file.write(document.dump(4).c_str());

    if (result == -1)
        qWarning() << "Could not write file" << filePath << file.error() << file.errorString();

    file.close();

    return true;
}

json createCategory(std::string_view name,
                    std::string_view type = customStr,
                    std::string_view color = defaultColor)
{
    return json::object({{nameAtt, name}, {typeAtt, type}, {colorAtt, color}});
}

// Checks if a is fully, partially or not at all contained in b.
Qt::CheckState checkState(const std::vector<std::string> &a, const std::vector<std::string> &b)
{
    unsigned count = 0;
    std::for_each(a.begin(), a.end(), [&](const std::string &s) {
        if (std::find(b.begin(), b.end(), s) != b.end())
            ++count;
    });

    if (count == 0)
        return Qt::Unchecked;
    else if (count == a.size())
        return Qt::Checked;

    return Qt::PartiallyChecked;
}

struct ModelBuilder
{
    ModelBuilder(const QString &filePath, ExternalDependenciesInterface &externalDependencies)
    {
        const QString fileContent = fileToString(filePath);
        if (fileContent.isEmpty()) {
            qWarning() << "File is empty" << filePath;
            return;
        }

        document = std::make_unique<QTextDocument>(fileContent);
        modifier = std::make_unique<NotIndentingTextEditModifier>(document.get(),
                                                                  QTextCursor{document.get()});

        rewriter = std::make_unique<RewriterView>(externalDependencies, RewriterView::Amend);
        rewriter->setCheckSemanticErrors(false);
        rewriter->setCheckLinkErrors(false);
        rewriter->setTextModifier(modifier.get());

        model = QmlDesigner::Model::create("QtQuick.Item", 2, 1);
        model->setRewriterView(rewriter.get());
    }

    std::unique_ptr<QTextDocument> document;
    std::unique_ptr<NotIndentingTextEditModifier> modifier;
    std::unique_ptr<RewriterView> rewriter;
    ModelPointer model;
};

} // namespace

InsightModel::InsightModel(InsightView *view, ExternalDependenciesInterface &externalDependencies)
    : m_insightView(view)
    , m_externalDependencies(externalDependencies)
    , m_fileSystemWatcher(new Utils::FileSystemWatcher(this))
{
    QObject::connect(ProjectExplorer::ProjectManager::instance(),
                     &ProjectExplorer::ProjectManager::startupProjectChanged,
                     this,
                     [&](ProjectExplorer::Project *project) {
                         if (project)
                             m_initialized = false;
                     });

    QObject::connect(m_fileSystemWatcher,
                     &Utils::FileSystemWatcher::fileChanged,
                     this,
                     &InsightModel::handleFileChange);
}

int InsightModel::rowCount(const QModelIndex &) const
{
    return m_qtdsConfig.empty() ? 0 : static_cast<int>(m_qtdsConfig.size());
}

QVariant InsightModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount() || m_qtdsConfig.empty())
        return QVariant();

    json::json_pointer ptr;
    ptr.push_back(std::to_string(index.row()));

    if (!m_qtdsConfig.contains(ptr))
        return QVariant();

    auto element = m_qtdsConfig[ptr];

    switch (role) {
    case CategoryName:
        return QString::fromStdString(element.value(nameAtt, ""));
    case CategoryColor:
        return QString::fromStdString(element.value(colorAtt, ""));
    case CategoryType:
        return QString::fromStdString(element.value(typeAtt, ""));
    case CategoryActive: {
        auto categories = activeCategories();
        auto category = element.value(nameAtt, "");
        return std::find(std::begin(categories), std::end(categories), category)
               != std::end(categories);
    }
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> InsightModel::roleNames() const
{
    static QHash<int, QByteArray> roleNames{{CategoryName, "categoryName"},
                                            {CategoryColor, "categoryColor"},
                                            {CategoryType, "categoryType"},
                                            {CategoryActive, "categoryActive"}};
    return roleNames;
}

void InsightModel::setup()
{
    if (m_initialized)
        return;

    const QString projectUrl = m_externalDependencies.projectUrl().toLocalFile();

    m_mainQmlInfo = QFileInfo(projectUrl + "/main.qml");
    m_configInfo = QFileInfo(projectUrl + "/" + insightConfFile);
    m_qtdsConfigInfo = QFileInfo(projectUrl + "/" + qtdsConfFile);

    parseMainQml();
    parseDefaultConfig();
    parseConfig();
    parseQtdsConfig();

    beginResetModel();

    if (m_qtdsConfig.empty())
        createQtdsConfig();
    else
        updateQtdsConfig();

    endResetModel();

    updateCheckState();

    if (m_enabled) {
        // Flush config files and start listening to modifications
        writeJSON(m_configInfo.absoluteFilePath(), m_config);
        writeJSON(m_qtdsConfigInfo.absoluteFilePath(), m_qtdsConfig);
    }

    m_fileSystemWatcher->addFile(m_mainQmlInfo.absoluteFilePath(),
                                 Utils::FileSystemWatcher::WatchModifiedDate);
    m_fileSystemWatcher->addFile(m_configInfo.absoluteFilePath(),
                                 Utils::FileSystemWatcher::WatchModifiedDate);
    m_fileSystemWatcher->addFile(m_qtdsConfigInfo.absoluteFilePath(),
                                 Utils::FileSystemWatcher::WatchModifiedDate);

    m_initialized = true;
}

void InsightModel::addCategory()
{
    int counter = 0;
    QString newCategory = defaultCategoryName.toString();

    while (hasCategory(newCategory)) {
        ++counter;
        newCategory = QString(QStringLiteral("%1%2")).arg(defaultCategoryName).arg(counter);
    }

    json tmp = m_qtdsConfig;
    tmp.push_back(createCategory(newCategory.toStdString()));
    writeJSON(m_qtdsConfigInfo.absoluteFilePath(), tmp);
}

void InsightModel::removeCateogry(int idx)
{
    json::json_pointer ptr;
    ptr.push_back(std::to_string(idx));
    ptr.push_back(std::string(nameAtt));

    // Check if category is active and remove it there as well
    auto active = activeCategories();
    auto category = m_qtdsConfig.contains(ptr) ? m_qtdsConfig[ptr].get<std::string>() : "";
    auto it = std::find(std::begin(active), std::end(active), category);
    if (it != std::end(active)) {
        active.erase(it);

        json tmp = m_config;
        tmp[categoriesAtt] = active;
        writeJSON(m_configInfo.absoluteFilePath(), tmp);
    }

    json tmp = m_qtdsConfig;
    tmp.erase(idx);
    writeJSON(m_qtdsConfigInfo.absoluteFilePath(), tmp);
}

bool InsightModel::renameCategory(int idx, const QString &name)
{
    if (hasCategory(name))
        return false;

    json::json_pointer ptr;
    ptr.push_back(std::to_string(idx));
    ptr.push_back(std::string(nameAtt));

    // Check if category is active and rename it there as well
    auto active = activeCategories();
    auto category = m_qtdsConfig.contains(ptr) ? m_qtdsConfig[ptr].get<std::string>() : "";
    auto it = std::find(std::begin(active), std::end(active), category);
    if (it != std::end(active)) {
        *it = name.toStdString();

        json tmp = m_config;
        tmp[categoriesAtt] = active;
        writeJSON(m_configInfo.absoluteFilePath(), tmp);
    }

    json tmp = m_qtdsConfig;
    tmp[ptr] = name.toStdString();
    writeJSON(m_qtdsConfigInfo.absoluteFilePath(), tmp);
    return true;
}

void InsightModel::setCategoryActive(int idx, bool value)
{
    json::json_pointer ptr;
    ptr.push_back(std::to_string(idx));
    ptr.push_back(std::string(nameAtt));

    auto categoryName = m_qtdsConfig.contains(ptr) ? m_qtdsConfig[ptr].get<std::string>() : "";
    auto categories = activeCategories();

    if (value) { // active = true
        if (std::find(std::begin(categories), std::end(categories), categoryName)
            == std::end(categories))
            categories.push_back(categoryName);
    } else { // active = false
        categories.erase(std::remove(categories.begin(), categories.end(), categoryName),
                         categories.end());
    }

    json tmp = m_config;
    tmp[categoriesAtt] = categories;
    writeJSON(m_configInfo.absoluteFilePath(), tmp);
}

bool InsightModel::enabled() const
{
    return m_enabled;
}

void InsightModel::setEnabled(bool value)
{
    if (!m_mainQmlInfo.exists()) {
        qWarning() << "File does not exist" << m_mainQmlInfo.absoluteFilePath();
        return;
    }

    ModelBuilder builder(m_mainQmlInfo.absoluteFilePath(), m_externalDependencies);

    if (!builder.model) {
        qWarning() << "Could not create model" << m_mainQmlInfo.absoluteFilePath();
        return;
    }

    // Add import if it does not exist yet
    Import import = Import::createLibraryImport(insightImport.toString(), "1.0");
    if (!builder.model->hasImport(import, true, true) && value) {
        builder.model->changeImports({import}, {});
    }

    bool insightEnabled = isNodeEnabled(builder.rewriter->rootModelNode());
    if (insightEnabled == value)
        return;

    setNodeEnabled(builder.rewriter->rootModelNode(), value);

    QFile file(m_mainQmlInfo.absoluteFilePath());

    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Could not open" << m_mainQmlInfo.absoluteFilePath() << file.error()
                   << file.errorString();
        return;
    }

    auto result = file.write(builder.rewriter->textModifierContent().toUtf8());

    if (result == -1)
        qWarning() << "Could not write file" << m_mainQmlInfo.absoluteFilePath() << file.error()
                   << file.errorString();

    // If enabled and config files do not exist yet, write them
    if (value) {
        if (!m_configInfo.exists())
            writeJSON(m_configInfo.absoluteFilePath(), m_config);
        if (!m_qtdsConfigInfo.exists())
            writeJSON(m_qtdsConfigInfo.absoluteFilePath(), m_qtdsConfig);
    }

    m_enabled = value;
    setAuxiliaryEnabled(m_enabled);

    QmlDesignerPlugin::instance()->viewManager().resetPropertyEditorView();
}

QString InsightModel::token() const
{
    if (m_config.empty())
        return {};

    return QString::fromStdString(m_config.value(tokenAtt, ""));
}

void InsightModel::setToken(const QString &value)
{
    writeConfigValue(json::json_pointer("/" + std::string(tokenAtt)), value.toStdString());
}

int InsightModel::minutes() const
{
    if (m_config.empty())
        return {};

    json::json_pointer ptr;
    ptr.push_back(std::string(syncAtt));
    ptr.push_back(std::string(intervalAtt));
    ptr.push_back(std::string(minutesAtt));

    return m_config.value(ptr, 0);
}

void InsightModel::setMinutes(int value)
{
    json::json_pointer ptr;
    ptr.push_back(std::string(syncAtt));
    ptr.push_back(std::string(intervalAtt));
    ptr.push_back(std::string(minutesAtt));

    writeConfigValue(ptr, value);
}

void InsightModel::selectAllPredefined()
{
    selectAll(predefinedCategories(), m_predefinedCheckState);
}

void InsightModel::selectAllCustom()
{
    selectAll(customCategories(), m_customCheckState);
}

void InsightModel::handleFileChange(const QString &path)
{
    if (m_mainQmlInfo.absoluteFilePath() == path)
        parseMainQml();
    else if (m_configInfo.absoluteFilePath() == path)
        parseConfig();
    else if (m_qtdsConfigInfo.absoluteFilePath() == path) {
        beginResetModel();
        parseQtdsConfig();
        endResetModel();
    }
}

void InsightModel::setAuxiliaryEnabled(bool value)
{
    ModelNode root = m_insightView->rootModelNode();
    if (root.isValid())
        root.setAuxiliaryData(insightEnabledProperty, value);
}

void InsightModel::setAuxiliaryCategories(const std::vector<std::string> &categories)
{
    ModelNode root = m_insightView->rootModelNode();
    if (root.isValid()) {
        QStringList c;
        std::for_each(categories.begin(), categories.end(), [&](const std::string &s) {
            c.append(QString::fromStdString(s));
        });

        root.setAuxiliaryData(insightCategoriesProperty, c);
    }
}

void InsightModel::hideCursor()
{
    if (QApplication::overrideCursor())
        return;

    QApplication::setOverrideCursor(QCursor(Qt::BlankCursor));

    if (QWidget *w = QApplication::activeWindow())
        m_lastPos = QCursor::pos(w->screen());
}

void InsightModel::restoreCursor()
{
    if (!QApplication::overrideCursor())
        return;

    QApplication::restoreOverrideCursor();

    if (QWidget *w = QApplication::activeWindow())
        QCursor::setPos(w->screen(), m_lastPos);
}

void InsightModel::holdCursorInPlace()
{
    if (!QApplication::overrideCursor())
        return;

    if (QWidget *w = QApplication::activeWindow())
        QCursor::setPos(w->screen(), m_lastPos);
}

int InsightModel::devicePixelRatio()
{
    if (QWidget *w = QApplication::activeWindow())
        return w->devicePixelRatio();

    return 1;
}

void InsightModel::parseMainQml()
{
    ModelBuilder builder(m_mainQmlInfo.absoluteFilePath(), m_externalDependencies);

    if (!builder.model)
        return;

    Import import = Import::createLibraryImport(insightImport.toString(), "1.0");
    if (!builder.model->hasImport(import, true, true))
        return;

    bool insightEnabled = isNodeEnabled(builder.rewriter->rootModelNode());

    if (m_enabled != insightEnabled) {
        m_enabled = insightEnabled;
        emit enabledChanged();

        setAuxiliaryEnabled(m_enabled);
    }
}

void InsightModel::parseDefaultConfig()
{
    // Load default insight config from plugin
    const ProjectExplorer::Target *target = ProjectExplorer::ProjectTree::currentTarget();
    if (target) {
        const QtSupport::QtVersion *qtVersion = QtSupport::QtKitAspect::qtVersion(target->kit());

        if (qtVersion) {
            m_defaultConfig = readJSON(qtVersion->dataPath().toString() + "/" + dataFolder + "/"
                                       + insightConfFile);
        }
    }
}

void InsightModel::parseConfig()
{
    json target = readJSON(m_configInfo.absoluteFilePath());

    if (target.empty()) {
        if (m_defaultConfig.empty()) {
            qWarning() << "Could not find default or user insight config.";
            return;
        }
        // Copy default config
        m_config = m_defaultConfig;

        // Try to overwrite seconds entry in config as it is set in the default config to 60,
        // but currently not supported in QtDS.
        json::json_pointer ptr;
        ptr.push_back(std::string(syncAtt));
        ptr.push_back(std::string(intervalAtt));

        if (m_config.contains(ptr))
            m_config[ptr][secondsAtt] = 0;
    } else {
        bool resetModel = false;

        if (m_config.empty()) {
            m_config = target;

            emit tokenChanged();
            emit minutesChanged();
            resetModel = true;
        } else {
            json patch = json::diff(m_config, target);

            m_config = target;
            for (auto it : patch) {
                if (!it.contains("path"))
                    continue;

                json::json_pointer tmp(it["path"].get<std::string>());

                if (tmp.back() == tokenAtt)
                    emit tokenChanged();

                if (tmp.back() == minutesAtt)
                    emit minutesChanged();

                if (!tmp.to_string().compare(1, categoriesAtt.size(), categoriesAtt))
                    resetModel = true;
            }
        }

        if (resetModel) {
            updateCheckState();
            beginResetModel();
            endResetModel();
        }
    }
}

void InsightModel::parseQtdsConfig()
{
    m_qtdsConfig = readJSON(m_qtdsConfigInfo.absoluteFilePath());
    updateCheckState();
    setAuxiliaryCategories(customCategories());
}

// Create new QtDS Insight configuration
void InsightModel::createQtdsConfig()
{
    json categories = json::array();

    auto active = activeCategories();
    auto predefined = predefinedCategories();

    std::vector<std::string> custom;

    std::set_difference(std::make_move_iterator(active.begin()),
                        std::make_move_iterator(active.end()),
                        std::make_move_iterator(predefined.begin()),
                        std::make_move_iterator(predefined.end()),
                        std::back_inserter(custom));

    for (const auto &c : predefined)
        categories.push_back(createCategory(c, predefinedStr));

    for (const auto &c : custom)
        categories.push_back(createCategory(c));

    m_qtdsConfig = categories;
}

// Update existing QtDS Insight configuration
void InsightModel::updateQtdsConfig()
{
    auto contains = [&](const json &arr, const std::string &val) {
        for (auto it : arr) {
            if (val == it[nameAtt].get<std::string>())
                return true;
        }
        return false;
    };

    auto active = activeCategories();
    auto predefined = predefinedCategories();
    std::vector<std::string> custom;

    std::set_difference(std::make_move_iterator(active.begin()),
                        std::make_move_iterator(active.end()),
                        std::make_move_iterator(predefined.begin()),
                        std::make_move_iterator(predefined.end()),
                        std::back_inserter(custom));

    for (const auto &c : predefined) {
        if (!contains(m_qtdsConfig, c))
            m_qtdsConfig.push_back(createCategory(c, predefinedStr));
    }

    for (const auto &c : custom) {
        if (!contains(m_qtdsConfig, c))
            m_qtdsConfig.push_back(createCategory(c));
    }
}

void InsightModel::selectAll(const std::vector<std::string> &categories, Qt::CheckState checkState)
{
    auto active = activeCategories();

    if (checkState == Qt::Unchecked || checkState == Qt::PartiallyChecked) {
        // Select all
        std::for_each(categories.begin(), categories.end(), [&](const std::string &s) {
            if (std::find(active.begin(), active.end(), s) == active.end())
                active.push_back(s);
        });
    } else {
        // Unselect all
        std::vector<std::string> diff;

        std::set_difference(active.begin(),
                            active.end(),
                            categories.begin(),
                            categories.end(),
                            std::inserter(diff, diff.begin()));
        active = diff;
    }

    json tmp = m_config;
    tmp[categoriesAtt] = active;
    writeJSON(m_configInfo.absoluteFilePath(), tmp);
}

std::vector<std::string> InsightModel::predefinedCategories() const
{
    std::vector<std::string> categories;
    if (!m_defaultConfig.empty() && m_defaultConfig.contains(categoriesAtt))
        categories = m_defaultConfig[categoriesAtt].get<std::vector<std::string>>();

    std::sort(categories.begin(), categories.end());
    categories.erase(std::unique(categories.begin(), categories.end()), categories.end());

    return categories;
}

std::vector<std::string> InsightModel::activeCategories() const
{
    std::vector<std::string> categories;
    if (!m_config.empty() && m_config.contains(categoriesAtt))
        categories = m_config[categoriesAtt].get<std::vector<std::string>>();

    std::sort(categories.begin(), categories.end());
    categories.erase(std::unique(categories.begin(), categories.end()), categories.end());

    return categories;
}

std::vector<std::string> InsightModel::customCategories() const
{
    std::vector<std::string> categories;
    if (!m_qtdsConfig.empty()) {
        for (auto it : m_qtdsConfig) {
            if (it.contains(typeAtt) && it.contains(nameAtt)
                && it[typeAtt].get<std::string>() == customStr)
                categories.push_back(it[nameAtt].get<std::string>());
        }
    }

    std::sort(categories.begin(), categories.end());
    categories.erase(std::unique(categories.begin(), categories.end()), categories.end());

    return categories;
}

std::vector<std::string> InsightModel::categories() const
{
    std::vector<std::string> categories;
    if (!m_qtdsConfig.empty()) {
        for (auto it : m_qtdsConfig) {
            if (it.contains(nameAtt))
                categories.push_back(it[nameAtt].get<std::string>());
        }
    }

    return categories;
}

bool InsightModel::hasCategory(const QString &name) const
{
    auto c = categories();
    return std::find(std::begin(c), std::end(c), name.toStdString()) != std::end(c);
}

void InsightModel::updateCheckState()
{
    auto active = activeCategories();
    auto predefined = predefinedCategories();
    auto custom = customCategories();

    Qt::CheckState predefinedCheckState = checkState(predefined, active);
    Qt::CheckState customCheckState = checkState(custom, active);

    if (m_predefinedCheckState != predefinedCheckState) {
        m_predefinedCheckState = predefinedCheckState;
        emit predefinedSelectStateChanged();
    }

    if (m_customCheckState != customCheckState) {
        m_customCheckState = customCheckState;
        emit customSelectStateChanged();
    }
}

template<typename T>
void InsightModel::writeConfigValue(const json::json_pointer &ptr, T value)
{
    T configValue{};

    if (!m_config.empty())
        configValue = m_config.value(ptr, configValue);

    if (configValue == value)
        return;

    json tmp = m_config;
    tmp[ptr] = value;

    writeJSON(m_configInfo.absoluteFilePath(), tmp);
}

} // namespace QmlDesigner
