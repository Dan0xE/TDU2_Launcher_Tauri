// Prevents additional console window on Windows in release, DO NOT REMOVE!!
#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

use std::ffi::CString;

#[link(name = "inject", kind = "static")]
extern "C" {
    fn InjectDLL(dll_path: *const u8) -> bool;
    fn run_game_with_mutex(path: *const u8) -> ();
    fn is_process_running() -> bool;
    fn handle_game_thread() -> bool;
}

#[tauri::command]
fn run_game(path: String) -> String {
    std::thread::spawn(|| {
        let c_string: CString = CString::new(path).unwrap();
        let ptr: *const u8 = c_string.as_ptr() as *const u8;
        unsafe { run_game_with_mutex(ptr) };
    });

    return "Run Game Thread Spawned".to_string();
}

#[tauri::command]
fn is_game_running() -> bool {
    unsafe { is_process_running() }
}

#[tauri::command]
fn inject(path: String) -> bool {
    let c_string: CString = CString::new(path).unwrap();
    let ptr: *const u8 = c_string.as_ptr() as *const u8;
    unsafe { InjectDLL(ptr) }
}

#[tauri::command]
fn handle_thread() -> bool {
    unsafe { handle_game_thread() }
}

fn main() {
    tauri::Builder::default()
        .invoke_handler(tauri::generate_handler![
            inject,
            run_game,
            is_game_running,
            handle_thread
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
