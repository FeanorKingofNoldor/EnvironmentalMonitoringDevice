# File: scripts/unified_build.py
#!/usr/bin/env python3
"""
Unified build script for AeroEnv/AeroLiquid devices
Generates build information and validates configuration based on device type
"""

Import("env")
import time
import os
import json
import hashlib
from datetime import datetime

# Device type detection from build flags
def get_device_type():
    build_flags = env.ParseFlags(env.get("BUILD_FLAGS", ""))
    defines = build_flags.get("CPPDEFINES", [])
    
    for define in defines:
        if isinstance(define, tuple):
            name, value = define
        else:
            name, value = define, None
            
        if name == "DEVICE_TYPE_ENVIRONMENTAL":
            return "environmental"
        elif name == "DEVICE_TYPE_LIQUID":
            return "liquid"
    
    return "unknown"

# Build information generation
def generate_build_info(source, target, env):
    """Generate build information header with device-specific content"""
    
    device_type = get_device_type()
    build_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    build_timestamp = int(time.time())
    
    # Get git information if available
    git_hash = "unknown"
    git_branch = "unknown"
    git_dirty = False
    
    try:
        import subprocess
        git_hash = subprocess.check_output(['git', 'rev-parse', '--short', 'HEAD']).decode('ascii').strip()
        git_branch = subprocess.check_output(['git', 'rev-parse', '--abbrev-ref', 'HEAD']).decode('ascii').strip()
        
        # Check if repository is dirty
        try:
            subprocess.check_output(['git', 'diff-index', '--quiet', 'HEAD'])
        except subprocess.CalledProcessError:
            git_dirty = True
            
    except Exception as e:
        print(f"Warning: Could not get git information: {e}")
    
    # Device-specific information
    device_info = {
        "environmental": {
            "name": "AeroEnv",
            "full_name": "AeroEnv Environmental Controller", 
            "sensors": ["temperature", "humidity", "pressure"],
            "actuators": ["lights", "spray", "fan"]
        },
        "liquid": {
            "name": "AeroLiquid",
            "full_name": "AeroLiquid Chemical Controller",
            "sensors": ["ph", "ec", "water_temp"],
            "actuators": ["pumps", "valves", "circulation"]
        },
        "unknown": {
            "name": "Unknown",
            "full_name": "Unknown Device",
            "sensors": [],
            "actuators": []
        }
    }
    
    device = device_info.get(device_type, device_info["unknown"])
    
    # Create build info header
    build_info_content = f"""#ifndef BUILD_INFO_H
#define BUILD_INFO_H

// Build information
#define BUILD_TIME "{build_time}"
#define BUILD_TIMESTAMP {build_timestamp}
#define BUILD_GIT_HASH "{git_hash}{'*' if git_dirty else ''}"
#define BUILD_GIT_BRANCH "{git_branch}"
#define BUILD_GIT_DIRTY {"1" if git_dirty else "0"}

// Firmware information
#define FIRMWARE_VERSION "1.0.0"
#define FIRMWARE_BUILD_TYPE "{env.get('BUILD_TYPE', 'debug').upper()}"

// Device information
#define DEVICE_TYPE "{device_type}"
#define DEVICE_NAME "{device['name']}"
#define DEVICE_FULL_NAME "{device['full_name']}"

// Feature flags based on device type
"""

    # Add device-specific feature flags
    if device_type == "environmental":
        build_info_content += """
// Environmental device features
#define HAS_TEMPERATURE_SENSOR 1
#define HAS_HUMIDITY_SENSOR 1
#define HAS_PRESSURE_SENSOR 1
#define HAS_LIGHT_CONTROL 1
#define HAS_SPRAY_CONTROL 1
#define HAS_FAN_CONTROL 1
#define MAX_SENSORS 8
#define MAX_ACTUATORS 6
"""
    elif device_type == "liquid":
        build_info_content += """
// Liquid device features  
#define HAS_PH_SENSOR 1
#define HAS_EC_SENSOR 1
#define HAS_WATER_TEMP_SENSOR 1
#define HAS_CHEMICAL_PUMPS 1
#define HAS_DOSING_CONTROL 1
#define HAS_CHEMICAL_SAFETY 1
#define MAX_SENSORS 6
#define MAX_ACTUATORS 12
"""
    
    build_info_content += """
// Build environment
#define BUILD_PLATFORM "PlatformIO"
#define BUILD_COMPILER_VERSION __VERSION__

// Configuration
#define CONFIG_VERSION 1
#define CONFIG_FILE_PATH "/config.json"

#endif // BUILD_INFO_H
"""
    
    # Write build info header
    os.makedirs("include", exist_ok=True)
    with open("include/build_info.h", "w") as f:
        f.write(build_info_content)
    
    print(f"✓ Generated build info for {device['name']}: {build_time} ({git_branch}:{git_hash})")

