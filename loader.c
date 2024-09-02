#include "loader.h"
#include <stdbool.h> 

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;

int fd;
void loader_cleanup() {

    free(ehdr);
    free(phdr);
}
bool elf_check_file(Elf32_Ehdr *hdr) {
    if(!hdr) return false;
    bool check=
        hdr->e_ident[EI_MAG0] == ELFMAG0 &&
        hdr->e_ident[EI_MAG1] == ELFMAG1 &&
        hdr->e_ident[EI_MAG2] == ELFMAG2 &&
        hdr->e_ident[EI_MAG3] == ELFMAG3 &&
	memcmp(hdr->e_ident,ELFMAG,SELFMAG)==0;
    if(check){
	    return true;
    }
    else{
    return false;}
}



/*
 * Load and run the ELF executable file
 */
void load_and_run_elf(char** exe) {
    fd = open(exe[1], O_RDONLY);
    if (fd < 0) {
        perror("file is not open");
        return;
    }
// Making space for 32 bit elf header in memory
    ehdr = malloc(sizeof(Elf32_Ehdr));
   
   

    // Reading ELF header into memory
    if (ehdr==NULL || read(fd, ehdr, sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr)) {
        perror("error in loading ELF HEADER");
        loader_cleanup();
        return;
    }
    if (!elf_check_file(ehdr)) {
        perror("file is not valid");
        loader_cleanup();
            return;
    }

    

    lseek(fd, ehdr->e_phoff, SEEK_SET);// Setting the fd to the start of programme headers

    phdr = malloc(ehdr->e_phentsize * ehdr->e_phnum); //Creating the space in heap memory for all the programme headers
 

    // Reading program headers into memory
    if (phdr==NULL ||read(fd, phdr, ehdr->e_phentsize * ehdr->e_phnum) != ehdr->e_phentsize * ehdr->e_phnum) {
        perror("error in loading PROGRAMME HEADER");
        loader_cleanup();
        return;
    }

    void* load_segment = NULL;
    uintptr_t segment_start_add = 0;
    uintptr_t entry_point = ehdr->e_entry;

    // Iterate through the PHDR table and find the section of PT_LOAD type
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            uintptr_t segment_start = phdr[i].p_vaddr;
            uintptr_t segment_end = segment_start + phdr[i].p_memsz;
		// Checking whether Programme Header resides in this segment or not
            if (entry_point >= segment_start && entry_point < segment_end) {
                // Entry point is inside this PT_LOAD segment
                segment_start_add = segment_start;
                

                // Allocate memory of the size "p_memsz" using mmap function
                load_segment = mmap((void *)(uintptr_t)phdr[i].p_vaddr, phdr[i].p_memsz, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                // Loading the segment content into memory
                lseek(fd, phdr[i].p_offset, SEEK_SET);
		
                if (read(fd, load_segment, phdr[i].p_filesz) != phdr[i].p_filesz) {
                    perror("Failed in Loading the PT_LOAD segment");
                    return;
                }
                break;
            }
        }
    }

    // Navigate to the entry point address in the segment loaded in memory
    void* real_start_address = (entry_point - segment_start_add)+load_segment;

    // Typecast the address to a function pointer matching "_start" method
    int (*start_func)();
    start_func = (int (*)())real_start_address;
    
    
	close(fd);
    // Call the "_start" method and print the value returned
    int res = start_func();
    printf("User _start return value = %d\n", res);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: %s <ELF Executable>\n", argv[0]);
        exit(1);
    }
    load_and_run_elf(argv);
    // Invoke the cleanup routine inside the loader
    loader_cleanup();
    return 0;
}
