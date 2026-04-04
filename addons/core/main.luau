---@class HookRegisters
---@field eax integer
---@field ebx integer
---@field ecx integer
---@field edx integer
---@field esi integer
---@field edi integer
---@field ebp integer
---@field esp integer

---@class HookContext
---@field call_site integer
---@field return_address integer
---@field registers HookRegisters
---@field removed_bytes string|table
---@field trampoline_bytes string|table

local function dump(value, indent, visited, out)
    indent  = indent or 0
    visited = visited or {}
    out     = out or {}

    local function push(s) out[#out + 1] = s end

    if type(value) ~= "table" then
        push(tostring(value))
        return out
    end

    if visited[value] then
        push("<cycle>")
        return out
    end
    visited[value] = true

    push("{\n")
    for k, v in pairs(value) do
        push(string.rep("  ", indent + 1))
        push(tostring(k))
        push(" = ")
        dump(v, indent + 1, visited, out)
        push(",\n")
    end
    push(string.rep("  ", indent))
    push("}")

    return out
end

---@param ctx HookContext
function handle_hook(ctx)
    if type(ctx) ~= "table" then
        error("Invalid argument")
    end

    -- required top-level fields
    if not ctx.call_site or not ctx.return_address or not ctx.registers then
        error("Invalid argument")
    end

    local r = ctx.registers

    -- validate registers
    if type(r) ~= "table"
        or r.eax == nil or r.ebx == nil
        or r.ecx == nil or r.edx == nil
        or r.esi == nil or r.edi == nil
        or r.ebp == nil or r.esp == nil
    then
        error("Invalid argument")
    end

    -- optional but expected fields
    local removed = ctx.removed_bytes
    local tramp   = ctx.trampoline_bytes

    -- usage example
    -- minimal logic, replace with actual handling
    -- print(string.format("call_site=-3x%X return=0x%X", ctx.call_site, ctx.return_address))
    print(string.format("call_site=0x%X return=0x%X", ctx.call_site, ctx.return_address))

    -- print entire ctx
    local parts = dump(ctx)
    print(table.concat(parts))

    -- print(string.format("removed bytes=%s", removed))

    info( "TEST MESSAGE")

    error("Invalid argument")

    return {
        run_trampoline = true,
        registers = r,
        trampoline_bytes = tramp
    }
end

return {
    -- Called when the mod is loaded
    init = function ()
        warning( "ON INIT")
    end,

    post_init = function ()
        info( "POST INIT CALLED")
    end,

    unload = function ()
        warning( "UNLOAD CALLED")
    end,

    load = function ()
        warning( "LOAD CALLED")
    end,

    quit = function ()
        info( "QUIT CALLED")
    end,

    test_event = function(ctx)
        warning( "TEST EVENT CALLED")

        handle_hook(ctx)
    end,
}
