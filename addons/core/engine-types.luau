--!strict
--[[
    engine-types.luau

    Static declarations for globals injected by CCCaster at runtime.
    These are not standard Luau globals; they come from the host engine.
]]

export type HexByte = string -- e.g. "90" or "FF"

export type HookRegisters = {
    eax: number,
    ebx: number,
    ecx: number,
    edx: number,
    esi: number,
    edi: number,
    ebp: number,
    esp: number,
}

export type HookContext = {
    call_site: number,
    return_address: number,
    registers: HookRegisters,
    removed_bytes: string | { [number]: number }?,
    trampoline_bytes: string | { [number]: number }?,
}

export type HookDecision = {
    run_trampoline: boolean?,
    registers: HookRegisters?,
    trampoline_bytes: { [number]: number }?,
}

export type EngineLogLevel = {
    error: number,
    warning: number,
    info: number,
    debug: number,
    trace: number,
}

export type EngineLogApi = {
    -- Log with an explicit level constant from Engine.log.level.
    write: (level: number, message: string | number | boolean | nil) -> (),
    level: EngineLogLevel,
}

export type EngineMemoryHookApi = {
    -- Execute bytes replaced by a registered detour hook at a call-site address.
    execute: (call_site: number | string) -> boolean,
}

export type EngineMemoryPatchApi = {
    -- Create one or many runtime patches. Returns patch handle(s) or nil on failure.
    make: (...any) -> number | { [number]: number } | nil,
    -- Remove a previously created patch handle.
    remove: (id: number) -> boolean,
}

export type EngineMemoryApi = {
    -- Read bytes as a 1-indexed array of uppercase hex strings.
    read: (address: number | string, length: number) -> { [number]: HexByte } | nil,
    -- Write bytes from a hex payload string; returns true on success.
    write: (address: number | string, bytes: string) -> boolean,
    hook: EngineMemoryHookApi,
    patch: EngineMemoryPatchApi,
}

export type EngineFsMode = {
    write: "write",
    append: "append",
}

export type EngineFsLocalApi = {
    read: (path: string) -> string?,
    write: (path: string, content: string, mode: "write" | "append" | "w" | "a") -> boolean,
}

export type EngineFsGlobalApi = {
    read: (path: string) -> string?,
    write: (path: string, content: string, mode: "write" | "append" | "w" | "a") -> boolean,
}

export type EngineFsApi = {
    mode: EngineFsMode,
    immanent: EngineFsLocalApi,
    global: EngineFsGlobalApi,
}

export type EngineApi = {
    -- Namespaced runtime logger.
    log: EngineLogApi,

    -- Runtime memory, hook and patch helpers.
    memory: EngineMemoryApi,

    -- ADDON_* environment variables copied from process env.
    env: { [string]: string },

    -- Virtual filesystem APIs (registered per mod during load).
    fs: EngineFsApi,

    -- Dispatch an event into loaded mods that subscribed to it.
    dispatch: (event_name: string, ...any) -> any,

    -- Require a Luau script relative to the current mod root.
    require: (path: string) -> { [string]: any },

    -- Request unloading of the currently executing mod.
    unload: () -> boolean,

    -- Comparison mode constants for expect/assert APIs.
    EQ: "eq",
    NE: "ne",
    LT: "lt",
    LE: "le",
    GT: "gt",
    GE: "ge",

    -- Runtime assertion helpers.
    expect: (mode: string, left: any, right: any, message: string?) -> boolean,
    assert: (mode: string, left: any, right: any, message: string?) -> boolean,
    expect_death: (...any) -> boolean,

    -- Externally registered values from native host integrations.
    external: { [string]: any },

    -- Per-mod exports are attached under Engine[mod_id] at runtime.
    [string]: any,
}

-- Global engine namespace.
declare Engine: EngineApi

-- Global require is replaced with Engine.require.
declare function require(path: string): { [string]: any }

-- Host logging globals.
declare function warning(message: string | number | boolean | nil): ()
declare function info(message: string | number | boolean | nil): ()

-- Host error override: always raises.
declare function error(message: string | number | boolean | nil): never
