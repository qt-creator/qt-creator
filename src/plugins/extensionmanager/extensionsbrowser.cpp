// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "extensionsbrowser.h"

#include "extensionmanagertr.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/welcomepagehelper.h>

#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginspec.h>
#include <extensionsystem/pluginview.h>
#include <extensionsystem/pluginmanager.h>

#include <utils/fancylineedit.h>
#include <utils/icon.h>
#include <utils/layoutbuilder.h>
#include <utils/stylehelper.h>

#include <QItemDelegate>
#include <QLabel>
#include <QListView>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QStandardItemModel>
#include <QStyle>

using namespace ExtensionSystem;
using namespace Core;
using namespace Utils;

namespace ExtensionManager::Internal {

Q_LOGGING_CATEGORY(browserLog, "qtc.extensionmanager.browser", QtWarningMsg)

using Tags = QStringList;

constexpr QSize itemSize = {330, 86};
constexpr int gapSize = StyleHelper::SpacingTokens::ExVPaddingGapXl;
constexpr QSize cellSize = {itemSize.width() + gapSize, itemSize.height() + gapSize};

enum Role {
    RoleName = Qt::UserRole,
    RoleItemType,
    RoleTags,
    RolePluginSpecs,
    RoleSearchText,
};

ItemData itemData(const QModelIndex &index)
{
    return {
        index.data(RoleName).toString(),
        index.data(RoleItemType).value<ItemType>(),
        index.data(RoleTags).toStringList(),
        index.data(RolePluginSpecs).value<PluginSpecList>(),
    };
}

static QColor colorForExtensionName(const QString &name)
{
    const size_t hash = qHash(name);
    return QColor::fromHsv(hash % 360, 180, 110);
}

static QStandardItemModel *extensionsModel()
{
    // The new extensions concept renames plugins to extensions and adds "packs" which are
    // groups of extensions.
    //
    // TODO: The "meta data" here which is injected into the model is only a place holder that
    // helps exploring the upcoming extensions concept.
    //
    // Before this loses the WIP prefix, we should at least have a concrete idea of how the data
    // is structured and where it lives. Ideally, it continues to reside exclusively in the
    // extension meta data.
    //
    // The grouping of extensions into packs could be done via extension tag. Extensions and will
    // receive tags and if possible screen shots.
    // Packs will also have a complete set of meta data. That could be an accumulation of the data
    // of the contained extensions. Or simply the data from the "first" extension in a pack.

    static const char tagBuildTools[] =             "Build Tools";
    static const char tagCodeAnalyzing[] =          "Code Analyzing";
    static const char tagConnectivity[] =           "Connectivity";
    static const char tagCore[] =                   "Core";
    static const char tagCpp[] =                    "C++";
    static const char tagEditorConvenience[] =      "Editor Convenience";
    static const char tagEditor[] =                 "Editor";
    static const char tagEssentials[] =             "Essentials";
    static const char tagGlsl[] =                   "GLSL";
    static const char tagPackageManager[] =         "Package Manager";
    static const char tagPlatformSupport[] =        "Platform Support";
    static const char tagProgrammingLanguage[] =    "Programming Language";
    static const char tagPython[] =                 "Python";
    static const char tagQml[] =                    "QML";
    static const char tagQuick[] =                  "Quick";
    static const char tagService[] =                "Service";
    static const char tagTestAutomation[] =         "Test Automation";
    static const char tagUiEditor[] =               "Visual UI Editor" ;
    static const char tagVersionControl[] =         "Version Control";
    static const char tagVisualEditor[] =           "Visual editor";
    static const char tagWidgets[] =                "Widgets";

    static const char tagTagUndefined[] =           "Tag undefined";

    static const struct {
        const QString name;
        const QStringList extensions;
        const Tags tags;
    } packs[] = {
        {"Core",
            {"Core", "Help", "ProjectExplorer", "TextEditor", "Welcome", "GenericProjectManager",
             "QtSupport"},
            {tagCore}
        },
        {"Core (from Installer)",
            {"LicenseChecker", "Marketplace", "UpdateInfo"},
            {tagCore}
        },
        {"Essentials",
            {"Bookmarks", "BinEditor", "Debugger", "DiffEditor", "ImageViewer", "Macros",
             "LanguageClient", "ResourceEditor"},
            {tagEssentials}
        },
        {"C++ Language support",
            {"ClangCodeModel", "ClangFormat", "ClassView", "CppEditor"},
            {tagProgrammingLanguage, tagCpp}
        },
        {"QML Language Support (Qt Quick libraries)",
            {"QmlJSEditor", "QmlJSTools", "QmlPreview", "QmlProfiler", "QmlProjectManager"},
            {tagProgrammingLanguage, tagQml}
        },
        {"Visual QML UI Editor",
            {"QmlDesigner", "QmlDesignerBase"},
            {tagUiEditor, tagQml, tagQuick}
        },
        {"Visual C++ Widgets UI Editor",
            {"Designer"},
            {tagUiEditor, tagCpp, tagWidgets}
        },
    };

    static const struct {
        const QString name;
        const Tags tags;
    } extensions[] = {
        {"GLSLEditor",                          {tagProgrammingLanguage, tagGlsl}},
        {"Nim",                                 {tagProgrammingLanguage}},
        {"Python",                              {tagProgrammingLanguage, tagPython}},
        {"Haskell",                             {tagProgrammingLanguage}},

        {"ModelEditor",                         {tagVisualEditor}},
        {"ScxmlEditor",                         {tagVisualEditor}},

        {"Bazaar",                              {tagVersionControl}},
        {"CVS",                                 {tagVersionControl}},
        {"ClearCase",                           {tagVersionControl}},
        {"Fossil",                              {tagVersionControl}},
        {"Git",                                 {tagVersionControl}},
        {"Mercurial",                           {tagVersionControl}},
        {"Perforce",                            {tagVersionControl}},
        {"Subversion",                          {tagVersionControl}},
        {"VcsBase",                             {tagVersionControl}},
        {"GitLab",                              {tagVersionControl, tagService}},

        {"AutoTest",                            {tagTestAutomation}},
        {"Squish",                              {tagTestAutomation}},
        {"Coco",                                {tagTestAutomation}},

        {"Vcpkg",                               {tagPackageManager}},
        {"Conan",                               {tagPackageManager}},

        {"Copilot",                             {tagEditorConvenience}},
        {"EmacsKeys",                           {tagEditorConvenience}},
        {"FakeVim",                             {tagEditorConvenience}},
        {"Terminal",                            {tagEditorConvenience}},
        {"Todo",                                {tagEditorConvenience}},
        {"CodePaster",                          {tagEditorConvenience}},
        {"Beautifier",                          {tagEditorConvenience}},

        {"SerialTerminal",                      {tagConnectivity}},

        {"SilverSearcher",                      {tagEditor}},

        {"AutotoolsProjectManager",             {tagBuildTools}},
        {"CMakeProjectManager",                 {tagBuildTools}},
        {"CompilationDatabaseProjectManager",   {tagBuildTools}},
        {"IncrediBuild",                        {tagBuildTools}},
        {"MesonProjectManager",                 {tagBuildTools}},
        {"QbsProjectManager",                   {tagBuildTools}},
        {"QmakeProjectManager",                 {tagBuildTools}},

        {"Axivion",                             {tagCodeAnalyzing}},
        {"ClangTools",                          {tagCodeAnalyzing}},
        {"Cppcheck",                            {tagCodeAnalyzing}},
        {"CtfVisualizer",                       {tagCodeAnalyzing}},
        {"PerfProfiler",                        {tagCodeAnalyzing}},
        {"Valgrind",                            {tagCodeAnalyzing}},

        {"Android",                             {tagPlatformSupport}},
        {"BareMetal",                           {tagPlatformSupport}},
        {"Boot2Qt",                             {tagPlatformSupport}},
        {"Ios",                                 {tagPlatformSupport}},
        {"McuSupport",                          {tagPlatformSupport}},
        {"Qnx",                                 {tagPlatformSupport}},
        {"RemoteLinux",                         {tagPlatformSupport}},
        {"SafeRenderer",                        {tagPlatformSupport}},
        {"VxWorks",                             {tagPlatformSupport}},
        {"WebAssembly",                         {tagPlatformSupport}},
        {"Docker",                              {tagPlatformSupport}},

        // Missing in Kimmo's excel sheet:
        {"CompilerExplorer",                    {tagTagUndefined}},
        {"ExtensionManager",                    {tagTagUndefined}},
        {"ScreenRecorder",                      {tagTagUndefined}},
    };

    QList<QStandardItem*> items;
    QStringList expectedExtensions;
    QStringList unexpectedExtensions;
    QHash<const QString, const PluginSpec*> installedPlugins;
    for (const PluginSpec *ps : PluginManager::plugins()) {
        installedPlugins.insert(ps->name(), ps);
        unexpectedExtensions.append(ps->name());
    }

    const auto handleExtension = [&] (const ItemData &extension, bool addToBrowser) {
        if (!installedPlugins.contains(extension.name)) {
            expectedExtensions.append(extension.name);
            return false;
        }
        unexpectedExtensions.removeOne(extension.name);

        if (addToBrowser) {
            QStandardItem *item = new QStandardItem;
            const PluginSpecList pluginSpecs = {installedPlugins.value(extension.name)};
            item->setData(ItemTypeExtension, RoleItemType);
            item->setData(QVariant::fromValue(extension.tags), RoleTags);
            item->setData(QVariant::fromValue<PluginSpecList>(pluginSpecs), RolePluginSpecs);
            item->setData(extension.name, RoleName);
            items.append(item);
        }

        return true;
    };

    const bool addPackedExtensionsToBrowser = true; // TODO: Determine how we want this. As setting?
    for (const auto &pack : packs) {
        PluginSpecList pluginSpecs;
        for (const QString &extension : pack.extensions) {
            const ItemData extensionData = {extension, {}, pack.tags, {}};
            if (!handleExtension(extensionData, addPackedExtensionsToBrowser))
                continue;
            pluginSpecs.append(installedPlugins.value(extension));
        }
        if (pluginSpecs.isEmpty())
            continue;

        QStandardItem *item = new QStandardItem;
        item->setData(ItemTypePack, RoleItemType);
        item->setData(QVariant::fromValue(pack.tags), RoleTags);
        item->setData(QVariant::fromValue<PluginSpecList>(pluginSpecs), RolePluginSpecs);
        item->setData(pack.name, RoleName);
        items.append(item);
    }

    for (const auto &extension : extensions) {
        const ItemData extensionData = {extension.name, {}, extension.tags, {}};
        handleExtension(extensionData, true);
    }

    QStandardItemModel *result = new QStandardItemModel;
    for (auto item : items) {
        QStringList searchTexts;
        searchTexts.append(item->data(RoleName).toString());
        searchTexts.append(item->data(RoleTags).toStringList());
        const PluginSpecList pluginSpecs = item->data(RolePluginSpecs).value<PluginSpecList>();
        for (auto pluginSpec : pluginSpecs) {
            searchTexts.append(pluginSpec->name());
            searchTexts.append(pluginSpec->description());
            searchTexts.append(pluginSpec->longDescription());
            searchTexts.append(pluginSpec->category());
            searchTexts.append(pluginSpec->copyright());
        }
        searchTexts.removeDuplicates();
        item->setData(searchTexts.join(" "), RoleSearchText);

        item->setDragEnabled(false);
        item->setEditable(false);

        result->appendRow(item);
    }

    if (browserLog().isDebugEnabled()) {
        if (!expectedExtensions.isEmpty())
            qCDebug(browserLog) << "Expected extensions/plugins are not installed:"
                              << expectedExtensions.join(", ");
        if (!unexpectedExtensions.isEmpty())
            qCDebug(browserLog) << "Unexpected extensions/plugins are installed:"
                                << unexpectedExtensions.join(", ");
    }

    return result;
}

class ExtensionItemDelegate : public QItemDelegate
{
public:
    explicit ExtensionItemDelegate(QObject *parent = nullptr)
        : QItemDelegate(parent)
    {
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index)
        const override
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        const ItemData data = itemData(index);
        const bool isPack = data.type == ItemTypePack;
        const QRectF itemRect(option.rect.topLeft(), itemSize);
        {
            const bool selected = option.state & QStyle::State_Selected;
            const bool hovered = option.state & QStyle::State_MouseOver;
            const QColor fillColor =
                creatorTheme()->color(hovered ? WelcomePageHelpers::cardHoverBackground
                                              : WelcomePageHelpers::cardDefaultBackground);
            const QColor strokeColor =
                creatorTheme()->color(selected ? Theme::Token_Stroke_Strong
                                      : hovered ? WelcomePageHelpers::cardHoverStroke
                                                : WelcomePageHelpers::cardDefaultStroke);
            WelcomePageHelpers::drawCardBackground(painter, itemRect, fillColor, strokeColor);
        }
        {
            constexpr QRectF bigCircle(16, 16, 48, 48);
            constexpr double gradientMargin = 0.14645;
            const QRectF bigCircleLocal = bigCircle.translated(itemRect.topLeft());
            QPainterPath bigCirclePath;
            bigCirclePath.addEllipse(bigCircleLocal);
            QLinearGradient gradient(bigCircleLocal.topLeft(), bigCircleLocal.bottomRight());
            const QColor startColor = isPack ? qRgb(0x1e, 0x99, 0x6e)
                                             : colorForExtensionName(data.name);
            const QColor endColor = isPack ? qRgb(0x07, 0x6b, 0x6d) : startColor.lighter(150);
            gradient.setColorAt(gradientMargin, startColor);
            gradient.setColorAt(1 - gradientMargin, endColor);
            painter->fillPath(bigCirclePath, gradient);

            static const QIcon packIcon =
                Icon({{":/extensionmanager/images/packsmall.png",
                       Theme::Token_Text_Default}}, Icon::Tint).icon();
            static const QIcon extensionIcon =
                Icon({{":/extensionmanager/images/extensionsmall.png",
                       Theme::Token_Text_Default}}, Icon::Tint).icon();
            QRectF iconRect(0, 0, 32, 32);
            iconRect.moveCenter(bigCircleLocal.center());
            (isPack ? packIcon : extensionIcon).paint(painter, iconRect.toRect());
        }
        if (isPack) {
            constexpr QRectF smallCircle(47, 50, 18, 18);
            constexpr qreal strokeWidth = 1;
            constexpr qreal shrink = strokeWidth / 2;
            constexpr QRectF smallCircleAdjusted = smallCircle.adjusted(shrink, shrink,
                                                                        -shrink, -shrink);
            const QRectF smallCircleLocal = smallCircleAdjusted.translated(itemRect.topLeft());
            const QColor fillColor = creatorTheme()->color(Theme::Token_Foreground_Muted);
            const QColor strokeColor = creatorTheme()->color(Theme::Token_Stroke_Subtle);
            painter->setBrush(fillColor);
            painter->setPen(strokeColor);
            painter->drawEllipse(smallCircleLocal);

            painter->setFont(StyleHelper::uiFont(StyleHelper::UiElementCaptionStrong));
            const QColor textColor = creatorTheme()->color(Theme::Token_Text_Default);
            painter->setPen(textColor);
            painter->drawText(smallCircleLocal, QString::number(data.plugins.count()),
                              QTextOption(Qt::AlignCenter));
        }
        {
            constexpr int textX = 80;
            constexpr int rightMargin = StyleHelper::SpacingTokens::ExVPaddingGapXl;
            constexpr int maxTextWidth = itemSize.width() - textX - rightMargin;
            constexpr Qt::TextElideMode elideMode = Qt::ElideRight;

            constexpr int titleY = 30;
            const QPointF titleOrigin(itemRect.topLeft() + QPointF(textX, titleY));
            painter->setPen(creatorTheme()->color(Theme::Token_Text_Default));
            painter->setFont(StyleHelper::uiFont(StyleHelper::UiElementH6));
            const QString titleElided = painter->fontMetrics().elidedText(
                data.name, elideMode, maxTextWidth);
            painter->drawText(titleOrigin, titleElided);

            constexpr int copyrightY = 52;
            const QPointF copyrightOrigin(itemRect.topLeft() + QPointF(textX, copyrightY));
            painter->setPen(creatorTheme()->color(Theme::Token_Text_Muted));
            painter->setFont(StyleHelper::uiFont(StyleHelper::UiElementCaptionStrong));
            const QString copyrightElided = painter->fontMetrics().elidedText(
                data.plugins.first()->copyright(), elideMode, maxTextWidth);
            painter->drawText(copyrightOrigin, copyrightElided);

            constexpr int tagsY = 70;
            const QPointF tagsOrigin(itemRect.topLeft() + QPointF(textX, tagsY));
            const QString tags = data.tags.join(", ");
            painter->setPen(creatorTheme()->color(Theme::Token_Text_Default));
            painter->setFont(StyleHelper::uiFont(StyleHelper::UiElementCaption));
            const QString tagsElided = painter->fontMetrics().elidedText(
                tags, elideMode, maxTextWidth);
            painter->drawText(tagsOrigin, tagsElided);
        }

