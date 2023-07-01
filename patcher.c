#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif

FILE *romfile;
FILE *outfile;
uint32_t romsize;
uint8_t rom[0x02000000];

/*
 * MOV R0, # 0x09000000
 * STRB R0, [R0]
 * LDR PC, [PC, #-4]
 * .word 0x080000C0
 */
static unsigned char thunk[] = { 0x09, 0x04, 0xA0, 0xE3, 0x00, 0x00, 0xC0, 0xE5, 0x04, 0xF0, 0x1F, 0xE5, 0xC0, 0x00, 0x00, 0x08};

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        puts("Wrong number of args. Try dragging and dropping your ROM onto the .exe file in the file browser.");
		scanf("%*s");
        return 1;
    }
	
	memset(rom, 0x00ff, sizeof rom);
    
    size_t romfilename_len = strlen(argv[1]);
    if (romfilename_len < 4 || strcasecmp(argv[1] + romfilename_len - 4, ".gba"))
    {
        puts("File does not have .gba extension.");
		scanf("%*s");
        return 1;
    }

    // Open ROM file
    if (!(romfile = fopen(argv[1], "rb")))
    {
        puts("Could not open input file");
        puts(strerror(errno));
		scanf("%*s");
        return 1;
    }

    // Load ROM into memory
    fseek(romfile, 0, SEEK_END);
    romsize = ftell(romfile);

    if (romsize > sizeof rom)
    {
        puts("ROM too large - not a GBA ROM?");
		scanf("%*s");
        return 1;
    }

    if (romsize & 0x3ffff)
    {
		puts("ROM has been trimmed and is misaligned. Padding to 256KB alignment");
		romsize &= ~0x3ffff;
		romsize += 0x40000;
    }

    fseek(romfile, 0, SEEK_SET);
    fread(rom, 1, romsize, romfile);

    // Find a location to insert the payload 
	int payload_base;
    for (payload_base = romsize - sizeof thunk; payload_base >= 0; payload_base -= 4)
    {
        int is_all_zeroes = 1;
        int is_all_ones = 1;
        for (int i = 0; i < sizeof thunk; ++i)
        {
            if (rom[payload_base+i] != 0)
            {
                is_all_zeroes = 0;
            }
            if (rom[payload_base+i] != 0xFF)
            {
                is_all_ones = 0;
            }
        }
        if (is_all_zeroes || is_all_ones)
        {
           break;
		}
    }
	if (payload_base < 0)
	{
		puts("ROM too small to install payload.");
		if (romsize + sizeof thunk > 0x2000000)
		{
			puts("ROM alraedy max size. Cannot expand. Cannot install payload");
            scanf("%*s");
			return 1;
		}
		else
		{
			puts("Expanding ROM");
			romsize += sizeof thunk;
			payload_base = romsize;
		}
	}
	
	printf("Installing payload at offset %x\n", payload_base);
	memcpy(rom + payload_base, thunk, sizeof thunk);
    
	if (rom[3] != 0xea)
	{
		puts("Unexpected entrypoint instruction");
		scanf("%*s");
		return 1;
	}
	unsigned long original_entrypoint_offset = rom[0];
	original_entrypoint_offset |= rom[1] << 8;
	original_entrypoint_offset |= rom[2] << 16;
	unsigned long original_entrypoint_address = 0x08000000 + 8 + (original_entrypoint_offset << 2);
	printf("Original offset was %lx, original entrypoint was %lx\n", original_entrypoint_offset, original_entrypoint_address);
	// little endian assumed, deal with it
    
	3[(uint32_t*) &rom[payload_base]] = original_entrypoint_address;

	unsigned long new_entrypoint_address = 0x08000000 + payload_base;
	0[(uint32_t*) rom] = 0xea000000 | (new_entrypoint_address - 0x08000008) >> 2;



	// Flush all changes to new file
    char *suffix = "_pin.gba";
    size_t suffix_length = strlen(suffix);
    char new_filename[FILENAME_MAX];
    strncpy(new_filename, argv[1], FILENAME_MAX);
    strncpy(new_filename + romfilename_len - 4, suffix, strlen(suffix));
    
    if (!(outfile = fopen(new_filename, "wb")))
    {
        puts("Could not open output file");
        puts(strerror(errno));
		scanf("%*s");
        return 1;
    }
    
    fwrite(rom, 1, romsize, outfile);
    fflush(outfile);

    printf("Patched successfully. Changes written to %s\n", new_filename);
    scanf("%*s");
	return 0;
	
}
