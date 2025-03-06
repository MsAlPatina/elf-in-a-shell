/* SHELL.c - UNIX-like Shell for SBC1806 */
#include <olduino.h>
#include <nstdlib.h>
#include <scc.h>

/* Memory Map Constants */
#define HEAP_START   0x8000
#define HEAP_END     0x8FFF
#define PROG_START   0xC001
#define PROG_END     0xCFFF
#define USER_START   0xD000
#define USER_END     0xEFFF
#define RUN_START 0x9000
#define RUN_END 0xC000
#define MAX_FILES    32
#define MAX_NAME_LEN 16

/* ANSI Colors */
#define COLOR_RED "\x1b[31m"
#define COLOR_GREEN "\x1b[32m"
#define COLOR_CYAN "\x1b[36m"
#define COLOR_RESET "\x1b[0m"

/* System Constants */
#define MAX_ARGS 5
#define MAX_CMD 80
#define VERSION "1.2"
#define NULL 0
#define MAX_DIR_DEPTH 4
#define TYPE_FILE 0
#define TYPE_DIR  1
#define MAX_PATH_LEN 64
#define MAX_DISPLAY_NAME 15  // Leave room for null terminator



/* File System Structure */
typedef struct {
    char name[MAX_NAME_LEN];
    void (*start)(void);
    unsigned size;
    unsigned char type;    // TYPE_FILE or TYPE_DIR
    int parent;      // -1 for root, index otherwise
} FileEntry;

/* Add these global state variables */
static int current_dir;// = -1;  // -1 = root
static char current_path[64];// = "/";

/* Memory Access Functions */
FileEntry* get_file_table(void) {  // Use the typedef name
    return (FileEntry*)PROG_START;  // No "struct" needed
}

/* Custom strrchr - Find last occurrence of character in string */
char* strrchr(const char *s, int c) {
    const char *found = NULL;
    
    do {
        if(*s == (char)c) {
            found = s;
        }
    } while(*s++);
    
    return (char*)found;
}

/* Custom strncat - Append n characters from src to dest */
char* strncat(char *dest, const char *src, unsigned int n) {
    char *orig_dest = dest;
    
    // Find end of dest
    while(*dest) dest++;
    
    // Copy up to n chars or until null
    while(n-- && (*dest++ = *src++)) {
        if(n == 0) {
            *dest = '\0';
        }
    }
    
    return orig_dest;
}

int atoi(const char *s) {
    int n = 0, sign = 1;
    if(*s == '-') sign = -1, s++;
    while(*s >= '0' && *s <= '9') n = n*10 + (*s++ - '0');
    return sign * n;
}

/* Hexadecimal conversion */
int htoi(const char *s) {
    int n = 0;
    while(*s) {
        if(*s >= '0' && *s <= '9') n = n*16 + (*s - '0');
        else if(*s >= 'a' && *s <= 'f') n = n*16 + (*s - 'a' + 10);
        else if(*s >= 'A' && *s <= 'F') n = n*16 + (*s - 'A' + 10);
        else break;
        s++;
    }
    return n;
}

unsigned* get_user_ptr(void) {
    return (unsigned*)(USER_START);
}

/* Revised shell_cd function */
void shell_cd(int argc, char **argv) {
    FileEntry *files = get_file_table();
    char new_path[MAX_PATH_LEN];
    unsigned int current_len;
    
    int new_dir = current_dir;
    int i = 0;
    int found = -1;
    char *last_slash;

    if(argc != 2) {
        printf("Usage: cd <directory>\r\n");
        return;
    }

    strcpy(new_path, current_path);
    new_path[MAX_PATH_LEN-1] = '\0';

    if(strcmp(argv[1], "..") == 0) {
        // Handle parent directory
        if(current_dir != -1) {
            new_dir = files[current_dir].parent;
            last_slash = strrchr(new_path, '/');
            if(last_slash) {
                // Preserve root slash
                *last_slash = (last_slash == new_path) ? '\0' : '\0';
                if(new_path[0] == '\0') {
                    new_path[0] = '/';
                    new_path[1] = '\0';
                }
            }
        }
    } else {
        // Find directory in current location
        found = -1;
        for(i = 0; i < MAX_FILES; i++) {
            if(files[i].type == TYPE_DIR && 
               files[i].parent == current_dir &&
               strcmp(files[i].name, argv[1]) == 0) {
                found = i;
                break;
            }
        }

        if(found == -1) {
            printf("Directory not found: %s\r\n", argv[1]);
            return;
        }

        // Build new path manually
        strcpy(new_path, current_path);
        current_len = strlen(new_path);
        
        // Handle root differently
        if(current_len == 1 && new_path[0] == '/') {
            strncat(new_path, argv[1], MAX_PATH_LEN - current_len - 1);
        } else {
            strncat(new_path, "/", MAX_PATH_LEN - current_len - 1);
            strncat(new_path, argv[1], MAX_PATH_LEN - current_len - 2);
        }
        
        new_path[MAX_PATH_LEN-1] = '\0';  // Ensure termination
        new_dir = found;
    }

    // Update state if valid
    if(new_dir != current_dir) {
        // Validate path length
        if(strlen(new_path) >= MAX_PATH_LEN) {
            printf("Path too long!\r\n");
            return;
        }

        strcpy(current_path, new_path);
        current_dir = new_dir;
    }
}

