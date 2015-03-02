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
#include "themeeditor/themesettingstablemodel.h"

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

const char themeNameKey[] = "ThemeName";

static QString customThemesPath()
{
    return ICore::userResourcePath() + QLatin1String("/themes/");
}

static QString createThemeFileName(const QString &pattern)
{
    const QString stylesPath = customThemesPath();
    QString baseFileName = stylesPath;
    baseFileName += pattern;

    // Find an available file name
    int i = 1;
    QString fileName;
    do {
        fileName = baseFileName.arg((i == 1) ? QString() : QString::number(i));
        ++i;
    } while (QFile::exists(fileName));

    // Create the base directory when it doesn't exist
    if (!QFile::exists(stylesPath) && !QDir().mkpath(stylesPath)) {
        qWarning() << "Failed to create theme directory:" << stylesPath;
        return QString();
    }
    return fileName;
}


struct ThemeEntry
{
    ThemeEntry() : m_readOnly(true) {}
    ThemeEntry(const QString &name, const QString &filePath, bool readOnly):
        m_name(name),
        m_filePath(filePath),
        m_readOnly(readOnly)
    {
    }

    QString name() const { return m_name; }
    QString displayName() const;
    QString filePath() const { return m_filePath; }
    bool readOnly() const { return m_readOnly; }

private:
    QString m_name;
    QString m_filePath;
    mutable QString m_displayName;
    bool m_readOnly;
};

QString ThemeEntry::displayName() const
{
    if (m_displayName.isEmpty()) {
        QSettings settings(filePath(), QSettings::IniFormat);
        m_displayName = settings.value(QLatin1String(themeNameKey),
                                       QCoreApplication::tr("unnamed")).toString();
        if (false) // TODO: Revert to m_readOnly
            m_displayName = QCoreApplication::tr("%1 (built-in)").arg(m_displayName);
    }
    return m_displayName;
}


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
    ThemeEntry m_currentTheme;
};

ThemeSettingsPrivate::ThemeSettingsPrivate(QWidget *widget)
    : m_themeListModel(new ThemeListModel)
    , m_refreshingThemeList(false)
    , m_ui(new Ui::ThemeSettings)
{
    m_currentTheme = ThemeEntry(creatorTheme()->name(), creatorTheme()->filePath(), true);
    m_ui->setupUi(widget);
    // TODO: Restore the editor and the buttons after improving the editor
    m_ui->editor->hide();
    m_ui->copyButton->hide();
    m_ui->deleteButton->hide();
    m_ui->renameButton->hide();
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

    connect(d->m_ui->themeComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
           this, &ThemeSettingsWidget::themeSelected);
    connect(d->m_ui->copyButton, &QAbstractButton::clicked, this, &ThemeSettingsWidget::copyTheme);
    connect(d->m_ui->renameButton, &QAbstractButton::clicked, this, &ThemeSettingsWidget::renameTheme);
    connect(d->m_ui->deleteButton, &QAbstractButton::clicked, this, &ThemeSettingsWidget::confirmDeleteTheme);

    refreshThemeList();
}

ThemeSettingsWidget::~ThemeSettingsWidget()
{
    delete d;
}

void ThemeSettingsWidget::refreshThemeList()
{
    QList<ThemeEntry> themes;

    QDir themeDir(ICore::resourcePath() + QLatin1String("/themes"));
    themeDir.setNameFilters(QStringList() << QLatin1String("*.creatortheme"));
    themeDir.setFilter(QDir::Files);

    int selected = 0;

    QStringList themeList = themeDir.entryList();
    const QString defaultTheme = QLatin1String("default.creatortheme");
    if (themeList.removeOne(defaultTheme))
        themeList.prepend(defaultTheme);
    const QLatin1String extension(".creatortheme");
    for (int i = 0, total = themeList.count(); i < total; ++i) {
        const QString fileName = themeList.at(i);
        if (d->m_currentTheme.name() + extension == fileName)
            selected = i;
        QString name = fileName;
        name.remove(extension);
        themes.append(ThemeEntry(name, themeDir.absoluteFilePath(fileName), true));
    }

    if (themes.isEmpty())
        qWarning() << "Warning: no themes found in path:" << themeDir.path();

    themeDir.setPath(customThemesPath());
    foreach (const QString &file, themeDir.entryList()) {
        const QString fileName = themeDir.absoluteFilePath(file);
        if (d->m_currentTheme.name() == fileName)
            selected = themes.size();
        themes.append(ThemeEntry(fileName, fileName, false));
    }

    d->m_currentTheme = themes[selected];

    d->m_refreshingThemeList = true;
    d->m_themeListModel->setThemes(themes);
    d->m_ui->themeComboBox->setCurrentIndex(selected);
    d->m_refreshingThemeList = false;
}

void ThemeSettingsWidget::themeSelected(int index)
{
    bool readOnly = true;
    if (index != -1) {
        // Check whether we're switching away from a changed theme
        if (!d->m_refreshingThemeList)
            maybeSaveTheme();

        const ThemeEntry &entry = d->m_themeListModel->themeAt(index);
        readOnly = entry.readOnly();
        d->m_currentTheme = entry;

        QSettings settings(entry.filePath(), QSettings::IniFormat);
        Theme theme(entry.name());
        theme.readSettings(settings);
        d->m_ui->editor->initFrom(&theme);
    }
    d->m_ui->copyButton->setEnabled(index != -1);
    d->m_ui->deleteButton->setEnabled(!readOnly);
    d->m_ui->renameButton->setEnabled(!readOnly);
    d->m_ui->editor->setReadOnly(readOnly);
}

