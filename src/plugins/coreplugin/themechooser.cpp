// Copyright (C) 2016 Thorben Kroeger <thorbenkroeger@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "themechooser.h"

#include "coreconstants.h"
#include "coreplugintr.h"
#include "icore.h"

#include <utils/algorithm.h>
#include <utils/theme/theme.h>
#include <utils/theme/theme_p.h>

#include <QAbstractListModel>
#include <QComboBox>
#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QSettings>
#include <QSpacerItem>

using namespace Utils;

namespace Core::Internal {

const char themeNameKey[] = "ThemeName";

ThemeEntry::ThemeEntry(Id id, const QString &filePath)
    : m_id(id)
    , m_filePath(filePath)
{}

Id ThemeEntry::id() const
{
    return m_id;
}

QString ThemeEntry::displayName() const
{
    if (m_displayName.isEmpty() && !m_filePath.isEmpty()) {
        QSettings settings(m_filePath, QSettings::IniFormat);
        m_displayName = settings.value(QLatin1String(themeNameKey),
                                       Tr::tr("unnamed")).toString();
    }
    return m_displayName;
}

QString ThemeEntry::filePath() const
{
    return m_filePath;
}

class ThemeListModel final : public QAbstractListModel
{
public:
    ThemeListModel() = default;

    int rowCount(const QModelIndex &parent) const final
    {
        return parent.isValid() ? 0 : m_themes.size();
    }

    QVariant data(const QModelIndex &index, int role) const final
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
    ThemeListModel m_themeListModel;
    QComboBox *m_themeComboBox;
};

ThemeChooser::ThemeChooser()
   : d(new ThemeChooserPrivate)
{
    d->m_themeComboBox = new QComboBox;

    const QList<ThemeEntry> themes = ThemeEntry::availableThemes();
    d->m_themeListModel.setThemes(themes);

    d->m_themeComboBox->setModel(&d->m_themeListModel);
    const int selected =
            Utils::indexOf(themes, Utils::equal(&ThemeEntry::id, ThemeEntry::themeSetting()));
    if (selected >= 0)
        d->m_themeComboBox->setCurrentIndex(selected);

    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(d->m_themeComboBox);
    layout->addWidget(new QLabel(Tr::tr("Current theme: %1").arg(creatorTheme()->displayName())));
    layout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
}

ThemeChooser::~ThemeChooser()
{
    delete d;
}

static QString defaultThemeId()
{
    switch (Theme::systemColorScheme()) {
    case Qt::ColorScheme::Light:
        return "light-2024";
    case Qt::ColorScheme::Dark:
        return "dark-2024";
    default:
        return "flat";
    }
}

void ThemeChooser::apply()
{
    const int index = d->m_themeComboBox->currentIndex();
    if (index == -1)
        return;
    const QString themeId = d->m_themeListModel.themeAt(index).id().toString();
    QtcSettings *settings = ICore::settings();
    const QString currentThemeId = ThemeEntry::themeSetting().toString();
    if (currentThemeId != themeId) {
        // save filename of selected theme in global config
        settings->setValueWithDefault(Constants::SETTINGS_THEME, themeId, defaultThemeId());
        ICore::askForRestart(Tr::tr("The theme change will take effect after restart."));
    }
}

static void addThemesFromPath(const QString &path, QList<ThemeEntry> *themes)
{
    static const QLatin1String extension("*.creatortheme");
    QDir themeDir(path);
    themeDir.setNameFilters({extension});
    themeDir.setFilter(QDir::Files);
    const QStringList themeList = themeDir.entryList();
    for (const QString &fileName : std::as_const(themeList)) {
        QString id = QFileInfo(fileName).completeBaseName();
        const bool addTheme = Core::ICore::isQtDesignStudio() == id.startsWith("design");
        if (addTheme)
            themes->append(ThemeEntry(Id::fromString(id), themeDir.absoluteFilePath(fileName)));
    }
}

QList<ThemeEntry> ThemeEntry::availableThemes()
{
    QList<ThemeEntry> themes;

    static const FilePath installThemeDir = ICore::resourcePath("themes");
    static const FilePath userThemeDir = ICore::userResourcePath("themes");
    addThemesFromPath(installThemeDir.toUrlishString(), &themes);
    if (themes.isEmpty())
        qWarning() << "Warning: No themes found in installation: "
                   << installThemeDir.toUserOutput();
    // move default theme to front
    const int defaultIndex = Utils::indexOf(themes, Utils::equal(&ThemeEntry::id,
                                                                 Id::fromString(defaultThemeId())));
    if (defaultIndex > 0) { // == exists and not at front
        ThemeEntry defaultEntry = themes.takeAt(defaultIndex);
        themes.prepend(defaultEntry);
    }
    addThemesFromPath(userThemeDir.toUrlishString(), &themes);
    return themes;
}

Id ThemeEntry::themeSetting()
{
    const Id setting = Id::fromSetting(
        ICore::settings()->value(Constants::SETTINGS_THEME, defaultThemeId()));

    const QList<ThemeEntry> themes = availableThemes();
    if (themes.empty())
        return Id();
    const bool settingValid = Utils::contains(themes, Utils::equal(&ThemeEntry::id, setting));

    return settingValid ? setting : themes.first().id();
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

} // namespace Core::Internal
