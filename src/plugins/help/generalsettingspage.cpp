/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "generalsettingspage.h"

#include "centralwidget.h"
#include "helpconstants.h"
#include "helpviewer.h"
#include "localhelpmanager.h"
#include "xbelsupport.h"

#include <bookmarkmanager.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/icore.h>

#include <utils/fileutils.h>

#include <QCoreApplication>
#include <QSettings>
#include <QTextStream>

#include <QApplication>
#include <QFileDialog>

#if !defined(QT_NO_WEBKIT)
#include <QWebSettings>
#endif

using namespace Core;
using namespace Help::Internal;

GeneralSettingsPage::GeneralSettingsPage()
    : m_ui(0)
{
    setId("A.General settings");
    setDisplayName(tr("General"));
    setCategory(Help::Constants::HELP_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("Help", Help::Constants::HELP_TR_CATEGORY));
    setCategoryIcon(QLatin1String(Help::Constants::HELP_CATEGORY_ICON));
}

QWidget *GeneralSettingsPage::widget()
{
    if (!m_widget) {
        m_widget = new QWidget;
        m_ui = new Ui::GeneralSettingsPage;
        m_ui->setupUi(m_widget);
        m_ui->sizeComboBox->setEditable(false);
        m_ui->styleComboBox->setEditable(false);

        m_font = LocalHelpManager::fallbackFont();

        updateFontSize();
        updateFontStyle();
        updateFontFamily();

        m_homePage = LocalHelpManager::homePage();
        m_ui->homePageLineEdit->setText(m_homePage);

        m_startOption = LocalHelpManager::startOption();
        m_ui->helpStartComboBox->setCurrentIndex(m_startOption);

        m_contextOption = LocalHelpManager::contextHelpOption();
        m_ui->contextHelpComboBox->setCurrentIndex(m_contextOption);

        connect(m_ui->currentPageButton, &QPushButton::clicked,
                this, &GeneralSettingsPage::setCurrentPage);
        connect(m_ui->blankPageButton, &QPushButton::clicked,
                this, &GeneralSettingsPage::setBlankPage);
        connect(m_ui->defaultPageButton, &QPushButton::clicked,
                this, &GeneralSettingsPage::setDefaultPage);

        HelpViewer *viewer = CentralWidget::instance()->currentViewer();
        if (!viewer)
            m_ui->currentPageButton->setEnabled(false);

        m_ui->errorLabel->setVisible(false);
        connect(m_ui->importButton, &QPushButton::clicked,
                this, &GeneralSettingsPage::importBookmarks);
        connect(m_ui->exportButton, &QPushButton::clicked,
                this, &GeneralSettingsPage::exportBookmarks);

        m_returnOnClose = LocalHelpManager::returnOnClose();
        m_ui->m_returnOnClose->setChecked(m_returnOnClose);
    }
    return m_widget;
}

void GeneralSettingsPage::apply()
{
    if (!m_ui) // page was never shown
        return;
    QFont newFont;
    const QString &family = m_ui->familyComboBox->currentFont().family();
    newFont.setFamily(family);

    int fontSize = 14;
    int currentIndex = m_ui->sizeComboBox->currentIndex();
    if (currentIndex != -1)
        fontSize = m_ui->sizeComboBox->itemData(currentIndex).toInt();
    newFont.setPointSize(fontSize);

    QString fontStyle = QLatin1String("Normal");
    currentIndex = m_ui->styleComboBox->currentIndex();
    if (currentIndex != -1)
        fontStyle = m_ui->styleComboBox->itemText(currentIndex);
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

    if (newFont != m_font) {
        m_font = newFont;
        LocalHelpManager::setFallbackFont(newFont);
        emit fontChanged();
    }

    QString homePage = QUrl::fromUserInput(m_ui->homePageLineEdit->text()).toString();
    if (homePage.isEmpty())
        homePage = Help::Constants::AboutBlank;
    m_ui->homePageLineEdit->setText(homePage);
    if (m_homePage != homePage) {
        m_homePage = homePage;
        LocalHelpManager::setHomePage(homePage);
    }

    const int startOption = m_ui->helpStartComboBox->currentIndex();
    if (m_startOption != startOption) {
        m_startOption = startOption;
        LocalHelpManager::setStartOption((LocalHelpManager::StartOption)m_startOption);
    }

    const int helpOption = m_ui->contextHelpComboBox->currentIndex();
    if (m_contextOption != helpOption) {
        m_contextOption = helpOption;
        LocalHelpManager::setContextHelpOption((HelpManager::HelpViewerLocation)m_contextOption);
    }

    const bool close = m_ui->m_returnOnClose->isChecked();
    if (m_returnOnClose != close) {
        m_returnOnClose = close;
        LocalHelpManager::setReturnOnClose(m_returnOnClose);
    }
}

