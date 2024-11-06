#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdint.h>
#include <ctype.h>

#define TRUE 1
#define FALSE 0
#define MAX_PROCESSES 4
#define TLB_SIZE 8

typedef struct {
    uint32_t vpn;
    uint32_t pfn;
    int valid;
    uint32_t pid;
    uint32_t timestamp;
} TLBEntry;

typedef struct {
    uint32_t pfn;
    int valid;
} PageTableEntry;

typedef struct {
    uint32_t r1;
    uint32_t r2;
} ProcessRegisters;

// Global variables
FILE* output_file;
char* strategy;
uint32_t* physical_memory;
TLBEntry tlb[TLB_SIZE];
PageTableEntry** page_tables;
ProcessRegisters process_registers[MAX_PROCESSES];
uint32_t current_pid;
uint32_t timestamp;

// Memory configuration
uint32_t offset_bits;
uint32_t pfn_bits;
uint32_t vpn_bits;
uint32_t page_size;
uint32_t num_frames;
uint32_t num_pages;
uint32_t fifo_next = 0;  
int defined = FALSE;

// Function prototypes
void handle_define(char** tokens);
void handle_instruction(char** tokens);
void handle_inspection(char** tokens);
uint32_t translate_address(uint32_t virtual_addr);
int is_number(const char* str);
uint32_t extract_vpn(uint32_t virtual_addr);
uint32_t extract_offset(uint32_t addr);
void initialize_memory(void);
void handle_ctxswitch(char** tokens);


char** tokenize_input(char* input) {
    char** tokens = NULL;
    char* token = strtok(input, " \n");
    int num_tokens = 0;

    while (token != NULL) {
        num_tokens++;
        tokens = realloc(tokens, num_tokens * sizeof(char*));
        tokens[num_tokens - 1] = malloc(strlen(token) + 1);
        strcpy(tokens[num_tokens - 1], token);
        token = strtok(NULL, " \n");
    }

    num_tokens++;
    tokens = realloc(tokens, num_tokens * sizeof(char*));
    tokens[num_tokens - 1] = NULL;

    return tokens;
}

void initialize_memory() {
    // Initialize physical memory
    page_size = 1 << offset_bits;
    num_frames = 1 << pfn_bits;
    num_pages = 1 << vpn_bits;
    physical_memory = calloc((1 << (offset_bits + pfn_bits)), sizeof(uint32_t));

    // Initialize page tables for all processes
    page_tables = malloc(MAX_PROCESSES * sizeof(PageTableEntry*));
    for (int i = 0; i < MAX_PROCESSES; i++) {
        page_tables[i] = malloc(num_pages * sizeof(PageTableEntry));
        for (uint32_t j = 0; j < num_pages; j++) {
            page_tables[i][j].valid = FALSE;
        }
    }

    // Initialize TLB
    for (int i = 0; i < TLB_SIZE; i++) {
        tlb[i].valid = FALSE;
    }

    // Initialize process registers
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_registers[i].r1 = 0;
        process_registers[i].r2 = 0;
    }

    current_pid = 0;
    timestamp = 0;
    defined = TRUE;
}

void handle_define(char** tokens) {
    if (defined) {
        fprintf(output_file, "Current PID: %u. Error: multiple calls to define in the same trace\n", current_pid);
        exit(1);
    }

    offset_bits = atoi(tokens[1]);
    pfn_bits = atoi(tokens[2]);
    vpn_bits = atoi(tokens[3]);

    initialize_memory();

    fprintf(output_file, "Current PID: %u. Memory instantiation complete. OFF bits: %u. PFN bits: %u. VPN bits: %u\n",
            current_pid, offset_bits, pfn_bits, vpn_bits);
}


int is_number(const char* str) {
    // Skip the '#' if present
    if (*str == '#') str++;
    
    // Check if empty string
    if (*str == '\0') return FALSE;
    
    // Check each character
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isdigit(str[i])) return FALSE;
    }
    
    return TRUE;
}

uint32_t extract_vpn(uint32_t virtual_addr) {
    return virtual_addr >> offset_bits;
}

uint32_t extract_offset(uint32_t addr) {
    return addr & ((1 << offset_bits) - 1);
}

