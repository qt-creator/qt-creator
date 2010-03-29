/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "generalsettingspage.h"

#include "bookmarkmanager.h"
#include "centralwidget.h"
#include "helpconstants.h"
#include "helpmanager.h"
#include "helpviewer.h"
#include "xbelsupport.h"

#include <coreplugin/coreconstants.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QTextStream>

#if defined(QT_NO_WEBKIT)
#include <QtGui/QApplication>
#else
#include <QtWebKit/QWebSettings>
#endif
#include <QtGui/QFileDialog>

#include <QtHelp/QHelpEngineCore>

using namespace Help::Internal;

GeneralSettingsPage::GeneralSettingsPage()
{
#if !defined(QT_NO_WEBKIT)
    QWebSettings* webSettings = QWebSettings::globalSettings();
    m_font.setFamily(webSettings->fontFamily(QWebSettings::StandardFont));
    m_font.setPointSize(webSettings->fontSize(QWebSettings::DefaultFontSize));
#else
    m_font = qApp->font();
#endif
}

QString GeneralSettingsPage::id() const
{
    return QLatin1String("A.General settings");
}

QString GeneralSettingsPage::displayName() const
{
    return tr("General settings");
}

QString GeneralSettingsPage::category() const
{
    return QLatin1String(Help::Constants::HELP_CATEGORY);
}

QString GeneralSettingsPage::displayCategory() const
{
    return QCoreApplication::translate("Help", Help::Constants::HELP_TR_CATEGORY);
}

QIcon GeneralSettingsPage::categoryIcon() const
{
    return QIcon(QLatin1String(Help::Constants::HELP_CATEGORY_ICON));
}

QWidget *GeneralSettingsPage::createPage(QWidget *parent)
{
    QWidget *widget = new QWidget(parent);

    m_ui.setupUi(widget);
    m_ui.sizeComboBox->setEditable(false);
    m_ui.styleComboBox->setEditable(false);

    const QHelpEngineCore &engine = HelpManager::helpEngineCore();
    m_font = qVariantValue<QFont>(engine.customValue(QLatin1String("font"), m_font));

    updateFontSize();
    updateFontStyle();
    updateFontFamily();

    m_homePage = engine.customValue(QLatin1String("HomePage"), QString()).toString();
    if (m_homePage.isEmpty()) {
        m_homePage = engine.customValue(QLatin1String("DefaultHomePage"),
            Help::Constants::AboutBlank).toString();
    }
    m_ui.homePageLineEdit->setText(m_homePage);

    m_startOption = engine.customValue(QLatin1String("StartOption"), 2).toInt();
    m_ui.helpStartComboBox->setCurrentIndex(m_startOption);

    m_helpOption = engine.customValue(QLatin1String("ContextHelpOption"), 0).toInt();
    m_ui.contextHelpComboBox->setCurrentIndex(m_helpOption);

    connect(m_ui.currentPageButton, SIGNAL(clicked()), this, SLOT(setCurrentPage()));
    connect(m_ui.blankPageButton, SIGNAL(clicked()), this, SLOT(setBlankPage()));
    connect(m_ui.defaultPageButton, SIGNAL(clicked()), this, SLOT(setDefaultPage()));

    HelpViewer *viewer = CentralWidget::instance()->currentHelpViewer();
    if (viewer == 0)
        m_ui.currentPageButton->setEnabled(false);

    m_ui.errorLabel->setVisible(false);
    connect(m_ui.importButton, SIGNAL(clicked()), this, SLOT(importBookmarks()));
    connect(m_ui.exportButton, SIGNAL(clicked()), this, SLOT(exportBookmarks()));

    if (m_searchKeywords.isEmpty()) {
        QTextStream(&m_searchKeywords) << ' ' << m_ui.contextHelpLabel->text()
           << ' ' << m_ui.startPageLabel->text() << ' ' << m_ui.homePageLabel->text()
           << ' ' << m_ui.bookmarkGroupBox->title();
        m_searchKeywords.remove(QLatin1Char('&'));
    }
    return widget;
}

void GeneralSettingsPage::apply()
{
    QFont newFont;
    const QString &family = m_ui.familyComboBox->currentFont().family();
    newFont.setFamily(family);

    int fontSize = 14;
    int currentIndex = m_ui.sizeComboBox->currentIndex();
    if (currentIndex != -1)
        fontSize = m_ui.sizeComboBox->itemData(currentIndex).toInt();
    newFont.setPointSize(fontSize);

    QString fontStyle = QLatin1String("Normal");
    currentIndex = m_ui.styleComboBox->currentIndex();
    if (currentIndex != -1)
        fontStyle = m_ui.styleComboBox->itemText(currentIndex);
    newFont.setBold(m_fontDatabase.bold(family, fontStyle));
    if (fontStyle.contains(QLatin1String("Italic")))
        newFont.setStyle(QFont::StyleItalic);
    else if (fontStyle.contains(QLatin1String("Oblique")))
        newFont.setStyle(QFont::StyleOblique);
    else
        newFont.setStyle(QFont::StyleNormal);

    const int weight = m_fontDatabase.weight(family, fontStyle);
    if (weight >= 0)    // Weight < 0 asserts...
        newFont.setWeight(weight);

    QHelpEngineCore *engine = &HelpManager::helpEngineCore();
    engine->setCustomValue(QLatin1String("font"), newFont);

#if !defined(QT_NO_WEBKIT)
    QWebSettings* webSettings = QWebSettings::globalSettings();
    webSettings->setFontFamily(QWebSettings::StandardFont, m_font.family());
    webSettings->setFontSize(QWebSettings::DefaultFontSize, m_font.pointSize());
#else
    emit fontChanged();
#endif

    QString homePage = m_ui.homePageLineEdit->text();
    if (homePage.isEmpty())
        homePage = Help::Constants::AboutBlank;
    engine->setCustomValue(QLatin1String("HomePage"), homePage);

    const int startOption = m_ui.helpStartComboBox->currentIndex();
    engine->setCustomValue(QLatin1String("StartOption"), startOption);

    const int helpOption = m_ui.contextHelpComboBox->currentIndex();
    engine->setCustomValue(QLatin1String("ContextHelpOption"), helpOption);

    // no need to call setup on the gui engine since we use only core engine
}

