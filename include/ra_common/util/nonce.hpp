#pragma once

#include <algorithm>
#include <deque>
#include <string>
#include <unordered_set>

namespace ra::common {

/// Tracks recently-seen ids and rejects duplicates. Ports ra.common.Nonce.
class Nonce {
public:
    explicit Nonce(size_t max_size = 1'000'000, int prune_percent = 10)
        : max_size_(max_size), prune_percent_(std::clamp(prune_percent, 0, 100)) {}

    /// Register `id`. Returns true if new, false if a replay.
    bool ContinueOn(const std::string& id) {
        Prune();
        if (!seen_.insert(id).second) return false;
        order_.push_back(id);
        return true;
    }

    size_t Size() const { return order_.size(); }

private:
    void Prune() {
        if (order_.size() <= max_size_) return;
        if (prune_percent_ == 100) {
            seen_.clear();
            order_.clear();
            return;
        }
        size_t to_prune = max_size_ * static_cast<size_t>(prune_percent_) / 100;
        for (size_t i = 0; i < to_prune && !order_.empty(); i++) {
            seen_.erase(order_.front());
            order_.pop_front();
        }
    }

    std::unordered_set<std::string> seen_;
    std::deque<std::string> order_;
    size_t max_size_;
    int prune_percent_;
};

}  // namespace ra::common
