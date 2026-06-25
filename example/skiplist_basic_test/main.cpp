#include <iostream>
#include <string>

#include "skip_list.h"

int main() {
  SkipList<std::string, std::string> db(6);

  db.insert_set_element("name", "alice");

  std::string value;
  if (!db.search_element("name", value) || value != "alice") {
    std::cerr << "search failed, value=" << value << std::endl;
    return 1;
  }

  db.insert_set_element("name", "alice-bob");

  value.clear();
  if (!db.search_element("name", value) || value != "alice-bob") {
    std::cerr << "overwrite failed, value=" << value << std::endl;
    return 1;
  }

  std::string dump = db.dump_file();

  SkipList<std::string, std::string> restored(6);
  restored.load_file(dump);

  value.clear();
  if (!restored.search_element("name", value) || value != "alice-bob") {
    std::cerr << "load_file failed, value=" << value << std::endl;
    return 1;
  }

  std::cout << "skiplist basic test passed" << std::endl;
  return 0;
}