void handle_inspection(char** tokens) {
    if (!defined) {
        fprintf(output_file, "Current PID: %u. Error: attempt to execute instruction before define\n", current_pid);
        exit(1);
    }

    if (strcmp(tokens[0], "pinspect") == 0) {
        uint32_t vpn = atoi(tokens[1]);
        PageTableEntry pte = page_tables[current_pid][vpn];
        fprintf(output_file, "Current PID: %u. Inspected page table entry %u. Physical frame number: %u. Valid: %d\n", 
                current_pid, vpn, pte.pfn, pte.valid);
    }
    else if (strcmp(tokens[0], "tinspect") == 0) {
        uint32_t tlb_entry = atoi(tokens[1]);
        if (tlb_entry < TLB_SIZE) {
            TLBEntry entry = tlb[tlb_entry];
            fprintf(output_file, "Current PID: %u. Inspected TLB entry %u. VPN: %u. PFN: %u. Valid: %d. PID: %u. Timestamp: %u\n",
                    current_pid, tlb_entry, entry.vpn, entry.pfn, entry.valid, entry.pid, entry.timestamp+1);
        }
        timestamp++;  // Increment after inspection
    }
    else if (strcmp(tokens[0], "linspect") == 0) {
        uint32_t physical_loc = atoi(tokens[1]);
        fprintf(output_file, "Current PID: %u. Inspected physical location %u. Value: %u\n",
                current_pid, physical_loc, physical_memory[physical_loc]);
        timestamp++;  // Increment after inspection
    }
    else if (strcmp(tokens[0], "rinspect") == 0) {
        if (strcmp(tokens[1], "r1") == 0) {
            fprintf(output_file, "Current PID: %u. Inspected register r1. Content: %u\n",
                    current_pid, process_registers[current_pid].r1);
        }
        else if (strcmp(tokens[1], "r2") == 0) {
            fprintf(output_file, "Current PID: %u. Inspected register r2. Content: %u\n",
                    current_pid, process_registers[current_pid].r2);
        }
        else {
            fprintf(output_file, "Current PID: %u. Error: invalid register operand %s\n", current_pid, tokens[1]);
            exit(1);
        }
        timestamp++;  // Increment after inspection
    }
}

void handle_ctxswitch(char** tokens) {
    if (!defined) {
        fprintf(output_file, "Current PID: %u. Error: attempt to execute instruction before define\n", current_pid);
        exit(1);
    }

    uint32_t new_pid = atoi(tokens[1]);
    
    if (new_pid >= MAX_PROCESSES) {
        fprintf(output_file, "Current PID: %u. Invalid context switch to process %u\n", current_pid, new_pid);
        exit(1);
    }

    current_pid = new_pid;
    timestamp++;  // Increment after context switch
    fprintf(output_file, "Current PID: %u. Switched execution context to process: %u\n", current_pid, current_pid);
}

void handle_instruction(char** tokens) {
    if (!defined) {
        fprintf(output_file, "Current PID: %u. Error: attempt to execute instruction before define\n", current_pid);
        exit(1);
    }

    if (strcmp(tokens[0], "load") == 0) {
        // Validate register
        if (strcmp(tokens[1], "r1") != 0 && strcmp(tokens[1], "r2") != 0) {
            fprintf(output_file, "Current PID: %u. Error: invalid register operand %s\n", current_pid, tokens[1]);
            exit(1);
        }

        uint32_t value;
        if (tokens[2][0] == '#') {  // Immediate value
            value = atoi(tokens[2] + 1);
            fprintf(output_file, "Current PID: %u. Loaded immediate %u into register %s\n",
                    current_pid, value, tokens[1]);
        }
        else {  // Memory location
            uint32_t virtual_addr = atoi(tokens[2]);
            timestamp++;  // Increment before translation
            uint32_t physical_addr = translate_address(virtual_addr);
            value = physical_memory[physical_addr];
            fprintf(output_file, "Current PID: %u. Loaded value of location %u (%u) into register %s\n",
                    current_pid, virtual_addr, value, tokens[1]);
        }

        // Store value in appropriate register
        if (strcmp(tokens[1], "r1") == 0) {
            process_registers[current_pid].r1 = value;
        } else {
            process_registers[current_pid].r2 = value;
        }
    }
    else if (strcmp(tokens[0], "store") == 0) {
        uint32_t virtual_addr = atoi(tokens[1]);
        timestamp++;  // Increment before translation
        uint32_t physical_addr = translate_address(virtual_addr);
        uint32_t value;

        if (tokens[2][0] == '#') {  // Immediate value
            value = atoi(tokens[2] + 1);
            fprintf(output_file, "Current PID: %u. Stored immediate %u into location %u\n",
                    current_pid, value, virtual_addr);
        }
        else {  // Register
            if (strcmp(tokens[2], "r1") == 0) {
                value = process_registers[current_pid].r1;
            }
            else if (strcmp(tokens[2], "r2") == 0) {
                value = process_registers[current_pid].r2;
            }
            else {
                fprintf(output_file, "Current PID: %u. Error: invalid register operand %s\n", current_pid, tokens[2]);
                exit(1);
            }
            fprintf(output_file, "Current PID: %u. Stored value of register %s (%u) into location %u\n",
                    current_pid, tokens[2], value, virtual_addr);
        }

        physical_memory[physical_addr] = value;
    }
    else if (strcmp(tokens[0], "add") == 0) {
        timestamp++;  // Increment for add operation
        uint32_t result = process_registers[current_pid].r1 + process_registers[current_pid].r2;
        fprintf(output_file, "Current PID: %u. Added contents of registers r1 (%u) and r2 (%u). Result: %u\n",
                current_pid, process_registers[current_pid].r1, process_registers[current_pid].r2, result);
        process_registers[current_pid].r1 = result;
    }
}


