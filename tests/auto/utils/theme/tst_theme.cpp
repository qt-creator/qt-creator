// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/icon.h>
#include <utils/theme/theme.h>

#include <QFileInfo>
#include <QImage>
#include <QMetaEnum>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace Utils;

class tst_Theme : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void colorsFollowTheTheme();
    void iconsHandedOutBeforeTheChangeFollowTheTheme();
    void anEmptyHandlerRemovesTheEarlierOne();

private:
    Theme *theme(const QColor &color, Qt::ColorScheme colorScheme);

    QTemporaryDir m_dir;
    FilePath m_mask;
};

void tst_Theme::initTestCase()
{
    QVERIFY(m_dir.isValid());

    // A fully black mask turns into the plain tint color, so a single pixel of
    // the rendered icon tells which theme color it was rendered with. Only its
    // rgb does: the mask is applied to the alpha channel as well.
    QImage mask(4, 4, QImage::Format_ARGB32);
    mask.fill(Qt::black);
    m_mask = FilePath::fromString(m_dir.filePath("mask.png"));
    QVERIFY(mask.save(m_mask.toFSPathString()));
}

void tst_Theme::cleanup()
{
    setCreatorTheme(nullptr);
}

// A theme whose every color is the same, so that any color role can be read
// back as the color the theme was made with.
Theme *tst_Theme::theme(const QColor &color, Qt::ColorScheme colorScheme)
{
    const bool dark = colorScheme == Qt::ColorScheme::Dark;
    const QString rgb = color.name(QColor::HexRgb).mid(1);
    // The same color is asked for with either color scheme, and the file is
    // only written once, so the scheme has to be part of what identifies it.
    const QString id = rgb + (dark ? "-dark" : "-light");
    const QString path = m_dir.filePath(id + ".creatortheme");
    if (!QFileInfo::exists(path)) {
        QSettings settings(path, QSettings::IniFormat);
        settings.setValue("ThemeName", id);
        settings.beginGroup("Colors");
        const QMetaEnum colors = QMetaEnum::fromType<Theme::Color>();
        for (int i = 0; i < colors.keyCount(); ++i)
            settings.setValue(QLatin1String(colors.key(i)), rgb);
        settings.endGroup();
        settings.beginGroup("Flags");
        settings.setValue("DarkUserInterface", dark);
        settings.endGroup();
    }

    QSettings settings(path, QSettings::IniFormat);
    auto result = new Theme(id);
    result->readSettings(settings);
    return result;
}

void tst_Theme::colorsFollowTheTheme()
{
    setCreatorTheme(theme(Qt::red, Qt::ColorScheme::Light));
    QCOMPARE(creatorColor(Theme::TextColorNormal), QColor(Qt::red));

    const int generation = ThemeManager::generation();
    QSignalSpy changed(ThemeManager::instance(), &ThemeManager::changed);

    setCreatorTheme(theme(Qt::blue, Qt::ColorScheme::Dark));

    QCOMPARE(changed.count(), 1);
    QVERIFY(ThemeManager::generation() != generation);
    QCOMPARE(creatorColor(Theme::TextColorNormal), QColor(Qt::blue));
}

// Icons are handed to a QAction or a QWidget once, at construction, and are
// never set again. They have to resolve their colors when they are drawn.
void tst_Theme::iconsHandedOutBeforeTheChangeFollowTheTheme()
{
    setCreatorTheme(theme(Qt::red, Qt::ColorScheme::Light));

    const Icon icon({{m_mask, Theme::IconsBaseColor}}, Icon::Tint);
    const QIcon handedOut = icon.icon();
    const auto drawnColor = [&handedOut] {
        return handedOut.pixmap(4, 4).toImage().pixelColor(0, 0).rgb();
    };
    QCOMPARE(drawnColor(), QColor(Qt::red).rgb());

    setCreatorTheme(theme(Qt::blue, Qt::ColorScheme::Dark));

    QCOMPARE(drawnColor(), QColor(Qt::blue).rgb());
}

// Which is how QtcButton::setPixmap() keeps a plain pixmap that was set on top
// of a themed one, without disturbing the owner's other handlers.
void tst_Theme::anEmptyHandlerRemovesTheEarlierOne()
{
    setCreatorTheme(theme(Qt::red, Qt::ColorScheme::Light));

    QObject owner;
    int calls = 0;
    const auto count = [&calls] { ++calls; };
    ThemeManager::onChanged(&owner, "pixmap", count);
    ThemeManager::onChanged(&owner, "styleSheet", count);

    setCreatorTheme(theme(Qt::blue, Qt::ColorScheme::Dark));
    QCOMPARE(calls, 2);

    ThemeManager::onChanged(&owner, "pixmap", {});

    setCreatorTheme(theme(Qt::green, Qt::ColorScheme::Light));
    QCOMPARE(calls, 3);
}

QTEST_MAIN(tst_Theme)

#include "tst_theme.moc"
