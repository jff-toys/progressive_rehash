#pragma once

#include <cstdint>
#include <list>
#include <vector>
#include <functional>
#include <stdexcept>

template <typename K, typename V>
class hashmap {
public:
    /**
     *  @param  capacity    初始容量
     *  @param  load_factor 负载因子，默认0.75
     *  @note   当实际元素数量超过capacity * load_factor时将会进行rehash
     */
    hashmap(int capacity, double load_factor = 0.75);

    virtual ~hashmap();

    /**
     *  检测一个key是否存在
     *  @param  key
     *  @return 存在则true，否则false
     */
    bool contain(const K &key);

    /**
     *  获取值
     *  @param  key
     *  @return key对应的value，如果不存在则抛出logic_error异常
     */
    V& get(const K &key);

    /**
     *  设置值
     *  @param  key
     *  @param  value
     *  @note   内部保存的是key和value的一个备份
     */
    void set(const K &key, const V &value);

    /**
     *  移除一组键值对
     *  @param  key
     *  @return key对应的值，如果不存在则抛出logic_error异常
     */
    V remove(const K &key);

private:
    /** 内部用于存储一个键值对 */
    struct entry {
        K key;
        V value;
    };

    /** 内部数组（slots）的类型 */
    typedef std::vector<std::list<entry>> slots_t;

private:
    /** 判断key是否属于旧数组 */
    bool is_key_belongs_old(const K &key);

    /**
     *  从一个内部数组中获取键值对
     *  @param  slots   一个内部数组
     *  @param  key
     *  @return 键值对，不存在则抛出logic_error异常
     */
    entry& get_from_slots(slots_t &slots, const K &key);

    void move_into_slots(slots_t &slots, entry &&e);

    /**
     *  从数组中删除一个键值对
     *  @param  slots   一个内部数组
     *  @param  key
     *  @return key对应的键值对，如果不存在则抛出logic_error异常
     */
    entry remove_from_slots(slots_t &slots, const K &key);

    /** 开始渐进rehash */
    void start_rehash();

    /** 结束渐进rehash */
    void finish_rehash();

    /** 对下一个slot执行rehash */
    void rehash_next_slot();

private:
    /** 已存储的总元素数量 */
    int total_entries;

    /** 负载因子 */
    double load_factor;

    /** 内部数组，0是当前数组，1是备用旧数组 */
    slots_t double_slots[2];
    constexpr static int slots_current = 0;
    constexpr static int slots_old = 1;

    /** 是否正处于渐进式rehash过程中 */
    bool is_rehashing;

    /** double_slots[slots_old]尚未rehash的剩余部分slot的起始索引（渐进式rehash进度） */
    int remain_old_index;

    /** hash函数 */
    std::hash<K> hasher;
};


template<typename K, typename V>
hashmap<K, V>::hashmap(int capacity, double load_factor)
    :total_entries(0), load_factor(load_factor), is_rehashing(false), remain_old_index(0) {
    double_slots[slots_current].resize(capacity);
}

template<typename K, typename V>
hashmap<K, V>::~hashmap() {}

template<typename K, typename V>
bool
hashmap<K, V>::contain(const K &key) {
    try {
        this->get_from_slots(this->double_slots[this->is_key_belongs_old(key)], key);
        return true;
    } catch(std::logic_error) {
    }
    return false;
}

template<typename K, typename V>
V&
hashmap<K, V>::get(const K &key) {
    // 如果正在渐进式rehash，那么就做一步rehash
    if (this->is_rehashing) {
        this->rehash_next_slot();
    }

    return this->get_from_slots(this->double_slots[this->is_key_belongs_old(key)], key).value;
}

template<typename K, typename V>
void
hashmap<K, V>::set(const K &key, const V &value) {
    // 如果正在渐进式rehash，那么就做一步rehash
    if (this->is_rehashing) {
        this->rehash_next_slot();
    }

    // 新键值对存入
    this->move_into_slots(this->double_slots[this->is_key_belongs_old(key)], {key, value});
}

template<typename K, typename V>
V
hashmap<K, V>::remove(const K &key) {
    // 如果正在渐进式rehash，那么就做一步rehash
    if (this->is_rehashing) {
        this->rehash_next_slot();
    }

        return this->remove_from_slots(this->double_slots[this->is_key_belongs_old(key)], key).value;
}

template<typename K, typename V>
bool
hashmap<K, V>::is_key_belongs_old(const K &key) {
    // 渐进式rehash过程中，如果key在旧数组中的索引处于remain_old_index之后，那么说明该key还存储在旧数组中
    return this->is_rehashing && (int)(this->hasher(key) % this->double_slots[slots_old].size()) >= this->remain_old_index;
}

template<typename K, typename V>
typename hashmap<K, V>::entry&
hashmap<K, V>::get_from_slots(slots_t &slots, const K &key) {
    auto index = this->hasher(key) % slots.size();
    for (auto &e : slots[index]) {
        if (e.key == key) {
            return e;
        }
    }
    throw std::logic_error("no entry for key");
}

template<typename K, typename V>
void
hashmap<K, V>::move_into_slots(slots_t &slots, entry &&e) {
    auto index = this->hasher(e.key) % slots.size();
    auto &slot = slots[index];
    for (auto &old : slot) {
        if (old.key == e.key) {
            old.value = std::move(e.value);
            return;
        }
    }
    slot.push_front(std::move(e));

    ++this->total_entries;

    // 容量不足的情况下需要触发rehash，但是如果正在rehash就暂时不触发新的。
    if (!this->is_rehashing && this->total_entries >= this->double_slots[slots_current].size() * this->load_factor) {
        this->start_rehash();
    }
}

template<typename K, typename V>
typename hashmap<K, V>::entry
hashmap<K, V>::remove_from_slots(slots_t &slots, const K &key) {
    auto index = this->hasher(key) % slots.size();
    auto &slot = slots[index];
    for (auto iter = slot.begin(); iter != slot.end(); ++iter) {
        if (iter->key == key) {
            auto old = std::move(*iter);
            slot.erase(iter);
            --this->total_entries;
            return old;
        }
    }
    throw std::logic_error("no entry for key");
}

template<typename K, typename V>
void
hashmap<K, V>::start_rehash() {
    this->is_rehashing = true;

    // 把当前数组全部内容全都移动到旧数组中以待rehash
    this->double_slots[slots_old] = std::move(this->double_slots[slots_current]);
    this->remain_old_index = 0;

    // 当前数组扩容
    this->double_slots[slots_current].resize(this->total_entries * 2);
}

template<typename K, typename V>
void
hashmap<K, V>::finish_rehash() {
    this->is_rehashing = false;
    this->double_slots[slots_old] = slots_t(); // 彻底释放vector
    this->remain_old_index = 0;
}

template<typename K, typename V>
void
hashmap<K, V>::rehash_next_slot() {
    for (; this->remain_old_index < (int)this->double_slots[slots_old].size(); ++this->remain_old_index) {
        if (!this->double_slots[slots_old][this->remain_old_index].empty()) {
            break;
        }
    }

    if (this->remain_old_index < (int)this->double_slots[slots_old].size()) {
        auto& slot = this->double_slots[slots_old][this->remain_old_index];
        // 把一整个slot中的所有元素全部rehash到新的当前数组中
        for (auto &e : slot) {
            this->move_into_slots(this->double_slots[slots_current], std::move(e));
        }
        slot.clear();

        ++this->remain_old_index;
    } else {
        // 旧数组已经全部rehash完毕
        this->finish_rehash();
    }
}
