// Memory manager functions

//-----------------------------------------------------------------------------
// These functions manage dynamic memory allocation and access for different
// SRAM regions in a TM4C123GH6PM microcontroller-based RTOS project.
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "mm.h"

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Constants and Definitions
//-----------------------------------------------------------------------------
// Define block sizes and base addresses for memory regions.
// Each region is divided into subregions of specific sizes for dynamic allocation.

#define BLOCK_SIZE_512     512
#define BLOCK_SIZE_1024    1024
#define BLOCK_SIZE_1536    1536

// Base addresses for each memory region

#define REGION_4K0_BASE_ADDR    0x20000000
#define REGION_4K1_BASE_ADDR    0x20001000
#define REGION_8K1_BASE_ADDR    0x20002000
#define REGION_4K2_BASE_ADDR    0x20004000
#define REGION_4K3_BASE_ADDR    0x20005000
#define REGION_8K2_BASE_ADDR    0x20006000

#define TOTAL_REGIONS   40          // Total number of subregions available across all memory regions


//-----------------------------------------------------------------------------
// Memory Region Struct and Initialization
//-----------------------------------------------------------------------------
// Define a structure for memory regions and initialize them with base addresses and subregion sizes.

typedef struct {
    uint32_t baseAddress;    // Base address of the region
    uint32_t subregionSize;  // Size of each subregion (512B or 1024B)
} MemoryRegion;

// Define the array of regions with correct structure initialization
MemoryRegion regions[5] = {
    { 0x20001000, 512  },  // Region 0: 4 KB with 512B subregions
    { 0x20002000, 1024 },  // Region 1: 8 KB with 1024B subregions
    { 0x20004000, 512  },  // Region 2: 4 KB with 512B subregions
    { 0x20005000, 512  },  // Region 3: 4 KB with 512B subregions
    { 0x20006000, 1024 }   // Region 4: 8 KB with 1024B subregions
};

uint64_t* srdBitMask = 0x0000000000000000;

typedef struct
{
    void *address;                                              // Base of the allocation
    uint8_t subRegions;                                         // Number of subregions allocated here
} virtualdata;

//-----------------------------------------------------------------------------
// Allocation Management
//-----------------------------------------------------------------------------
// Structures and arrays to track allocated memory blocks.

uint8_t heapTop = 0;
uint8_t virtualdata_allotment[TOTAL_REGIONS] = {0, };      // A ledger to keep track allocated subregions, initialised to 0
virtualdata virtualdata_a[TOTAL_REGIONS] = {{0, 0}, };

#define BLOCK_4K1_START 0
#define BLOCK_4K1_END   7
#define BLOCK_8K1_START 8
#define BLOCK_8K1_END   15
#define BLOCK_4K2_START 16
#define BLOCK_4K2_END   23
#define BLOCK_4K3_START 24
#define BLOCK_4K3_END   31
#define BLOCK_8K2_START 32
#define BLOCK_8K2_END   39



void *allocation(uint8_t subRegions, uint8_t startRange, uint8_t endRange, uint32_t baseAddr, int16_t offset, uint16_t blockSize)
{
    uint8_t contiguousSpaces = 0, currentIndex;
    for (currentIndex = startRange; currentIndex <= endRange; currentIndex++)
    {
        if (!(virtualdata_allotment[currentIndex]))
        {
            contiguousSpaces++;
            if (contiguousSpaces >= subRegions)
            {
                int8_t i;
                for ( i = 0; i < subRegions; i++)
                {
                    virtualdata_allotment[currentIndex - i] = 1;
                }
                virtualdata_a[heapTop].subRegions = subRegions;
                virtualdata_a[heapTop].address = (void *)(baseAddr + ((currentIndex - subRegions  + offset) * blockSize));
                return (void *)virtualdata_a[heapTop++].address;
            }

        }
        else
        {
            contiguousSpaces = 0;
        }

    }
    return (void *)0;

}
    // Allocate memory based on requested size, rounding up to nearest block size.
    // Attempts allocation in different regions based on block availability.
    // Returns a pointer to allocated memory or NULL if allocation fails.
