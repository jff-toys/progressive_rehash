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
    // ��������
    hashmap<std::string, std::string> dict(10);

    // �������ݣ���dictͬ�������Ա����һ����
    std::unordered_map<std::string, std::string> bak;

    // �����������key�ķ�Χ
    const int key_space = 50;

    std::default_random_engine gen(124);

    for (int i = 0; i < 1000; ++i) {
        // �������K/V
        std::string key = to_string(gen() % key_space);
        std::string value = to_string(gen());

        // ���һ����
        auto iter = bak.find(key);
        if (iter != bak.end()) {
            assert(dict.contain(key));
            assert(dict.get(key) == iter->second);
        } else {
            assert(!dict.contain(key));
            try {
                // key�����ڱ�Ȼ���쳣
                dict.get(key);
                assert(false);
            } catch(std::logic_error) {
            }
            try {
                // key�����ڱ�Ȼ���쳣
                dict.remove(key);
                assert(false);
            } catch(std::logic_error) {
            }
        }

        // ���ѡ��һ��������
        switch (gen() % 2) {
        case 0: {
            // ���Բ���K/V
            dict.set(key, value);
            bak.insert({key, value}).first->second = value;

            assert(dict.contain(key));
            assert(dict.get(key) == value);
        }
        break;
        case 1: {
            // �����Ƴ�K/V
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

    // ������������һ��һ����
    for (auto &e : bak) {
        assert(dict.contain(e.first));
        assert(dict.get(e.first) == e.second);
    }

    std::cout << "OK, test succeed!" << std::endl;

    return 0;
}
