# Counter
Counter was our Operative System project for the second year of Bachelor in Computer Science in University of Trento (Italy). The main focus of this project is to count the occurrences of chars inside one or more files (normal ASCII). Our idea was to make all as extensible and modular as possible. There are several components, every of them has a specific task. A little brief:

* **Counter**: the counter spawns the reporter and the analyzer.
* [**Reporter**](./src/reporter/README.md): the reporter creates the terminal user interface and communicates with the analyzer.
* **Analyzer**: the analyzer takes all inputs from the user and finds all files given a directory. Then, while files are being discovered,  analyzer sends founded ones to managers.
* **Manager**: the manager takes files and split them in several works. A work starts from a specific point of the file and ends in another. When works are ready they are sent to workers.
* **Worker**: the worker takes the file, the start point end the end point. After that he reads the portion of the file

Obs: With a small amount of work is possible to change the lowest component of the system, the worker, to allow the system to handle different types of problems, not only counting occurrences

## Goal
The goal of this project is to learn most of C system calls and to take confident with GNU/Linux environment.

## Implementation choices
There are several implementations choices inside all files in compontens' folder. Here are listed some common choices.

### Memory
Inside all code except worker when we allocate some memory on the heap we always control if the allocation gone well. In the most case if the malloc fail we closed the program because we think that if a user has saturated the RAM may prefer that some program unlock it to make computer more usable.
The only component that checks the amount of free memory is the worker. Before reading from a file he checks if there is a enough free space in memory in order to read his work amount. If not he allocates the 50% of the free memory and read multiples times from the file moving the cursor. This check is made only in worker because it is the only component of that do a high memory usage task. Nowadays computers have enough memory to support other component and the check seem, for us, an useless overhead. We made some test to prove our decision:

| Number of files | size   |
|-----------------|--------|
|0                |2.83 mb |
|1                |3.03 mb |
|10               |3.08 mb |
|100              |3.93 mb |
|1000             |12.13 mb|
|10000            |93.39 mb|

The files were empty and all inside the same folder but we think that, also with different configurations, the memory usage is similar.

## Team
![Federico Izzo](./team/FedericoIzzo.png)
![Simone Alghisi](./team/SimoneAlghisi.png)
![Emanuele Beozzo](./team/EmanueleBeozzo.png)
![Samuele Bortolotti](./team/SamueleBortolotti.png)
