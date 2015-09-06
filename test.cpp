#include <iostream>
#include <cassert>
#include <random>
#include <unordered_map>
#include <sstream>

#include "hashmap.h"

std::string
to_string(int value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

int main() {
    // 测试主体
    hashmap<std::string, std::string> dict(10);

    // 用作备份，与dict同步更新以便测试一致性
    std::unordered_map<std::string, std::string> bak;

    // 生成随机测试key的范围
    const int key_space = 50;

    std::default_random_engine gen(124);

    for (int i = 0; i < 1000; ++i) {
        // 随机生成K/V
        std::string key = to_string(gen() % key_space);
        std::string value = to_string(gen());

        // 检查一致性
        auto iter = bak.find(key);
        if (iter != bak.end()) {
            assert(dict.contain(key));
            assert(dict.get(key) == iter->second);
        } else {
            assert(!dict.contain(key));
            try {
                // key不存在必然抛异常
                dict.get(key);
                assert(false);
            } catch(std::logic_error) {
            }
            try {
                // key不存在必然抛异常
                dict.remove(key);
                assert(false);
            } catch(std::logic_error) {
            }
        }

        // 随机选择一个测试项
        switch (gen() % 2) {
        case 0: {
            // 测试插入K/V
            dict.set(key, value);
            bak.insert({key, value}).first->second = value;

            assert(dict.contain(key));
            assert(dict.get(key) == value);
        }
        break;
        case 1: {
            // 测试移除K/V
            auto iter = bak.find(key);
            if (iter != bak.end()) {
                auto ret = dict.remove(key) == iter->second;
                assert(ret);
                assert(!dict.contain(key));

                bak.erase(iter);
            }
        }
        break;
        }
    }

    // 最后再完整检测一次一致性
    for (auto &e : bak) {
        assert(dict.contain(e.first));
        assert(dict.get(e.first) == e.second);
    }

    std::cout << "OK, test succeed!" << std::endl;

    return 0;
}
