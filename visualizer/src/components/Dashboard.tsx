import { useState, useEffect } from 'react';
import { invoke } from '@tauri-apps/api/tauri';
import './Dashboard.css';

interface SystemStatus {
  central_engine: string;
  relay_node: string;
  optical_tracker: string;
  node_bridge: string;
  ik_engine?: string;
}

export default function Dashboard({ children, telemetry }: { children?: React.ReactNode, telemetry?: any }) {
  const [status, setStatus] = useState<SystemStatus>({
    central_engine: 'BOOTING',
    relay_node: 'BOOTING',
    optical_tracker: 'BOOTING',
    node_bridge: 'BOOTING',
    ik_engine: 'OFFLINE'
  } as any);

  useEffect(() => {
    // Poll the Rust backend for process status every 2 seconds
    const interval = setInterval(async () => {
      try {
        const res: SystemStatus = await invoke('get_system_status');
        setStatus(res);
      } catch (e) {
        console.error("IPC Error:", e);
      }
    }, 2000);

    return () => clearInterval(interval);
  }, []);

  const getStatusClass = (state: string) => {
    if (state === 'ONLINE') return 'status-online';
    if (state === 'OFFLINE' || state === 'ERROR') return 'status-offline';
    return 'status-booting';
  };

  // Helper to safely format angles from telemetry
  const getAngle = (index: number) => {
    if (telemetry?.ik_angles && telemetry.ik_angles.length > index) {
      return (telemetry.ik_angles[index] * (180/Math.PI)).toFixed(1) + '°';
    }
    return '0.0°';
  }

  return (
    <div className="tactical-dashboard">
      <div className="header">
        <span>MIME | TACTICAL ROBOTICS CONTROL SOFTWARE</span>
        <span>STATUS: <span className="status-online">OPERATIONAL</span> [UNIT: ALPHA-7]</span>
        <span>16:34:21 GMT</span>
      </div>

      <div className="main-content">
        <div className="sidebar">
          <div className="panel">
            <div className="panel-title">[SYSTEM STATUS]</div>
            <div className="status-row">
              <span>CENTRAL ENGINE</span>
              <span className={getStatusClass(status.central_engine)}>{status.central_engine}</span>
            </div>
            <div className="status-row">
              <span>RELAY NODE</span>
              <span className={getStatusClass(status.relay_node)}>{status.relay_node}</span>
            </div>
            <div className="status-row">
              <span>OPT. TRACKER</span>
              <span className={getStatusClass(status.optical_tracker)}>{status.optical_tracker}</span>
            </div>
            <div className="status-row">
              <span>NODE BRIDGE</span>
              <span className={getStatusClass(status.node_bridge)}>{status.node_bridge}</span>
            </div>
            <div className="status-row">
              <span>IK SOLVER (REAL ARM)</span>
              <span className={getStatusClass((status as any).ik_engine || 'OFFLINE')}>{(status as any).ik_engine || 'OFFLINE'}</span>
            </div>
          </div>

          <div className="panel">
            <div className="panel-title">[ACTUATOR STATES]</div>
            <div className="table-header">
              <span>ACT</span>
              <span>ANGLE</span>
              <span>TORQUE</span>
            </div>
            <div className="table-row">
              <span>J1 (Base)</span>
              <span>{getAngle(0)}</span>
              <span>12.1Nm</span>
            </div>
            <div className="table-row">
              <span>J2 (Shoulder)</span>
              <span>{getAngle(1)}</span>
              <span>8.4Nm</span>
            </div>
            <div className="table-row">
              <span>J3 (Elbow)</span>
              <span>{getAngle(2)}</span>
              <span>2.1Nm</span>
            </div>
            <div className="table-row">
              <span>J4 (Wrist)</span>
              <span>{getAngle(3)}</span>
              <span>1.0Nm</span>
            </div>
          </div>

          <div className="panel">
            <div className="panel-title">[MISSION CONTROL]</div>
            <div className="button-group">
              <button className="tactical-btn" onClick={() => invoke('start_ik_engine')}>[START IK ENGINE]</button>
              <button className="tactical-btn" onClick={() => invoke('replay_latest_log')}>[REPLAY LOG]</button>
            </div>
            <div className="progress-bar">
              {telemetry ? 'TELEMETRY STREAM: ACTIVE' : 'TELEMETRY STREAM: WAITING'}
              <div className="progress-fill" style={{width: telemetry ? '100%' : '0%'}}></div>
            </div>
          </div>
        </div>

        <div className="center-panel">
          <div className="panel-title">[3D VISUALIZATION]</div>
          <div className="canvas-container">
            {children}
          </div>
        </div>
      </div>
    </div>
  );
}
