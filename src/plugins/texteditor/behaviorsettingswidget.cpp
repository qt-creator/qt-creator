/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "behaviorsettingswidget.h"
#include "ui_behaviorsettingswidget.h"

#include <texteditor/tabsettings.h>
#include <texteditor/storagesettings.h>
#include <texteditor/behaviorsettings.h>
#include <texteditor/extraencodingsettings.h>

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QByteArray>
#include <QtCore/QTextCodec>
#include <QtCore/QTextStream>

#include <algorithm>
#include <functional>

namespace TextEditor {

struct BehaviorSettingsWidgetPrivate
{
    Ui::BehaviorSettingsWidget m_ui;
    QList<QTextCodec *> m_codecs;
};

BehaviorSettingsWidget::BehaviorSettingsWidget(QWidget *parent)
    : QWidget(parent)
    , m_d(new BehaviorSettingsWidgetPrivate)
{
    m_d->m_ui.setupUi(this);

    QList<int> mibs = QTextCodec::availableMibs();
    qSort(mibs);
    QList<int>::iterator firstNonNegative =
        std::find_if(mibs.begin(), mibs.end(), std::bind2nd(std::greater_equal<int>(), 0));
    if (firstNonNegative != mibs.end())
        std::rotate(mibs.begin(), firstNonNegative, mibs.end());
    foreach (int mib, mibs) {
        QTextCodec *codec = QTextCodec::codecForMib(mib);
        QString compoundName = codec->name();
        foreach (const QByteArray &alias, codec->aliases()) {
            compoundName += QLatin1String(" / ");
            compoundName += QString::fromLatin1(alias);
        }
        m_d->m_ui.encodingBox->addItem(compoundName);
        m_d->m_codecs.append(codec);
    }

    connect(m_d->m_ui.insertSpaces, SIGNAL(clicked(bool)), this, SIGNAL(insertSpacesChanged(bool)));
    connect(m_d->m_ui.autoInsertSpaces, SIGNAL(clicked(bool)),
            this, SIGNAL(autoInsertSpacesChanged(bool)));
    connect(m_d->m_ui.autoIndent, SIGNAL(clicked(bool)), this, SIGNAL(autoIndentChanged(bool)));
    connect(m_d->m_ui.smartBackspace, SIGNAL(clicked(bool)),
            this, SIGNAL(smartBackSpaceChanged(bool)));
    connect(m_d->m_ui.tabSize, SIGNAL(valueChanged(int)), this, SIGNAL(tabSizeChanged(int)));
    connect(m_d->m_ui.indentSize, SIGNAL(valueChanged(int)), this, SIGNAL(indentSizeChanged(int)));
    connect(m_d->m_ui.indentBlocksBehavior, SIGNAL(currentIndexChanged(int)),
            this, SIGNAL(indentBlocksBehaviorChanged(int)));
    connect(m_d->m_ui.tabKeyBehavior, SIGNAL(currentIndexChanged(int)),
            this, SIGNAL(tabKeyBehaviorChanged(int)));
    connect(m_d->m_ui.continuationAlignBehavior, SIGNAL(currentIndexChanged(int)),
            this, SIGNAL(continuationAlignBehaviorChanged(int)));
    connect(m_d->m_ui.cleanWhitespace, SIGNAL(clicked(bool)),
            this, SIGNAL(cleanWhiteSpaceChanged(bool)));
    connect(m_d->m_ui.inEntireDocument, SIGNAL(clicked(bool)),
            this, SIGNAL(inEntireDocumentChanged(bool)));
    connect(m_d->m_ui.addFinalNewLine, SIGNAL(clicked(bool)),
            this, SIGNAL(addFinalNewLineChanged(bool)));
    connect(m_d->m_ui.cleanIndentation, SIGNAL(clicked(bool)),
            this, SIGNAL(cleanIndentationChanged(bool)));
    connect(m_d->m_ui.mouseNavigation, SIGNAL(clicked(bool)),
            this, SIGNAL(mouseNavigationChanged(bool)));
    connect(m_d->m_ui.scrollWheelZooming, SIGNAL(clicked(bool)),
            this, SIGNAL(scrollWheelZoomingChanged(bool)));
    connect(m_d->m_ui.utf8BomBox, SIGNAL(currentIndexChanged(int)),
            this, SIGNAL(utf8BomSettingsChanged(int)));
    connect(m_d->m_ui.encodingBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(handleEncodingBoxChange(int)));
}

BehaviorSettingsWidget::~BehaviorSettingsWidget()
{
    delete m_d;
}

void BehaviorSettingsWidget::setActive(bool active)
{
    m_d->m_ui.groupBoxEncodings->setEnabled(active);
    m_d->m_ui.groupBoxMouse->setEnabled(active);
    m_d->m_ui.groupBoxStorageSettings->setEnabled(active);
    m_d->m_ui.groupBoxTabAndIndentSettings->setEnabled(active);
}

void BehaviorSettingsWidget::setAssignedCodec(QTextCodec *codec)
{
    for (int i = 0; i < m_d->m_codecs.size(); ++i) {
        if (codec == m_d->m_codecs.at(i)) {
            m_d->m_ui.encodingBox->setCurrentIndex(i);
            break;
        }
    }
}

QTextCodec *BehaviorSettingsWidget::assignedCodec() const
{
    return m_d->m_codecs.at(m_d->m_ui.encodingBox->currentIndex());
}

void BehaviorSettingsWidget::setAssignedTabSettings(const TabSettings &tabSettings)
{
    m_d->m_ui.insertSpaces->setChecked(tabSettings.m_spacesForTabs);
    m_d->m_ui.autoInsertSpaces->setChecked(tabSettings.m_autoSpacesForTabs);
    m_d->m_ui.autoIndent->setChecked(tabSettings.m_autoIndent);
    m_d->m_ui.smartBackspace->setChecked(tabSettings.m_smartBackspace);
    m_d->m_ui.tabSize->setValue(tabSettings.m_tabSize);
    m_d->m_ui.indentSize->setValue(tabSettings.m_indentSize);
    m_d->m_ui.indentBlocksBehavior->setCurrentIndex(tabSettings.m_indentBraces ?
                                                      (tabSettings.m_doubleIndentBlocks ? 2 : 1)
                                                        : 0);
    m_d->m_ui.tabKeyBehavior->setCurrentIndex(tabSettings.m_tabKeyBehavior);
    m_d->m_ui.continuationAlignBehavior->setCurrentIndex(tabSettings.m_continuationAlignBehavior);
}

void BehaviorSettingsWidget::assignedTabSettings(TabSettings *tabSettings) const
{
    tabSettings->m_spacesForTabs = m_d->m_ui.insertSpaces->isChecked();
    tabSettings->m_autoSpacesForTabs = m_d->m_ui.autoInsertSpaces->isChecked();
    tabSettings->m_autoIndent = m_d->m_ui.autoIndent->isChecked();
    tabSettings->m_smartBackspace = m_d->m_ui.smartBackspace->isChecked();
    tabSettings->m_tabSize = m_d->m_ui.tabSize->value();
    tabSettings->m_indentSize = m_d->m_ui.indentSize->value();
    tabSettings->m_indentBraces = m_d->m_ui.indentBlocksBehavior->currentIndex() >= 1;
    tabSettings->m_doubleIndentBlocks = m_d->m_ui.indentBlocksBehavior->currentIndex() >= 2;

    tabSettings->m_tabKeyBehavior =
        (TabSettings::TabKeyBehavior)m_d->m_ui.tabKeyBehavior->currentIndex();
    tabSettings->m_continuationAlignBehavior =
        (TabSettings::ContinuationAlignBehavior)m_d->m_ui.continuationAlignBehavior->currentIndex();
}

void BehaviorSettingsWidget::setAssignedStorageSettings(const StorageSettings &storageSettings)
{
    m_d->m_ui.cleanWhitespace->setChecked(storageSettings.m_cleanWhitespace);
    m_d->m_ui.inEntireDocument->setChecked(storageSettings.m_inEntireDocument);
    m_d->m_ui.cleanIndentation->setChecked(storageSettings.m_cleanIndentation);
    m_d->m_ui.addFinalNewLine->setChecked(storageSettings.m_addFinalNewLine);
}

void BehaviorSettingsWidget::assignedStorageSettings(StorageSettings *storageSettings) const
{
    storageSettings->m_cleanWhitespace = m_d->m_ui.cleanWhitespace->isChecked();
    storageSettings->m_inEntireDocument = m_d->m_ui.inEntireDocument->isChecked();
    storageSettings->m_cleanIndentation = m_d->m_ui.cleanIndentation->isChecked();
    storageSettings->m_addFinalNewLine = m_d->m_ui.addFinalNewLine->isChecked();
}

void BehaviorSettingsWidget::setAssignedBehaviorSettings(const BehaviorSettings &behaviorSettings)
{
    m_d->m_ui.mouseNavigation->setChecked(behaviorSettings.m_mouseNavigation);
    m_d->m_ui.scrollWheelZooming->setChecked(behaviorSettings.m_scrollWheelZooming);
}

void BehaviorSettingsWidget::assignedBehaviorSettings(BehaviorSettings *behaviorSettings) const
{
    behaviorSettings->m_mouseNavigation = m_d->m_ui.mouseNavigation->isChecked();
    behaviorSettings->m_scrollWheelZooming = m_d->m_ui.scrollWheelZooming->isChecked();
}

void BehaviorSettingsWidget::setAssignedExtraEncodingSettings(
    const ExtraEncodingSettings &encodingSettings)
{
    m_d->m_ui.utf8BomBox->setCurrentIndex(encodingSettings.m_utf8BomSetting);
}

void BehaviorSettingsWidget::assignedExtraEncodingSettings(
    ExtraEncodingSettings *encodingSettings) const
{
    encodingSettings->m_utf8BomSetting =
        (ExtraEncodingSettings::Utf8BomSetting)m_d->m_ui.utf8BomBox->currentIndex();
}

QString BehaviorSettingsWidget::collectUiKeywords() const
{
    static const QLatin1Char sep(' ');
    QString keywords;
    QTextStream(&keywords)
        << m_d->m_ui.insertSpaces->text()
        << sep << m_d->m_ui.autoInsertSpaces->text()
        << sep << m_d->m_ui.autoIndent->text()
        << sep << m_d->m_ui.smartBackspace->text()
        << sep << m_d->m_ui.indentBlocksLabel->text()
        << sep << m_d->m_ui.continuationAlignLabel->text()
        << sep << m_d->m_ui.tabKeyIndentLabel->text()
        << sep << m_d->m_ui.cleanWhitespace->text()
        << sep << m_d->m_ui.inEntireDocument->text()
        << sep << m_d->m_ui.cleanIndentation->text()
        << sep << m_d->m_ui.addFinalNewLine->text()
        << sep << m_d->m_ui.encodingLabel->text()
        << sep << m_d->m_ui.utf8BomLabel->text()
        << sep << m_d->m_ui.mouseNavigation->text()
        << sep << m_d->m_ui.scrollWheelZooming->text()
        << sep << m_d->m_ui.groupBoxTabAndIndentSettings->title()
        << sep << m_d->m_ui.groupBoxStorageSettings->title()
        << sep << m_d->m_ui.groupBoxEncodings->title()
        << sep << m_d->m_ui.groupBoxMouse->title();
    keywords.remove(QLatin1Char('&'));
    return keywords;
}

void BehaviorSettingsWidget::handleEncodingBoxChange(int index)
{
    emit textCodecChanged(m_d->m_codecs.at(index));
}

} // TextEditor
