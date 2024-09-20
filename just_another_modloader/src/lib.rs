#![cfg(windows)]

use {
    // Copyright (c) 2020 Canop

    // Permission is hereby granted, free of charge, to any person obtaining a copy
    // of this software and associated documentation files (the "Software"), to deal
    // in the Software without restriction, including without limitation the rights
    // to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    // copies of the Software, and to permit persons to whom the Software is
    // furnished to do so, subject to the following conditions:

    // The above copyright notice and this permission notice shall be included in all
    // copies or substantial portions of the Software.

    // THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    // IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    // FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    // AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    // LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    // OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    // SOFTWARE.
    deser_hjson,

    // Copyright (c) 2010 The Rust Project Developers

    // Permission is hereby granted, free of charge, to any
    // person obtaining a copy of this software and associated
    // documentation files (the "Software"), to deal in the
    // Software without restriction, including without
    // limitation the rights to use, copy, modify, merge,
    // publish, distribute, sublicense, and/or sell copies of
    // the Software, and to permit persons to whom the Software
    // is furnished to do so, subject to the following
    // conditions:

    // The above copyright notice and this permission notice
    // shall be included in all copies or substantial portions
    // of the Software.

    // THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
    // ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
    // TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
    // PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
    // SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    // CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
    // OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
    // IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    // DEALINGS IN THE SOFTWARE.
    lazy_static::lazy_static,

    // Copyright (c) 2014 The Rust Project Developers

    // Permission is hereby granted, free of charge, to any
    // person obtaining a copy of this software and associated
    // documentation files (the "Software"), to deal in the
    // Software without restriction, including without
    // limitation the rights to use, copy, modify, merge,
    // publish, distribute, sublicense, and/or sell copies of
    // the Software, and to permit persons to whom the Software
    // is furnished to do so, subject to the following
    // conditions:

    // The above copyright notice and this permission notice
    // shall be included in all copies or substantial portions
    // of the Software.

    // THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
    // ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
    // TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
    // PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
    // SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    // CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
    // OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
    // IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    // DEALINGS IN THE SOFTWARE.
    serde::Deserialize,

    std::{collections::HashMap, fs, sync::Mutex, thread, vec::Vec},

    // Copyright (c) 2015-2018 The winapi-rs Developers

    // Permission is hereby granted, free of charge, to any person obtaining a copy
    // of this software and associated documentation files (the "Software"), to deal
    // in the Software without restriction, including without limitation the rights
    // to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    // copies of the Software, and to permit persons to whom the Software is
    // furnished to do so, subject to the following conditions:

    // The above copyright notice and this permission notice shall be included in all
    // copies or substantial portions of the Software.

    // THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    // IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    // FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    // AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    // LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    // OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    // SOFTWARE.
    winapi::{
        shared::{
            minwindef::{BOOL, DWORD, FARPROC, HINSTANCE, HMODULE, LPVOID, TRUE},
            ntdef::{LPCSTR, NULL},
        },
        um::{
            consoleapi::AllocConsole,
            libloaderapi::{GetProcAddress, LoadLibraryA},
        },
    },
};

#[derive(Clone, Default, Deserialize, PartialEq, Debug)]
struct ModInfo {
    name: String,
    description: Option<String>,
    update: Option<String>,
    version: Option<String>,
    source: Option<String>,
    main: String,
    events: HashMap<String, Option<String>>,
}

macro_rules! v_address_to_function {
    ( $address:expr, $t:ty ) => {
        std::mem::transmute::<*const (), $t>($address as _)
    };
}

lazy_static! {
    #[ derive( Debug ) ]
    static ref ON_EVENT_FUNCTIONS: Mutex< HashMap< String, Vec< usize > > > = Mutex::new( HashMap::new() );
}

/// Rust doesn't officially support a "life before main()",
/// though it is unclear what that that means exactly for DllMain.
#[no_mangle]
#[allow(non_snake_case, unused_variables)]
unsafe extern "system" fn DllMain(
    dll_module: HINSTANCE,
    call_reason: DWORD,
    reserved: LPVOID,
) -> BOOL {
    const DLL_PROCESS_ATTACH: DWORD = 1;
    const DLL_PROCESS_DETACH: DWORD = 0;

    match call_reason {
        DLL_PROCESS_ATTACH => init(),
        DLL_PROCESS_DETACH => (),
        _ => (),
    }

    TRUE
}

