#!/usr/bin/env python3
"""
Test script to validate the release workflow configuration.
This script checks that the workflow YAML files are valid and consistent.
"""

import yaml
import sys
import os

def load_workflow(filename):
    """Load and parse a workflow YAML file."""
    try:
        with open(filename, 'r') as f:
            # Use yaml.safe_load with version 1.2 to handle 'on' properly
            data = yaml.safe_load(f)
            # Handle the case where 'on' becomes True
            if True in data and 'on' not in data:
                data['on'] = data.pop(True)
            return data
    except Exception as e:
        print(f"Error loading {filename}: {e}")
        return None

def test_workflow_structure():
    """Test that both workflows have the expected structure."""
    ci_workflow = load_workflow('.github/workflows/ci.yml')
    release_workflow = load_workflow('.github/workflows/release.yml')
    
    if not ci_workflow or not release_workflow:
        return False
    
    # Check that both workflows have the expected keys
    expected_keys = ['name', 'on', 'jobs']
    for workflow, name in [(ci_workflow, 'CI'), (release_workflow, 'Release')]:
        for key in expected_keys:
            if key not in workflow:
                print(f"Missing key '{key}' in {name} workflow")
                return False
    
    print("✓ Both workflows have valid structure")
    return True

def test_platform_consistency():
    """Test that both workflows target the same platforms."""
    ci_workflow = load_workflow('.github/workflows/ci.yml')
    release_workflow = load_workflow('.github/workflows/release.yml')
    
    if not ci_workflow or not release_workflow:
        return False
    
    # Extract platforms from CI workflow
    ci_platforms = set()
    if 'jobs' in ci_workflow and 'build' in ci_workflow['jobs']:
        strategy = ci_workflow['jobs']['build'].get('strategy', {})
        matrix = strategy.get('matrix', {})
        ci_platforms = set(matrix.get('os', []))
    
    # Extract platforms from Release workflow  
    release_platforms = set()
    if 'jobs' in release_workflow and 'build-and-release' in release_workflow['jobs']:
        strategy = release_workflow['jobs']['build-and-release'].get('strategy', {})
        matrix = strategy.get('matrix', {})
        includes = matrix.get('include', [])
        release_platforms = set(item.get('os') for item in includes if 'os' in item)
    
    if ci_platforms != release_platforms:
        print(f"Platform mismatch: CI={ci_platforms}, Release={release_platforms}")
        return False
    
    print(f"✓ Both workflows target the same platforms: {ci_platforms}")
    return True

def test_artifact_paths():
    """Test that the artifact paths are consistent."""
    ci_workflow = load_workflow('.github/workflows/ci.yml')
    release_workflow = load_workflow('.github/workflows/release.yml')
    
    if not ci_workflow or not release_workflow:
        return False
    
    # Check Windows DLL path
    ci_jobs = ci_workflow.get('jobs', {})
    release_jobs = release_workflow.get('jobs', {})
    
    # Look for Windows artifact paths in CI
    ci_windows_path = None
    if 'build' in ci_jobs:
        steps = ci_jobs['build'].get('steps', [])
        for step in steps:
            if step.get('name') == 'Upload build artifacts (Windows)' and step.get('if') == "runner.os == 'Windows'":
                ci_windows_path = step.get('with', {}).get('path')
                break
    
    # Look for Windows artifact paths in Release
    release_windows_path = None
    if 'build-and-release' in release_jobs:
        steps = release_jobs['build-and-release'].get('steps', [])
        for step in steps:
            if step.get('name') == 'Upload Release Asset (Windows)' and step.get('if') == "runner.os == 'Windows'":
                release_windows_path = step.get('with', {}).get('files')
                break
    
    if ci_windows_path != release_windows_path:
        print(f"Windows artifact path mismatch: CI='{ci_windows_path}', Release='{release_windows_path}'")
        return False
    
    print(f"✓ Windows artifact paths match: {ci_windows_path}")
    
    # Check macOS dylib path
    ci_macos_path = None
    if 'build' in ci_jobs:
        steps = ci_jobs['build'].get('steps', [])
        for step in steps:
            if step.get('name') == 'Upload build artifacts (macOS)' and step.get('if') == "runner.os == 'macOS'":
                ci_macos_path = step.get('with', {}).get('path')
                break
    
    release_macos_path = None
    if 'build-and-release' in release_jobs:
        steps = release_jobs['build-and-release'].get('steps', [])
        for step in steps:
            if step.get('name') == 'Upload Release Asset (macOS)' and step.get('if') == "runner.os == 'macOS'":
                release_macos_path = step.get('with', {}).get('files')
                break
    
    if ci_macos_path != release_macos_path:
        print(f"macOS artifact path mismatch: CI='{ci_macos_path}', Release='{release_macos_path}'")
        return False
    
    print(f"✓ macOS artifact paths match: {ci_macos_path}")
    return True

def test_build_steps_consistency():
    """Test that build steps are consistent between workflows."""
    ci_workflow = load_workflow('.github/workflows/ci.yml')
    release_workflow = load_workflow('.github/workflows/release.yml')
    
    if not ci_workflow or not release_workflow:
        return False
    
    # Check that both use the same CMake version
    ci_cmake_version = None
    release_cmake_version = None
    
    ci_steps = ci_workflow.get('jobs', {}).get('build', {}).get('steps', [])
    for step in ci_steps:
        if step.get('name') == 'Setup CMake':
            ci_cmake_version = step.get('with', {}).get('cmake-version')
            break
    
    release_steps = release_workflow.get('jobs', {}).get('build-and-release', {}).get('steps', [])
    for step in release_steps:
        if step.get('name') == 'Setup CMake':
            release_cmake_version = step.get('with', {}).get('cmake-version')
            break
    
    if ci_cmake_version != release_cmake_version:
        print(f"CMake version mismatch: CI='{ci_cmake_version}', Release='{release_cmake_version}'")
        return False
    
    print(f"✓ CMake versions match: {ci_cmake_version}")
    
    # Check Python version consistency
    ci_python_version = None
    release_python_version = None
    
    for step in ci_steps:
        if step.get('name') == 'Setup Python (Windows)':
            ci_python_version = step.get('with', {}).get('python-version')
            break
    
    for step in release_steps:
        if step.get('name') == 'Setup Python (Windows)':
            release_python_version = step.get('with', {}).get('python-version')
            break
    
    if ci_python_version != release_python_version:
        print(f"Python version mismatch: CI='{ci_python_version}', Release='{release_python_version}'")
        return False
    
    print(f"✓ Python versions match: {ci_python_version}")
    return True

def main():
    """Run all tests."""
    print("Testing release workflow configuration...\n")
    
    tests = [
        test_workflow_structure,
        test_platform_consistency,
        test_artifact_paths,
        test_build_steps_consistency,
    ]
    
    all_passed = True
    for test in tests:
        if not test():
            all_passed = False
        print()
    
    if all_passed:
        print("✅ All tests passed! Release workflow appears to be properly configured.")
        return 0
    else:
        print("❌ Some tests failed. Please check the workflow configuration.")
        return 1

if __name__ == '__main__':
    sys.exit(main())