def validate_project_structure(source, target, env):
    """Validate project structure and configuration"""
    
    device_type = get_device_type()
    
    # Check for required source files
    required_core_files = [
        "src/main.cpp",
        "src/core/EventBus.cpp", 
        "src/core/Config.cpp",
        "src/core/BaseClasses.h",
        "src/core/Managers.cpp"
    ]
    
    device_specific_files = {
        "environmental": [
            "src/device/EnvironmentalDevice.cpp",
            "src/device/sensors/SHT3xSensor.cpp",
            "src/device/actuators/RelayActuator.cpp"
        ],
        "liquid": [
            "src/device/LiquidDevice.cpp", 
            "src/device/sensors/PHSensor.cpp",
            "src/device/actuators/PeristalticPump.cpp"
        ]
    }
    
    missing_files = []
    
    # Check core files
    for file_path in required_core_files:
        if not os.path.exists(file_path):
            missing_files.append(file_path)
    
    # Check device-specific files
    if device_type in device_specific_files:
        for file_path in device_specific_files[device_type]:
            if not os.path.exists(file_path):
                missing_files.append(file_path)
    
    if missing_files:
        print("WARNING: Missing required files:")
        for file_path in missing_files:
            print(f"  - {file_path}")
    else:
        print("✓ All required source files found")
    
    # Validate partition table
    partition_file = f"partitions_{device_type}.csv"
    if os.path.exists(partition_file):
        print(f"✓ Device-specific partition table found: {partition_file}")
    elif os.path.exists("partitions.csv"):
        print("✓ Generic partition table found")
    else:
        print("WARNING: No partition table found")
    
    # Validate configuration
    if os.path.exists("device_config.json"):
        try:
            with open("device_config.json", "r") as f:
                config = json.load(f)
                if config.get("device_type") == device_type:
                    print("✓ Device configuration matches build target")
                else:
                    print(f"WARNING: Device config type mismatch: {config.get('device_type')} != {device_type}")
        except Exception as e:
            print(f"WARNING: Could not validate device config: {e}")
    
    print(f"✓ Project validation complete for {device_type} device")

def create_version_file(source, target, env):
    """Create version information file for runtime access"""
    
    device_type = get_device_type()
    
    version_info = {
        "firmware_version": "1.0.0",
        "build_time": datetime.now().isoformat(),
        "device_type": device_type,
        "build_type": env.get('BUILD_TYPE', 'debug'),
        "git_hash": "unknown",
        "git_branch": "unknown"
    }
    
    try:
        import subprocess
        version_info["git_hash"] = subprocess.check_output(['git', 'rev-parse', '--short', 'HEAD']).decode('ascii').strip()
        version_info["git_branch"] = subprocess.check_output(['git', 'rev-parse', '--abbrev-ref', 'HEAD']).decode('ascii').strip()
    except:
        pass
    
    # Create data directory if it doesn't exist
    os.makedirs("data", exist_ok=True)
    
    # Write version file
    with open("data/version.json", "w") as f:         
        json.dump(version_info, f, indent=2)
    
    print(f"✓ Created version file: data/version.json")

