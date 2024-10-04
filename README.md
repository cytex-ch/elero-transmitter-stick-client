
# Elero Transmitter Serial Program

This program facilitates communication with a Elero Transmitter device connected to a serial port. It allows you to check which channels are learned by the device, send control commands, and retrieve status information.

## Features

- **Check**: Verifies which channels are learned by the device.
- **Send**: Sends control commands such as "up", "down", "stop", etc.
- **Info**: Retrieves status information from the device.

## Prerequisites

- A Elero Transmitter device connected to a serial port.
- A POSIX-compliant system (Linux, macOS).

## Installation

### Compile the Program

Use `gcc` to compile the program:

```bash
gcc -o elero main.c
```

Ensure that the user has permissions to access the serial port (e.g., being part of the `dialout` group on Linux).

## Usage

### 1. Check Device Channels

The `check` scope will output the list of channels and whether they are learned by the device. Run the following command:

```bash
./elero check [-v] [-d <device>]
```

**Example Output**:

```bash
[+] Channel 1 is learned.
[-] Channel 2 is not learned.
[+] Channel 3 is learned.
```

Where:

- `-v` (optional) enables verbose mode for more detailed output.
- `-d <device>` (optional) specifies the serial device (default: `/dev/ttyUSB0`).

### 2. Send Commands to a Device

You can send control commands like "up", "down", "stop", etc., using the `send` scope.

```bash
./elero send <channel> <command> [-v | -d <device>]
```

Where:

- `channel`: The device channel (1-16).
- `command`: One of `up`, `down`, `stop`, `tilt`, or `intermediate`.
- `-v`: (Optional) Enables verbose mode.
- `-d <device>`: (Optional) Specifies the serial device (default: `/dev/ttyUSB0`).

**Example**:

```bash
./elero send 3 up 
```

This sends the "up" command to channel 3.

### 3. Retrieve Device Information

You can retrieve status information from the device using the `info` scope:

```bash
./elero info <channel> [-v] [-d <device>]
```

Where:

- `channel`: The device channel (1-16).
- `-v`: (Optional) Enables verbose mode.
- `-d <device>`: (Optional) Specifies the serial device (default: `/dev/ttyUSB0`).

**Example**:

```bash
./elero info 3
```

This will print information about the device's current status on the specified channel.

## Command Summary

| Command   | Description                                              |
|-----------|----------------------------------------------------------|
| `check`   | Verifies the learned channels of the device.              |
| `send`    | Sends a control command to the device (up, down, etc.).   |
| `info`    | Retrieves status information from the device.             |
| `-v`      | Enables verbose mode for detailed output.                 |

### Command-Line Arguments

- `check`: Verifies which channels are learned.
- `send <channel> <command>`: Sends a command to the specified channel.
- `info <channel>`: Retrieves information about the status of a specific channel.
- `-v`: Enables verbose mode, printing raw data and additional details.

## Exit Status

- `0`: Successful execution.
- `1`: Error occurred (e.g., invalid arguments, serial port configuration issues).

## Notes

- Ensure the serial port `/dev/ttyUSB0` is configured properly and accessible or specify a different device using the `-d` flag.
- The program attempts to read from the serial port up to 10 times, each time waiting 100 milliseconds before retrying. It will print an error message and exit if it fails to receive a response.
- The program supports up to 16 channels (1-16).
- The `send` scope will only work on learned channels, which you can check using the `check` command.

## Troubleshooting

1. **Permissions Issue**: If you get a permission error on `/dev/ttyUSB0`, ensure your user is in the `dialout` group (on Linux):

   ```bash
   sudo usermod -aG dialout $USER
   ```

   Then, log out and log back in to apply the changes.

2. **Timeouts**: If the program times out while waiting for a response, ensure the serial device is connected properly and is functioning.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
