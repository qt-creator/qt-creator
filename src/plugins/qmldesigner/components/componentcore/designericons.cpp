// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "designericons.h"

#include <invalidargumentexception.h>

#include <QCache>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaEnum>
#include <QRegularExpression>

using namespace QmlDesigner;
namespace { // Blank namespace

template <typename EType>
struct DesignerIconEnums
{
    typedef EType EnumType;
    static QString toString(const EnumType &enumValue);
    static EnumType value(const QString &keyStr, bool *ok = nullptr);

    static const QMetaEnum metaEnum;
    static const QString keyName;
};

template <typename EType>
struct DesignerEnumConfidentType
{
    typedef EType EnumType;
};

template <>
struct DesignerEnumConfidentType<QIcon::Mode>
{
    typedef DesignerIcons::Mode EnumType;
};

template <>
struct DesignerEnumConfidentType<QIcon::State>
{
    typedef DesignerIcons::State EnumType;
};

template <typename T>
QString getEnumName() {
    QMetaEnum metaEnum = QMetaEnum::fromType<T>();
    QString enumName = QString::fromLatin1(metaEnum.enumName());
    if (enumName.size() && enumName.at(0).isUpper())
        enumName.replace(0, 1, enumName.at(0).toLower());
    return enumName;
};

template <>
QString getEnumName<Theme::Icon>() {
    return QLatin1String("iconName");
};

template <typename EType>
const QMetaEnum DesignerIconEnums<EType>::metaEnum =
        QMetaEnum::fromType<typename DesignerEnumConfidentType<EType>::EnumType>();

template <typename EType>
const QString DesignerIconEnums<EType>::keyName =
        getEnumName<typename DesignerEnumConfidentType<EType>::EnumType>();

template <typename EType>
QString DesignerIconEnums<EType>::toString(const EType &enumValue)
{
    return QString::fromLatin1(metaEnum.valueToKey(enumValue));
}

template <typename EType>
EType DesignerIconEnums<EType>::value(const QString &keyStr, bool *ok)
{
    return static_cast<EType>(metaEnum.keyToValue(keyStr.toLatin1(), ok));
}

QJsonObject mergeJsons(const QJsonObject &prior, const QJsonObject &joiner)
{
    QJsonObject object = prior;
    const QStringList joinerKeys = joiner.keys();
    for (const QString &joinerKey : joinerKeys) {
        if (!object.contains(joinerKey)) {
            object.insert(joinerKey, joiner.value(joinerKey));
        } else {
            QJsonValue ov = object.value(joinerKey);
            QJsonValue jv = joiner.value(joinerKey);
            if (ov.isObject() && jv.isObject()) {
                QJsonObject mg = mergeJsons(ov.toObject(), jv.toObject());
                object.insert(joinerKey, mg);
            }
        }
    }
    return object;
}

inline QString toJsonSize(const QSize &size)
{
    return QString::fromLatin1("%1x%2").arg(size.width()).arg(size.height());
}

template <typename EnumType>
void pushSimpleEnumValue(QJsonObject &object, const EnumType &enumVal)
{
    const QString &enumKey = DesignerIconEnums<EnumType>::keyName;
    QString enumValue = DesignerIconEnums<EnumType>::toString(enumVal);
    object.insert(enumKey, enumValue);
}

template <typename T>
T jsonSafeValue(const QJsonObject &jsonObject, const QString &symbolName,
                       std::function<bool (const T&)> validityCheck = [](const T&) -> bool {return true;})
{
    if (!jsonObject.contains(symbolName))
        throw InvalidArgumentException(__LINE__, __FUNCTION__, __FILE__, symbolName.toLatin1());

    QVariant symbolVar = jsonObject.value(symbolName);
    T extractedVal = symbolVar.value<T>();
    if (!validityCheck(extractedVal))
        throw InvalidArgumentException(__LINE__, __FUNCTION__, __FILE__, symbolName.toLatin1());

    return extractedVal;
}

QSize jsonSafeSize(const QJsonObject &jsonObject, const QString &symbolName)
{
    QString extractedVal = jsonSafeValue<QString>(jsonObject, symbolName);
    QStringList dims = extractedVal.split("x");
    if (dims.size() == 2) {
        bool wOk;
        bool hOk;
        int cWidth = dims.first().toInt(&wOk);
        int cHeight = dims.last().toInt(&hOk);
        if (wOk && hOk)
            return {cWidth, cHeight};
    }
    throw InvalidArgumentException(__LINE__, __FUNCTION__, __FILE__, symbolName.toLatin1());
    return {};
}

template <typename T>
T jsonSafeMetaEnum(const QJsonObject &jsonObject, const QString &symbolName = DesignerIconEnums<T>::keyName)
{
    QString extractedVal = jsonSafeValue<QString>(jsonObject, symbolName);
    bool ok;
    T enumIndex = static_cast<T> (DesignerIconEnums<T>::value(extractedVal, &ok));
    if (ok)
        return enumIndex;

    throw InvalidArgumentException(__LINE__, __FUNCTION__, __FILE__, symbolName.toLatin1());
    return {};
}

template <typename T>
struct JsonMap
{};

template <>
struct JsonMap<IconFontHelper>
{
    static IconFontHelper value(const QJsonObject &jsonObject, const QJsonObject &telescopingMap)
    {
        QJsonObject fontHelperJson = mergeJsons(jsonObject, telescopingMap);
        return IconFontHelper::fromJson(fontHelperJson);
    }

