return {
    -- Called when the mod is loaded
    init = function ()
        Engine.log(Engine.log.level.error, "ON INIT")
    end,

    post_init = function ()
        Engine.log(Engine.log.level.error,"POST INIT CALLED")
    end,

    unload = function ()
        Engine.log(Engine.log.level.error,"UNLOAD CALLED")
    end,

    load = function ()
        Engine.log(Engine.log.level.error,"LOAD CALLED")
    end,

    quit = function ()
        Engine.log(Engine.log.level.error,"QUIT CALLED")
    end,
}
