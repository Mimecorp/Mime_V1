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
      // Commented out temporarily to prevent "Rocketship Drift" from IMU double-integration without optical tracking
      // groupRef.current.position.set(telemetry.px, telemetry.py, telemetry.pz)

      if (telemetry.orientation) {
        const { w, x, y, z } = telemetry.orientation
        // Translate IMU coordinate system into the browser's 3D viewport (Y-up right-handed)
        // using the exact mapping from your blueprint: -q2, -q3, q1, q0 (where q0=w, q1=x, q2=y, q3=z)
        const targetQuat = new THREE.Quaternion(-y, -z, x, w)
        // Smoothly interpolate for a premium 60fps feel
        groupRef.current.quaternion.slerp(targetQuat, 0.3)
      }

      // Actuate the fingers if the URDF is loaded and flex data exists
      if (robot && robot.joints && telemetry.flex_sensors) {
        const thumbFlex = telemetry.flex_sensors.thumb || 0;
        const indexFlex = telemetry.flex_sensors.index || 0;
        
        // Map 0.0-1.0 flex to radians (approx 90 degrees)
        if (robot.joints['thumb_joint']) {
          robot.joints['thumb_joint'].setJointValue(-1.57 * thumbFlex);
        }
        if (robot.joints['index_joint']) {
          robot.joints['index_joint'].setJointValue(1.57 * indexFlex);
        }
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
