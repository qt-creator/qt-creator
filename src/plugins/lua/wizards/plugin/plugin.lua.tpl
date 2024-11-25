return {
    Id = "%{JS: value('ProjectName').toLowerCase().replace(/ /g, '')}",
    Name = "%{PluginName}",
    Version = "1.0.0",
    CompatVersion = "1.0.0",
    Vendor = "%{VendorName}",
    VendorId = "%{JS: value('VendorName').toLowerCase().replace(/ /g, '')}",
    Copyright = "%{Copyright}",
    License = "%{License}",
    Category = "My Plugins",
    Description = "%{Description}",
    Url = "%{Url}",
    DocumentationUrl = "",
    Experimental = true,
    DisabledByDefault = false,
    LongDescription = [[
This plugin provides some functionality.
You can describe it more here.
    ]],
    Dependencies = {
        { Id = "lua",  Version = "%{JS: Util.qtCreatorIdeVersion()}" },
    },
    setup = function()
        require 'init'.setup()
    end
} --[[@as QtcPlugin]]
