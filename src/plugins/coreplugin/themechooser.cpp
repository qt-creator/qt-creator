// Copyright (C) 2016 Thorben Kroeger <thorbenkroeger@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "themechooser.h"

#include "coreconstants.h"
#include "coreplugintr.h"
#include "dialogs/restartdialog.h"
#include "icore.h"

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
                                       Tr::tr("unnamed")).toString();
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
    ThemeListModel(QObject *parent = nullptr):
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
    auto layout = new QHBoxLayout(widget);
    layout->addWidget(m_themeComboBox);
    auto overriddenLabel = new QLabel;
    overriddenLabel->setText(Tr::tr("Current theme: %1").arg(creatorTheme()->displayName()));
    layout->addWidget(overriddenLabel);
    layout->setContentsMargins(0, 0, 0, 0);
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

static QString defaultThemeId()
{
    return Theme::systemUsesDarkMode() ? QString(Constants::DEFAULT_DARK_THEME)
                                       : QString(Constants::DEFAULT_THEME);
}

void ThemeChooser::apply()
{
    const int index = d->m_themeComboBox->currentIndex();
    if (index == -1)
        return;
    const QString themeId = d->m_themeListModel->themeAt(index).id().toString();
    QtcSettings *settings = ICore::settings();
    const QString currentThemeId = ThemeEntry::themeSetting().toString();
    if (currentThemeId != themeId) {
        // save filename of selected theme in global config
        settings->setValueWithDefault(Constants::SETTINGS_THEME, themeId, defaultThemeId());
        RestartDialog restartDialog(ICore::dialogParent(),
                                    Tr::tr("The theme change will take effect after restart."));
        restartDialog.exec();
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
        themes->append(ThemeEntry(Id::fromString(id), themeDir.absoluteFilePath(fileName)));
    }
}

QList<ThemeEntry> ThemeEntry::availableThemes()
{
    QList<ThemeEntry> themes;

    static const FilePath installThemeDir = ICore::resourcePath("themes");
    static const FilePath userThemeDir = ICore::userResourcePath("themes");
    addThemesFromPath(installThemeDir.toString(), &themes);
    if (themes.isEmpty())
        qWarning() << "Warning: No themes found in installation: "
                   << installThemeDir.toUserOutput();
    // move default theme to front
    int defaultIndex = Utils::indexOf(themes, Utils::equal(&ThemeEntry::id, Id(Constants::DEFAULT_THEME)));
    if (defaultIndex > 0) { // == exists and not at front
        ThemeEntry defaultEntry = themes.takeAt(defaultIndex);
        themes.prepend(defaultEntry);
    }
    addThemesFromPath(userThemeDir.toString(), &themes);
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

} // namespace Internal
} // namespace Core
