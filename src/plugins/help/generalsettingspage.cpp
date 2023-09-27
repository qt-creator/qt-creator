// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "generalsettingspage.h"

#include "helpconstants.h"
#include "helpplugin.h"
#include "helptr.h"
#include "helpviewer.h"
#include "helpwidget.h"
#include "localhelpmanager.h"
#include "xbelsupport.h"

#include <bookmarkmanager.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/helpmanager.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/layoutbuilder.h>

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QFontComboBox>
#include <QFontDatabase>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTextStream>
#include <QVBoxLayout>

using namespace Core;
using namespace Utils;

namespace Help::Internal {

class GeneralSettingsPageWidget : public IOptionsPageWidget
{
public:
    GeneralSettingsPageWidget();

private:
    void apply() final;

    void setCurrentPage();
    void setBlankPage();
    void setDefaultPage();
    void importBookmarks();
    void exportBookmarks();

    void updateFontSizeSelector();
    void updateFontStyleSelector();
    void updateFontFamilySelector();
    void updateFont();
    int closestPointSizeIndex(int desiredPointSize) const;

    QFont m_font;
    int m_fontZoom = 100;
    QFontDatabase m_fontDatabase;

    QString m_homePage;
    int m_contextOption;

    int m_startOption;
    bool m_returnOnClose;
    bool m_scrollWheelZoomingEnabled;

