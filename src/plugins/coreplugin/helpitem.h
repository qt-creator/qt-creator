// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <utils/filepath.h>

#include <QString>
#include <QUrl>
#include <QVariant>

#include <optional>
#include <vector>

QT_BEGIN_NAMESPACE
class QVersionNumber;
QT_END_NAMESPACE

namespace Core {

class CORE_EXPORT HelpItem
{
public:
    using Link = std::pair<QString, QUrl>;
    using Links = std::vector<Link>;
    using LinkNarrower = std::function<Links(const HelpItem &, const Links &)>;

    enum Category {
        ClassOrNamespace,
        Enum,
        Typedef,
        Macro,
        Brief,
        Function,
        QmlComponent,
        QmlProperty,
        QMakeVariableOfFunction,
        Unknown
    };

    HelpItem();
    HelpItem(const char *helpId);
    HelpItem(const QString &helpId);
    HelpItem(const QString &helpId,
             const Utils::FilePath &filePath,
             const QString &docMark,
             Category category);
    HelpItem(const QStringList &helpIds,
             const Utils::FilePath &filePath,
             const QString &docMark,
             Category category);
    explicit HelpItem(const QUrl &url);
    HelpItem(const QUrl &url, const QString &docMark, Category category);

    void setHelpUrl(const QUrl &url);
    const QUrl &helpUrl() const;

    void setHelpIds(const QStringList &ids);
    const QStringList &helpIds() const;

    void setDocMark(const QString &mark);
    const QString &docMark() const;

    void setCategory(Category cat);
    Category category() const;

    void setFilePath(const Utils::FilePath &filePath);
    Utils::FilePath filePath() const;

    bool isEmpty() const;
    bool isValid() const;

    QString firstParagraph() const;
    const Links &links() const;
    const Links bestLinks() const;
    const QString keyword() const;
    bool isFuzzyMatch() const;

    // used by QtSupport to narrow to "best" Qt version
    static void setLinkNarrower(const LinkNarrower &narrower);
    static std::pair<QUrl, QVersionNumber> extractQtVersionNumber(const QUrl &url);

private:
    QString extractContent(bool extended) const;

    QUrl m_helpUrl;
    QStringList m_helpIds;
    QString m_docMark;
    Category m_category = Unknown;
    Utils::FilePath m_filePath;
    mutable std::optional<Links> m_helpLinks; // cached help links
    mutable std::optional<QString> m_firstParagraph;
    mutable QString m_keyword;
    mutable bool m_isFuzzyMatch = false;
};

} // namespace Core

Q_DECLARE_METATYPE(Core::HelpItem)
