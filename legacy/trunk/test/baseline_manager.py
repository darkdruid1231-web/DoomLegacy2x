#!/usr/bin/env python3
"""
Baseline management tool for Doom Legacy visual tests.
Allows updating baselines from current test screenshots.
"""

import os
import shutil
import argparse

def update_baselines(source_dir='.', baseline_dir='baselines'):
    """Update baselines by copying current screenshots to baselines directory."""
    screenshots = [f for f in os.listdir(source_dir) if f.startswith('screenshot_') and f.endswith('.png')]
    for screenshot in screenshots:
        src = os.path.join(source_dir, screenshot)
        dst = os.path.join(baseline_dir, screenshot)
        os.makedirs(baseline_dir, exist_ok=True)
        shutil.copy2(src, dst)
        print(f"Updated baseline: {dst}")

def list_baselines(baseline_dir='baselines'):
    """List current baselines."""
    if not os.path.exists(baseline_dir):
        print("No baselines directory found.")
        return
    for f in os.listdir(baseline_dir):
        print(f)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Manage visual test baselines.")
    parser.add_argument('action', choices=['update', 'list'], help="Action to perform.")
    args = parser.parse_args()

    if args.action == 'update':
        update_baselines()
    elif args.action == 'list':
        list_baselines()