void * mallocFromHeap(uint32_t size_in_bytes)
{
    void *ptr;


    // Standardize the size based on the requested size
    if (size_in_bytes <= 512) {
        size_in_bytes = 512;  // Round up to 512 bytes
    } else if (size_in_bytes <= 1024) {
        size_in_bytes = 1024;  // Round up to 1024 bytes
    } else if (size_in_bytes <= 1536) {
        size_in_bytes = 1536;  // Set size to 1536 bytes
    } else
    {
        // If size is greater than 1536 bytes, round up to the next multiple of 1024
        size_in_bytes = ((size_in_bytes + 1023) / 1024) * 1024;
    }



    if (size_in_bytes == BLOCK_SIZE_512)
    {
        // Allocate a 512-byte block if available
        if ((size_in_bytes / BLOCK_SIZE_512) > 0)
        {
            ptr = allocation(((size_in_bytes / BLOCK_SIZE_512)), (BLOCK_4K1_START), (BLOCK_4K1_END - 1), REGION_4K1_BASE_ADDR, 1, BLOCK_SIZE_512);
            if (ptr != 0) return (void *)ptr;

            ptr = allocation(((size_in_bytes / BLOCK_SIZE_512)), (BLOCK_4K2_START + 1), BLOCK_4K2_END, REGION_4K2_BASE_ADDR, -15, BLOCK_SIZE_512);
            if (ptr != 0) return (void *)ptr;

            ptr = allocation(((size_in_bytes / BLOCK_SIZE_512)), (BLOCK_4K3_START), (BLOCK_4K3_END - 1), REGION_4K3_BASE_ADDR, -22, BLOCK_SIZE_512);
            if (ptr != 0) return (void *)ptr;
           
        }
        else
        {
            // No 512-byte block available, return NULL or handle error
        }
    }
    else if (size_in_bytes == BLOCK_SIZE_1024)
    {
        // Allocate a 1024-byte block if available
        if ((size_in_bytes / BLOCK_SIZE_512) >= 2) // Check if there are at least 2 consecutive 512B blocks
        {
            ptr = allocation((2), (BLOCK_4K1_START), (BLOCK_4K1_END - 1), REGION_4K1_BASE_ADDR, 1, BLOCK_SIZE_512);
            if (ptr != 0) return (void *)ptr;

            ptr = allocation((2), (BLOCK_4K2_START + 1), BLOCK_4K2_END, REGION_4K2_BASE_ADDR, -15, BLOCK_SIZE_512);
            if (ptr != 0) return (void *)ptr;

            ptr = allocation((2), (BLOCK_4K3_START), (BLOCK_4K3_END - 1), REGION_4K3_BASE_ADDR, -22, BLOCK_SIZE_512);
            if (ptr != 0) return (void *)ptr;
            // Code to allocate two consecutive 512-byte blocks
        }
        else if ((size_in_bytes / BLOCK_SIZE_1024) > 0)
        {
            ptr = allocation(((size_in_bytes / BLOCK_SIZE_1024)), (BLOCK_8K1_START + 1), (BLOCK_8K1_END - 1), REGION_8K1_BASE_ADDR, -7, BLOCK_SIZE_1024);
            if (ptr != 0) return (void *)ptr;

            ptr = allocation(((size_in_bytes / BLOCK_SIZE_1024)), (BLOCK_8K2_START + 1), (BLOCK_8K2_END), REGION_8K2_BASE_ADDR, -31, BLOCK_SIZE_1024);
            if (ptr != 0) return (void *)ptr;
           
        }

        else
        {
            // No 1024-byte or two consecutive 512-byte blocks available, return NULL or handle error
        }
    }
    else if (size_in_bytes == BLOCK_SIZE_1536)
    {
        // Allocate a 1536-byte block if available
        if(size_in_bytes == BLOCK_SIZE_1536)
        {
            ptr = allocation((2), (BLOCK_4K1_END), (BLOCK_8K1_START), REGION_4K1_BASE_ADDR, 1, BLOCK_SIZE_512);
            if (ptr != 0) return (void *)ptr;

            ptr = allocation((2), (BLOCK_8K1_END), (BLOCK_4K2_START), REGION_8K1_BASE_ADDR, -7, BLOCK_SIZE_1024);
            if (ptr != 0) return (void *)ptr;

            ptr = allocation((2), (BLOCK_4K3_END), (BLOCK_8K2_START), REGION_4K3_BASE_ADDR, -23, BLOCK_SIZE_512);
            if (ptr != 0) return (void *)ptr;
            
        }
        else if ((size_in_bytes / BLOCK_SIZE_1024) > 0 && (size_in_bytes / BLOCK_SIZE_512) > 0) // Check if there is one 1024B and one 512B block
        {
            ptr = allocation((2), (BLOCK_8K1_START + 1), (BLOCK_8K1_END - 1), REGION_8K1_BASE_ADDR, -7, BLOCK_SIZE_1024);
            if (ptr != 0) return (void *)ptr;

            ptr = allocation((2), (BLOCK_8K2_START + 1), (BLOCK_8K2_END), REGION_8K2_BASE_ADDR, -31, BLOCK_SIZE_1024);
            if (ptr != 0) return (void *)ptr;
           
        }
        else if ((size_in_bytes / BLOCK_SIZE_512) >= 3) // Check if there are at least 3 consecutive 512B blocks
        {
            ptr = allocation((3), (BLOCK_4K1_START), (BLOCK_4K1_END - 1), REGION_4K1_BASE_ADDR, 1, BLOCK_SIZE_512);
            if (ptr != 0) return (void *)ptr;

            ptr = allocation((3), (BLOCK_4K2_START + 1), BLOCK_4K2_END, REGION_4K2_BASE_ADDR, -15, BLOCK_SIZE_512);
            if (ptr != 0) return (void *)ptr;

            ptr = allocation((3), (BLOCK_4K3_START), (BLOCK_4K3_END - 1), REGION_4K3_BASE_ADDR, -22, BLOCK_SIZE_512);
            if (ptr != 0) return (void *)ptr;
         
        }

    }
    else if (size_in_bytes >= 1536)
    {
       // Allocate multiple 1024-byte blocks for sizes 4096 bytes or larger
       uint32_t required_blocks = size_in_bytes / BLOCK_SIZE_1024;

       ptr = allocation(required_blocks, BLOCK_8K1_START + 1, BLOCK_8K1_END - 1, REGION_8K1_BASE_ADDR, -7, BLOCK_SIZE_1024);
       if (ptr != 0) return ptr;

       ptr = allocation(required_blocks, BLOCK_8K2_START + 1, BLOCK_8K2_END, REGION_8K2_BASE_ADDR, -31, BLOCK_SIZE_1024);
       if (ptr != 0) return ptr;

       required_blocks = size_in_bytes / BLOCK_SIZE_512;

       ptr = allocation((required_blocks), (BLOCK_4K1_START), (BLOCK_4K1_END - 1), REGION_4K1_BASE_ADDR, 1, BLOCK_SIZE_512);
       if (ptr != 0) return (void *)ptr;

       ptr = allocation((required_blocks), (BLOCK_4K2_START + 1), BLOCK_4K2_END, REGION_4K2_BASE_ADDR, -15, BLOCK_SIZE_512);
       if (ptr != 0) return (void *)ptr;

       ptr = allocation((required_blocks), (BLOCK_4K3_START), (BLOCK_4K3_END - 1), REGION_4K3_BASE_ADDR, -22, BLOCK_SIZE_512);
       if (ptr != 0) return (void *)ptr;


    }



    return 0;
}

