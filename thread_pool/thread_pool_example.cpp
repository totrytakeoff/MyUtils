#include "thread_pool.h"
#include <iostream>
#include <string>
#include <chrono>

// 示例1：普通函数
int add(int a, int b) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return a + b;
}

// 示例2：类成员函数
class Calculator {
public:
    int multiply(int a, int b) const {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return a * b;
    }
    
    std::string format(int result) {
        return "Result: " + std::to_string(result);
    }
};

// 示例3：函数对象
struct Divider {
    int operator()(int a, int b) const {
        if (b == 0) throw std::runtime_error("Division by zero");
        return a / b;
    }
};

int main() {
    // 初始化线程池
    ThreadPool::GetInstance().Initialize(4);
    
    std::cout << "=== 线程池使用示例 ===" << std::endl;
    
    // 示例1：提交普通函数
    std::cout << "\n1. 提交普通函数:" << std::endl;
    auto future1 = ThreadPool::GetInstance().Enqueue(add, 10, 20);
    std::cout << "任务已提交，等待结果..." << std::endl;
    int result1 = future1.get(); // 阻塞等待结果
    std::cout << "add(10, 20) = " << result1 << std::endl;
    
    // 示例2：提交类成员函数
    std::cout << "\n2. 提交类成员函数:" << std::endl;
    Calculator calc;
    auto future2 = ThreadPool::GetInstance().Enqueue(&Calculator::multiply, &calc, 5, 6);
    int result2 = future2.get();
    std::cout << "multiply(5, 6) = " << result2 << std::endl;
    
    // 示例3：提交函数对象
    std::cout << "\n3. 提交函数对象:" << std::endl;
    Divider div;
    auto future3 = ThreadPool::GetInstance().Enqueue(div, 100, 5);
    int result3 = future3.get();
    std::cout << "divide(100, 5) = " << result3 << std::endl;
    
    // 示例4：提交 lambda 表达式
    std::cout << "\n4. 提交 lambda 表达式:" << std::endl;
    auto future4 = ThreadPool::GetInstance().Enqueue(
        [](const std::string& name, int age) -> std::string {
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            return "Hello " + name + ", you are " + std::to_string(age) + " years old";
        },
        "Alice", 25
    );
    std::string result4 = future4.get();
    std::cout << result4 << std::endl;
    
    // 示例5：批量提交任务
    std::cout << "\n5. 批量提交任务:" << std::endl;
    std::vector<std::future<int>> futures;
    for (int i = 0; i < 10; ++i) {
        futures.push_back(
            ThreadPool::GetInstance().Enqueue(
                [i]() -> int {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    return i * i;
                }
            )
        );
    }
    
    // 收集所有结果
    for (size_t i = 0; i < futures.size(); ++i) {
        std::cout << "Task " << i << " result: " << futures[i].get() << std::endl;
    }
    
    // 示例6：异常处理
    std::cout << "\n6. 异常处理:" << std::endl;
    try {
        auto future5 = ThreadPool::GetInstance().Enqueue(
            [](int a, int b) -> int {
                if (b == 0) throw std::runtime_error("Division by zero");
                return a / b;
            },
            10, 0
        );
        int result5 = future5.get(); // 这里会抛出异常
    } catch (const std::exception& e) {
        std::cout << "Caught exception: " << e.what() << std::endl;
    }
    
    // 示例7：演示完美转发
    std::cout << "\n7. 演示完美转发:" << std::endl;
    
    // 左值
    std::string left_value = "left value";
    auto future6 = ThreadPool::GetInstance().Enqueue(
        [](const std::string& s) -> std::string {
            return "Processed: " + s;
        },
        left_value  // 左值，会被转发为左值引用
    );
    
    // 右值
    auto future7 = ThreadPool::GetInstance().Enqueue(
        [](std::string s) -> std::string {
            return "Moved: " + s;
        },
        std::string("right value")  // 右值，会被转发为右值引用
    );
    
    std::cout << future6.get() << std::endl;
    std::cout << future7.get() << std::endl;
    
    // 关闭线程池
    ThreadPool::GetInstance().Shutdown();
    
    std::cout << "\n=== 所有示例完成 ===" << std::endl;
    return 0;
} 