#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "main.h"

// Funktion zum Testen der interpret_data() Funktion
void test_interpret_data()
{
    // Testfall 1: data = 0x00
    uint8_t data1 = 0x00;
    char *result1 = interpret_data(data1);
    assert(strcmp(result1, "No information") == 0);

    // Testfall 2: data = 0x08
    uint8_t data2 = 0x08;
    char *result2 = interpret_data(data2);
    assert(strcmp(result2, "Start to move up") == 0);

    // Testfall 3: data = 0x10
    uint8_t data3 = 0x10;
    char *result3 = interpret_data(data3);
    assert(strcmp(result3, "Switching device switched off") == 0);

    // Weitere Testfälle hinzufügen...

    printf("interpret_data() tests passed!\n");
}

// Funktion zum Testen der get_key_data() Funktion
void test_get_key_data()
{
    // Testfall 1: command = "up"
    const char *command1 = "up";
    uint8_t result1 = get_key_data(command1);
    assert(result1 == 0x20);

    // Testfall 2: command = "down"
    const char *command2 = "down";
    uint8_t result2 = get_key_data(command2);
    assert(result2 == 0x40);

    // Testfall 3: command = "stop"
    const char *command3 = "stop";
    uint8_t result3 = get_key_data(command3);
    assert(result3 == 0x10);

    // Weitere Testfälle hinzufügen...

    printf("get_key_data() tests passed!\n");
}

// Funktion zum Testen der is_learned_channel() Funktion
void test_is_learned_channel()
{
    // Testfall 1: channels = 0x0001, channel = 1
    uint16_t channels1 = 0x0001;
    int channel1 = 1;
    int result1 = is_learned_channel(channels1, channel1);
    assert(result1 == 1);

    // Testfall 2: channels = 0x0001, channel = 2
    uint16_t channels2 = 0x0001;
    int channel2 = 2;
    int result2 = is_learned_channel(channels2, channel2);
    assert(result2 == 0);

    // Weitere Testfälle hinzufügen...

    printf("is_learned_channel() tests passed!\n");
}

// Funktion zum Testen der calculate_checksum() Funktion
void test_calculate_checksum()
{
    // Testfall 1: message = {0xAA, 0x02, 0x4A}, length = 3
    uint8_t message1[] = {0xAA, 0x02, 0x4A};
    size_t length1 = 3;
    uint8_t result1 = calculate_checksum(message1, length1);
    assert(result1 == 0x0A);

    // Testfall 2: message = {0xAA, 0x05, 0x4C, 0x00, 0x00, 0x24}, length = 6
    uint8_t message2[] = {0xAA, 0x05, 0x4C, 0x00, 0x00, 0x24};
    size_t length2 = 6;
    uint8_t result2 = calculate_checksum(message2, length2);
    assert(result2 == 0xE1);

    printf("calculate_checksum() tests passed!\n");
}

// Funktion zum Testen der parse_args() Funktion
void test_parse_args()
{
    // Testfall 1: argc = 5, argv = {"program", "check", "-v", "-d", "ttyUSB1"}
    int argc1 = 5;
    char *argv1[] = {"program", "check", "-v", "-d", "ttyUSB1"};
    args_t result1 = parse_args(argc1, argv1);
    assert(strcmp(result1.scope, "check") == 0);
    assert(result1.verbose == 1);
    assert(strcmp(result1.device, "ttyUSB1") == 0);

    // Testfall 2: argc = 5, argv = {"program", "send", "1", "up", "-v"}
    int argc2 = 5;
    char *argv2[] = {"program", "send", "1", "up", "-v"};
    args_t result2 = parse_args(argc2, argv2);
    assert(strcmp(result2.scope, "send") == 0);
    assert(result2.channel == 1);
    assert(strcmp(result2.command, "up") == 0);
    assert(result2.verbose == 1);

    // Testfall 3: argc = 3, argv = {"program", "info", "2"}
    int argc3 = 3;
    char *argv3[] = {"program", "info", "2"};
    args_t result3 = parse_args(argc3, argv3);
    assert(strcmp(result3.scope, "info") == 0);
    assert(result3.channel == 2);

    // Weitere Testfälle hinzufügen...

    printf("parse_args() tests passed!\n");
}

int main()
{
    test_interpret_data();
    test_get_key_data();
    test_is_learned_channel();
    test_calculate_checksum();
    test_parse_args();

    return 0;
}
