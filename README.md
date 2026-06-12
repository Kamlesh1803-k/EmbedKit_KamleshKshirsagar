# EmbedKit_KamleshKshirsagar

## Name
Kamlesh Kshirsagar

## Description
UART Frame Parser implemented in C using a state machine.

Features:
- SOF detection (0xAA)
- CMD parsing
- LEN parsing
- Payload handling
- XOR checksum verification
- Inter-byte timeout detection
- Automatic recovery after timeout

## Build

gcc -Wall -std=c99 uart_parser.c -o uart_parser

## Run

./uart_parser
