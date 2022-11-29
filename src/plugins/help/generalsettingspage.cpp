// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

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

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QFontComboBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QTextStream>
#include <QVBoxLayout>

#include <QApplication>

using namespace Core;
using namespace Utils;

namespace Help {
namespace Internal {

class GeneralSettingsPageWidget : public QWidget
{
public:
    GeneralSettingsPageWidget();

    QSpinBox *zoomSpinBox;
    QFontComboBox *familyComboBox;
    QComboBox *styleComboBox;
    QComboBox *sizeComboBox;
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
    QCheckBox *m_returnOnClose;
    QComboBox *viewerBackend;
};

GeneralSettingsPageWidget::GeneralSettingsPageWidget()
{
    // font group box
    auto fontGroupBox = new QGroupBox(Tr::tr("Font"));
    auto familyLabel = new QLabel(Tr::tr("Family:"));

    familyComboBox = new QFontComboBox;
    auto styleLabel = new QLabel(Tr::tr("Style:"));
    styleComboBox = new QComboBox;
    auto sizeLabel = new QLabel(Tr::tr("Size:"));
    sizeComboBox = new QComboBox;

    auto fontLayout = new QHBoxLayout();
    fontLayout->addWidget(familyComboBox);
    fontLayout->addSpacing(20);
    fontLayout->addWidget(styleLabel);
    fontLayout->addWidget(styleComboBox);
    fontLayout->addSpacing(20);
    fontLayout->addWidget(sizeLabel);
    fontLayout->addWidget(sizeComboBox);
    fontLayout->addStretch();

    auto noteLabel = new QLabel(Tr::tr(
        "Note: The above setting takes effect only if the HTML file does not use a style sheet."));
    noteLabel->setWordWrap(true);
    auto zoomLabel = new QLabel(Tr::tr("Zoom:"));

    zoomSpinBox = new QSpinBox;
    zoomSpinBox->setMinimum(10);
    zoomSpinBox->setMaximum(3000);
    zoomSpinBox->setSingleStep(10);
    zoomSpinBox->setValue(100);
    zoomSpinBox->setSuffix(Tr::tr("%"));

    auto zoomLayout = new QHBoxLayout();
    zoomLayout->addWidget(zoomSpinBox);
    zoomLayout->addStretch();

    auto fontGroupBoxLayout = new QGridLayout;
    fontGroupBox->setLayout(fontGroupBoxLayout);
    fontGroupBoxLayout->addWidget(familyLabel, 0, 0);
    fontGroupBoxLayout->addLayout(fontLayout, 0, 1);
    fontGroupBoxLayout->addWidget(noteLabel, 1, 0, 1, 2);
    fontGroupBoxLayout->addWidget(zoomLabel, 2, 0);
    fontGroupBoxLayout->addLayout(zoomLayout, 2, 1);

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

    m_returnOnClose = new QCheckBox(Tr::tr("Return to editor on closing the last page"));
    m_returnOnClose->setToolTip(
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
    behaviourGroupBoxLayout->addWidget(m_returnOnClose);
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
}

GeneralSettingsPage::GeneralSettingsPage()
{
    setId("A.General settings");
    setDisplayName(Tr::tr("General"));
    setCategory(Help::Constants::HELP_CATEGORY);
    setDisplayCategory(Tr::tr("Help"));
    setCategoryIconPath(":/help/images/settingscategory_help.png");
}

QWidget *GeneralSettingsPage::widget()
{
    if (!m_widget) {
        m_widget = new GeneralSettingsPageWidget;

        m_font = LocalHelpManager::fallbackFont();
        m_fontZoom = LocalHelpManager::fontZoom();
        m_widget->zoomSpinBox->setValue(m_fontZoom);

        updateFontSizeSelector();
        updateFontStyleSelector();
        updateFontFamilySelector();

        connect(m_widget->familyComboBox, &QFontComboBox::currentFontChanged, this, [this] {
            updateFont();
            updateFontStyleSelector();
            updateFontSizeSelector();
            updateFont(); // changes that might have happened when updating the selectors
        });

        connect(m_widget->styleComboBox, &QComboBox::currentIndexChanged, this, [this] {
            updateFont();
            updateFontSizeSelector();
            updateFont(); // changes that might have happened when updating the selectors
        });

        connect(m_widget->sizeComboBox, &QComboBox::currentIndexChanged,
                this, &GeneralSettingsPage::updateFont);

        connect(m_widget->zoomSpinBox, &QSpinBox::valueChanged,
                this, [this](int value) { m_fontZoom = value; });

        m_homePage = LocalHelpManager::homePage();
        m_widget->homePageLineEdit->setText(m_homePage);

        m_startOption = LocalHelpManager::startOption();
        m_widget->helpStartComboBox->setCurrentIndex(m_startOption);

        m_contextOption = LocalHelpManager::contextHelpOption();
        m_widget->contextHelpComboBox->setCurrentIndex(m_contextOption);

        connect(m_widget->currentPageButton, &QPushButton::clicked,
                this, &GeneralSettingsPage::setCurrentPage);
        connect(m_widget->blankPageButton, &QPushButton::clicked,
                this, &GeneralSettingsPage::setBlankPage);
        connect(m_widget->defaultPageButton,
                &QPushButton::clicked,
                this,
                &GeneralSettingsPage::setDefaultPage);

        HelpViewer *viewer = HelpPlugin::modeHelpWidget()->currentViewer();
        if (!viewer)
            m_widget->currentPageButton->setEnabled(false);

        m_widget->errorLabel->setVisible(false);
        connect(m_widget->importButton, &QPushButton::clicked,
                this, &GeneralSettingsPage::importBookmarks);
        connect(m_widget->exportButton, &QPushButton::clicked,
                this, &GeneralSettingsPage::exportBookmarks);

        m_returnOnClose = LocalHelpManager::returnOnClose();
        m_widget->m_returnOnClose->setChecked(m_returnOnClose);

        m_scrollWheelZoomingEnabled = LocalHelpManager::isScrollWheelZoomingEnabled();
        m_widget->scrollWheelZooming->setChecked(m_scrollWheelZoomingEnabled);

        m_widget->viewerBackend->addItem(
            Tr::tr("Default (%1)", "Default viewer backend")
                .arg(LocalHelpManager::defaultViewerBackend().displayName));
        const QByteArray currentBackend = LocalHelpManager::viewerBackendId();
        const QVector<HelpViewerFactory> backends = LocalHelpManager::viewerBackends();
        for (const HelpViewerFactory &f : backends) {
            m_widget->viewerBackend->addItem(f.displayName, f.id);
            if (f.id == currentBackend)
                m_widget->viewerBackend->setCurrentIndex(m_widget->viewerBackend->count() - 1);
        }
        if (backends.size() == 1)
            m_widget->viewerBackend->setEnabled(false);
    }
    return m_widget;
}

void GeneralSettingsPage::apply()
{
    if (!m_widget) // page was never shown
        return;

    if (m_font != LocalHelpManager::fallbackFont())
        LocalHelpManager::setFallbackFont(m_font);

    if (m_fontZoom != LocalHelpManager::fontZoom())
        LocalHelpManager::setFontZoom(m_fontZoom);

    QString homePage = QUrl::fromUserInput(m_widget->homePageLineEdit->text()).toString();
    if (homePage.isEmpty())
        homePage = Help::Constants::AboutBlank;
    m_widget->homePageLineEdit->setText(homePage);
    if (m_homePage != homePage) {
        m_homePage = homePage;
        LocalHelpManager::setHomePage(homePage);
    }

    const int startOption = m_widget->helpStartComboBox->currentIndex();
    if (m_startOption != startOption) {
        m_startOption = startOption;
        LocalHelpManager::setStartOption(LocalHelpManager::StartOption(m_startOption));
    }

    const int helpOption = m_widget->contextHelpComboBox->currentIndex();
    if (m_contextOption != helpOption) {
        m_contextOption = helpOption;
        LocalHelpManager::setContextHelpOption(HelpManager::HelpViewerLocation(m_contextOption));
    }

    const bool close = m_widget->m_returnOnClose->isChecked();
    if (m_returnOnClose != close) {
        m_returnOnClose = close;
        LocalHelpManager::setReturnOnClose(m_returnOnClose);
    }

    const bool zoom = m_widget->scrollWheelZooming->isChecked();
    if (m_scrollWheelZoomingEnabled != zoom) {
        m_scrollWheelZoomingEnabled = zoom;
        LocalHelpManager::setScrollWheelZoomingEnabled(m_scrollWheelZoomingEnabled);
    }

    const QByteArray viewerBackendId = m_widget->viewerBackend->currentData().toByteArray();
    LocalHelpManager::setViewerBackendId(viewerBackendId);
}

void GeneralSettingsPage::setCurrentPage()
{
    HelpViewer *viewer = HelpPlugin::modeHelpWidget()->currentViewer();
    if (viewer)
        m_widget->homePageLineEdit->setText(viewer->source().toString());
}

void GeneralSettingsPage::setBlankPage()
{
    m_widget->homePageLineEdit->setText(Help::Constants::AboutBlank);
}

void GeneralSettingsPage::setDefaultPage()
{
    m_widget->homePageLineEdit->setText(LocalHelpManager::defaultHomePage());
}

void GeneralSettingsPage::importBookmarks()
{
    m_widget->errorLabel->setVisible(false);

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

    m_widget->errorLabel->setVisible(true);
    m_widget->errorLabel->setText(Tr::tr("Cannot import bookmarks."));
}

void GeneralSettingsPage::exportBookmarks()
{
    m_widget->errorLabel->setVisible(false);

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
        m_widget->errorLabel->setVisible(true);
        m_widget->errorLabel->setText(saver.errorString());
    }
}

void GeneralSettingsPage::updateFontSizeSelector()
{
    const QString &family = m_font.family();
    const QString &fontStyle = m_fontDatabase.styleString(m_font);

    QList<int> pointSizes = m_fontDatabase.pointSizes(family, fontStyle);
    if (pointSizes.empty())
        pointSizes = QFontDatabase::standardSizes();

    QSignalBlocker blocker(m_widget->sizeComboBox);
    m_widget->sizeComboBox->clear();
    m_widget->sizeComboBox->setCurrentIndex(-1);
    m_widget->sizeComboBox->setEnabled(!pointSizes.empty());

    //  try to maintain selection or select closest.
    if (!pointSizes.empty()) {
        QString n;
        for (int pointSize : std::as_const(pointSizes))
            m_widget->sizeComboBox->addItem(n.setNum(pointSize), QVariant(pointSize));
        const int closestIndex = closestPointSizeIndex(m_font.pointSize());
        if (closestIndex != -1)
            m_widget->sizeComboBox->setCurrentIndex(closestIndex);
    }
}

void GeneralSettingsPage::updateFontStyleSelector()
{
    const QString &fontStyle = m_fontDatabase.styleString(m_font);
    const QStringList &styles = m_fontDatabase.styles(m_font.family());

    QSignalBlocker blocker(m_widget->styleComboBox);
    m_widget->styleComboBox->clear();
    m_widget->styleComboBox->setCurrentIndex(-1);
    m_widget->styleComboBox->setEnabled(!styles.empty());

    if (!styles.empty()) {
        int normalIndex = -1;
        const QString normalStyle = "Normal";
        for (const QString &style : styles) {
            // try to maintain selection or select 'normal' preferably
            const int newIndex = m_widget->styleComboBox->count();
            m_widget->styleComboBox->addItem(style);
            if (fontStyle == style) {
                m_widget->styleComboBox->setCurrentIndex(newIndex);
            } else {
                if (fontStyle ==  normalStyle)
                    normalIndex = newIndex;
            }
        }
        if (m_widget->styleComboBox->currentIndex() == -1 && normalIndex != -1)
            m_widget->styleComboBox->setCurrentIndex(normalIndex);
    }
}

void GeneralSettingsPage::updateFontFamilySelector()
{
    m_widget->familyComboBox->setCurrentFont(m_font);
}

void GeneralSettingsPage::updateFont()
{
    const QString &family = m_widget->familyComboBox->currentFont().family();
    m_font.setFamily(family);

    int fontSize = 14;
    int currentIndex = m_widget->sizeComboBox->currentIndex();
    if (currentIndex != -1)
        fontSize = m_widget->sizeComboBox->itemData(currentIndex).toInt();
    m_font.setPointSize(fontSize);

    currentIndex = m_widget->styleComboBox->currentIndex();
    if (currentIndex != -1)
        m_font.setStyleName(m_widget->styleComboBox->itemText(currentIndex));
}

int GeneralSettingsPage::closestPointSizeIndex(int desiredPointSize) const
{
    //  try to maintain selection or select closest.
    int closestIndex = -1;
    int closestAbsError = 0xFFFF;

    const int pointSizeCount = m_widget->sizeComboBox->count();
    for (int i = 0; i < pointSizeCount; i++) {
        const int itemPointSize = m_widget->sizeComboBox->itemData(i).toInt();
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

void GeneralSettingsPage::finish()
{
    delete m_widget;
}

} // Internal
} // Help
