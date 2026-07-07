import React, { useEffect, useState } from 'react'
import { Canvas } from '@react-three/fiber'
import { OrbitControls, Environment, Grid } from '@react-three/drei'
import { io } from 'socket.io-client'
import { UrdfArm } from './components/UrdfArm'
import { Overlay } from './components/Overlay'

// Connect to our Node.js bridge
const socket = io('http://localhost:3001')

function App() {
  const [telemetry, setTelemetry] = useState<any>(null)

  useEffect(() => {
    socket.on('telemetry', (data) => {
      setTelemetry(data)
    })
    
    socket.on('connect', () => console.log('Connected to Node.js Bridge'))
    socket.on('disconnect', () => setTelemetry(null))
    
    return () => {
      socket.off('telemetry')
    }
  }, [])

  return (
    <>
      <Overlay telemetry={telemetry} />
      
      <Canvas shadows camera={{ position: [0.5, 0.5, 0.5], fov: 50 }}>
        <color attach="background" args={['#0f1115']} />
        
        {/* Cinematic Lighting */}
        <ambientLight intensity={0.2} />
        <directionalLight 
          position={[5, 5, 5]} 
          intensity={1} 
          castShadow 
          shadow-mapSize={2048}
        />
        <pointLight position={[-5, 5, -5]} intensity={0.5} color="#64ffda" />
        
        {/* Environment Reflections */}
        <Environment preset="city" />

        {/* Minimalist Grid Floor */}
        <Grid 
          infiniteGrid 
          fadeDistance={10} 
          sectionColor="#333" 
          cellColor="#111" 
          position={[0, -0.2, 0]}
        />
        
        {/* Render the new URDF Loader Component */}
        <UrdfArm telemetry={telemetry} />
        
        <OrbitControls makeDefault />
      </Canvas>
    </>
  )
}

export default App
