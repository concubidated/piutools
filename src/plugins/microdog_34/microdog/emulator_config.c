#include <stdlib.h>
#include <memory.h>

#include <plugin_sdk/ini.h>
#include "utils.h"
#include "emulator_config.h"


static int count_entries = 1;
static int entry_index = 0;
static char* microdog_ini_path;

EMULATED_DOG emu_dog;
static int parse_microdog_info(void* user, const char* section, const char* name, const char* value){
    char *ptr;
    if(!strcmp(section,"INFO")){
        if(!strcmp(name,"serial")){
            emu_dog.serial = strtoul(value,&ptr,16);
        }else if(!strcmp(name,"id")){
            str_to_hex(emu_dog.id,8,value);
        }else if(!strcmp(name,"mfg_serial")){
            emu_dog.mfg_serial = strtoul(value,&ptr,16);
        }else if(!strcmp(name,"password")){
            emu_dog.password = strtoul(value,&ptr,16);
        }else if(!strcmp(name,"memory")){
            str_to_hex(emu_dog.memory,200,value);
            memcpy(&emu_dog.algorithm,emu_dog.memory+196,4);
        }
    }
    return 1;
}

static int count_microdog_entries(void* user, const char* section, const char* name, const char* value){
    // Create tag for Convert
    char convert_tag[17] = {0x00};
    snprintf(convert_tag,sizeof(convert_tag),"CONVERT_%08x",emu_dog.algorithm);
    if(!strcmp(section,convert_tag)){
        emu_dog.num_convert_entries++;
    }
    if(!emu_dog.num_convert_entries){return -1;}
    return 1;
}

static int load_convert_entries(void* user, const char* section, const char* name, const char* value){
    // Create tag for Convert
    char *ptr;
    char convert_tag[17] = {0x00};
    snprintf(convert_tag,sizeof(convert_tag),"CONVERT_%08x",emu_dog.algorithm);
    if(!strcmp(section,convert_tag)){
        if(strlen(name) % 2){
            DEBUG_PRINT("ERROR - Convert Table Value is Not Even!\n");
            return -1;
        }

        DOG_CONVERT_ENTRY* current_entry = emu_dog.convert_entry + entry_index;
        current_entry->request_len = strlen(name) / 2;
        str_to_hex(current_entry->request,current_entry->request_len,name);
        current_entry->response = strtoul(value,&ptr,16);
        entry_index++;
    }
    return 1;
}


void print_info(){
    DEBUG_PRINT("Dog Serial: %04X",emu_dog.serial);
    DEBUG_PRINT("Dog ID: %s",hex_to_str(emu_dog.id,sizeof(emu_dog.id)));
    DEBUG_PRINT("Mfg Serial: %04X",emu_dog.mfg_serial);
    DEBUG_PRINT("Algorithm Descriptor: %04X",emu_dog.algorithm);
    DEBUG_PRINT("Dog Memory: %s",hex_to_str(emu_dog.memory,sizeof(emu_dog.memory)));
    DEBUG_PRINT("Dog Crypto Convert Table Entries: %d",emu_dog.num_convert_entries);
}


void load_config(const char* path_to_dongle_file){
    if(!path_to_dongle_file){
        path_to_dongle_file = "./io.microdog.ini";
    }
    memset(&emu_dog,0x00,sizeof(struct _EMULATED_DOG));
    // Pass 1 - Get INFO
    int error = ini_parse(path_to_dongle_file,parse_microdog_info,NULL);
    if (error < 0) {
        DEBUG_PRINT("[IO.MicroDog::FATAL] Can't read microdog ini!");
        exit(-1);
    }
    // Pass 2 - Get Number of Convert Entries
    error = ini_parse(path_to_dongle_file,count_microdog_entries,NULL);
    DEBUG_PRINT("Found %d Convert Entries...",emu_dog.num_convert_entries);

    emu_dog.convert_entry = calloc(1,emu_dog.num_convert_entries * sizeof(struct _CONVERT_ENTRY));
    if(!emu_dog.convert_entry){
        DEBUG_PRINT("Error Allocating Space for Convert Table!");
        exit(-1);
    }
    // Pass 2: Get CONVERT Entries
    DEBUG_PRINT("Loading Dog Table - Please Wait...");
    ini_parse(path_to_dongle_file,load_convert_entries,NULL);
    DEBUG_PRINT("Loaded Emulated Microdog Device");
    print_info();
}