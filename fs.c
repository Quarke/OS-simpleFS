
#include "fs.h"
#include "disk.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>

#define FS_MAGIC           0xf0f03410
#define INODES_PER_BLOCK   128
#define POINTERS_PER_INODE 5
#define POINTERS_PER_BLOCK 1024

int * bitmap = NULL;

struct fs_superblock {
    int magic;
    int nblocks;
    int ninodeblocks;
    int ninodes;
};

struct fs_inode {
    int isvalid;
    int size;
    int direct[POINTERS_PER_INODE];
    int indirect;
};

union fs_block {
    struct fs_superblock super;
    struct fs_inode inode[INODES_PER_BLOCK];
    int pointers[POINTERS_PER_BLOCK];
    char data[DISK_BLOCK_SIZE];
};

int fs_format()
{
    
    union fs_block block;
    
    if(disk_size() < 3){
        printf("Disk to small\n");
        return 0;
    }else if(bitmap != NULL){
        printf("Cannot format mounted image\n");
        return 0;
    }
    
    // set the super block
    block.super.magic = FS_MAGIC;
    block.super.nblocks = disk_size();
    block.super.ninodeblocks = disk_size()/10 + 1;
    block.super.ninodes = 128 * block.super.ninodeblocks;
    
    // write the super block
    disk_write(0, block.data);
    
    // zero out the inode blocks you just made
    union fs_block zero;
    memset(zero.data, 0, 4096);
    
    for(int inode_block = 1; inode_block <= block.super.ninodeblocks; inode_block++){
        disk_write(inode_block, zero.data);
    }
    
    return 1;
}

void fs_debug()
{
    union fs_block block;
    
    disk_read(0, block.data);
    
    printf("superblock:\n");
    printf("    %d blocks\n", block.super.nblocks);
    printf("    %d inode blocks\n", block.super.ninodeblocks);
    printf("    %d inodes\n", block.super.ninodes);
    
    union fs_block inode_block;
    struct fs_inode inode;
    for(int i = 1; i < block.super.ninodeblocks; i++){
        
        disk_read(i, inode_block.data);
        
        for (int i_node = 0; i_node < INODES_PER_BLOCK; i_node++) {
            
            inode = inode_block.inode[i_node];
            
            if(inode.isvalid){
                printf("inode %d:\n", i_node);
                printf("    size: %d\n", inode.size);
                printf("    direct blocks:");
                
                for (int d_blocks = 0; d_blocks * 4096 < inode.size && d_blocks < 5; d_blocks++) {
                    printf(" %d", inode.direct[d_blocks]);
                }
                
                printf("\n");
                
                if(inode.size > 5 * 4096){
                    
                    printf("    indirect: %d\n", inode.indirect);
                    union fs_block temp_block;
                    disk_read(inode.indirect, temp_block.data);
                    
                    
                    printf("    indirect data blocks:");
                    for( int indirect_block = 0; indirect_block < (double)inode.size/4096 - 5; indirect_block++){
                        printf(" %d", temp_block.pointers[indirect_block]);
                    }
                    printf("\n");
                }
            }
        }
    }
}

int fs_mount()
{
    union fs_block block;
    disk_read(0, block.data);

    bitmap = calloc(5, sizeof *bitmap);
    
    return 1;
}

int fs_create()
{
    if(bitmap == NULL){
        printf("No mounted disk\n");
        return 0;
    }

    union fs_block block;
    disk_read(0, block.data);
    
    
    for(int inode_block_index = 1; inode_block_index < block.super.nblocks; inode_block_index++){
        // read the inode block and begin checking it for open spaces
        disk_read(inode_block_index, block.data);
        
        // must start at one because of the union
        struct fs_inode inode;
        for(int inode_index = 1; inode_index < POINTERS_PER_BLOCK; inode_index++){
            
            // read space as an inode, and check valid flag
            inode = block.inode[inode_index];
            
            if(!inode.isvalid){
                
                // if the inode is invalid, we can fill the space safely
                inode.isvalid = true;
                inode.size = 0;
                memset(inode.direct, 0, sizeof(inode.direct));
                inode.indirect = 0;
                
                bitmap[inode_block_index] = 1;
                return inode_index + inode_block_index * 128;
            }
        }
    }

    printf("Could not create inode, bitmaps are full");
    return 0;
}

int fs_delete( int inumber )
{
    // calculate correct inode block
    int inode_block_index = (inumber + 128 - 1)/128;
    
    union fs_block block;
    disk_read(0, block.data);
    
    if(inode_block_index > block.super.ninodeblocks){
        printf("Inode number outside limit\n");
        return 0;
    }
    
    // read the inode block and begin checking it for open spaces
    disk_read(inode_block_index, block.data);
    
    struct fs_inode inode = block.inode[inumber%128];
    if(inode.isvalid){
        //zero it out and return 1;
        inode = (struct fs_inode){0};
        return 1;
        
    }else{
        //cannot delete invalid inode, I think...
        printf("Not a valid inode, cannot delete\n");
        return 0;
    }
}

int fs_getsize( int inumber )
{
    return -1;
}

int fs_read( int inumber, char *data, int length, int offset )
{
    return 0;
}

int fs_write( int inumber, const char *data, int length, int offset )
{
    return 0;
}