    static QJsonObject json(const IconFontHelper &iconFontHelper)
    {
        QJsonObject object;
        pushSimpleEnumValue(object, iconFontHelper.themeIcon());
        pushSimpleEnumValue(object, iconFontHelper.themeColor());
        object.insert("size", toJsonSize(iconFontHelper.size()));
        return object;
    }
};

template <typename Key, typename Value>
struct JsonMap<QMap<Key, Value>>
{
    typedef QMap<Key, Value> MapType;
    static MapType value(const QJsonObject &mapObject, const QJsonObject &telescopingMap)
    {
        typedef typename MapType::key_type KeyType;
        typedef typename MapType::mapped_type ValueType;

        QMap<KeyType, ValueType> output;
        QJsonObject curObject = mergeJsons(mapObject, telescopingMap);
        const QStringList keyList = curObject.keys();
        QStringList validKeys;
        QString keyTypeStr = DesignerIconEnums<KeyType>::keyName;
        QJsonObject nextTelescopingMap = telescopingMap;

        for (const QString &jsonKey : keyList) {
            bool keyAvailable = false;
            DesignerIconEnums<KeyType>::value(jsonKey, &keyAvailable);
            if (keyAvailable)
                validKeys.append(jsonKey);
            else
                nextTelescopingMap.insert(jsonKey, curObject.value(jsonKey));
        }

        for (const QString &jsonKey : validKeys) {
            bool keyAvailable = false;
            const KeyType key = DesignerIconEnums<KeyType>::value(jsonKey, &keyAvailable);
            QJsonValue jsonValue = curObject.value(jsonKey);

            nextTelescopingMap.insert(keyTypeStr, jsonKey);
            if (!jsonValue.isObject()) {
                qWarning() << Q_FUNC_INFO << __LINE__ << "Value of the" << jsonKey << "should be a json object.";
                continue;
            }
            output.insert(key, JsonMap<ValueType>::value(jsonValue.toObject(), nextTelescopingMap));
        }

        return output;
    }

    static QJsonObject json(const QMap<Key, Value> &map)
    {
        QJsonObject output;

        #if (QT_VERSION >= QT_VERSION_CHECK(6, 4, 0))
                for (const auto &[key, val] : map.asKeyValueRange())
                    output[DesignerIconEnums<Key>::toString(key)] = JsonMap<Value>::json(val);
        #else
                const auto mapKeys = map.keys();
                for (const Key &key : mapKeys) {
                    const Value &val = map.value(key);
                    output[DesignerIconEnums<Key>::toString(key)] = JsonMap<Value>::json(val);
                }
        #endif

        return output;
    }
};

void jsonParseErrorOffset(int &line,
                          int &offset,
                          const QJsonParseError &jpe,
                          const QString &filePath)
{
    line = -1;
    offset = -1;
    if (!jpe.error || jpe.offset < 0)
        return;

    QFile errorFile(filePath);
    if (!errorFile.open(QFile::ReadOnly)) {
        qWarning() << Q_FUNC_INFO << __LINE__ << "Cannot open file" << filePath
                   << "to get error line";
        return;
    }
    line = 0;
    QByteArray data;
    do {
        int linePos = errorFile.pos();
        data = errorFile.readLine();
        line++;
        if (jpe.offset < errorFile.pos()) {
            offset = jpe.offset - linePos;
            break;
        }
    } while (!errorFile.atEnd());
    errorFile.close();
}

} // End of blank namespace