// List files command
/* Enhanced shell_ls with safety checks */
/* Revised path display in shell_ls */
void shell_ls(int argc, char **argv) {
    int i = 0;
    char safe_name[MAX_DISPLAY_NAME + 1];
    unsigned int len;
    FileEntry *files = get_file_table();
    char display_path[MAX_PATH_LEN + 1];
    
    // Build display path
    strcpy(display_path, current_path);
    display_path[MAX_PATH_LEN] = '\0';
    
    // Add trailing slash if not root
    len = strlen(display_path);
    if(len > 0 && display_path[len-1] != '/') {
        if(len < MAX_PATH_LEN) {
            display_path[len] = '/';
            display_path[len+1] = '\0';
        }
    }
    
    printf("Contents of %s:\r\n", display_path);
    printf("%s %s %s\r\n", "Type", "Name", "Size");
    
    for(i = 0; i < MAX_FILES; i++) {
        if(files[i].name[0] != '\0' && 
           files[i].parent == current_dir) {
            //char safe_name[MAX_DISPLAY_NAME + 1];
            strcpy(safe_name, files[i].name);
            safe_name[MAX_DISPLAY_NAME] = '\0';
            
            printf("%s %s %d\r\n", 
                   files[i].type == TYPE_DIR ? "DIR" : "FILE",
                   safe_name, 
                   files[i].size);
        }
    }
}

// Delete file command
void shell_del(int argc, char **argv) {
    int i = 0;
    FileEntry *files = get_file_table();
    
    if(argc != 2) {
        printf("Usage: del <name>\r\n");
        return;
    }

    for(i = 0; i < MAX_FILES; i++) {
        if(strcmp(files[i].name, argv[1]) == 0) {
            // Mark entry as free
            files[i].name[0] = '\0';
            files[i].start = NULL;
            files[i].size = 0;
            printf("Deleted '%s'\r\n", argv[1]);
            return;
        }
    }
    printf("File not found!\r\n");
}




void print_hex_dump(const char *data, unsigned size) {
    unsigned offset = 0;
    char ascii[17];
    int i;
    while(offset < size) {
        /* Print address */
        printf("%x: ", offset);
        
        /* Print hex bytes */
        for(i = 0; i < 16; i++) {
            if(offset + i < size) {
                printf("%x ", (unsigned char)data[offset + i]);
                ascii[i] = (data[offset + i] >= 32 && data[offset + i] < 127) ? 
                          data[offset + i] : '.';
            } else {
                printf("   ");
                ascii[i] = ' ';
            }
        }
        
        /* Print ASCII representation */
        ascii[16] = '\0';
        printf("| %s |\r\n", ascii);
        
        offset += 16;
    }
}

