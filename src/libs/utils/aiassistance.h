// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "filepath.h"
#include "id.h"
#include "result.h"
#include "textutils.h"

#include <QtTaskTree/QTaskTree>

#include <QIcon>
#include <QImage>
#include <QList>
#include <QMap>
#include <QObject>
#include <QPointer>

#include <functional>
#include <optional>

QT_BEGIN_NAMESPACE
class QAction;
class QWidget;
QT_END_NAMESPACE

namespace Utils {

class QTCREATOR_UTILS_EXPORT AiContextItem
{
public:
    enum Kind { File, Text, Image };

    static AiContextItem fromFile(const FilePath &filePath,
                              const std::optional<Text::Range> &range = {});
    static AiContextItem fromText(const QString &name, const QString &text);
    static AiContextItem fromImage(const QString &name, const QImage &image);

    Kind kind = Text;
    QString name;
    FilePath filePath;
    std::optional<Text::Range> range;
    QString text;
    QImage image;
};

class QTCREATOR_UTILS_EXPORT AiRequest
{
public:
    enum Mode { Interactive, Headless };

    Id id;
    QString title;
    QString prompt;
    QList<AiContextItem> context;
    FilePath workingDirectory;
    Mode mode = Interactive;
};

class QTCREATOR_UTILS_EXPORT AiParameterChoice
{
public:
    QString value;
    QString displayName;
    QString description;
};

class QTCREATOR_UTILS_EXPORT AiTargetParameter
{
public:
    enum Category { Model, Effort, Mode, Other };

    QString id;
    QString displayName;
    Category category = Other;
    QList<AiParameterChoice> choices;
    QString defaultValue;
};

class QTCREATOR_UTILS_EXPORT AiTarget
{
public:
    QString id;
    QString displayName;
    QString description;
    QIcon icon;
    bool supportsHeadless = false;
    QList<AiTargetParameter> parameters;
};

class QTCREATOR_UTILS_EXPORT AiTargetSelection
{
public:
    Id providerId;
    QString targetId;
    QMap<QString, QString> parameters;

    friend bool operator==(const AiTargetSelection &, const AiTargetSelection &) = default;
};

class QTCREATOR_UTILS_EXPORT AiResponse : public QObject
{
    Q_OBJECT

public:
    AiResponse();
    ~AiResponse() override;

    bool isFinished() const;
    void cancel();

    // For providers.
    void reportText(const QString &chunk);
    void reportFinished(const Result<QString> &result);

signals:
    void textReceived(const QString &chunk);
    void finished(const Utils::Result<QString> &result);
    void cancelRequested();

private:
    bool m_finished = false;
};

class QTCREATOR_UTILS_EXPORT AiProvider : public QObject
{
    Q_OBJECT

public:
    AiProvider(Id id, const QString &displayName);
    ~AiProvider() override;

    Id id() const;
    QString displayName() const;

    QList<AiTarget> targets() const;
    std::optional<AiTarget> target(const QString &targetId) const;

    virtual void execute(const AiRequest &request,
                         const AiTargetSelection &selection,
                         AiResponse *response) = 0;

signals:
    void targetsChanged();

protected:
    void setTargets(const QList<AiTarget> &targets);

private:
    Id m_id;
    QString m_displayName;
    QList<AiTarget> m_targets;
};

class QTCREATOR_UTILS_EXPORT AiRequestRunner : public QObject
{
    Q_OBJECT

public:
    AiRequestRunner();
    ~AiRequestRunner() override;

    void setRequest(const AiRequest &request);
    void setSelection(const AiTargetSelection &selection);

    void start();
    Result<QString> result() const;

signals:
    void textReceived(const QString &chunk);
    void done(QtTaskTree::DoneResult result);

private:
    AiRequest m_request;
    std::optional<AiTargetSelection> m_selection;
    QPointer<AiResponse> m_response;
    Result<QString> m_result;
};

using AiRequestTask = QtTaskTree::QCustomTask<AiRequestRunner>;

namespace AiAssistance {

QTCREATOR_UTILS_EXPORT QList<AiProvider *> providers();
QTCREATOR_UTILS_EXPORT AiProvider *provider(Id providerId);
QTCREATOR_UTILS_EXPORT bool hasTargets(AiRequest::Mode mode = AiRequest::Interactive);
QTCREATOR_UTILS_EXPORT void onTargetsChanged(QObject *guard,
                                             const std::function<void()> &callback);

QTCREATOR_UTILS_EXPORT AiResponse *submit(const AiRequest &request);
QTCREATOR_UTILS_EXPORT AiResponse *submit(const AiRequest &request,
                                          const AiTargetSelection &selection);

QTCREATOR_UTILS_EXPORT std::optional<AiTargetSelection> rememberedSelection(Id requestId);

QTCREATOR_UTILS_EXPORT QAction *createAction(const QString &text,
                                             const std::function<AiRequest()> &request,
                                             QObject *parent);

QTCREATOR_UTILS_EXPORT QString toPlainText(const AiRequest &request);
QTCREATOR_UTILS_EXPORT QString excerpt(const FilePath &filePath, const Text::Range &range);
QTCREATOR_UTILS_EXPORT Id clipboardProviderId();

} // namespace AiAssistance

} // namespace Utils
