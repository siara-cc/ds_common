#ifndef _FREQ_GRP_HPP_
#define _FREQ_GRP_HPP_

#include <math.h>
#include <vector>

namespace gen {

const size_t freq_byt_caps[] = {64, 64 + 8192, 64 + 8192 + 1048576, 64 + 8192 + 1048576 + 134217728,
                                64 + 8192 + 1048576 + 134217728 + 17179869184};
class freq_grp {
  private:
    size_t total_freq;
    size_t cur_freq_sum;
    int cur_grp;
    int cur_count;
    int cur_size;
    std::vector<uint32_t> freq_grps;
    std::vector<uint32_t> freq_counts;
    std::vector<uint32_t> freq_sizes;
  public:
    freq_grp(size_t _total_freq) : total_freq (_total_freq) {
      cur_freq_sum = 0;
      cur_grp = 0;
      cur_count = 0;
      cur_size = 0;
    }
    int add_freq(uint32_t freq, int len) {
      cur_freq_sum += freq;
      cur_count++;
      cur_size += len;
      if (cur_count >= freq_byt_caps[cur_grp]) {
        freq_grps.push_back(cur_freq_sum);
        freq_counts.push_back(cur_count);
        freq_sizes.push_back(cur_size);
        cur_grp++;
        cur_freq_sum = 0;
        cur_size = 0;
        return cur_grp;
      }
      return cur_grp + 1;
    }
    std::vector<uint32_t> *get_freq_grps() {
      if (cur_grp >= freq_grps.size()) {
        freq_grps.push_back(cur_freq_sum);
        freq_counts.push_back(cur_count);
        freq_sizes.push_back(cur_size);
      }
      return &freq_grps;
    }
    std::vector<uint32_t> *get_freq_counts() {
      if (cur_grp >= freq_grps.size()) {
        freq_grps.push_back(cur_freq_sum);
        freq_counts.push_back(cur_count);
        freq_sizes.push_back(cur_size);
      }
      return &freq_counts;
    }
    std::vector<uint32_t> *get_freq_sizes() {
      if (cur_grp >= freq_grps.size()) {
        freq_grps.push_back(cur_freq_sum);
        freq_counts.push_back(cur_count);
        freq_sizes.push_back(cur_size);
      }
      return &freq_sizes;
    }
};

}

#endif