unsafe fn init() {
    // Allocate console, so the game will print out loaded assets.
    if cfg!(debug_assertions) {
        AllocConsole();
    }

    // Load addons.
    load_addons_directory();
}

// Load addons.
unsafe fn load_addons_directory() {
    let addons_paths: Vec<fs::DirEntry> = match fs::read_dir("addons") {
        Ok(directory) => directory
            .into_iter()
            .filter(|result| result.is_ok())
            .map(|result| result.unwrap())
            .filter(|entry| entry.path().is_dir())
            .collect(),
        // If there is no addons folder.
        Err(error) => {
            println!("addon: {:?}", error);

            vec![]
        }
    };

    addons_paths.into_iter().for_each(|entry| {
        if cfg!(debug_assertions) {
            println!("addon loaded: {}", entry.path().display());
        }

        // Load mod information from info.hjson.
        let mod_info: ModInfo = deser_hjson::from_str(
            &fs::read_to_string(format!("{}/info.hjson", entry.path().to_str().unwrap()))
                .unwrap_or_default(),
        )
        .unwrap_or_default();

        if cfg!(debug_assertions) {
            println!("name       : {},", mod_info.name);

            println!("description: {},", mod_info.description.unwrap_or_default());

            println!("version    : {},", mod_info.version.unwrap_or_default());
        }

        // Load main DLL into the process.
        let mod_h_module: HMODULE = LoadLibraryA(
            format!("{}\\{}\0", entry.path().to_str().unwrap(), &mod_info.main).as_ptr() as LPCSTR,
        ) as HMODULE;

        let not_specified = Option::from(String::from("Not specified"));

        // Get specified to events function addresses.
        for (event_name, event) in mod_info.events.iter() {
            if event != &not_specified {
                let on_event_old_functions = ON_EVENT_FUNCTIONS.lock().unwrap().insert(
                    String::from(event_name),
                    vec![GetProcAddress(
                        mod_h_module,
                        format!("{}\0", event.as_ref().unwrap()).as_ptr() as LPCSTR,
                    ) as usize],
                );

                if let Some(on_event_old_functions) = on_event_old_functions {
                    let mut events = on_event_old_functions.clone();

                    events.push(GetProcAddress(
                        mod_h_module,
                        format!("{}\0", event.as_ref().unwrap()).as_ptr() as LPCSTR,
                    ) as usize);

                    ON_EVENT_FUNCTIONS
                        .lock()
                        .unwrap()
                        .insert(String::from(event_name), events);
                }

                if cfg!(debug_assertions) {
                    println!(
                        "{}: {:#?}\n",
                        String::from(event_name),
                        ON_EVENT_FUNCTIONS.lock().unwrap().get(event_name)
                    );
                }
            }
        }
    });

    // Load event handling DLL into the game process.
    let states_h_module = LoadLibraryA(format!("states.dll\0",).as_ptr() as LPCSTR) as HMODULE;

    if cfg!(debug_assertions) {
        println!("states.dll : {:?}", states_h_module);
    }

    if states_h_module != NULL as HMODULE {
        let states_function_add_callbacks = v_address_to_function!(
            GetProcAddress(states_h_module, ("addCallbacks\0").as_ptr() as LPCSTR) as FARPROC,
            extern "C" fn(LPCSTR, usize, *const usize, u32)
        );

        if cfg!(debug_assertions) {
            println!("addCallbacks : {:?}", states_function_add_callbacks);
        }

        if states_function_add_callbacks != v_address_to_function!(
            NULL,
            extern "C" fn(LPCSTR, usize, *const usize, u32)
        ) {
            // Load callbacks into the event handling DLL.
            thread::spawn(move || {
                for (event_name, function_addresses) in ON_EVENT_FUNCTIONS.lock().unwrap().iter() {
                    if cfg!(debug_assertions) {
                        println!(
                            "{} : {} -> {:#02X?}",
                            event_name,
                            function_addresses.len(),
                            function_addresses.to_owned()
                        );
                    }

                    states_function_add_callbacks(
                        format!("{}\0", event_name).as_ptr() as LPCSTR,
                        function_addresses.len(),
                        function_addresses.to_owned().as_ptr(),
                        0,
                    );
                }
            });
        }
    }
}
