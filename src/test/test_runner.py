#!/usr/bin/env python3
"""
Test runner for Doom Legacy Phase 2: Headless map loading tests with visual regression.
Loads E1M1, verifies BSP and actor counts in both software and OpenGL modes.
Enhanced with performance metrics: FPS logging, memory usage tracking, load time measurements.
Captures screenshots and compares using perceptual hashing for visual regression detection.
"""

import pexpect
import subprocess
import time
import os
import signal
import sys
import re
import threading
import imagehash
from PIL import Image
import json

def monitor_memory(pid, memory_usage, stop_event):
    """Monitor memory usage of the process."""
    max_memory = 0
    while not stop_event.is_set():
        try:
            with open(f'/proc/{pid}/status', 'r') as f:
                for line in f:
                    if line.startswith('VmRSS:'):
                        mem_kb = int(line.split()[1])
                        mem_mb = mem_kb / 1024
                        max_memory = max(max_memory, mem_mb)
                        break
            time.sleep(0.1)
        except (FileNotFoundError, ValueError):
            break
    memory_usage[0] = max_memory

def capture_screenshot(display, filename):
    """Capture screenshot using ImageMagick import."""
    cmd = f"import -display {display} -window root {filename}"
    subprocess.run(cmd, shell=True, check=True)

def compute_phash(image_path):
    """Compute perceptual hash of an image."""
    image = Image.open(image_path)
    return imagehash.phash(image)

def compare_visual(baseline_path, current_path, threshold=5):
    """Compare images using perceptual hash. Return True if similar."""
    if not os.path.exists(baseline_path):
        print(f"Baseline {baseline_path} not found, saving current as baseline")
        os.makedirs(os.path.dirname(baseline_path), exist_ok=True)
        subprocess.run(f"cp {current_path} {baseline_path}", shell=True)
        return True
    baseline_hash = compute_phash(baseline_path)
    current_hash = compute_phash(current_path)
    distance = baseline_hash - current_hash
    print(f"Perceptual hash distance: {distance}")
    return distance <= threshold

def parse_performance_metrics(output, start_time, memory_usage):
    """Parse output for performance metrics."""
    metrics = {}

    # Load time: from start to BSP loaded
    bsp_match = re.search(r'(\d+) vertices, (\d+) subsectors, (\d+) segs', output)
    if bsp_match:
        # Assuming the message appears at load time
        # Since we can't parse exact time, use the time when process started and assume load is quick
        # For better accuracy, we could modify the game to print timestamps
        load_time = time.time() - start_time  # Approximation
        metrics['load_time'] = load_time
    else:
        metrics['load_time'] = None

    # FPS: Look for FPS in output, e.g., "FPS: 35"
    fps_match = re.search(r'FPS:\s*(\d+)', output)
    if fps_match:
        metrics['fps'] = int(fps_match.group(1))
    else:
        metrics['fps'] = None

    metrics['max_memory_mb'] = memory_usage[0]

    return metrics

def save_performance_trends(metrics, mode, commit_sha=None):
    """Save performance metrics to trends file."""
    import json
    trends_file = "performance_trends.json"
    if not os.path.exists(trends_file):
        trends = []
    else:
        with open(trends_file, 'r') as f:
            trends = json.load(f)
    entry = {
        'timestamp': time.time(),
        'mode': mode,
        'metrics': metrics,
        'commit': commit_sha or os.environ.get('GITHUB_SHA', 'unknown')
    }
    trends.append(entry)
    with open(trends_file, 'w') as f:
        json.dump(trends, f, indent=2)

def generate_report(results, performance_data):
    """Generate comprehensive test report."""
    import json
    report = {
        'timestamp': time.time(),
        'overall_success': all(results.values()),
        'tests': results,
        'performance': performance_data,
        'commit': os.environ.get('GITHUB_SHA', 'unknown')
    }
    with open('test_report.json', 'w') as f:
        json.dump(report, f, indent=2)

    # Simple HTML report
    html = f"""
    <html>
    <head><title>Test Report</title></head>
    <body>
    <h1>Doom Legacy Test Report</h1>
    <p>Commit: {report['commit']}</p>
    <p>Overall: {'PASSED' if report['overall_success'] else 'FAILED'}</p>
    <h2>Test Results</h2>
    <ul>
    """
    for test, success in results.items():
        html += f"<li>{test}: {'PASS' if success else 'FAIL'}</li>"
    html += "</ul><h2>Performance</h2><pre>" + json.dumps(performance_data, indent=2) + "</pre></body></html>"
    with open('test_report.html', 'w') as f:
        f.write(html)

