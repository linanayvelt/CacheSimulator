# CacheSimulator
Created a simulation representing a computer's memory hierarchy. The simulation contains an L1 cache and memory. 

The machine being simulated is big-endian and byte-addressed. Addresses are 16-bits. Cache misses are satisfied by main memory. At the beginning of the simulation all blocks start out as invalid in the L1 cache and all memory is set to zero.

Limitations:
1. All cache size, associativity, block size, and access size inputs are powers of 2.
2. Size of cache won't exceed 16KB, size of one block won't exceed 64B.
3. Each access must fit within a single block. 
