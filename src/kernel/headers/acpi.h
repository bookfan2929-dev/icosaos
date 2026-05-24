#ifndef ACPI_H
#define ACPI_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


// Standard text colors for your VGA output engine
#define ACPI_LOG_COLOR_FG 0x00AAAA // Cyan
#define ACPI_LOG_COLOR_BG 0x000000 // Black

// Every ACPI table starts with this exact header structure
typedef struct {
    char     signature[4];
    uint32_t length;
    uint8_t  revision;
    uint8_t  checksum;
    char     oem_id[6];
    char     oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__((packed)) acpi_header_t;

// Root System Description Pointer (ACPI 1.0 Specification)
typedef struct {
    char     signature[8]; // "_ RSD PTR "
    uint8_t  checksum;
    char     oem_id[6];
    uint8_t  revision;     // 0 for ACPI 1.0
    uint32_t rsdt_address; // 32-bit physical address of the RSDT
} __attribute__((packed)) acpi_rsdp_t;

// Root System Description Table
typedef struct {
    acpi_header_t header;
    uint32_t      table_pointers[]; // Flexible array of 32-bit physical pointers to other tables
} __attribute__((packed)) acpi_rsdt_t;

// Core functions
bool acpi_validate_checksum(const void* address, size_t length);
acpi_rsdp_t* acpi_find_rsdp(void);
void acpi_init(void);

// Bridged memory helpers pointing back to paging.c layout
extern int map_page(void *virtual_addr, void *physical_addr, uint32_t flags);
// Uncomment the line below if you implemented unmap_page in your paging.c
extern void unmap_page(void *virtual_addr);

#endif // ACPI_H