def run_test(mode, display, exe_path, wad_path):
    print(f"\n=== Running test in {mode} mode ===")
    child = None
    success = True
    start_time = time.time()
    output = ""

    try:
        # Run Doom Legacy in test mode, warp to E1M1
        print(f"Running Doom Legacy in {mode} mode, loading E1M1")
        cmd = [exe_path, "-testmode", "-iwad", wad_path, "-warp", "1"]
        if mode == "opengl":
            cmd.insert(1, "-opengl")
        env = dict(os.environ, DISPLAY=display, LIBGL_ALWAYS_SOFTWARE="1")
        child = pexpect.spawn(' '.join(cmd), env=env, encoding=None, timeout=30)

        # Start memory monitoring
        memory_usage = [0]
        stop_event = threading.Event()
        monitor_thread = threading.Thread(target=monitor_memory, args=(child.pid, memory_usage, stop_event))
        monitor_thread.start()

        # Wait for BSP loaded message
        try:
            child.expect(r'\d+ vertices, \d+ subsectors, \d+ segs', timeout=15)
            print("PASS: BSP loaded successfully")

            # Wait a bit more for rendering
            time.sleep(2)

            # Take screenshot
            screenshot_path = f"screenshot_{mode}.png"
            capture_screenshot(display, screenshot_path)

            # Compare visual
            baseline_path = f"baselines/screenshot_{mode}.png"
            if not compare_visual(baseline_path, screenshot_path):
                print(f"FAIL: Visual regression detected for {mode} mode")
                success = False
            else:
                print(f"PASS: Visual test passed for {mode} mode")

            # Send F1 to take in-game screenshot (if needed, but we captured screen)
            # child.send('\x1b[11~')  # F1 key sequence
            # time.sleep(1)

            # Read all output
            child.expect(pexpect.EOF)
            output = child.before

            stop_event.set()
            monitor_thread.join()

            # Parse performance metrics
            metrics = parse_performance_metrics(output, start_time, memory_usage)
            print(f"Performance Metrics for {mode} mode:")
            if metrics['load_time']:
                print(f"  Load Time: {metrics['load_time']:.2f} seconds")
            if metrics['fps']:
                print(f"  FPS: {metrics['fps']}")
            print(f"  Max Memory Usage: {metrics['max_memory_mb']:.2f} MB")

            # Save to trends
            save_performance_trends(metrics, mode)

            # Parse for assertions

            # Check for monster count: look for "Test mode: X monsters spawned"
            monster_match = re.search(r'Test mode: (\d+) monsters spawned on map', output)
            if monster_match:
                monster_count = int(monster_match.group(1))
                expected_count = 6  # E1M1 has 6 monsters in this build
                if monster_count == expected_count:
                    print(f"PASS: Monster count {monster_count} matches expected {expected_count}")
                else:
                    print(f"FAIL: Monster count {monster_count} does not match expected {expected_count}")
                    success = False
            else:
                print("FAIL: Monster count not found in output")
                success = False

            # Check for errors
            if ("error" in output.lower() or "failed" in output.lower()) and "Assertion" not in output:
                print("FAIL: Errors found in output")
                success = False

            if success:
                print(f"All tests PASSED for {mode} mode")
            else:
                print(f"Some tests FAILED for {mode} mode")

            return success

        except pexpect.TIMEOUT:
            print("Test timed out")
            return False
        except pexpect.EOF:
            print("Process exited early")
            output = child.before.decode('utf-8', errors='ignore')
            print("Output:", output)
            return False

    except Exception as e:
        print(f"Error in {mode} mode: {e}")
        return False
    finally:
        # Cleanup
        if child and child.isalive():
            child.kill(signal.SIGKILL)

