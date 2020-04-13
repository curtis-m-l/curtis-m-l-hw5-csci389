#include <iostream>
#include <cassert>
#include <time.h>
#include <variant>
#include "cache.hh"
#include "evictor.hh"

// CONSTANTS
int REQUESTCOUNT = 10000;
int GETPROB = 67;
int SETPROB = 98;   //SETPROB is 100 - (desired prob). It's the top of the range.
                    //Don't need a DELPROB, it's just in an else statement.
std::string HOST = "127.0.0.1";
std::string PORT = "3618";

// global_variables
int total_gets = 0;
int get_hits = 0;

int new_key_ascii = 65; //Whenever we want to generate a new key, we use and update this value.

std::vector<std::string> keys_in_use;

/*

*/

void 
cache_set(Cache& items, key_type name, Cache::val_type data, Cache::size_type size)
{
    std::cout << "Attempting to add item of size " << size << " ...\n";
    /* Create an item with key 'name', value 'data', and size 'size'. Add it to the cache. */
    Cache::val_type val = data;
    items.set(name, val, size);
}

void 
cache_get(Cache& items, key_type key)
{
    std::cout << "Getting item " << key << " from cache...\n";
    total_gets += 1;
    Cache::size_type got_item_size = 0;
    Cache::val_type got_item = items.get(key, got_item_size);
    if (got_item != nullptr) {
        get_hits += 1;
        std::cout << key << " gotten successfully\n";
    }
    else {
        std::cout << "Falied to get " << key << "\n";
    }
}

// 
void 
cache_del(Cache& items, key_type key)
{
    bool delete_success = items.del(key);
    if (delete_success) {
        std::cout << "Deleted " << key << " from the cache.\n";
    }
    else {
        std::cout << "Invalid delete on item " << key << "\n";
    }
}

// Helper for generate_request()
bool 
useRealKey(int fail_prob)
{
    auto use_real_key_prob = rand() % 100;
    if (use_real_key_prob > fail_prob) {
        return true;
    }
    else {
        return false;
    }
}

// Helper for generate_request(), specifically when setting
Cache::val_type
generate_random_data(int length){
    std::string data = "";
    while(length > 0) {                          //TODO: Check for null-termination
        char char_ascii = rand() % 26 + 97;     //Outputs something between 'a' and 'z'.
        data = data + char_ascii;
        length = length - 1;
    }
    return data.c_str();
}

// Helper for generate_request()
key_type
select_key(){
    auto rand_index = rand() % keys_in_use.size();
    key_type active_key = keys_in_use[rand_index];
    return active_key;
}

// Generate a random request for the cache
std::vector<std::variant<std::string, int>> 
generate_request()
{
    std::vector<std::variant<std::string, int>> random_request;
    srand(time(NULL));
    auto op_prob = rand() % 100;
    /*
    Probability Details:
    Total range: 0-99
    0 <-> (GETPROB - 1): Get
    GETPROB <-> (SETPROB - 1): Delete
    SETPROB <-> 99: Set
    */
    if (op_prob < GETPROB) {
        // Get
        auto fail_prob = 0;
        if (useRealKey(fail_prob)){
            key_type active_key = select_key();

            random_request.push_back("get");
            random_request.push_back(active_key);

            return random_request;
        }
        else {
            // Return a request to get a dummy key (which will never exist in the cache)
            auto dummy_key = "-A";

            random_request.push_back("get");
            random_request.push_back(dummy_key);

            return random_request;
        }
    }
    else if (op_prob > SETPROB) {       // Note that this is >, not <
        // Set
        int data_length = 0;
        auto size_prob = rand() % 100;
        if (size_prob < 85) {
            data_length = rand() % 6 + 2;
        }
        else if (size_prob < 98) {
            data_length = rand() % 35 + 10;
        }
        else {
            data_length = rand() % 15 + 100;
        }
        Cache::val_type rand_data = generate_random_data (data_length);
        

        auto fail_prob = 2;
        if (useRealKey(fail_prob)) {
            // Select a key from our list to set the value of (frequency distribution?)
            key_type active_key = select_key();
            
            random_request.push_back("set");
            random_request.push_back(rand_data);
            random_request.push_back(active_key);
            random_request.push_back(data_length);

            return random_request;
        }
        else {
            // Return a request to set a brand new key, and add that key to our list.
            char ascii_char = new_key_ascii;
            new_key_ascii = new_key_ascii + 1;    // We've used this key now, so update the ascii code
            key_type new_key;
            new_key = ascii_char;

            random_request.push_back("set");
            random_request.push_back(new_key);
            random_request.push_back(rand_data);
            random_request.push_back(data_length);

            keys_in_use.push_back(new_key);
            
            return random_request;
        }
    }
    else {
        // Delete
        auto fail_prob = 40;
        if (useRealKey(fail_prob)) {
            // Select a key from our list to delete (frequency distribution?)
            key_type active_key = select_key();

            random_request.push_back("delete");
            random_request.push_back(active_key);

            return random_request;
        }
        else {
            // Return a request to delete a non-existant key
            auto dummy_key = "-A";

            random_request.push_back("delete");
            random_request.push_back(dummy_key);

            return random_request;
        }
    }
}

void
cache_warmup(Cache items){
    //Set a bunch of values to the cache, to fill it up
    int data_length;
    for(int i = 0; i < 100; i++) {
        auto size_prob = rand() % 100;
        if (size_prob < 85) {
            data_length = rand() % 6 + 2;
        }
        else if (size_prob < 98) {
            data_length = rand() % 35 + 10;
        }
        else {
            data_length = rand() % 15 + 100;
        }
        Cache::val_type rand_data = generate_random_data (data_length);

        char ascii_char = new_key_ascii;
        new_key_ascii += 1;
        key_type new_key;
        new_key = ascii_char;
        cache_set(items, new_key, rand_data, data_length);
    }
    return;
}

int main() 
{
    Cache items(HOST, PORT);
    for (int i = 0; i < REQUESTCOUNT; i++) {

        std::vector<std::variant<std::string, int>> new_req = generate_request();

        if (std::get<std::string>(new_req[0]) == "set") {
            assert(new_req.size() == 4 && "'set' request generated with the wrong number of elements!\n");
            // Vector contents: {"set", key_type key, val_type data, size_type size}
            Cache::val_type data = std::get<std::string>(new_req[1]).c_str();
            key_type key = std::get<std::string>(new_req[2]);
            Cache::size_type size = std::get<int>(new_req[3]);
            cache_set(items, key, data, size);
        }
        else if (std::get<std::string>(new_req[0]) == "get") {
            assert(new_req.size() == 2 && "'get' request generated with the wrong number of elements!\n");
            // Vector contents: {"get", key_type key}

            key_type key = std::get<std::string>(new_req[1]);
            cache_get(items, key);
        }
        else if (std::get<std::string>(new_req[0]) == "delete") {
            assert(new_req.size() == 2 && "'del' request generated with the wrong number of elements!\n");
            // Vector contents: {"get", key_type key}

            key_type key = std::get<std::string>(new_req[1]);
            cache_del(items, key);
        }
    }
    int get_hit_ratio = get_hits / total_gets;
    std::cout << "get() hit ratio: " << get_hit_ratio << "\n";
    return 0;
}