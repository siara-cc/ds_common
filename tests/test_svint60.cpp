#include <vint.hpp>
#include <stdint.h>
#include <vector>

#define TIMES 10000000
int main() {

std::vector<uint8_t> values;

for (size_t i = 0; i < TIMES; i++) {
  gen::append_svint61(values, i);
}

size_t id_len = 0;
size_t cur_pos = 0;
for (size_t i = 0; i < TIMES; i++) {
  size_t val = gen::read_svint61(values.data() + cur_pos);
  cur_pos += gen::read_svint61_len(values.data() + cur_pos);
  if (i != val)
    printf("Mismatch: %lu, %lu\n", i, val);
}

printf("done\n");

}

