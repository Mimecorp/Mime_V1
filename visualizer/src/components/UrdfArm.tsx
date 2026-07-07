import React, { useRef, useEffect, useState } from 'react'
import { useFrame } from '@react-three/fiber'
import * as THREE from 'three'
import URDFLoader from 'urdf-loader'

interface UrdfArmProps {
  telemetry: any
}

export function UrdfArm({ telemetry }: UrdfArmProps) {
  const groupRef = useRef<THREE.Group>(null)
  const [robot, setRobot] = useState<any>(null)

  useEffect(() => {
    // Load the URDF file
    const manager = new THREE.LoadingManager()
    const loader = new URDFLoader(manager)
    
    loader.load('/robot.urdf', (loadedRobot: any) => {
      // Add materials/shadows to all parsed meshes
      loadedRobot.traverse((child: any) => {
        if (child.isMesh) {
          child.castShadow = true
          child.receiveShadow = true
          
          // Apply a generic metallic material to make it look premium
          child.material = new THREE.MeshStandardMaterial({
            color: child.material.color || '#64ffda',
            metalness: 0.7,
            roughness: 0.2
          })
        }
      })
      
      // Scale it up slightly so it's easier to see
      loadedRobot.scale.set(3, 3, 3)
      setRobot(loadedRobot)
    })
  }, [])

  useFrame(() => {
    if (groupRef.current && telemetry) {
      // Map EKF telemetry directly to the entire URDF group's transform
      groupRef.current.position.set(telemetry.px, telemetry.py, telemetry.pz)

      if (telemetry.orientation) {
        const { w, x, y, z } = telemetry.orientation
        const targetQuat = new THREE.Quaternion(x, y, z, w)
        // Smoothly interpolate for a premium 60fps feel
        groupRef.current.quaternion.slerp(targetQuat, 0.3)
      }
    }
  })

  return (
    <group ref={groupRef}>
      {/* Mount the loaded URDF object if it has finished parsing */}
      {robot && <primitive object={robot} />}
    </group>
  )
}