        painter->restore();
    }

    QSize sizeHint([[maybe_unused]] const QStyleOptionViewItem &option,
                   [[maybe_unused]] const QModelIndex &index) const override
    {
        return cellSize;
    }
};

ExtensionsBrowser::ExtensionsBrowser()
{
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

    auto manageLabel = new QLabel(Tr::tr("Manage Extensions"));
    manageLabel->setFont(StyleHelper::uiFont(StyleHelper::UiElementH1));

    m_searchBox = new Core::SearchBox;
    m_searchBox->setFixedWidth(itemSize.width());

    m_updateButton = new Button(Tr::tr("Install..."), Button::MediumPrimary);

    m_filterProxyModel = new QSortFilterProxyModel(this);
    m_filterProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_filterProxyModel->setFilterRole(RoleSearchText);
    m_filterProxyModel->setSortRole(RoleItemType);

    m_extensionsView = new QListView;
    m_extensionsView->setFrameStyle(QFrame::NoFrame);
    m_extensionsView->setItemDelegate(new ExtensionItemDelegate(this));
    m_extensionsView->setResizeMode(QListView::Adjust);
    m_extensionsView->setSelectionMode(QListView::SingleSelection);
    m_extensionsView->setUniformItemSizes(true);
    m_extensionsView->setViewMode(QListView::IconMode);
    m_extensionsView->setModel(m_filterProxyModel);
    m_extensionsView->setMouseTracking(true);

    using namespace Layouting;
    Column {
        Space(15),
        manageLabel,
        Space(15),
        Row { m_searchBox, st, m_updateButton, Space(extraListViewWidth() + gapSize) },
        Space(gapSize),
        m_extensionsView,
        noMargin(), spacing(0),
    }.attachTo(this);

    WelcomePageHelpers::setBackgroundColor(this, Theme::Token_Background_Default);
    WelcomePageHelpers::setBackgroundColor(m_extensionsView, Theme::Token_Background_Default);
    WelcomePageHelpers::setBackgroundColor(m_extensionsView->viewport(),
                                           Theme::Token_Background_Default);

    auto updateModel = [this] {
        m_model.reset(extensionsModel());
        m_filterProxyModel->setSourceModel(m_model.data());
        m_filterProxyModel->sort(0);

        if (m_selectionModel == nullptr) {
            m_selectionModel = new QItemSelectionModel(m_filterProxyModel, m_extensionsView);
            m_extensionsView->setSelectionModel(m_selectionModel);
            connect(m_extensionsView->selectionModel(), &QItemSelectionModel::currentChanged,
                    this, &ExtensionsBrowser::itemSelected);
        }
    };

    connect(ExtensionSystem::PluginManager::instance(),
            &ExtensionSystem::PluginManager::pluginsChanged, this, updateModel);
    connect(m_searchBox, &QLineEdit::textChanged,
            m_filterProxyModel, &QSortFilterProxyModel::setFilterWildcard);
}

void ExtensionsBrowser::adjustToWidth(const int width)
{
    const int widthForItems = width - extraListViewWidth();
    m_columnsCount = qMax(1, qFloor(widthForItems / cellSize.width()));
    m_updateButton->setVisible(m_columnsCount > 1);
    updateGeometry();
}

QSize ExtensionsBrowser::sizeHint() const
{
    const int columsWidth = m_columnsCount * cellSize.width();
    return { columsWidth + extraListViewWidth(), 0};
}

int ExtensionsBrowser::extraListViewWidth() const
{
    // TODO: Investigate "transient" scrollbar, just for this list view.
    return m_extensionsView->style()->pixelMetric(QStyle::PM_ScrollBarExtent)
           + 1; // Needed
}

} // ExtensionManager::Internal
