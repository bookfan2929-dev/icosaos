#include "headers/acpi.h"
#include "headers/handler.h"      // Gives us panic()
#include "headers/vga_graphics.h" // Gives us vga_print()
#include "headers/klog.h"

// Quick helper to write logging messages across our ACPI parsing routine
static void acpi_log(const char* message) {
    vga_print(message, ACPI_LOG_COLOR_FG, ACPI_LOG_COLOR_BG);
}

/**
 * @brief Validates the simple two's complement checksum used by ACPI tables.
 */
bool acpi_validate_checksum(const void* address, size_t length) {
    const uint8_t* byte_ptr = (const uint8_t*)address;
    uint8_t sum = 0;

    for (size_t i = 0; i < length; i++) {
        sum += byte_ptr[i];
    }

    return (sum == 0);
}

/**
 * @brief Scans physical memory ranges for the ACPI 1.0 RSDP signature.
 */
acpi_rsdp_t* acpi_find_rsdp(void) {
    uintptr_t bios_region_start = 0x000E0000;
    uintptr_t bios_region_end   = 0x000FFFFF;

    for (uintptr_t phys = bios_region_start; phys < bios_region_end; phys += 16) {
        char* candidate = (char*)phys;
        
        if (candidate[0] == '_' && candidate[1] == ' ' && candidate[2] == 'R' && candidate[3] == 'S' &&
            candidate[4] == 'D' && candidate[5] == ' ' && candidate[6] == 'P' && candidate[7] == 'T') {
            
            acpi_rsdp_t* rsdp = (acpi_rsdp_t*)phys;
            if (acpi_validate_checksum(rsdp, sizeof(acpi_rsdp_t))) {
                return rsdp;
            }
        }
    }

    return NULL;
}

/**
 * @brief Maps a range of pages identically using the kernel's native paging mechanisms.
 */
static void acpi_map_range(uint32_t base_phys, uint32_t length) {
    uint32_t page_size = 4096;
    uint32_t start_addr = base_phys & ~(page_size - 1);
    uint32_t end_addr = (base_phys + length + page_size - 1) & ~(page_size - 1);

    for (uint32_t addr = start_addr; addr < end_addr; addr += page_size) {
        // Flags: Present (0x1) | Write (0x2)
        map_page((void*)addr, (void*)addr, 0x1 | 0x2);
    }
}

/**
 * @brief Unmaps a range of pages identically.
 */
static void acpi_unmap_range(uint32_t base_phys, uint32_t length) {
    uint32_t page_size = 4096;
    uint32_t start_addr = base_phys & ~(page_size - 1);
    uint32_t end_addr = (base_phys + length + page_size - 1) & ~(page_size - 1);

    for (uint32_t addr = start_addr; addr < end_addr; addr += page_size) {
        unmap_page((void*)addr);
    }
}

/**
 * @brief Initializes the ACPI subsystem, parses the RSDT, and reveals headers.
 */
void acpi_init(void) {
    klog_info("ACPI: Searching for Root System Description Pointer...");

    acpi_rsdp_t* rsdp = acpi_find_rsdp();
    if (!rsdp) {
        //acpi_log("ACPI: Error - RSDP signature not found via manual BIOS scanning.\\n");
		klog_status(LOG_FAILED);
        return;
    }

    //acpi_log("ACPI: Found valid RSDP block structure.\\n");
    klog_status(LOG_SUCCESS);

    // Map the initial page containing the RSDT physical target address
    acpi_map_range(rsdp->rsdt_address, sizeof(acpi_header_t));
    acpi_rsdt_t* rsdt = (acpi_rsdt_t*)rsdp->rsdt_address;

    // Verify the fundamental signature label 
    if (rsdt->header.signature[0] != 'R' || rsdt->header.signature[1] != 'S' ||
        rsdt->header.signature[2] != 'D' || rsdt->header.signature[3] != 'T') {
        panic("ACPI: Found table index is not a valid RSDT block!");
    }

    // Now map the remainder of the table dynamically based on what the header claims
    uint32_t full_rsdt_length = rsdt->header.length;
    
    // Unmap the preview and map the full layout 
    acpi_unmap_range(rsdp->rsdt_address, sizeof(acpi_header_t));
    acpi_map_range(rsdp->rsdt_address, full_rsdt_length);

    // Validate the complete root table payload
    if (!acpi_validate_checksum(rsdt, rsdt->header.length)) {
        panic("ACPI: Root System Description Table checksum mismatch!");
    }

    // Each pointer entry within an ACPI 1.0 RSDT is 4 bytes wide
    size_t num_entries = (rsdt->header.length - sizeof(acpi_header_t)) / sizeof(uint32_t);
    acpi_log("ACPI: Successfully validated RSDT table indices.\\n");

    for (size_t i = 0; i < num_entries; i++) {
        uint32_t table_phys = rsdt->table_pointers[i];
        if (table_phys == 0) continue;

        // Map the target physical space so we can read its length and signature safely
        acpi_map_range(table_phys, sizeof(acpi_header_t));
        acpi_header_t* child_header = (acpi_header_t*)table_phys;
        uint32_t child_length = child_header->length;

        // Unmap the preview and map the full structure layout specified by the table
        acpi_unmap_range(table_phys, sizeof(acpi_header_t));
        acpi_map_range(table_phys, child_length);

        // Print table metadata out cleanly
        char sig_msg[32] = "ACPI: Discovered Table [....]\\n";
        sig_msg[24] = child_header->signature[0];
        sig_msg[25] = child_header->signature[1];
        sig_msg[26] = child_header->signature[2];
        sig_msg[27] = child_header->signature[3];
        acpi_log(sig_msg);

        // Parsing routing checks will happen right here:
        // if (child_header->signature[0] == 'A' && child_header->signature[1] == 'P' ...) {
        //     parse_madt(table_phys); // Inside here, you will copy data or extract addresses!
        // }

        // Clean cleanup: Unmap the child table now that we are done examining it
        acpi_unmap_range(table_phys, child_length);
    }

    // Finally, cleanly unmap the RSDT root table itself
    acpi_unmap_range(rsdp->rsdt_address, full_rsdt_length);
    acpi_log("ACPI: Subsystem parsing cycle complete. Tables unmapped.\\n");
}
