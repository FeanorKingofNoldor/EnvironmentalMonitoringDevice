#!/usr/bin/env python3
"""
Pre-build script for AeroEnv ESP32
Generates build information and validates configuration
"""

Import("env")
import time
import os
from datetime import datetime

def generate_build_info(source, target, env):
    """Generate build information header"""
    
    build_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    build_timestamp = int(time.time())
    
    # Get git info if available
    git_hash = "unknown"
    git_branch = "unknown"
    try:
        import subprocess
        git_hash = subprocess.check_output(['git', 'rev-parse', '--short', 'HEAD']).decode('ascii').strip()
        git_branch = subprocess.check_output(['git', 'rev-parse', '--abbrev-ref', 'HEAD']).decode('ascii').strip()
    except:
        pass
    
    # Create build info header
    build_info_content = f"""
#ifndef BUILD_INFO_H
#define BUILD_INFO_H

#define BUILD_TIME "{build_time}"
#define BUILD_TIMESTAMP {build_timestamp}
#define BUILD_GIT_HASH "{git_hash}"
#define BUILD_GIT_BRANCH "{git_branch}"
#define FIRMWARE_VERSION "1.0.0"
#define DEVICE_TYPE "AeroEnv"

#endif // BUILD_INFO_H
"""
    
    # Write build info header
    os.makedirs("include", exist_ok=True)
    with open("include/build_info.h", "w") as f:
        f.write(build_info_content)
    
    print(f"✓ Generated build info: {build_time} ({git_branch}:{git_hash})")

def validate_configuration(source, target, env):
    """Validate project configuration"""
    
    # Check for required source files
    required_files = [
        "src/main.cpp",
        "src/core/EventBus.cpp",
        "src/core/Config.cpp"
    ]
    
    missing_files = []
    for file_path in required_files:
        if not os.path.exists(file_path):
            missing_files.append(file_path)
    
    if missing_files:
        print("WARNING: Required files missing:")
        for file_path in missing_files:
            print(f"  - {file_path}")
    else:
        print("✓ All required source files found")
    
    # Validate partition table
    if os.path.exists("custom_partitions.csv"):
        print("✓ Custom partition table found")
    else:
        print("WARNING: Custom partition table not found")
    
    print("✓ Configuration validation complete")

# Register pre-build actions
env.AddPreAction("buildprog", generate_build_info)
env.AddPreAction("buildprog", validate_configuration)

print("Pre-build script loaded for AeroEnv ESP32")