// Deallocates previously allocated memory and updates the allocation ledger.
void freeToHeap(void *pMemory)
{
    uint8_t i,index;
       // Identify the allocation entry based on the address (ptr)
       for ( i = 0; i < heapTop; i++)
       {
           if (virtualdata_a[i].address == pMemory)  // Check if the address matches
           {
               uint8_t startIndex = 0;
               uint8_t endIndex = 0;
               uint32_t address = (uint32_t)pMemory;

               // Calculate the start index based on the region and address
               if (address >= REGION_4K1_BASE_ADDR && address < REGION_8K1_BASE_ADDR)
               {
                   // Region 4K1: Calculate the start index based on 512-byte blocks
                   startIndex = (address - REGION_4K1_BASE_ADDR) / BLOCK_SIZE_512;
               }
               else if (address >= REGION_8K1_BASE_ADDR && address < REGION_4K2_BASE_ADDR)
               {
                   // Region 8K1: Calculate the start index based on 1024-byte blocks
                   startIndex = 8 + (address - REGION_8K1_BASE_ADDR) / BLOCK_SIZE_1024;
               }
               else if (address >= REGION_4K2_BASE_ADDR && address < REGION_4K3_BASE_ADDR)
               {
                   // Region 4K2: Calculate the start index based on 512-byte blocks
                   startIndex = 16 + (address - REGION_4K2_BASE_ADDR) / BLOCK_SIZE_512;
               }
               else if (address >= REGION_4K3_BASE_ADDR && address < REGION_8K2_BASE_ADDR)
               {
                   // Region 4K3: Calculate the start index based on 512-byte blocks
                   startIndex = 24 + (address - REGION_4K3_BASE_ADDR) / BLOCK_SIZE_512;
               }
               else if (address >= REGION_8K2_BASE_ADDR)
               {
                   // Region 8K2: Calculate the start index based on 1024-byte blocks
                   startIndex = 32 + (address - REGION_8K2_BASE_ADDR) / BLOCK_SIZE_1024;
               }


               // Calculate the end index using the number of subregions
               endIndex = startIndex + virtualdata_a[i].subRegions - 1;

               // Free the allocated regions by marking them as 0 in virtualdata_allotment
               for ( index = startIndex; index <= endIndex; index++)
               {
                   virtualdata_allotment[index] = 0;
               }

               // Remove the entry from virtualdata_a
               virtualdata_a[i].address = 0;  // Clear the address
               virtualdata_a[i].subRegions = 0;  // Clear the subregions count

               //Adjust heapTop if necessary
               if (i == heapTop - 1)  // If this was the last entry
               {
                   heapTop--;
               }

               return;  // Exit after successfully freeing
           }
       }
}


