/****************************************************************************
**
** Copyright (C) 2016 Thorben Kroeger <thorbenkroeger@gmail.com>.
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

#include "coreconstants.h"
#include "icore.h"
#include "manhattanstyle.h"
#include "themechooser.h"

#include <utils/algorithm.h>
#include <utils/theme/theme.h>
#include <utils/theme/theme_p.h>

#include <QAbstractListModel>
#include <QComboBox>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QSettings>
#include <QSpacerItem>

using namespace Utils;

static const char themeNameKey[] = "ThemeName";

namespace Core {
namespace Internal {

ThemeEntry::ThemeEntry(Id id, const QString &filePath)
    : m_id(id)
    , m_filePath(filePath)
{
}

Id ThemeEntry::id() const
{
    return m_id;
}

QString ThemeEntry::displayName() const
{
    if (m_displayName.isEmpty() && !m_filePath.isEmpty()) {
        QSettings settings(m_filePath, QSettings::IniFormat);
        m_displayName = settings.value(QLatin1String(themeNameKey),
                                       QCoreApplication::tr("unnamed")).toString();
    }
    return m_displayName;
}

QString ThemeEntry::filePath() const
{
    return m_filePath;
}

class ThemeListModel : public QAbstractListModel
{
public:
    ThemeListModel(QObject *parent = 0):
        QAbstractListModel(parent)
    {
    }

    int rowCount(const QModelIndex &parent) const override
    {
        return parent.isValid() ? 0 : m_themes.size();
    }

    QVariant data(const QModelIndex &index, int role) const override
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


class ThemeChooserPrivate
{
public:
    ThemeChooserPrivate(QWidget *widget);
    ~ThemeChooserPrivate();

public:
    ThemeListModel *m_themeListModel;
    QComboBox *m_themeComboBox;
};

ThemeChooserPrivate::ThemeChooserPrivate(QWidget *widget)
    : m_themeListModel(new ThemeListModel)
    , m_themeComboBox(new QComboBox)
{
    QHBoxLayout *layout = new QHBoxLayout(widget);
    layout->addWidget(m_themeComboBox);
    auto overriddenLabel = new QLabel;
    overriddenLabel->setText(ThemeChooser::tr("Current theme: %1")
                             .arg(creatorTheme()->displayName()));
    layout->addWidget(overriddenLabel);
    layout->setMargin(0);
    auto horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    layout->addSpacerItem(horizontalSpacer);
    m_themeComboBox->setModel(m_themeListModel);
    const QList<ThemeEntry> themes = ThemeEntry::availableThemes();
    const Id themeSetting = ThemeEntry::themeSetting();
    const int selected = Utils::indexOf(themes, Utils::equal(&ThemeEntry::id, themeSetting));
    m_themeListModel->setThemes(themes);
    if (selected >= 0)
        m_themeComboBox->setCurrentIndex(selected);
}

ThemeChooserPrivate::~ThemeChooserPrivate()
{
    delete m_themeListModel;
}

ThemeChooser::ThemeChooser(QWidget *parent) :
    QWidget(parent)
{
    d = new ThemeChooserPrivate(this);
}

ThemeChooser::~ThemeChooser()
{
    delete d;
}

void ThemeChooser::apply()
{
    const int index = d->m_themeComboBox->currentIndex();
    if (index == -1)
        return;
    const QString themeId = d->m_themeListModel->themeAt(index).id().toString();
    QSettings *settings = ICore::settings();
    const QString currentThemeId = ThemeEntry::themeSetting().toString();
    if (currentThemeId != themeId) {
        QMessageBox::information(ICore::mainWindow(), tr("Restart Required"),
                                 tr("The theme change will take effect after a restart of Qt Creator."));

        // save filename of selected theme in global config
        settings->setValue(QLatin1String(Constants::SETTINGS_THEME), themeId);
    }
}

static void addThemesFromPath(const QString &path, QList<ThemeEntry> *themes)
{
    static const QLatin1String extension("*.creatortheme");
    QDir themeDir(path);
    themeDir.setNameFilters({extension});
    themeDir.setFilter(QDir::Files);
    const QStringList themeList = themeDir.entryList();
    foreach (const QString &fileName, themeList) {
        QString id = QFileInfo(fileName).completeBaseName();
        themes->append(ThemeEntry(Id::fromString(id), themeDir.absoluteFilePath(fileName)));
    }
}

QList<ThemeEntry> ThemeEntry::availableThemes()
{
    QList<ThemeEntry> themes;

    static const QString installThemeDir = ICore::resourcePath() + QLatin1String("/themes");
    static const QString userThemeDir = ICore::userResourcePath() + QLatin1String("/themes");
    addThemesFromPath(installThemeDir, &themes);
    if (themes.isEmpty())
        qWarning() << "Warning: No themes found in installation: "
                   << QDir::toNativeSeparators(installThemeDir);
    // move default theme to front
    int defaultIndex = Utils::indexOf(themes, Utils::equal(&ThemeEntry::id, Id(Constants::DEFAULT_THEME)));
    if (defaultIndex > 0) { // == exists and not at front
        ThemeEntry defaultEntry = themes.takeAt(defaultIndex);
        themes.prepend(defaultEntry);
    }
    addThemesFromPath(userThemeDir, &themes);
    return themes;
}

Id ThemeEntry::themeSetting()
{
    return Id::fromSetting(ICore::settings()->value(QLatin1String(Constants::SETTINGS_THEME),
                                                    QLatin1String(Constants::DEFAULT_THEME)));
}

Theme *ThemeEntry::createTheme(Id id)
{
    if (!id.isValid())
        return nullptr;
    const ThemeEntry entry = Utils::findOrDefault(availableThemes(),
                                                  Utils::equal(&ThemeEntry::id, id));
    if (!entry.id().isValid())
        return nullptr;
    QSettings themeSettings(entry.filePath(), QSettings::IniFormat);
    Theme *theme = new Theme(entry.id().toString());
    theme->readSettings(themeSettings);
    return theme;
}

} // namespace Internal
} // namespace Core