void shell_cat(int argc, char **argv) {
    FileEntry *files = get_file_table();
    int found = -1;
    int i = 0;
    char *data ;//= (char*)files[found].start;
    unsigned size ;//= files[found].size;
    unsigned j;
    
    if(argc != 3) {
        printf("Usage: cat <file> <HEX|TXT>\r\n");
        return;
    }

    /* Find the file */
    for(i = 0; i < MAX_FILES; i++) {
        if(files[i].name[0] && 
           strcmp(files[i].name, argv[1]) == 0 &&
           files[i].type == TYPE_FILE) {
            found = i;
            break;
        }
    }

    if(found == -1) {
        printf("File not found: %s\r\n", argv[1]);
        return;
    }

    data = (char*)files[found].start;
    size = files[found].size;

    /* Validate memory range */
    /*
    if((unsigned int *)data < USER_START || 
       (unsigned int *)data + size > USER_END) {
        printf("Invalid file memory!\r\n");
        return;
    }*/

    /* Process display mode */
    if(strcmp(argv[2], "TXT") == 0) {
        printf("Text view of %s (%d bytes):\r\n", argv[1], size);
        for(j = 0; i < size; i++) {
            putc(data[i]);
        }
        printf("\r\n");
    } else if(strcmp(argv[2], "HEX") == 0) {
        printf("Hex dump of %s (%d bytes):\r\n", argv[1], size);
        print_hex_dump(data, size);
    } else {
        printf("Invalid mode! Use HEX or TXT\r\n");
    }
}

/* Command Implementations */
void shell_load(int argc, char **argv) {
    unsigned size;
    int slot = -1;
    int i=0;
    FileEntry *files = get_file_table();
    unsigned *user_ptr = get_user_ptr();
    
    if(argc != 2) {
        printf("Usage: load <name>\r\n");
        return;
    }

    /* Memory validation */
    if(*user_ptr >= USER_END) {
        printf("Memory full!\r\n");
        return;
    }

    /* Find existing or free slot */
    for(i=0; i<MAX_FILES; i++) {
        // First check for existing file
        if(files[i].name[0] != '\0' && strcmp(files[i].name, argv[1]) == 0) {
            printf("File exists!\r\n");
            return;
        }
        // Then look for first empty slot
        if(slot == -1 && files[i].name[0] == '\0') {
            slot = i;
        }
    }
    
    if(slot == -1) {
        printf("File table full!\r\n");
        return;
    }

    /* Load from serial */
    printf("Send data now...\r\n");
    size = getsRTS((char*)*user_ptr);
    
    if(size == 0) {
        printf("Load failed!\r\n");
        return;
    }

    /* Clear and populate file entry */
    memset(&files[slot], 0, sizeof(FileEntry));  // Clear entire entry
    strcpy(files[slot].name, argv[1]);
    files[slot].type = TYPE_FILE;
    files[slot].parent = current_dir;
    files[slot].start = (void(*)())*user_ptr;
    files[slot].size = size;
    
    /* Advance memory pointer */
    *user_ptr += size;
    
    printf("Loaded '%s' (%d bytes)\r\n", argv[1], size);
}

void verbose_cpy(char* dest, void* src, unsigned int count) {
    unsigned int i;
    char *src_char = (char *)src; // Cast src to char*

    for (i = 0; i < count; i++) {
        printf("moved %d/%d bytes...\r", i, count);
        dest[i] = src_char[i];    // Now properly accesses bytes
        
    }
}



void shell_run(int argc, char **argv) {
    int i=0;
    unsigned int j;
    char *runzone = (char*)RUN_START;
    FileEntry *files = get_file_table();
    unsigned *user_ptr = get_user_ptr();
    //unsigned *saved_ptr;
    
    if(argc != 2) {
        printf("Usage: run <name>\r\n");
        return;
    }

    for(i=0; i<MAX_FILES; i++) {
        if(strcmp(files[i].name, argv[1]) == 0) {
            //printf("Running '%s'...\r\n", argv[1]);
            //files[i].start();
            //printf("running %d bytes...\r\n",files[i].size);
            printf("Moving %d bytes...",files[i].size);
            
            //memcpy((void*)0x9000,files[i].start ,files[i].size);
            //memcpy((void*)RUN_START, files[i].start, files[i].size);
            for(j=0;j<files[i].size;j++){
                runzone[j]=((char *)files[i].start)[j];
            }

            /*for anyone reading this: why did i do this?
            
            on nstdlib.c you can find memcpy, and during testing, the pointers were being moved.
            as such the moment we use the pointer again its in another totally different spot.

            */
            

            printf(" done!\r\n");
            printf("Validating load...\r\n");
            if(memcmp((char*)RUN_START, (char*)files[i].start, files[i].size) != 0) {
                printf(COLOR_RED "Copy verification failed!\r\n" COLOR_RESET);
                //asm(" ei\n");
                return;
            }else{
                printf(COLOR_GREEN "Copy verification OK!\r\n" COLOR_RESET);
            }
            
            asm("   LBR 0x9000\n");
            return;
        }
    }
    printf("File not found!\r\n");

}

