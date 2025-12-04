import urllib.request
import urllib.error
import json
import os
import sys

REPO = "hquip/frida"
TOKEN = os.environ.get("GITHUB_TOKEN")

if not TOKEN:
    print("Error: GITHUB_TOKEN environment variable not set.")
    sys.exit(1)

HEADERS = {
    "Authorization": f"Bearer {TOKEN}",
    "Accept": "application/vnd.github.v3+json",
    "User-Agent": "Python-Script"
}

def get_failed_runs():
    url = f"https://api.github.com/repos/{REPO}/actions/runs?status=failure&per_page=100"
    try:
        req = urllib.request.Request(url, headers=HEADERS)
        with urllib.request.urlopen(req) as response:
            data = json.loads(response.read().decode())
            return data.get("workflow_runs", [])
    except urllib.error.HTTPError as e:
        print(f"Error listing runs: {e}")
        print(e.read().decode())
        sys.exit(1)

def delete_run(run_id):
    url = f"https://api.github.com/repos/{REPO}/actions/runs/{run_id}"
    try:
        req = urllib.request.Request(url, headers=HEADERS, method="DELETE")
        with urllib.request.urlopen(req) as response:
            print(f"Deleted run {run_id}")
    except urllib.error.HTTPError as e:
        print(f"Error deleting run {run_id}: {e}")

def main():
    print(f"Fetching failed runs for {REPO}...")
    runs = get_failed_runs()
    print(f"Found {len(runs)} failed runs.")
    
    for run in runs:
        print(f"Deleting run {run['id']} ({run['name']})...")
        delete_run(run['id'])

if __name__ == "__main__":
    main()
