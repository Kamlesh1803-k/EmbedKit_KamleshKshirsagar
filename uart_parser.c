/*
 * uart_parser.c
 *
 * UART Frame Parser using Finite State Machine (FSM)
 *
 * Features:
 * - SOF Detection (0xAA)
 * - XOR Checksum Validation
 * - Inter-byte Timeout Handling
 * - Automatic Recovery after Timeout
 *
 * Author: Kamlesh Kshirsagar
 */

#include <stdio.h>
#include <stdint.h>

#define SOF_BYTE       0xAA
#define MAX_PAYLOAD    16

typedef enum
{
    STATE_WAIT_SOF,
    STATE_WAIT_CMD,
    STATE_WAIT_LEN,
    STATE_WAIT_PAYLOAD,
    STATE_WAIT_CHECKSUM
} ParserState;

typedef struct
{
    ParserState state;

    uint8_t cmd;
    uint8_t len;
    uint8_t payload[MAX_PAYLOAD];

    uint8_t checksum;
    uint8_t payload_count;

    uint32_t timeout_ms;
    uint32_t last_timestamp;

    uint8_t frame_in_progress;

} UARTParser;

static void parser_reset(UARTParser *parser)
{
    parser->state = STATE_WAIT_SOF;
    parser->cmd = 0;
    parser->len = 0;
    parser->checksum = 0;
    parser->payload_count = 0;
    parser->frame_in_progress = 0;
}

static void parser_init(UARTParser *parser, uint32_t timeout_ms)
{
    parser->timeout_ms = timeout_ms;
    parser->last_timestamp = 0;
    parser_reset(parser);
}

static void update_checksum(UARTParser *parser, uint8_t data)
{
    parser->checksum ^= data;
}

static int parser_feed_byte(UARTParser *parser,
                            uint8_t byte,
                            uint32_t timestamp_ms)
{
    /* Check if communication timed out */
    if (parser->timeout_ms &&
        parser->frame_in_progress &&
        ((timestamp_ms - parser->last_timestamp) > parser->timeout_ms))
    {
        parser_reset(parser);
        return -2;
    }

    switch (parser->state)
    {
        case STATE_WAIT_SOF:

            if (byte == SOF_BYTE)
            {
                parser->state = STATE_WAIT_CMD;
                parser->checksum = 0;
                parser->payload_count = 0;
                parser->frame_in_progress = 1;
            }
            break;

        case STATE_WAIT_CMD:

            parser->cmd = byte;
            update_checksum(parser, byte);
            parser->state = STATE_WAIT_LEN;
            break;

        case STATE_WAIT_LEN:

            parser->len = byte;

            if (parser->len > MAX_PAYLOAD)
            {
                parser_reset(parser);
                return -1;
            }

            update_checksum(parser, byte);

            if (parser->len == 0)
            {
                parser->state = STATE_WAIT_CHECKSUM;
            }
            else
            {
                parser->state = STATE_WAIT_PAYLOAD;
            }

            break;

        case STATE_WAIT_PAYLOAD:

            parser->payload[parser->payload_count++] = byte;
            update_checksum(parser, byte);

            if (parser->payload_count >= parser->len)
            {
                parser->state = STATE_WAIT_CHECKSUM;
            }

            break;

        case STATE_WAIT_CHECKSUM:

            if (byte == parser->checksum)
            {
                printf("\nFrame Received Successfully\n");
                printf("CMD : 0x%02X\n", parser->cmd);
                printf("LEN : %u\n", parser->len);
                printf("PAYLOAD : ");

                for (uint8_t i = 0; i < parser->len; i++)
                {
                    printf("%02X ", parser->payload[i]);
                }

                printf("\n");

                parser_reset(parser);
                return 1;
            }
            else
            {
                parser_reset(parser);
                return -1;
            }

        default:
            break;
    }

    parser->last_timestamp = timestamp_ms;
    return 0;
}

static void feed_stream(UARTParser *parser,
                        const uint8_t bytes[],
                        const uint32_t times[],
                        uint32_t count)
{
    for (uint32_t i = 0; i < count; i++)
    {
        int result = parser_feed_byte(parser,
                                      bytes[i],
                                      times[i]);

        printf("t=%3ums byte=0x%02X -> ",
               times[i],
               bytes[i]);

        if (result == 1)
        {
            printf("Frame Complete\n");
        }
        else if (result == -1)
        {
            printf("Checksum Error\n");
        }
        else if (result == -2)
        {
            printf("Timeout\n");

            /* Start parsing again using current byte */
            parser_feed_byte(parser,
                             bytes[i],
                             times[i]);

            printf("t=%3ums byte=0x%02X -> receiving... (re-fed after reset)\n",
                   times[i],
                   bytes[i]);
        }
        else
        {
            printf("receiving...\n");
        }
    }
}

int main(void)
{
    UARTParser parser;

    printf("\n========== TEST 1 : VALID FRAME ==========\n");

    parser_init(&parser, 50);

    uint8_t test1[] =
    {
        0xAA, 0x01, 0x03, 0x10, 0x20, 0x30, 0x02
    };

    uint32_t time1[] =
    {
        0, 5, 10, 15, 20, 25, 30
    };

    feed_stream(&parser, test1, time1, sizeof(test1));

    printf("\n========== TEST 2 : TIMEOUT RECOVERY ==========\n");

    parser_init(&parser, 50);

    uint8_t test2[] =
    {
        0xAA, 0x01, 0x03, 0x10,
        0xAA, 0x05, 0x01, 0x7F, 0x7B
    };

    uint32_t time2[] =
    {
        0, 5, 10, 15,
        200, 205, 210, 215, 220
    };

    feed_stream(&parser, test2, time2, sizeof(test2));

    printf("\n========== TEST 3 : TWO VALID FRAMES ==========\n");

    parser_init(&parser, 50);

    uint8_t test3[] =
    {
        0xAA, 0x03, 0x01, 0x55, 0x57,
        0xAA, 0x04, 0x02, 0xAA, 0xBB, 0x17
    };

    uint32_t time3[] =
    {
        0, 5, 10, 15, 20,
        25, 30, 35, 40, 45, 50
    };

    feed_stream(&parser, test3, time3, sizeof(test3));

    printf("\n========== TEST 4 : TIMEOUT DISABLED ==========\n");

    parser_init(&parser, 0);

    uint8_t test4[] =
    {
        0xAA, 0x01, 0x03, 0x10,
        0xAA, 0x05, 0x01, 0x7F, 0x7B
    };

    uint32_t time4[] =
    {
        0, 5, 10, 15,
        200, 205, 210, 215, 220
    };

    feed_stream(&parser, test4, time4, sizeof(test4));

    return 0;
}