class QmlDesigner::DesignerIconsPrivate
{
public:
    DesignerIconsPrivate(const QString &fontName)
        : mFontName(fontName) {}

    QString mFontName;
    DesignerIcons::IconsMap icons;
    static QCache<QString, DesignerIcons::IconsMap> cache;
};

QCache<QString, DesignerIcons::IconsMap> DesignerIconsPrivate::cache(100);

IconFontHelper::IconFontHelper(Theme::Icon themeIcon,
                               Theme::Color color,
                               const QSize &size,
                               QIcon::Mode mode,
                               QIcon::State state)
    : Super(Theme::getIconUnicode(themeIcon), Theme::getColor(color), size, mode, state)
    , mThemeIcon(themeIcon)
    , mThemeColor(color)
{}

IconFontHelper IconFontHelper::fromJson(const QJsonObject &jsonObject)
{
    try {
        Theme::Icon iconName = jsonSafeMetaEnum<Theme::Icon>(jsonObject);
        Theme::Color iconColor = jsonSafeMetaEnum<Theme::Color>(jsonObject);
        QSize iconSize = jsonSafeSize(jsonObject, "size");
        QIcon::Mode iconMode = jsonSafeMetaEnum<QIcon::Mode>(jsonObject);
        QIcon::State iconState = jsonSafeMetaEnum<QIcon::State>(jsonObject);
        return IconFontHelper(iconName, iconColor, iconSize, iconMode, iconState);
    } catch (const Exception &exception) {
        exception.showException("Faild to load IconFontHelper");
        return {};
    }
}

QJsonObject IconFontHelper::toJson() const
{
    QJsonObject jsonObject;
    pushSimpleEnumValue(jsonObject, themeIcon());
    pushSimpleEnumValue(jsonObject, themeColor());
    pushSimpleEnumValue(jsonObject, mode());
    pushSimpleEnumValue(jsonObject, state());
    jsonObject.insert("size", toJsonSize(size()));
    return jsonObject;
}

Theme::Icon IconFontHelper::themeIcon() const
{
    return mThemeIcon;
}

Theme::Color IconFontHelper::themeColor() const
{
    return mThemeColor;
}

IconFontHelper::IconFontHelper()
    : IconFontHelper({}, {}, {}, {}, {}) {}

DesignerIcons::DesignerIcons(const QString &fontName, const QString &iconDatabase)
    : d(new DesignerIconsPrivate(fontName))
{
    if (iconDatabase.size())
        loadIconSettings(iconDatabase);
}

DesignerIcons::~DesignerIcons()
{
    delete d;
}

QIcon DesignerIcons::icon(IconId icon, Area area) const
{
    return Utils::StyleHelper::getIconFromIconFont(d->mFontName, helperList(icon, area));
}

