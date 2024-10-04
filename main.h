#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <stdint.h>
#include <ctype.h>

// Struktur zur Speicherung von Befehlszeilenargumenten und Flags
typedef struct
{
    char *scope;   // Befehlsbereich (check, send, info)
    int channel;   // Kanalnummer (1-16)
    char *command; // Zu sendender Befehl (up, down, stop, etc.)
    int verbose;   // Verbose-Flag
    char *device;  // Gerätename
} args_t;

// Funktion zum Parsen der Befehlszeilenargumente
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

    // Validierung der erforderlichen Argumente
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

#define MAX_ATTEMPTS 10       // Maximale Versuche, um eine Antwort zu lesen
#define SLEEP_DURATION 100000 // Schlafdauer zwischen den Versuchen (in Mikrosekunden)

// Funktion zur Interpretation empfangener Daten basierend auf vordefinierten Codes
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

// Funktion zur Zuordnung des Befehlsstrings zu den entsprechenden Tastendaten
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

// Funktion zur Berechnung der Prüfsumme für die Nachrichtenintegrität
uint8_t calculate_checksum(uint8_t *message, size_t length)
{
    uint8_t sum = 0;
    for (size_t i = 0; i < length; i++)
        sum += message[i];
    return -sum;
}

// Funktion zur Konfiguration der seriellen Schnittstelle
int configure_serial_port(char *device)
{
    int fd = open(device, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd == -1)
    {
        perror("Failed to open serial port");
        return -1;
    }

    struct termios tty = {0};
    if (tcgetattr(fd, &tty) != 0)
    {
        perror("Error getting terminal attributes");
        close(fd);
        return -1;
    }

    // Baudrate auf 38400 setzen
    cfsetospeed(&tty, B38400);
    cfsetispeed(&tty, B38400);

    // Terminaleinstellungen konfigurieren
    tty.c_cflag = CS8 | CREAD | CLOCAL; // 8 Bits, Empfänger aktivieren, Modemsteuerleitungen ignorieren
    tty.c_iflag = 0;                    // Keine Eingabeverarbeitung
    tty.c_oflag = 0;                    // Keine Ausgabeverarbeitung
    tty.c_lflag = 0;                    // Keine lokale Verarbeitung
    tty.c_cc[VTIME] = 1;                // Lese-Timeout (Zehntelsekunden)
    tty.c_cc[VMIN] = 0;                 // Minimale Anzahl zu lesender Zeichen

    if (tcsetattr(fd, TCSANOW, &tty) != 0)
    {
        perror("Error setting terminal attributes");
        close(fd);
        return -1;
    }

    return fd;
}

// Funktion zum Warten auf eine Antwort vom Gerät
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

// Funktion zum Senden des Easy_Check-Befehls und Abrufen der gelernten Kanäle
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

// Funktion zum Überprüfen, ob ein Kanal gelernt ist
int is_learned_channel(uint16_t channels, int channel)
{
    return channels & (1 << (channel - 1));
}

// Funktion zum Senden des Easy_Send-Befehls zur Steuerung eines Geräts
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

// Funktion zur Verarbeitung der Easy_Ack-Antwort und Ausgabe der interpretierten Daten
void handle_easy_ack(uint8_t *response, int verbose)
{
    uint8_t data = response[5];

    if (verbose)
        printf("Data received: %02X (%s)\n", data, interpret_data(data));
    else
        printf("%s\n", interpret_data(data));
}

// Funktion zum Senden des Easy_Info-Befehls und Verarbeiten der Antwort
void send_easy_info(int fd, int channel, int verbose)
{
    // Sicherstellen, dass der Kanal gelernt ist
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
        // Wenn Easy_Confirm empfangen wird, Easy_Info erneut senden
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