void BackgroundRules(void)
{
    NVIC_MPU_NUMBER_R = NVIC_MPU_NUMBER_S;               // Set region number

    NVIC_MPU_BASE_R = 0x00000000;

    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_XN;
    NVIC_MPU_ATTR_R |= 0x03000000;
    NVIC_MPU_ATTR_R |= 0x00000000;
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_SHAREABLE;
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_CACHEABLE;
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_BUFFRABLE;
    NVIC_MPU_ATTR_R |= (31 << 1);
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_ENABLE;

}
// REQUIRED: include your solution from the mini project
void allowFlashAccess(void)
{
    NVIC_MPU_NUMBER_R   = 0x00000001;           // Set Flash region number

    NVIC_MPU_BASE_R     = 0x00000000;                      // Flash address

    NVIC_MPU_ATTR_R     |= 0x03000000;              // Full memory access
    NVIC_MPU_ATTR_R     |= NVIC_MPU_ATTR_CACHEABLE;         // Cacheable
    NVIC_MPU_ATTR_R     |= (17 << 1);        // Apply rules to Flash 0x00000 to 0x3FFFF
    NVIC_MPU_ATTR_R     |= NVIC_MPU_ATTR_ENABLE;            // Enable region

}

void allowPeripheralAccess(void)
{
}

