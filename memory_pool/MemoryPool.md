# MemoryPool 内存池组件文档

## 一、设计原理

内存池通过**预先分配大块内存**，将其切分为多个小块（节点），用链表管理空闲节点。分配对象时直接从链表取节点，释放时将节点归还链表，**避免频繁向操作系统申请/释放内存**，极大提升性能并减少碎片。

- **分配/释放速度快**：O(1)操作，无需系统调用
- **减少内存碎片**：所有小对象来自同一大块内存
- **便于管理**：统一释放，便于检测内存泄漏

## 二、接口说明

### MemoryPool<T, BlockSize>

- `T`：对象类型
- `BlockSize`：每次扩展分配的对象数量（默认1024）

#### 主要方法

- `T* allocate(Args&&... args)`分配一个T类型对象，支持任意构造参数。**用法**：`T* obj = pool.allocate(args...);`
- `void deallocate(T* ptr)`释放一个对象，自动调用析构函数。**用法**：`pool.deallocate(obj);`
- 构造/析构
  构造时自动初始化，析构时释放所有内存。

#### 线程安全

- 内部使用 `std::mutex`保证多线程安全。

## 三、适用场景

- **高频分配/释放小对象**：如消息、事件、任务、网络包等
- **对象生命周期短**：如即时通讯消息、临时缓冲区
- **对性能要求高**：如服务器、游戏、数据库等

## 四、使用示例

```cpp
#include "MemoryPool.h"
#include <vector>
#include <string>
#include <iostream>

class Message {
public:
    Message(int id, const std::string& text) : id_(id), text_(text) {}
    void print() const { std::cout << id_ << ": " << text_ << std::endl; }
private:
    int id_;
    std::string text_;
};

int main() {
    MemoryPool<Message, 128> pool;
    std::vector<Message*> msgs;
    for (int i = 0; i < 10; ++i) {
        Message* m = pool.allocate(i, "hello");
        msgs.push_back(m);
        m->print();
    }
    for (auto m : msgs) pool.deallocate(m);
    return 0;
}
```

## 五、注意事项

- **不适合大对象**：大对象建议单独分配，避免浪费内存
- **对象需手动释放**：分配的对象必须用 `deallocate`释放，不能用 `delete`
- **不支持数组分配**：仅支持单个对象分配
- **析构时统一释放**：内存池析构时会释放所有内存块

## 六、扩展建议

- 可实现对象池（Object Pool），结合内存池与对象重用
- 可支持定制分配策略（如多线程局部池）
- 可增加调试功能（如分配统计、泄漏检测）
