// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <memory>
#include <vector>
#include <QFile>
#include <QDebug>

#include <vector>

namespace StudioWelcome {

struct UserPresetData
{
    QString categoryId;
    QString wizardName;
    QString name;
    QString screenSize;

    bool useQtVirtualKeyboard;
    QString qtVersion;
    QString styleName;

    bool isValid() const
    {
        return !categoryId.isEmpty()
            && !wizardName.isEmpty()
            && !name.isEmpty();
    }
};

inline QDebug &operator<<(QDebug &d, const UserPresetData &preset)
{
    d << "UserPreset{category = " << preset.categoryId;
    d << "; wizardName = " << preset.wizardName;
    d << "; name = " << preset.name;
    d << "; screenSize = " << preset.screenSize;
    d << "; keyboard = " << preset.useQtVirtualKeyboard;
    d << "; qt = " << preset.qtVersion;
    d << "; style = " << preset.styleName;
    d << "}";

    return d;
}

inline bool operator==(const UserPresetData &lhs, const UserPresetData &rhs)
{
    return lhs.categoryId == rhs.categoryId && lhs.wizardName == rhs.wizardName
           && lhs.name == rhs.name && lhs.screenSize == rhs.screenSize
           && lhs.useQtVirtualKeyboard == rhs.useQtVirtualKeyboard && lhs.qtVersion == rhs.qtVersion
           && lhs.styleName == rhs.styleName;;
}

enum class StorePolicy {UniqueNames, UniqueValues};

class StoreIo
{
public:
    virtual ~StoreIo() {}

    virtual QByteArray read() const = 0;
    virtual void write(const QByteArray &bytes) = 0;
};

class FileStoreIo : public StoreIo
{
public:
    explicit FileStoreIo(const QString &fileName);
    FileStoreIo(FileStoreIo &&other): m_file{std::move(other.m_file)} {}
    FileStoreIo& operator=(FileStoreIo &&other) { m_file = std::move(other.m_file); return *this; }

    QByteArray read() const override;
    void write(const QByteArray &data) override;

private:
    QString fullFilePath(const QString &fileName) const;

    std::unique_ptr<QFile> m_file;

    Q_DISABLE_COPY(FileStoreIo)
};

class UserPresetsStore
{
public:
    UserPresetsStore(const QString &fileName, StorePolicy policy);
    UserPresetsStore(std::unique_ptr<StoreIo> &&store, StorePolicy policy);

    bool save(const UserPresetData &preset);
    std::vector<UserPresetData> fetchAll() const;
    void remove(const QString &category, const QString &name);
    std::vector<UserPresetData> remove(const UserPresetData &preset);

    void setMaximum(int maximum) { m_maximum = maximum; }
    void setReverseOrder() { m_reverse = true; }

private:
    void savePresets(const std::vector<UserPresetData> &presets);

    std::unique_ptr<StoreIo> m_store;
    StorePolicy m_policy = StorePolicy::UniqueNames;
    bool m_reverse = false;
    int m_maximum = -1;
};

} // namespace StudioWelcome