void setupSramAccess(void)
{
    NVIC_MPU_NUMBER_R   = 0x00000002;           // Set Flash region number

    NVIC_MPU_BASE_R     = 0x20000000;                      // Flash address
    NVIC_MPU_ATTR_R     = NVIC_MPU_ATTR_XN;
    NVIC_MPU_ATTR_R     |= 0x00000000;
    NVIC_MPU_ATTR_R     |= NVIC_MPU_ATTR_SHAREABLE;
    NVIC_MPU_ATTR_R     |= (0x01 << 24);              // Full memory access
    NVIC_MPU_ATTR_R     |= NVIC_MPU_ATTR_CACHEABLE;         // Cacheable
    NVIC_MPU_ATTR_R     |= (11 << 1);        // Apply rules to Flash 0x00000 to 0x3FFFF
    NVIC_MPU_ATTR_R     |= NVIC_MPU_ATTR_ENABLE;            // Enable region

    NVIC_MPU_NUMBER_R   = 0x00000003;           // Set Flash region number

    NVIC_MPU_BASE_R     = 0x20001000;                      // Flash address

    NVIC_MPU_ATTR_R     |= (0x01 << 24);              // Full memory access
    NVIC_MPU_ATTR_R     |= NVIC_MPU_ATTR_CACHEABLE;         // Cacheable
    NVIC_MPU_ATTR_R     |= (11 << 1);        // Apply rules to Flash 0x00000 to 0x3FFFF
    NVIC_MPU_ATTR_R     |= NVIC_MPU_ATTR_SRD_M;
    NVIC_MPU_ATTR_R     |= NVIC_MPU_ATTR_ENABLE;            // Enable region
    NVIC_MPU_ATTR_R     |= NVIC_MPU_ATTR_XN;
    NVIC_MPU_ATTR_R     |= 0x00000000;
    NVIC_MPU_ATTR_R     |= NVIC_MPU_ATTR_SHAREABLE;

    NVIC_MPU_NUMBER_R   = 0x00000004;           // Set Flash region number

    NVIC_MPU_BASE_R     = 0x20002000;                      // Flash address

    NVIC_MPU_ATTR_R     |= NVIC_MPU_ATTR_XN;
    NVIC_MPU_ATTR_R     |= 0x00000000;
    NVIC_MPU_ATTR_R     |= NVIC_MPU_ATTR_SHAREABLE;
    NVIC_MPU_ATTR_R     |= (0x01 << 24);              // Full memory access
    NVIC_MPU_ATTR_R     |= NVIC_MPU_ATTR_CACHEABLE;         // Cacheable
    NVIC_MPU_ATTR_R     |= (12 << 1);        // Apply rules to Flash 0x00000 to 0x3FFFF
    NVIC_MPU_ATTR_R     |= NVIC_MPU_ATTR_SRD_M;
    NVIC_MPU_ATTR_R     |= NVIC_MPU_ATTR_ENABLE;            // Enable region

    NVIC_MPU_NUMBER_R   = 0x00000005;           // Set Flash region number

    NVIC_MPU_BASE_R     = 0x20004000;                      // Flash address

    NVIC_MPU_ATTR_R     |= NVIC_MPU_ATTR_XN;
    NVIC_MPU_ATTR_R     |= 0x00000000;
    NVIC_MPU_ATTR_R     |= NVIC_MPU_ATTR_SHAREABLE;
    NVIC_MPU_ATTR_R     |= (0x01 << 24);              // Full memory access
    NVIC_MPU_ATTR_R     |= NVIC_MPU_ATTR_CACHEABLE;         // Cacheable
    NVIC_MPU_ATTR_R     |= (11 << 1);        // Apply rules to Flash 0x00000 to 0x3FFFF
    NVIC_MPU_ATTR_R     |= NVIC_MPU_ATTR_SRD_M;
    NVIC_MPU_ATTR_R     |= NVIC_MPU_ATTR_ENABLE;            // Enable region

    NVIC_MPU_NUMBER_R   = 0x00000006;           // Set Flash region number

    NVIC_MPU_BASE_R     = 0x20005000;                      // Flash address

    NVIC_MPU_ATTR_R     |= NVIC_MPU_ATTR_XN;
    NVIC_MPU_ATTR_R     |= 0x00000000;
    NVIC_MPU_ATTR_R     |= NVIC_MPU_ATTR_SHAREABLE;
    NVIC_MPU_ATTR_R     |= (0x01 << 24);              // Full memory access
    NVIC_MPU_ATTR_R     |= NVIC_MPU_ATTR_CACHEABLE;         // Cacheable
    NVIC_MPU_ATTR_R     |= (11 << 1);        // Apply rules to Flash 0x00000 to 0x3FFFF
    NVIC_MPU_ATTR_R     |= NVIC_MPU_ATTR_SRD_M;
    NVIC_MPU_ATTR_R     |= NVIC_MPU_ATTR_ENABLE;            // Enable region

    NVIC_MPU_NUMBER_R   = 0x00000007;           // Set Flash region number

    NVIC_MPU_BASE_R     = 0x20006000;                      // Flash address

    NVIC_MPU_ATTR_R     |= NVIC_MPU_ATTR_XN;
    NVIC_MPU_ATTR_R     |= 0x00000000;
    NVIC_MPU_ATTR_R     |= NVIC_MPU_ATTR_SHAREABLE;
    NVIC_MPU_ATTR_R     |= (0x01 << 24);              // Full memory access
    NVIC_MPU_ATTR_R     |= NVIC_MPU_ATTR_CACHEABLE;         // Cacheable
    NVIC_MPU_ATTR_R     |= (12 << 1);        // Apply rules to Flash 0x00000 to 0x3FFFF
    NVIC_MPU_ATTR_R     |= NVIC_MPU_ATTR_SRD_M;
    NVIC_MPU_ATTR_R     |= NVIC_MPU_ATTR_ENABLE;            // Enable region

}

