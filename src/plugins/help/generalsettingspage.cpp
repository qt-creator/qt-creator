/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "generalsettingspage.h"

#include "bookmarkmanager.h"
#include "centralwidget.h"
#include "helpconstants.h"
#include "helpviewer.h"
#include "localhelpmanager.h"
#include "xbelsupport.h"

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

using namespace Help::Internal;

GeneralSettingsPage::GeneralSettingsPage()
    : m_ui(0)
{
    m_font = qApp->font();
#if !defined(QT_NO_WEBKIT)
    QWebSettings* webSettings = QWebSettings::globalSettings();
    m_font.setPointSize(webSettings->fontSize(QWebSettings::DefaultFontSize));
#endif
    setId("A.General settings");
    setDisplayName(tr("General"));
    setCategory(Help::Constants::HELP_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("Help", Help::Constants::HELP_TR_CATEGORY));
    setCategoryIcon(QLatin1String(Help::Constants::HELP_CATEGORY_ICON));
}

QWidget *GeneralSettingsPage::createPage(QWidget *parent)
{
    QWidget *widget = new QWidget(parent);
    m_ui = new Ui::GeneralSettingsPage;
    m_ui->setupUi(widget);
    m_ui->sizeComboBox->setEditable(false);
    m_ui->styleComboBox->setEditable(false);

    Core::HelpManager *manager = Core::HelpManager::instance();
    m_font = qvariant_cast<QFont>(manager->customValue(QLatin1String("font"),
        m_font));

    updateFontSize();
    updateFontStyle();
    updateFontFamily();

    m_homePage = manager->customValue(QLatin1String("HomePage"), QString())
        .toString();
    if (m_homePage.isEmpty()) {
        m_homePage = manager->customValue(QLatin1String("DefaultHomePage"),
            Help::Constants::AboutBlank).toString();
    }
    m_ui->homePageLineEdit->setText(m_homePage);

    m_startOption = manager->customValue(QLatin1String("StartOption"),
        Help::Constants::ShowLastPages).toInt();
    m_ui->helpStartComboBox->setCurrentIndex(m_startOption);

    m_contextOption = manager->customValue(QLatin1String("ContextHelpOption"),
        Help::Constants::SideBySideIfPossible).toInt();
    m_ui->contextHelpComboBox->setCurrentIndex(m_contextOption);

    connect(m_ui->currentPageButton, SIGNAL(clicked()), this, SLOT(setCurrentPage()));
    connect(m_ui->blankPageButton, SIGNAL(clicked()), this, SLOT(setBlankPage()));
    connect(m_ui->defaultPageButton, SIGNAL(clicked()), this, SLOT(setDefaultPage()));

    HelpViewer *viewer = CentralWidget::instance()->currentHelpViewer();
    if (!viewer)
        m_ui->currentPageButton->setEnabled(false);

    m_ui->errorLabel->setVisible(false);
    connect(m_ui->importButton, SIGNAL(clicked()), this, SLOT(importBookmarks()));
    connect(m_ui->exportButton, SIGNAL(clicked()), this, SLOT(exportBookmarks()));

    if (m_searchKeywords.isEmpty()) {
        QTextStream(&m_searchKeywords) << ' ' << m_ui->contextHelpLabel->text()
           << ' ' << m_ui->startPageLabel->text() << ' ' << m_ui->homePageLabel->text();
        m_searchKeywords.remove(QLatin1Char('&'));
    }

    m_returnOnClose = manager->customValue(QLatin1String("ReturnOnClose"),
        false).toBool();
    m_ui->m_returnOnClose->setChecked(m_returnOnClose);

    return widget;
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

    Core::HelpManager *manager = Core::HelpManager::instance();
    if (newFont != m_font) {
        m_font = newFont;
        manager->setCustomValue(QLatin1String("font"), newFont);
        emit fontChanged();
    }

    QString homePage = QUrl::fromUserInput(m_ui->homePageLineEdit->text()).toString();
    if (homePage.isEmpty())
        homePage = Help::Constants::AboutBlank;
    m_ui->homePageLineEdit->setText(homePage);
    if (m_homePage != homePage) {
        m_homePage = homePage;
        manager->setCustomValue(QLatin1String("HomePage"), homePage);
    }

    const int startOption = m_ui->helpStartComboBox->currentIndex();
    if (m_startOption != startOption) {
        m_startOption = startOption;
        manager->setCustomValue(QLatin1String("StartOption"), startOption);
    }

    const int helpOption = m_ui->contextHelpComboBox->currentIndex();
    if (m_contextOption != helpOption) {
        m_contextOption = helpOption;
        manager->setCustomValue(QLatin1String("ContextHelpOption"), helpOption);

        QSettings *settings = Core::ICore::settings();
        settings->beginGroup(QLatin1String(Help::Constants::ID_MODE_HELP));
        settings->setValue(QLatin1String("ContextHelpOption"), helpOption);
        settings->endGroup();

        emit contextHelpOptionChanged();
    }

    const bool close = m_ui->m_returnOnClose->isChecked();
    if (m_returnOnClose != close) {
        m_returnOnClose = close;
        manager->setCustomValue(QLatin1String("ReturnOnClose"), close);
        emit returnOnCloseChanged();
    }
}

void GeneralSettingsPage::setCurrentPage()
{
    HelpViewer *viewer = CentralWidget::instance()->currentHelpViewer();
    if (viewer)
        m_ui->homePageLineEdit->setText(viewer->source().toString());
}

void GeneralSettingsPage::setBlankPage()
{
    m_ui->homePageLineEdit->setText(Help::Constants::AboutBlank);
}

void GeneralSettingsPage::setDefaultPage()
{
    const QString &defaultHomePage = Core::HelpManager::instance()
        ->customValue(QLatin1String("DefaultHomePage"), QString()).toString();
    m_ui->homePageLineEdit->setText(defaultHomePage);
}

void GeneralSettingsPage::importBookmarks()
{
    m_ui->errorLabel->setVisible(false);

    QString fileName = QFileDialog::getOpenFileName(0, tr("Import Bookmarks"),
        QDir::currentPath(), tr("Files (*.xbel)"));

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

    QString fileName = QFileDialog::getSaveFileName(0, tr("Save File"),
        QLatin1String("untitled.xbel"), tr("Files (*.xbel)"));

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

bool GeneralSettingsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

void GeneralSettingsPage::finish()
{
    if (!m_ui) // page was never shown
        return;
    delete m_ui;
    m_ui = 0;
}
