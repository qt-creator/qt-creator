/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
** contact the sales department at http://www.qtsoftware.com/contact.
**
**************************************************************************/

#include "generalsettingspage.h"

#include <QtHelp/QHelpEngine>

#if defined(QT_NO_WEBKIT)
#include <QtGui/QApplication>
#else
#include <QtWebKit/QWebSettings>
#endif

using namespace Help::Internal;

GeneralSettingsPage::GeneralSettingsPage(QHelpEngine *helpEngine)
    : m_currentPage(0)
    , m_helpEngine(helpEngine)
{
#if !defined(QT_NO_WEBKIT)
    QWebSettings* webSettings = QWebSettings::globalSettings();
    font.setFamily(webSettings->fontFamily(QWebSettings::StandardFont));
    font.setPointSize(webSettings->fontSize(QWebSettings::DefaultFontSize));
#else
    font = qApp->font();
#endif
}

QString GeneralSettingsPage::id() const
{
    return QLatin1String("General settings");
}

QString GeneralSettingsPage::trName() const
{
    return tr("General settings");
}

QString GeneralSettingsPage::category() const
{
    return QLatin1String("Help");
}

QString GeneralSettingsPage::trCategory() const
{
    return tr("Help");
}

QWidget *GeneralSettingsPage::createPage(QWidget *parent)
{
    m_currentPage = new QWidget(parent);

    m_ui.setupUi(m_currentPage);
    m_ui.sizeComboBox->setEditable(false);
    m_ui.styleComboBox->setEditable(false);

    font = qVariantValue<QFont>(m_helpEngine->customValue(QLatin1String("font"),
        font));

    updateFontSize();
    updateFontStyle();
    updateFontFamily();

    return m_currentPage;
}

void GeneralSettingsPage::apply()
{
    const QString &family = m_ui.familyComboBox->currentFont().family();
    font.setFamily(family);

    int fontSize = 14;
    int currentIndex = m_ui.sizeComboBox->currentIndex();
    if (currentIndex != -1)
        fontSize = m_ui.sizeComboBox->itemData(currentIndex).toInt();
    font.setPointSize(fontSize);

    QString fontStyle = QLatin1String("Normal");
    currentIndex = m_ui.styleComboBox->currentIndex();
    if (currentIndex != -1)
        fontStyle = m_ui.styleComboBox->itemText(currentIndex);
    font.setBold(fontDatabase.bold(family, fontStyle));
    font.setItalic(fontDatabase.italic(family, fontStyle));

    const int weight = fontDatabase.weight(family, fontStyle);
    if (weight >= 0)    // Weight < 0 asserts...
        font.setWeight(weight);

    m_helpEngine->setCustomValue(QLatin1String("font"), font);

#if !defined(QT_NO_WEBKIT)
    QWebSettings* webSettings = QWebSettings::globalSettings();
    webSettings->setFontFamily(QWebSettings::StandardFont, font.family());
    webSettings->setFontSize(QWebSettings::DefaultFontSize, font.pointSize());
#else
    emit fontChanged();
#endif
}

void GeneralSettingsPage::finish()
{
    // Hmm, what to do here?
}

void GeneralSettingsPage::updateFontSize()
{
    const QString &family = font.family();
    const QString &fontStyle = fontDatabase.styleString(font);

    QList<int> pointSizes =  fontDatabase.pointSizes(family, fontStyle);
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
        const int closestIndex = closestPointSizeIndex(font.pointSize());
        if (closestIndex != -1)
            m_ui.sizeComboBox->setCurrentIndex(closestIndex);
    }
}

void GeneralSettingsPage::updateFontStyle()
{
    const QString &fontStyle = fontDatabase.styleString(font);
    const QStringList &styles = fontDatabase.styles(font.family());

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
    m_ui.familyComboBox->setCurrentFont(font);
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
