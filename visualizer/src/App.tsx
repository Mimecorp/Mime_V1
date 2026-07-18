import { useEffect, useState } from 'react'
import { Canvas } from '@react-three/fiber'
import { OrbitControls, Environment, Grid } from '@react-three/drei'
import { io } from 'socket.io-client'
import { UrdfArm } from './components/UrdfArm'
import Dashboard from './components/Dashboard'
import './App.css'

// Connect to our Node.js bridge
const socket = io('http://localhost:3001')

function App() {
  const [telemetry, setTelemetry] = useState<any>(null)

  useEffect(() => {
    // 1. Live Socket Data
    socket.on('telemetry', (data) => setTelemetry(data))
    socket.on('connect', () => console.log('Connected to Node.js Bridge'))
    socket.on('disconnect', () => setTelemetry(null))
    
    // 2. Tauri IPC Replay Data (Overrides Live Data)
    import('@tauri-apps/api/event').then(({ listen }) => {
      listen('replay_telemetry', (event) => {
        setTelemetry(event.payload)
      })
      
      listen('ik_angles', (event) => {
        // Expected payload: "J1:45.0° J2:30.0° J3:10.0° ..."
        const str = event.payload as string;
        const matches = str.match(/J\d:(-?\d+\.\d+)°/g);
        if (matches) {
          const angles = matches.map(m => parseFloat(m.split(':')[1]) * (Math.PI / 180)); // Convert to radians
          setTelemetry((prev: any) => ({ ...prev, ik_angles: angles, raw_ik_string: str }))
        }
      })
    });
    
    return () => {
      socket.off('telemetry')
    }
  }, [])

  return (
    <div style={{ position: 'relative', width: '100vw', height: '100vh', backgroundColor: '#020611', margin: 0, padding: 0, overflow: 'hidden' }}>
      <Dashboard telemetry={telemetry}>
        <Canvas 
          style={{ position: 'absolute', top: 0, left: 0, width: '100%', height: '100%' }}
          shadows 
          camera={{ position: [0.5, 0.5, 0.5], fov: 50 }}
        >
          <color attach="background" args={['transparent']} />
          
          <ambientLight intensity={0.5} />
          <directionalLight castShadow position={[2, 5, 2]} intensity={1.5} color="#ffffff" />
          <pointLight position={[-2, -2, -2]} intensity={1.0} color="#00f0ff" />
          <pointLight position={[2, 2, 2]} intensity={0.8} color="#00f0ff" />
          
          <Environment preset="night" />
          <Grid infiniteGrid fadeDistance={4} sectionColor="#00f0ff" sectionThickness={1.5} cellColor="#0a192f" />
          
          <UrdfArm telemetry={telemetry} />
          <OrbitControls makeDefault />
        </Canvas>
      </Dashboard>
    </div>
  )
}

export default App
