#pragma once

#include <thread>
#include <mutex>
#include <functional>
#include <semaphore>
#include <atomic>
#include <deque>
#include <type_traits>
#include <variant>
#include <condition_variable>
#include <iostream>

enum ThreadType {
    THREAD_TYPE_NULL,
    THREAD_TYPE_ONE_IN_ONE_OUT, // Load is run once and then discarded
    THREAD_TYPE_REPEAT_LOAD, // Load is run repeatedly until a new load is given or thread is ended TODO
    THREAD_TYPE_QUEUE // run Load on every element in a thread_obj<std::deque> given by reference
};

/// @brief allows an object to be thread safe (maybe lmao)
/// @tparam _T type of the object to be accessed
/// @param .access() returns a guard_access<_T> type structure allowing modification of object during its existence
template <typename _T>
class thread_obj {
protected:

    _T object;
    std::mutex obj_mutex;

public:
    // constructors allow for copy and move operations with _T objects
    thread_obj() = default;
    explicit thread_obj(const _T& obj) : object(obj) {}
    explicit thread_obj(_T&& obj) : object(std::move(obj)) {}

    // delete move and copy constructors and assignments to pervent shared ownership of mutex
    thread_obj(thread_obj&&) = delete;
    thread_obj& operator=(thread_obj&&) = delete;
    thread_obj(const thread_obj&) = delete;
    thread_obj& operator=(const thread_obj&) = delete;

    // behaves like a pointer to object
    class access_proxy {
    private:
        // will be locked for entire lifetime of structure access_proxy
        std::lock_guard<std::mutex> lock;
        _T& object_ref;

    public:
        // lock parent mutex during construction and assign reference 
        access_proxy(std::mutex& mtx, _T& obj)
            : lock(mtx), object_ref(obj) {}

        // overload operators to access object_ref and its members
        _T& operator*() { return object_ref; }
        _T* operator->() { return &object_ref; }
        _T* operator&() { return &object_ref; }
    };

    // same as access_proxy buit with unique_lock slightly more overhead but used for condition variable access
    class unique_access_proxy {
    private:
        // will be locked for entire lifetime of structure access_proxy
        std::unique_lock<std::mutex> lock;
        _T& object_ref;

    public:
        // lock parent mutex during construction and assign reference 
        unique_access_proxy(std::mutex& mtx, _T& obj)
            : lock(mtx), object_ref(obj) {}

        // overload operators to access object_ref and its members
        _T& operator*() { return object_ref; }
        _T* operator->() { return &object_ref; }
        _T* operator&() { return &object_ref; }

        // give access to lock
        std::unique_lock<std::mutex>& get_lock() { return lock; }
    };

    // allow scoped safe read and write access to object
    access_proxy access() { return access_proxy(obj_mutex, object); }
    unique_access_proxy unique_access() { return unique_access_proxy(obj_mutex, object); }
};

/// @brief better readability for locking structure
/// @tparam _T template type of thread_obj called 
template <typename _T>
using guard_access = typename thread_obj<_T>::access_proxy;

/*
* type evaluation helper functions
*/
template <typename _cond, typename _is_void, typename _not_void>
consteval auto get_type() {
    if constexpr (std::is_void_v<_cond>) {
        return std::type_identity<_is_void>{};
    } else {
        return std::type_identity<_not_void>{};
    }
}

template <typename _inner>
consteval auto func_void_test() {
    if constexpr (std::is_void_v<_inner>) {
        return std::type_identity<std::function<void()>>{};
    } else {
        return std::type_identity<std::function<void(_inner)>>{};
    }
}

/*
* concepts
*/
template <typename _T>
concept is_void = std::is_void_v<_T>;

template <typename _T, typename _Q>
concept is_thread_save_queue = !std::is_void_v<_Q> && std::is_same_v<_T, thread_obj<std::deque<_Q>>>;

/*
* thread wrapper object
*/
template <typename _Q = void>
class EngineThread {

    // thread that will work on the load
    std::thread worker;

    // queue type and function type set for case _Q = void
    using queue_type = typename decltype(get_type<_Q, std::monostate, thread_obj<std::deque<_Q>>>())::type;
    using _not_void_func_type = typename decltype(func_void_test<_Q>())::type;
    using func_type = typename decltype(get_type<_Q, std::function<void()>, _not_void_func_type>())::type;
    
    // reference of object holding data for queueing threadwork
    queue_type* const queue_data;
    std::condition_variable queue_cv;

    // only std::move to and read load_read while mutex is locked to ensure no data race
    func_type load_write;
    func_type load_read;
    std::mutex load_mutex;
    
    // true when Thread is no longer used
    std::atomic<bool> end;

    // semaphores for synchronizing thread
    std::binary_semaphore work_ready;
    std::binary_semaphore load_empty;

    // function run by thread initiated in the constructor
    void start_11_work();
    void start_repeat_work();
    void start_queued_work() requires is_thread_save_queue<queue_type, _Q>;

    inline void run_read() requires is_void<_Q> { if (!load_read) return; load_read(); };
    inline void run_read() requires is_thread_save_queue<queue_type, _Q> {
        if (!load_read) return;
        
        auto p_data = queue_data->access(); // getting access to data

        load_read(p_data->front()); // calling current load on data
    
        p_data->pop_front(); // removing data
    };

public:

