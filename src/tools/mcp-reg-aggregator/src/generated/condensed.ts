/* eslint-disable */
// Auto-generated from the JSON Schema files. Do not edit by hand.
// Run `npm run gen:types` to regenerate.

/**
 * Condensed snapshot of the MCP server registry
 */
export interface McpRegistry {
  /**
   * ISO 8601 timestamp when this snapshot was generated
   */
  generated_at: string;
  /**
   * Number of servers in this snapshot
   */
  count: number;
  servers: Server[];
}
export interface Server {
  /**
   * Reverse-DNS scoped server name, e.g. 'io.github.user/repo'
   */
  name: string;
  /**
   * Human-readable display name
   */
  title?: string;
  /**
   * Short description of what the server does
   */
  description: string;
  /**
   * Semantic version of this server entry
   */
  version: string;
  /**
   * Lifecycle status of this server
   */
  status: "active" | "deprecated" | "deleted";
  /**
   * URL to the source repository
   */
  repository_url?: string;
  /**
   * URL to the project website or documentation
   */
  website_url?: string;
  /**
   * Base64-embedded icons for this server
   */
  icons?: Icon[];
  /**
   * Distribution packages for running this server locally
   */
  packages?: Package[];
  /**
   * Remote transport endpoints for this server
   */
  remotes?: Remote[];
}
export interface Icon {
  /**
   * Original icon URL
   */
  url: string;
  /**
   * Base64-encoded image bytes, present for SVG icons only
   */
  data?: string;
  /**
   * MIME type of the image, e.g. 'image/png'
   */
  mime_type?: string;
  /**
   * Icon size descriptors, e.g. ['48x48', 'any']
   */
  sizes?: string[];
  /**
   * Preferred color scheme this icon is designed for
   */
  theme?: "light" | "dark";
}
export interface Package {
  /**
   * Package registry type
   */
  registry_type: "npm" | "pypi" | "oci" | "nuget" | "mcpb";
  /**
   * Package name or download URL
   */
  identifier: string;
  /**
   * Pinned version of the package
   */
  version?: string;
  /**
   * Transport mechanism used by this package
   */
  transport_type: "stdio" | "streamable-http" | "sse";
  /**
   * Suggested runtime command, e.g. 'npx', 'uvx', 'docker'
   */
  runtime_hint?: string;
  /**
   * Arguments to pass to the package's runtime command
   */
  runtime_arguments?: Argument[];
  /**
   * Arguments to pass to the package's binary
   */
  package_arguments?: Argument[];
  /**
   * HTTP headers to include in requests, present for streamable-http and sse transports only
   */
  headers?: KeyValueInput[];
  /**
   * Environment variables accepted or required by this package
   */
  env_vars?: KeyValueInput[];
}
export interface Argument {
  /**
   * Whether this is a positional or named (flag) argument
   */
  type: "positional" | "named";
  /**
   * Flag name including leading dashes, e.g. '--port'. Null for positional arguments.
   */
  name?: string;
  /**
   * Identifier for positional arguments, e.g. 'file_path'. Null for named arguments.
   */
  value_hint?: string;
  /**
   * What this argument is for
   */
  description?: string;
  /**
   * Whether this argument must be provided
   */
  required: boolean;
  /**
   * Whether this argument can be repeated multiple times
   */
  repeated: boolean;
  /**
   * Default value if not provided
   */
  default?: string;
  /**
   * Fixed value for this argument
   */
  value?: string;
}
export interface KeyValueInput {
  /**
   * Environment variable name, e.g. 'OPENWEATHER_API_KEY'
   */
  name: string;
  /**
   * What this variable is for
   */
  description?: string;
  /**
   * Whether this variable must be set to run the server
   */
  required: boolean;
  /**
   * Whether this variable holds a secret / credential
   */
  secret: boolean;
  /**
   * Default value if not provided
   */
  default?: string;
}
export interface Remote {
  /**
   * Remote transport protocol
   */
  type: "streamable-http" | "sse";
  /**
   * Endpoint URL for this remote transport
   */
  url: string;
  /**
   * HTTP headers to include in requests
   */
  headers?: KeyValueInput[];
}
