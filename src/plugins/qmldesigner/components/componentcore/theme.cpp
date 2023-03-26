// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "theme.h"
#include "qmldesignericonprovider.h"

#include <qmldesignerplugin.h>

#include <coreplugin/icore.h>

#include <utils/stylehelper.h>

#include <QApplication>
#include <QMainWindow>
#include <QPointer>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlProperty>
#include <QRegularExpression>
#include <QScreen>
#include <qqml.h>

static Q_LOGGING_CATEGORY(themeLog, "qtc.qmldesigner.theme", QtWarningMsg)

namespace QmlDesigner {

Theme::Theme(Utils::Theme *originTheme, QObject *parent)
    : Utils::Theme(originTheme, parent)
    , m_constants(nullptr)
{
    QString constantsPath
        = Core::ICore::resourcePath(
                  "qmldesigner/propertyEditorQmlSources/imports/StudioTheme/InternalConstants.qml")
              .toString();

    QQmlEngine* engine = new QQmlEngine(this);
    QQmlComponent component(engine, QUrl::fromLocalFile(constantsPath));

    if (component.status() == QQmlComponent::Ready) {
        m_constants = component.create();
    }
    else if (component.status() == QQmlComponent::Error ) {
        qCWarning(themeLog) << "Couldn't load" << constantsPath
                            << "due to the following error(s):";
        for (const QQmlError &error : component.errors())
            qCWarning(themeLog) << error.toString();
    }
    else {
        qCWarning(themeLog) << "Couldn't load" << constantsPath
                            << "the status of the QQmlComponent is" << component.status();
    }
}

QColor Theme::evaluateColorAtThemeInstance(const QString &themeColorName)
{
    const QMetaObject &m = *metaObject();
    const QMetaEnum e = m.enumerator(m.indexOfEnumerator("Color"));
    for (int i = 0, total = e.keyCount(); i < total; ++i) {
        if (QString::fromLatin1(e.key(i)) == themeColorName)
            return color(static_cast<Utils::Theme::Color>(i));
    }

    qWarning() << Q_FUNC_INFO << "error while evaluating" << themeColorName;
    return {};
}

Theme *Theme::instance()
{
    static QPointer<Theme> qmldesignerTheme =
        new Theme(Utils::creatorTheme(), QmlDesigner::QmlDesignerPlugin::instance());
    return qmldesignerTheme;
}

QString Theme::replaceCssColors(const QString &input)
{
    const QRegularExpression rx("creatorTheme\\.(\\w+)");

    QString output = input;

    QRegularExpressionMatchIterator it = rx.globalMatch(input);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        const QString themeColorName = match.captured(1);
        const QRegularExpression replaceExp("creatorTheme\\." + themeColorName + "(\\s|;|\\n)");

        if (themeColorName == "smallFontPixelSize") {
            output.replace(replaceExp,
                           QString::number(instance()->smallFontPixelSize()) + "px" + "\\1");
        } else if (themeColorName == "captionFontPixelSize") {
            output.replace(replaceExp,
                           QString::number(instance()->captionFontPixelSize()) + "px" + "\\1");
        } else {
            const QColor color = instance()->evaluateColorAtThemeInstance(themeColorName);
            // Create rgba(r, g, b, a)
            const QString rgbaStr = QString("rgba(%1, %2, %3, %4)")
                                    .arg(color.red()).arg(color.green()).arg(color.blue())
                                    .arg(color.alpha());
            output.replace(replaceExp, rgbaStr + "\\1");
        }
    }

    return output;
}

void Theme::setupTheme(QQmlEngine *engine)
{
    [[maybe_unused]] static const int typeIndex = qmlRegisterSingletonType<Theme>(
        "QtQuickDesignerTheme", 1, 0, "Theme", [](QQmlEngine *, QJSEngine *) {
            return new Theme(Utils::creatorTheme(), nullptr);
        });

    engine->addImageProvider(QLatin1String("icons"), new QmlDesignerIconProvider());
}

QColor Theme::getColor(Theme::Color role)
{

    return instance()->color(role);

}

int Theme::smallFontPixelSize() const
{
    if (highPixelDensity())
        return 13;
    return 9;
}

int Theme::captionFontPixelSize() const
{
    if (highPixelDensity())
        return 14;
    return 11;
}

bool Theme::highPixelDensity() const
{
    return qApp->primaryScreen()->logicalDotsPerInch() > 100;
}

QWindow *Theme::mainWindowHandle() const
{
    return Core::ICore::mainWindow()->windowHandle();
}

QPixmap Theme::getPixmap(const QString &id)
{
    return QmlDesignerIconProvider::getPixmap(id);
}

QString Theme::getIconUnicode(Theme::Icon i)
{
    if (!instance()->m_constants)
        return QString();

    const QMetaObject *m = instance()->metaObject();
    const char *enumName = "Icon";
    int enumIndex = m->indexOfEnumerator(enumName);

    if (enumIndex == -1) {
        qCWarning(themeLog) << "Couldn't find enum" << enumName;
        return QString();
    }

    QMetaEnum e = m->enumerator(enumIndex);

    return instance()->m_constants->property(e.valueToKey(i)).toString();
}

QString Theme::getIconUnicode(const QString &name)
{
    return instance()->m_constants->property(name.toStdString().data()).toString();
}

QIcon Theme::iconFromName(Icon i, QColor c)
{
    QColor color = c;
    if (!color.isValid())
        color = getColor(Theme::Color::DSiconColor);

    const QString fontName = "qtds_propertyIconFont.ttf";
    return Utils::StyleHelper::getIconFromIconFont(fontName, Theme::getIconUnicode(i), 32, 32, color);
}

int Theme::toolbarSize()
{
    return 41;
}

QColor Theme::qmlDesignerBackgroundColorDarker() const
{
    return getColor(QmlDesigner_BackgroundColorDarker);
}

QColor Theme::qmlDesignerBackgroundColorDarkAlternate() const
{
    return getColor(QmlDesigner_BackgroundColorDarkAlternate);
}

QColor Theme::qmlDesignerTabLight() const
{
    return getColor(QmlDesigner_TabLight);
}

QColor Theme::qmlDesignerTabDark() const
{
    return getColor(QmlDesigner_TabDark);
}

QColor Theme::qmlDesignerButtonColor() const
{
    return getColor(QmlDesigner_ButtonColor);
}

QColor Theme::qmlDesignerBorderColor() const
{
    return getColor(QmlDesigner_BorderColor);
}

} // namespace QmlDesigner