    // join worker thread with main
    void end_work();

    // write lambda to load_write
    void next_load(func_type new_load);

    // notify condition variable
    void notify() { queue_cv.notify_one(); }

    //constructors/destructor
    EngineThread(ThreadType thread_type = THREAD_TYPE_NULL, queue_type* pQueue = nullptr);
    ~EngineThread() { end_work(); };
};

/*
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* EngineThread definitions
*/

/*
* Thread constructor initiating the thread for all ThreadType
*/
template <typename _Q>
EngineThread<_Q>::EngineThread(ThreadType thread_type, queue_type* pQueue) : queue_data(pQueue), end(false), work_ready(0), load_empty(1) {

    switch (thread_type) {
    case THREAD_TYPE_ONE_IN_ONE_OUT:

        worker = std::thread(&EngineThread<_Q>::start_11_work, this);
        break;
        
    case THREAD_TYPE_REPEAT_LOAD:

        worker = std::thread(&EngineThread<_Q>::start_repeat_work, this);
        break;

    case THREAD_TYPE_QUEUE:
        
        // only call worker if function is well formed
        if constexpr (is_thread_save_queue<queue_type, _Q>) {
            worker = std::thread(&EngineThread<_Q>::start_queued_work, this);
        }
        break;

    case THREAD_TYPE_NULL:

        end.store(true);
        break;
    }

}

/*
* Tell worker thread to finish its work and wait current thread until it is done
*/
template <typename _Q>
void EngineThread<_Q>::end_work() {
    
    // ensure no double ending of thread
    if (end.load()) {
        return;
    }

    { // safe load access scope

        // lock load to ensure no invalid load writing
        const std::lock_guard<std::mutex> lock(load_mutex);

        // set end to exit thread loop
        end.store(true);

        // notify all threads waiting on the condition variable if the queue is empty
        queue_cv.notify_all();

        // let worker thread continue and wait for it to join
        work_ready.release();
    } // safe load access scope

    // join thread
    if (worker.joinable()) {
        worker.join();
    }

}

/*
* Run loop that multithreads and runs lambdas written to Thread::load
*/
template <typename _Q>
void EngineThread<_Q>::start_11_work() {

    while (!end.load()) {
        // wait until load has been written
        work_ready.acquire();
        
        if (!load_write) {
            return;
        }
        
        {
            // ensure load write is not written to during move operation
            const std::lock_guard<std::mutex> lock(load_mutex);

            // move load_write to load_read
            load_read = std::move(load_write);
            load_write = nullptr;

            // allow more load writes
            load_empty.release();
        }

        // run load lambda
        run_read();
    }
}

/*
* Run loop over same load until load is changed
*/
template <typename _Q>
void EngineThread<_Q>::start_repeat_work() {

    while (!end.load()) {

        // pause thread if no load is present
        if (!load_read) {
            work_ready.acquire();
        }

        // test if new load has been acquired
        if (load_write) {

            // ensure no other thread accesses load_write during copy operation
            const std::lock_guard<std::mutex> lock(load_mutex);

            // move load_write to load_read
            load_read = std::move(load_write);
            load_write = nullptr;

            // tell next queue_load call that it can continue
            load_empty.release();
        }

        // run lambda
        run_read();
    }
}

/*
* Run load for every datapacket in queue_data
*/
template <typename _Q>
void EngineThread<_Q>::start_queued_work() requires is_thread_save_queue<queue_type, _Q> {
    
    while (!end.load()) {

        // pause thread if no load is present
        if (!load_read) {
            work_ready.acquire();
        }

        // test if new load has been acquired
        if (load_write) {

            // ensure no other thread accesses load_write during copy operation
            const std::lock_guard<std::mutex> lock(load_mutex);

            // move load_write to load_read
            load_read = std::move(load_write);
            load_write = nullptr;

            // tell next queue_load call that it can continue
            load_empty.release();
        }

        // wait until data is in queue_data
        { // safe queue_data access scope
            
            auto p_data = queue_data->unique_access();

            // wait until queue_data is not empty
            queue_cv.wait(p_data.get_lock(), [this, &data = *p_data] { return end.load() || (!data.empty()); });

            // end thread if end = true and no more data to process
            if (end.load() && p_data->empty()) {
                return;
            }
        } // safe queue_data access scope

        // run the load on the first element of the queue and lock queue_data
        run_read();
    }
}

/*
* safely write to load_write and call the thread to work with .release()
*/
template <typename _Q>
void  EngineThread<_Q>::next_load(func_type new_load) {

    if (!new_load) {
        std::cerr << "CANNOT RUN AN EMPTY FUNCTION\n";
        return;
    }
    // main waits for thread to finish moving load_write to load_read
    load_empty.acquire(); // should guarantee that queue_load/next_load is only called once before every load_write to load_read move

    {
        // ensure load write is not moved during writing
        const std::lock_guard<std::mutex> lock(load_mutex);

        // move new_load to load_write
        load_write = std::move(new_load);
    }

    // tells thread it has work
    work_ready.release();
}

/*
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/