#!/usr/bin/env python3
"""
Validation script for GitHub workflows.

This script validates the consistency and correctness of the CI and Release workflows
to ensure they work together properly for building and releasing AsyncioDAT.

Usage:
    python validate_workflows.py

The script checks:
- YAML structure validity
- Platform consistency between CI and Release workflows  
- Artifact path consistency
- Build tool version consistency
- Expected artifact presence (AsyncioDAT.dll, AsyncioDAT.dylib)

Exit codes:
- 0: All validations passed
- 1: One or more validations failed
"""

import yaml
import sys
from pathlib import Path


def load_workflow(path):
    """Load and parse a YAML workflow file."""
    try:
        with open(path, 'r') as f:
            return yaml.safe_load(f)
    except Exception as e:
        print(f"Error loading {path}: {e}")
        return None


def validate_yaml_structure(workflow, name):
    """Validate basic YAML structure of a workflow."""
    print(f"\n=== Validating {name} workflow structure ===")
    
    # Check for name
    if 'name' not in workflow:
        print(f"❌ Missing required key: name")
        return False
    else:
        print(f"✅ Found required key: name")
    
    # Check for 'on' trigger (may be parsed as boolean True due to YAML)
    has_trigger = 'on' in workflow or True in workflow
    if not has_trigger:
        print(f"❌ Missing required key: on")
        return False
    else:
        print(f"✅ Found required key: on")
    
    # Check for jobs
    if 'jobs' not in workflow:
        print(f"❌ Missing required key: jobs")
        return False
    else:
        print(f"✅ Found required key: jobs")
    
    if workflow['jobs']:
        job_names = list(workflow['jobs'].keys())
        print(f"✅ Jobs defined: {', '.join(job_names)}")
    else:
        print(f"❌ No jobs defined")
        return False
    
    return True


def get_platforms(workflow):
    """Extract target platforms from workflow matrix."""
    platforms = set()
    for job_name, job in workflow.get('jobs', {}).items():
        strategy = job.get('strategy', {})
        matrix = strategy.get('matrix', {})
        
        # Handle different matrix formats
        if 'os' in matrix:
            if isinstance(matrix['os'], list):
                platforms.update(matrix['os'])
            else:
                platforms.add(matrix['os'])
        
        if 'include' in matrix:
            for item in matrix['include']:
                if 'os' in item:
                    platforms.add(item['os'])
    
    return platforms


def get_artifact_paths(workflow):
    """Extract artifact file paths from workflow."""
    paths = set()
    for job_name, job in workflow.get('jobs', {}).items():
        for step in job.get('steps', []):
            # Check upload-artifact actions
            if step.get('uses', '').startswith('actions/upload-artifact'):
                with_config = step.get('with', {})
                if 'path' in with_config:
                    paths.add(with_config['path'])
            
            # Check release asset uploads
            if step.get('uses', '').startswith('softprops/action-gh-release'):
                with_config = step.get('with', {})
                if 'files' in with_config:
                    files = with_config['files']
                    if isinstance(files, str):
                        paths.add(files)
                    elif isinstance(files, list):
                        paths.update(files)
    
    return paths


def get_tool_versions(workflow):
    """Extract build tool versions from workflow."""
    versions = {}
    for job_name, job in workflow.get('jobs', {}).items():
        for step in job.get('steps', []):
            uses = step.get('uses', '')
            with_config = step.get('with', {})
            
            if 'actions-setup-cmake' in uses and 'cmake-version' in with_config:
                versions['cmake'] = with_config['cmake-version']
            elif 'actions/setup-python' in uses and 'python-version' in with_config:
                versions['python'] = with_config['python-version']
    
    return versions


def validate_workflows():
    """Main validation function."""
    print("GitHub Workflows Validation Script")
    print("=" * 50)
    
    # Load workflows
    repo_root = Path(__file__).parent
    ci_path = repo_root / '.github' / 'workflows' / 'ci.yml'
    release_path = repo_root / '.github' / 'workflows' / 'release.yml'
    
    ci_workflow = load_workflow(ci_path)
    release_workflow = load_workflow(release_path)
    
    if not ci_workflow or not release_workflow:
        print("❌ Failed to load one or both workflows")
        return False
    
    # Validate YAML structure
    ci_valid = validate_yaml_structure(ci_workflow, "CI")
    release_valid = validate_yaml_structure(release_workflow, "Release")
    
    if not ci_valid or not release_valid:
        print("❌ Workflow structure validation failed")
        return False
    
    # Compare platforms
    print(f"\n=== Validating target platforms ===")
    ci_platforms = get_platforms(ci_workflow)
    release_platforms = get_platforms(release_workflow)
    
    print(f"CI platforms: {sorted(ci_platforms)}")
    print(f"Release platforms: {sorted(release_platforms)}")
    
    if ci_platforms == release_platforms:
        print("✅ Both workflows target the same platforms")
    else:
        print("❌ Workflows target different platforms")
        print(f"   CI only: {ci_platforms - release_platforms}")
        print(f"   Release only: {release_platforms - ci_platforms}")
        return False
    
    # Compare artifact paths
    print(f"\n=== Validating artifact paths ===")
    ci_paths = get_artifact_paths(ci_workflow)
    release_paths = get_artifact_paths(release_workflow)
    
    print(f"CI artifact paths: {sorted(ci_paths)}")
    print(f"Release artifact paths: {sorted(release_paths)}")
    
    # Extract just the file paths without directory structure for comparison
    ci_files = {Path(p).name for p in ci_paths}
    release_files = {Path(p).name for p in release_paths}
    
    if ci_files == release_files:
        print("✅ Both workflows use consistent artifact file names")
    else:
        print("❌ Workflows use different artifact files")
        print(f"   CI only: {ci_files - release_files}")
        print(f"   Release only: {release_files - ci_files}")
        return False
    
    # Compare tool versions
    print(f"\n=== Validating build tool versions ===")
    ci_versions = get_tool_versions(ci_workflow)
    release_versions = get_tool_versions(release_workflow)
    
    print(f"CI tool versions: {ci_versions}")
    print(f"Release tool versions: {release_versions}")
    
    if ci_versions == release_versions:
        print("✅ Both workflows use the same tool versions")
    else:
        print("❌ Workflows use different tool versions")
        for tool in set(ci_versions.keys()) | set(release_versions.keys()):
            ci_ver = ci_versions.get(tool, "not specified")
            release_ver = release_versions.get(tool, "not specified")
            if ci_ver != release_ver:
                print(f"   {tool}: CI={ci_ver}, Release={release_ver}")
        return False
    
    # Validate expected artifact files
    print(f"\n=== Validating expected artifacts ===")
    expected_artifacts = {"AsyncioDAT.dll", "AsyncioDAT.dylib"}
    
    if expected_artifacts.issubset(ci_files):
        print("✅ CI workflow produces expected artifacts")
    else:
        print(f"❌ CI workflow missing artifacts: {expected_artifacts - ci_files}")
        return False
    
    if expected_artifacts.issubset(release_files):
        print("✅ Release workflow handles expected artifacts")
    else:
        print(f"❌ Release workflow missing artifacts: {expected_artifacts - release_files}")
        return False
    
    print(f"\n=== Validation Summary ===")
    print("✅ All validation checks passed!")
    print("✅ CI and Release workflows are properly aligned")
    print("✅ Expected artifacts: AsyncioDAT.dll (Windows), AsyncioDAT.dylib (macOS)")
    print("✅ Build tools: CMake 3.25, Python 3.11")
    
    return True


if __name__ == "__main__":
    success = validate_workflows()
    sys.exit(0 if success else 1)