/* eslint-disable */
// Auto-generated from the JSON Schema files. Do not edit by hand.
// Run `npm run gen:types` to regenerate.

/**
 * Warning: Arguments construct command-line parameters that may contain user-provided input. This creates potential command injection risks if clients execute commands in a shell environment. For example, a malicious argument value like ';rm -rf ~/Development' could execute dangerous commands. Clients should prefer non-shell execution methods (e.g., posix_spawn) when possible to eliminate injection risks entirely. Where not possible, clients should obtain consent from users or agents to run the resolved command before execution.
 *
 * This interface was referenced by `ServerJsonDefiningAModelContextProtocolMCPServer`'s JSON-Schema
 * via the `definition` "Argument".
 */
export type Argument = PositionalArgument | NamedArgument;
/**
 * A positional input is a value inserted verbatim into the command line.
 *
 * This interface was referenced by `ServerJsonDefiningAModelContextProtocolMCPServer`'s JSON-Schema
 * via the `definition` "PositionalArgument".
 */
export type PositionalArgument = InputWithVariables & {
  /**
   * Whether the argument can be repeated multiple times in the command line.
   */
  isRepeated?: boolean;
  type: "positional";
  /**
   * An identifier for the positional argument. It is not part of the command line. It may be used by client configuration as a label identifying the argument. It is also used to identify the value in transport URL variable substitution.
   */
  valueHint?: string;
};
/**
 * This interface was referenced by `ServerJsonDefiningAModelContextProtocolMCPServer`'s JSON-Schema
 * via the `definition` "InputWithVariables".
 */
export type InputWithVariables = Input & {
  /**
   * A map of variable names to their values. Keys in the input `value` that are wrapped in `{curly_braces}` will be replaced with the corresponding variable values.
   */
  variables?: {
    [k: string]: Input;
  };
};
/**
 * A command-line `--flag={value}`.
 *
 * This interface was referenced by `ServerJsonDefiningAModelContextProtocolMCPServer`'s JSON-Schema
 * via the `definition` "NamedArgument".
 */
export type NamedArgument = InputWithVariables & {
  /**
   * Whether the argument can be repeated multiple times.
   */
  isRepeated?: boolean;
  /**
   * The flag name, including any leading dashes.
   */
  name: string;
  type: "named";
};
/**
 * This interface was referenced by `ServerJsonDefiningAModelContextProtocolMCPServer`'s JSON-Schema
 * via the `definition` "KeyValueInput".
 */
export type KeyValueInput = InputWithVariables & {
  /**
   * Name of the header or environment variable.
   */
  name: string;
};
/**
 * Transport protocol configuration for local/package context
 *
 * This interface was referenced by `ServerJsonDefiningAModelContextProtocolMCPServer`'s JSON-Schema
 * via the `definition` "LocalTransport".
 */
export type LocalTransport = StdioTransport | StreamableHttpTransport | SseTransport;
/**
 * Transport protocol configuration for remote context - extends StreamableHttpTransport or SseTransport with variables
 *
 * This interface was referenced by `ServerJsonDefiningAModelContextProtocolMCPServer`'s JSON-Schema
 * via the `definition` "RemoteTransport".
 */
export type RemoteTransport = (StreamableHttpTransport | SseTransport) & {
  /**
   * Configuration variables that can be referenced in URL template {curly_braces}. The key is the variable name, and the value defines the variable properties.
   */
  variables?: {
    [k: string]: Input;
  };
};

export interface ServerJsonDefiningAModelContextProtocolMCPServer {
  [k: string]: unknown;
}
/**
 * This interface was referenced by `ServerJsonDefiningAModelContextProtocolMCPServer`'s JSON-Schema
 * via the `definition` "Input".
 */
export interface Input {
  /**
   * A list of possible values for the input. If provided, the user must select one of these values.
   */
  choices?: string[];
  /**
   * The default value for the input.  This should be a valid value for the input.  If you want to provide input examples or guidance, use the `placeholder` field instead.
   */
  default?: string;
  /**
   * A description of the input, which clients can use to provide context to the user.
   */
  description?: string;
  /**
   * Specifies the input format. Supported values include `filepath`, which should be interpreted as a file on the user's filesystem.
   *
   * When the input is converted to a string, booleans should be represented by the strings "true" and "false", and numbers should be represented as decimal values.
   */
  format?: "string" | "number" | "boolean" | "filepath";
  isRequired?: boolean;
  /**
   * Indicates whether the input is a secret value (e.g., password, token). If true, clients should handle the value securely.
   */
  isSecret?: boolean;
  /**
   * A placeholder for the input to be displaying during configuration. This is used to provide examples or guidance about the expected form or content of the input.
   */
  placeholder?: string;
  /**
   * The value for the input. If this is not set, the user may be prompted to provide a value. If a value is set, it should not be configurable by end users.
   *
   * Identifiers wrapped in `{curly_braces}` will be replaced with the corresponding properties from the input `variables` map. If an identifier in braces is not found in `variables`, or if `variables` is not provided, the `{curly_braces}` substring should remain unchanged.
   *
   */
  value?: string;
}
/**
 * An optionally-sized icon that can be displayed in a user interface.
 *
 * This interface was referenced by `ServerJsonDefiningAModelContextProtocolMCPServer`'s JSON-Schema
 * via the `definition` "Icon".
 */
