#pragma once

#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

namespace bh::sim {

// Uniform-grid spatial hash for entity queries (AoI, melee range, etc.).
// Cell size in tiles; positions are tile-space ints. Header-only — used by
// the server hot loop and exercised directly by tests.
template <int CellSize = 8>
class SpatialGrid {
 public:
  void insert(std::uint32_t id, int x, int y) {
    remove(id);
    pos_[id] = {x, y};
    cells_[key(x, y)].push_back(id);
  }

  void remove(std::uint32_t id) {
    const auto it = pos_.find(id);
    if (it == pos_.end()) return;
    auto& cell = cells_[key(it->second.first, it->second.second)];
    for (size_t i = 0; i < cell.size(); ++i) {
      if (cell[i] == id) {
        cell[i] = cell.back();
        cell.pop_back();
        break;
      }
    }
    pos_.erase(it);
  }

  void move(std::uint32_t id, int x, int y) {
    const auto it = pos_.find(id);
    if (it == pos_.end()) {
      insert(id, x, y);
      return;
    }
    if (key(it->second.first, it->second.second) == key(x, y)) {
      it->second = {x, y};
      return;
    }
    remove(id);
    insert(id, x, y);
  }

  // All ids in cells intersecting the tile-space rect (inclusive).
  // Callers refine by exact distance.
  std::vector<std::uint32_t> query(int x0, int y0, int x1, int y1) const {
    std::vector<std::uint32_t> out;
    const int cx0 = floorDiv(x0, CellSize);
    const int cx1 = floorDiv(x1, CellSize);
    const int cy0 = floorDiv(y0, CellSize);
    const int cy1 = floorDiv(y1, CellSize);
    for (int cy = cy0; cy <= cy1; ++cy) {
      for (int cx = cx0; cx <= cx1; ++cx) {
        const auto it = cells_.find(keyAt(cx, cy));
        if (it == cells_.end()) continue;
        out.insert(out.end(), it->second.begin(), it->second.end());
      }
    }
    return out;
  }

  size_t size() const { return pos_.size(); }

 private:
  static int floorDiv(int v, int d) {
    return v >= 0 ? v / d : -((-v + d - 1) / d);
  }
  static std::int64_t keyAt(int cx, int cy) {
    return (static_cast<std::int64_t>(cx) << 32) ^
           static_cast<std::uint32_t>(cy);
  }
  static std::int64_t key(int x, int y) {
    return keyAt(floorDiv(x, CellSize), floorDiv(y, CellSize));
  }

  std::unordered_map<std::int64_t, std::vector<std::uint32_t>> cells_;
  std::unordered_map<std::uint32_t, std::pair<int, int>> pos_;
};

}  // namespace bh::sim
