/****************************************************************************
**
** Copyright (C) 2015 Thorben Kroeger <thorbenkroeger@gmail.com>.
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

#include "themesettingswidget.h"
#include "coreconstants.h"
#include "icore.h"
#include "manhattanstyle.h"
#include "themesettings.h"

#include <utils/algorithm.h>
#include <utils/theme/theme.h>
#include <utils/theme/theme_p.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QDir>
#include <QInputDialog>
#include <QMessageBox>
#include <QSettings>
#include <QStyleFactory>

#include "ui_themesettings.h"

using namespace Utils;

namespace Core {
namespace Internal {

class ThemeListModel : public QAbstractListModel
{
public:
    ThemeListModel(QObject *parent = 0):
        QAbstractListModel(parent)
    {
    }

    int rowCount(const QModelIndex &parent) const
    {
        return parent.isValid() ? 0 : m_themes.size();
    }

    QVariant data(const QModelIndex &index, int role) const
    {
        if (role == Qt::DisplayRole)
            return m_themes.at(index.row()).displayName();
        return QVariant();
    }

    void removeTheme(int index)
    {
        beginRemoveRows(QModelIndex(), index, index);
        m_themes.removeAt(index);
        endRemoveRows();
    }

    void setThemes(const QList<ThemeEntry> &themes)
    {
        beginResetModel();
        m_themes = themes;
        endResetModel();
    }

    const ThemeEntry &themeAt(int index) const
    {
        return m_themes.at(index);
    }

private:
    QList<ThemeEntry> m_themes;
};


class ThemeSettingsPrivate
{
public:
    ThemeSettingsPrivate(QWidget *widget);
    ~ThemeSettingsPrivate();

public:
    ThemeListModel *m_themeListModel;
    bool m_refreshingThemeList;
    Ui::ThemeSettings *m_ui;
};

ThemeSettingsPrivate::ThemeSettingsPrivate(QWidget *widget)
    : m_themeListModel(new ThemeListModel)
    , m_refreshingThemeList(false)
    , m_ui(new Ui::ThemeSettings)
{
    m_ui->setupUi(widget);
    m_ui->themeComboBox->setModel(m_themeListModel);
}

ThemeSettingsPrivate::~ThemeSettingsPrivate()
{
    delete m_themeListModel;
    delete m_ui;
}

ThemeSettingsWidget::ThemeSettingsWidget(QWidget *parent) :
    QWidget(parent)
{
    d = new ThemeSettingsPrivate(this);

    refreshThemeList();
}

ThemeSettingsWidget::~ThemeSettingsWidget()
{
    delete d;
}

void ThemeSettingsWidget::refreshThemeList()
{
    const QList<ThemeEntry> themes = ThemeSettings::availableThemes();
    const int selected = Utils::indexOf(themes, Utils::equal(&ThemeEntry::id,
                                                             Id::fromString(creatorTheme()->id())));

    d->m_refreshingThemeList = true;
    d->m_themeListModel->setThemes(themes);
    if (selected >= 0)
        d->m_ui->themeComboBox->setCurrentIndex(selected);
    d->m_refreshingThemeList = false;
}

void ThemeSettingsWidget::apply()
{
    const int index = d->m_ui->themeComboBox->currentIndex();
    if (index == -1)
        return;
    ThemeEntry currentTheme = d->m_themeListModel->themeAt(index);
    const QString themeId = currentTheme.id().toString();
    Theme *newTheme = new Theme(themeId);
    QSettings themeSettings(currentTheme.filePath(), QSettings::IniFormat);
    newTheme->readSettings(themeSettings);
    setCreatorTheme(newTheme);
    emit ICore::instance()->themeChanged();
    QPalette pal = newTheme->flag(Theme::ApplyThemePaletteGlobally) ? newTheme->palette()
                                                                    : Theme::initialPalette();
    QApplication::setPalette(pal);
    if (ManhattanStyle *style = qobject_cast<ManhattanStyle *>(QApplication::style())) {
        QStyle *baseStyle = 0;
        foreach (const QString &s, creatorTheme()->preferredStyles()) {
            if ((baseStyle = QStyleFactory::create(s)))
                break;
        }
        style->setBaseStyle(baseStyle);
    }
    foreach (QWidget *w, QApplication::topLevelWidgets())
        w->update();

    // save filename of selected theme in global config
    QSettings *settings = ICore::settings();
    settings->setValue(QLatin1String(Constants::SETTINGS_THEME), themeId);
}

} // namespace Internal
} // namespace Core
