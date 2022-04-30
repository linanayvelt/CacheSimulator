#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits.h>

char memory[2*1<<16];
int global_counter = 0;

typedef struct block{
    int dirty_bit = 0;
    char data[64];
    int tag;
    int recency = 0;
    int valid = 0;
} block;

int main(int argc, char* argv[]){
    FILE *fp = fopen(argv[1], "r");
    int capacity = atoi(argv[2])*pow(2,10);
    int ways = atoi(argv[3]);
    char* cache_type = argv[4];
    int block_size = atoi(argv[5]);
    int word_size = 16;

    int num_frames = capacity/block_size;
    int num_rows = num_frames/ways;
    bool write_through = false;
    ///if statement to check cache type and set boolean variable to true for write through and a false for write-allocate
    if (strcmp(cache_type, "wt") == 0){
        write_through = true;
    }

    //math
    //calculate tag bits - save in separate int
    //index bits - separate int
    //offset bits - separate int
    //this stuff always varies based on the values
    //check if they add up to 16
    int offset_bits = log2(block_size);
    int index_bits = log2(num_rows);
    int tag_bits = 16 - offset_bits - index_bits;


    block cache[num_rows][ways];

    ///insn_str, data_hex, how many addresses are touched in int, the thing stores
    char* insn_str = (char *)malloc(5*sizeof(char *));
    int access_size = 0;
    unsigned int add_accessed = 0;
    int written_value = 0;

    while(fscanf(fp, "%s %04x %d %x", insn_str, &add_accessed, &access_size, &written_value) != EOF){
        bool load = false;
        bool hit = false;
        int hit_column = 0;
        bool set_full = true;
        int free_column = 0;

        ///1
        ///strcmp to see if insn_str is "load" and set bool load to true false for store
        if(strcmp(insn_str, "load") == 0 && strcmp(insn_str, "store") != 0){
            load = true;
        }

        ///2
        ///data_hex is the 16 bit integer address, we want to get the address values using the tag, index, offset
        ///and calculate offset by doing addess << (tag+index) and then right shift back address >> (tag+index)
        int tag = add_accessed/(num_rows*block_size);
        int index = (add_accessed/block_size)%num_rows;
        int offset = add_accessed%block_size;

        ///3
        ///go to the correct row number in the cache using the index_value
        ///iterate through the block single array to try to find the tag value, which means the block exists
        //so do if statement where if block.tag equals to tag_value then the block exists inside our cache
        ///therefore there is a hit
        // set boolean hit to true for existing in cache
        for(int i = 0; i < ways; i++){
            if(cache[index][i].tag == tag && cache[index][i].valid == 1){
                hit = true;
                hit_column = i;
                break;
            }
        }

            ///("\n\n--------Step: %d ---------\n", global_counter);
            ///printf("instruction: %s \n", insn_str);
            ///printf("address to access: %04x \n", add_accessed);
            ///printf("access size: %d \n", access_size);
            ///printf("written value: %x \n", written_value);
            ///printf("hit: %d \n", hit);
            ///printf("---Address: ----\n");
            ///printf("num_rows: %d \n", num_rows);
            ///printf("tag: %d \n", tag);
            ///printf("index: %d \n", index);
            ///printf("offset: %d \n", offset);

        ///4
        ///check if the set if full
        ///correct index to get row
        ///loop through entire row, checking if the valid bit is 1
        ///if all coluns are 1, set full to true
        for(int i = 0; i < ways; i++){
            if(cache[index][i].valid != 1){
                set_full = false;
            }
        }

        ///5
        ///do for loop to find the first free spot in the row where the valid)bit is zero, save it in int called Free-Column
        for(int i = 0; i < ways; i++){
            if(cache[index][i].valid != 1){
                free_column = i;
                break;
            }
        }

        ///5
        ///not differentiate between write through = ture <-not important load = true hit = true if statement
        ///get the block in the cache
        ///for loop to go through though data array starting from the offset index and ending at offset + "how many addresses touched"
        ///grab the data in the data array based on the offset, the offset is the index of the data array
        ///print somehow
        ///set block recency to global counter
        ///print appopriate information
        if(load == true && hit == true){
            printf("load %04x hit ", add_accessed);
            for(int i = offset; i < (offset+access_size); i++){
                printf("%02x", cache[index][hit_column].data[i] & 0xff);
            }
            cache[index][hit_column].recency = global_counter;
            printf("\n");
            global_counter++;
            continue;
        }

        ///6
        ///write through = true load = true hit = false full = true
        ///if full is true the evict smallest
        ///make valid bit zero, set the whole thing to zero
        ///if the set is full or not, we need to find the block in memory and put it in the cache
        ///int block_start = divide address by block size * block size to get the block we want
        ///change the tag of the block we are replacing to the tag of the block we are replacing it with from the file tag info
        ///this will be in the [row_index][first_indexntified_column]
        ///set dirty_bit to zero, update recency to global count
        ///now we write from memory 
        ///go to memory[block_start] to get the block
        ///do for loop to grab the block size amount of data bytes
        ///for loop start would be the block start value and then go to block_start+block_size, might need -1 to make sure index is correct, there will always be one block
        ///change valid bit to 1
        ///print appropriate information
        if(write_through == true && load == true && hit == false){
            if(set_full == true){
                int minimum = INT_MAX;
                for(int i = 0; i < ways; i++){
                    if(cache[index][i].recency < minimum){
                        minimum = cache[index][i].recency;
                        free_column = i;
                    }
                }
            }

            cache[index][free_column].valid = 0;
            cache[index][free_column].tag = tag;
            cache[index][free_column].dirty_bit = 0;
            cache[index][free_column].recency = global_counter;

            int block_start = add_accessed/block_size;
            block_start = block_start*block_size;

            for(int i = block_start; i < (block_start+block_size); i++){
                 cache[index][free_column].data[i-block_start] = memory[i];
            }
            cache[index][free_column].valid = 1;

            printf("load %04x miss ", add_accessed);
            for(int i = offset; i < (offset+access_size); i++){
                printf("%02x", cache[index][free_column].data[i] & 0xff);
            }

            printf("\n");
            global_counter++;
            continue;
        }

        ///7
        ///write_through = true load = false hit = true
        ///if hit that means the block exists in cache
        ///we have to update the block data array in the cache and memory
        ///do pointer char* to the beginning of the string
        ///fo for loop to het
        //first get the block, and then within the block cache offset is the start, and the end is offset plus the number of accesses
        ///%2hhx to parse it as a a two character hex value and store it into the memory
        ///use sscanf to get the string value and convert to hex
        ///increment the scanf position value 
        ///save it into the struct data array using ampersand
        ///memeory[index_value]
        ///for memory address is the start as it correlated to index in memory, address+ number fo accesses is the end
        ///do the same thing for memory but instead of the & store just do memory[index]
        ///update recency counter for cache block
        ///print appropriate information
        if(write_through == true && load == false && hit == true){
            int position = 0;
            char written_value_string[64];
            sprintf(written_value_string, "%x", written_value);
            for(int i = add_accessed; i < (add_accessed+access_size); i++){
                char* part = (char *)malloc(2*sizeof(char *));
                sscanf(&written_value_string[position], "%2s", part);
                memory[i] = (int)strtol(part, NULL, 16);
                position += 2;

            }

            position = 0;
            sprintf(written_value_string, "%x", written_value);
            for(int i = offset; i < (offset+access_size); i++){
                char* part = (char *)malloc(2*sizeof(char *));
                sscanf(&written_value_string[position], "%2s", part);
                cache[index][hit_column].data[i] = (int)strtol(part, NULL, 16);
                position += 2;
            }

            cache[index][hit_column].recency = global_counter;
            printf("store %04x hit\n", add_accessed);
            global_counter++;
            continue;
        }

        ///8
        ///write_through = true load = false hit = false
        ///memeory[index_value]
        ///for memory address is the start as it correlated to index in memory, address+ number fo accesses is the end
        ///do the same thing for memory but instead of the & store just do memory[index]
        ///print appropriate information
        if(write_through == true && load == false && hit == false){
            int position = 0;
            char written_value_string[64];
            sprintf(written_value_string, "%x", written_value);
            for(int i = add_accessed; i < (add_accessed+access_size); i++){
                char* part = (char *)malloc(2*sizeof(char *));
                sscanf(&written_value_string[position], "%2s", part);
                memory[i] = (int)strtol(part, NULL, 16);
                position += 2;
            }
            printf("store %04x miss\n", add_accessed);
            global_counter++;
            continue;
        }

        ///9
        ///write_through = false load = true hit = false
        ///check if set is full
        //evict
        ///do the same as the other one
        ///check if the block you are evicting check the dirty bit
        ///if the dirty bit is true
        ///transcribe all the data array chars to the appropriate spot in memory
        ///for loop 
        ///address/block_size*block_size to get start index in memory
        ///for loop from start index to start index + block_size, maybe -1
        ///print appropriate information and change the recency
        if(write_through == false && load == true && hit == false){
            if(set_full == true){
                int minimum = INT_MAX;
                for(int i = 0; i < ways; i++){
                    if(cache[index][i].recency < minimum){
                        minimum = cache[index][i].recency;
                        free_column = i;
                    }
                }
            }

            int enter_address = (cache[index][free_column].tag*(num_rows*block_size)) + index*block_size;

            if(cache[index][free_column].dirty_bit == 1  && cache[index][free_column].valid == 1){
                ///printf("-----Kicking out address: %x", enter_address);
                ///printf("-----Kicking out value: ");
                for(int i = enter_address; i < (enter_address+block_size); i++){
                    memory[i] = cache[index][free_column].data[i-enter_address];
                    ///printf("%x", cache[index][free_column].data[i-enter_address]);
                }
                ///printf("\n");
            }

            cache[index][free_column].valid = 0;
            cache[index][free_column].tag = tag;
            cache[index][free_column].dirty_bit = 0;
            cache[index][free_column].recency = global_counter;

            int block_start = add_accessed/block_size;
            block_start = block_start*block_size;

            for(int i = block_start; i < (block_start+block_size); i++){
                 cache[index][free_column].data[i-block_start] = memory[i];
            }
            cache[index][free_column].valid = 1;

            printf("load %04x miss ", add_accessed);
            for(int i = offset; i < (offset+access_size); i++){
                printf("%02x", cache[index][free_column].data[i] & 0xff);
            }
            printf("\n");
            global_counter++;
            continue;
        }

        ///10
        ///write_through = false load = false hit = true
        ///the block exists in the cache
        ///write what you need to the data in the cache block
        ///set dirty bit 1
        ///print appropriate information and update recency
        if(write_through == false && load == false && hit == true){
            int position = 0;
            ///char* written_value_string = (char *)malloc(64*sizeof(char *));
            char written_value_string[64];
            sprintf(written_value_string, "%x", written_value);

            position = 0;
            sprintf(written_value_string, "%x", written_value);
            for(int i = offset; i < (offset+access_size); i++){
                char* part = (char *)malloc(2*sizeof(char *));
                sscanf(&written_value_string[position], "%2s", part);
                cache[index][hit_column].data[i] = (int)strtol(part, NULL, 16);
                position += 2;
            }
            cache[index][hit_column].dirty_bit = 1;

            cache[index][hit_column].recency = global_counter;
            printf("store %04x hit\n", add_accessed);
            global_counter++;
            continue;
        }

        ///11
        ///write_through = false load = false hit = false
        ///1. check if set is full and evict if it is full
        ///2. find the block in mempory and bring it up to the cache
        ///3. for loop from start 
        ///address/block_size*block_size to get start index in memory
        ///for loop from start index to start index + block_size, maybe -1
        ///4. store into that block the data you need to store
        ///5. set dirty bit to one
        ///print appropriate information and update recency
        if(write_through == false && load == false && hit == false){
            if(set_full == true){
                int minimum = INT_MAX;
                for(int i = 0; i < ways; i++){
                    if(cache[index][i].recency < minimum){
                        minimum = cache[index][i].recency;
                        free_column = i;
                    }
                }
            }

            int enter_address = cache[index][free_column].tag*(num_rows*block_size) + index*block_size;

           if(cache[index][free_column].dirty_bit == 1 && cache[index][free_column].valid == 1){
               ///printf("-----Kicking out address: %x", enter_address);
               //printf("-----Kicking out value: ");
                for(int i =  enter_address; i < ( enter_address+block_size); i++){
                    memory[i] = cache[index][free_column].data[i-enter_address];
                    ///printf("%x", cache[index][free_column].data[i-enter_address]);
                }
            }
            ///printf("\n");

            cache[index][free_column].valid = 0;
            cache[index][free_column].tag = tag;
            cache[index][free_column].dirty_bit = 0;
            cache[index][free_column].recency = global_counter;

            int block_start = (add_accessed/block_size)*block_size;

            for(int i = block_start; i < (block_start+block_size); i++){
                 cache[index][free_column].data[i-block_start] = memory[i];
            }
            cache[index][free_column].valid = 1;

            int position = 0;
            char written_value_string[64];
            sprintf(written_value_string, "%x", written_value);

            position = 0;
            sprintf(written_value_string, "%x", written_value);
            for(int i = offset; i < (offset+access_size); i++){
                char* part = (char *)malloc(2*sizeof(char *));
                sscanf(&written_value_string[position], "%2s", part);
                cache[index][free_column].data[i] = (int)strtol(part, NULL, 16);
                position += 2;
            }
            cache[index][free_column].dirty_bit = 1;

            printf("store %04x miss ", add_accessed);
            printf("\n");
            global_counter++;
            continue;
        }

    }
    fclose(fp);
}