/* Memory Initialization (call once at startup) */
/*void init_memory_system(void) {
    unsigned *user_ptr = get_user_ptr();
    FileEntry *files = get_file_table();
    int i = 0;
    int root_initialized = 0;
    
    // Only initialize user pointer if invalid 
    if(*user_ptr < USER_START || *user_ptr > USER_END) {
        *user_ptr = USER_START + sizeof(unsigned);
    }

    ///* Initialize root directory only if needed 
    root_initialized = 0;
    for(i = 0; i < MAX_FILES; i++) {
        if(files[i].type == TYPE_DIR && files[i].parent == -1) {
            root_initialized = 1;
            break;
        }
    }
    
    if(!root_initialized) {
        // Create root directory in first available slot 
        for(i = 0; i < MAX_FILES; i++) {
            if(files[i].name[0] == '\0') {
                files[i].type = TYPE_DIR;
                files[i].parent = -1;
                strcpy(files[i].name, "");
                files[i].size = 0;
                break;
            }
        }
    }

    // Initialize current directory state 
    current_dir = -1;
    strcpy(current_path, "/");
    current_path[MAX_PATH_LEN-1] = '\0';
}
*/

void init_memory_system(void) {
    int i=0;
    /* Initialize user memory pointer */
    unsigned *user_ptr = get_user_ptr();
    FileEntry *files = get_file_table();
    *user_ptr = USER_START + sizeof(unsigned);  // Skip pointer storage
    
    /* Clear file table */
    /* Add these global state variables */
    current_dir = -1;  // -1 = root
    
    
    for(i=0;i<sizeof(current_path);i++){
        current_path[i]=NULL;
    }
    current_path[0] = '/';
    for(i=0; i<MAX_FILES; i++) {
        files[i].name[0] = '\0';
        files[i].start = NULL;
        files[i].size = 0;
    }

    /* Revised directory tracking */
    current_path[MAX_PATH_LEN] = '/';
    // Create root directory
    files[0].name[0] = '\0';  // Root has empty name
    files[0].type = TYPE_DIR;
    files[0].parent = -1;
    files[0].start = NULL;
    files[0].size = 0;

        
        
}

void shell_mkdir(int argc, char **argv) {
    int i = 0;
    int slot = -1;
    FileEntry *files = get_file_table();
    

    if(argc != 2) {
        printf("Usage: mkdir <name>\r\n");
        return;
    }

    // Find free slot
    for(i = 0; i < MAX_FILES; i++) {
        if(files[i].name[0] == '\0') {
            slot = i;
            break;
        }
    }

    if(slot == -1) {
        printf("No free slots!\r\n");
        return;
    }

    // Set directory entry
    strcpy(files[slot].name, argv[1]);
    files[slot].name[MAX_NAME_LEN-1] = '\0';
    files[slot].type = TYPE_DIR;
    files[slot].parent = current_dir;
    files[slot].start = NULL;
    files[slot].size = 0;

    printf("Created directory '%s'\r\n", argv[1]);
}

/* Modified shell_mem */
void shell_mem(int argc, char **argv) {
    int i = 0;
    unsigned int *user_ptr = get_user_ptr();
    FileEntry *files = get_file_table();
    unsigned int user_used = *user_ptr - USER_START;
    unsigned int user_total = USER_END - USER_START;
    int count = 0;
    unsigned int files_bytes = 0;

    for(i = 0; i < MAX_FILES; i++) {
        if(files[i].name[0] != '\0') {
            count++;
            files_bytes += files[i].size;  // Sum sizes of all files
        }
    }

    printf("Memory Map:\r\n");
    printf("  User:  %d/%d bytes used\r\n", user_used, user_total);
    printf("  Files: %d/%d slots occupied (%d/%d bytes used)\r\n", 
           count, MAX_FILES, files_bytes, user_total);
    printf("  Run zone: %d Bytes (address: %x/%x)\r\n",RUN_END-RUN_START,RUN_START,RUN_END);
}
void sccInit() {
    //asm(" seq");
    out(1,0x01);
    out(4,0x02);
    out(6,0x10);
    out(4,0x00);
    out(6,0xD3);
    out(6,0x07);
    out(4,0x02);
    out(6,0x05);
    out(4,0x01);
    out(6,0xBB);
    out(4,0x05);
    out(6,0x02);
    out(4,0x02);
    out(6,0x05);
    inp(6);
    out(4,0x04);
    out(6,0x80);
    //asm(" req");
    printf("Serial port initialized\r\n");
}

