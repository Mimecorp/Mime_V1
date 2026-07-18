

interface OverlayProps {
  telemetry: any
}

export function Overlay({ telemetry }: OverlayProps) {
  return (
    <div style={{ position: 'absolute', top: 20, left: 20, zIndex: 10 }}>
      <div className="glass-panel" style={{ width: 320 }}>
        <h2 style={{ margin: '0 0 15px 0', fontSize: '18px', color: '#fff', letterSpacing: '1px' }}>
          PROJECT MIME
        </h2>
        
        <div className="data-row">
          <span className="data-label">Math Engine</span>
          <span className="data-value" style={{ color: telemetry ? '#64ffda' : '#ff3366' }}>
            {telemetry ? 'ONLINE (EKF FUSED)' : 'OFFLINE'}
          </span>
        </div>

        {telemetry && (
          <>
            <div style={{ marginTop: 20, marginBottom: 8, fontSize: 11, color: '#fff', letterSpacing: '1px' }}>ABSOLUTE POSITION (M)</div>
            <div className="data-row">
              <span className="data-label">X</span><span className="data-value">{telemetry.px.toFixed(3)}</span>
            </div>
            <div className="data-row">
              <span className="data-label">Y</span><span className="data-value">{telemetry.py.toFixed(3)}</span>
            </div>
            <div className="data-row">
              <span className="data-label">Z</span><span className="data-value">{telemetry.pz.toFixed(3)}</span>
            </div>

            <div style={{ marginTop: 20, marginBottom: 8, fontSize: 11, color: '#fff', letterSpacing: '1px' }}>ABSOLUTE VELOCITY (M/S)</div>
            <div className="data-row">
              <span className="data-label">X</span><span className="data-value">{telemetry.vx.toFixed(3)}</span>
            </div>
            <div className="data-row">
              <span className="data-label">Y</span><span className="data-value">{telemetry.vy.toFixed(3)}</span>
            </div>
            <div className="data-row">
              <span className="data-label">Z</span><span className="data-value">{telemetry.vz.toFixed(3)}</span>
            </div>

            <div style={{ marginTop: 20, marginBottom: 8, fontSize: 11, color: '#fff', letterSpacing: '1px' }}>ORIENTATION (QUATERNION)</div>
            <div className="data-row">
              <span className="data-label">W</span><span className="data-value">{telemetry.orientation.w.toFixed(3)}</span>
            </div>
            <div className="data-row">
              <span className="data-label">X</span><span className="data-value">{telemetry.orientation.x.toFixed(3)}</span>
            </div>
            <div className="data-row">
              <span className="data-label">Y</span><span className="data-value">{telemetry.orientation.y.toFixed(3)}</span>
            </div>
            <div className="data-row">
              <span className="data-label">Z</span><span className="data-value">{telemetry.orientation.z.toFixed(3)}</span>
            </div>
          </>
        )}
      </div>
    </div>
  )
}