export interface Icon {
  /**
   * Optional MIME type override if the source MIME type is missing or generic. Must be one of: image/png, image/jpeg, image/jpg, image/svg+xml, image/webp.
   */
  mimeType?: "image/png" | "image/jpeg" | "image/jpg" | "image/svg+xml" | "image/webp";
  /**
   * Optional array of strings that specify sizes at which the icon can be used. Each string should be in WxH format (e.g., '48x48', '96x96') or 'any' for scalable formats like SVG. If not provided, the client should assume that the icon can be used at any size.
   */
  sizes?: string[];
  /**
   * A standard URI pointing to an icon resource. Must be an HTTPS URL. Consumers SHOULD take steps to ensure URLs serving icons are from the same domain as the server or a trusted domain. Consumers SHOULD take appropriate precautions when consuming SVGs as they can contain executable JavaScript.
   */
  src: string;
  /**
   * Optional specifier for the theme this icon is designed for. 'light' indicates the icon is designed to be used with a light background, and 'dark' indicates the icon is designed to be used with a dark background. If not provided, the client should assume the icon can be used with any theme.
   */
  theme?: "light" | "dark";
}
/**
 * This interface was referenced by `ServerJsonDefiningAModelContextProtocolMCPServer`'s JSON-Schema
 * via the `definition` "StdioTransport".
 */
export interface StdioTransport {
  /**
   * Transport type
   */
  type: "stdio";
}
/**
 * This interface was referenced by `ServerJsonDefiningAModelContextProtocolMCPServer`'s JSON-Schema
 * via the `definition` "StreamableHttpTransport".
 */
export interface StreamableHttpTransport {
  /**
   * HTTP headers to include
   */
  headers?: KeyValueInput[];
  /**
   * Transport type
   */
  type: "streamable-http";
  /**
   * URL template for the streamable-http transport. Must start with http://, https://, or a template variable (e.g., {baseUrl}). Variables in {curly_braces} are resolved based on context: In Package context, they reference argument valueHints, argument names, or environment variable names from the parent Package. In Remote context, they reference variables from the transport's 'variables' object. After variable substitution, this should produce a valid URI.
   */
  url: string;
}
/**
 * This interface was referenced by `ServerJsonDefiningAModelContextProtocolMCPServer`'s JSON-Schema
 * via the `definition` "SseTransport".
 */
export interface SseTransport {
  /**
   * HTTP headers to include
   */
  headers?: KeyValueInput[];
  /**
   * Transport type
   */
  type: "sse";
  /**
   * Server-Sent Events endpoint URL template. Must start with http://, https://, or a template variable (e.g., {baseUrl}). Variables in {curly_braces} are resolved based on context: In Package context, they reference argument valueHints, argument names, or environment variable names from the parent Package. In Remote context, they reference variables from the transport's 'variables' object. After variable substitution, this should produce a valid URI.
   */
  url: string;
}
/**
 * This interface was referenced by `ServerJsonDefiningAModelContextProtocolMCPServer`'s JSON-Schema
 * via the `definition` "Package".
 */
