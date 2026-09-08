#include "reassembler.hh"
#include "debug.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // parse current state and data 
  // uint64_t left_index = output_.reader().bytes_popped();
  uint64_t right_index = next_index + output_.writer().available_capacity();
  uint64_t final_index = first_index + data.size();  

  uint64_t accept_first_index = max(next_index, first_index);
  uint64_t accept_final_index = min(right_index, final_index);

  if(accept_first_index < accept_final_index){
    uint64_t accept_size = accept_final_index - accept_first_index;
    std::string accept_data = data.substr(accept_first_index-first_index, accept_size);

    // hold the data in pending first
    auto it = pending.begin();
    while (it != pending.end()) {
      uint64_t prev_first_index = it->first;
      uint64_t prev_final_index = prev_first_index + it->second.size();

      if (prev_final_index < accept_first_index){
        ++it;
        continue;
      }
      if (prev_first_index > accept_final_index){
        break;
      }
      // merge overlap or adjacent data segment
      uint64_t merged_first_index = min(accept_first_index, prev_first_index);
      uint64_t merged_final_index = max(accept_final_index, prev_final_index);

      std::string merged(merged_final_index - merged_first_index, '\0');
      merged.replace(prev_first_index - merged_first_index,it->second.size(),it->second);
      merged.replace(accept_first_index - merged_first_index,accept_data.size(),accept_data);

      accept_first_index = merged_first_index;
      accept_final_index = merged_final_index;
      accept_data = std::move(merged);
      it = pending.erase(it);
    }
    pending.insert({accept_first_index, accept_data});
  }
  // push segments that meets condition
  while(!pending.empty() && pending.begin()->first == next_index){
    output_.writer().push(pending.begin()->second);
    next_index += pending.begin()->second.size();
    pending.erase(pending.begin());
  }
  // record eof state
  if(is_last_substring){
    eof_index = first_index + data.size();
  }
  if(eof_index.has_value() && next_index == eof_index.value()){
    output_.writer().close();
  }
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  uint64_t reassembler_size = 0;
  for(const auto& segment : pending){
    reassembler_size += segment.second.size();
  }

  return reassembler_size;
}