void ThemeSettingsWidget::confirmDeleteTheme()
{
    const int index = d->m_ui->themeComboBox->currentIndex();
    if (index == -1)
        return;

    const ThemeEntry &entry = d->m_themeListModel->themeAt(index);
    if (entry.readOnly())
        return;

    QMessageBox *messageBox = new QMessageBox(QMessageBox::Warning,
                                              tr("Delete Theme"),
                                              tr("Are you sure you want to delete the theme \"%1\" permanently?").arg(entry.displayName()),
                                              QMessageBox::Discard | QMessageBox::Cancel,
                                              d->m_ui->deleteButton->window());

    // Change the text and role of the discard button
    QPushButton *deleteButton = static_cast<QPushButton*>(messageBox->button(QMessageBox::Discard));
    deleteButton->setText(tr("Delete"));
    messageBox->addButton(deleteButton, QMessageBox::AcceptRole);
    messageBox->setDefaultButton(deleteButton);

    connect(deleteButton, &QAbstractButton::clicked, messageBox, &QDialog::accept);
    connect(messageBox, &QDialog::accepted, this, &ThemeSettingsWidget::deleteTheme);
    messageBox->setAttribute(Qt::WA_DeleteOnClose);
    messageBox->open();
}

void ThemeSettingsWidget::deleteTheme()
{
    const int index = d->m_ui->themeComboBox->currentIndex();
    QTC_ASSERT(index != -1, return);

    const ThemeEntry &entry = d->m_themeListModel->themeAt(index);
    QTC_ASSERT(!entry.readOnly(), return);

    if (QFile::remove(entry.filePath()))
        d->m_themeListModel->removeTheme(index);
}

void ThemeSettingsWidget::copyTheme()
{
    QInputDialog *dialog = new QInputDialog(d->m_ui->copyButton->window());
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setInputMode(QInputDialog::TextInput);
    dialog->setWindowTitle(tr("Copy Theme"));
    dialog->setLabelText(tr("Theme name:"));

    //TODO
    //dialog->setTextValue(tr("%1 (copy)").arg(d_ptr->m_value.colorScheme().displayName()));

    connect(dialog, &QInputDialog::textValueSelected, this, &ThemeSettingsWidget::copyThemeByName);
    dialog->open();
}

void ThemeSettingsWidget::maybeSaveTheme()
{
    if (!d->m_ui->editor->model()->hasChanges())
        return;

    QMessageBox *messageBox = new QMessageBox(QMessageBox::Warning,
                                              tr("Theme Changed"),
                                              tr("The theme \"%1\" was modified, do you want to save the changes?")
                                                  .arg(d->m_currentTheme.displayName()),
                                              QMessageBox::Discard | QMessageBox::Save,
                                              d->m_ui->themeComboBox->window());

    // Change the text of the discard button
    QPushButton *discardButton = static_cast<QPushButton*>(messageBox->button(QMessageBox::Discard));
    discardButton->setText(tr("Discard"));
    messageBox->addButton(discardButton, QMessageBox::DestructiveRole);
    messageBox->setDefaultButton(QMessageBox::Save);

    if (messageBox->exec() == QMessageBox::Save) {
        Theme newTheme(d->m_currentTheme.name());
        d->m_ui->editor->model()->toTheme(&newTheme);
        newTheme.writeSettings(d->m_currentTheme.filePath());
    }
}

void ThemeSettingsWidget::renameTheme()
{
    int index = d->m_ui->themeComboBox->currentIndex();
    if (index == -1)
        return;
    const ThemeEntry &entry = d->m_themeListModel->themeAt(index);

    maybeSaveTheme();

    QInputDialog *dialog = new QInputDialog(d->m_ui->renameButton->window());
    dialog->setInputMode(QInputDialog::TextInput);
    dialog->setWindowTitle(tr("Rename Theme"));
    dialog->setLabelText(tr("Theme name:"));
    dialog->setTextValue(d->m_ui->editor->model()->m_name);
    int ret = dialog->exec();
    QString newName = dialog->textValue();
    delete dialog;

    if (ret != QDialog::Accepted || newName.isEmpty())
        return;

    // overwrite file with new name
    Theme newTheme(entry.name());
    d->m_ui->editor->model()->toTheme(&newTheme);
    newTheme.setName(newName);
    newTheme.writeSettings(entry.filePath());

    refreshThemeList();
}

void ThemeSettingsWidget::copyThemeByName(const QString &name)
{
    int index = d->m_ui->themeComboBox->currentIndex();
    if (index == -1)
        return;

    const ThemeEntry &entry = d->m_themeListModel->themeAt(index);

    QString baseFileName = QFileInfo(entry.filePath()).completeBaseName();
    baseFileName += QLatin1String("_copy%1.creatortheme");
    QString fileName = createThemeFileName(baseFileName);

    if (fileName.isEmpty())
        return;

    // Ask about saving any existing modifactions
    maybeSaveTheme();

    Theme newTheme(fileName);
    d->m_ui->editor->model()->toTheme(&newTheme);
    newTheme.setName(name);
    newTheme.writeSettings(fileName);

    d->m_currentTheme = ThemeEntry(fileName, fileName, true);

    refreshThemeList();
}

void ThemeSettingsWidget::apply()
{
    const QString themeName = d->m_currentTheme.name();
    Theme *newTheme = new Theme(themeName);
    if (d->m_currentTheme.readOnly()) {
        QSettings themeSettings(d->m_currentTheme.filePath(), QSettings::IniFormat);
        newTheme->readSettings(themeSettings);
    } else {
        d->m_ui->editor->model()->toTheme(newTheme);
        newTheme->writeSettings(d->m_currentTheme.filePath());
    }
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
    settings->setValue(QLatin1String(Constants::SETTINGS_THEME), themeName);
}

} // namespace Internal
} // namespace Core
