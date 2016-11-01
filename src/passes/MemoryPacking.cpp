/*
 * Copyright 2016 WebAssembly Community Group participants
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <wasm.h>
#include <pass.h>
#include <wasm-builder.h>

namespace wasm {

// Smaller than this is not worth it - adding segments adds overhead too
const Index MIN_SEGMENT = 16;

struct MemoryPacking : public Pass {
  void run(PassRunner* runner, Module* module) override {
    if (!module->memory.exists) return;
    std::vector<Memory::Segment> packed;
    for (auto& segment : module->memory.segments) {
      // skip final zeros
      while (segment.data.size() > 0 && segment.data.back() == 0) {
        segment.data.pop_back();
      }
      // we can only handle a constant offset for splitting
      if (auto* offset = segment.offset->dynCast<Const>()) {
        // Find runs of zeros, and split
        auto& data = segment.data;
        auto base = offset->value.geti32();
        Index start = 0;
        while (start < data.size()) {
          while (start < data.size() && data[start] == 0) {
            start++;
          }
          // find the end of the data-containing part before us
          Index end = start;
          while (end < data.size() && data[end] != 0) {
            end++;
          }
          // skip zeros to find where the next segment is
          Index next = end;
          while (next < data.size() && (next - start < MIN_SEGMENT || data[next] == 0)) {
            if (data[next]) end = next + 1;
            next++;
          }
          if (end != start) {
            packed.emplace_back(Builder(*module).makeConst(Literal(int32_t(base + start))), &data[start], end - start);
          }
          start = next;
        }
      } else {
        packed.push_back(segment);
      }
    }
    module->memory.segments.swap(packed);
  }
};

Pass *createMemoryPackingPass() {
  return new MemoryPacking();
}

} // namespace wasm

