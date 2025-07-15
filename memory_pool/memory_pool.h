#pragma once
#include <cstddef>
#include <vector>
#include <cassert>
#include <new>      // For placement new
#include <mutex>


/******************************************************************************
 *
 * @file       memory_pool.h
 * @brief      通用对象内存池模板类
 *
 * @author     myself
 * @date       2025/07/15
 * 
 *****************************************************************************/



// 通用对象内存池，适用于频繁分配/释放的小对象
// T: 对象类型，BlockSize: 每次扩展分配的对象数量
// 线程安全，适合多线程环境
// 用法：MemoryPool<MyClass, 128> pool;
//       MyClass* obj = pool.allocate(args...);
//       pool.deallocate(obj);
template<typename T, size_t BlockSize = 1024>
class MemoryPool {
public:
    // 构造函数
    MemoryPool() noexcept {
        expandPool();
    }

    // 禁止拷贝和赋值
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    // 分配一个对象，支持任意构造参数
    template<typename... Args>
    T* allocate(Args&&... args) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!freeList_) {
            expandPool();
        }
        // 从空闲链表取出一个节点
        Node* node = freeList_;
        freeList_ = freeList_->next;
        // 在分配的内存上构造对象（placement new）
        return new (&(node->storage)) T(std::forward<Args>(args)...);
    }

    // 释放一个对象
    void deallocate(T* ptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (ptr) {
            // 显式调用析构函数
            ptr->~T();
            // 将内存块放回空闲链表
            Node* node = reinterpret_cast<Node*>(ptr);
            node->next = freeList_;
            freeList_ = node;
        }
    }

    // 析构函数，释放所有内存块
    ~MemoryPool() {
        for (void* block : blocks_) {
            ::operator delete(block);
        }
    }

private:
    // 内部节点结构，存储对象或指向下一个空闲节点
    union Node {
        alignas(T) char storage[sizeof(T)];
        Node* next;
    };

    // 扩展内存池，分配一大块内存并分割成BlockSize个节点
    void expandPool() {
        // 分配一大块内存
        void* block = ::operator new(sizeof(Node) * BlockSize);
        blocks_.push_back(block);
        Node* nodes = static_cast<Node*>(block);
        // 构建空闲链表
        for (size_t i = 0; i < BlockSize - 1; ++i) {
            nodes[i].next = &nodes[i + 1];
        }
        nodes[BlockSize - 1].next = nullptr;
        freeList_ = nodes;
    }

    Node* freeList_ = nullptr;           // 空闲链表头
    std::vector<void*> blocks_;          // 所有分配的大块内存
    std::mutex mutex_;                   // 线程安全
}; 