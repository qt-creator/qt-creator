/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "colorthemes.h"
#include "colorthemedialog.h"
#include "colorthemeview.h"
#include "scxmldocument.h"
#include "scxmleditorconstants.h"
#include "scxmltag.h"

#include <QMenu>

#include <coreplugin/icore.h>

using namespace ScxmlEditor::Common;

ColorThemes::ColorThemes(QObject *parent)
    : QObject(parent)
{
    m_modifyAction = new QAction(QIcon(":/scxmleditor/images/colorthemes.png"), tr("Modify Color Themes..."), this);
    m_modifyAction->setToolTip(tr("Modify Color Theme"));

    m_toolButton = new QToolButton;
    m_toolButton->setIcon(QIcon(":/scxmleditor/images/colorthemes.png"));
    m_toolButton->setToolTip(tr("Select Color Theme"));
    m_toolButton->setPopupMode(QToolButton::InstantPopup);

    m_menu = new QMenu;

    connect(m_modifyAction, &QAction::triggered, this, &ColorThemes::showDialog);

    updateColorThemeMenu();
}

ColorThemes::~ColorThemes()
{
    delete m_toolButton;
    delete m_menu;
}

QAction *ColorThemes::modifyThemeAction() const
{
    return m_modifyAction;
}

QToolButton *ColorThemes::themeToolButton() const
{
    return m_toolButton;
}

QMenu *ColorThemes::themeMenu() const
{
    return m_menu;
}

void ColorThemes::showDialog()
{
    ColorThemeDialog dialog;
    if (dialog.exec() == QDialog::Accepted) {
        dialog.save();
        updateColorThemeMenu();
    }
}

void ColorThemes::updateColorThemeMenu()
{
    m_menu->clear();

    const QSettings *s = Core::ICore::settings();
    const QString currentTheme = s->value(Constants::C_SETTINGS_COLORSETTINGS_CURRENTCOLORTHEME, Constants::C_COLOR_SCHEME_DEFAULT).toString();
    const QVariantMap data = s->value(Constants::C_SETTINGS_COLORSETTINGS_COLORTHEMES).toMap();
    QStringList keys = data.keys();
    keys.append(Constants::C_COLOR_SCHEME_SCXMLDOCUMENT);
    keys.append(Constants::C_COLOR_SCHEME_DEFAULT);

    for (const QString &key: keys) {
        const QString actionText = key == Constants::C_COLOR_SCHEME_DEFAULT
                ? tr("Factory Default") : key == Constants::C_COLOR_SCHEME_SCXMLDOCUMENT
                  ? tr("Colors from SCXML-document")
                  : key;
        QAction *action = m_menu->addAction(actionText, this, [this, key]() {
            selectColorTheme(key);
        });
        action->setData(key);
        action->setCheckable(true);
    }

    m_menu->addSeparator();
    m_menu->addAction(m_modifyAction);
    m_toolButton->setMenu(m_menu);

    selectColorTheme(currentTheme);
}

void ColorThemes::selectColorTheme(const QString &name)
{
    QVariantMap colorData;
    if (m_document && !name.isEmpty()) {
        QSettings *s = Core::ICore::settings();

        if (name == Constants::C_COLOR_SCHEME_SCXMLDOCUMENT) {
            colorData = m_documentColors;
            s->setValue(Constants::C_SETTINGS_COLORSETTINGS_CURRENTCOLORTHEME, name);
        } else if (name == Constants::C_COLOR_SCHEME_DEFAULT) {
            colorData = QVariantMap();
            s->setValue(Constants::C_SETTINGS_COLORSETTINGS_CURRENTCOLORTHEME, name);
        } else {
            const QVariantMap data = s->value(Constants::C_SETTINGS_COLORSETTINGS_COLORTHEMES).toMap();
            if (data.contains(name)) {
                colorData = data[name].toMap();
                s->setValue(Constants::C_SETTINGS_COLORSETTINGS_CURRENTCOLORTHEME, name);
            }
        }
    }

    QList<QAction*> actions = m_menu->actions();
    for (int i = 0; i < actions.count(); ++i)
        actions[i]->setChecked(actions[i]->data().toString() == name);

    setCurrentColors(colorData);
}

void ColorThemes::setDocument(ScxmlEditor::PluginInterface::ScxmlDocument *doc)
{
    m_document = doc;

    // Get document colors
    QVariantMap documentColors;
    if (m_document) {
        PluginInterface::ScxmlTag *scxmlTag = m_document->scxmlRootTag();
        if (scxmlTag && scxmlTag->hasEditorInfo(Constants::C_SCXML_EDITORINFO_COLORS)) {
            const QStringList colors = scxmlTag->editorInfo(Constants::C_SCXML_EDITORINFO_COLORS).split(";;", QString::SkipEmptyParts);
            for (const QString &color : colors) {
                const QStringList colorInfo = color.split("_", QString::SkipEmptyParts);
                if (colorInfo.count() == 2)
                    documentColors[colorInfo[0]] = colorInfo[1];
            }
        }
    }

    m_documentColors = documentColors;
    if (m_documentColors.isEmpty())
        updateColorThemeMenu();
    else
        selectColorTheme(Constants::C_COLOR_SCHEME_SCXMLDOCUMENT);
}

void ColorThemes::setCurrentColors(const QVariantMap &colorData)
{
    if (!m_document)
        return;

    QStringList serializedColors;

    QVector<QColor> colors = ColorThemeView::defaultColors();
    for (QVariantMap::const_iterator i = colorData.begin(); i != colorData.end(); ++i) {
        const int k = i.key().toInt();
        if (k >= 0 && k < colors.count()) {
            colors[k] = QColor(i.value().toString());
            serializedColors << QString::fromLatin1("%1_%2").arg(k).arg(colors[k].name());
        }
    }

    m_document->setLevelColors(colors);
    m_document->setEditorInfo(m_document->scxmlRootTag(), Constants::C_SCXML_EDITORINFO_COLORS, serializedColors.join(";;"));
}