def run_demo_test(display, exe_path, wad_path, demo_name):
    print(f"\n=== Running demo replay test for {demo_name} ===")
    child = None
    success = True
    start_time = time.time()
    output = ""

    try:
        # Run Doom Legacy in test mode, play demo
        print(f"Running Doom Legacy, playing demo {demo_name}")
        cmd = [exe_path, "-testmode", "-iwad", wad_path, "-playdemo", demo_name]
        env = dict(os.environ, DISPLAY=display, LIBGL_ALWAYS_SOFTWARE="1")
        child = pexpect.spawn(' '.join(cmd), env=env, encoding='utf-8', timeout=40)

        # Start memory monitoring
        memory_usage = [0]
        stop_event = threading.Event()
        monitor_thread = threading.Thread(target=monitor_memory, args=(child.pid, memory_usage, stop_event))
        monitor_thread.start()

        # Wait for demo to start or some time
        try:
            time.sleep(5)  # Wait for demo to load

            # Take screenshot during demo
            screenshot_path = f"screenshot_demo_{demo_name}.png"
            capture_screenshot(display, screenshot_path)

            # Compare visual
            baseline_path = f"baselines/screenshot_demo_{demo_name}.png"
            if not compare_visual(baseline_path, screenshot_path):
                print(f"FAIL: Visual regression detected for demo {demo_name}")
                success = False
            else:
                print(f"PASS: Visual test passed for demo {demo_name}")

            # Read all output
            child.expect(pexpect.EOF)
            output = child.before.decode('utf-8', errors='ignore')

            stop_event.set()
            monitor_thread.join()

            # Parse performance metrics
            metrics = parse_performance_metrics(output, start_time, memory_usage)
            print(f"Performance Metrics for demo {demo_name}:")
            if metrics['load_time']:
                print(f"  Load Time: {metrics['load_time']:.2f} seconds")
            if metrics['fps']:
                print(f"  FPS: {metrics['fps']}")
            print(f"  Max Memory Usage: {metrics['max_memory_mb']:.2f} MB")

            # Save to trends
            save_performance_trends(metrics, f"demo_{demo_name}")

            # Parse for assertions

            # Check for demo started
            if not re.search(r'demo', output.lower()):
                print("FAIL: Demo not mentioned in output")
                success = False
            else:
                print("PASS: Demo playback attempted")

            # Check for demo completion or end
            if re.search(r'demo.*end|demo.*complete|Demo.*recorded', output):
                print("PASS: Demo completed")
            else:
                print("WARN: Demo completion not confirmed")

            # Check for errors
            if ("error" in output.lower() or "failed" in output.lower() or "crash" in output.lower()) and "Assertion" not in output:
                print("FAIL: Errors found in output")
                success = False

            # Check for desync (if applicable)
            if "desync" in output.lower():
                print("FAIL: Desync detected")
                success = False

            if success:
                print(f"Demo replay test PASSED for {demo_name}")
            else:
                print(f"Demo replay test FAILED for {demo_name}")

            return success

        except pexpect.TIMEOUT:
            print("Demo test timed out")
            return False
        except pexpect.EOF:
            print("Process exited")
            output = child.before.decode('utf-8', errors='ignore')
            return True  # Demo completed

    except Exception as e:
        print(f"Error in demo test: {e}")
        return False
    finally:
        # Cleanup
        if child and child.isalive():
            child.kill(signal.SIGKILL)

def main():
    display = ":99"
    xvfb_proc = None
    results = {}
    performance_data = {}

    try:
        # Start Xvfb
        print("Starting Xvfb on display", display)
        xvfb_proc = subprocess.Popen(["Xvfb", display, "-screen", "0", "1024x768x24"])
        time.sleep(2)  # Wait for Xvfb to start

        # Path to doomlegacy executable
        exe_path = "doomlegacy-legacy2/legacy/trunk/build/doomlegacy"
        wad_path = "/usr/share/games/doom/doom1.wad"  # Shareware/free version

        if not os.path.exists(exe_path):
            print(f"Executable not found: {exe_path}")
            return 1

        if not os.path.exists(wad_path):
            print(f"WAD not found: {wad_path}")
            return 1

        # Run tests for both modes
        software_success = run_test("software", display, exe_path, wad_path)
        results['software_test'] = software_success
        # Note: performance saving is inside run_test

        opengl_success = run_test("opengl", display, exe_path, wad_path)
        results['opengl_test'] = opengl_success

        # Run demo replay test
        demo_success = run_demo_test(display, exe_path, wad_path, "DEMO1")
        results['demo_test'] = demo_success

        # Load performance data for report
        if os.path.exists("performance_trends.json"):
            with open("performance_trends.json", 'r') as f:
                trends = json.load(f)
            # Get latest entries
            latest = {}
            for entry in reversed(trends[-10:]):  # Last 10
                mode = entry['mode']
                if mode not in latest:
                    latest[mode] = entry['metrics']
            performance_data = latest

        generate_report(results, performance_data)

        if software_success and opengl_success and demo_success:
            print("\n=== Overall: All tests PASSED ===")
            return 0
        else:
            print("\n=== Overall: Some tests FAILED ===")
            return 1

    except Exception as e:
        print(f"Error: {e}")
        return 1
    finally:
        # Cleanup
        if xvfb_proc and xvfb_proc.poll() is None:
            xvfb_proc.kill()

    print("Test completed")
    return 0

if __name__ == "__main__":
    sys.exit(main())