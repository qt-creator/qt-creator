// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "colorthemedialog.h"
#include "colorthemes.h"
#include "colorthemeview.h"
#include "scxmldocument.h"
#include "scxmleditorconstants.h"
#include "scxmleditortr.h"
#include "scxmltag.h"

#include <coreplugin/icore.h>

#include <utils/qtcsettings.h>
#include <utils/stringutils.h>

#include <QMenu>
#include <QToolButton>

using namespace Utils;

namespace ScxmlEditor::Common {

ColorThemes::ColorThemes(QObject *parent)
    : QObject(parent)
{
    m_modifyAction = new QAction(QIcon(":/scxmleditor/images/colorthemes.png"), Tr::tr("Modify Color Themes..."), this);
    m_modifyAction->setToolTip(Tr::tr("Modify Color Theme"));

    m_toolButton = new QToolButton;
    m_toolButton->setIcon(QIcon(":/scxmleditor/images/colorthemes.png"));
    m_toolButton->setToolTip(Tr::tr("Select Color Theme"));
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

    const QtcSettings *s = Core::ICore::settings();
    const QString currentTheme = s->value(Constants::C_SETTINGS_COLORSETTINGS_CURRENTCOLORTHEME,
                                          QString(Constants::C_COLOR_SCHEME_DEFAULT)).toString();
    const QVariantMap data = s->value(Constants::C_SETTINGS_COLORSETTINGS_COLORTHEMES).toMap();
    QStringList keys = data.keys();
    keys.append(Constants::C_COLOR_SCHEME_SCXMLDOCUMENT);
    keys.append(Constants::C_COLOR_SCHEME_DEFAULT);

    for (const QString &key: keys) {
        const QString actionText = key == Constants::C_COLOR_SCHEME_DEFAULT
                ? Tr::tr("Factory Default") : key == Constants::C_COLOR_SCHEME_SCXMLDOCUMENT
                  ? Tr::tr("Colors from SCXML Document")
                  : key;
        QAction *action = m_menu->addAction(actionText, this, [this, key] {
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
        QtcSettings *s = Core::ICore::settings();

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
            const QStringList colors = scxmlTag->editorInfo(Constants::C_SCXML_EDITORINFO_COLORS).split(";;", Qt::SkipEmptyParts);
            for (const QString &color : colors) {
                const QStringList colorInfo = color.split("_", Qt::SkipEmptyParts);
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

} // ScxmlEditor::Common
