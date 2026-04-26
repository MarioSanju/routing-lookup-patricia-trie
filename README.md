# Route Lookup using Patricia Trie

Implementation of an IP route lookup algorithm in C using a Patricia Trie data structure.

## Overview

This project processes a routing table and performs efficient route lookups for IP addresses using longest prefix matching.

The implementation reads routing information from input files, builds a compressed binary trie, and returns the corresponding output interface for each queried IP address.

## Features

- IP route lookup using a Patricia Trie
- Longest prefix matching
- Routing table parsing from text files
- Query processing from input files
- Output interface resolution
- Implementation in C

## Tech Stack

- C
- Makefile
- File-based input/output

## Project Structure

- `my_route_lookup.c` → main route lookup implementation
- `io.c`, `io.h` → input/output handling
- `utils.c`, `utils.h` → utility functions
- `routing_table.txt` → routing table example
- `routing_table_simple.txt` → simplified routing table example
- `prueba*.txt` → input test files
- `Makefile` → build configuration

## How to run

Compile the project:

```bash
make
