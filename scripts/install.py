import os
import subprocess
import sys
from pathlib import Path

import git

current_dir = os.getcwd()


def initialize_git_submodule(repo_path):
    """
    Initialize and update a git submodule in the given path using GitPython.
    """
    if not Path(repo_path).is_dir():
        print(f"Error: Directory '{repo_path}' does not exist.")
        sys.exit(1)

    print(f"Initializing submodules in {repo_path}...")

    # Initialize the repo object using GitPython
    try:
        repo = git.Repo(repo_path)
        output = repo.git.submodule("update", "--init", "--depth=1")
        print(f"Updating submodules: \n{output}")
    except git.exc.InvalidGitRepositoryError:
        sys.exit(1)


def checkout_submodules(script_path, platform_args):
    """
    Run the submodule checkout Python script for the given platform.
    """
    script_file = Path(script_path)
    if not script_file.is_file():
        print(f"Error: Script '{script_path}' not found.")
        sys.exit(1)

    print(
        f"Running submodule checkout script: {script_file} with arguments {platform_args}"
    )

    try:
        result = subprocess.run(
            ["python", str(script_file)] + platform_args,
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        print(result.stdout)
    except subprocess.CalledProcessError as e:
        print(f"Error running the submodule checkout script: {e.stderr}")
        sys.exit(e.returncode)


def main():
    """
    Main automation workflow.
    """
    # Step 1: Initialize main repository's submodules
    print("\nStep 1: Initializing main repository submodules...")
    initialize_git_submodule(current_dir)

    # Step 2: Initialize submodules in the esp-matter vendor folder
    esp_matter_sdk_path = os.path.join(current_dir, "vendor", "esp-matter")
    print(f"\nStep 2: Initializing submodules in {esp_matter_sdk_path}...")
    initialize_git_submodule(esp_matter_sdk_path)

    # Step 3: Checkout submodules for connectedhomeip
    connectedhomeip_path = os.path.join(
        esp_matter_sdk_path, "connectedhomeip", "connectedhomeip"
    )
    script_path = os.path.join(
        connectedhomeip_path, "scripts", "checkout_submodules.py"
    )
    platform_args = ["--platform", "esp32", "linux", "--shallow"]

    print(f"\nStep 3: Checking out submodules for {script_path}...")
    checkout_submodules(script_path, platform_args)

    print("\n✅ Installation complete!")


if __name__ == "__main__":
    main()