void get_command(char *buf) {
    char *p = buf;
    char c;
    int length = 0;

    while(1) {
        c = getc();
        
        /* Handle Enter */
        if(c == '\r' || c == '\n') {
            *p = '\0';
            printf("\r\n");
            return;
        }
        
        /* Handle Backspace */
        if(c == 8 || c == 127) {
            if(p > buf) {
                p--;
                length--;
                printf("\b \b");
            }
            continue;
        }
        
        /* Regular Character */
        if(length < MAX_CMD-1 && c >= 32 && c <= 126) {
            *p++ = c;
            length++;
            putc(c);
        }
    }
}

void shell_help(int argc, char **argv) {
    //const char *names[] = {"help", "mem", "rtc", "io", "clear","load","run","ls","del","cd","mkdir","hardwipe", NULL};
    const char *names[] = {"help", "mem", "rtc", "io", "clear","load","run","ls","del","cd","mkdir","hardwipe","cat", NULL};
    int i = 0;
    const char *helps[] = {
        "Show this help",
        "Memory operations",
        "Real-Time Clock control",
        "Hardware I/O control",
        "Clear screen",
        "Load <filename> lets user upload a program as .bin and save it",
        "Run <filename> run binary file",
        "list current directory",
        "del <filename> delete a file",
        "cd <path> change directory",
        "mkdir <dir> make a directory",
        "hardwipe wipes all data",
        "cat <file> <TXT|HEX>",
        NULL
    };

    printf("SBC1806 Shell v" VERSION "\r\nAvailable commands:\r\n");
    for(i = 0; names[i]; i++)
        printf("  %s %s\r\n", names[i], helps[i]);
}



void shell_rtc(int argc, char **argv) {
    char flags;
    char seconds;
    char minutes;
    char hours; 
	char zeror;
	out(1,0x01);
	out(4,0x0D);
	flags=inp(7);
	if(flags&0x01){
		printf("\033[37;42mRTC battery OK\033[0m\r\n");
	}else{
		printf("\033[37;41;1mRTC battery BAD\033[0m\r\n");
	}
	if(flags&0x02){
		printf("\033[34;43mPower bus fault\033[0m\r\n");
	}
	printf("%x\r\n",flags);
    out(4,0x00);
    seconds=inp(7);
    out(4,0x02);
    minutes=inp(7);
    out(4,0x04);
    hours=inp(7);
	out(4,0x0F);
	zeror=inp(7);
	printf("%cx\r\n",zeror);
	if(zeror){
		printf("Address fault.\r\n");
	}

    printf("%cx:%cx:%cx\r\n",hours,minutes,seconds);
    


}

void shell_clear(int argc, char **argv) {
    printf("\033[2J\033[H");
}


void shell_io(int argc, char **argv) {
    unsigned bank, port, value;
    const char *bank_name;
    
    if(argc < 3) {
        printf(COLOR_CYAN "Usage: io <bank> <port> [value]\r\n"
               "  Read:  io <A|B|C|CTRL> <port>\r\n"
               "  Write: io <A|B|C|CTRL> <port> <value>\r\n" COLOR_RESET);
        return;
    }

    /* Parse bank selection */
    if(strcmp(argv[1], "A") == 0) {
        bank = 0x00;
        bank_name = "A";
    } else if(strcmp(argv[1], "B") == 0) {
        bank = 0x01;
        bank_name = "B";
    } else if(strcmp(argv[1], "C") == 0) {
        bank = 0x02;
        bank_name = "C";
    } else if(strcmp(argv[1], "CTRL") == 0) {
        bank = 0x03;
        bank_name = "CTRL";
    } else {
        printf(COLOR_RED "Invalid bank! Use A/B/C/CTRL\r\n" COLOR_RESET);
        return;
    }

    /* Parse port number */
    port = atoi(argv[2]);
    if(port > 7) {
        printf(COLOR_RED "Invalid port (0-7)\r\n" COLOR_RESET);
        return;
    }

    /* Select bank */
    out(1, bank);

    if(argc == 3) {
        /* Read mode */
        value = inp(port);
        printf("Bank %s Port %d: 0x%cx (%d)\r\n", 
               bank_name, port, value, value);
    } else {
        /* Write mode */
        value = (argv[3][0] == '0' && argv[3][1] == 'x') ? 
               htoi(argv[3]+2) : atoi(argv[3]);
        
        if(value > 255) {
            printf(COLOR_RED "Invalid value (0-255)\r\n" COLOR_RESET);
            return;
        }
        
        out(port, value);
        printf("Bank %s Port %d set to 0x%cx (%d)\r\n", 
               bank_name, port, value, value);
    }
}

