// Prevents additional console window on Windows in release, DO NOT REMOVE!!
#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

use std::process::{Command, Child, Stdio};
use std::sync::Mutex;
use std::io::{BufRead, BufReader};
use tauri::{State, Manager, WindowEvent, Window};

struct AppState {
    central_engine: Mutex<Option<Child>>,
    relay_node: Mutex<Option<Child>>,
    optical_tracker: Mutex<Option<Child>>,
    node_bridge: Mutex<Option<Child>>,
    ik_engine: Mutex<Option<Child>>,
}

#[tauri::command]
fn get_system_status(state: State<AppState>) -> serde_json::Value {
    let check_process = |child: &Mutex<Option<Child>>| -> &'static str {
        let mut guard = child.lock().unwrap();
        if let Some(ref mut c) = *guard {
            match c.try_wait() {
                Ok(Some(_)) => "OFFLINE",
                Ok(None) => "ONLINE",
                Err(_) => "ERROR",
            }
        } else {
            "OFFLINE"
        }
    };

    serde_json::json!({
        "central_engine": check_process(&state.central_engine),
        "relay_node": check_process(&state.relay_node),
        "optical_tracker": check_process(&state.optical_tracker),
        "node_bridge": check_process(&state.node_bridge),
        "ik_engine": check_process(&state.ik_engine),
    })
}

#[tauri::command]
fn start_ik_engine(window: Window, state: State<AppState>) {
    println!("[Tauri] Booting IK Engine...");
    let mut child = Command::new("python3")
        .args(["ik_engine.py"])
        .current_dir("../../central_engine/ik_solver")
        .stdout(Stdio::piped())
        .spawn()
        .expect("Failed to spawn IK engine");
        
    let stdout = child.stdout.take().expect("Failed to open stdout");
    
    // Store process so it gets killed on exit
    *state.ik_engine.lock().unwrap() = Some(child);
    
    std::thread::spawn(move || {
        let reader = BufReader::new(stdout);
        for line in reader.lines() {
            if let Ok(l) = line {
                if l.contains("[IK Angles]") {
                    // Extract just the angles part: "J1:45.0° J2:..."
                    if let Some(idx) = l.find("J1:") {
                        let angles = &l[idx..];
                        let _ = window.emit("ik_angles", angles);
                    }
                }
            }
        }
    });
}

#[tauri::command]
fn replay_latest_log(window: Window) {
    std::thread::spawn(move || {
        println!("[Tauri] Searching for latest telemetry log...");
        let entries = std::fs::read_dir("../../central_engine");
        if let Ok(dir) = entries {
            let mut files: Vec<_> = dir.filter_map(Result::ok)
                .filter(|e| e.file_name().to_string_lossy().starts_with("telemetry_log_"))
                .collect();
            files.sort_by_key(|e| e.metadata().unwrap().modified().unwrap());
            
            if let Some(latest) = files.last() {
                println!("[Tauri] Replaying: {:?}", latest.path());
                let file = std::fs::File::open(latest.path()).unwrap();
                let reader = BufReader::new(file);
                
                // Read CSV header and then stream
                for line in reader.lines().skip(1) {
                    if let Ok(l) = line {
                        let parts: Vec<&str> = l.split(',').collect();
                        if parts.len() >= 13 {
                            let json = serde_json::json!({
                                "px": parts[1].parse::<f64>().unwrap_or(0.0),
                                "py": parts[2].parse::<f64>().unwrap_or(0.0),
                                "pz": parts[3].parse::<f64>().unwrap_or(0.0),
                                "orientation": {
                                    "w": parts[7].parse::<f64>().unwrap_or(1.0),
                                    "x": parts[8].parse::<f64>().unwrap_or(0.0),
                                    "y": parts[9].parse::<f64>().unwrap_or(0.0),
                                    "z": parts[10].parse::<f64>().unwrap_or(0.0)
                                },
                                "flex_sensors": {
                                    "thumb": parts[11].parse::<f64>().unwrap_or(0.0),
                                    "index": parts[12].parse::<f64>().unwrap_or(0.0)
                                }
                            });
                            let _ = window.emit("replay_telemetry", json);
                            std::thread::sleep(std::time::Duration::from_millis(10)); // ~100Hz
                        }
                    }
                }
                println!("[Tauri] Replay finished.");
            } else {
                println!("[Tauri] No logs found!");
            }
        }
    });
}

fn main() {
    println!("[Tauri] Booting Mime Microservices...");
    
    let central = Command::new("bazel").args(["run", "//engine:server", "--jobs=2"]).current_dir("../../central_engine").spawn().ok();
    let relay = Command::new("./build/mime_edge_node").current_dir("../../edge_nodes").spawn().ok();
    let tracker = Command::new("python3").args(["tracker.py"]).current_dir("../../edge_nodes/vision").spawn().ok();
    let bridge = Command::new("node").args(["server/index.js"]).current_dir("..").spawn().ok();

    let app_state = AppState {
        central_engine: Mutex::new(central),
        relay_node: Mutex::new(relay),
        optical_tracker: Mutex::new(tracker),
        node_bridge: Mutex::new(bridge),
        ik_engine: Mutex::new(None),
    };

    tauri::Builder::default()
        .manage(app_state)
        .invoke_handler(tauri::generate_handler![get_system_status, start_ik_engine, replay_latest_log])
        .on_window_event(|event| {
            if let WindowEvent::CloseRequested { .. } = event.event() {
                println!("[Tauri] Window closing, killing child processes...");
                let state: State<AppState> = event.window().state();
                
                let kill = |child: &Mutex<Option<Child>>| {
                    if let Some(mut c) = child.lock().unwrap().take() { let _ = c.kill(); }
                };
                
                kill(&state.central_engine);
                kill(&state.relay_node);
                kill(&state.optical_tracker);
                kill(&state.node_bridge);
                kill(&state.ik_engine);
            }
        })
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