void GeneralSettingsPage::setCurrentPage()
{
    HelpViewer *viewer = CentralWidget::instance()->currentViewer();
    if (viewer)
        m_ui->homePageLineEdit->setText(viewer->source().toString());
}

void GeneralSettingsPage::setBlankPage()
{
    m_ui->homePageLineEdit->setText(Help::Constants::AboutBlank);
}

void GeneralSettingsPage::setDefaultPage()
{
    m_ui->homePageLineEdit->setText(LocalHelpManager::defaultHomePage());
}

void GeneralSettingsPage::importBookmarks()
{
    m_ui->errorLabel->setVisible(false);

    QString fileName = QFileDialog::getOpenFileName(ICore::dialogParent(),
        tr("Import Bookmarks"), QDir::currentPath(), tr("Files (*.xbel)"));

    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly)) {
        const BookmarkManager &manager = LocalHelpManager::bookmarkManager();
        XbelReader reader(manager.treeBookmarkModel(), manager.listBookmarkModel());
        if (reader.readFromFile(&file))
            return;
    }

    m_ui->errorLabel->setVisible(true);
    m_ui->errorLabel->setText(tr("Cannot import bookmarks."));
}

void GeneralSettingsPage::exportBookmarks()
{
    m_ui->errorLabel->setVisible(false);

    QString fileName = QFileDialog::getSaveFileName(ICore::dialogParent(),
        tr("Save File"), QLatin1String("untitled.xbel"), tr("Files (*.xbel)"));

    QLatin1String suffix(".xbel");
    if (!fileName.endsWith(suffix))
        fileName.append(suffix);

    Utils::FileSaver saver(fileName);
    if (!saver.hasError()) {
        XbelWriter writer(LocalHelpManager::bookmarkManager().treeBookmarkModel());
        writer.writeToFile(saver.file());
        saver.setResult(&writer);
    }
    if (!saver.finalize()) {
        m_ui->errorLabel->setVisible(true);
        m_ui->errorLabel->setText(saver.errorString());
    }
}

void GeneralSettingsPage::updateFontSize()
{
    const QString &family = m_font.family();
    const QString &fontStyle = m_fontDatabase.styleString(m_font);

    QList<int> pointSizes = m_fontDatabase.pointSizes(family, fontStyle);
    if (pointSizes.empty())
        pointSizes = QFontDatabase::standardSizes();

    m_ui->sizeComboBox->clear();
    m_ui->sizeComboBox->setCurrentIndex(-1);
    m_ui->sizeComboBox->setEnabled(!pointSizes.empty());

    //  try to maintain selection or select closest.
    if (!pointSizes.empty()) {
        QString n;
        foreach (int pointSize, pointSizes)
            m_ui->sizeComboBox->addItem(n.setNum(pointSize), QVariant(pointSize));
        const int closestIndex = closestPointSizeIndex(m_font.pointSize());
        if (closestIndex != -1)
            m_ui->sizeComboBox->setCurrentIndex(closestIndex);
    }
}

void GeneralSettingsPage::updateFontStyle()
{
    const QString &fontStyle = m_fontDatabase.styleString(m_font);
    const QStringList &styles = m_fontDatabase.styles(m_font.family());

    m_ui->styleComboBox->clear();
    m_ui->styleComboBox->setCurrentIndex(-1);
    m_ui->styleComboBox->setEnabled(!styles.empty());

    if (!styles.empty()) {
        int normalIndex = -1;
        const QString normalStyle = QLatin1String("Normal");
        foreach (const QString &style, styles) {
            // try to maintain selection or select 'normal' preferably
            const int newIndex = m_ui->styleComboBox->count();
            m_ui->styleComboBox->addItem(style);
            if (fontStyle == style) {
                m_ui->styleComboBox->setCurrentIndex(newIndex);
            } else {
                if (fontStyle ==  normalStyle)
                    normalIndex = newIndex;
            }
        }
        if (m_ui->styleComboBox->currentIndex() == -1 && normalIndex != -1)
            m_ui->styleComboBox->setCurrentIndex(normalIndex);
    }
}

void GeneralSettingsPage::updateFontFamily()
{
    m_ui->familyComboBox->setCurrentFont(m_font);
}

int GeneralSettingsPage::closestPointSizeIndex(int desiredPointSize) const
{
    //  try to maintain selection or select closest.
    int closestIndex = -1;
    int closestAbsError = 0xFFFF;

    const int pointSizeCount = m_ui->sizeComboBox->count();
    for (int i = 0; i < pointSizeCount; i++) {
        const int itemPointSize = m_ui->sizeComboBox->itemData(i).toInt();
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
    if (!m_ui) // page was never shown
        return;
    delete m_ui;
    m_ui = 0;
}