export interface Package {
  /**
   * A mapping of environment variables to be set when running the package.
   */
  environmentVariables?: KeyValueInput[];
  /**
   * SHA-256 hash of the package file for integrity verification. Required for MCPB packages and optional for other package types. Authors are responsible for generating correct SHA-256 hashes when creating server.json. If present, MCP clients must validate the downloaded file matches the hash before running packages to ensure file integrity.
   */
  fileSha256?: string;
  /**
   * Package identifier - either a package name (for registries) or URL (for direct downloads)
   */
  identifier: string;
  /**
   * A list of arguments to be passed to the package's binary.
   */
  packageArguments?: Argument[];
  /**
   * Base URL of the package registry
   */
  registryBaseUrl?: string;
  /**
   * Registry type indicating how to download packages (e.g., 'npm', 'pypi', 'cargo', 'oci', 'nuget', 'mcpb')
   */
  registryType: string;
  /**
   * A list of arguments to be passed to the package's runtime command (such as docker or npx). The `runtimeHint` field should be provided when `runtimeArguments` are present.
   */
  runtimeArguments?: Argument[];
  /**
   * A hint to help clients determine the appropriate runtime for the package. This field should be provided when `runtimeArguments` are present.
   */
  runtimeHint?: string;
  /**
   * Transport protocol configuration for the package
   */
  transport: StdioTransport | StreamableHttpTransport | SseTransport;
  /**
   * Package version. Must be a specific version. Version ranges are rejected (e.g., '^1.2.3', '~1.2.3', '>=1.2.3', '1.x', '1.*').
   */
  version?: string;
}
/**
 * Repository metadata for the MCP server source code. Enables users and security experts to inspect the code, improving transparency.
 *
 * This interface was referenced by `ServerJsonDefiningAModelContextProtocolMCPServer`'s JSON-Schema
 * via the `definition` "Repository".
 */
export interface Repository {
  /**
   * Repository identifier from the hosting service (e.g., GitHub repo ID). Owned and determined by the source forge. Should remain stable across repository renames and may be used to detect repository resurrection attacks - if a repository is deleted and recreated, the ID should change. For GitHub, use: gh api repos/<owner>/<repo> --jq '.id'
   */
  id?: string;
  /**
   * Repository hosting service identifier. Used by registries to determine validation and API access methods.
   */
  source: string;
  /**
   * Optional relative path from repository root to the server location within a monorepo or nested package structure. Must be a clean relative path.
   */
  subfolder?: string;
  /**
   * Repository URL for browsing source code. Should support both web browsing and git clone operations.
   */
  url: string;
}
/**
 * Schema for a static representation of an MCP server. Used in various contexts related to discovery, installation, and configuration.
 *
 * This interface was referenced by `ServerJsonDefiningAModelContextProtocolMCPServer`'s JSON-Schema
 * via the `definition` "ServerDetail".
 */
export interface ServerDetail {
  /**
   * JSON Schema URI for this server.json format
   */
  $schema?: string;
  /**
   * Extension metadata using reverse DNS namespacing for vendor-specific data
   */
  _meta?: {
    /**
     * Publisher-provided metadata for downstream registries
     */
    "io.modelcontextprotocol.registry/publisher-provided"?: {
      [k: string]: unknown;
    };
  };
  /**
   * Clear human-readable explanation of server functionality. Should focus on capabilities, not implementation details.
   */
  description: string;
  /**
   * Optional set of sized icons that the client can display in a user interface. Clients that support rendering icons MUST support at least the following MIME types: image/png and image/jpeg (safe, universal compatibility). Clients SHOULD also support: image/svg+xml (scalable but requires security precautions) and image/webp (modern, efficient format).
   */
  icons?: Icon[];
  /**
   * Server name in reverse-DNS format. Must contain exactly one forward slash separating namespace from server name.
   */
  name: string;
  packages?: Package[];
  remotes?: RemoteTransport[];
  repository?: Repository1;
  /**
   * Optional human-readable title or display name for the MCP server. MCP subregistries or clients MAY choose to use this for display purposes.
   */
  title?: string;
  /**
   * Version string for this server. SHOULD follow semantic versioning (e.g., '1.0.2', '2.1.0-alpha'). Equivalent of Implementation.version in MCP specification. Non-semantic versions are allowed but may not sort predictably. Version ranges are rejected (e.g., '^1.2.3', '~1.2.3', '>=1.2.3', '1.x', '1.*').
   */
  version: string;
  /**
   * Optional URL to the server's homepage, documentation, or project website. This provides a central link for users to learn more about the server. Particularly useful when the server has custom installation instructions or setup requirements.
   */
  websiteUrl?: string;
}
/**
 * Optional repository metadata for the MCP server source code. Recommended for transparency and security inspection.
 */
export interface Repository1 {
  /**
   * Repository identifier from the hosting service (e.g., GitHub repo ID). Owned and determined by the source forge. Should remain stable across repository renames and may be used to detect repository resurrection attacks - if a repository is deleted and recreated, the ID should change. For GitHub, use: gh api repos/<owner>/<repo> --jq '.id'
   */
  id?: string;
  /**
   * Repository hosting service identifier. Used by registries to determine validation and API access methods.
   */
  source: string;
  /**
   * Optional relative path from repository root to the server location within a monorepo or nested package structure. Must be a clean relative path.
   */
  subfolder?: string;
  /**
   * Repository URL for browsing source code. Should support both web browsing and git clone operations.
   */
  url: string;
}
