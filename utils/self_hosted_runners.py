import subprocess
import argparse
import sys


def start_vm(vm_name):
    try:
        subprocess.run(["C:\\Program Files\\Oracle\\VirtualBox\\VBoxManage.exe",
                       "startvm", vm_name, "--type", "headless"], check=True)
        print(f"VM '{vm_name}' started in headless mode.")
    except subprocess.CalledProcessError as e:
        print(f"Failed to start VM '{vm_name}': {e}")


def stop_vm(vm_name):
    try:
        # Attempt a graceful shutdown first
        subprocess.run(["C:\\Program Files\\Oracle\\VirtualBox\\VBoxManage.exe",
                       "controlvm", vm_name, "acpipowerbutton"], check=True)
        print(f"Attempting graceful shutdown of VM '{vm_name}'...")
    except subprocess.CalledProcessError as e:
        print(f"Failed to initiate graceful shutdown for VM '{vm_name}': {e}")
        try:
            # If graceful shutdown fails, force power off
            print(f"Forcefully powering off VM '{vm_name}'...")
            subprocess.run(["VBoxManage", "controlvm",
                           vm_name, "poweroff"], check=True)
            print(f"VM '{vm_name}' powered off.")
        except subprocess.CalledProcessError as e:
            print(f"Failed to forcefully power off VM '{vm_name}': {e}")


def run_powershell_command(command):
    try:
        subprocess.run(["powershell", "-Command", command], check=True)
        print(f"Command executed successfully: {command}")
    except subprocess.CalledProcessError as e:
        print(f"Command execution failed: {command}\nError: {e}")


def run_powershell_command_as_admin(command):
    try:
        # Combining the PowerShell command with Start-Process to run as admin
        ps_command = f"Start-Process powershell -ArgumentList '-Command {command}' -Verb RunAs"
        subprocess.run(["powershell", "-Command", ps_command], check=True)
        print(f"Attempted to execute command with admin privileges: {command}")
    except subprocess.CalledProcessError as e:
        print(
            f"Failed to execute command with admin privileges: {command}\nError: {e}")


def start_windows_server():
    # Set execution policy to RemoteSigned
    # run_powershell_command_as_admin("Set-ExecutionPolicy RemoteSigned")

    # Start the actions.runner.* services
    run_powershell_command_as_admin('Start-Service "actions.runner.*"')


def stop_windows_server():
    # Stop the actions.runner.* services
    run_powershell_command_as_admin('Stop-Service "actions.runner.*"')

    # Set execution policy back to Restricted
    # run_powershell_command_as_admin("Set-ExecutionPolicy Restricted")


VM_NAME = "Linux Main"


def main():
    parser = argparse.ArgumentParser(description="Manage self-hosted runners")
    parser.add_argument('--start', action='store_true',
                        help='Start self-hosted runners')
    parser.add_argument('--stop', action='store_true',
                        help='Stop self-hosted runner')

    args = parser.parse_args()

    if args.start:
        start_vm(VM_NAME)
        start_windows_server()
    elif args.stop:
        stop_windows_server()
        stop_vm(VM_NAME)
    else:
        print("No action specified. Use --start to runners or --stop to stop them.")
        sys.exit(1)


if __name__ == "__main__":
    main()