uint32_t translate_address(uint32_t virtual_addr) {
    uint32_t vpn = extract_vpn(virtual_addr);
    uint32_t offset = extract_offset(virtual_addr);
    uint32_t pfn;
    int tlb_hit = FALSE;
    // int tlb_index = -1;

    // Check TLB first
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].valid && tlb[i].vpn == vpn && tlb[i].pid == current_pid) {
            tlb_hit = TRUE;
            // tlb_index = i;
            pfn = tlb[i].pfn;

            // Update timestamp for LRU if needed
            if (strcmp(strategy, "LRU") == 0) {
                tlb[i].timestamp = timestamp;
            }

            fprintf(output_file, "Current PID: %u. Translating. Lookup for VPN %u hit in TLB entry %d. PFN is %u\n",
                    current_pid, vpn, i, pfn);
            break;
        }
    }

    if (!tlb_hit) {
        fprintf(output_file, "Current PID: %u. Translating. Lookup for VPN %u caused a TLB miss\n",
                current_pid, vpn);

        // Check page table
        PageTableEntry pte = page_tables[current_pid][vpn];
        if (!pte.valid) {
            fprintf(output_file, "Current PID: %u. Translating. Translation for VPN %u not found in page table\n",
                    current_pid, vpn);
            exit(1);
        }

        pfn = pte.pfn;

        // Find an empty TLB slot or apply replacement policy
        int insert_index = -1;
        
        // First check for empty slots
        for (int i = 0; i < TLB_SIZE; i++) {
            if (!tlb[i].valid) {
                insert_index = i;
                break;
            }
        }

        // If no empty slots, apply replacement policy
        if (insert_index == -1) {
            if (strcmp(strategy, "LRU") == 0) {
                // LRU replacement
                int lru_index = 0;
                for (int i = 1; i < TLB_SIZE; i++) {
                    if (tlb[i].timestamp < tlb[lru_index].timestamp) {
                        lru_index = i;
                    }
                }
                insert_index = lru_index;
            } else {
                // FIFO replacement
                insert_index = fifo_next;
                fifo_next = (fifo_next + 1) % TLB_SIZE;
            }
        }

        // Update the TLB entry
        tlb[insert_index].vpn = vpn;
        tlb[insert_index].pfn = pfn;
        tlb[insert_index].pid = current_pid;
        tlb[insert_index].valid = TRUE;
        tlb[insert_index].timestamp = timestamp;
    }

    return (pfn << offset_bits) | offset;
}

