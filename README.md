# EmbedKit_KamleshKshirsagar

## Author
Kamlesh Kshirsagar

## Description
UART Frame Parser implemented in C using a Finite State Machine (FSM).

Features:
- SOF detection (0xAA)
- CMD, LEN and Payload parsing
- XOR checksum verification
- Inter-byte timeout detection
- Automatic recovery after timeout
- Multiple test cases

## Build

gcc -Wall -std=c99 uart_parser.c -o uart_parser

## Run

./uart_parser

## Concepts Used
- Finite State Machine (FSM)
- UART Protocol Parsing
- XOR Checksum Validation
- Timeout Handling
- Error Recovery