    QSpinBox *zoomSpinBox;
    QFontComboBox *familyComboBox;
    QComboBox *styleComboBox;
    QComboBox *sizeComboBox;
    QCheckBox *antialiasCheckBox;
    QLineEdit *homePageLineEdit;
    QComboBox *helpStartComboBox;
    QComboBox *contextHelpComboBox;
    QPushButton *currentPageButton;
    QPushButton *blankPageButton;
    QPushButton *defaultPageButton;
    QLabel *errorLabel;
    QPushButton *importButton;
    QPushButton *exportButton;
    QCheckBox *scrollWheelZooming;
    QCheckBox *m_returnOnCloseCheckBox;
    QComboBox *viewerBackend;
};

GeneralSettingsPageWidget::GeneralSettingsPageWidget()
{
    using namespace Layouting;

    // font group box
    familyComboBox = new QFontComboBox;
    styleComboBox = new QComboBox;
    sizeComboBox = new QComboBox;
    zoomSpinBox = new QSpinBox;
    zoomSpinBox->setMinimum(10);
    zoomSpinBox->setMaximum(3000);
    zoomSpinBox->setSingleStep(10);
    zoomSpinBox->setValue(100);
    zoomSpinBox->setSuffix(Tr::tr("%"));
    antialiasCheckBox = new QCheckBox(Tr::tr("Antialias"));

    auto fontGroupBox = new QGroupBox(Tr::tr("Font"));
    // clang-format off
    Column {
        Row { Tr::tr("Family:"), familyComboBox,
              Tr::tr("Style:"), styleComboBox,
              Tr::tr("Size:"), sizeComboBox, st },
        Row { Tr::tr("Note: The above setting takes effect only if the "
                     "HTML file does not use a style sheet.") },
        Row { Tr::tr("Zoom:"), zoomSpinBox, antialiasCheckBox, st }
    }.attachTo(fontGroupBox);
    // clang-format on

    // startup group box
    auto startupGroupBox = new QGroupBox(Tr::tr("Startup"));
    startupGroupBox->setObjectName("startupGroupBox");

    contextHelpComboBox = new QComboBox(startupGroupBox);
    contextHelpComboBox->setObjectName("contextHelpComboBox");
    contextHelpComboBox->addItem(Tr::tr("Show Side-by-Side if Possible"));
    contextHelpComboBox->addItem(Tr::tr("Always Show Side-by-Side"));
    contextHelpComboBox->addItem(Tr::tr("Always Show in Help Mode"));
    contextHelpComboBox->addItem(Tr::tr("Always Show in External Window"));

    helpStartComboBox = new QComboBox(startupGroupBox);
    helpStartComboBox->addItem(Tr::tr("Show My Home Page"));
    helpStartComboBox->addItem(Tr::tr("Show a Blank Page"));
    helpStartComboBox->addItem(Tr::tr("Show My Tabs from Last Session"));

    auto startupFormLayout = new QFormLayout;
    startupGroupBox->setLayout(startupFormLayout);
    startupFormLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    startupFormLayout->addRow(Tr::tr("On context help:"), contextHelpComboBox);
    startupFormLayout->addRow(Tr::tr("On help start:"), helpStartComboBox);

    homePageLineEdit = new QLineEdit;
    currentPageButton = new QPushButton(Tr::tr("Use &Current Page"));
    blankPageButton = new QPushButton(Tr::tr("Use &Blank Page"));
    defaultPageButton = new QPushButton(Tr::tr("Reset"));
    defaultPageButton->setToolTip(Tr::tr("Reset to default."));

    auto homePageLayout = new QHBoxLayout;
    homePageLayout->addWidget(homePageLineEdit);
    homePageLayout->addWidget(currentPageButton);
    homePageLayout->addWidget(blankPageButton);
    homePageLayout->addWidget(defaultPageButton);

    startupFormLayout->addRow(Tr::tr("Home page:"), homePageLayout);

    // behavior group box
    auto behaviourGroupBox = new QGroupBox(Tr::tr("Behaviour"));
    scrollWheelZooming = new QCheckBox(Tr::tr("Enable scroll wheel zooming"));

    m_returnOnCloseCheckBox = new QCheckBox(Tr::tr("Return to editor on closing the last page"));
    m_returnOnCloseCheckBox->setToolTip(
        Tr::tr("Switches to editor context after last help page is closed."));

    auto viewerBackendLabel = new QLabel(Tr::tr("Viewer backend:"));
    viewerBackend = new QComboBox;
    const QString description = Tr::tr("Change takes effect after reloading help pages.");
    auto viewerBackendDescription = new QLabel(description);
    viewerBackendLabel->setToolTip(description);
    viewerBackend->setToolTip(description);

    auto viewerBackendLayout = new QHBoxLayout();
    viewerBackendLayout->addWidget(viewerBackendLabel);
    viewerBackendLayout->addWidget(viewerBackend);
    viewerBackendLayout->addWidget(viewerBackendDescription);
    viewerBackendLayout->addStretch();

    auto behaviourGroupBoxLayout = new QVBoxLayout;
    behaviourGroupBox->setLayout(behaviourGroupBoxLayout);
    behaviourGroupBoxLayout->addWidget(scrollWheelZooming);
    behaviourGroupBoxLayout->addWidget(m_returnOnCloseCheckBox);
    behaviourGroupBoxLayout->addLayout(viewerBackendLayout);

    // bookmarks
    errorLabel = new QLabel(this);
    QPalette palette;
    QBrush brush(QColor(255, 0, 0, 255));
    brush.setStyle(Qt::SolidPattern);
    palette.setBrush(QPalette::Active, QPalette::Text, brush);
    palette.setBrush(QPalette::Inactive, QPalette::Text, brush);
    QBrush brush1(QColor(120, 120, 120, 255));
    brush1.setStyle(Qt::SolidPattern);
    palette.setBrush(QPalette::Disabled, QPalette::Text, brush1);
    errorLabel->setPalette(palette);

    importButton = new QPushButton(Tr::tr("Import Bookmarks..."));
    exportButton = new QPushButton(Tr::tr("Export Bookmarks..."));

    auto bookmarksLayout = new QHBoxLayout();
    bookmarksLayout->addStretch();
    bookmarksLayout->addWidget(errorLabel);
    bookmarksLayout->addWidget(importButton);
    bookmarksLayout->addWidget(exportButton);

    auto mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    mainLayout->addWidget(fontGroupBox);
    mainLayout->addWidget(startupGroupBox);
    mainLayout->addWidget(behaviourGroupBox);
    mainLayout->addLayout(bookmarksLayout);
    mainLayout->addStretch(1);

    m_font = LocalHelpManager::fallbackFont();
    m_fontZoom = LocalHelpManager::fontZoom();
    zoomSpinBox->setValue(m_fontZoom);
    antialiasCheckBox->setChecked(LocalHelpManager::antialias());

    updateFontSizeSelector();
    updateFontStyleSelector();
    updateFontFamilySelector();

    connect(familyComboBox, &QFontComboBox::currentFontChanged, this, [this] {
        updateFont();
        updateFontStyleSelector();
        updateFontSizeSelector();
        updateFont(); // changes that might have happened when updating the selectors
    });

    connect(styleComboBox, &QComboBox::currentIndexChanged, this, [this] {
        updateFont();
        updateFontSizeSelector();
        updateFont(); // changes that might have happened when updating the selectors
    });

    connect(sizeComboBox, &QComboBox::currentIndexChanged,
            this, &GeneralSettingsPageWidget::updateFont);

    connect(zoomSpinBox, &QSpinBox::valueChanged,
            this, [this](int value) { m_fontZoom = value; });

    m_homePage = LocalHelpManager::homePage();
    homePageLineEdit->setText(m_homePage);

    m_startOption = LocalHelpManager::startOption();
    helpStartComboBox->setCurrentIndex(m_startOption);

    m_contextOption = LocalHelpManager::contextHelpOption();
    contextHelpComboBox->setCurrentIndex(m_contextOption);

    connect(currentPageButton, &QPushButton::clicked,
            this, &GeneralSettingsPageWidget::setCurrentPage);
    connect(blankPageButton, &QPushButton::clicked,
            this, &GeneralSettingsPageWidget::setBlankPage);
    connect(defaultPageButton, &QPushButton::clicked,
            this, &GeneralSettingsPageWidget::setDefaultPage);

    HelpViewer *viewer = HelpPlugin::modeHelpWidget()->currentViewer();
    if (!viewer)
        currentPageButton->setEnabled(false);

    errorLabel->setVisible(false);
    connect(importButton, &QPushButton::clicked,
            this, &GeneralSettingsPageWidget::importBookmarks);
    connect(exportButton, &QPushButton::clicked,
            this, &GeneralSettingsPageWidget::exportBookmarks);

    m_returnOnClose = LocalHelpManager::returnOnClose();
    m_returnOnCloseCheckBox->setChecked(m_returnOnClose);

    m_scrollWheelZoomingEnabled = LocalHelpManager::isScrollWheelZoomingEnabled();
    scrollWheelZooming->setChecked(m_scrollWheelZoomingEnabled);

    viewerBackend->addItem(
        Tr::tr("Default (%1)", "Default viewer backend")
            .arg(LocalHelpManager::defaultViewerBackend().displayName));
    const QByteArray currentBackend = LocalHelpManager::viewerBackendId();
    const QVector<HelpViewerFactory> backends = LocalHelpManager::viewerBackends();
    for (const HelpViewerFactory &f : backends) {
        viewerBackend->addItem(f.displayName, f.id);
        if (f.id == currentBackend)
            viewerBackend->setCurrentIndex(viewerBackend->count() - 1);
    }
    if (backends.size() == 1)
        viewerBackend->setEnabled(false);
}

void GeneralSettingsPageWidget::apply()
{
    if (m_font != LocalHelpManager::fallbackFont())
        LocalHelpManager::setFallbackFont(m_font);

    if (m_fontZoom != LocalHelpManager::fontZoom())
        LocalHelpManager::setFontZoom(m_fontZoom);

    LocalHelpManager::setAntialias(antialiasCheckBox->isChecked());

    QString homePage = QUrl::fromUserInput(homePageLineEdit->text()).toString();
    if (homePage.isEmpty())
        homePage = Help::Constants::AboutBlank;
    homePageLineEdit->setText(homePage);
    if (m_homePage != homePage) {
        m_homePage = homePage;
        LocalHelpManager::setHomePage(homePage);
    }

    const int startOption = helpStartComboBox->currentIndex();
    if (m_startOption != startOption) {
        m_startOption = startOption;
        LocalHelpManager::setStartOption(LocalHelpManager::StartOption(m_startOption));
    }

    const int helpOption = contextHelpComboBox->currentIndex();
    if (m_contextOption != helpOption) {
        m_contextOption = helpOption;
        LocalHelpManager::setContextHelpOption(HelpManager::HelpViewerLocation(m_contextOption));
    }

    const bool close = m_returnOnCloseCheckBox->isChecked();
    if (m_returnOnClose != close) {
        m_returnOnClose = close;
        LocalHelpManager::setReturnOnClose(m_returnOnClose);
    }

    const bool zoom = scrollWheelZooming->isChecked();
    if (m_scrollWheelZoomingEnabled != zoom) {
        m_scrollWheelZoomingEnabled = zoom;
        LocalHelpManager::setScrollWheelZoomingEnabled(m_scrollWheelZoomingEnabled);
    }

    const QByteArray viewerBackendId = viewerBackend->currentData().toByteArray();
    LocalHelpManager::setViewerBackendId(viewerBackendId);
}

void GeneralSettingsPageWidget::setCurrentPage()
{
    HelpViewer *viewer = HelpPlugin::modeHelpWidget()->currentViewer();
    if (viewer)
        homePageLineEdit->setText(viewer->source().toString());
}

void GeneralSettingsPageWidget::setBlankPage()
{
    homePageLineEdit->setText(Help::Constants::AboutBlank);
}

void GeneralSettingsPageWidget::setDefaultPage()
{
    homePageLineEdit->setText(LocalHelpManager::defaultHomePage());
}

void GeneralSettingsPageWidget::importBookmarks()
{
    errorLabel->setVisible(false);

    FilePath filePath = FileUtils::getOpenFilePath(nullptr,
                                                   Tr::tr("Import Bookmarks"),
                                                   FilePath::fromString(QDir::currentPath()),
                                                   Tr::tr("Files (*.xbel)"));

    if (filePath.isEmpty())
        return;

    QFile file(filePath.toString());
    if (file.open(QIODevice::ReadOnly)) {
        const BookmarkManager &manager = LocalHelpManager::bookmarkManager();
        XbelReader reader(manager.treeBookmarkModel(), manager.listBookmarkModel());
        if (reader.readFromFile(&file))
            return;
    }

    errorLabel->setVisible(true);
    errorLabel->setText(Tr::tr("Cannot import bookmarks."));
}

void GeneralSettingsPageWidget::exportBookmarks()
{
    errorLabel->setVisible(false);

    FilePath filePath = FileUtils::getSaveFilePath(nullptr,
                                                   Tr::tr("Save File"),
                                                   "untitled.xbel",
                                                   Tr::tr("Files (*.xbel)"));

    QLatin1String suffix(".xbel");
    if (!filePath.endsWith(suffix))
        filePath = filePath.stringAppended(suffix);

    FileSaver saver(filePath);
    if (!saver.hasError()) {
        XbelWriter writer(LocalHelpManager::bookmarkManager().treeBookmarkModel());
        writer.writeToFile(saver.file());
        saver.setResult(&writer);
    }
    if (!saver.finalize()) {
        errorLabel->setVisible(true);
        errorLabel->setText(saver.errorString());
    }
}

void GeneralSettingsPageWidget::updateFontSizeSelector()
{
    const QString &family = m_font.family();
    const QString &fontStyle = m_fontDatabase.styleString(m_font);

    QList<int> pointSizes = m_fontDatabase.pointSizes(family, fontStyle);
    if (pointSizes.empty())
        pointSizes = QFontDatabase::standardSizes();

    QSignalBlocker blocker(sizeComboBox);
    sizeComboBox->clear();
    sizeComboBox->setCurrentIndex(-1);
    sizeComboBox->setEnabled(!pointSizes.empty());

    //  try to maintain selection or select closest.
    if (!pointSizes.empty()) {
        QString n;
        for (int pointSize : std::as_const(pointSizes))
            sizeComboBox->addItem(n.setNum(pointSize), QVariant(pointSize));
        const int closestIndex = closestPointSizeIndex(m_font.pointSize());
        if (closestIndex != -1)
            sizeComboBox->setCurrentIndex(closestIndex);
    }
}

void GeneralSettingsPageWidget::updateFontStyleSelector()
{
    const QString &fontStyle = m_fontDatabase.styleString(m_font);
    const QStringList &styles = m_fontDatabase.styles(m_font.family());

    QSignalBlocker blocker(styleComboBox);
    styleComboBox->clear();
    styleComboBox->setCurrentIndex(-1);
    styleComboBox->setEnabled(!styles.empty());

    if (!styles.empty()) {
        int normalIndex = -1;
        const QString normalStyle = "Normal";
        for (const QString &style : styles) {
            // try to maintain selection or select 'normal' preferably
            const int newIndex = styleComboBox->count();
            styleComboBox->addItem(style);
            if (fontStyle == style) {
                styleComboBox->setCurrentIndex(newIndex);
            } else {
                if (fontStyle ==  normalStyle)
                    normalIndex = newIndex;
            }
        }
        if (styleComboBox->currentIndex() == -1 && normalIndex != -1)
            styleComboBox->setCurrentIndex(normalIndex);
    }
}

void GeneralSettingsPageWidget::updateFontFamilySelector()
{
    familyComboBox->setCurrentFont(m_font);
}

void GeneralSettingsPageWidget::updateFont()
{
    const QString &family = familyComboBox->currentFont().family();
    m_font.setFamily(family);

    int fontSize = 14;
    int currentIndex = sizeComboBox->currentIndex();
    if (currentIndex != -1)
        fontSize = sizeComboBox->itemData(currentIndex).toInt();
    m_font.setPointSize(fontSize);

    currentIndex = styleComboBox->currentIndex();
    if (currentIndex != -1)
        m_font.setStyleName(styleComboBox->itemText(currentIndex));
}

int GeneralSettingsPageWidget::closestPointSizeIndex(int desiredPointSize) const
{
    //  try to maintain selection or select closest.
    int closestIndex = -1;
    int closestAbsError = 0xFFFF;

    const int pointSizeCount = sizeComboBox->count();
    for (int i = 0; i < pointSizeCount; i++) {
        const int itemPointSize = sizeComboBox->itemData(i).toInt();
        const int absError = qAbs(desiredPointSize - itemPointSize);
        if (absError < closestAbsError) {
            closestIndex  = i;
            closestAbsError = absError;
            if (closestAbsError == 0)
                break;
        } else {    // past optimum
            if (absError > closestAbsError)
                break;
        }
    }
    return closestIndex;
}

// GeneralSettingPage

GeneralSettingsPage::GeneralSettingsPage()
{
    setId("A.General settings");
    setDisplayName(Tr::tr("General"));
    setCategory(Help::Constants::HELP_CATEGORY);
    setDisplayCategory(Tr::tr("Help"));
    setCategoryIconPath(":/help/images/settingscategory_help.png");
    setWidgetCreator([] { return new GeneralSettingsPageWidget; });
}

} // Help::Interal
