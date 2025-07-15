#include <iostream>
#include <vector>
#include "memory_pool.h"

// 一个简单的消息类，模拟即时通讯中的消息对象
class Message {
public:
    Message(int id, const std::string& text)
        : id_(id), text_(text) {
        std::cout << "Message constructed: " << id_ << std::endl;
    }
    ~Message() {
        std::cout << "Message destructed: " << id_ << std::endl;
    }
    void print() const {
        std::cout << "Message[" << id_ << "]: " << text_ << std::endl;
    }
private:
    int id_;
    std::string text_;
};

int main() {
    // 创建一个Message类型的内存池，每次扩展分配128个对象
    MemoryPool<Message, 128> pool;

    // 分配10个消息对象
    std::vector<Message*> messages;
    for (int i = 0; i < 10; ++i) {
        // 使用内存池分配对象，构造参数直接传递
        Message* msg = pool.allocate(i, "Hello, MemoryPool!");
        messages.push_back(msg);
        msg->print();
    }

    // 释放消息对象
    for (Message* msg : messages) {
        pool.deallocate(msg); // 必须用 pool.deallocate 释放，不能用 delete
    }

    // 再次分配，验证内存复用
    Message* msg = pool.allocate(100, "Reused memory!");
    msg->print();
    pool.deallocate(msg);

    return 0;
} 