def generate_config_template(source, target, env):
    """Generate device-specific configuration template"""
    
    device_type = get_device_type()
    
    templates = {
        "environmental": {
            "device": {
                "type": "environmental",
                "name": "AeroEnv Controller",
                "version": "1.0.0"
            },
            "sensors": [
                {
                    "name": "sht3x",
                    "type": "SHT3x", 
                    "i2c_address": "0x44",
                    "enabled": True
                },
                {
                    "name": "pressure",
                    "type": "AnalogPressure",
                    "pin": 36,
                    "enabled": True
                }
            ],
            "actuators": [
                {
                    "name": "lights",
                    "type": "Relay",
                    "pin": 23,
                    "enabled": True
                },
                {
                    "name": "spray",
                    "type": "VenturiNozzle", 
                    "pin": 22,
                    "enabled": True,
                    "pulse_width_ms": 5000
                }
            ]
        },
        "liquid": {
            "device": {
                "type": "liquid",
                "name": "AeroLiquid Controller",
                "version": "1.0.0"
            },
            "sensors": [
                {
                    "name": "ph_sensor",
                    "type": "AnalogPH",
                    "pin": 36,
                    "enabled": True
                },
                {
                    "name": "ec_sensor", 
                    "type": "AnalogEC",
                    "pin": 39,
                    "enabled": True
                }
            ],
            "actuators": [
                {
                    "name": "ph_up_pump",
                    "type": "PeristalticPump",
                    "pin": 23,
                    "enabled": True
                },
                {
                    "name": "nutrient_pump",
                    "type": "PeristalticPump", 
                    "pin": 22,
                    "enabled": True
                }
            ]
        }
    }
    
    if device_type in templates:
        os.makedirs("data", exist_ok=True)
        
        config_file = f"data/config_template_{device_type}.json"
        with open(config_file, "w") as f:
            json.dump(templates[device_type], f, indent=2)
        
        print(f"✓ Generated config template: {config_file}")

# Register build actions
env.AddPreAction("buildprog", generate_build_info)
env.AddPreAction("buildprog", validate_project_structure) 
env.AddPreAction("buildprog", create_version_file)
env.AddPreAction("buildprog", generate_config_template)

print(f"Build script loaded for {get_device_type()} device")

# Post-build actions for creating distribution packages
def create_distribution_package(source, target, env):
    """Create distribution package with firmware and documentation"""
    
    device_type = get_device_type()
    build_type = env.get('BUILD_TYPE', 'debug')
    
    # Create distribution directory
    dist_dir = f"dist/{device_type}_{build_type}"
    os.makedirs(dist_dir, exist_ok=True)
    
    # Copy firmware binary
    firmware_path = target[0].get_abspath()
    if os.path.exists(firmware_path):
        firmware_size = os.path.getsize(firmware_path)
        dist_firmware = f"{dist_dir}/firmware.bin"
        
        import shutil
        shutil.copy2(firmware_path, dist_firmware)
        print(f"✓ Firmware copied: {dist_firmware} ({firmware_size:,} bytes)")
    
    # Copy partition table
    partition_files = [f"partitions_{device_type}.csv", "partitions.csv"]
    for partition_file in partition_files:
        if os.path.exists(partition_file):
            shutil.copy2(partition_file, f"{dist_dir}/partitions.csv")
            print(f"✓ Partition table copied: {partition_file}")
            break
    
    # Copy configuration template
    config_template = f"data/config_template_{device_type}.json"
    if os.path.exists(config_template):
        shutil.copy2(config_template, f"{dist_dir}/default_config.json")
        print(f"✓ Configuration template copied")
    
    # Create installation instructions
    install_doc = f"""# {device_type.title()} Device Installation

## Files Included
- `firmware.bin`: Main firmware binary
- `partitions.csv`: Partition table
- `default_config.json`: Default configuration template

## Installation Steps

### 1. Install esptool
```bash
pip install esptool
```

### 2. Erase flash (first installation only)
```bash
esptool.py --chip esp32s3 --port /dev/ttyUSB0 erase_flash
```

### 3. Flash partition table
```bash  
esptool.py --chip esp32s3 --port /dev/ttyUSB0 write_flash 0x8000 partitions.csv
```

### 4. Flash firmware
```bash
esptool.py --chip esp32s3 --port /dev/ttyUSB0 write_flash 0x10000 firmware.bin
```

### 5. Monitor output
```bash
esptool.py --chip esp32s3 --port /dev/ttyUSB0 monitor
```

## Configuration
Device will create default configuration on first boot. Connect via serial at 115200 baud to monitor startup and configure WiFi settings.

## Support
Check serial output for diagnostic information and error messages.
"""
    
    with open(f"{dist_dir}/INSTALL.md", "w") as f:
        f.write(install_doc)
    
    print(f"✓ Distribution package created: {dist_dir}/")

# Register post-build action
env.AddPostAction("buildprog", create_distribution_package)