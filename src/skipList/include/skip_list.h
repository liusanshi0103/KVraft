#pragma once

#include <random>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

template <typename K, typename V>
class SkipList {
 private:
  struct Node {
    K key;
    V value;
    std::vector<Node*> forward;

    Node(int level, const K& k = K{}, const V& v = V{})
        : key(k), value(v), forward(level + 1, nullptr) {}
  };

 public:
  explicit SkipList(int max_level)
      : max_level_(max_level),
        current_level_(0),
        element_count_(0),
        header_(new Node(max_level)) {}

  ~SkipList() {
    Clear();
    delete header_;
  }

  SkipList(const SkipList&) = delete;
  SkipList& operator=(const SkipList&) = delete;

  bool search_element(const K& key, V& value) const {
    Node* cur = header_;

    for (int i = current_level_; i >= 0; --i) {
      while (cur->forward[i] && cur->forward[i]->key < key) {
        cur = cur->forward[i];
      }
    }

    cur = cur->forward[0];

    if (cur && cur->key == key) {
      value = cur->value;
      return true;
    }

    return false;
  }

  int insert_element(const K& key, const V& value) {
    std::vector<Node*> update(max_level_ + 1, nullptr);
    Node* cur = header_;

    for (int i = current_level_; i >= 0; --i) {
      while (cur->forward[i] && cur->forward[i]->key < key) {
        cur = cur->forward[i];
      }
      update[i] = cur;
    }

    cur = cur->forward[0];

    if (cur && cur->key == key) {
      return 1;
    }

    int level = RandomLevel();

    if (level > current_level_) {
      for (int i = current_level_ + 1; i <= level; ++i) {
        update[i] = header_;
      }
      current_level_ = level;
    }

    Node* node = new Node(level, key, value);

    for (int i = 0; i <= level; ++i) {
      node->forward[i] = update[i]->forward[i];
      update[i]->forward[i] = node;
    }

    ++element_count_;
    return 0;
  }

  void insert_set_element(const K& key, const V& value) {
    delete_element(key);
    insert_element(key, value);
  }

  void delete_element(const K& key) {
    std::vector<Node*> update(max_level_ + 1, nullptr);
    Node* cur = header_;

    for (int i = current_level_; i >= 0; --i) {
      while (cur->forward[i] && cur->forward[i]->key < key) {
        cur = cur->forward[i];
      }
      update[i] = cur;
    }

    cur = cur->forward[0];

    if (!cur || cur->key != key) {
      return;
    }

    for (int i = 0; i <= current_level_; ++i) {
      if (update[i]->forward[i] != cur) {
        break;
      }
      update[i]->forward[i] = cur->forward[i];
    }

    delete cur;
    --element_count_;

    while (current_level_ > 0 && header_->forward[current_level_] == nullptr) {
      --current_level_;
    }
  }

  int size() const {
    return element_count_;
  }

  std::string dump_file() const {
    std::string data;

    data += std::to_string(element_count_);
    data += "\n";

    Node* cur = header_->forward[0];

    while (cur) {
      AppendString(&data, ToString(cur->key));
      AppendString(&data, ToString(cur->value));
      cur = cur->forward[0];
    }

    return data;
  }

  void load_file(const std::string& dump) {
    Clear();

    if (dump.empty()) {
      return;
    }

    size_t pos = 0;
    std::string line;

    if (!ReadLine(dump, &pos, &line)) {
      return;
    }

    int count = std::stoi(line);

    for (int i = 0; i < count; ++i) {
      std::string key_str;
      std::string value_str;

      if (!ReadString(dump, &pos, &key_str)) {
        return;
      }

      if (!ReadString(dump, &pos, &value_str)) {
        return;
      }

      K key = FromString<K>(key_str);
      V value = FromString<V>(value_str);

      insert_element(key, value);
    }
  }

 private:
  int RandomLevel() const {
    static thread_local std::mt19937 gen(std::random_device{}());
    static thread_local std::uniform_int_distribution<int> dist(0, 1);

    int level = 0;
    while (dist(gen) == 1 && level < max_level_) {
      ++level;
    }

    return level;
  }

  void Clear() {
    Node* cur = header_->forward[0];

    while (cur) {
      Node* next = cur->forward[0];
      delete cur;
      cur = next;
    }

    for (auto& p : header_->forward) {
      p = nullptr;
    }

    current_level_ = 0;
    element_count_ = 0;
  }

  static void AppendString(std::string* out, const std::string& value) {
    *out += std::to_string(value.size());
    *out += "\n";
    *out += value;
    *out += "\n";
  }

  static bool ReadLine(const std::string& data, size_t* pos, std::string* line) {
    size_t end = data.find('\n', *pos);
    if (end == std::string::npos) {
      return false;
    }

    *line = data.substr(*pos, end - *pos);
    *pos = end + 1;
    return true;
  }

  static bool ReadString(const std::string& data, size_t* pos, std::string* value) {
    std::string size_line;
    if (!ReadLine(data, pos, &size_line)) {
      return false;
    }

    int size = std::stoi(size_line);
    if (*pos + size > data.size()) {
      return false;
    }

    *value = data.substr(*pos, size);
    *pos += size;

    if (*pos < data.size() && data[*pos] == '\n') {
      ++(*pos);
    }

    return true;
  }

  template <typename T>
  static std::string ToString(const T& value) {
    if constexpr (std::is_same_v<T, std::string>) {
      return value;
    } else {
      std::ostringstream oss;
      oss << value;
      return oss.str();
    }
  }

  template <typename T>
  static T FromString(const std::string& value) {
    if constexpr (std::is_same_v<T, std::string>) {
      return value;
    } else {
      std::istringstream iss(value);
      T result{};
      iss >> result;
      return result;
    }
  }

 private:
  int max_level_;
  int current_level_;
  int element_count_;
  Node* header_;
};