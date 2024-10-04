#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <stdint.h> 
#include <ctype.h>

#define MAX_ATTEMPTS 10       // Maximum attempts to read a response
#define SLEEP_DURATION 100000 // Sleep duration between attempts (in microseconds)

// Function to interpret received data based on predefined codes
char *interpret_data(uint8_t data)
{
    switch (data)
    {
    case 0x00:
        return "No information";
    case 0x01:
        return "Top position stop";
    case 0x02:
        return "Bottom position stop";
    case 0x03:
        return "Intermediate position stop";
    case 0x04:
        return "Tilt/Ventilation position stop";
    case 0x05:
        return "Blocking";
    case 0x06:
        return "Overheating";
    case 0x07:
        return "Timeout";
    case 0x08:
        return "Start to move up";
    case 0x09:
        return "Start to move down";
    case 0x0A:
        return "Moving up";
    case 0x0B:
        return "Moving down";
    case 0x0D:
        return "Stopped in undefined position";
    case 0x0E:
        return "Top position stop, tilt position";
    case 0x0F:
        return "Bottom position stop, intermediate position";
    case 0x10:
        return "Switching device switched off";
    case 0x11:
        return "Switching device switched on";
    default:
        return "Unknown";
    }
}

// Structure to store command-line arguments and flags
typedef struct
{
    char *scope;   // Command scope (check, send, info)
    int channel;   // Channel number (1-16)
    char *command; // Command to send (up, down, stop, etc.)
    int verbose;   // Verbose flag
    char *device;  // Device name
} args_t;

// Function to map command string to corresponding key data
uint8_t get_key_data(const char *command)
{
    if (strcmp(command, "up") == 0)
        return 0x20;
    else if (strcmp(command, "down") == 0)
        return 0x40;
    else if (strcmp(command, "stop") == 0)
        return 0x10;
    else if (strcmp(command, "tilt") == 0)
        return 0x24;
    else if (strcmp(command, "intermediate") == 0)
        return 0x44;

    fprintf(stderr, "Invalid command: %s. Use up, down, stop, tilt, or intermediate.\n", command);
    exit(1);
}

// Function to parse command-line arguments
args_t parse_args(int argc, char *argv[])
{
    args_t args = {0};
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-v") == 0)
            args.verbose = 1;
        else if (strcmp(argv[i], "-d") == 0)
            args.device = argv[++i];
        else if (strcmp(argv[i], "check") == 0 || strcmp(argv[i], "send") == 0 || strcmp(argv[i], "info") == 0)
            args.scope = argv[i];
        else if (isdigit(argv[i][0]))
            args.channel = atoi(argv[i]);
        else
            args.command = argv[i];
    }

    // Validate required arguments
    if (!args.scope)
    {
        fprintf(stderr, "Scope is required (check, send, or info).\n");
        exit(1);
    }

    if (strcmp(args.scope, "send") == 0)
    {
        if (!args.channel)
        {
            fprintf(stderr, "Channel is required for send scope.\n");
            exit(1);
        }
        if (!args.command)
        {
            fprintf(stderr, "Command is required for send scope.\n");
            exit(1);
        }
    }

    if (strcmp(args.scope, "info") == 0 && !args.channel)
    {
        fprintf(stderr, "Channel is required for info scope.\n");
        exit(1);
    }

    return args;
}

// Function to calculate checksum for message integrity
uint8_t calculate_checksum(uint8_t *message, size_t length)
{
    uint8_t sum = 0;
    for (size_t i = 0; i < length; i++)
        sum += message[i];
    return -sum;
}

// Function to configure the serial port
int configure_serial_port(char *device)
{
    int fd = open(device, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd == -1)
    {
        perror("Failed to open  serial port");
        return -1;
    }

    struct termios tty = {0};
    if (tcgetattr(fd, &tty) != 0)
    {
        perror("Error getting terminal attributes");
        close(fd);
        return -1;
    }

    // Set baud rate to 38400
    cfsetospeed(&tty, B38400);
    cfsetispeed(&tty, B38400);

    // Configure terminal settings
    tty.c_cflag = CS8 | CREAD | CLOCAL; // 8 bits, enable receiver, ignore modem control lines
    tty.c_iflag = 0;                    // No input processing
    tty.c_oflag = 0;                    // No output processing
    tty.c_lflag = 0;                    // No local processing
    tty.c_cc[VTIME] = 1;                // Read timeout (tenths of seconds)
    tty.c_cc[VMIN] = 0;                 // Minimum number of characters to read

    if (tcsetattr(fd, TCSANOW, &tty) != 0)
    {
        perror("Error setting terminal attributes");
        close(fd);
        return -1;
    }

    return fd;
}

