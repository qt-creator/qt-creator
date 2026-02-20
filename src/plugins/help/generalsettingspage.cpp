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
    void cancel() final { helpSettings().cancel(); }

    bool isDirty() const final
    {
        return helpSettings().isDirty() || m_font != LocalHelpManager::fallbackFont();
    }

    void setCurrentPage();
    void setBlankPage();
    void setDefaultPage();
    void importBookmarks();
    void exportBookmarks();

    void updateFontSizeSelector();
    void updateFontFamilySelector();
    void updateFont();
    int closestPointSizeIndex(int desiredPointSize) const;

    QFont m_font;

    QFontComboBox *familyComboBox;
    QComboBox *sizeComboBox;
    QPushButton *currentPageButton;
    QPushButton *blankPageButton;
    QPushButton *defaultPageButton;
    QLabel *errorLabel;
};

GeneralSettingsPageWidget::GeneralSettingsPageWidget()
{
    using namespace Layouting;

    // font group box
    familyComboBox = new QFontComboBox;
    sizeComboBox = new QComboBox;

    HelpSettings &s = helpSettings();

    auto fontGroupBox = new QGroupBox(Tr::tr("Font"));
    // clang-format off
    Column {
        Row { Tr::tr("Family:"), familyComboBox,
              Tr::tr("Size:"), sizeComboBox, st },
        Row { Tr::tr("Note: The above setting takes effect only if the "
                     "HTML file does not use a style sheet.") },
        Row { Tr::tr("Zoom:"), s.fontZoom, s.antiAlias, st }
    }.attachTo(fontGroupBox);
    // clang-format on

    currentPageButton = new QPushButton(Tr::tr("Use &Current Page"));
    blankPageButton = new QPushButton(Tr::tr("Use &Blank Page"));
    defaultPageButton = new QPushButton(Tr::tr("Reset"));
    defaultPageButton->setToolTip(Tr::tr("Reset to default."));

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

    Column {
        fontGroupBox,
        Group {
            title(Tr::tr("Startup")),
            Layouting::objectName("startupGroupBox"),
            Form {
                fieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow),
                s.contextHelpOption, br,
                s.startOption, br,
                s.homePage, currentPageButton, blankPageButton, defaultPageButton
            }
        },
        Group {
            title(Tr::tr("Behaviour")),
            Column {
                s.scrollWheelZooming,
                s.returnOnClose,
                Row { Tr::tr("Viewer backend:"), s.viewerBackend, st }
            }
        },
        Row {
            st,
            errorLabel,
            PushButton {
                text(Tr::tr("Import Bookmarks...")),
                onClicked(this, [this] { importBookmarks(); })
            },
            PushButton {
                text(Tr::tr("Export Bookmarks...")),
                onClicked(this, [this] { exportBookmarks(); })
            },
        },
        st
    }.attachTo(this);

    m_font = LocalHelpManager::fallbackFont();

    updateFontSizeSelector();
    updateFontFamilySelector();

    connect(familyComboBox, &QFontComboBox::currentFontChanged, this, [this] {
        updateFont();
        updateFontSizeSelector();
        updateFont(); // changes that might have happened when updating the selectors
    });

    connect(sizeComboBox, &QComboBox::currentIndexChanged,
            this, &GeneralSettingsPageWidget::updateFont);

    connect(currentPageButton, &QPushButton::clicked,
            this, &GeneralSettingsPageWidget::setCurrentPage);
    connect(blankPageButton, &QPushButton::clicked,
            this, &GeneralSettingsPageWidget::setBlankPage);
    connect(defaultPageButton, &QPushButton::clicked,
            this, &GeneralSettingsPageWidget::setDefaultPage);

    HelpViewer *viewer = modeHelpWidget()->currentViewer();
    if (!viewer)
        currentPageButton->setEnabled(false);

    errorLabel->setVisible(false);

    connect(&helpSettings(), &AspectContainer::volatileValueChanged, this, &checkSettingsDirty);
    connect(familyComboBox, &QComboBox::currentIndexChanged, &helpSettings(), checkSettingsDirty);
    connect(familyComboBox, &QComboBox::currentTextChanged, &helpSettings(), checkSettingsDirty);
    installCheckSettingsDirtyTrigger(sizeComboBox);
}

void GeneralSettingsPageWidget::apply()
{
    if (m_font != LocalHelpManager::fallbackFont())
        LocalHelpManager::setFallbackFont(m_font);

    helpSettings().apply();

    if (helpSettings().homePage().isEmpty())
        helpSettings().homePage.setValue(Help::Constants::AboutBlank);

    helpSettings().writeSettings();
}

void GeneralSettingsPageWidget::setCurrentPage()
{
    if (HelpViewer *viewer = modeHelpWidget()->currentViewer())
        helpSettings().homePage.setVolatileValue(viewer->source().toString());
}

void GeneralSettingsPageWidget::setBlankPage()
{
    helpSettings().homePage.setVolatileValue(Help::Constants::AboutBlank);
}

void GeneralSettingsPageWidget::setDefaultPage()
{
    helpSettings().homePage.setVolatileValue(helpSettings().homePage.defaultValue());
}

void GeneralSettingsPageWidget::importBookmarks()
{
    errorLabel->setVisible(false);

    FilePath filePath = FileUtils::getOpenFilePath(Tr::tr("Import Bookmarks"),
                                                   FilePath::fromString(QDir::currentPath()),
                                                   Tr::tr("Files (*.xbel)"));

    if (filePath.isEmpty())
        return;

    QFile file(filePath.toUrlishString());
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

    FilePath filePath = FileUtils::getSaveFilePath(Tr::tr("Save File"),
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
    if (const Result<> res = saver.finalize(); !res) {
        errorLabel->setVisible(true);
        errorLabel->setText(res.error());
    }
}

void GeneralSettingsPageWidget::updateFontSizeSelector()
{
    const QString &family = m_font.family();
    const QString &fontStyle = QFontDatabase::styleString(m_font);

    QList<int> pointSizes = QFontDatabase::pointSizes(family, fontStyle);
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
    setCategory(Core::Constants::HELP_CATEGORY);
    setWidgetCreator([] { return new GeneralSettingsPageWidget; });
}

} // Help::Interal
