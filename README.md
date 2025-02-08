# not-your-usual-encoder

## sequential RLE
I first implemented nyuenc as a single-threaded program. The encoder reads from one or more files specified as command-line arguments and writes to STDOUT. Thus, the typical usage of nyuenc would use shell redirection to write the encoded output to a file.

Note: use xxd to inspect a binary file since not all characters are printable. xxd dumps the file content in hexadecimal.

For example, let’s encode the aforementioned file.
```
$ echo -n "aaaaaabbbbbbbbba" > file.txt
$ xxd file.txt
0000000: 6161 6161 6161 6262 6262 6262 6262 6261  aaaaaabbbbbbbbba
$ ./nyuenc file.txt > file.enc
$ xxd file.enc
0000000: 6106 6209 6101                           a.b.a.
```
If multiple files are passed to nyuenc, they will be concatenated and encoded into a single compressed output. For example:

```
$ echo -n "aaaaaabbbbbbbbba" > file.txt
$ xxd file.txt
0000000: 6161 6161 6161 6262 6262 6262 6262 6261  aaaaaabbbbbbbbba
$ ./nyuenc file.txt file.txt > file2.enc
$ xxd file2.enc
0000000: 6106 6209 6107 6209 6101                 a.b.a.b.a.
```
Note that the last a in the first file and the leading a’s in the second file are merged.

For simplicity, there are no more than 100 files, and the total size of all files is no more than 1GB.

## parallel RLE
Next, I parallelized the encoding using POSIX threads. In particular, I implemented a thread pool for executing encoding tasks.

I used mutexes, condition variables, or semaphores to realize proper synchronization among threads. The code is free of race conditions without any use of busy waiting or sleep(), usleep() or nanosleep().

It also takes an optional command-line option -j jobs, which specifies the number of worker threads. (If no such option is provided, it runs sequentially.)

For example:

```
$ time ./nyuenc file.txt > /dev/null
real    0m0.527s
user    0m0.475s
sys     0m0.233s
$ time ./nyuenc -j 3 file.txt > /dev/null
real    0m0.191s
user    0m0.443s
sys     0m0.179s
```
You can see the difference in running time between the sequential version and the parallel version. (Note: redirecting to /dev/null discards all output, so the time won’t be affected by I/O.)



At the beginning of the program, I created a pool of worker threads. The number of worker threads is specified by the command-line argument -j jobs.

The main thread divided the input data logically into fixed-size 4KB (i.e., 4,096-byte) chunks and submit the tasks to the task queue, where each task would encode a chunk. Whenever a worker thread becomes available, it executes the next task in the task queue.

For simplicity, the task queue is unbounded. In other words, all tasks cab be submitted at once without being blocked.

After submitting all tasks, the main thread should collect the results (the yellow circles in the figure above) and write them to STDOUT. Note that you may need to stitch the chunk boundaries. For example, if the previous chunk ends with aaaaa, and the next chunk starts with aaa, instead of writing a5a3, you should write a8.

Threads are synchronized properly so that there are no deadlocks or race conditions.




I have included some text files for tests. Feel free to try them :)
