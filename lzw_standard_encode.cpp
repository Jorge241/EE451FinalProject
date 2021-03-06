#include <unordered_map>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <ctime>
#include <cmath>
#include <sys/stat.h>
#include <pthread.h>

using namespace std;

#define PRINT 0
#define DEBUG 0

using std::cout;
using std::cerr;
using std::endl;

int num_thread;

pthread_attr_t* thread_attr;
pthread_t* threads;

struct thread_args_t {
    int id;
    std::string filename;
    long long start;
    long long block_size;
    std::unordered_map<std::string, long long>* table;
    long long code_begin;
    std::vector<long long> output_code;
};

thread_args_t* thread_args;



void* encode(void* args)
{ 
    thread_args_t* curr_arg = (thread_args_t*)args;
    std::ifstream ifile(curr_arg->filename);
    ifile.seekg(curr_arg->start);

    #if DEBUG
    printf("Thread %d starts running with the following params\nstart = %lld\nblock_size = %lld\n", curr_arg->id, curr_arg->start, curr_arg->block_size);
    #endif


    std::unordered_map<std::string, int> table; 
    for (int i = 0; i <= 255; i++) { 
        std::string ch = ""; 
        ch += char(i); 
        table[ch] = i; 
    } 
  
    std::string p = "", c = ""; 
    p += (char)ifile.get();

    #if DEBUG
        printf("Thread %d: Initial Read: %s\n", curr_arg->id, p.c_str());
    #endif

    int code = 256; 
    std::vector<int> output_code; 

    char ch;
    unsigned long long cnt = 1;
    while (!ifile.fail()) { 
        // if (i != s1.length() - 1) 
        //     c += s1[i + 1]; 
        ch = ifile.get();
        if (!ifile.eof()) {
            c += ch;
        }
        if (table.find(p + c) != table.end()) { 
            p = p + c; 
        } 
        else { 
            #if DEBUG
            std::cout << p << "\t" << table[p]  << "\t\t" 
                 << p + c << "\t" << code << std::endl; 
            #endif
            curr_arg->output_code.push_back(table[p]); 
            
            table[p + c] = code; 
            code++; 
            p = c; 
        } 
        c = ""; 
        cnt++;
        if (curr_arg->block_size > 0 && cnt >= (unsigned long long)curr_arg->block_size) {
            break;
        }
    }

    #if DEBUG
    std::cout << p << "\t" << curr_arg->table->at(p) << std::endl; 
    #endif
    curr_arg->output_code.push_back(table[p]);
    return NULL;
}

long long get_file_size(std::string file_name) {
    struct stat stat_buf;
    int rc = stat(file_name.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

    
int main(int argc, char** argv) {
    if (argc != 4) {
        cerr << "usage: ./lzw_parallel_encode num_thread input_file output_file" << endl;
        return 1;
    }

    num_thread = atoi(argv[1]);
    if (num_thread < 1) {
        cerr << "Unsupported number of threads" << endl;
        return 1;
    }

    std::string input_file_name = argv[2];
    std::string output_file_name = argv[3];


    long long input_file_size = get_file_size(input_file_name);
    if (input_file_size == -1) {
        cerr << "Error reading input file" << endl;
        return 1;
    }

    threads = new pthread_t[num_thread];
    thread_args = new thread_args_t[num_thread];

    long long block_size = input_file_size / num_thread;
    for (int i = 0; i < num_thread; ++i) {
        thread_args[i].id = i+1;
        thread_args[i].filename = input_file_name;
        thread_args[i].start = i*block_size;
        thread_args[i].block_size = i == num_thread-1 ? -1 : block_size; // -1 indicates reading until the end
    }

    struct timespec start, stop;
    double time = 0;
    if( clock_gettime(CLOCK_REALTIME, &start) == -1) { perror("clock gettime");}


    for(int i = 0; i < num_thread; ++i)
        {
            pthread_create(&threads[i], thread_attr, encode, (void*)&thread_args[i]);
        }

    for(int i = 0; i < num_thread; ++i)
        {
            pthread_join(threads[i], NULL);
        }

    if( clock_gettime( CLOCK_REALTIME, &stop) == -1 ) { perror("clock gettime");}       
    time = (stop.tv_sec - start.tv_sec)+ (double)(stop.tv_nsec - start.tv_nsec)/1e9;
    std::cout << "Encoding time = " << time << " sec " <<std::endl;


    // Gather output codes
    std::ofstream ofile(output_file_name);
    long long max_code = 0;
    unsigned long long total_size = 0;
    // Update: Add a block header in the beginning
    ofile << -2 << " " << num_thread << endl;
    for (int i = 0; i < num_thread; i++) {
        total_size += thread_args[i].output_code.size();
        // Update: Add a file breaker for decode
        ofile << -1 << " " << thread_args[i].output_code.size() << endl;
        for (size_t j = 0; j < thread_args[i].output_code.size(); j++) {
            ofile << thread_args[i].output_code[j] << endl;
            max_code = thread_args[i].output_code[j] > max_code ? thread_args[i].output_code[j] : max_code;
        }
        
    }
    int num_bits = (int)ceil(log2(max_code));
    unsigned long long compressed_file_size = total_size * num_bits / 8;

    #if PRINT
    std::cout << "Largest code assigned: " << max_code << endl;
    std::cout << "Number of bits to store each code: " << num_bits << endl;
    std::cout << "Number of output codes: " << total_size << endl;
    std::cout << "Estimated best-case compressed size: " << compressed_file_size << " bytes" << endl;
    #endif
    std::cout << "Estimated Compression Rate = " << (double)input_file_size / compressed_file_size << endl;


    delete thread_args[0].table;
    delete [] threads;
    delete [] thread_args;
    delete thread_attr;
    return 0;
}