void DesignerIcons::loadIconSettings(const QString &fileName)
{
    if (d->cache.contains(fileName)) {
        d->icons = *d->cache.object(fileName);
        return;
    }

    QFile designerIconFile(fileName);

    if (!designerIconFile.open(QFile::ReadOnly)) {
        qWarning() << Q_FUNC_INFO << __LINE__ << "Can not open file:" << fileName << designerIconFile.errorString();
        return;
    }

    if (designerIconFile.size() > 100e3) {
        qWarning() << Q_FUNC_INFO << __LINE__ << "Large File:" << fileName;
        return;
    }

    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(designerIconFile.readAll(), &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        int line = 0;
        int offset = 0;
        jsonParseErrorOffset(line, offset, parseError, fileName);
        qWarning() << Q_FUNC_INFO << __LINE__ << "Json Parse Error - " << parseError.errorString()
                   << "---\t File: " << fileName << "---\t Line:" << line
                   << "---\t File Offset:" << offset;
        return;
    }

    if (!jsonDoc.isObject() && !jsonDoc.isArray()) {
        qWarning() << Q_FUNC_INFO << __LINE__ << "Invalid Json Array in file: " << fileName;
        return;
    }

    clearAll();
    if (jsonDoc.isObject()) {
        d->icons = JsonMap<IconsMap>::value(jsonDoc.object(), {});
    } else if (jsonDoc.isArray()) {
        DesignerIcons::IconsMap singleAreaMap;
        const QJsonArray jArray = jsonDoc.array();
        for (const QJsonValue &areaPack : jArray) {
            if (areaPack.isObject()) {
                QJsonObject areaPackObject = areaPack.toObject();
                singleAreaMap = JsonMap<IconsMap>::value(areaPackObject, {});

#if (QT_VERSION >= QT_VERSION_CHECK(6, 4, 0))
                for (const auto &mapItem : singleAreaMap.asKeyValueRange()) {
                    const IconId &id = mapItem.first;
                    const AreaMap &areaMap = mapItem.second;
#else
                const auto mapKeys = singleAreaMap.keys();
                for (const IconId &id : mapKeys) {
                    const AreaMap &areaMap = singleAreaMap.value(id);
#endif
                    if (d->icons.contains(id)) {
                        AreaMap &oldAreaMap = d->icons[id];
                        for (const auto &areaMapItem : areaMap.asKeyValueRange())
                            oldAreaMap.insert(areaMapItem.first, areaMapItem.second);
                    } else {
                        d->icons.insert(id, areaMap);
                    }
                }
            } else {
                qWarning() << Q_FUNC_INFO << __LINE__ << "Invalid Json Array in file: " << fileName;
                return;
            }
        }
    }

    d->cache.insert(fileName, new IconsMap(d->icons), 1);
}

void DesignerIcons::exportSettings(const QString &fileName) const
{
    QFile outFile(fileName);
    if (!outFile.open(QFile::WriteOnly)) {
        qWarning() << Q_FUNC_INFO << __LINE__ << "Can not open file for writing:" << fileName;
        return;
    }

    QJsonDocument jsonDocument;
    jsonDocument.setObject(JsonMap<IconsMap>::json(d->icons));

    outFile.write(jsonDocument.toJson());
    outFile.close();
}

void DesignerIcons::clearAll()
{
    d->icons.clear();
}

void DesignerIcons::addIcon(const IconId &iconId, const Area &area, const IconFontHelper &fontHelper)
{
    AreaMap &areaMap = d->icons[iconId];
    IconMap &iconMap = areaMap[area];
    ModeMap &modeMap = iconMap[static_cast<State>(fontHelper.state())];
    modeMap.insert(static_cast<Mode>(fontHelper.mode()), fontHelper);
}

void DesignerIcons::addIcon(IconId iconId,
                            Area area,
                            QIcon::Mode mode,
                            QIcon::State state,
                            Theme::Icon themeIcon,
                            Theme::Color color,
                            const QSize &size)
{
    addIcon(iconId, area, {themeIcon, color, size, mode, state});
}

void DesignerIcons::addIcon(IconId iconId, Area area, Theme::Icon themeIcon, Theme::Color color, const QSize &size)
{
    addIcon(iconId, area, {themeIcon, color, size});
}

QIcon DesignerIcons::rotateIcon(const QIcon &icon, const double &degrees)
{
    QIcon result;
    const QMetaEnum &modeMetaEnum = DesignerIconEnums<QIcon::Mode>().metaEnum;
    const QMetaEnum &stateMetaEnum = DesignerIconEnums<QIcon::State>().metaEnum;

    const int maxModeIdx = modeMetaEnum.keyCount();
    const int maxStateIdx = stateMetaEnum.keyCount();
    for (int modeI = 0; modeI < maxModeIdx; ++modeI) {
        const QIcon::Mode mode = static_cast<QIcon::Mode>(modeMetaEnum.value(modeI));
        for (int stateI = 0; stateI < maxStateIdx; ++stateI) {
            const QIcon::State state = static_cast<QIcon::State>(stateMetaEnum.value(stateI));
            const QList<QSize> iconSizes = icon.availableSizes();
            for (const QSize &size : iconSizes) {
                QTransform tr;
                tr.translate(size.width()/2, size.height()/2);
                tr.rotate(degrees);

                QPixmap pix = icon.pixmap(size, mode, state).transformed(tr);
                result.addPixmap(pix, mode, state);
            }
        }
    }

    return result;
}

QList<Utils::StyleHelper::IconFontHelper> DesignerIcons::helperList(const IconId &iconId,
                                                                    const Area &area) const
{
    QList<Utils::StyleHelper::IconFontHelper> helperList;
    const IconMap &iconMap = d->icons.value(iconId).value(area);
    for (const ModeMap &modMap : iconMap) {
        for (const IconFontHelper &iconFontHelper : modMap)
            helperList.append(iconFontHelper);
    }
    return helperList;
}