// Function to wait for a response from the device
void wait_for_response(int fd, uint8_t *response, int verbose)
{
    for (int attempts = 0; attempts < MAX_ATTEMPTS; attempts++)
    {
        ssize_t bytes_read = read(fd, response, 7);
        if (bytes_read > 0 && response[0] == 0xAA)
        {
            if (verbose)
            {
                printf("Raw response: ");
                for (size_t i = 0; i < 7; i++)
                    printf("%02X ", response[i]);
                printf("\n");
            }
            return;
        }
        usleep(SLEEP_DURATION);
    }

    fprintf(stderr, "Timeout while waiting for response.\n");
    exit(1);
}

// Function to send Easy_Check command and get the learned channels
uint16_t send_easy_check(int fd, int verbose)
{
    uint8_t message[] = {0xAA, 0x02, 0x4A, 0x00};
    message[3] = calculate_checksum(message, 3);

    write(fd, message, sizeof(message));
    if (verbose)
        printf("Easy_Check sent.\n");

    uint8_t response[7] = {0};
    wait_for_response(fd, response, verbose);

    if (response[2] == 0x4B)
    {
        uint16_t channels = (response[3] << 8) | response[4];
        return channels;
    }

    fprintf(stderr, "Invalid response received.\n");
    exit(1);
}

// Function to check if a channel is learned
int is_learned_channel(uint16_t channels, int channel)
{
    return channels & (1 << (channel - 1));
}

// Function to send Easy_Send command to control a device
void send_easy_send(int fd, int channel, uint8_t key_data, int verbose)
{
    uint8_t message[7] = {0xAA, 0x05, 0x4C, 0x00, 0x00, key_data, 0x00};
    if (channel > 8)
        message[3] = (1 << (channel - 9));
    else
        message[4] = (1 << (channel - 1));

    message[6] = calculate_checksum(message, 6);

    write(fd, message, sizeof(message));

    if (verbose)
        printf("Easy_Send sent on channel %d with command 0x%02X.\n", channel, key_data);
}

// Function to handle Easy_Ack response and print interpreted data
void handle_easy_ack(uint8_t *response, int verbose)
{
    uint8_t data = response[5];

    if (verbose)
        printf("Data received: %02X (%s)\n", data, interpret_data(data));
    else
        printf("%s\n", interpret_data(data));
}

// Function to send Easy_Info command and process the response
void send_easy_info(int fd, int channel, int verbose)
{
    // Ensure the channel is learned
    uint16_t channels = send_easy_check(fd, verbose);
    if (!is_learned_channel(channels, channel))
    {
        fprintf(stderr, "Channel %d is not learned.\n", channel);
        return;
    }

    uint8_t message[6] = {0xAA, 0x04, 0x4E, 0x00, 0x00, 0x00};
    if (channel > 8)
        message[3] = (1 << (channel - 9));
    else
        message[4] = (1 << (channel - 1));

    message[5] = calculate_checksum(message, 5);

    write(fd, message, sizeof(message));

    if (verbose)
        printf("Easy_Info sent on channel %d.\n", channel);

    uint8_t response[7] = {0};
    wait_for_response(fd, response, verbose);

    if (response[2] == 0x4D)
    {
        handle_easy_ack(response, verbose);
    }
    else if (response[2] == 0x4B)
    {
        // If we receive Easy_Confirm, retry sending Easy_Info
        channels = (response[3] << 8) | response[4];
        if (is_learned_channel(channels, channel))
            send_easy_info(fd, channel, verbose);
        else
            fprintf(stderr, "Channel %d is not learned.\n", channel);
    }
    else
    {
        fprintf(stderr, "Invalid response received.\n");
    }
}

// Main function to handle user input and execute corresponding actions
int main(int argc, char *argv[])
{
    args_t args = parse_args(argc, argv);
    args.device = args.device ? args.device : "/dev/ttyUSB0";

    int fd = configure_serial_port(args.device);
    if (fd == -1)
        return 1;

    if (strcmp(args.scope, "check") == 0)
    {
        uint16_t channels = send_easy_check(fd, args.verbose);

        // List learned channels
        for (int i = 0; i < 16; i++)
        {
            if (channels & (1 << i))
                printf("[+] Channel %d is learned.\n", i + 1);
            else
                printf("[-] Channel %d is not learned.\n", i + 1);
        }
    }
    else if (strcmp(args.scope, "send") == 0)
    {
        uint8_t key_data = get_key_data(args.command);
        send_easy_send(fd, args.channel, key_data, args.verbose);
    }
    else if (strcmp(args.scope, "info") == 0)
    {
        send_easy_info(fd, args.channel, args.verbose);
    }
    else
    {
        fprintf(stderr, "Invalid scope.\n");
        close(fd);
        return 1;
    }

    close(fd);
    return 0;
}