char *strtok(char *str, const char *delim) {
    char *last = NULL;
    char *start;
    static char *save_ptr;
    
    if(str) save_ptr = str;
    if(!save_ptr || !*save_ptr) return NULL;
    
    /* Skip leading delimiters */
    while(*save_ptr) {
        const char *d = delim;
        while(*d) {
            if(*save_ptr == *d++) {
                save_ptr++;
                break;
            }
        }
        if(!*d) break;
    }
    
    start = save_ptr;
    
    /* Find end of token */
    while(*save_ptr) {
        const char *d = delim;
        while(*d) {
            if(*save_ptr == *d++) {
                *save_ptr++ = 0;
                return start;
            }
        }
        save_ptr++;
    }
    
    /* Return last token */
    if(*start) return start;
    return NULL;
}

void print_prompt(void) {
    if(strcmp(current_path,"/")==0){
        printf(COLOR_GREEN "sbc1806:" COLOR_CYAN "/" COLOR_RESET "$ ");
    }else{
        printf(COLOR_GREEN "sbc1806:" COLOR_CYAN "%s" COLOR_RESET "$ ",current_path);
    }
}

void shell_hardwipe(int argc, char **argv){
    // Clear all entries thoroughly
    /*int i = 0;
    FileEntry *files = get_file_table();
    unsigned *user_ptr = get_user_ptr();

    ///* Clear all file entries 
    for(i = 0; i < MAX_FILES; i++) {
        memset(&files[i], 0, sizeof(FileEntry));
    }

    ///* Reset memory pointer 
    *user_ptr = USER_START + sizeof(unsigned);

    ///* Recreate root directory 
    files[0].type = TYPE_DIR;
    files[0].parent = -1;
    strcpy(files[0].name, "");

    ///* Reset directory state 
    */
    init_memory_system();
    current_dir = -1;
    strcpy(current_path, "/");
    
    printf("System fully reset\r\n");
    
}

void execute_command(char *cmdline) {
    char *args[MAX_ARGS] = {0};
    int argc = 0;
    int i = 0;
    
    /* Local command definitions */
    const char *names[] = {"help", "mem", "rtc", "io", "clear","load","run","ls","del","cd","mkdir","hardwipe","cat", NULL};
    void (*funcs[])(int, char**) = {shell_help, shell_mem, shell_rtc, 
                                   shell_io, shell_clear,shell_load,shell_run,shell_ls,shell_del,shell_cd,shell_mkdir,shell_hardwipe,shell_cat, NULL};

    /* Tokenize command line */
    args[argc] = strtok(cmdline, " ");
    while(args[argc] && argc < MAX_ARGS-1)
        args[++argc] = strtok(NULL, " ");

    if(!argc) return;

    /* Find matching command */
    for(i = 0; names[i]; i++) {
        if(strcmp(args[0], names[i]) == 0) {
            funcs[i](argc, args);
            return;
        }
    }
    printf(COLOR_RED "Command not found: %s\r\n" COLOR_RESET, args[0]);
}

/* Main Function */
int main() {
    char cmd_buffer[MAX_CMD];
    
    sccInit();
    //init_memory_system();
    printf("\033[2J\033[H");
    printf("SBC1806 Shell v" VERSION "\r\n");
    
    while(1) {
        print_prompt();
        get_command(cmd_buffer);
        execute_command(cmd_buffer);
    }
    return 0;
}
#include <olduino.c>
#include <nstdlib.c>
