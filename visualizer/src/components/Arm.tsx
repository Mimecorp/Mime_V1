import React, { useRef } from 'react'
import { useFrame } from '@react-three/fiber'
import * as THREE from 'three'

interface ArmProps {
  telemetry: any
}

export function Arm({ telemetry }: ArmProps) {
  const meshRef = useRef<THREE.Group>(null)

  useFrame(() => {
    if (meshRef.current && telemetry) {
      // Apply Fused Position from EKF
      meshRef.current.position.set(telemetry.px, telemetry.py, telemetry.pz)

      // Apply Orientation (Quaternion) from EKF
      if (telemetry.orientation) {
        const { w, x, y, z } = telemetry.orientation
        const quaternion = new THREE.Quaternion(x, y, z, w) // Three.js constructor is x,y,z,w
        meshRef.current.quaternion.slerp(quaternion, 0.5) // Smooth interpolation for 60fps
      }
    }
  })

  return (
    <group ref={meshRef}>
      {/* Wrist/Hand Representation */}
      <mesh castShadow receiveShadow>
        <boxGeometry args={[0.1, 0.05, 0.2]} />
        <meshStandardMaterial color="#64ffda" metalness={0.8} roughness={0.2} />
      </mesh>
      
      {/* Directional Nub (points forward so you know orientation) */}
      <mesh position={[0, 0, 0.1]} castShadow>
        <cylinderGeometry args={[0.01, 0.01, 0.1]} />
        <meshStandardMaterial color="#ff3366" />
      </mesh>
    </group>
  )
}
