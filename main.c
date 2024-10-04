#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <stdint.h>
#include <ctype.h>
#include "main.h"

// Hauptfunktion zur Verarbeitung von Benutzereingaben und Ausführung entsprechender Aktionen
int main(int argc, char *argv[])
{
    // Argumente parsen
    args_t args = parse_args(argc, argv);
    args.device = args.device ? args.device : "/dev/ttyUSB0";

    // Serielle Schnittstelle konfigurieren
    int fd = configure_serial_port(args.device);
    if (fd == -1)
    {
        fprintf(stderr, "Fehler beim Konfigurieren der seriellen Schnittstelle.\n");
        return 1;
    }

    // Aktionen basierend auf dem scope-Argument ausführen
    if (strcmp(args.scope, "check") == 0)
    {
        uint16_t channels = send_easy_check(fd, args.verbose);

        // Gelernte Kanäle auflisten
        for (int i = 0; i < 16; i++)
        {
            if (channels & (1 << i))
                printf("[+] Kanal %d ist gelernt.\n", i + 1);
            else
                printf("[-] Kanal %d ist nicht gelernt.\n", i + 1);
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
        fprintf(stderr, "Ungültiger scope.\n");
        close(fd);
        return 1;
    }

    // Serielle Schnittstelle schließen
    close(fd);
    return 0;
}
