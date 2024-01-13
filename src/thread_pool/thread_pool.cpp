#include <queue>
#include <functional>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

class TaskQueue {
public:
    std::queue<std::function<void()>> tasks;
    std::mutex mutex;
    std::atomic<uint32_t> remainingTasks = 0;

    template<typename T>
    void addTask(T&& callback) {
        std::lock_guard<std::mutex> lock_guard{mutex};
        tasks.push(std::forward<T>(callback));
        remainingTasks++;
    }

    static void pause() {
        std::this_thread::yield();
    }

    void waitTaskCompletion() const {
        while(remainingTasks > 0) {
            pause();
        }
    }

    void done() {
        remainingTasks--;
    }

    void getTask(std::function<void()>& target_callback) {
        {
            std::lock_guard<std::mutex> lock_guard{mutex};
            if(tasks.empty()) {
                return;
            }

            target_callback = std::move(tasks.front());
            tasks.pop();
        }
    }
};

class TaskWorker {
public:
    uint32_t id = 0;
    std::thread thread;
    std::function<void()> task = nullptr;
    bool isRunning = true;
    TaskQueue* queue = nullptr;

    TaskWorker() = default;

    TaskWorker(TaskQueue& queue_, uint32_t id_) : id{id_}, queue(&queue_){
        thread = std::thread([this]() {
            run();
        });
    }

    void run() {
        while(isRunning) {
            queue->getTask(task);
            if(task == nullptr) {
                TaskQueue::pause();
            } else {
                task();
                queue->done();
                task = nullptr;
            }
        }
    }

    void stop() {
        isRunning = false;
        thread.join();
    }

};

class ThreadPool {
public:
    uint32_t  size = 0;
    TaskQueue queue;
    std::vector<TaskWorker> task_workers;

    explicit
    ThreadPool(uint32_t size_) : size(size_) {
        task_workers.reserve(size_);

        for(uint32_t i = size; i > 0; i--) {
            task_workers.emplace_back(queue, static_cast<uint32_t>(task_workers.size()));
        }
    }

    virtual  ~ThreadPool() {
        for(TaskWorker& worker : task_workers) {
            worker.stop();
        }
    }

    template<typename T>
    void addTask(T&& callback) {
        queue.addTask((std::forward<T>(callback)));
    }

    void waitTaskCompletion() const {
        queue.waitTaskCompletion();
    }

    template<typename T>
    void dispatch(uint32_t element_count, T&& callback) {
        const uint32_t batch_size = element_count / size;

        for(uint32_t i{0}; i < size; i++) {
            const uint32_t start = batch_size * i;
            const uint32_t end = start + batch_size;
            addTask([start, end, &callback](){ callback(start, end); });
        }

        if(batch_size * size < element_count) {
            const uint32_t start = batch_size * size;
            callback(start, element_count);
        }

        waitTaskCompletion();
    }
};