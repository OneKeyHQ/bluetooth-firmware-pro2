#ifndef __FMNA_STORAGE_H__
#define __FMNA_STORAGE_H__

#include <stdint.h>
#include "fmna_constants.h"

/*****************************************************************************
 ******************************** Memory Map *********************************
            0x80000  <--- |-----------------------|------
                          |      DFU settings     |  Size 4 Kb
            0x7F000  <--- |-----------------------|------
                          |      MBR params       |  Size 4 Kb
            0x7E000  <--- |-----------------------|------ 
                          |      bootloader       |  Size 56 Kb
            0x70000  <--- |-----------------------|------
                          |     peer manage       |  Size 12 Kb
            0x6D000  <--- |-----------------------|------
                          |     onekey info       |  Size 4 Kb
            0x6C000  <--- |-----------------------|------  
                          |findmy init token/uuid |  Size 4 Kb
            0x6B000  <--- |-----------------------|------  
                          |     findmy info       |  Size 16 Kb (nvs)
            0x67000  <--- |-----------------------|------
                          |          APP          |  Size 260 Kb          
            0x26000  <--- |-----------------------|------
                          |       softdevice      |  Size 152 Kb    
            0x0000   <--- |-----------------------|------
*****************************************************************************/
// please refer to @ok_device_config.h for onekey flash configuration 
#define FSTORAGE_FMNA_NVS_START_ADDR 0x67000
#define FSTORAGE_FMNA_NVS_END_ADDR   0x6AFFF
#define FMNA_NVS_PAGE_SIZE           0x1000

#define SOFTWARE_AUTH_UUID_ADDR      0x6B000
#define SOFTWARE_AUTH_TOKEN_ADDR     SOFTWARE_AUTH_UUID_ADDR + SOFTWARE_AUTH_UUID_BLEN

// fmna storage IDs
#define FMNA_PUBLIC_KEY_P            0x1001
#define FMNA_INITIAL_PRIMARY_SK      0x1002
#define FMNA_INITIAL_SECONDARY_SK    0x1003
#define FMNA_SERVER_SHARED_KEY       0x1004
#define FMNA_CURRENT_PRIMARY_SK      0x1005
#define FMNA_CURRENT_SECONDARY_SK    0x1006
#define FMNA_ICLOUD_ID               0x1007
#define FMNA_PAIRED_STATE            0x1008
#define FMNA_AUTH_TOKEN_UUID         0x1009

void fmna_storage_init(void);
int  fmna_storage_read(uint16_t id, void *data, size_t size);
int  fmna_storage_write(uint16_t id, const void *data, size_t size);
int  fmna_storage_erase(uint16_t id);

#endif /* __FMNA_STORAGE_H__ */