uint64_t createNoSramAccessMask(void)
{
    return 0x0000000000000000;
}

void addSramAccessWindow(uint64_t *srdBitMask, uint32_t *baseAdd, uint32_t size_in_bytes)
{
    uint8_t i,j;
        uint32_t regionIndex;
        for (i = 0; i < 5; i++) {
                if ((uint32_t)baseAdd >= regions[i].baseAddress && (uint32_t)baseAdd < regions[i].baseAddress + (8 * regions[i].subregionSize)) {
                    regionIndex = i;
                    break;
                }
            }
        // Calculate the starting subregion offset for the specified base address in the identified region
            uint32_t offset = ((uint32_t)baseAdd - regions[regionIndex].baseAddress) / regions[regionIndex].subregionSize;

            // Calculate the number of subregions to enable based on the size
            uint32_t subregionCount = size_in_bytes / regions[regionIndex].subregionSize;

            for (j = 0; j < subregionCount; j++) {
                    // Calculate the bit position for the given subregion in the 64-bit srdBitMask
                    uint64_t bitPosition = regionIndex * 8 + offset + j;

                    // Clear the bit at `bitPosition` to enable access to that subregion
                    *srdBitMask |= (1 << bitPosition);
            }
}

void applySramAccessMask(uint64_t srdBitMask)
{
    uint8_t region;
    uint8_t subregionMask;
    for (region = 3; region < 7; region++)
    {
        // Iterate through each of the 5 SRAM MPU regions
            NVIC_MPU_NUMBER_R = region;  // Select the current MPU region

            // Calculate the 8 bits for the current region
           subregionMask = (srdBitMask >> ((region - 3) * 8)) & 0xFF;

            // Clear existing SRD bits and set new SRD bits for the current region
            NVIC_MPU_ATTR_R = (NVIC_MPU_ATTR_R & 0xFFFF00FF) | (subregionMask << 8);
    }

}

uint64_t setSramAccessWindow(uint32_t *baseAdd, uint32_t size_in_bytes)
{

    uint64_t srdBitMask = createNoSramAccessMask();
    addSramAccessWindow(&srdBitMask, baseAdd, size_in_bytes);
    //applySramAccessMask(srdBitMask);
    return srdBitMask;
}
void initFaultInterrupts()
{
    NVIC_SYS_HND_CTRL_R |= NVIC_SYS_HND_CTRL_BUS;     //enable bus faults
    NVIC_SYS_HND_CTRL_R |= NVIC_SYS_HND_CTRL_USAGE;    //ENABLE USAGE FAULTS
    NVIC_SYS_HND_CTRL_R |= NVIC_SYS_HND_CTRL_MEM;      //ENABLE MPU FAULTS
    NVIC_CFG_CTRL_R     |= NVIC_CFG_CTRL_DIV0;

}