void map_page(uint32_t vpn, uint32_t pfn) {
    if (!defined) {
        fprintf(output_file, "Current PID: %u. Error: attempt to map page before define\n", current_pid);
        exit(1);
    }

    if (vpn >= num_pages || pfn >= num_frames) {
        fprintf(output_file, "Current PID: %u. Error: invalid VPN or PFN\n", current_pid);
        exit(1);
    }

    // Update page table entry
    page_tables[current_pid][vpn].pfn = pfn;
    page_tables[current_pid][vpn].valid = TRUE;

    // Find an empty TLB slot or apply the replacement policy
    int insert_index = -1;
    for (int i = 0; i < TLB_SIZE; i++) {
        if (!tlb[i].valid) {
            insert_index = i;
            break;
        }
    }

    // If no empty slot found, apply replacement policy
    if (insert_index == -1) {
        if (strcmp(strategy, "LRU") == 0) {
            int lru_index = 0;
            for (int i = 1; i < TLB_SIZE; i++) {
                if (tlb[i].timestamp < tlb[lru_index].timestamp) {
                    lru_index = i;
                }
            }
            insert_index = lru_index;
        } else {
            // FIFO replacement
            insert_index = fifo_next;
            fifo_next = (fifo_next + 1) % TLB_SIZE;
        }
    }

    timestamp++;  // Increment timestamp for map operation
    
    // Insert the VPN to PFN mapping in the TLB
    tlb[insert_index].vpn = vpn;
    tlb[insert_index].pfn = pfn;
    tlb[insert_index].pid = current_pid;
    tlb[insert_index].valid = TRUE;
    tlb[insert_index].timestamp = timestamp;

    fprintf(output_file, "Current PID: %u. Mapped virtual page number %u to physical frame number %u\n",
            current_pid, vpn, pfn);
}

void unmap_page(uint32_t vpn) {
    if (!defined) {
        fprintf(output_file, "Current PID: %u. Error: attempt to unmap page before define\n", current_pid);
        exit(1);
    }

    if (vpn >= num_pages) {
        fprintf(output_file, "Current PID: %u. Error: invalid VPN\n", current_pid);
        exit(1);
    }

    // Update page table entry
    page_tables[current_pid][vpn].valid = FALSE;
    page_tables[current_pid][vpn].pfn = 0;

    // Remove any existing TLB entries for this VPN/PID combination
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].valid && tlb[i].vpn == vpn && tlb[i].pid == current_pid) {
            tlb[i].valid = FALSE;
        }
    }

    fprintf(output_file, "Current PID: %u. Unmapped virtual page number %u\n",
            current_pid, vpn);
}

int main(int argc, char* argv[]) {
    const char usage[] = "Usage: memsym.out <strategy> <input trace> <output trace>\n";
    char* input_trace;
    char* output_trace;
    char buffer[1024];

    // Parse command line arguments
    if (argc != 4) {
        printf("%s", usage);
        return 1;
    }
    strategy = argv[1];
    input_trace = argv[2];
    output_trace = argv[3];

    // Open input and output files
    FILE* input_file = fopen(input_trace, "r");
    output_file = fopen(output_trace, "w");

    while (!feof(input_file)) {
        // Read input file line by line
        char* rez = fgets(buffer, sizeof(buffer), input_file);
        if (!rez) {
            break;
        }

        // Skip comment lines
        if (buffer[0] == '%' || buffer[0] == '\n') {
            continue;
        }

        // Remove endline character if present
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[len-1] = '\0';
        }

        char** tokens = tokenize_input(buffer);
        
        if (tokens[0] == NULL) {
            // Free tokens and continue if empty line
            free(tokens);
            continue;
        }

        if (strcmp(tokens[0], "define") == 0) {
            handle_define(tokens);
        }
        else if (strcmp(tokens[0], "ctxswitch") == 0) {
                 handle_ctxswitch(tokens);
        }
        else if (strcmp(tokens[0], "load") == 0 || 
                 strcmp(tokens[0], "store") == 0 || 
                 strcmp(tokens[0], "add") == 0) {
            handle_instruction(tokens);
        }
        else if (strcmp(tokens[0], "pinspect") == 0 || 
                 strcmp(tokens[0], "tinspect") == 0 || 
                 strcmp(tokens[0], "linspect") == 0 || 
                 strcmp(tokens[0], "rinspect") == 0) {
            handle_inspection(tokens);
        }
        else if(strcmp(tokens[0], "map") == 0){
            map_page(atoi(tokens[1]), atoi(tokens[2]));
        }
        else if(strcmp(tokens[0], "unmap") == 0){
            unmap_page(atoi(tokens[1]));
        }
       
        

        // Deallocate tokens
        for (int i = 0; tokens[i] != NULL; i++) {
            free(tokens[i]);
        }
        free(tokens);
    }

    // Cleanup
    free(physical_memory);
    for (int i = 0; i < MAX_PROCESSES; i++) {
        free(page_tables[i]);
    }
    free(page_tables);

    // Close files
    fclose(input_file);
    fclose(output_file);

    return 0;
}