void GeneralSettingsPage::setCurrentPage()
{
    HelpViewer *viewer = CentralWidget::instance()->currentHelpViewer();
    if (viewer)
        m_ui.homePageLineEdit->setText(viewer->source().toString());
}

void GeneralSettingsPage::setBlankPage()
{
    m_ui.homePageLineEdit->setText(Help::Constants::AboutBlank);
}

void GeneralSettingsPage::setDefaultPage()
{
    const QString &defaultHomePage = HelpManager::helpEngineCore()
        .customValue(QLatin1String("DefaultHomePage"), QString()).toString();
    m_ui.homePageLineEdit->setText(defaultHomePage);
}

void GeneralSettingsPage::importBookmarks()
{
    m_ui.errorLabel->setVisible(false);

    QString fileName = QFileDialog::getOpenFileName(0, tr("Open Image"),
        QDir::currentPath(), tr("Files (*.xbel)"));

    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly)) {
        const BookmarkManager &manager = HelpManager::bookmarkManager();
        XbelReader reader(manager.treeBookmarkModel(), manager.listBookmarkModel());
        if (reader.readFromFile(&file))
            return;
    }

    m_ui.errorLabel->setVisible(true);
    m_ui.errorLabel->setText(tr("There was an error while importing bookmarks!"));
}

void GeneralSettingsPage::exportBookmarks()
{
    m_ui.errorLabel->setVisible(false);

    QString fileName = QFileDialog::getSaveFileName(0, tr("Save File"),
        "untitled.xbel", tr("Files (*.xbel)"));

    QLatin1String suffix(".xbel");
    if (!fileName.endsWith(suffix))
        fileName.append(suffix);

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        XbelWriter writer(HelpManager::bookmarkManager().treeBookmarkModel());
        writer.writeToFile(&file);
    }
}

void GeneralSettingsPage::updateFontSize()
{
    const QString &family = m_font.family();
    const QString &fontStyle = m_fontDatabase.styleString(m_font);

    QList<int> pointSizes = m_fontDatabase.pointSizes(family, fontStyle);
    if (pointSizes.empty())
        pointSizes = QFontDatabase::standardSizes();

    m_ui.sizeComboBox->clear();
    m_ui.sizeComboBox->setCurrentIndex(-1);
    m_ui.sizeComboBox->setEnabled(!pointSizes.empty());

    //  try to maintain selection or select closest.
    if (!pointSizes.empty()) {
        QString n;
        foreach (int pointSize, pointSizes)
            m_ui.sizeComboBox->addItem(n.setNum(pointSize), QVariant(pointSize));
        const int closestIndex = closestPointSizeIndex(m_font.pointSize());
        if (closestIndex != -1)
            m_ui.sizeComboBox->setCurrentIndex(closestIndex);
    }
}

void GeneralSettingsPage::updateFontStyle()
{
    const QString &fontStyle = m_fontDatabase.styleString(m_font);
    const QStringList &styles = m_fontDatabase.styles(m_font.family());

    m_ui.styleComboBox->clear();
    m_ui.styleComboBox->setCurrentIndex(-1);
    m_ui.styleComboBox->setEnabled(!styles.empty());

    if (!styles.empty()) {
        int normalIndex = -1;
        const QString normalStyle = QLatin1String("Normal");
        foreach (const QString &style, styles) {
            // try to maintain selection or select 'normal' preferably
            const int newIndex = m_ui.styleComboBox->count();
            m_ui.styleComboBox->addItem(style);
            if (fontStyle == style) {
                m_ui.styleComboBox->setCurrentIndex(newIndex);
            } else {
                if (fontStyle ==  normalStyle)
                    normalIndex = newIndex;
            }
        }
        if (m_ui.styleComboBox->currentIndex() == -1 && normalIndex != -1)
            m_ui.styleComboBox->setCurrentIndex(normalIndex);
    }
}

void GeneralSettingsPage::updateFontFamily()
{
    m_ui.familyComboBox->setCurrentFont(m_font);
}

int GeneralSettingsPage::closestPointSizeIndex(int desiredPointSize) const
{
    //  try to maintain selection or select closest.
    int closestIndex = -1;
    int closestAbsError = 0xFFFF;

    const int pointSizeCount = m_ui.sizeComboBox->count();
    for (int i = 0; i < pointSizeCount; i++) {
        const int itemPointSize = m_ui.sizeComboBox->itemData(i).toInt();
        const int absError = qAbs(desiredPointSize - itemPointSize);
        if (absError < closestAbsError) {
            closestIndex  = i;
            closestAbsError = absError;
            if (closestAbsError == 0)
                break;
        } else {    // past optimum
            if (absError > closestAbsError) {
                break;
            }
        }
    }
    return closestIndex;
}

bool GeneralSettingsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}
