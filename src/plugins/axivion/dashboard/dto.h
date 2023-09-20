#pragma once

/*
 * Copyright (C) 2022-current by Axivion GmbH
 * https://www.axivion.com/
 *
 * SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
 *
 * Purpose: Dashboard C++ API Header
 *
 * !!!!!! GENERATED, DO NOT EDIT !!!!!!
 *
 * This file was generated with the script at
 * <AxivionSuiteRepo>/projects/libs/dashboard_cpp_api/generator/generate_dashboard_cpp_api.py
 */

#include <QAnyStringView>
#include <QByteArray>
#include <QHashFunctions>
#include <QLatin1String>
#include <QString>
#include <QStringView>
#include <QtGlobal>

#include <array>
#include <cstddef>
#include <exception>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace Axivion::Internal::Dto
{
    class invalid_dto_exception : public std::runtime_error
    {
    public:
        invalid_dto_exception(const std::string_view type_name, const std::exception &ex);

        invalid_dto_exception(const std::string_view type_name, const std::string_view message);
    };

    template<typename O, typename I>
    std::optional<O> optionalTransform(const std::optional<I> &input, const std::function<O(const I&)> &transformer)
    {
        if (input.has_value())
        {
            return transformer(*input);
        }
        return std::nullopt;
    }

    class Serializable
    {
    public:
        virtual QByteArray serialize() const = 0;

        virtual ~Serializable() = default;
    };

    class Any : public Serializable
    {
    public:
        using Map = std::map<QString, Any>;
        using MapEntry = std::pair<const QString, Any>;
        using Vector = std::vector<Any>;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static Any deserialize(const QByteArray &json);

        Any();

        Any(QString value);

        Any(double value);

        Any(Map value);

        Any(Vector value);

        Any(bool value);

        bool isNull() const;

        bool isString() const;

        // Throws std::bad_variant_access
        QString &getString();

        // Throws std::bad_variant_access
        const QString &getString() const;

        bool isDouble() const;

        // Throws std::bad_variant_access
        double &getDouble();

        // Throws std::bad_variant_access
        const double &getDouble() const;

        bool isMap() const;

        // Throws std::bad_variant_access
        Map &getMap();

        // Throws std::bad_variant_access
        const Map &getMap() const;

        bool isList() const;

        // Throws std::bad_variant_access
        Vector &getList();

        // Throws std::bad_variant_access
        const Vector &getList() const;

        bool isBool() const;

        // Throws std::bad_variant_access
        bool &getBool();

        // Throws std::bad_variant_access
        const bool &getBool() const;

        virtual QByteArray serialize() const override;

    private:
        std::variant<
            std::nullptr_t, // .index() == 0
            QString, // .index() == 1
            double, // .index() == 2
            Map, // .index() == 3
            Vector, // .index() == 4
            bool // .index() == 5
        > data;
    };

    class ApiVersion
    {
    public:
        static const std::array<qint32, 4> number;
        static const QLatin1String string;
        static const QLatin1String name;
        static const QLatin1String timestamp;
    };

    /**
     * Describes an analyzed file in a version.
     */
    class AnalyzedFileDto : public Serializable
    {
    public:

        /**
         * <p>The absolute path of the file.
         */
        QString path;

        /**
         * <p>Indicates whether this file is a system header file.
         */
        std::optional<bool> isSystemHeader;

        /**
         * <p>The name of the language used to analyze this file.
         */
        std::optional<QString> languageName;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static AnalyzedFileDto deserialize(const QByteArray &json);

        AnalyzedFileDto(
            QString path,
            std::optional<bool> isSystemHeader,
            std::optional<QString> languageName
        );

        virtual QByteArray serialize() const override;
    };

    /**
     * Available types of ApiToken
     * 
     * <p>* `General` - Powerful token kind granting the same permissions as
     *   a regular password based login.
     * * `IdePlugin` - Limits user permissions to those typically needed by IDE
     *   plugins.
     * * `SourceFetch` - Used internally for local build. Cannot be created via this API.
     * * `LogIn` - Used internally by browsers for the &quot;Keep me logged in&quot; functionality. Cannot be created via this API.
     * * `ContinuousIntegration` - Limits user permissions to those typically needed for CI purposes.
     * 
     * <p>For the types `IdePlugin` and `LogIn` the Dashboard will automatically
     * delete the tokens when the owner changes his password. The Dashboard
     * will try to detect this for external password changes as well
     * but cannot guarantee this will always work.
     * 
     * @since 7.1.0
     */
    enum class ApiTokenType
    {
        sourcefetch,
        general,
        ideplugin,
        login,
        continuousintegration
    };

    class ApiTokenTypeMeta final
    {
    public:
        static const QLatin1String sourcefetch;
        static const QLatin1String general;
        static const QLatin1String ideplugin;
        static const QLatin1String login;
        static const QLatin1String continuousintegration;

        // Throws std::range_error
        static ApiTokenType strToEnum(QAnyStringView str);

        static QLatin1String enumToStr(ApiTokenType e);

        ApiTokenTypeMeta() = delete;
        ~ApiTokenTypeMeta() = delete;
    };

    /**
     * Request data for changing a user password
     */
    class ChangePasswordFormDto : public Serializable
    {
    public:

        /**
         * <p>The current password
         */
        QString currentPassword;

        /**
         * <p>The new password
         */
        QString newPassword;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static ChangePasswordFormDto deserialize(const QByteArray &json);

        ChangePasswordFormDto(
            QString currentPassword,
            QString newPassword
        );

        virtual QByteArray serialize() const override;
    };

    /**
     * Additional TypeInfo of a ColumnType
     */
    class ColumnTypeOptionDto : public Serializable
    {
    public:

        /**
         * <p>The name of the option that shall be used for displaying and filtering as well.
         */
        QString key;

        /**
         * <p>Name for displaying the option in UIs.
         * 
         * <p>Deprecated since 6.9.0. Use `key` instead. # Older aclipse versions rely on the field being non-null
         * 
         * @deprecated
         */
        std::optional<QString> displayName;

        /**
         * <p>A color hex code recommended for displaying the value in GUIs.
         * 
         * <p>Example colors are: &quot;#FF0000&quot; - red &quot;#008000&quot; - green
         */
        QString displayColor;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static ColumnTypeOptionDto deserialize(const QByteArray &json);

        ColumnTypeOptionDto(
            QString key,
            std::optional<QString> displayName,
            QString displayColor
        );

        virtual QByteArray serialize() const override;
    };

    /**
     * Describes a Comment Request
     */
    class CommentRequestDto : public Serializable
    {
    public:

        /**
         * <p>The comment text
         */
        QString text;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static CommentRequestDto deserialize(const QByteArray &json);

        CommentRequestDto(
            QString text
        );

        virtual QByteArray serialize() const override;
    };

    /**
     * CSRF token
     * 
     * @since 7.7.0
     */
    class CsrfTokenDto : public Serializable
    {
    public:

        /**
         * <p>The value expected to be sent along the ``csrfTokenHeader`` for all HTTP requests that are not GET, HEAD, OPTIONS or TRACE.
         * 
         * <p>Note, that this does not replace authentication of subsequent requests. Also the token is combined with the authentication
         * data, so will not work when authenticating as another user. Its lifetime is limited, so when creating a very long-running
         * application you should consider refreshing this token from time to time.
         */
        QString csrfToken;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static CsrfTokenDto deserialize(const QByteArray &json);

        CsrfTokenDto(
            QString csrfToken
        );

        virtual QByteArray serialize() const override;
    };

    /**
     * A Project Entity such as a Class, a Method, a File or a Module
     * or the System Entity.
     */
    class EntityDto : public Serializable
    {
    public:

        /**
         * <p>The project-wide ID used to refer to this entity.
         */
        QString id;

        /**
         * <p>a non-unique name of the entity
         */
        QString name;

        /**
         * <p>The type of the entity
         */
        QString type;

        /**
         * <p>The file path of an entity if it can be associated with a file
         */
        std::optional<QString> path;

        /**
         * <p>The line number of an entity if it can be associated with a file location
         */
        std::optional<qint32> line;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static EntityDto deserialize(const QByteArray &json);

        EntityDto(
            QString id,
            QString name,
            QString type,
            std::optional<QString> path,
            std::optional<qint32> line
        );

        virtual QByteArray serialize() const override;
    };

    /**
     * Describes an error returned from the server
     * 
     * <p>Usually they are caused by wrong API usage but they may also happen
     * in case of bugs in the server
     */
    class ErrorDto : public Serializable
    {
    public:

        /**
         * <p>a parseable version number indicating the server version
         * 
         * @since 6.6.0
         */
        std::optional<QString> dashboardVersionNumber;

        /**
         * <p>the name of the error kind
         */
        QString type;

        /**
         * <p>A human readable short english message describing the error.
         * Ideally it does not contain any linebreaks.
         */
        QString message;

        /**
         * <p>use this instead of message in order to display a message translated
         * according to your language preferences. Will contain exactly the same
         * contents as `message` in case no translation is available.
         */
        QString localizedMessage;

        /**
         * <p>Optional error details.
         * Consumers should expect this to be a multiline string and display it
         * in a mono-space font without adding additional linebreaks.
         * 
         * @since 7.5.0
         */
        std::optional<QString> details;

        /**
         * <p>Optional translation of `details` according to your language preferences.
         * Will not be available in case no translation is available.
         * 
         * @since 7.5.0
         */
        std::optional<QString> localizedDetails;

        /**
         * <p>E-mail address for support requests
         * 
         * @since 7.5.0
         */
        std::optional<QString> supportAddress;

        /**
         * <p>If this is `true`, this is an indication by the server, that this error
         * is probably a bug on server-side and clients are encouraged to encourage
         * users to escalate this problem, e.g. using `supportAddress`.
         * 
         * <p>Keep in mind that this boolean may be wrong in both directions, e.g.:
         * * if a timeout is configured too low, then this can be a false positive
         * * if the bug lies in the exception judgment than this is a false negative
         * 
         * <p>So it should always be clear to users that they have the ultimate choice
         * on what to do with an error popup.
         * 
         * @since 7.5.0
         */
        std::optional<bool> displayServerBugHint;

        /**
         * <p>Optional field containing additional error information meant for automatic processing.
         * 
         * <p>* This data is meant for helping software that uses the API to better understand and communicate
         *   certain types of error to the user.
         * * Always inspect the `type` so you know what keys you can expect.
         * 
         * <p>Error types having additional information:
         * 
         * <p>  * type = InvalidFilterException (since 6.5.0):
         * 
         * <p>    * optionally has a string datum &#x27;column&#x27; referencing the column that has the invalid
         *       filter value. The file filter is referred to by the string &quot;any path&quot;.
         *     * optionally has a string datum &#x27;help&#x27; providing an ascii-encoded URL
         *       pointing to human-readable help that might help a user understanding
         *       and resolving the error. If the URL is relative, then it is meant
         *       relative to the Dashboard the error originated from.
         * 
         * <p>  * type = PasswordVerificationException (since 7.1.0):
         * 
         * <p>    * optionally has a boolean flag &#x27;passwordMayBeUsedAsApiToken&#x27; to indicate that the
         *       provided password may be used as API token with the respective API. E.g. use
         *       &#x27;Authorization: AxToken ...&#x27; header instead of HTTP basic auth.
         * 
         * @since 6.5.0
         */
        std::optional<std::map<QString, Any>> data;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static ErrorDto deserialize(const QByteArray &json);

        ErrorDto(
            std::optional<QString> dashboardVersionNumber,
            QString type,
            QString message,
            QString localizedMessage,
            std::optional<QString> details,
            std::optional<QString> localizedDetails,
            std::optional<QString> supportAddress,
            std::optional<bool> displayServerBugHint,
            std::optional<std::map<QString, Any>> data
        );

        virtual QByteArray serialize() const override;
    };

    /**
     * Describes an issue comment
     */
    class IssueCommentDto : public Serializable
    {
    public:

        /**
         * <p>The loginname of the user that created the comment.
         */
        QString username;

        /**
         * <p>The recommended display name of the user that wrote the comment.
         */
        QString userDisplayName;

        /**
         * <p>The Date when the comment was created as a ISO8601-parseable string.
         */
        QString date;

        /**
         * <p>The Date when the comment was created for UI-display.
         * 
         * <p>It is formatted as a human-readable string relative to query time, e.g.
         * ``2 minutes ago``.
         */
        QString displayDate;

        /**
         * <p>The comment text.
         */
        QString text;

        /**
         * <p>The linkified comment text.
         * 
         * @since 7.6.0
         */
        std::optional<QString> html;

        /**
         * <p>The id for comment deletion.
         * 
         * <p>When the requesting user is allowed to delete the comment, contains an id
         * that can be used to mark the comment as deleted using another API. This is
         * never set when the Comment is returned as the result of an Issue-List query.
         */
        std::optional<QString> commentDeletionId;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static IssueCommentDto deserialize(const QByteArray &json);

        IssueCommentDto(
            QString username,
            QString userDisplayName,
            QString date,
            QString displayDate,
            QString text,
            std::optional<QString> html,
            std::optional<QString> commentDeletionId
        );

        virtual QByteArray serialize() const override;
    };

    /**
     * Shortname for an erosion issue kind
     * 
     * <p>one of ``AV``, ``CL``, ``CY``, ``DE``, ``MV``, ``SV``
     */
    enum class IssueKind
    {
        av,
        cl,
        cy,
        de,
        mv,
        sv
    };

    class IssueKindMeta final
    {
    public:
        static const QLatin1String av;
        static const QLatin1String cl;
        static const QLatin1String cy;
        static const QLatin1String de;
        static const QLatin1String mv;
        static const QLatin1String sv;

        // Throws std::range_error
        static IssueKind strToEnum(QAnyStringView str);

        static QLatin1String enumToStr(IssueKind e);

        IssueKindMeta() = delete;
        ~IssueKindMeta() = delete;
    };

    /**
     * IssueKind for Named-Filter creation
     * 
     * <p>one of ``AV``, ``CL``, ``CY``, ``DE``, ``MV``, ``SV``, ``UNIVERSAL``
     */
    enum class IssueKindForNamedFilterCreation
    {
        av,
        cl,
        cy,
        de,
        mv,
        sv,
        universal
    };

    class IssueKindForNamedFilterCreationMeta final
    {
    public:
        static const QLatin1String av;
        static const QLatin1String cl;
        static const QLatin1String cy;
        static const QLatin1String de;
        static const QLatin1String mv;
        static const QLatin1String sv;
        static const QLatin1String universal;

        // Throws std::range_error
        static IssueKindForNamedFilterCreation strToEnum(QAnyStringView str);

        static QLatin1String enumToStr(IssueKindForNamedFilterCreation e);

        IssueKindForNamedFilterCreationMeta() = delete;
        ~IssueKindForNamedFilterCreationMeta() = delete;
    };

    /**
     * <p>Identifies a source code location which may be a single line of code or an
     * couple of adjacent lines (fragment) inside the same source file.
     */
    class IssueSourceLocationDto : public Serializable
    {
    public:

        /**
         * <p>Refers to the file with a normalized path relative to the current project
         * root.
         */
        QString fileName;

        /**
         * <p>A user-readable description of this source location&#x27;s role in the
         * issue (e.g. &#x27;Source&#x27; or &#x27;Target&#x27;)
         */
        std::optional<QString> role;

        /**
         * <p>Host-relative URL of the source code version for which these
         * startLine/endLine values are valid
         */
        QString sourceCodeUrl;

        /**
         * <p>The first line of the fragment
         */
        qint32 startLine;

        /**
         * The column of the start line in which the issue starts.
         * 
         * <p>1-relative
         * 
         * <p>0 iff unknown
         * 
         * @since 7.2.0
         */
        qint32 startColumn;

        /**
         * The last line of the fragment (inclusive)
         * 
         * <p>This is the same as `startLine` if the location has only one line.
         */
        qint32 endLine;

        /**
         * The column of the end line in which the issue ends.
         * 
         * <p>1-relative
         * 
         * <p>0 iff unknown
         * 
         * @since 7.2.0
         */
        qint32 endColumn;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static IssueSourceLocationDto deserialize(const QByteArray &json);

        IssueSourceLocationDto(
            QString fileName,
            std::optional<QString> role,
            QString sourceCodeUrl,
            qint32 startLine,
            qint32 startColumn,
            qint32 endLine,
            qint32 endColumn
        );

        virtual QByteArray serialize() const override;
    };

    /**
     * An issue tag as returned by the Issue-List API.
     */
    class IssueTagDto : public Serializable
    {
    public:

        /**
         * <p>Use this for displaying the tag
         */
        QString tag;

        /**
         * <p>An RGB hex color in the form #RRGGBB directly usable by css.
         * 
         * <p>The colors are best suited to draw a label on bright background and to
         * contain white letters for labeling.
         */
        QString color;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static IssueTagDto deserialize(const QByteArray &json);

        IssueTagDto(
            QString tag,
            QString color
        );

        virtual QByteArray serialize() const override;
    };

    /**
     * Describes an issue tag as returned by the Issue-Tags API.
     */
    class IssueTagTypeDto : public Serializable
    {
    public:

        /**
         * <p>A canonicalized variant of the tag name that can be used for client-side equality checks and sorting.
         */
        QString id;

        /**
         * <p>Deprecated since 6.9.0, use ``tag`` instead.
         * 
         * @deprecated
         */
        std::optional<QString> text;

        /**
         * <p>Use this for displaying the tag
         * 
         * @since 6.9.0
         */
        std::optional<QString> tag;

        /**
         * <p>An RGB hex color in the form #RRGGBB directly usable by css.
         * 
         * <p>The colors are best suited to draw a label on bright background and to
         * contain white letters for labeling.
         */
        QString color;

        /**
         * <p>The description of the tag. It can be assumed to be plain text with no need for further
         * syntactic interpretation.
         */
        std::optional<QString> description;

        /**
         * <p>Whether the tag is attached to the issue or only proposed.
         */
        std::optional<bool> selected;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static IssueTagTypeDto deserialize(const QByteArray &json);

        IssueTagTypeDto(
            QString id,
            std::optional<QString> text,
            std::optional<QString> tag,
            QString color,
            std::optional<QString> description,
            std::optional<bool> selected
        );

        virtual QByteArray serialize() const override;
    };

    /**
     * The log-level of a message
     * 
     * <p>* `DEBUG`
     * * `INFO`
     * * `WARNING`
     * * `ERROR`
     * * `FATAL`
     */
    enum class MessageSeverity
    {
        debug,
        info,
        warning,
        error,
        fatal
    };

    class MessageSeverityMeta final
    {
    public:
        static const QLatin1String debug;
        static const QLatin1String info;
        static const QLatin1String warning;
        static const QLatin1String error;
        static const QLatin1String fatal;

        // Throws std::range_error
        static MessageSeverity strToEnum(QAnyStringView str);

        static QLatin1String enumToStr(MessageSeverity e);

        MessageSeverityMeta() = delete;
        ~MessageSeverityMeta() = delete;
    };

    /**
     * Describes a Metric as configured for a project in a version
     */
    class MetricDto : public Serializable
    {
    public:

        /**
         * <p>The ID of the metric
         */
        QString name;

        /**
         * <p>a more descriptive name of the metric
         */
        QString displayName;

        /**
         * <p>The configured minimum threshold for the metric.
         * 
         * <p>Can have two possible string values ``-Infinity`` and ``Infinity``
         * otherwise it is a number. If not configured, this field will not
         * be available.
         */
        Any minValue;

        /**
         * <p>The configured maximum threshold for the metric.
         * 
         * <p>Can have two possible string values ``-Infinity`` and ``Infinity``
         * otherwise it is a number. If not configured, this field will not
         * be available.
         */
        Any maxValue;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static MetricDto deserialize(const QByteArray &json);

        MetricDto(
            QString name,
            QString displayName,
            Any minValue,
            Any maxValue
        );

        virtual QByteArray serialize() const override;
    };

    /**
     * An entity table row
     * 
     * <p>Note, that if you specify ``forceStrings=true`` when querying the table,
     * the field types will all be ``string`` and the empty string will indicate
     * an absent value.
     */
    class MetricValueTableRowDto : public Serializable
    {
    public:

        /**
         * <p>The Metric Id
         */
        QString metric;

        /**
         * <p>The source file of the entity definition
         */
        std::optional<QString> path;

        /**
         * <p>The source file line number of the entity definition
         */
        std::optional<qint32> line;

        /**
         * <p>The measured or aggregated metric value
         */
        std::optional<double> value;

        /**
         * <p>The non-unique entity name
         */
        QString entity;

        /**
         * <p>The entity type
         */
        QString entityType;

        /**
         * <p>The project-wide entity ID
         */
        QString entityId;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static MetricValueTableRowDto deserialize(const QByteArray &json);

        MetricValueTableRowDto(
            QString metric,
            std::optional<QString> path,
            std::optional<qint32> line,
            std::optional<double> value,
            QString entity,
            QString entityType,
            QString entityId
        );

        virtual QByteArray serialize() const override;
    };

    /**
     * A named filter type
     * 
     * <p>* `PREDEFINED` - Named filters of this type are immutable and exist out of the box and can be used by everyone
     * * `GLOBAL` - Named filters of this type are usable by everyone and managed by the so called filter managers
     * * `CUSTOM` - Named filters of this type are creatable by everyone but only visible to their owner
     */
    enum class NamedFilterType
    {
        predefined,
        global,
        custom
    };

    class NamedFilterTypeMeta final
    {
    public:
        static const QLatin1String predefined;
        static const QLatin1String global;
        static const QLatin1String custom;

        // Throws std::range_error
        static NamedFilterType strToEnum(QAnyStringView str);

        static QLatin1String enumToStr(NamedFilterType e);

        NamedFilterTypeMeta() = delete;
        ~NamedFilterTypeMeta() = delete;
    };

    /**
     * NamedFilter visibility configuration
     * 
     * <p>Only applicable for global named filters.
     * 
     * <p>You may not have access to this information depending on your permissions.
     * 
     * @since 7.3.0
     */
    class NamedFilterVisibilityDto : public Serializable
    {
    public:

        /**
         * <p>IDs of user-groups, that are allowed to see the named filter.
         * 
         * <p>The named filter is visible to all users, if this property is not given.
         */
        std::optional<std::vector<QString>> groups;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static NamedFilterVisibilityDto deserialize(const QByteArray &json);

        NamedFilterVisibilityDto(
            std::optional<std::vector<QString>> groups
        );

        virtual QByteArray serialize() const override;
    };

    /**
     * A reference to a project
     */
    class ProjectReferenceDto : public Serializable
    {
    public:

        /**
         * <p>The name of the project. Use this string to refer to the project.
         */
        QString name;

        /**
         * <p>URI to get further information about the project.
         */
        QString url;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static ProjectReferenceDto deserialize(const QByteArray &json);

        ProjectReferenceDto(
            QString name,
            QString url
        );

        virtual QByteArray serialize() const override;
    };

    /**
     * A rule configured on a project
     * 
     * @since 6.5.0
     */
    class RuleDto : public Serializable
    {
    public:

        /**
         * <p>the rule name (possibly renamed via configuration)
         */
        QString name;

        /**
         * <p>the original (unrenamed) rule name
         */
        QString original_name;

        /**
         * <p>Whether or not the rule was disabled.
         * 
         * <p>Note, that this value is only available for analysis runs
         * done with at least 6.5.0
         */
        std::optional<bool> disabled;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static RuleDto deserialize(const QByteArray &json);

        RuleDto(
            QString name,
            QString original_name,
            std::optional<bool> disabled
        );

        virtual QByteArray serialize() const override;
    };

    /**
     * A sorting direction
     * 
     * <p>* `ASC` - `smaller` values first
     * * `DESC` - `greater` values first
     */
    enum class SortDirection
    {
        asc,
        desc
    };

    class SortDirectionMeta final
    {
    public:
        static const QLatin1String asc;
        static const QLatin1String desc;

        // Throws std::range_error
        static SortDirection strToEnum(QAnyStringView str);

        static QLatin1String enumToStr(SortDirection e);

        SortDirectionMeta() = delete;
        ~SortDirectionMeta() = delete;
    };

    /**
     * How the column values should be aligned.
     * 
     * <p>* `left` - Align value to the left of its cell
     * * `right` - Align value to the right of its cell
     * * `center` - Center value in its cell
     */
    enum class TableCellAlignment
    {
        left,
        right,
        center
    };

    class TableCellAlignmentMeta final
    {
    public:
        static const QLatin1String left;
        static const QLatin1String right;
        static const QLatin1String center;

        // Throws std::range_error
        static TableCellAlignment strToEnum(QAnyStringView str);

        static QLatin1String enumToStr(TableCellAlignment e);

        TableCellAlignmentMeta() = delete;
        ~TableCellAlignmentMeta() = delete;
    };

    /**
     * Refers to a specific version of the Axivion Suite
     */
    class ToolsVersionDto : public Serializable
    {
    public:

        /**
         * <p>Version number for display purposes
         */
        QString name;

        /**
         * <p>Parseable, numeric version number suitable for version comparisons
         */
        QString number;

        /**
         * <p>Build date in an ISO8601-parseable string of the form YYYY-MM-DD
         */
        QString buildDate;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static ToolsVersionDto deserialize(const QByteArray &json);

        ToolsVersionDto(
            QString name,
            QString number,
            QString buildDate
        );

        virtual QByteArray serialize() const override;
    };

    /**
     * User Type
     * 
     * <p>* `VIRTUAL_USER` - virtual user that does not represent a specific person
     * * `DASHBOARD_USER` - a `regular`, explicitly managed user
     * * `UNMAPPED_USER` - a user that is not explicitly managed and e.g. only known via an analysis result
     * 
     * @since 7.1.0
     */
    enum class UserRefType
    {
        virtual_user,
        dashboard_user,
        unmapped_user
    };

    class UserRefTypeMeta final
    {
    public:
        static const QLatin1String virtual_user;
        static const QLatin1String dashboard_user;
        static const QLatin1String unmapped_user;

        // Throws std::range_error
        static UserRefType strToEnum(QAnyStringView str);

        static QLatin1String enumToStr(UserRefType e);

        UserRefTypeMeta() = delete;
        ~UserRefTypeMeta() = delete;
    };

    /**
     * Kind-specific issue count statistics that are cheaply available.
     */
    class VersionKindCountDto : public Serializable
    {
    public:

        /**
         * <p>The number of issues of a kind in a version.
         */
        qint32 Total;

        /**
         * <p>The number of issues of a kind present in a version that were not present in the previous version.
         */
        qint32 Added;

        /**
         * <p>The number of issues of a kind that were present in the previous version and are not present in the current version any more.
         */
        qint32 Removed;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static VersionKindCountDto deserialize(const QByteArray &json);

        VersionKindCountDto(
            qint32 Total,
            qint32 Added,
            qint32 Removed
        );

        virtual QByteArray serialize() const override;
    };

    /**
     * Describes the version of analysis data of a certain Project.
     */
    class AnalysisVersionDto : public Serializable
    {
    public:

        /**
         * <p>The ID used to refer to an `AnalysisVersion` in this API.
         * 
         * <p>ISO8601 compatible date-strings that are guaranteed to
         * contain time zone information.
         * 
         * <p>An example version string is `2020-11-23T09:37:04.797286Z`.
         * 
         * <p>The precision of these dates is undefined, thus in order to be sure to refer
         * to exactly the same version when specifying a version as an argument e.g.
         * when querying issues, you must use this exact string.
         * 
         * <p>If you want to interpret the version-date e.g. for passing it to a
         * graph-drawing library or to order a list of `AnalysisVersion`s,
         * it is easier to use the field `millis` instead of parsing these dates.
         */
        QString date;

        /**
         * <p>Optional label of the version.
         * 
         * @since 7.6.0
         */
        std::optional<QString> label;

        /**
         * <p>The 0-based index of all the known analysis versions of a project.
         * 
         * <p>The version with index 0 never contains actual analysis data but always
         * refers to a fictional version without any issues that happened before
         * version 1.
         */
        qint32 index;

        /**
         * <p>The display name of the analysis version consisting of the date formatted with the
         * preferred time zone of the user making the request and the label in parentheses.
         */
        QString name;

        /**
         * <p>Analysis version timestamp in milliseconds
         * 
         * <p>The number of milliseconds passed since 1970-01-01T00:00:00 UTC
         * 
         * <p>Meant for programmatic interpretation of the actual instant in time that
         * represents a version.
         */
        qint64 millis;

        /**
         * <p>For every Issue Kind contains some Issue counts.
         * 
         * <p>Namely the Total count, as well as the newly Added and newly Removed issues in comparison
         * with the version before.
         * 
         * <p>N.B. The Bauhaus Version used to analyze the project must be at least 6.5.0 in order for
         * these values to be available.
         * 
         * @since 6.6.0
         */
        Any issueCounts;

        /**
         * Refers to a specific version of the Axivion Suite
         * 
         * <p>Version information of the Axivion Suite used to do this analysis run.
         * 
         * <p>Note, that this field is only available when the analysis was done with at least version
         * 6.5.0.
         * 
         * @since 6.9.15
         */
        std::optional<ToolsVersionDto> toolsVersion;

        /**
         * <p>The total lines of code of the project at the current version if available
         * 
         * @since 7.0.4
         */
        std::optional<qint64> linesOfCode;

        /**
         * <p>The clone ratio of the project at the current version if available
         * 
         * @since 7.0.4
         */
        std::optional<double> cloneRatio;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static AnalysisVersionDto deserialize(const QByteArray &json);

        AnalysisVersionDto(
            QString date,
            std::optional<QString> label,
            qint32 index,
            QString name,
            qint64 millis,
            Any issueCounts,
            std::optional<ToolsVersionDto> toolsVersion,
            std::optional<qint64> linesOfCode,
            std::optional<double> cloneRatio
        );

        virtual QByteArray serialize() const override;
    };

    /**
     * Contains Request-Data for creating an ApiToken
     * 
     * @since 7.1.0
     */
    class ApiTokenCreationRequestDto : public Serializable
    {
    public:

        /**
         * <p>Dashboard password of the user that requests the token creation
         */
        QString password;

        /**
         * Available types of ApiToken
         * 
         * <p>the type of the token to create
         * 
         * @since 7.1.0
         */
        QString type;

        /**
         * <p>Purpose of the Token
         */
        QString description;

        /**
         * <p>Used for configuring the Token expiration.
         * 
         * <p>* positive values are maxAge in milliseconds
         * * 0 means: choose a default for me (recommended)
         * * negative values are not allowed
         * 
         * <p>Note, that the server clock is decisive for when the actual token expiration will occur.
         * Expired tokens will be invalidated or deleted on the server depending on their type.
         */
        qint64 maxAgeMillis;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static ApiTokenCreationRequestDto deserialize(const QByteArray &json);

        ApiTokenCreationRequestDto(
            QString password,
            QString type,
            QString description,
            qint64 maxAgeMillis
        );

        ApiTokenCreationRequestDto(
            QString password,
            ApiTokenType type,
            QString description,
            qint64 maxAgeMillis
        );

        // Throws std::range_error
        ApiTokenType getTypeEnum() const;

        void setTypeEnum(ApiTokenType newValue);

        virtual QByteArray serialize() const override;
    };

    /**
     * Contains Meta-Data of an ApiToken
     * 
     * <p>When this is returned as response of a creation request it will also
     * contain the token secret
     * 
     * @since 7.1.0
     */
    class ApiTokenInfoDto : public Serializable
    {
    public:

        /**
         * <p>The unique Token-ID.
         */
        QString id;

        /**
         * <p>The token URL
         */
        QString url;

        /**
         * <p>Whether the token was still valid at query time.
         * 
         * <p>Invalid Tokens are effectively tombstones and cannot be used for
         * authentication any more.
         * Note, that this field is no indication on whether or not this object
         * is transporting the secret.
         */
        bool isValid;

        /**
         * Available types of ApiToken
         * 
         * <p>The type of the Token
         * 
         * @since 7.1.0
         */
        QString type;

        /**
         * <p>Description that was given on token creation.
         */
        QString description;

        /**
         * <p>The secret token value.
         * 
         * <p>This is only initialized upon token creation. Use this to authenticate
         * against the Dashboard as described in :ref:`authentication`.
         */
        std::optional<QString> token;

        /**
         * <p>ISO8601 format date string
         */
        QString creationDate;

        /**
         * <p>Alternative representation of the token creation date, like &quot;2 days ago&quot; etc
         */
        QString displayCreationDate;

        /**
         * <p>ISO8601 format date string
         */
        QString expirationDate;

        /**
         * <p>Alternative representation of the token expiration date, like &quot;3 months from now&quot; etc
         */
        QString displayExpirationDate;

        /**
         * <p>ISO8601 format date if the token has already been used
         */
        std::optional<QString> lastUseDate;

        /**
         * <p>Alternative representation of the token last use date, e.g. &quot;2 days ago&quot; or &quot;Never&quot;
         */
        QString displayLastUseDate;

        /**
         * <p>Whether this token is used by the current request.
         * 
         * <p>Deletion of this token will invalidate the currently used credentials
         */
        bool usedByCurrentRequest;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static ApiTokenInfoDto deserialize(const QByteArray &json);

        ApiTokenInfoDto(
            QString id,
            QString url,
            bool isValid,
            QString type,
            QString description,
            std::optional<QString> token,
            QString creationDate,
            QString displayCreationDate,
            QString expirationDate,
            QString displayExpirationDate,
            std::optional<QString> lastUseDate,
            QString displayLastUseDate,
            bool usedByCurrentRequest
        );

        ApiTokenInfoDto(
            QString id,
            QString url,
            bool isValid,
            ApiTokenType type,
            QString description,
            std::optional<QString> token,
            QString creationDate,
            QString displayCreationDate,
            QString expirationDate,
            QString displayExpirationDate,
            std::optional<QString> lastUseDate,
            QString displayLastUseDate,
            bool usedByCurrentRequest
        );

        // Throws std::range_error
        ApiTokenType getTypeEnum() const;

        void setTypeEnum(ApiTokenType newValue);

        virtual QByteArray serialize() const override;
    };

    /**
     * Represents a table column.
     */
    class ColumnInfoDto : public Serializable
    {
    public:

        /**
         * <p>Key used to identify the column in the rows and for sorting/filtering.
         * 
         * <p>For Issue-Table columns, see :ref:`column &lt;issue-table-columns&gt;`.
         */
        QString key;

        /**
         * <p>Nice header name for the column for UI display purposes.
         */
        std::optional<QString> header;

        /**
         * <p>Specifies whether this column can be sorted.
         */
        bool canSort;

        /**
         * <p>Specifies whether this column can be filtered.
         */
        bool canFilter;

        /**
         * How the column values should be aligned.
         * 
         * <p>How the column values should be aligned.
         */
        QString alignment;

        /**
         * <p>The column type.
         * 
         * <p>Possible values:
         * * `string` - a unicode string or ``null``
         * * `number` - either ``null`` or ``&quot;Infinity&quot;`` or ``&quot;-Infinity&quot;`` or ``&quot;NaN&quot;`` (string!) or a decimal number with base 10.
         * * `state` - The fields are strings. Possible values are defined via ``typeOptions``.
         * * `boolean` - The fields are boolean values, either ``false`` or ``true``. Has ``typeOptions``.
         * * `path` - **Since 6.4.1** similar to ``string`` or ``null``, however they are normalized (guaranteed single-slash separators) for easy parsing as path
         * * `tags` - **Since 6.5.0** an array of :json:object:`IssueTag`
         * * `comments` - **Since 6.9.0** array of :json:object:`IssueComment`
         * * `owners` - **Since 7.0.0** array of :json:object:`UserRef`
         */
        QString type;

        /**
         * <p>Describes possible values for the field.
         * 
         * <p>Currently this is only used for the types ``state`` and ``boolean``.
         * In case of type ``boolean`` this always contains exactly 2 elements,
         * the first describing the false-equivalent, the second describing the
         * true-equivalent. In case of type ``state`` this contains the possible string
         * values the field may have.
         */
        std::optional<std::vector<ColumnTypeOptionDto>> typeOptions;

        /**
         * <p>Suggested column width in pixels
         */
        qint32 width;

        /**
         * <p>whether a gui should show this column by default.
         */
        bool showByDefault;

        /**
         * <p>Key used to identify a column that provides the hyperlink targets
         * for this column (&#x27;ErrorLink&#x27; or null)
         */
        std::optional<QString> linkKey;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static ColumnInfoDto deserialize(const QByteArray &json);

        ColumnInfoDto(
            QString key,
            std::optional<QString> header,
            bool canSort,
            bool canFilter,
            QString alignment,
            QString type,
            std::optional<std::vector<ColumnTypeOptionDto>> typeOptions,
            qint32 width,
            bool showByDefault,
            std::optional<QString> linkKey
        );

        ColumnInfoDto(
            QString key,
            std::optional<QString> header,
            bool canSort,
            bool canFilter,
            TableCellAlignment alignment,
            QString type,
            std::optional<std::vector<ColumnTypeOptionDto>> typeOptions,
            qint32 width,
            bool showByDefault,
            std::optional<QString> linkKey
        );

        // Throws std::range_error
        TableCellAlignment getAlignmentEnum() const;

        void setAlignmentEnum(TableCellAlignment newValue);

        virtual QByteArray serialize() const override;
    };

    /**
     * Main API endpoint response
     */
    class DashboardInfoDto : public Serializable
    {
    public:

        /**
         * <p>The complete dashboard base URL.
         * 
         * <p>Use this to point your browser to the Dashboard or
         * combine it with the other host-relative URLs returned by this API
         * in order to get complete URLs.
         * 
         * <p>Note, that this URL may be different from the URL you are currently
         * accessing the Dashboard with for various reasons.
         * 
         * <p>Also note, that the Dashboard cannot always know the proper value
         * to return here, e.g. when running behind a proxy. So an administrator
         * might need to help out the Dashboard by configuring the proper value
         * in the global settings.
         * 
         * @since 7.4.0
         */
        std::optional<QString> mainUrl;

        /**
         * <p>Axivion Dashboard version serving this API.
         */
        QString dashboardVersion;

        /**
         * <p>Parseable Axivion Dashboard Version.
         * 
         * @since 6.6.0
         */
        std::optional<QString> dashboardVersionNumber;

        /**
         * <p>Dashboard Server Build date.
         */
        QString dashboardBuildDate;

        /**
         * <p>Name of the successfully authenticated user if a dashboard-user is associated with the request.
         */
        std::optional<QString> username;

        /**
         * <p>The HTTP-Request Header expected present for all HTTP requests that are not GET, HEAD, OPTIONS or TRACE.
         * 
         * <p>Deprecated since 7.7.0: the header name is always ``AX-CSRF-Token``.
         * 
         * @deprecated
         */
        std::optional<QString> csrfTokenHeader;

        /**
         * <p>The value expected to be sent along the ``csrfTokenHeader`` for all HTTP requests that are not GET, HEAD, OPTIONS or TRACE.
         * 
         * <p>Note, that this does not replace authentication of subsequent requests. Also the token is combined with the authentication
         * data, so will not work when authenticating as another user. Its lifetime is limited, so when creating a very long-running
         * application you should consider refreshing this token from time to time.
         */
        QString csrfToken;

        /**
         * <p>An URI that can be used to check credentials via GET. It returns `&quot;ok&quot;` in case of valid credentials.
         * 
         * @since 6.5.4
         */
        std::optional<QString> checkCredentialsUrl;

        /**
         * <p>Endpoint for managing global named filters
         * 
         * @since 7.3.0
         */
        std::optional<QString> namedFiltersUrl;

        /**
         * <p>List of references to the projects visible to the authenticated user.
         */
        std::optional<std::vector<ProjectReferenceDto>> projects;

        /**
         * <p>Endpoint for creating and listing api tokens of the current user
         * 
         * @since 7.1.0
         */
        std::optional<QString> userApiTokenUrl;

        /**
         * <p>Endpoint for managing custom named filters of the current user
         * 
         * @since 7.3.0
         */
        std::optional<QString> userNamedFiltersUrl;

        /**
         * <p>E-mail address for support requests
         * 
         * @since 7.4.3
         */
        std::optional<QString> supportAddress;

        /**
         * <p>A host-relative URL that can be used to display filter-help meant for humans.
         * 
         * @since 7.4.3
         */
        std::optional<QString> issueFilterHelp;

        /**
         * <p>Endoint for creating new CSRF tokens.
         * 
         * @since 7.7.0
         */
        std::optional<QString> csrfTokenUrl;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static DashboardInfoDto deserialize(const QByteArray &json);

        DashboardInfoDto(
            std::optional<QString> mainUrl,
            QString dashboardVersion,
            std::optional<QString> dashboardVersionNumber,
            QString dashboardBuildDate,
            std::optional<QString> username,
            std::optional<QString> csrfTokenHeader,
            QString csrfToken,
            std::optional<QString> checkCredentialsUrl,
            std::optional<QString> namedFiltersUrl,
            std::optional<std::vector<ProjectReferenceDto>> projects,
            std::optional<QString> userApiTokenUrl,
            std::optional<QString> userNamedFiltersUrl,
            std::optional<QString> supportAddress,
            std::optional<QString> issueFilterHelp,
            std::optional<QString> csrfTokenUrl
        );

        virtual QByteArray serialize() const override;
    };

    /**
     * Response when listing comments of an issue
     */
    class IssueCommentListDto : public Serializable
    {
    public:

        /**
         * <p>Comments in chronological order
         */
        std::vector<IssueCommentDto> comments;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static IssueCommentListDto deserialize(const QByteArray &json);

        IssueCommentListDto(
            std::vector<IssueCommentDto> comments
        );

        virtual QByteArray serialize() const override;
    };

    /**
     * Describes an Issue Kind.
     */
    class IssueKindInfoDto : public Serializable
    {
    public:

        /**
         * Shortname for an erosion issue kind
         * 
         * <p>one of ``AV``, ``CL``, ``CY``, ``DE``, ``MV``, ``SV``
         */
        QString prefix;

        /**
         * <p>A singular string for using in UI texts about the issue kind.
         */
        QString niceSingularName;

        /**
         * <p>A plural string for using in UI texts about the issue kind.
         */
        QString nicePluralName;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static IssueKindInfoDto deserialize(const QByteArray &json);

        IssueKindInfoDto(
            QString prefix,
            QString niceSingularName,
            QString nicePluralName
        );

        IssueKindInfoDto(
            IssueKind prefix,
            QString niceSingularName,
            QString nicePluralName
        );

        // Throws std::range_error
        IssueKind getPrefixEnum() const;

        void setPrefixEnum(IssueKind newValue);

        virtual QByteArray serialize() const override;
    };

    /**
     * List of Issue Tag Types
     */
    class IssueTagTypeListDto : public Serializable
    {
    public:

        /**
         * <p>Result when querying tags of a given issue
         */
        std::vector<IssueTagTypeDto> tags;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static IssueTagTypeListDto deserialize(const QByteArray &json);

        IssueTagTypeListDto(
            std::vector<IssueTagTypeDto> tags
        );

        virtual QByteArray serialize() const override;
    };

    /**
     * <p>Describes a tainted location of a source file
     */
    class LineMarkerDto : public Serializable
    {
    public:

        /**
         * The issue kind
         * 
         * <p>one of ``AV``, ``CL``, ``CY``, ``DE``, ``MV``, ``SV``
         */
        QString kind;

        /**
         * The issue id.
         * 
         * <p>This id is not global. It is only unique for the same project and kind.
         * 
         * <p>Note, that this may not be available e.g. when served by the PluginARServer.
         */
        std::optional<qint64> id;

        /**
         * The first tainted line.
         * 
         * <p>First line in file is line 1.
         */
        qint32 startLine;

        /**
         * The column of the start line in which the issue starts.
         * 
         * <p>First column in line is column 1.
         * 
         * <p>0 iff unknown
         * 
         * @since 7.2.0
         */
        qint32 startColumn;

        /**
         * The last tainted line.
         * 
         * <p>First line in file is line 1.
         */
        qint32 endLine;

        /**
         * The column of the end line in which the issue ends.
         * 
         * <p>First column in line is column 1.
         * 
         * <p>0 iff unknown
         * 
         * @since 7.2.0
         */
        qint32 endColumn;

        /**
         * <p>A prosaic (one-liner) description of the issue.
         */
        QString description;

        /**
         * <p>Host-relative API URI to access further information about the issue.
         * 
         * <p>Note, that this may not be available e.g. when served by the PluginARServer.
         */
        std::optional<QString> issueUrl;

        /**
         * <p>Determines, whether the issue is new in the given version, i.e. it did not
         * exist in the version before the given version and the given version is not
         * the first analyzed version.
         * 
         * @since 6.5.0
         */
        std::optional<bool> isNew;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static LineMarkerDto deserialize(const QByteArray &json);

        LineMarkerDto(
            QString kind,
            std::optional<qint64> id,
            qint32 startLine,
            qint32 startColumn,
            qint32 endLine,
            qint32 endColumn,
            QString description,
            std::optional<QString> issueUrl,
            std::optional<bool> isNew
        );

        LineMarkerDto(
            IssueKind kind,
            std::optional<qint64> id,
            qint32 startLine,
            qint32 startColumn,
            qint32 endLine,
            qint32 endColumn,
            QString description,
            std::optional<QString> issueUrl,
            std::optional<bool> isNew
        );

        // Throws std::range_error
        IssueKind getKindEnum() const;

        void setKindEnum(IssueKind newValue);

        virtual QByteArray serialize() const override;
    };

    /**
     * A log message with an associated log level
     * 
     * <p>Contains messages returned from the VCS-adapters reporting on
     * the VCS update result.
     */
    class RepositoryUpdateMessageDto : public Serializable
    {
    public:

        /**
         * The log-level of a message
         * 
         * <p>The log-level of the message.
         */
        std::optional<QString> severity;

        /**
         * <p>the log-message. It may contain new-lines.
         */
        std::optional<QString> message;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static RepositoryUpdateMessageDto deserialize(const QByteArray &json);

        RepositoryUpdateMessageDto(
            std::optional<QString> severity,
            std::optional<QString> message
        );

        RepositoryUpdateMessageDto(
            std::optional<MessageSeverity> severity,
            std::optional<QString> message
        );

        // Throws std::range_error
        std::optional<MessageSeverity> getSeverityEnum() const;

        void setSeverityEnum(std::optional<MessageSeverity> newValue);

        virtual QByteArray serialize() const override;
    };

    /**
     * Contains a list of rules
     * 
     * @since 6.5.0
     */
    class RuleListDto : public Serializable
    {
    public:

        
        std::vector<RuleDto> rules;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static RuleListDto deserialize(const QByteArray &json);

        RuleListDto(
            std::vector<RuleDto> rules
        );

        virtual QByteArray serialize() const override;
    };

    /**
     * A Column Key + Sort Direction to indicate table sort preferences.
     */
    class SortInfoDto : public Serializable
    {
    public:

        /**
         * <p>The :ref:`column key&lt;issue-table-columns&gt;`
         */
        QString key;

        /**
         * A sorting direction
         * 
         * <p>The sort direction associated with this column.
         */
        QString direction;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static SortInfoDto deserialize(const QByteArray &json);

        SortInfoDto(
            QString key,
            QString direction
        );

        SortInfoDto(
            QString key,
            SortDirection direction
        );

        // Throws std::range_error
        SortDirection getDirectionEnum() const;

        void setDirectionEnum(SortDirection newValue);

        virtual QByteArray serialize() const override;
    };

    /**
     * Information about a user
     */
    class UserRefDto : public Serializable
    {
    public:

        /**
         * <p>User name. Use this to refer to the same user.
         */
        QString name;

        /**
         * <p>Use this for display of the user in a UI.
         */
        QString displayName;

        /**
         * User Type
         * 
         * <p>User Type
         * 
         * @since 7.1.0
         */
        std::optional<QString> type;

        /**
         * <p>Whether this user is a so-called `public` user.
         * 
         * @since 7.1.0
         */
        std::optional<bool> isPublic;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static UserRefDto deserialize(const QByteArray &json);

        UserRefDto(
            QString name,
            QString displayName,
            std::optional<QString> type,
            std::optional<bool> isPublic
        );

        UserRefDto(
            QString name,
            QString displayName,
            std::optional<UserRefType> type,
            std::optional<bool> isPublic
        );

        // Throws std::range_error
        std::optional<UserRefType> getTypeEnum() const;

        void setTypeEnum(std::optional<UserRefType> newValue);

        virtual QByteArray serialize() const override;
    };

    /**
     * Contains a list of analyzed file descriptions.
     */
    class AnalyzedFileListDto : public Serializable
    {
    public:

        /**
         * Describes the version of analysis data of a certain Project.
         * 
         * <p>The version this list was queried with.
         */
        AnalysisVersionDto version;

        
        std::vector<AnalyzedFileDto> rows;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static AnalyzedFileListDto deserialize(const QByteArray &json);

        AnalyzedFileListDto(
            AnalysisVersionDto version,
            std::vector<AnalyzedFileDto> rows
        );

        virtual QByteArray serialize() const override;
    };

    /**
     * Contains a list of entities and the version of their versioned aspects
     */
    class EntityListDto : public Serializable
    {
    public:

        /**
         * Describes the version of analysis data of a certain Project.
         * 
         * <p>The version this entity list was queried with.
         */
        std::optional<AnalysisVersionDto> version;

        
        std::vector<EntityDto> entities;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static EntityListDto deserialize(const QByteArray &json);

        EntityListDto(
            std::optional<AnalysisVersionDto> version,
            std::vector<EntityDto> entities
        );

        virtual QByteArray serialize() const override;
    };

    /**
     * <p>Describes information that can be used to display a file with erosion information.
     * 
     * @since 6.2.0
     */
    class FileViewDto : public Serializable
    {
    public:

        /**
         * <p>The complete path of the file relative to the projects file-root
         */
        QString fileName;

        /**
         * <p>ISO-8601 Datestring of the file&#x27;s version that can be used when manually
         * constructing an URL to retrieve the file&#x27;s source-code.
         * 
         * <p>The format used is yyyy-MM-ddTHH:mm:ss.SSSZZ
         * 
         * @since 6.4.2
         */
        std::optional<QString> version;

        /**
         * <p>Refers to the source code which can be gotten as text/plain or as an
         * application/json object containing a token-sequence
         * 
         * <p>Note, that this may not be available e.g. when served by the PluginARServer.
         */
        std::optional<QString> sourceCodeUrl;

        /**
         * <p>The erosion information associated with the file
         */
        std::vector<LineMarkerDto> lineMarkers;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static FileViewDto deserialize(const QByteArray &json);

        FileViewDto(
            QString fileName,
            std::optional<QString> version,
            std::optional<QString> sourceCodeUrl,
            std::vector<LineMarkerDto> lineMarkers
        );

        virtual QByteArray serialize() const override;
    };

    /**
     * <p>Describes an issue
     */
    class IssueDto : public Serializable
    {
    public:

        /**
         * The issue kind
         * 
         * <p>one of ``AV``, ``CL``, ``CY``, ``DE``, ``MV``, ``SV``
         */
        QString kind;

        /**
         * The issue id
         * 
         * <p>This id is not global. It is only unique for the same project and kind.
         */
        qint64 id;

        /**
         * The Project the issue belongs to
         */
        ProjectReferenceDto parentProject;

        /**
         * Source locations associated with the issue
         */
        std::vector<IssueSourceLocationDto> sourceLocations;

        /**
         * The issue kind
         */
        IssueKindInfoDto issueKind;

        /**
         * Whether or not the issue is hidden
         * 
         * @since 6.4.0
         */
        bool isHidden;

        /**
         * <p>Versioned host relative URL to view issue in browser
         * 
         * @since 7.2.1
         */
        std::optional<QString> issueViewUrl;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static IssueDto deserialize(const QByteArray &json);

        IssueDto(
            QString kind,
            qint64 id,
            ProjectReferenceDto parentProject,
            std::vector<IssueSourceLocationDto> sourceLocations,
            IssueKindInfoDto issueKind,
            bool isHidden,
            std::optional<QString> issueViewUrl
        );

        IssueDto(
            IssueKind kind,
            qint64 id,
            ProjectReferenceDto parentProject,
            std::vector<IssueSourceLocationDto> sourceLocations,
            IssueKindInfoDto issueKind,
            bool isHidden,
            std::optional<QString> issueViewUrl
        );

        // Throws std::range_error
        IssueKind getKindEnum() const;

        void setKindEnum(IssueKind newValue);

        virtual QByteArray serialize() const override;
    };

    /**
     * Result of querying the issue-list retrieval entry point mainly
     * containing a list of issues.
     */
    class IssueTableDto : public Serializable
    {
    public:

        /**
         * Describes the version of analysis data of a certain Project.
         * 
         * <p>The version of the ``removed`` issues.
         * 
         * <p>If the query was not an actual diff query this will be unset.
         */
        std::optional<AnalysisVersionDto> startVersion;

        /**
         * Describes the version of analysis data of a certain Project.
         * 
         * <p>The version of the ``added`` issues for a diff query
         * or simply the version of a normal issue list query (no startVersion)
         */
        AnalysisVersionDto endVersion;

        /**
         * <p>Url to view the issues in the Dashboard Browser UI
         * 
         * @since 7.2.1
         */
        std::optional<QString> tableViewUrl;

        /**
         * <p>The Issue Table Columns describing the issue fields.
         * 
         * <p>Deprecated since 7.1.0. Use :json:object`TableInfo` instead
         * 
         * @deprecated
         */
        std::optional<std::vector<ColumnInfoDto>> columns;

        /**
         * <p>The actual issue data objects.
         * 
         * <p>The issue object contents are dynamic and depend on the queried
         * issue kind. See :ref:`here&lt;issue-table-columns&gt;` for the individual field
         * descriptions depending on the issue kind. The values need to be interpreted
         * according to their columntype.
         * 
         * <p>This only contains a subset of the complete data if paging is enabled via
         * ``offset`` and ``limit``.
         */
        std::vector<std::map<QString, Any>> rows;

        /**
         * <p>The total number of issues.
         * 
         * <p>Only available when ``computeTotalRowCount`` was specified as ``true``.
         * Mostly useful when doing paged queries using the query parameters ``limit``
         * and ``offset``.
         */
        std::optional<qint32> totalRowCount;

        /**
         * <p>The total number of issues existing in the ``current`` version and not in
         * the ``baseline`` version.
         * 
         * <p>Only useful in diff queries and only calculated when
         * ``computeTotalRowCount`` was specified as ``true``.
         */
        std::optional<qint32> totalAddedCount;

        /**
         * <p>The total number of issues existing in the ``baseline`` version and not in
         * the ``current`` version.
         * 
         * <p>Only useful in diff queries and only calculated when
         * ``computeTotalRowCount`` was specified as ``true``.
         */
        std::optional<qint32> totalRemovedCount;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static IssueTableDto deserialize(const QByteArray &json);

        IssueTableDto(
            std::optional<AnalysisVersionDto> startVersion,
            AnalysisVersionDto endVersion,
            std::optional<QString> tableViewUrl,
            std::optional<std::vector<ColumnInfoDto>> columns,
            std::vector<std::map<QString, Any>> rows,
            std::optional<qint32> totalRowCount,
            std::optional<qint32> totalAddedCount,
            std::optional<qint32> totalRemovedCount
        );

        virtual QByteArray serialize() const override;
    };

    /**
     * Contains a list of metric descriptions
     */
    class MetricListDto : public Serializable
    {
    public:

        /**
         * Describes the version of analysis data of a certain Project.
         * 
         * <p>The version this metric list was queried with.
         */
        std::optional<AnalysisVersionDto> version;

        
        std::vector<MetricDto> metrics;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static MetricListDto deserialize(const QByteArray &json);

        MetricListDto(
            std::optional<AnalysisVersionDto> version,
            std::vector<MetricDto> metrics
        );

        virtual QByteArray serialize() const override;
    };

    /**
     * The result of a metric values query
     */
    class MetricValueRangeDto : public Serializable
    {
    public:

        /**
         * Describes the version of analysis data of a certain Project.
         * 
         * <p>The start version of the metric value range.
         */
        AnalysisVersionDto startVersion;

        /**
         * Describes the version of analysis data of a certain Project.
         * 
         * <p>The end version of the metric value range.
         */
        AnalysisVersionDto endVersion;

        /**
         * <p>The id of the entity
         */
        QString entity;

        /**
         * <p>The id of the metric
         */
        QString metric;

        /**
         * <p>An array with the metric values.
         * 
         * <p>The array size is ``endVersion.index - startVersion.index + 1``.
         * Its values are numbers or ``null`` if no value is available.
         * They correspond to the range defined by ``startVersion`` and ``endVersion``.
         */
        std::vector<std::optional<double>> values;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static MetricValueRangeDto deserialize(const QByteArray &json);

        MetricValueRangeDto(
            AnalysisVersionDto startVersion,
            AnalysisVersionDto endVersion,
            QString entity,
            QString metric,
            std::vector<std::optional<double>> values
        );

        virtual QByteArray serialize() const override;
    };

    /**
     * The result of a metric value table query
     */
    class MetricValueTableDto : public Serializable
    {
    public:

        /**
         * <p>The column descriptions of the entity columns.
         * 
         * <p>Only contains the two fields ``key`` and ``header``.
         */
        std::vector<ColumnInfoDto> columns;

        /**
         * <p>The entity data.
         */
        std::vector<MetricValueTableRowDto> rows;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static MetricValueTableDto deserialize(const QByteArray &json);

        MetricValueTableDto(
            std::vector<ColumnInfoDto> columns,
            std::vector<MetricValueTableRowDto> rows
        );

        virtual QByteArray serialize() const override;
    };

    /**
     * Named Filter Creation Request Data
     * 
     * <p>Contains information to create a named filter from scratch
     * 
     * @since 7.3.0
     */
    class NamedFilterCreateDto : public Serializable
    {
    public:

        
        QString displayName;

        /**
         * IssueKind for Named-Filter creation
         * 
         * <p>one of ``AV``, ``CL``, ``CY``, ``DE``, ``MV``, ``SV``, ``UNIVERSAL``
         */
        QString kind;

        /**
         * <p>The actual filters with column-id as key and filter-value as value
         * 
         * <p>* Possible keys are described here :ref:`column keys&lt;issue-table-columns&gt;`
         * * This does not necessarily contain an entry for every column. In fact it may even be empty.
         * * A filter value can never be null.
         * * May contain references to columns that do not exist in the table column configuration.
         */
        std::map<QString, QString> filters;

        /**
         * <p>Defines the sort order to apply.
         * 
         * <p>* The first entry has the highest sort priority and the last one the lowest.
         * * No column key may be referenced twice.
         * * May contain references to columns that do not exist in the table column configuration.
         */
        std::vector<SortInfoDto> sorters;

        /**
         * NamedFilter visibility configuration
         * 
         * <p>Only applicable for global named filters.
         * 
         * <p>You may not have access to this information depending on your permissions.
         * 
         * @since 7.3.0
         */
        std::optional<NamedFilterVisibilityDto> visibility;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static NamedFilterCreateDto deserialize(const QByteArray &json);

        NamedFilterCreateDto(
            QString displayName,
            QString kind,
            std::map<QString, QString> filters,
            std::vector<SortInfoDto> sorters,
            std::optional<NamedFilterVisibilityDto> visibility
        );

        NamedFilterCreateDto(
            QString displayName,
            IssueKindForNamedFilterCreation kind,
            std::map<QString, QString> filters,
            std::vector<SortInfoDto> sorters,
            std::optional<NamedFilterVisibilityDto> visibility
        );

        // Throws std::range_error
        IssueKindForNamedFilterCreation getKindEnum() const;

        void setKindEnum(IssueKindForNamedFilterCreation newValue);

        virtual QByteArray serialize() const override;
    };

    /**
     * A Named Filter configuration.
     * 
     * <p>It can consist of the following:
     *   - Filter values for the individual columns and the path-filter
     *   - A sort configuration indicating how to sort the table columns
     * 
     * @since 6.9.0
     */
    class NamedFilterInfoDto : public Serializable
    {
    public:

        /**
         * <p>Uniquely identifies this filter configuration.
         */
        QString key;

        /**
         * <p>The name to use when displaying this filter.
         */
        QString displayName;

        /**
         * <p>The URL for getting/updating/deleting this named filter.
         * 
         * @since 7.3.0
         */
        std::optional<QString> url;

        /**
         * <p>Whether this filter is a predefined filter.
         */
        bool isPredefined;

        /**
         * A named filter type
         * 
         * <p>* `PREDEFINED` - Named filters of this type are immutable and exist out of the box and can be used by everyone
         * * `GLOBAL` - Named filters of this type are usable by everyone and managed by the so called filter managers
         * * `CUSTOM` - Named filters of this type are creatable by everyone but only visible to their owner
         * 
         * @since 7.3.0
         */
        std::optional<QString> type;

        /**
         * <p>Whether the user that requested this object can change or delete this filter configuration
         */
        bool canWrite;

        /**
         * <p>The actual filters with column-id as key and filter-value as value
         * 
         * <p>* Possible keys are described here :ref:`column keys&lt;issue-table-columns&gt;`
         * * This does not necessarily contain an entry for every column. In fact it may even be empty.
         * * A filter value can never be null.
         * * May contain references to columns that do not exist in the table column configuration.
         */
        std::map<QString, QString> filters;

        /**
         * <p>Defines the sort order to apply.
         * 
         * <p>* The first entry has the highest sort priority and the last one the lowest.
         * * No column key may be referenced twice.
         * * May contain references to columns that do not exist in the table column configuration.
         */
        std::optional<std::vector<SortInfoDto>> sorters;

        /**
         * <p>True if this filter is valid for all issue kinds, false otherwise.
         */
        bool supportsAllIssueKinds;

        /**
         * <p>Supported Issue Kinds.
         * 
         * <p>If ``supportsAllIssueKinds`` is false, then this set indicates, which issue kinds are supported.
         */
        std::optional<std::unordered_set<QString>> issueKindRestrictions;

        /**
         * NamedFilter visibility configuration
         * 
         * <p>Only applicable for global named filters.
         * 
         * <p>You may not have access to this information depending on your permissions.
         * 
         * @since 7.3.0
         */
        std::optional<NamedFilterVisibilityDto> visibility;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static NamedFilterInfoDto deserialize(const QByteArray &json);

        NamedFilterInfoDto(
            QString key,
            QString displayName,
            std::optional<QString> url,
            bool isPredefined,
            std::optional<QString> type,
            bool canWrite,
            std::map<QString, QString> filters,
            std::optional<std::vector<SortInfoDto>> sorters,
            bool supportsAllIssueKinds,
            std::optional<std::unordered_set<QString>> issueKindRestrictions,
            std::optional<NamedFilterVisibilityDto> visibility
        );

        NamedFilterInfoDto(
            QString key,
            QString displayName,
            std::optional<QString> url,
            bool isPredefined,
            std::optional<NamedFilterType> type,
            bool canWrite,
            std::map<QString, QString> filters,
            std::optional<std::vector<SortInfoDto>> sorters,
            bool supportsAllIssueKinds,
            std::optional<std::unordered_set<QString>> issueKindRestrictions,
            std::optional<NamedFilterVisibilityDto> visibility
        );

        // Throws std::range_error
        std::optional<NamedFilterType> getTypeEnum() const;

        void setTypeEnum(std::optional<NamedFilterType> newValue);

        virtual QByteArray serialize() const override;
    };

    /**
     * Named Filter Update Request Data
     * 
     * <p>Contains information to update an existing named filter.
     * Fields that are not given, won&#x27;t be touched.
     * 
     * @since 7.3.0
     */
    class NamedFilterUpdateDto : public Serializable
    {
    public:

        /**
         * <p>An optional new name for the named filter.
         * 
         * <p>Changing this will also result in a changed ID and URL.
         */
        std::optional<QString> name;

        /**
         * <p>The actual filters with column-id as key and filter-value as value
         * 
         * <p>* Possible keys are described here :ref:`column keys&lt;issue-table-columns&gt;`
         * * This does not necessarily contain an entry for every column. In fact it may even be empty.
         * * A filter value can never be null.
         * * May contain references to columns that do not exist in the table column configuration.
         */
        std::optional<std::map<QString, QString>> filters;

        /**
         * <p>Defines the sort order to apply.
         * 
         * <p>* The first entry has the highest sort priority and the last one the lowest.
         * * No column key may be referenced twice.
         * * May contain references to columns that do not exist in the table column configuration.
         */
        std::optional<std::vector<SortInfoDto>> sorters;

        /**
         * NamedFilter visibility configuration
         * 
         * <p>Only applicable for global named filters.
         * 
         * <p>You may not have access to this information depending on your permissions.
         * 
         * @since 7.3.0
         */
        std::optional<NamedFilterVisibilityDto> visibility;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static NamedFilterUpdateDto deserialize(const QByteArray &json);

        NamedFilterUpdateDto(
            std::optional<QString> name,
            std::optional<std::map<QString, QString>> filters,
            std::optional<std::vector<SortInfoDto>> sorters,
            std::optional<NamedFilterVisibilityDto> visibility
        );

        virtual QByteArray serialize() const override;
    };

    /**
     * Information about a project
     */
    class ProjectInfoDto : public Serializable
    {
    public:

        /**
         * <p>The name of the project. Use this string to refer to the project.
         */
        QString name;

        /**
         * <p>A host-relative URL that can be used to display filter-help meant for humans.
         * 
         * @since 6.5.0
         */
        std::optional<QString> issueFilterHelp;

        /**
         * <p>URL to query the table meta data (column definitions). Needs the issue kind as a query parameter ``kind``.
         * 
         * @since 6.9.0
         */
        std::optional<QString> tableMetaUri;

        /**
         * <p>List of users associated with the project and visible to the authenticated user.
         */
        std::vector<UserRefDto> users;

        /**
         * <p>List of analysis versions associated with the project.
         */
        std::vector<AnalysisVersionDto> versions;

        /**
         * <p>List of IssueKinds associated with the project.
         */
        std::vector<IssueKindInfoDto> issueKinds;

        /**
         * <p>Whether or not the project has hidden issues. When this is false then UI should try to hide all the complexity related with hidden issues.
         */
        bool hasHiddenIssues;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static ProjectInfoDto deserialize(const QByteArray &json);

        ProjectInfoDto(
            QString name,
            std::optional<QString> issueFilterHelp,
            std::optional<QString> tableMetaUri,
            std::vector<UserRefDto> users,
            std::vector<AnalysisVersionDto> versions,
            std::vector<IssueKindInfoDto> issueKinds,
            bool hasHiddenIssues
        );

        virtual QByteArray serialize() const override;
    };

    /**
     * Response to Repository Update Request
     * 
     * <p>Contains messages returned from the VCS-adapters reporting on
     * the VCS update result.
     */
    class RepositoryUpdateResponseDto : public Serializable
    {
    public:

        /**
         * <p>The messages returned from the VCS-adapters
         */
        std::vector<RepositoryUpdateMessageDto> messages;

        /**
         * <p>Whether at least one of the messages is at least an ERROR
         */
        bool hasErrors;

        /**
         * <p>Whether at least one of the messages is at least a WARNING
         */
        bool hasWarnings;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static RepositoryUpdateResponseDto deserialize(const QByteArray &json);

        RepositoryUpdateResponseDto(
            std::vector<RepositoryUpdateMessageDto> messages,
            bool hasErrors,
            bool hasWarnings
        );

        virtual QByteArray serialize() const override;
    };

    /**
     * Contains meta information for issuelist querying of a specific issue kind.
     * 
     * @since 6.9.0
     */
    class TableInfoDto : public Serializable
    {
    public:

        /**
         * <p>Host-relative URL to query the rows of the issue table.
         * 
         * <p>Needs get parameters to specify which data is requested exactly.
         */
        QString tableDataUri;

        /**
         * <p>Host-relative URL base to view an issue in the dashboard.
         * 
         * <p>In order to construct the issue URL it is necessary to append the issue-id (e.g. SV123)
         * to the URL path. Also the query-parameter is not part of the URL base and can be added.
         * 
         * @since 7.2.1
         */
        std::optional<QString> issueBaseViewUri;

        /**
         * <p>Columns with detailed information on how to create a table to display the data.
         */
        std::vector<ColumnInfoDto> columns;

        /**
         * <p>The ``Named Filters`` available for the chosen issue kind.
         * 
         * <p>This will contain predefined and custom filter configurations and never be empty.
         * 
         * <p>The list order is a recommendation for UI display.
         */
        std::vector<NamedFilterInfoDto> filters;

        /**
         * <p>The key of the configured default filter.
         * 
         * <p>This will not be given if the configured default is not also visible
         * to the requesting user.
         */
        std::optional<QString> userDefaultFilter;

        /**
         * <p>The key of the factory default filter.
         * 
         * <p>This will always point to an existing and visible named filter.
         */
        QString axivionDefaultFilter;

        // Throws Axivion::Internal::Dto::invalid_dto_exception
        static TableInfoDto deserialize(const QByteArray &json);

        TableInfoDto(
            QString tableDataUri,
            std::optional<QString> issueBaseViewUri,
            std::vector<ColumnInfoDto> columns,
            std::vector<NamedFilterInfoDto> filters,
            std::optional<QString> userDefaultFilter,
            QString axivionDefaultFilter
        );

        virtual QByteArray serialize() const override;
    };

}
