---@meta Device
local Device = {}

---@class DeviceParams
---@field type string The device type id (e.g. "GenericLinuxOsType").
---@field displayName? string The name shown in the device list.
---@field host? string The SSH host name or IP address.
---@field port? integer The SSH port (default 22).
---@field userName? string The SSH user name.
---@field privateKeyFile? string Path to the private key file.
---@field useKeyFile? boolean If true, authenticate with the given private key only.
---@field timeout? integer Connection timeout in seconds.
---@field hostKeyCheckingMode? string One of "none", "strict", "allowNoMatch".
local DeviceParams = {}

---@class DeviceKit
---@field id string The kit id.
---@field name string The kit display name.
---@field valid boolean Whether the kit is valid.
local DeviceKit = {}

---Creates a device from the given parameters and adds it to the device manager.
---@param params DeviceParams
---@return string id The id of the newly created device.
function Device.createDevice(params) end

---Removes the device with the given id from the device manager.
---@param id string
function Device.removeDevice(id) end

---Connects to the device and runs tool auto-detection (toolchains, debuggers,
---Qt versions and on-device build tools), then (re)creates the device's kits.
---Call `a.wait` on the returned value to get the result: a list of DeviceKit on
---success, or an error string on failure.
---@param id string The device id.
---@return DeviceKit[]|string
function Device.detectTools(id) end

---Checks whether a TCP connection to host:port succeeds within timeoutMs
---(default 5000). Call `a.wait` on the returned value to get a boolean.
---@param host string The host name or IP address.
---@param port integer The TCP port.
---@param timeoutMs? integer Connection timeout in milliseconds (default 5000).
---@return boolean
function Device.isReachable(host, port, timeoutMs) end

return Device
